#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "os_types.h"
#include "process.h"
#include "scheduler.h"
#include "memory.h"
#include "log.h"
#include "config.h"

// 进程控制块扩展，包含CPU上下文
typedef struct {
    process_t base;                 // 基础进程结构
    uint32_t start_time;            // 进程开始执行时间
    uint32_t wait_time;             // 等待时间
    uint32_t turnaround_time;       // 周转时间
    uint32_t response_time;         // 响应时间
    uint32_t executed_time;         // 已执行时间
} pcb_t;

// 系统状态
static pcb_t pcb_table[MAX_PROCESSES];
static uint32_t total_processes = 0;
static uint32_t system_time = 0;    // 系统时间计数器
static uint32_t last_interrupt_time = 0; // 最后中断时间
static uint32_t interrupt_interval = 1;   // 中断间隔（时间单位）

// 进程执行历史记录
typedef struct execution_record {
    uint32_t timestamp;
    uint32_t pid;
    process_state_t state;
    uint32_t remaining_time;
    uint32_t executed_time;
    struct execution_record* next;
} execution_record_t;

static execution_record_t* execution_history = NULL;
static execution_record_t* history_tail = NULL;

// 函数声明
void save_execution_state(pcb_t* pcb);
void restore_execution_state(pcb_t* pcb);
void simulate_interrupt();
void save_execution_history(uint32_t pid, process_state_t state, uint32_t remaining_time, uint32_t executed_time);
void save_execution_to_file(const char* filename);
void load_execution_from_file(const char* filename);
void replay_execution();
void print_system_status();
void initialize_cpu_context(pcb_t* pcb);

// 初始化CPU上下文
void initialize_cpu_context(pcb_t* pcb) {
    if (!pcb) return;
    
    // 初始化PCB中的进程信息
    pcb->start_time = 0;
    pcb->wait_time = 0;
    pcb->turnaround_time = 0;
    pcb->response_time = 0;
    pcb->executed_time = 0;
}

// 保存进程上下文
void save_execution_state(pcb_t* pcb) {
    if (!pcb) return;
    
    // 使用系统调用保存CPU上下文
    save_cpu_context((pcb_t*)pcb);
    
    // 记录执行历史
    save_execution_history(pcb->base.pid, pcb->base.state, 
                          pcb->base.remaining_time, pcb->executed_time);
}

// 恢复进程上下文
void restore_execution_state(pcb_t* pcb) {
    if (!pcb) return;
    
    // 使用系统调用恢复CPU上下文
    restore_cpu_context((pcb_t*)pcb);
    
    DEBUG_PRINT("Restored context for process %d", pcb->base.pid);
}

// 模拟硬件中断
void simulate_interrupt() {
    system_time++;
    
    // 检查是否需要调度
    if (g_scheduler.current_process) {
        pcb_t* current_pcb = NULL;
        
        // 找到对应的PCB
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (pcb_table[i].base.pid == g_scheduler.current_process->pid && 
                pcb_table[i].base.state != PROC_TERMINATED) {
                current_pcb = &pcb_table[i];
                break;
            }
        }
        
        if (current_pcb) {
            // 更新执行时间
            current_pcb->executed_time++;
            
            // 检查是否完成
            if (current_pcb->base.remaining_time > 0) {
                current_pcb->base.remaining_time--;
                
                // 检查时间片是否用完
                if (g_scheduler.current_time_slice > 0) {
                    g_scheduler.current_time_slice--;
                    
                    if (g_scheduler.current_time_slice == 0 && g_scheduler.type == SCHED_RR) {
                        // 时间片用完，进行进程切换
                        save_execution_state(current_pcb);
                        
                        // 将当前进程放回就绪队列
                        process_set_state(&current_pcb->base, PROC_READY);
                        scheduler_add_process(&current_pcb->base);
                        
                        g_scheduler.current_process = NULL;
                    }
                }
            } else {
                // 进程完成
                save_execution_state(current_pcb);
                process_set_state(&current_pcb->base, PROC_TERMINATED);
                free_memory(&current_pcb->base);
                g_scheduler.current_process = NULL;
            }
        }
    }
    
    // 执行调度
    scheduler_schedule();
    
    // 如果有可运行进程，恢复其上下文
    if (g_scheduler.current_process) {
        pcb_t* current_pcb = NULL;
        
        // 找到对应的PCB
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (pcb_table[i].base.pid == g_scheduler.current_process->pid && 
                pcb_table[i].base.state != PROC_TERMINATED) {
                current_pcb = &pcb_table[i];
                break;
            }
        }
        
        if (current_pcb) {
            restore_execution_state(current_pcb);
            process_set_state(&current_pcb->base, PROC_RUNNING);
        }
    }
}

