#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

// 调度算法类型
typedef enum {
    FCFS = 0,        // 先来先服务
    RR = 1,          // 时间片轮转
    PRIORITY = 2     // 优先级调度
} SchedulingAlgorithm;

// 调度器状态
typedef struct {
    SchedulingAlgorithm algorithm;  // 当前调度算法
    int current_process_index;      // 当前运行进程索引
    int time_quantum;              // 时间片大小
    int last_schedule_time;        // 上次调度时间
} SchedulerState;

// 全局调度器状态
extern SchedulerState scheduler_state;

// 调度函数声明
int scheduler_init();
int schedule_next();
void set_scheduling_algorithm(SchedulingAlgorithm algo);
int find_highest_priority_ready_process();
int find_next_rr_process();
void context_switch(int from_process, int to_process);
void print_scheduler_info();

#endif // SCHEDULER_H