#ifndef _PROCESS_H
#define _PROCESS_H

#include "os_types.h"

// 首先前向声明 partition_t
struct partition_t;

// 进程状态
typedef enum {
    PROC_CREATED,    // 已创建
    PROC_READY,      // 就绪
    PROC_RUNNING,    // 运行
    PROC_WAITING,    // 等待
    PROC_TERMINATED  // 终止
} process_state_t;

// 进程结构
typedef struct process_t {
    uint32_t pid;              // 进程ID
    char name[16];             // 进程名称
    process_state_t state;     // 进程状态

    // 内存需求
    uint32_t memory_size;      // 需要的内存大小
    uint32_t memory_start;     // 分配的内存起始地址
    uint32_t memory_end;       // 分配的内存结束地址

    // 执行时间
    uint32_t arrival_time;     // 到达时间
    uint32_t burst_time;       // 执行时间
    uint32_t remaining_time;   // 剩余执行时间

    // 其他信息
    uint32_t priority;         // 优先级
    uint32_t io_requests;      // I/O请求数

    // 链表指针
    struct process_t* next;
} process_t;

// 全局变量声明（extern）
extern process_t process_table[MAX_PROCESSES];

// 内核API
void process_init(void);
process_t* create_process(uint32_t pid, const char* name, uint32_t memory_size,
    uint32_t burst_time, uint32_t arrival_time);
process_t* find_process_by_pid(uint32_t pid);
void terminate_process(process_t* proc);
void process_set_state(process_t* proc, process_state_t new_state);
void dump_process_info(process_t* proc);

#endif // _PROCESS_H