// 记录执行历史
void save_execution_history(uint32_t pid, process_state_t state, 
                           uint32_t remaining_time, uint32_t executed_time) {
    execution_record_t* record = (execution_record_t*)malloc(sizeof(execution_record_t));
    if (!record) return;
    
    record->timestamp = system_time;
    record->pid = pid;
    record->state = state;
    record->remaining_time = remaining_time;
    record->executed_time = executed_time;
    record->next = NULL;
    
    if (!execution_history) {
        execution_history = record;
        history_tail = record;
    } else {
        history_tail->next = record;
        history_tail = record;
    }
}

// 保存执行历史到文件
void save_execution_to_file(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        kernel_log(LOG_ERROR, "Cannot open file %s for writing", filename);
        return;
    }
    
    fprintf(file, "System_Time,Process_ID,State,Remaining_Time,Executed_Time\n");
    
    execution_record_t* current = execution_history;
    while (current) {
        fprintf(file, "%u,%u,%d,%u,%u\n", 
                current->timestamp, current->pid, current->state, 
                current->remaining_time, current->executed_time);
        current = current->next;
    }
    
    fclose(file);
    kernel_log(LOG_INFO, "Execution history saved to %s", filename);
}

// 从文件加载执行历史
void load_execution_from_file(const char* filename) {
    // 清理现有历史
    execution_record_t* current = execution_history;
    while (current) {
        execution_record_t* next = current->next;
        free(current);
        current = next;
    }
    execution_history = NULL;
    history_tail = NULL;
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        kernel_log(LOG_ERROR, "Cannot open file %s for reading", filename);
        return;
    }
    
    char line[256];
    // 跳过标题行
    if (fgets(line, sizeof(line), file)) {
        // 读取数据行
        while (fgets(line, sizeof(line), file)) {
            execution_record_t* record = (execution_record_t*)malloc(sizeof(execution_record_t));
            if (!record) break;
            
            if (sscanf(line, "%u,%u,%d,%u,%u", 
                      &record->timestamp, &record->pid, (int*)&record->state,
                      &record->remaining_time, &record->executed_time) == 5) {
                record->next = NULL;
                
                if (!execution_history) {
                    execution_history = record;
                    history_tail = record;
                } else {
                    history_tail->next = record;
                    history_tail = record;
                }
            } else {
                free(record);
            }
        }
    }
    
    fclose(file);
    kernel_log(LOG_INFO, "Execution history loaded from %s", filename);
}

// 重放执行历史
void replay_execution() {
    kernel_log(LOG_INFO, "Starting execution replay...");
    
    execution_record_t* current = execution_history;
    uint32_t last_timestamp = 0;
    
    while (current) {
        // 打印时间变化
        if (current->timestamp > last_timestamp) {
            printf("\n--- Time: %u ---\n", current->timestamp);
            last_timestamp = current->timestamp;
        }
        
        const char* state_str[] = {"CREATED", "READY", "RUNNING", "WAITING", "TERMINATED"};
        printf("Process %u: State=%s, Remaining=%u, Executed=%u\n", 
               current->pid, 
               state_str[current->state], 
               current->remaining_time, 
               current->executed_time);
        
        current = current->next;
    }
    
    kernel_log(LOG_INFO, "Execution replay completed");
}

