#include "os_types.h"
#include "log.h"
#include "memory.h"
#include "process.h"
#include "partition.h"
#include "config.h"

// 声明外部变量
extern process_t process_table[MAX_PROCESSES];
extern partition_t* free_list;
extern partition_t* allocated_list;


// 高级紧凑算法 (移动进程数据)
void advanced_compact_memory(void) {
    kernel_log(LOG_INFO, "Performing advanced memory compaction");

    // 步骤1: 收集所有活动进程
    process_t* active_processes[MAX_PROCESSES];
    uint32_t active_count = 0;

    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROC_TERMINATED) {
            active_processes[active_count++] = &process_table[i];
        }
    }

    kernel_log(LOG_INFO, "Found %d active processes for compaction", active_count);

    // 步骤2: 按内存地址排序进程
    for (uint32_t i = 0; i < active_count - 1; i++) {
        for (uint32_t j = 0; j < active_count - i - 1; j++) {
            if (active_processes[j]->memory_start > active_processes[j + 1]->memory_start) {
                process_t* temp = active_processes[j];
                active_processes[j] = active_processes[j + 1];
                active_processes[j + 1] = temp;
            }
        }
    }

    // 步骤3: 重新分配内存
    uint32_t current_address = OS_PARTITION_SIZE;
    uint8_t* memory_base = get_memory_base();

    for (uint32_t i = 0; i < active_count; i++) {
        process_t* proc = active_processes[i];
        partition_t* old_partition = NULL;

        // 查找旧分区
        partition_t* current = allocated_list;
        while (current) {
            if (current->owner_pid == proc->pid) {
                old_partition = current;
                break;
            }
            current = current->next;
        }

        if (!old_partition) {
            continue;
        }

        // 保存数据
        uint8_t* old_data = memory_base + old_partition->start;
        uint8_t* new_data = memory_base + current_address;

        // 移动数据 (模拟)
        if (old_partition->start != current_address) {
            DEBUG_PRINT("Moving process %d data from 0x%x to 0x%x",
                proc->pid, old_partition->start, current_address);

            // 在真实系统中，这里会复制进程数据
            // memcpy(new_data, old_data, proc->memory_size);
        }

        // 更新进程内存信息
        proc->memory_start = current_address;
        proc->memory_end = current_address + proc->memory_size - 1;

        // 更新分区信息
        old_partition->start = current_address;
        // old_partition->size 保持不变

        // 推进当前地址
        current_address += proc->memory_size;

        // 对齐
        if (current_address % 4 != 0) {
            current_address += 4 - (current_address % 4);
        }
    }

    // 步骤4: 创建一个新的空闲分区
    if (current_address < MEMORY_SIZE) {
        // 简化：只使用一个空闲分区
        partition_table[1].start = current_address;
        partition_table[1].size = MEMORY_SIZE - current_address;
        partition_table[1].state = PARTITION_FREE;
        partition_table[1].owner_pid = 0;

        // 更新链表
        free_list = &partition_table[1];
        partition_table[1].next = NULL;
    }
    else {
        // 无空闲内存
        free_list = NULL;
    }

    kernel_log(LOG_INFO, "Advanced memory compaction completed");
    dump_memory_map();
    dump_memory_statistics();
}