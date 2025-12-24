#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>  // 添加unistd.h用于usleep函数

// 包含内核头文件
#include "os_types.h"
#include "memory.h"
#include "process.h"
#include "partition.h"
#include "log.h"
#include "config.h"
#include "kernel.h"
#include "scheduler.h"

// 全局变量
static BOOL use_timer = FALSE;
static BOOL running = TRUE;
static uint32_t simulated_time = 0;
extern allocation_strategy_t current_strategy;
extern scheduler_t g_scheduler;

FILE* log_file = NULL;

// 日志函数
void init_logging() {
    // 创建日志文件名 (包含时间戳)
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char filename[50];
    strftime(filename, sizeof(filename), "memory_log_%Y%m%d_%H%M%S.txt", t);

    // 打开日志文件
    log_file = fopen(filename, "w");
    if (log_file) {
        printf("日志已保存到: %s\n", filename);
    }
    else {
        printf("无法创建日志文件!\n");
    }
}

void close_logging() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

// 核心日志打印函数
void log_printf(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // 打印到控制台
    printf("%s", buffer);

    // 打印到日志文件
    if (log_file) {
        fprintf(log_file, "%s", buffer);
        fflush(log_file);  // 确保内容立即写入文件
    }
}

// 自定义清屏函数
void log_clear_screen() {
    system("clear");  // Linux系统使用clear命令
    if (log_file) {
        fprintf(log_file, "\n--- 屏幕已清空 (时间: %d) ---\n", simulated_time);
        fflush(log_file);
    }
}

// 用于键盘输入的函数（Linux兼容版本）
char get_char_input() {
    char input[2];
    fgets(input, sizeof(input), stdin);
    // 消费剩余字符直到换行符
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    return input[0];
}

int get_int_input() {
    int value;
    scanf("%d", &value);
    // 消费剩余字符直到换行符
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    return value;
}

// 生成自动进程
void generate_auto_processes(int count) {
    srand((unsigned)time(NULL));

    for (int i = 0; i < count; i++) {
        char name[16];
        sprintf(name, "自动进程%d", i + 1);
        uint32_t memory_size = (rand() % 128) + 32; // 32-159 bytes
        uint32_t burst_time = (rand() % 10) + 1;   // 1-10 units
        uint32_t arrival_time = rand() % 5;        // 0-4

        process_t* proc = create_process(0, name, memory_size, burst_time, arrival_time);
        if (proc) {
            log_printf("创建自动进程: %s, 内存=%d, 时间=%d, 到达=%d\n",
                name, memory_size, burst_time, arrival_time);
        }
    }
}

// 生成手动进程
void generate_manual_processes() {
    int count;
    log_printf("请输入进程数量 (1-%d): ", MAX_PROCESSES);
    count = get_int_input();

    if (count < 1) count = 1;
    if (count > MAX_PROCESSES) count = MAX_PROCESSES;

    for (int i = 0; i < count; i++) {
        char name[16];
        uint32_t memory_size, burst_time, arrival_time;

        log_printf("\n进程 %d:\n", i + 1);
        log_printf("名称: ");
        fgets(name, 16, stdin);
        name[strcspn(name, "\n")] = '\0';
        if (strlen(name) == 0) sprintf(name, "手动进程%d", i + 1);

        log_printf("内存大小 (字节): ");
        memory_size = get_int_input();

        log_printf("执行时间 (单位): ");
        burst_time = get_int_input();

        log_printf("到达时间: ");
        arrival_time = get_int_input();

        process_t* proc = create_process(0, name, memory_size, burst_time, arrival_time);
        if (proc) {
            log_printf("创建手动进程: %s, 内存=%d, 时间=%d, 到达=%d\n",
                name, memory_size, burst_time, arrival_time);
        }
    }
}

