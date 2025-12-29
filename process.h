#ifndef PROCESS_H
#define PROCESS_H

#include "kernel.h"

#define MAX_PROCESSES 100
#define MAX_NAME_LENGTH 50

// 进程状态枚举
typedef enum {
    EMPTY = 0,      // 空槽位
    READY = 1,      // 就绪态
    RUNNING = 2,    // 运行态
    BLOCKED = 3,    // 阻塞态
    TERMINATED = 4  // 终止态
} ProcessState;

// 进程控制块 (PCB)
typedef struct {
    int id;                    // 进程ID
    char name[MAX_NAME_LENGTH]; // 进程名称
    ProcessState state;         // 进程状态
    int priority;              // 优先级
    int exec_time;             // 总执行时间
    int remaining_time;        // 剩余执行时间
    int arrival_time;          // 到达时间
    int start_time;            // 开始执行时间
    int finish_time;           // 完成时间
    CPUContext context;        // CPU上下文
    int time_slice;           // 时间片
    int last_run_time;        // 上次运行时间
} Process;

// 全局进程表
extern Process process_table[MAX_PROCESSES];

// 进程相关函数声明
int create_process(int id, const char* name, int priority, int exec_time);
int delete_process(int id);
int get_process_count();
int find_process_by_id(int id);
void print_process_table();
void update_process_states();
int get_next_ready_process();
void save_process_state(int process_id);
void restore_process_state(int process_id);

#endif // PROCESS_H