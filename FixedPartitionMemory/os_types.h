#ifndef _OS_TYPES_H
#define _OS_TYPES_H

// 首先包含标准头文件，避免重定义
#include <stddef.h>  // 包含 size_t
#include <stdint.h>  // 包含标准整数类型

// 基础类型 (使用标准类型，不重新定义)
typedef uint8_t uint8_t;
typedef uint16_t uint16_t;
typedef uint32_t uint32_t;
typedef uint64_t uint64_t;
typedef int8_t int8_t;
typedef int16_t int16_t;
typedef int32_t int32_t;
typedef int64_t int64_t;

// 使用标准NULL定义
#include <stddef.h>

// 定义布尔类型
typedef int BOOL;
#define TRUE 1
#define FALSE 0

// 常量
#define MAX_MEMORY_SIZE 1024    // 1KB内存
#define MAX_PARTITIONS 16       // 最大分区数
#define MAX_PROCESSES 32        // 最大进程数

// 用于键盘输入检测的函数声明
#ifdef __linux__
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

static struct termios orig_termios;

// 设置终端为非规范模式
static void init_termios(void) {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

// 恢复原始终端设置
static void reset_termios(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

// 检查是否有键盘输入
static int kbhit(void) {
    fd_set read_fds;
    struct timeval timeout;
    
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    return select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout) > 0;
}
#else
// Windows版本的kbhit
int kbhit(void);
#endif

// 内存操作函数 (声明但不定义，避免与标准库冲突)
void* memset(void* dst, int val, size_t count);
void* memcpy(void* dst, const void* src, size_t count);
int memcmp(const void* s1, const void* s2, size_t n);
size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);

#endif // _OS_TYPES_H