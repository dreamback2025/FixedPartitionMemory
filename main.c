#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#endif

#include "kernel.h"
#include "process.h"
#include "scheduler.h"

// 全局变量
int g_time_counter = 0;
int g_simulation_running = 1;
int g_manual_input_mode = 0;

// 函数声明
void print_menu();
void handle_manual_input();
void handle_auto_input();
void run_simulation();
void save_execution_log();
void load_and_replay();

int main() {
    printf("=== 多道程序设计技术 - 进程调度模拟系统 ===\n");
    printf("操作系统内核多任务并发运行机制演示\n\n");
    
    // 初始化系统
    kernel_init();
    
    while(g_simulation_running) {
        print_menu();
        
        char choice;
        scanf(" %c", &choice);
        getchar(); // consume newline
        
        switch(choice) {
            case '1':
                handle_manual_input();
                break;
            case '2':
                handle_auto_input();
                break;
            case '3':
                run_simulation();
                break;
            case '4':
                save_execution_log();
                break;
            case '5':
                load_and_replay();
                break;
            case '6':
                g_simulation_running = 0;
                printf("系统退出。\n");
                break;
            case '7':
                print_process_table();
                break;
            default:
                printf("无效选择，请重新输入。\n");
        }
    }
    
    kernel_cleanup();
    return 0;
}

void print_menu() {
    printf("\n=== 菜单 ===\n");
    printf("1. 手工输入进程\n");
    printf("2. 自动产生进程\n");
    printf("3. 运行模拟 (按任意键推进时间)\n");
    printf("4. 保存执行日志\n");
    printf("5. 读取并重放\n");
    printf("6. 退出\n");
    printf("7. 查看进程表\n");
    printf("请选择操作: ");
}

void handle_manual_input() {
    printf("手工输入进程模式\n");
    printf("请输入进程信息 (格式: ID 名称 优先级 执行时间, 输入-1结束):\n");
    
    int id, priority, exec_time;
    char name[50];
    
    while(1) {
        printf("进程信息: ");
        if(scanf("%d", &id) != 1) {
            break;
        }
        
        if(id == -1) break;
        
        scanf("%s %d %d", name, &priority, &exec_time);
        
        if(create_process(id, name, priority, exec_time)) {
            printf("进程 %d 创建成功\n", id);
        } else {
            printf("进程 %d 创建失败\n", id);
        }
    }
    
    // 清空输入缓冲区
    while(getchar() != '\n');
}

void handle_auto_input() {
    printf("自动产生进程模式\n");
    printf("请输入要创建的进程数量: ");
    
    int count;
    scanf("%d", &count);
    
    srand((unsigned int)time(NULL));
    
    for(int i = 0; i < count; i++) {
        int id = 100 + i;
        char name[50];
        sprintf(name, "Proc_%d", id);
        int priority = rand() % 5 + 1;  // 1-5
        int exec_time = rand() % 10 + 1;  // 1-10
        
        if(create_process(id, name, priority, exec_time)) {
            printf("自动创建进程 %d: %s (优先级: %d, 执行时间: %d)\n", id, name, priority, exec_time);
        }
    }
    
    printf("自动创建了 %d 个进程\n", count);
}

#ifdef _WIN32
int kbhit() {
    return _kbhit();
}

char getch_wrapper() {
    return _getch();
}
#else
// Linux下的键盘检测函数
int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    
    return 0;
}

char getch_wrapper() {
    char ch = getchar();
    return ch;
}
#endif

void run_simulation() {
    if(get_process_count() == 0) {
        printf("没有进程可运行，请先创建进程。\n");
        return;
    }
    
    printf("开始模拟... 按任意键推进一个时间单位，按 'q' 退出模拟。\n");
    
    while(1) {  // 持续运行直到用户选择退出
        if(kbhit()) {
            char ch = getch_wrapper();
            if(ch == 'q' || ch == 'Q') {
                break;
            }
        }
        
        // 推进一个时间单位
        g_time_counter++;
        printf("\n=== 时间单位 %d ===\n", g_time_counter);
        
        // 执行调度
        schedule_next();
        
        // 打印当前状态
        print_process_table();
        
        // 简单延迟以控制速度
#ifdef _WIN32
        Sleep(1000);
#else
        usleep(1000000); // 1秒 = 1,000,000微秒
#endif
    }
}

void save_execution_log() {
    FILE* file = fopen("execution_log.txt", "w");
    if(file == NULL) {
        printf("无法创建日志文件\n");
        return;
    }
    
    fprintf(file, "执行日志 - 时间: %d\n", g_time_counter);
    fprintf(file, "进程总数: %d\n", get_process_count());
    fprintf(file, "\n进程执行情况:\n");
    
    // 遍历并保存所有进程的状态
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(process_table[i].state != EMPTY) {
            fprintf(file, "ID: %d, Name: %s, State: %d, Priority: %d, ExecTime: %d, Remaining: %d\n",
                   process_table[i].id, process_table[i].name, 
                   process_table[i].state, process_table[i].priority,
                   process_table[i].exec_time, process_table[i].remaining_time);
        }
    }
    
    fclose(file);
    printf("执行日志已保存到 execution_log.txt\n");
}

void load_and_replay() {
    FILE* file = fopen("execution_log.txt", "r");
    if(file == NULL) {
        printf("无法打开日志文件进行重放\n");
        return;
    }
    
    printf("=== 重放执行日志 ===\n");
    
    char line[256];
    while(fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }
    
    fclose(file);
    printf("\n重放完成\n");
}