// 打印系统状态
void print_system_status() {
    printf("\n=== System Status at Time %u ===\n", system_time);
    printf("Total Processes: %u\n", total_processes);
    printf("Current Process: %s\n", 
           g_scheduler.current_process ? g_scheduler.current_process->name : "None");
    
    printf("Ready Queue Count: %u\n", g_scheduler.ready_queue.count);
    
    printf("Process List:\n");
    for (uint32_t i = 0; i < total_processes; i++) {
        pcb_t* pcb = &pcb_table[i];
        if (pcb->base.state != PROC_TERMINATED) {
            const char* state_str[] = {"CREATED", "READY", "RUNNING", "WAITING", "TERMINATED"};
            printf("  PID: %u, Name: %s, State: %s, Remaining: %u, Memory: %uKB\n",
                   pcb->base.pid, pcb->base.name, state_str[pcb->base.state],
                   pcb->base.remaining_time, pcb->base.memory_size / 1024);
        }
    }
    printf("===============================\n");
}

// 创建进程（自动模式）
pcb_t* create_auto_process(const char* name, uint32_t burst_time, uint32_t memory_size) {
    if (total_processes >= MAX_PROCESSES) {
        kernel_log(LOG_ERROR, "Maximum number of processes reached");
        return NULL;
    }
    
    pcb_t* pcb = &pcb_table[total_processes];
    pcb->base.pid = total_processes + 1;
    strncpy(pcb->base.name, name, sizeof(pcb->base.name) - 1);
    pcb->base.name[sizeof(pcb->base.name) - 1] = '\0';
    pcb->base.state = PROC_CREATED;
    pcb->base.memory_size = memory_size;
    pcb->base.burst_time = burst_time;
    pcb->base.remaining_time = burst_time;
    pcb->base.arrival_time = system_time;
    pcb->base.priority = rand() % 10 + 1;  // 随机优先级
    pcb->base.io_requests = rand() % 5;    // 随机IO请求数
    pcb->base.next = NULL;
    
    // 初始化PCB的额外信息
    pcb->start_time = 0;
    pcb->wait_time = 0;
    pcb->turnaround_time = 0;
    pcb->response_time = 0;
    pcb->executed_time = 0;
    
    // 初始化CPU上下文
    initialize_cpu_context(pcb);
    
    // 分配内存
    if (allocate_memory(&pcb->base) != 0) {
        kernel_log(LOG_ERROR, "Failed to allocate memory for process %s", name);
        return NULL;
    }
    
    // 设置为就绪状态
    process_set_state(&pcb->base, PROC_READY);
    scheduler_add_process(&pcb->base);
    
    total_processes++;
    
    kernel_log(LOG_INFO, "Auto-created process %s (PID: %u, Burst: %u, Memory: %uKB)", 
               name, pcb->base.pid, burst_time, memory_size / 1024);
    
    return pcb;
}

