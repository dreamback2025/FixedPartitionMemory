#include "os_types.h"
#include "log.h"
#include "partition.h"
#include "config.h"
#include "process.h"  // 包含process.h

// 预定义固定分区大小 - 创建8个固定分区 (总和896字节)
static const uint32_t FIXED_PARTITION_SIZES[] = {128, 128, 128, 128, 96, 96, 96, 96};
#define FIXED_PARTITION_COUNT (sizeof(FIXED_PARTITION_SIZES) / sizeof(FIXED_PARTITION_SIZES[0]))

partition_t partition_table[MAX_PARTITIONS];
uint32_t partition_count = 0;

// 分区初始化 - 固定分区分配系统
void partition_init(void) {
    // 重置分区表
    for (uint32_t i = 0; i < MAX_PARTITIONS; i++) {
        partition_table[i].state = PARTITION_FREE;
        partition_table[i].owner_pid = 0;
    }

    // 创建操作系统分区
    partition_table[0].start = 0;
    partition_table[0].size = OS_PARTITION_SIZE;
    partition_table[0].state = PARTITION_OS;
    partition_table[0].owner_pid = 0;
    
    partition_count = 1;  // 从索引1开始放置用户分区

    // 创建固定分区 - 从内存的剩余部分开始分配
    uint32_t current_addr = OS_PARTITION_SIZE;

    // 按预定义大小创建固定分区
    for (uint32_t size_idx = 0; size_idx < FIXED_PARTITION_COUNT && partition_count < MAX_PARTITIONS; size_idx++) {
        uint32_t partition_size = FIXED_PARTITION_SIZES[size_idx];
        
        if (current_addr + partition_size <= MEMORY_SIZE) {
            partition_table[partition_count].start = current_addr;
            partition_table[partition_count].size = partition_size;
            partition_table[partition_count].state = PARTITION_FREE;
            partition_table[partition_count].owner_pid = 0;
            
            current_addr += partition_size;
            partition_count++;
        }
    }

    DEBUG_PRINT("Fixed partition table initialized with %d partitions", partition_count);
    dump_memory_map();
}

// 查找空闲分区 - 固定分区系统需要找到大小合适的分区
partition_t* find_free_partition(uint32_t size) {
    partition_t* best_fit = NULL;
    uint32_t best_fit_diff = 0xFFFFFFFF;
    
    // 在固定分区系统中，我们寻找能够容纳指定大小的最小合适分区
    for (uint32_t i = 1; i < partition_count; i++) {
        if (partition_table[i].state == PARTITION_FREE && partition_table[i].size >= size) {
            // 寻找最佳匹配（最小的足够大的分区）
            uint32_t diff = partition_table[i].size - size;
            if (diff < best_fit_diff) {
                best_fit_diff = diff;
                best_fit = &partition_table[i];
            }
        }
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

    // 分配分区
    part->state = PARTITION_ALLOCATED;
    part->owner_pid = proc->pid;
    proc->memory_start = part->start;
    proc->memory_end = part->start + part->size - 1;

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

    // 释放分区
    part->state = PARTITION_FREE;
    part->owner_pid = 0;
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
    
    // 显示操作系统分区
    const char* state_str_os;
    switch (partition_table[0].state) {
        case PARTITION_FREE: state_str_os = "FREE"; break;
        case PARTITION_ALLOCATED: state_str_os = "ALLOC"; break;
        case PARTITION_OS: state_str_os = "OS"; break;
        default: state_str_os = "UNK";
    }
    
    kernel_log(LOG_INFO, "0x%04x   0x%04x   %4d     %-5s    %d",
        partition_table[0].start,
        partition_table[0].start + partition_table[0].size - 1,
        partition_table[0].size,
        state_str_os,
        partition_table[0].owner_pid);

    // 显示所有固定分区
    for (uint32_t i = 1; i < partition_count; i++) {
        const char* state_str;
        switch (partition_table[i].state) {
            case PARTITION_FREE: state_str = "FREE"; break;
            case PARTITION_ALLOCATED: state_str = "ALLOC"; break;
            case PARTITION_OS: state_str = "OS"; break;
            default: state_str = "UNK";
        }

        kernel_log(LOG_INFO, "0x%04x   0x%04x   %4d     %-5s    %d",
            partition_table[i].start,
            partition_table[i].start + partition_table[i].size - 1,
            partition_table[i].size,
            state_str,
            partition_table[i].owner_pid);
    }
}