// 显示系统状态
void display_system_status() {
    log_clear_screen();
    log_printf("=== 固定分区内存管理系统 ===\n");
    log_printf("当前时间: %d\n", simulated_time);
    log_printf("分配策略: ");
    switch (current_strategy) {
    case FIRST_FIT: log_printf("首次适应算法\n"); break;
    case BEST_FIT: log_printf("最佳适应算法\n"); break;
    case WORST_FIT: log_printf("最坏适应算法\n"); break;
    }
    log_printf("运行模式: %s\n", use_timer ? "自动模式" : "手动模式");

    // 显示内存映射 (安全遍历)
    log_printf("\n--- 内存映射 ---\n");
    log_printf("起始地址  结束地址  大小    状态      所有者PID\n");

    // 从第一个分区开始遍历
    partition_t* current = &partition_table[0];
    int count = 0;

    while (current && count < MAX_PARTITIONS) {
        const char* state_str;
        switch (current->state) {
        case PARTITION_FREE: state_str = "空闲"; break;
        case PARTITION_ALLOCATED: state_str = "已分配"; break;
        case PARTITION_OS: state_str = "操作系统"; break;
        default: state_str = "未知";
        }

        log_printf("0x%04x   0x%04x   %4d    %-8s    %d\n",
            current->start,
            current->start + current->size - 1,
            current->size,
            state_str,
            current->owner_pid);

        // 安全地移动到下一个分区
        current = current->next;
        count++;

        // 防止无限循环
        if (count >= MAX_PARTITIONS) {
            log_printf("警告: 分区遍历达到最大限制，可能存在链表错误\n");
            break;
        }
    }

    // 显示进程状态
    log_printf("\n--- 进程状态 ---\n");
    log_printf("PID  名称           状态      内存大小  剩余时间  到达时间\n");

    extern process_t process_table[MAX_PROCESSES];
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        process_t* proc = &process_table[i];
        if (proc->state != PROC_TERMINATED) {
            const char* state_str;
            switch (proc->state) {
            case PROC_CREATED: state_str = "已创建"; break;
            case PROC_READY: state_str = "就绪"; break;
            case PROC_RUNNING: state_str = "运行中"; break;
            case PROC_WAITING: state_str = "等待"; break;
            case PROC_TERMINATED: state_str = "终止"; break;
            default: state_str = "未知";
            }

            log_printf("%-4d %-12s  %-8s  %4d    %4d       %4d\n",
                proc->pid, proc->name, state_str,
                proc->memory_size, proc->remaining_time, proc->arrival_time);
        }
    }

    // 显示内存统计
    uint32_t total_free = get_total_free_memory();
    uint32_t largest_block = get_largest_free_block();
    uint32_t total_used = MEMORY_SIZE - OS_PARTITION_SIZE - total_free;

    log_printf("\n--- 内存统计 ---\n");
    log_printf("总内存: %d 字节\n", MEMORY_SIZE);
    log_printf("操作系统占用: %d 字节\n", OS_PARTITION_SIZE);
    log_printf("总空闲内存: %d 字节 (%.1f%%)\n",
        total_free, (float)total_free * 100 / (MEMORY_SIZE - OS_PARTITION_SIZE));
    log_printf("总已用内存: %d 字节 (%.1f%%)\n",
        total_used, (float)total_used * 100 / (MEMORY_SIZE - OS_PARTITION_SIZE));
    log_printf("最大空闲块: %d 字节\n", largest_block);

    // 显示调度器状态
    log_printf("\n--- 调度器状态 ---\n");
    log_printf("调度算法: %s\n", 
              g_scheduler.type == SCHED_FIFO ? "先进先出(FIFO)" :
              g_scheduler.type == SCHED_RR ? "时间片轮转(RR)" : "优先级调度");
    log_printf("就绪队列进程数: %d\n", g_scheduler.ready_queue.count);
    log_printf("当前运行进程: %s\n", 
              g_scheduler.current_process ? 
              g_scheduler.current_process->name : "无");
    log_printf("当前时间片: %d/%d\n", 
              g_scheduler.current_time_slice, g_scheduler.time_slice);
    log_printf("Q=退出, C=内存紧凑, F=首次适应, B=最佳适应, W=最坏适应\n");
    log_printf("按任意键继续...\n");
}

