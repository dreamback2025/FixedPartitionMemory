#ifndef _KERNEL_H
#define _KERNEL_H

#include "os_types.h"
#include "log.h"
#include "process.h"
#include "partition.h"
#include "memory.h"
#include "scheduler.h"

// Global scheduler variable declaration
extern scheduler_t g_scheduler;

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

// CPU context management
void save_cpu_context(pcb_t* pcb);
void restore_cpu_context(pcb_t* pcb);
void initialize_cpu_context(pcb_t* pcb);

// Process creation functions
pcb_t* create_auto_process(const char* name, uint32_t burst_time, uint32_t memory_size);
pcb_t* create_manual_process(void);

// Interrupt and scheduling
void simulate_interrupt(void);
void scheduler_schedule(void);

// Execution history functions
void save_execution_to_file(const char* filename);
void load_execution_from_file(const char* filename);
void replay_execution(void);

// System status functions
void print_system_status(void);
void show_menu(void);

// Main kernel function
int kernel_main(void);

#endif // _KERNEL_H