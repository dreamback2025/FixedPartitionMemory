#include "process.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <string.h>
#endif

// 全局进程表
Process process_table[MAX_PROCESSES];

// 创建进程
int create_process(int id, const char* name, int priority, int exec_time) {
    // 查找空槽位
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(process_table[i].state == EMPTY) {
            process_table[i].id = id;
            strncpy(process_table[i].name, name, MAX_NAME_LENGTH - 1);
            process_table[i].name[MAX_NAME_LENGTH - 1] = '\0'; // 确保字符串结束
            process_table[i].state = READY;
            process_table[i].priority = priority;
            process_table[i].exec_time = exec_time;
            process_table[i].remaining_time = exec_time;
            process_table[i].arrival_time = get_system_time();
            process_table[i].start_time = -1;
            process_table[i].finish_time = -1;
            process_table[i].time_slice = 2; // 默认时间片为2
            process_table[i].last_run_time = 0;
            
            // 初始化CPU上下文
#ifdef _WIN32
            ZeroMemory(&process_table[i].context, sizeof(CPUContext));
#else
            memset(&process_table[i].context, 0, sizeof(CPUContext));
#endif
            
            printf("进程创建成功: ID=%d, Name=%s, Priority=%d, ExecTime=%d\n", 
                   id, name, priority, exec_time);
            return 1;
        }
    }
    
    printf("错误: 进程表已满，无法创建进程\n");
    return 0;
}

// 删除进程
int delete_process(int id) {
    int index = find_process_by_id(id);
    if(index != -1) {
        process_table[index].state = EMPTY;
        printf("进程 %d 已删除\n", id);
        return 1;
    }
    
    printf("错误: 找不到进程 %d\n", id);
    return 0;
}

// 获取进程总数
int get_process_count() {
    int count = 0;
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(process_table[i].state != EMPTY) {
            count++;
        }
    }
    return count;
}

// 根据ID查找进程
int find_process_by_id(int id) {
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(process_table[i].state != EMPTY && process_table[i].id == id) {
            return i;
        }
    }
    return -1;
}

// 打印进程表
void print_process_table() {
    printf("\n--- 进程表 ---\n");
    printf("%-5s %-15s %-8s %-8s %-10s %-10s %-10s\n", 
           "ID", "Name", "State", "Priority", "ExecTime", "Remain", "Arrival");
    
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(process_table[i].state != EMPTY) {
            char state_str[10];
            switch(process_table[i].state) {
                case READY: strcpy(state_str, "READY"); break;
                case RUNNING: strcpy(state_str, "RUN"); break;
                case BLOCKED: strcpy(state_str, "BLOCK"); break;
                case TERMINATED: strcpy(state_str, "TERM"); break;
                default: strcpy(state_str, "EMPTY"); break;
            }
            
            printf("%-5d %-15s %-8s %-8d %-10d %-10d %-10d\n",
                   process_table[i].id, 
                   process_table[i].name,
                   state_str,
                   process_table[i].priority,
                   process_table[i].exec_time,
                   process_table[i].remaining_time,
                   process_table[i].arrival_time);
        }
    }
    printf("----------------\n");
}

// 更新进程状态
void update_process_states() {
    int current_time = get_system_time();
    
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(process_table[i].state == RUNNING) {
            // 减少剩余时间
            process_table[i].remaining_time--;
            
            // 如果进程完成
            if(process_table[i].remaining_time <= 0) {
                process_table[i].state = TERMINATED;
                process_table[i].finish_time = current_time;
                printf("进程 %d (%s) 已完成\n", process_table[i].id, process_table[i].name);
            }
            // 如果时间片用完
            else if(current_time - process_table[i].last_run_time >= process_table[i].time_slice) {
                process_table[i].state = READY;  // 改为就绪态，准备被调度
                printf("进程 %d (%s) 时间片用完，切换到就绪态\n", process_table[i].id, process_table[i].name);
            }
        }
    }
}

// 获取下一个就绪进程
int get_next_ready_process() {
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(process_table[i].state == READY) {
            return i;
        }
    }
    return -1; // 没有就绪进程
}

// 保存进程状态
void save_process_state(int process_id) {
    int index = find_process_by_id(process_id);
    if(index != -1) {
        // 保存当前CPU上下文
        cpu_context_save(&process_table[index].context);
        printf("进程 %d 的状态已保存\n", process_id);
    }
}

// 恢复进程状态
void restore_process_state(int process_id) {
    int index = find_process_by_id(process_id);
    if(index != -1) {
        // 恢复CPU上下文
        cpu_context_restore(&process_table[index].context);
        printf("进程 %d 的状态已恢复\n", process_id);
    }
}