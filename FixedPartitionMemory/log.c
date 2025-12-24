#include "os_types.h"
#include "log.h"
#include "config.h"
#include <stdarg.h>

static char log_buffer[LOG_BUFFER_SIZE];
static uint32_t log_buffer_pos = 0;

// 简单的内存复制
void* memcpy(void* dst, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

// 简单的内存设置
void* memset(void* dst, int val, size_t n) {
    uint8_t* d = (uint8_t*)dst;
    while (n--) {
        *d++ = (uint8_t)val;
    }
    return dst;
}

// 字符串长度
size_t strlen(const char* str) {
    const char* s = str;
    while (*s) s++;
    return (size_t)(s - str);
}

// 字符串复制
char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

// 整数转字符串
static char* itoa(int num, char* str, int base) {
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    int tmp_value;
    int neg = 0;

    if (num == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return str;
    }

    if (num < 0 && base == 10) {
        neg = 1;
        num = -num;
    }

    while (num) {
        tmp_value = num % base;
        if (tmp_value < 10)
            tmp_char = '0' + tmp_value;
        else
            tmp_char = 'A' + (tmp_value - 10);
        *ptr++ = tmp_char;
        num /= base;
    }

    if (neg) {
        *ptr++ = '-';
    }
    *ptr-- = '\0';

    while (ptr1 < ptr) {
        tmp_char = *ptr1;
        *ptr1++ = *ptr;
        *ptr-- = tmp_char;
    }

    return str;
}

// 内核日志初始化
void kernel_log_init(void) {
    log_buffer_pos = 0;
    memset(log_buffer, 0, LOG_BUFFER_SIZE);
}

// 内核日志函数 (简化版)
void kernel_log(log_level_t level, const char* fmt, ...) {
    static const char* level_str[] = {
        "EMERG", "ALERT", "CRIT", "ERR", "WARN", "NOTE", "INFO", "DEBUG"
    };

    if (level > KERNEL_LOG_LEVEL) return;

    // 构建日志行
    char log_line[256];
    uint32_t pos = 0;

    // 添加级别
    log_line[pos++] = '[';
    const char* lvl = level_str[level];
    while (*lvl && pos < 250) {
        log_line[pos++] = *lvl++;
    }
    log_line[pos++] = ']';
    log_line[pos++] = ' ';

    // 处理格式化字符串
    va_list args;
    va_start(args, fmt);

    uint32_t fmt_pos = 0;
    while (fmt[fmt_pos] && pos < 250) {
        if (fmt[fmt_pos] == '%' && fmt[fmt_pos + 1]) {
            fmt_pos++;
            if (fmt[fmt_pos] == 'd') {
                int num = va_arg(args, int);
                char num_str[12];
                itoa(num, num_str, 10);
                char* p = num_str;
                while (*p && pos < 250) {
                    log_line[pos++] = *p++;
                }
            }
            else if (fmt[fmt_pos] == 's') {
                const char* str = va_arg(args, const char*);
                while (*str && pos < 250) {
                    log_line[pos++] = *str++;
                }
            }
            else if (fmt[fmt_pos] == 'x') {
                unsigned int num = va_arg(args, unsigned int);
                char num_str[12];
                itoa(num, num_str, 16);
                char* p = num_str;
                while (*p && pos < 250) {
                    log_line[pos++] = *p++;
                }
            }
            else {
                log_line[pos++] = '%';
                log_line[pos++] = fmt[fmt_pos];
            }
        }
        else {
            log_line[pos++] = fmt[fmt_pos];
        }
        fmt_pos++;
    }

    va_end(args);

    log_line[pos++] = '\n';
    log_line[pos] = '\0';

    // 添加到日志缓冲区
    if (log_buffer_pos + pos < LOG_BUFFER_SIZE) {
        memcpy(log_buffer + log_buffer_pos, log_line, pos);
        log_buffer_pos += pos;
    }
}

// 内核panic
void kernel_panic(const char* msg) {
    kernel_log(LOG_EMERG, "KERNEL PANIC: %s", msg);

    // 在真实内核中，这里会停止系统
    while (1) {
        // 死循环
    }
}