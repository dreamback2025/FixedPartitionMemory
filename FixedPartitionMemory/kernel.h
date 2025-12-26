#ifndef _KERNEL_H
#define _KERNEL_H

#include "os_types.h"
#include "log.h"
#include "process.h"
#include "partition.h"
#include "memory.h"

// Kernel initialization function
void kernel_init(void);

// Get current time
uint32_t get_current_time(void);

// Advance time
void advance_time(void);

// Get memory pointer
uint8_t* get_memory_base(void);

// Get memory size
uint32_t get_memory_size(void);

#endif // _KERNEL_H