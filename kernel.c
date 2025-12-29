#include "kernel.h"
#include "process.h"
#include "scheduler.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

// 全局内核状态
KernelState kernel_state = {0};

// 定时器相关
#ifdef _WIN32
static HANDLE timer_thread = NULL;
#else
static pthread_t timer_thread;
#endif
static int timer_running = 0;

// 模拟CPU上下文保存和恢复
void cpu_context_save(CPUContext* context) {
    if(context != NULL) {
        // 在实际系统中，这些值会从CPU寄存器读取
        // 这里我们只是模拟
        context->eax = 0x12345678;
        context->ebx = 0xABCDEF00;
        context->ecx = 0x98765432;
        context->edx = 0xFEDCBA00;
        context->esi = 0x11111111;
        context->edi = 0x22222222;
        context->ebp = 0x33333333;
        context->esp = 0x44444444;
        context->eip = 0x55555555;
        context->eflags = 0x00000202; // 标准EFLAGS值
        
        printf("CPU上下文已保存\n");
    }
}

void cpu_context_restore(CPUContext* context) {
    if(context != NULL) {
        // 在实际系统中，这些值会写入CPU寄存器
        // 这里我们只是模拟
        printf("CPU上下文已恢复: EAX=%08X, ESP=%08X, EIP=%08X\n", 
               context->eax, context->esp, context->eip);
    }
}

// 中断处理函数
void interrupt_handler(int interrupt_number) {
    switch(interrupt_number) {
        case TIMER_INTERRUPT:
            timer_interrupt_handler();
            break;
        case KEYBOARD_INTERRUPT:
            printf("键盘中断处理\n");
            break;
        case SYSTEM_CALL:
            printf("系统调用中断处理\n");
            break;
        default:
            printf("未知中断: %d\n", interrupt_number);
    }
}

// 定时器中断处理
void timer_interrupt_handler() {
    kernel_state.system_time++;
    
    // 执行时间片轮转调度
    if(kernel_state.interrupt_enabled) {
        // 检查当前进程是否需要切换
        schedule_next();
    }
}

// 启用中断
void enable_interrupts() {
    kernel_state.interrupt_enabled = 1;
    printf("中断已启用\n");
}

// 禁用中断
void disable_interrupts() {
    kernel_state.interrupt_enabled = 0;
    printf("中断已禁用\n");
}

// 定时器中断设置
void timer_interrupt_setup() {
    kernel_state.timer_interval = 1000; // 1秒
    printf("定时器中断已设置，间隔: %d ms\n", kernel_state.timer_interval);
}

#ifdef _WIN32
// Windows定时器线程函数
DWORD WINAPI timer_thread_proc(LPVOID param) {
    while(timer_running) {
        Sleep(kernel_state.timer_interval); // 等待指定时间
        interrupt_handler(TIMER_INTERRUPT); // 触发定时器中断
    }
    return 0;
}
#else
// Linux定时器线程函数
void* timer_thread_proc(void* param) {
    while(timer_running) {
        usleep(kernel_state.timer_interval * 1000); // 等待指定时间 (转换为微秒)
        interrupt_handler(TIMER_INTERRUPT); // 触发定时器中断
    }
    return NULL;
}
#endif

// 内核初始化
int kernel_init() {
    // 初始化内核状态
    kernel_state.initialized = 1;
    kernel_state.system_time = 0;
    kernel_state.interrupt_enabled = 1;
    
    // 初始化CPU上下文
#ifdef _WIN32
    ZeroMemory(&kernel_state.current_context, sizeof(CPUContext));
#else
    memset(&kernel_state.current_context, 0, sizeof(CPUContext));
#endif
    
    // 设置定时器
    timer_interrupt_setup();
    
    // 启动定时器线程
    timer_running = 1;
#ifdef _WIN32
    timer_thread = CreateThread(NULL, 0, timer_thread_proc, NULL, 0, NULL);
    
    if(timer_thread == NULL) {
        printf("错误: 无法创建定时器线程\n");
        return 0;
    }
#else
    if(pthread_create(&timer_thread, NULL, timer_thread_proc, NULL) != 0) {
        printf("错误: 无法创建定时器线程\n");
        return 0;
    }
#endif
    
    printf("内核初始化完成\n");
    return 1;
}

// 内核清理
void kernel_cleanup() {
    // 停止定时器线程
    timer_running = 0;
    
#ifdef _WIN32
    if(timer_thread != NULL) {
        WaitForSingleObject(timer_thread, INFINITE);
        CloseHandle(timer_thread);
        timer_thread = NULL;
    }
#else
    // 等待线程结束
    pthread_join(timer_thread, NULL);
#endif
    
    kernel_state.initialized = 0;
    printf("内核已清理\n");
}

// 获取系统时间
int get_system_time() {
    return kernel_state.system_time;
}