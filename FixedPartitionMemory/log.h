#ifndef _LOG_H
#define _LOG_H

#include "os_types.h"

typedef enum {
    LOG_EMERG,
    LOG_ALERT,
    LOG_CRIT,
    LOG_ERR,
    LOG_WARNING,
    LOG_NOTICE,
    LOG_INFO,
    LOG_DEBUG
} log_level_t;

// 内核日志函数
void kernel_log_init(void);
void kernel_log(log_level_t level, const char* fmt, ...);
void kernel_panic(const char* msg);

#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) kernel_log(LOG_DEBUG, "[DEBUG] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

#endif // _LOG_H