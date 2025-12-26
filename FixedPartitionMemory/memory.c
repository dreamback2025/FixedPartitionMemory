#include "os_types.h"
#include "log.h"
#include "memory.h"
#include "process.h"  // 包含process.h
#include "partition.h"
#include "config.h"


extern uint32_t partition_count;
extern partition_t partition_table[MAX_PARTITIONS];
extern process_t process_table[MAX_PROCESSES];

allocation_strategy_t current_strategy = BEST_FIT;  // 默认策略


// 内存初始化
void memory_init(void) {
    current_strategy = DEFAULT_ALLOCATION_STRATEGY;
    DEBUG_PRINT("Memory manager initialized with strategy %d", current_strategy);
}

// 分配内存 - 固定分区系统
int allocate_memory(process_t* proc, allocation_strategy_t strategy) {
    if (!proc || proc->memory_size == 0) {
        return -1;
    }

    DEBUG_PRINT("Allocating memory for PID=%d, Size=%d, Strategy=%d",
        proc->pid, proc->memory_size, strategy);

    // 在固定分区系统中，我们只寻找合适大小的分区
    partition_t* selected = find_free_partition(proc->memory_size);

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
    for (uint32_t i = 1; i < partition_count; i++) {
        if (partition_table[i].owner_pid == proc->pid) {
            free_partition(&partition_table[i]);
            break;
        }
    }

    // 终止进程
    terminate_process(proc);
}

// 紧凑内存 (内存整理) - 在固定分区系统中，紧凑操作有限
void compact_memory(void) {
    kernel_log(LOG_INFO, "Compacting memory in fixed partition system");

    // 在固定分区系统中，紧凑操作的含义与可变分区不同
    // 我们将终止所有用户进程，但保持分区结构不变
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROC_TERMINATED && process_table[i].pid != 0) {
            // 只终止非操作系统进程
            if (process_table[i].pid > 0) {
                terminate_process(&process_table[i]);
            }
        }
    }

    kernel_log(LOG_INFO, "Memory compaction completed - all user processes terminated");
    dump_memory_map();
}

// 获取总空闲内存
uint32_t get_total_free_memory(void) {
    uint32_t total = 0;
    for (uint32_t i = 1; i < partition_count; i++) {
        if (partition_table[i].state == PARTITION_FREE) {
            total += partition_table[i].size;
        }
    }
    return total;
}

// 获取最大空闲块
uint32_t get_largest_free_block(void) {
    uint32_t largest = 0;
    for (uint32_t i = 1; i < partition_count; i++) {
        if (partition_table[i].state == PARTITION_FREE && partition_table[i].size > largest) {
            largest = partition_table[i].size;
        }
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