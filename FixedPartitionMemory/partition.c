#include "os_types.h"
#include "log.h"
#include "partition.h"
#include "config.h"
#include "process.h"



partition_t partition_table[MAX_PARTITIONS];
partition_t* free_list = NULL;
partition_t* allocated_list = NULL;
process_t process_table[MAX_PROCESSES];

// 分区初始化
void partition_init(void) {
    // 重置分区表
    for (uint32_t i = 0; i < MAX_PARTITIONS; i++) {
        partition_table[i].state = PARTITION_FREE;
        partition_table[i].next = NULL;
    }

    // 创建初始分区
    partition_table[0].start = 0;
    partition_table[0].size = OS_PARTITION_SIZE;
    partition_table[0].state = PARTITION_OS;
    partition_table[0].owner_pid = 0;

    partition_table[1].start = OS_PARTITION_SIZE;
    partition_table[1].size = MEMORY_SIZE - OS_PARTITION_SIZE;
    partition_table[1].state = PARTITION_FREE;
    partition_table[1].owner_pid = 0;

    // 建立正确的链表结构
    partition_table[0].next = &partition_table[1];
    partition_table[1].next = NULL;

    // 初始化全局指针
    free_list = &partition_table[1];  // 只有用户分区是空闲的
    allocated_list = &partition_table[0];  // OS分区是已分配的

    DEBUG_PRINT("Partition table correctly initialized");
    dump_memory_map();
}

// 查找空闲分区
partition_t* find_free_partition(uint32_t size) {
    partition_t* current = free_list;
    while (current) {
        if (current->state == PARTITION_FREE && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// 分配分区
int allocate_partition(partition_t* part, process_t* proc) {
    if (!part || !proc || part->state != PARTITION_FREE) {
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

    // 添加到已分配列表 (正确维护链表)
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

// 合并相邻空闲分区
void merge_adjacent_free_partitions(void) {
    // 简化版本：只检查前两个空闲分区
    if (!free_list || !free_list->next) {
        return;
    }

    partition_t* p1 = free_list;
    partition_t* p2 = free_list->next;

    // 按起始地址排序
    if (p1->start > p2->start) {
        // 交换
        partition_t* temp = p1;
        p1 = p2;
        p2 = temp;
    }

    // 检查是否相邻
    if (p1->start + p1->size == p2->start) {
        DEBUG_PRINT("Merging adjacent partitions: 0x%x+%d and 0x%x+%d",
            p1->start, p1->size, p2->start, p2->size);

        // 合并
        p1->size += p2->size;

        // 从链表中移除p2
        partition_t* current = free_list;
        while (current && current->next != p2) {
            current = current->next;
        }
        if (current) {
            current->next = p2->next;
        }
    }
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