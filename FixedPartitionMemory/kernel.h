#ifndef _KERNEL_H
#define _KERNEL_H

#include "os_types.h"
#include "log.h"
#include "process.h"
#include "partition.h"
#include "memory.h"

// 内核初始化函数
void kernel_init(void);

// 获取当前时间
uint32_t get_current_time(void);

// 推进时间
void advance_time(void);

// 获取内存指针
uint8_t* get_memory_base(void);

// 获取内存大小
uint32_t get_memory_size(void);

#endif // _KERNEL_H