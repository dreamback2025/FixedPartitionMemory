#include "scheduler.h"
#include "kernel.h"
#include <stdio.h>

// 全局调度器状态
SchedulerState scheduler_state = {0};

// 调度器初始化
int scheduler_init() {
    scheduler_state.algorithm = RR;  // 默认使用时间片轮转
    scheduler_state.current_process_index = -1;
    scheduler_state.time_quantum = 2;  // 默认时间片为2
    scheduler_state.last_schedule_time = 0;
    
    printf("调度器初始化完成，算法: 时间片轮转(RR)\n");
    return 1;
}

// 设置调度算法
void set_scheduling_algorithm(SchedulingAlgorithm algo) {
    scheduler_state.algorithm = algo;
    
    char* algo_names[] = {"FCFS", "RR", "PRIORITY"};
    printf("调度算法已设置为: %s\n", algo_names[algo]);
}

// 查找最高优先级的就绪进程
int find_highest_priority_ready_process() {
    int highest_priority = -1;
    int selected_process = -1;
    
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(process_table[i].state == READY) {
            if(highest_priority == -1 || process_table[i].priority < highest_priority) {
                highest_priority = process_table[i].priority;
                selected_process = i;
            }
        }
    }
    
    return selected_process;
}

// 查找下一个RR进程
int find_next_rr_process() {
    static int last_index = -1;
    
    // 从上次调度的进程之后开始查找
    for(int i = last_index + 1; i < MAX_PROCESSES; i++) {
        if(process_table[i].state == READY) {
            last_index = i;
            return i;
        }
    }
    
    // 如果没找到，从头开始查找
    for(int i = 0; i < last_index; i++) {
        if(process_table[i].state == READY) {
            last_index = i;
            return i;
        }
    }
    
    return -1; // 没有找到就绪进程
}

// 上下文切换
void context_switch(int from_process, int to_process) {
    // 保存当前进程的上下文
    if(from_process != -1 && process_table[from_process].state == RUNNING) {
        save_process_state(process_table[from_process].id);
        process_table[from_process].state = READY;
        printf("保存进程 %d 上下文\n", process_table[from_process].id);
    }
    
    // 恢复目标进程的上下文
    if(to_process != -1) {
        process_table[to_process].state = RUNNING;
        process_table[to_process].last_run_time = get_system_time();
        
        if(process_table[to_process].start_time == -1) {
            process_table[to_process].start_time = get_system_time();
        }
        
        restore_process_state(process_table[to_process].id);
        printf("恢复进程 %d 上下文，开始运行\n", process_table[to_process].id);
    }
    
    scheduler_state.current_process_index = to_process;
}

// 执行调度
int schedule_next() {
    int next_process = -1;
    
    // 更新进程状态
    update_process_states();
    
    switch(scheduler_state.algorithm) {
        case FCFS:
            // 先来先服务 - 查找最早到达的就绪进程
            {
                int earliest_time = -1;
                for(int i = 0; i < MAX_PROCESSES; i++) {
                    if(process_table[i].state == READY) {
                        if(earliest_time == -1 || process_table[i].arrival_time < earliest_time) {
                            earliest_time = process_table[i].arrival_time;
                            next_process = i;
                        }
                    }
                }
            }
            break;
            
        case RR: // 时间片轮转
            next_process = find_next_rr_process();
            break;
            
        case PRIORITY: // 优先级调度
            next_process = find_highest_priority_ready_process();
            break;
            
        default:
            next_process = find_next_rr_process();
            break;
    }
    
    // 如果当前没有运行进程或当前进程已完成时间片，则切换
    if(next_process != -1) {
        if(scheduler_state.current_process_index == -1 || 
           process_table[scheduler_state.current_process_index].state != RUNNING) {
            // 没有当前运行进程，直接切换
            context_switch(-1, next_process);
        } else if(scheduler_state.current_process_index != next_process) {
            // 当前进程与目标进程不同，进行上下文切换
            context_switch(scheduler_state.current_process_index, next_process);
        }
    } else if(scheduler_state.current_process_index != -1 && 
              process_table[scheduler_state.current_process_index].state != RUNNING) {
        // 没有就绪进程，但当前进程不在运行状态
        scheduler_state.current_process_index = -1;
    }
    
    scheduler_state.last_schedule_time = get_system_time();
    
    // 打印调度信息
    if(next_process != -1) {
        printf("调度: 进程 %d (%s) 获得CPU\n", 
               process_table[next_process].id, process_table[next_process].name);
    } else {
        printf("调度: 没有就绪进程\n");
    }
    
    return next_process != -1;
}

// 打印调度器信息
void print_scheduler_info() {
    char* algo_names[] = {"FCFS", "RR", "PRIORITY"};
    
    printf("\n--- 调度器信息 ---\n");
    printf("当前算法: %s\n", algo_names[scheduler_state.algorithm]);
    printf("时间片大小: %d\n", scheduler_state.time_quantum);
    printf("当前运行进程: %d\n", 
           scheduler_state.current_process_index != -1 ? 
           process_table[scheduler_state.current_process_index].id : -1);
    printf("上次调度时间: %d\n", scheduler_state.last_schedule_time);
    printf("------------------\n");
}