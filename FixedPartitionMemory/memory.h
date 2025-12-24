#ifndef _MEMORY_H
#define _MEMORY_H

#include "os_types.h"
#include "process.h"
#include "partition.h"

// 分配策略
typedef enum {
    FIRST_FIT,      // 首次适应
    BEST_FIT,       // 最佳适应
    WORST_FIT       // 最坏适应
} allocation_strategy_t;

// 全局变量声明
extern allocation_strategy_t current_strategy;

// 内核API
void memory_init(void);
int allocate_memory(process_t* proc, allocation_strategy_t strategy);
void free_memory(process_t* proc);
void compact_memory(void);  // 紧凑算法
uint32_t get_total_free_memory(void);
uint32_t get_largest_free_block(void);
void dump_memory_statistics(void);

#endif // _MEMORY_H