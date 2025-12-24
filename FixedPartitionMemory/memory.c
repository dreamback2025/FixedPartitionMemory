#include "os_types.h"
#include "log.h"
#include "memory.h"
#include "process.h"
#include "partition.h"
#include "config.h"


extern partition_t* free_list;
extern partition_t* allocated_list;
extern process_t process_table[MAX_PROCESSES];

allocation_strategy_t current_strategy = BEST_FIT;  // 默认策略


// 内存初始化
void memory_init(void) {
    current_strategy = DEFAULT_ALLOCATION_STRATEGY;
    DEBUG_PRINT("Memory manager initialized with strategy %d", current_strategy);
}

// 分配内存
int allocate_memory(process_t* proc, allocation_strategy_t strategy) {
    if (!proc || proc->memory_size == 0) {
        return -1;
    }

    DEBUG_PRINT("Allocating memory for PID=%d, Size=%d, Strategy=%d",
        proc->pid, proc->memory_size, strategy);

    partition_t* best_fit = NULL;
    partition_t* first_fit = NULL;
    partition_t* worst_fit = NULL;

    // 检查空闲分区
    partition_t* current = free_list;
    while (current) {
        if (current->state == PARTITION_FREE && current->size >= proc->memory_size) {
            // 首次适应
            if (!first_fit) {
                first_fit = current;
            }

            // 最佳适应
            if (!best_fit || current->size < best_fit->size) {
                best_fit = current;
            }

            // 最坏适应
            if (!worst_fit || current->size > worst_fit->size) {
                worst_fit = current;
            }
        }
        current = current->next;
    }

    // 根据策略选择分区
    partition_t* selected = NULL;
    switch (strategy) {
    case FIRST_FIT: selected = first_fit; break;
    case BEST_FIT: selected = best_fit; break;
    case WORST_FIT: selected = worst_fit; break;
    }

    if (!selected) {
        kernel_log(LOG_WARNING, "No suitable partition for PID=%d (size=%d)",
            proc->pid, proc->memory_size);
        return -1;
    }

    // 分配分区
    if (allocate_partition(selected, proc) != 0) {
        return -1;
    }

    return 0;
}

// 释放内存
void free_memory(process_t* proc) {
    if (!proc || proc->state == PROC_TERMINATED) {
        return;
    }

    DEBUG_PRINT("Freeing memory for PID=%d", proc->pid);

    // 查找对应的分区
    partition_t* current = allocated_list;
    while (current) {
        if (current->owner_pid == proc->pid) {
            free_partition(current);
            break;
        }
        current = current->next;
    }

    // 终止进程
    terminate_process(proc);
}

// 紧凑内存 (内存整理)
void compact_memory(void) {
    kernel_log(LOG_INFO, "Compacting memory to reduce fragmentation");

    // 简化版紧凑算法：
    // 1. 终止所有进程
    // 2. 释放所有分区
    // 3. 创建一个大的空闲分区

    // 终止所有进程
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROC_TERMINATED) {
            terminate_process(&process_table[i]);
        }
    }

    // 重建分区表
    partition_table[0].start = 0;
    partition_table[0].size = OS_PARTITION_SIZE;
    partition_table[0].state = PARTITION_OS;
    partition_table[0].owner_pid = 0;
    partition_table[0].next = &partition_table[1];

    partition_table[1].start = OS_PARTITION_SIZE;
    partition_table[1].size = MEMORY_SIZE - OS_PARTITION_SIZE;
    partition_table[1].state = PARTITION_FREE;
    partition_table[1].owner_pid = 0;
    partition_table[1].next = NULL;

    // 重建链表
    free_list = &partition_table[1];
    allocated_list = &partition_table[0];

    kernel_log(LOG_INFO, "Memory compacted successfully");
    dump_memory_map();
}

// 获取总空闲内存
uint32_t get_total_free_memory(void) {
    uint32_t total = 0;
    partition_t* current = free_list;
    while (current) {
        if (current->state == PARTITION_FREE) {
            total += current->size;
        }
        current = current->next;
    }
    return total;
}

// 获取最大空闲块
uint32_t get_largest_free_block(void) {
    uint32_t largest = 0;
    partition_t* current = free_list;
    while (current) {
        if (current->state == PARTITION_FREE && current->size > largest) {
            largest = current->size;
        }
        current = current->next;
    }
    return largest;
}

// 转储内存统计
void dump_memory_statistics(void) {
    uint32_t total_free = get_total_free_memory();
    uint32_t largest_block = get_largest_free_block();
    uint32_t total_used = MEMORY_SIZE - OS_PARTITION_SIZE - total_free;

    kernel_log(LOG_INFO, "Memory Statistics:");
    kernel_log(LOG_INFO, "  Total Memory: %d bytes", MEMORY_SIZE);
    kernel_log(LOG_INFO, "  OS Memory: %d bytes", OS_PARTITION_SIZE);
    kernel_log(LOG_INFO, "  Total Free: %d bytes (%.1f%%)",
        total_free, (float)total_free * 100 / (MEMORY_SIZE - OS_PARTITION_SIZE));
    kernel_log(LOG_INFO, "  Total Used: %d bytes (%.1f%%)",
        total_used, (float)total_used * 100 / (MEMORY_SIZE - OS_PARTITION_SIZE));
    kernel_log(LOG_INFO, "  Largest Free Block: %d bytes", largest_block);
    kernel_log(LOG_INFO, "  External Fragmentation: %.1f%%",
        (largest_block > 0) ? (float)(total_free - largest_block) * 100 / total_free : 0.0f);
}