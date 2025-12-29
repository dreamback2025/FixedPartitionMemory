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

// 进程控制块扩展，包含CPU上下文
typedef struct {
    process_t base;                 // 基础进程结构
    uint32_t eax, ebx, ecx, edx;    // 通用寄存器
    uint32_t eip;                   // 指令指针
    uint32_t esp;                   // 栈指针
    uint32_t ebp;                   // 基址指针
    uint32_t eflags;                // 标志寄存器
    uint32_t cs, ds, es, ss;        // 段寄存器
    uint32_t start_time;            // 进程开始执行时间
    uint32_t wait_time;             // 等待时间
    uint32_t turnaround_time;       // 周转时间
    uint32_t response_time;         // 响应时间
    uint32_t executed_time;         // 已执行时间
} pcb_t;

// 全局变量声明（extern）
extern process_t process_table[MAX_PROCESSES];
extern pcb_t pcb_table[MAX_PROCESSES];

// 内核API
void process_init(void);
process_t* create_process(uint32_t pid, const char* name, uint32_t memory_size,
    uint32_t burst_time, uint32_t arrival_time);
process_t* find_process_by_pid(uint32_t pid);
void terminate_process(process_t* proc);
void process_set_state(process_t* proc, process_state_t new_state);
void dump_process_info(process_t* proc);

// CPU上下文管理API
void save_cpu_context(pcb_t* pcb);
void restore_cpu_context(pcb_t* pcb);
void initialize_cpu_context(pcb_t* pcb);

#endif // _PROCESS_H