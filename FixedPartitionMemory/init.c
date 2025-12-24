#include "os_types.h"
#include "log.h"
#include "process.h"
#include "partition.h"
#include "memory.h"
#include "config.h"

// 全局内存状态
static uint8_t system_memory[MEMORY_SIZE];
static uint32_t current_time = 0;

// 内核初始化
void kernel_init(void) {
    kernel_log_init();
    kernel_log(LOG_INFO, "Kernel initialization started");

    // 初始化进程管理
    process_init();
    kernel_log(LOG_INFO, "Process management initialized");

    // 初始化分区管理
    partition_init();
    kernel_log(LOG_INFO, "Partition management initialized");

    // 初始化内存管理
    memory_init();
    kernel_log(LOG_INFO, "Memory management initialized");

    kernel_log(LOG_INFO, "Kernel initialization completed");
}

// 获取当前时间
uint32_t get_current_time(void) {
    return current_time;
}

// 推进时间
void advance_time(void) {
    current_time++;
    kernel_log(LOG_DEBUG, "Time advanced to %d", current_time);
}

// 获取内存指针
uint8_t* get_memory_base(void) {
    return system_memory;
}

// 获取内存大小
uint32_t get_memory_size(void) {
    return MEMORY_SIZE;
}