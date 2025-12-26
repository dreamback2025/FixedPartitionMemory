#include "os_types.h"
#include "log.h"
#include "partition.h"
#include "config.h"
#include "process.h"  // 包含process.h

// 预定义固定分区大小
static const uint32_t FIXED_PARTITION_SIZES[] = {64, 128, 256, 512};
#define FIXED_PARTITION_COUNT (sizeof(FIXED_PARTITION_SIZES) / sizeof(FIXED_PARTITION_SIZES[0]))

partition_t partition_table[MAX_PARTITIONS];
partition_t* free_list = NULL;
partition_t* allocated_list = NULL;

// 分区初始化 - 固定分区分配系统
void partition_init(void) {
    // 重置分区表
    for (uint32_t i = 0; i < MAX_PARTITIONS; i++) {
        partition_table[i].state = PARTITION_FREE;
        partition_table[i].next = NULL;
    }

    // 创建操作系统分区
    partition_table[0].start = 0;
    partition_table[0].size = OS_PARTITION_SIZE;
    partition_table[0].state = PARTITION_OS;
    partition_table[0].owner_pid = 0;

    // 创建固定分区 - 从内存的剩余部分开始分配
    uint32_t current_addr = OS_PARTITION_SIZE;
    uint32_t partition_idx = 1;

    // 为每种大小分配一定比例的 partitions
    // 为了更合理的分配，我们确保每种大小至少有一个分区
    uint32_t remaining_memory = MEMORY_SIZE - OS_PARTITION_SIZE;
    
    // 首先为每种大小分配至少一个分区 to ensure larger processes can be accommodated
    for (uint32_t size_idx = 0; size_idx < FIXED_PARTITION_COUNT && partition_idx < MAX_PARTITIONS - 1; size_idx++) {
        uint32_t partition_size = FIXED_PARTITION_SIZES[size_idx];
        
        if (current_addr + partition_size <= MEMORY_SIZE) {
            partition_table[partition_idx].start = current_addr;
            partition_table[partition_idx].size = partition_size;
            partition_table[partition_idx].state = PARTITION_FREE;
            partition_table[partition_idx].owner_pid = 0;
            partition_table[partition_idx].next = &partition_table[partition_idx + 1];
            
            current_addr += partition_size;
            partition_idx++;
        }
    }
    
    // Now distribute the remaining memory proportionally
    while (partition_idx < MAX_PARTITIONS - 1 && current_addr < MEMORY_SIZE) {
        // Cycle through partition sizes to create more of each type
        BOOL added_partition = FALSE;
        for (uint32_t size_idx = 0; size_idx < FIXED_PARTITION_COUNT && !added_partition && 
             partition_idx < MAX_PARTITIONS - 1 && current_addr < MEMORY_SIZE; size_idx++) {
            uint32_t partition_size = FIXED_PARTITION_SIZES[size_idx];
            
            if (current_addr + partition_size <= MEMORY_SIZE) {
                partition_table[partition_idx].start = current_addr;
                partition_table[partition_idx].size = partition_size;
                partition_table[partition_idx].state = PARTITION_FREE;
                partition_table[partition_idx].owner_pid = 0;
                partition_table[partition_idx].next = &partition_table[partition_idx + 1];
                
                current_addr += partition_size;
                partition_idx++;
                added_partition = TRUE;
            }
        }
        
        // If no partition could be added (remaining memory is too small for any partition size)
        if (!added_partition) {
            break;
        }
    }
    
    if (partition_idx > 0) {
        partition_table[partition_idx - 1].next = NULL;
    }

    // 初始化全局指针 - 遍历分区表建立free_list和allocated_list
    free_list = NULL;
    allocated_list = &partition_table[0];  // OS分区是已分配的
    
    // 建立空闲分区链表
    for (uint32_t i = 1; i < partition_idx; i++) {
        if (partition_table[i].state == PARTITION_FREE) {
            if (free_list == NULL) {
                free_list = &partition_table[i];
            }
        }
    }
    
    // 重新建立空闲分区链表（按地址排序）
    partition_t* current_free = NULL;
    for (uint32_t i = 1; i < partition_idx; i++) {
        if (partition_table[i].state == PARTITION_FREE) {
            if (current_free == NULL) {
                free_list = &partition_table[i];
                current_free = free_list;
            } else {
                current_free->next = &partition_table[i];
                current_free = current_free->next;
            }
        }
    }
    if (current_free) {
        current_free->next = NULL;
    }

    DEBUG_PRINT("Fixed partition table initialized with %d partitions", partition_idx);
    dump_memory_map();
}

