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

// 常量
#define TRUE 1
#define FALSE 0
#define MAX_MEMORY_SIZE 1024    // 1KB内存
#define MAX_PARTITIONS 16       // 最大分区数
#define MAX_PROCESSES 32        // 最大进程数

// 内存操作函数 (声明但不定义，避免与标准库冲突)
void* memset(void* dst, int val, size_t count);
void* memcpy(void* dst, const void* src, size_t count);
int memcmp(const void* s1, const void* s2, size_t n);
size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);

#endif // _OS_TYPES_H