// 创建进程（手动输入模式）
pcb_t* create_manual_process() {
    if (total_processes >= MAX_PROCESSES) {
        kernel_log(LOG_ERROR, "Maximum number of processes reached");
        return NULL;
    }
    
    char name[16];
    uint32_t burst_time, memory_size;
    
    printf("Enter process name: ");
    scanf("%15s", name);
    
    printf("Enter burst time: ");
    scanf("%u", &burst_time);
    
    printf("Enter memory size (KB): ");
    scanf("%u", &memory_size);
    memory_size *= 1024;  // 转换为字节
    
    pcb_t* pcb = &pcb_table[total_processes];
    pcb->base.pid = total_processes + 1;
    strncpy(pcb->base.name, name, sizeof(pcb->base.name) - 1);
    pcb->base.name[sizeof(pcb->base.name) - 1] = '\0';
    pcb->base.state = PROC_CREATED;
    pcb->base.memory_size = memory_size;
    pcb->base.burst_time = burst_time;
    pcb->base.remaining_time = burst_time;
    pcb->base.arrival_time = system_time;
    pcb->base.priority = 5;  // 默认优先级
    pcb->base.io_requests = 0;
    pcb->base.next = NULL;
    
    // 初始化PCB的额外信息
    pcb->start_time = 0;
    pcb->wait_time = 0;
    pcb->turnaround_time = 0;
    pcb->response_time = 0;
    pcb->executed_time = 0;
    
    // 初始化CPU上下文
    initialize_cpu_context(pcb);
    
    // 分配内存
    if (allocate_memory(&pcb->base) != 0) {
        kernel_log(LOG_ERROR, "Failed to allocate memory for process %s", name);
        return NULL;
    }
    
    // 设置为就绪状态
    process_set_state(&pcb->base, PROC_READY);
    scheduler_add_process(&pcb->base);
    
    total_processes++;
    
    kernel_log(LOG_INFO, "Manually created process %s (PID: %u, Burst: %u, Memory: %uKB)", 
               name, pcb->base.pid, burst_time, memory_size / 1024);
    
    return pcb;
}

// 显示菜单
void show_menu() {
    printf("\n=== Multitasking Kernel Menu ===\n");
    printf("A/a - Auto create process\n");
    printf("M/m - Manual create process\n");
    printf("S/s - Simulate one time unit (interrupt)\n");
    printf("T/t - Simulate 5 time units\n");
    printf("D/d - Show scheduler status\n");
    printf("P/p - Show system status\n");
    printf("H/h - Show this menu\n");
    printf("W/w - Save execution to file\n");
    printf("L/l - Load execution from file\n");
    printf("R/r - Replay execution\n");
    printf("Q/q - Quit\n");
    printf("================================\n");
}

// 主函数
int main() {
    // 初始化系统
    srand(time(NULL));  // 初始化随机数种子
    kernel_log_init();
    process_init();
    memory_init();
    scheduler_init(SCHED_RR);  // 使用时间片轮转调度
    
    kernel_log(LOG_INFO, "Multitasking Kernel Started");
    
    // 创建一些初始进程用于演示
    create_auto_process("Process1", 10, 1024);  // 1KB
    create_auto_process("Process2", 8, 2048);   // 2KB
    create_auto_process("Process3", 15, 1536);  // 1.5KB
    
    show_menu();
    
    char choice;
    while (1) {
        printf("\nEnter command: ");
        scanf(" %c", &choice);
        
        switch (choice) {
            case 'A':
            case 'a':
                create_auto_process("AutoProc", 5 + rand() % 10, 1024 + rand() % 4096);
                break;
                
            case 'M':
            case 'm':
                create_manual_process();
                break;
                
            case 'S':
            case 's':
                simulate_interrupt();
                break;
                
            case 'T':
            case 't':
                for (int i = 0; i < 5; i++) {
                    simulate_interrupt();
                }
                break;
                
            case 'D':
            case 'd':
                scheduler_dump_status();
                break;
                
            case 'P':
            case 'p':
                print_system_status();
                break;
                
            case 'H':
            case 'h':
                show_menu();
                break;
                
            case 'W':
            case 'w': {
                char filename[256];
                printf("Enter filename to save: ");
                scanf("%255s", filename);
                save_execution_to_file(filename);
                break;
            }
            
            case 'L':
            case 'l': {
                char filename[256];
                printf("Enter filename to load: ");
                scanf("%255s", filename);
                load_execution_from_file(filename);
                break;
            }
            
            case 'R':
            case 'r':
                replay_execution();
                break;
                
            case 'Q':
            case 'q':
                kernel_log(LOG_INFO, "Shutting down multitasking kernel");
                
                // 清理资源
                execution_record_t* current = execution_history;
                while (current) {
                    execution_record_t* next = current->next;
                    free(current);
                    current = next;
                }
                
                return 0;
                
            default:
                printf("Invalid command. Press H/h for help.\n");
                break;
        }
    }
    
    return 0;
}