// 查找空闲分区 - 固定分区系统需要找到大小合适的分区
partition_t* find_free_partition(uint32_t size) {
    partition_t* current = free_list;
    partition_t* best_fit = NULL;
    
    // 在固定分区系统中，我们寻找能够容纳指定大小的最小合适分区
    while (current) {
        if (current->state == PARTITION_FREE && current->size >= size) {
            // 寻找最佳匹配（最小的足够大的分区）
            if (!best_fit || current->size < best_fit->size) {
                best_fit = current;
            }
        }
        current = current->next;
    }
    
    return best_fit;
}

// 分配分区 - 固定分区系统
int allocate_partition(partition_t* part, process_t* proc) {
    if (!part || !proc || part->state != PARTITION_FREE) {
        return -1;
    }

    // 检查分区是否足够大
    if (part->size < proc->memory_size) {
        kernel_log(LOG_WARNING, "Partition size %d is too small for process memory size %d", 
                   part->size, proc->memory_size);
        return -1;
    }

    // 保存下一个分区指针
    partition_t* next_partition = part->next;

    // 分配分区
    part->state = PARTITION_ALLOCATED;
    part->owner_pid = proc->pid;
    proc->memory_start = part->start;
    proc->memory_end = part->start + part->size - 1;

    // 从空闲列表移除
    if (free_list == part) {
        free_list = part->next;
    }
    else {
        partition_t* prev = free_list;
        while (prev && prev->next != part) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = part->next;
        }
    }

    // 添加到已分配列表
    part->next = allocated_list->next;
    allocated_list->next = part;

    DEBUG_PRINT("Partition allocated: PID=%d, Start=0x%x, Size=%d",
        proc->pid, part->start, part->size);

    return 0;
}

// 释放分区
void free_partition(partition_t* part) {
    if (!part || part->state != PARTITION_ALLOCATED) {
        return;
    }

    DEBUG_PRINT("Freeing partition: Start=0x%x, Size=%d, Owner PID=%d",
        part->start, part->size, part->owner_pid);

    // 从已分配列表移除
    partition_t* prev = allocated_list;
    while (prev && prev->next != part) {
        prev = prev->next;
    }
    if (prev) {
        prev->next = part->next;
    }

    // 释放分区
    part->state = PARTITION_FREE;
    part->owner_pid = 0;

    // 插入到空闲列表 (按地址排序)
    if (!free_list || part->start < free_list->start) {
        part->next = free_list;
        free_list = part;
    }
    else {
        partition_t* current = free_list;
        while (current->next && current->next->start < part->start) {
            current = current->next;
        }
        part->next = current->next;
        current->next = part;
    }

    // 合并相邻空闲分区
    merge_adjacent_free_partitions();
}

// 合并相邻空闲分区 - 在固定分区系统中，这个操作不适用，因为分区大小固定
void merge_adjacent_free_partitions(void) {
    // 固定分区系统中不需要合并，因为分区大小是固定的，不能改变
    // 保留此函数是为了兼容接口
}

// 转储内存映射
void dump_memory_map(void) {
    kernel_log(LOG_INFO, "Memory Map:");
    kernel_log(LOG_INFO, "Start    End      Size     State    Owner");

    partition_t* current = &partition_table[0];
    while (current) {
        const char* state_str;
        switch (current->state) {
        case PARTITION_FREE: state_str = "FREE"; break;
        case PARTITION_ALLOCATED: state_str = "ALLOC"; break;
        case PARTITION_OS: state_str = "OS"; break;
        default: state_str = "UNK";
        }

        kernel_log(LOG_INFO, "0x%04x   0x%04x   %4d     %-5s    %d",
            current->start,
            current->start + current->size - 1,
            current->size,
            state_str,
            current->owner_pid);

        current = current->next;
    }
}