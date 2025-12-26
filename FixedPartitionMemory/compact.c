#include "os_types.h"
#include "log.h"
#include "memory.h"
#include "process.h"
#include "partition.h"
#include "config.h"
#include "kernel.h"

// 声明外部变量
extern process_t process_table[MAX_PROCESSES];
extern uint32_t partition_count;
extern partition_t partition_table[MAX_PARTITIONS];


// 高级紧凑算法 (移动进程数据)
void advanced_compact_memory(void) {
    kernel_log(LOG_INFO, "Performing advanced memory compaction");

    // 在固定分区系统中，紧凑操作有局限性
    // 我们将终止所有用户进程，但保持分区结构不变
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROC_TERMINATED && process_table[i].pid != 0) {
            // 只终止非操作系统进程
            if (process_table[i].pid > 0) {
                terminate_process(&process_table[i]);
            }
        }
    }

    // 释放所有分区
    for (uint32_t i = 1; i < partition_count; i++) {
        if (partition_table[i].state == PARTITION_ALLOCATED) {
            partition_table[i].state = PARTITION_FREE;
            partition_table[i].owner_pid = 0;
        }
    }

    kernel_log(LOG_INFO, "Memory compaction completed - all user processes terminated");
    dump_memory_map();
    dump_memory_statistics();
}