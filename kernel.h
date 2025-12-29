#ifndef KERNEL_H
#define KERNEL_H

#ifdef _WIN32
#include <windows.h>
#else
// 为非Windows平台定义DWORD类型
typedef unsigned long DWORD;
#endif

// CPU寄存器状态结构 - 模拟CPU现场
typedef struct {
    DWORD eax;    // 累加器寄存器
    DWORD ebx;    // 基址寄存器
    DWORD ecx;    // 计数寄存器
    DWORD edx;    // 数据寄存器
    DWORD esi;    // 源索引寄存器
    DWORD edi;    // 目标索引寄存器
    DWORD ebp;    // 基址指针
    DWORD esp;    // 栈指针
    DWORD eip;    // 指令指针
    DWORD eflags; // 标志寄存器
} CPUContext;

// 中断相关定义
typedef enum {
    TIMER_INTERRUPT = 0x20,      // 定时器中断
    KEYBOARD_INTERRUPT = 0x21,   // 键盘中断
    SYSTEM_CALL = 0x80           // 系统调用中断
} InterruptType;

// 中断处理函数指针
typedef void (*InterruptHandler)(int interrupt_number);

// 内核状态结构
typedef struct {
    int initialized;              // 内核是否已初始化
    int system_time;             // 系统时间
    int interrupt_enabled;       // 中断是否启用
    CPUContext current_context;  // 当前CPU上下文
    int timer_interval;          // 定时器间隔（毫秒）
} KernelState;

// 全局内核状态
extern KernelState kernel_state;

// 内核函数声明
int kernel_init();
void kernel_cleanup();
void cpu_context_save(CPUContext* context);
void cpu_context_restore(CPUContext* context);
void interrupt_handler(int interrupt_number);
void enable_interrupts();
void disable_interrupts();
void timer_interrupt_setup();
void timer_interrupt_handler();
int get_system_time();

#endif // KERNEL_H