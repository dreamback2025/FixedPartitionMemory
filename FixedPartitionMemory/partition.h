#ifndef _PARTITION_H
#define _PARTITION_H

#include "os_types.h"

// 先包含process.h，这样可以使用完整的process_t定义
#include "process.h"

// 分区状态
typedef enum {
    PARTITION_FREE,    // 空闲
    PARTITION_ALLOCATED, // 已分配
    PARTITION_OS       // 操作系统占用
} partition_state_t;

// 内存分区 (不包含函数指针，简化结构)
typedef struct partition_t {
    uint32_t start;            // 起始地址
    uint32_t size;             // 大小
    partition_state_t state;   // 状态
    uint32_t owner_pid;        // 所有者PID (0表示无)

    // 链表指针
    struct partition_t* next;
} partition_t;

// 全局变量声明
extern partition_t partition_table[MAX_PARTITIONS];
extern partition_t* free_list;
extern partition_t* allocated_list;
extern process_t process_table[MAX_PROCESSES];

// 内核API
void partition_init(void);
partition_t* find_free_partition(uint32_t size);
int allocate_partition(partition_t* part, process_t* proc);
void free_partition(partition_t* part);
void merge_adjacent_free_partitions(void);
void dump_memory_map(void);

#endif // _PARTITION_H