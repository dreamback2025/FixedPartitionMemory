#ifndef _CONFIG_H
#define _CONFIG_H

#include "os_types.h"

// 系统配置
#define TOTAL_MEMORY (1024 * 1024)  // 1MB内存
#define OS_PARTITION_SIZE (64 * 1024)   // 操作系统占用64KB
#define MIN_PARTITION_SIZE (32 * 1024)  // 最小分区大小32KB
#define MAX_PARTITIONS 16         // 最大分区数 - 使用与os_types.h一致的值
#define MAX_PROCESSES 32          // 最大进程数 - 使用与os_types.h一致的值

// 时间配置
#define TIME_SLICE 3              // 时间片大小
#define TIMER_INTERVAL 1000       // 1秒

// 日志配置
#define KERNEL_LOG_LEVEL LOG_INFO
#define LOG_BUFFER_SIZE 1024
#define MAX_LOG_ENTRIES 1000

// 多道程序设计配置
#define MAX_EXECUTION_RECORDS 10000  // 最大执行记录数

// 内存分配策略
#define DEFAULT_ALLOCATION_STRATEGY FIRST_FIT

// 定义内存大小常量
#define MEMORY_SIZE TOTAL_MEMORY

#endif // _CONFIG_H