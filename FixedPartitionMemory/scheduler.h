#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "os_types.h"
#include "process.h"

// 调度算法类型
typedef enum {
    SCHED_FIFO,      // 先进先出
    SCHED_RR,        // 时间片轮转
    SCHED_PRIORITY   // 优先级调度
} scheduler_type_t;

// 就绪队列结构
typedef struct ready_queue_t {
    process_t* front;    // 队列前端
    process_t* rear;     // 队列后端
    uint32_t count;      // 队列中的进程数量
} ready_queue_t;

// 调度器状态
typedef struct scheduler_t {
    ready_queue_t ready_queue;
    process_t* current_process;  // 当前运行的进程
    scheduler_type_t type;       // 调度算法类型
    uint32_t time_slice;         // 时间片大小
    uint32_t current_time_slice; // 当前时间片剩余
} scheduler_t;

// 调度器API
void scheduler_init(scheduler_type_t type);
void scheduler_add_process(process_t* proc);
process_t* scheduler_get_next_process(void);
void scheduler_schedule(void);
void scheduler_run_current_process(void);
void scheduler_dump_status(void);

// 全局调度器变量声明
extern scheduler_t g_scheduler;

// 时间片轮转调度相关函数
void rr_scheduler_init(void);
process_t* rr_scheduler_get_next(void);
void rr_scheduler_run_process(process_t* proc);

#endif // _SCHEDULER_H