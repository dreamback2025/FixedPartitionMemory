#ifndef _CONFIG_H
#define _CONFIG_H

#include "os_types.h"

// 系统配置
#define MEMORY_SIZE 1024        // 1KB内存
#define OS_PARTITION_SIZE 128   // 操作系统占用128字节
#define MIN_PARTITION_SIZE 32   // 最小分区大小
#define DEFAULT_ALLOCATION_STRATEGY BEST_FIT

// 时间配置
#define TIME_SLICE 2            // 时间片大小
#define TIMER_INTERVAL 1000     // 1秒

// 日志配置
#define KERNEL_LOG_LEVEL LOG_INFO
#define LOG_BUFFER_SIZE 1024

#endif // _CONFIG_H