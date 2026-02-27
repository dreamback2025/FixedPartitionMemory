#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "kernel.h"
#include "os_types.h"
#include "process.h"
#include "scheduler.h"
#include "memory.h"
#include "log.h"
#include "config.h"

// 系统时间计数器
static uint32_t system_time = 0;

// CPU寄存器模拟结构
typedef struct {
    uint32_t eax, ebx, ecx, edx;    // 通用寄存器
    uint32_t eip;                   // 指令指针
    uint32_t esp;                   // 栈指针
    uint32_t ebp;                   // 基址指针
    uint32_t eflags;                // 标志寄存器
    uint32_t cs, ds, es, ss;        // 段寄存器
} cpu_context_t;

// 模拟CPU上下文
static cpu_context_t current_cpu_context;

// 内存管理相关
static uint8_t* system_memory = NULL;
static uint32_t memory_size = MAX_MEMORY_SIZE * 1024;  // 将KB转为字节

// 系统状态
static uint32_t total_processes = 0;
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
void kernel_init(void);
uint32_t get_current_time(void);
void advance_time(void);
uint8_t* get_memory_base(void);
uint32_t get_memory_size(void);
void save_execution_state(pcb_t* pcb);
void restore_execution_state(pcb_t* pcb);
void simulate_interrupt(void);
void save_execution_history(uint32_t pid, process_state_t state, uint32_t remaining_time, uint32_t executed_time);
void save_execution_to_file(const char* filename);
void load_execution_from_file(const char* filename);
void replay_execution(void);
void print_system_status(void);
void initialize_cpu_context(pcb_t* pcb);
pcb_t* create_auto_process(const char* name, uint32_t burst_time, uint32_t memory_size);
pcb_t* create_manual_process(void);

// 内核初始化函数
void kernel_init(void) {
    // 初始化随机数种子
    srand((unsigned int)time(NULL));
    
    // 初始化日志系统
    kernel_log_init();
    
    // 初始化进程系统
    process_init();
    
    // 初始化内存管理系统
    memory_init();
    
    // 初始化调度器（使用时间片轮转算法）
    scheduler_init(SCHED_RR);
    
    // 初始化CPU上下文
    memset(&current_cpu_context, 0, sizeof(cpu_context_t));
    
    total_processes = 0;
    system_time = 0;
    last_interrupt_time = 0;
    
    // 初始化内存分配
    system_memory = (uint8_t*)malloc(memory_size);
    if (system_memory == NULL) {
        kernel_log(LOG_ERR, "Failed to allocate system memory");
        return;
    }
    
    kernel_log(LOG_INFO, "Kernel initialized successfully");
    kernel_log(LOG_INFO, "System memory allocated: %u bytes", memory_size);
}

// 获取当前时间
uint32_t get_current_time(void) {
    return system_time;
}

// 推进时间
void advance_time(void) {
    system_time++;
}

// 获取内存基址
uint8_t* get_memory_base(void) {
    return system_memory;
}

// 获取内存大小
uint32_t get_memory_size(void) {
    return memory_size;
}

// 初始化CPU上下文
void initialize_cpu_context(pcb_t* pcb) {
    if (!pcb) return;
    
    // 初始化CPU寄存器
    pcb->eax = 0;
    pcb->ebx = 0;
    pcb->ecx = 0;
    pcb->edx = 0;
    pcb->eip = 0x1000;  // 假设进程从0x1000开始执行
    pcb->esp = 0x2000;  // 假设栈指针
    pcb->ebp = 0x2000;
    pcb->eflags = 0x200;
    pcb->cs = 0x08;
    pcb->ds = 0x10;
    pcb->es = 0x10;
    pcb->ss = 0x10;
    
    // 初始化PCB中的进程信息
    pcb->start_time = 0;
    pcb->wait_time = 0;
    pcb->turnaround_time = 0;
    pcb->response_time = 0;
    pcb->executed_time = 0;
}

// 保存进程上下文
void save_cpu_context(pcb_t* pcb) {
    if (!pcb) return;
    
    // 保存当前CPU上下文到PCB
    pcb->eax = current_cpu_context.eax;
    pcb->ebx = current_cpu_context.ebx;
    pcb->ecx = current_cpu_context.ecx;
    pcb->edx = current_cpu_context.edx;
    pcb->eip = current_cpu_context.eip;
    pcb->esp = current_cpu_context.esp;
    pcb->ebp = current_cpu_context.ebp;
    pcb->eflags = current_cpu_context.eflags;
    pcb->cs = current_cpu_context.cs;
    pcb->ds = current_cpu_context.ds;
    pcb->es = current_cpu_context.es;
    pcb->ss = current_cpu_context.ss;
    
    // 记录执行历史
    save_execution_history(pcb->base.pid, pcb->base.state, 
                          pcb->base.remaining_time, pcb->executed_time);
    
    DEBUG_PRINT("Saved context for process %d", pcb->base.pid);
}

// 恢复进程上下文
void restore_cpu_context(pcb_t* pcb) {
    if (!pcb) return;
    
    // 从PCB恢复CPU上下文
    current_cpu_context.eax = pcb->eax;
    current_cpu_context.ebx = pcb->ebx;
    current_cpu_context.ecx = pcb->ecx;
    current_cpu_context.edx = pcb->edx;
    current_cpu_context.eip = pcb->eip;
    current_cpu_context.esp = pcb->esp;
    current_cpu_context.ebp = pcb->ebp;
    current_cpu_context.eflags = pcb->eflags;
    current_cpu_context.cs = pcb->cs;
    current_cpu_context.ds = pcb->ds;
    current_cpu_context.es = pcb->es;
    current_cpu_context.ss = pcb->ss;
    
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
                        save_cpu_context(current_pcb);
                        
                        // 将当前进程放回就绪队列
                        process_set_state(&current_pcb->base, PROC_READY);
                        scheduler_add_process(&current_pcb->base);
                        
                        g_scheduler.current_process = NULL;
                    }
                }
            } else {
                // 进程完成
                save_cpu_context(current_pcb);
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
            restore_cpu_context(current_pcb);
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
        kernel_log(LOG_ERR, "Cannot open file %s for writing", filename);
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
        kernel_log(LOG_ERR, "Cannot open file %s for reading", filename);
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
    printf("================================\n");
}

// 创建进程（自动模式）
pcb_t* create_auto_process(const char* name, uint32_t burst_time, uint32_t memory_size) {
    if (total_processes >= MAX_PROCESSES) {
        kernel_log(LOG_ERR, "Maximum number of processes reached");
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
    if (allocate_memory(&pcb->base, FIRST_FIT) != 0) {
        kernel_log(LOG_ERR, "Failed to allocate memory for process %s", name);
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
        kernel_log(LOG_ERR, "Maximum number of processes reached");
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
    if (allocate_memory(&pcb->base, FIRST_FIT) != 0) {
        kernel_log(LOG_ERR, "Failed to allocate memory for process %s", name);
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
int kernel_main() {
    // 初始化系统
    kernel_init();
    
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
                
                if (system_memory) {
                    free(system_memory);
                    system_memory = NULL;
                }
                
                return 0;
                
            default:
                printf("Invalid command. Press H/h for help.\n");
                break;
        }
    }
    
    return 0;
}