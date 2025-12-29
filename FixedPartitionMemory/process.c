#include "os_types.h"
#include "log.h"
#include "process.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>

// 定义全局变量
process_t process_table[MAX_PROCESSES];
pcb_t pcb_table[MAX_PROCESSES];
static uint32_t next_pid = 1;

void process_init(void) {
    uint32_t i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].state = PROC_TERMINATED;
        process_table[i].next = NULL;
        
        // 初始化PCB
        pcb_table[i].base.pid = 0;
        pcb_table[i].base.state = PROC_TERMINATED;
        pcb_table[i].base.next = NULL;
    }
    next_pid = 1;
    DEBUG_PRINT("Process table initialized");
}

process_t* create_process(uint32_t pid, const char* name, uint32_t memory_size,
    uint32_t burst_time, uint32_t arrival_time) {
    uint32_t i;
    uint32_t name_len;
    process_t* proc;

    for (i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_TERMINATED) {
            proc = &process_table[i];
            proc->pid = (pid == 0) ? next_pid++ : pid;
            if (next_pid > MAX_PROCESSES) next_pid = 1;

            name_len = strlen(name);
            if (name_len > 15) name_len = 15;
            memcpy(proc->name, name, name_len);
            proc->name[name_len] = '\0';

            proc->state = PROC_CREATED;
            proc->memory_size = memory_size;
            proc->memory_start = 0;
            proc->memory_end = 0;
            proc->arrival_time = arrival_time;
            proc->burst_time = burst_time;
            proc->remaining_time = burst_time;
            proc->priority = 3;
            proc->io_requests = 0;
            proc->next = NULL;

            // 初始化对应的PCB
            pcb_t* pcb = &pcb_table[i];
            pcb->base = *proc;  // 复制进程信息
            initialize_cpu_context(pcb);  // 初始化CPU上下文

            DEBUG_PRINT("Process created: PID=%d, Name=%s, Memory=%d, Time=%d",
                proc->pid, proc->name, proc->memory_size, proc->burst_time);
            return proc;
        }
    }

    kernel_log(LOG_ERR, "Failed to create process: no free slots");
    return NULL;
}

process_t* find_process_by_pid(uint32_t pid) {
    uint32_t i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROC_TERMINATED) {
            return &process_table[i];
        }
    }
    return NULL;
}

void terminate_process(process_t* proc) {
    if (proc && proc->state != PROC_TERMINATED) {
        proc->state = PROC_TERMINATED;
        proc->memory_start = 0;
        proc->memory_end = 0;
    }
}

void process_set_state(process_t* proc, process_state_t new_state) {
    if (proc) {
        proc->state = new_state;
    }
}

void dump_process_info(process_t* proc) {
    if (!proc) return;
    
    const char* state_str[] = {"CREATED", "READY", "RUNNING", "WAITING", "TERMINATED"};
    kernel_log(LOG_INFO, "Process Info: PID=%d, Name=%s, State=%s, Remaining=%d",
               proc->pid, proc->name, state_str[proc->state], proc->remaining_time);
}