int main() {
    // 初始化日志
    init_logging();

    log_printf("=== 固定分区内存管理系统 ===\n");

    // 获取当前时间
    time_t now = time(NULL);
    char* time_str = ctime(&now);
    log_printf("程序启动时间: %s", time_str);

    // 初始化内核
    kernel_init();
    
    // 初始化调度器（使用时间片轮转算法）
    scheduler_init(SCHED_RR);

    // 选择进程生成方式
    log_printf("\n请选择进程生成方式:\n");
    log_printf("1. 自动生成进程\n");
    log_printf("2. 手动输入进程\n");
    log_printf("请选择: ");

    char choice = get_char_input();
    log_printf("\n");

    switch (choice) {
    case '1':
        log_printf("请输入进程数量 (1-10): ");
        int count = get_int_input();
        if (count < 1) count = 1;
        if (count > 10) count = 10;
        generate_auto_processes(count);
        break;
    case '2':
        generate_manual_processes();
        break;
    default:
        log_printf("无效选择，使用默认5个进程\n");
        generate_auto_processes(5);
    }

    // 选择时间推进方式
    log_printf("\n请选择时间推进方式:\n");
    log_printf("1. 手动模式 (按任意键推进时间)\n");
    log_printf("2. 自动模式 (定时器)\n");
    log_printf("请选择: ");

    choice = get_char_input();
    log_printf("\n");

    if (choice == '1') {
        use_timer = FALSE;
        log_printf("手动模式已选择. 按任意键推进时间.\n");
    }
    else if (choice == '2') {
        use_timer = TRUE;
        log_printf("自动模式已选择. 定时器间隔: %dms\n", TIMER_INTERVAL);
    }
    else {
        log_printf("无效选择，使用手动模式\n");
        use_timer = FALSE;
    }

    // 显示初始状态
    display_system_status();

    if (use_timer) {
        // 自动模式 - 使用sleep代替WM_TIMER
        running = TRUE;
        log_printf("自动模式开始. 按 'q' 退出, 'c' 进行内存紧凑...\n");

        uint32_t last_display_time = 0;
        while (running) {
            // 处理键盘输入 - Linux下使用非阻塞输入
            if (kbhit()) {
                char key = get_char_input();
                if (key == 'q' || key == 'Q') {
                    running = FALSE;
                    break;
                }
                else if (key == 'c' || key == 'C') {
                    compact_memory();
                    display_system_status();
                    last_display_time = simulated_time;
                    continue;
                }
                else if (key == 'f' || key == 'F') {
                    current_strategy = FIRST_FIT;
                }
                else if (key == 'b' || key == 'B') {
                    current_strategy = BEST_FIT;
                }
                else if (key == 'w' || key == 'W') {
                    current_strategy = WORST_FIT;
                }
            }

            // 每次循环推进一单位时间
            simulated_time++;

            // 检查新到达的进程
            for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
                process_t* proc = &process_table[i];
                if (proc->state == PROC_CREATED && proc->arrival_time <= simulated_time) {
                    log_printf("\n进程 %s (PID=%d) 在时间 %d 到达\n",
                        proc->name, proc->pid, simulated_time);

                    // 尝试分配内存
                    if (allocate_memory(proc, current_strategy) == 0) {
                        log_printf("\n内存已分配给进程 %s\n", proc->name);
                        scheduler_add_process(proc);  // 添加到调度器
                    }
                    else {
                        log_printf("暂时无法为进程 %s 分配内存，将在下次尝试\n", proc->name);
                        // 保持PROC_CREATED状态，下次时间点再尝试
                    }
                }
            }

            // 执行调度
            scheduler_schedule();
            scheduler_run_current_process();

            // 每5个时间单位显示一次系统状态
            if (simulated_time - last_display_time >= 5 || simulated_time < 10) {
                display_system_status();
                last_display_time = simulated_time;
            }

            // 检查是否所有进程都已完成
            int all_completed = 1;
            for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
                process_t* proc = &process_table[i];
                if (proc->state != PROC_TERMINATED && proc->state != PROC_CREATED) {
                    all_completed = 0;
                    break;
                }
            }

            if (all_completed && simulated_time > 20) {
                log_printf("\n所有进程已完成！按任意键退出...\n");
                get_char_input();
                running = FALSE;
                break;
            }

            // 控制模拟速度 - 使用Linux下的usleep
            usleep(TIMER_INTERVAL * 1000);  // 转换为微秒
        }
    }
    else {
        // 手动模式
        while (running) {
            log_printf("\n按任意键推进时间 (Q=退出, C=内存紧凑): ");
            char key = get_char_input();

            if (key == 'q' || key == 'Q') {
                break;
            }
            else if (key == 'c' || key == 'C') {
                compact_memory();
                display_system_status();
                continue;
            }
            else if (key == 'f' || key == 'F') {
                current_strategy = FIRST_FIT;
            }
            else if (key == 'b' || key == 'B') {
                current_strategy = BEST_FIT;
            }
            else if (key == 'w' || key == 'W') {
                current_strategy = WORST_FIT;
            }

            simulated_time++;

            // 检查新到达的进程
            extern process_t process_table[MAX_PROCESSES];
            for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
                process_t* proc = &process_table[i];
                if (proc->state == PROC_CREATED && proc->arrival_time <= simulated_time) {
                    log_printf("\n进程 %s (PID=%d) 在时间 %d 到达\n",
                        proc->name, proc->pid, simulated_time);

                    if (allocate_memory(proc, current_strategy) == 0) {
                        log_printf("内存已分配给进程 %s\n", proc->name);
                        scheduler_add_process(proc);  // 添加到调度器
                    }
                    else {
                        log_printf("无法为进程 %s 分配内存\n", proc->name);
                    }
                }
            }

            // 执行调度
            scheduler_schedule();
            scheduler_run_current_process();

            display_system_status();
        }
    }

    // 程序结束
    now = time(NULL);
    time_str = ctime(&now);
    log_printf("\n程序结束时间: %s", time_str);
    log_printf("按任意键退出...\n");
    get_char_input();

    // 关闭日志
    close_logging();
    return 0;
}