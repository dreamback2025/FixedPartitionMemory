#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>  // Add unistd.h for usleep function

// Include kernel headers
#include "os_types.h"
#include "memory.h"
#include "process.h"
#include "partition.h"
#include "log.h"
#include "config.h"
#include "kernel.h"
#include "scheduler.h"

// Global variables
static BOOL use_timer = FALSE;
static BOOL running = TRUE;
static uint32_t simulated_time = 0;
extern allocation_strategy_t current_strategy;
extern scheduler_t g_scheduler;

FILE* log_file = NULL;

// Logging functions
void init_logging() {
    // Create log filename (with timestamp)
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char filename[50];
    strftime(filename, sizeof(filename), "memory_log_%Y%m%d_%H%M%S.txt", t);

    // Open log file
    log_file = fopen(filename, "w");
    if (log_file) {
        printf("Log saved to: %s\n", filename);
    }
    else {
        printf("Cannot create log file!\n");
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

// Custom screen clear function
void log_clear_screen() {
    system("clear");  // Linux system uses clear command
    if (log_file) {
        fprintf(log_file, "\n--- Screen cleared (Time: %d) ---\n", simulated_time);
        fflush(log_file);
    }
}

// Functions for keyboard input (Linux compatible version)
char get_char_input() {
    char input[2];
    fgets(input, sizeof(input), stdin);
    // Consume remaining characters until newline
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    return input[0];
}

int get_int_input() {
    int value;
    scanf("%d", &value);
    // Consume remaining characters until newline
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    return value;
}

// Generate automatic processes
void generate_auto_processes(int count) {
    srand((unsigned)time(NULL));

    for (int i = 0; i < count; i++) {
        char name[16];
        sprintf(name, "AutoProcess%d", i + 1);
        uint32_t memory_size = (rand() % 128) + 32; // 32-159 bytes
        uint32_t burst_time = (rand() % 10) + 1;   // 1-10 units
        uint32_t arrival_time = rand() % 5;        // 0-4

        process_t* proc = create_process(0, name, memory_size, burst_time, arrival_time);
        if (proc) {
            log_printf("Created auto process: %s, Memory=%d, Time=%d, Arrival=%d\n",
                name, memory_size, burst_time, arrival_time);
        }
    }
}

// Generate manual processes
void generate_manual_processes() {
    int count;
    log_printf("Please enter number of processes (1-%d): ", MAX_PROCESSES);
    count = get_int_input();

    if (count < 1) count = 1;
    if (count > MAX_PROCESSES) count = MAX_PROCESSES;

    for (int i = 0; i < count; i++) {
        char name[16];
        uint32_t memory_size, burst_time, arrival_time;

        log_printf("\nProcess %d:\n", i + 1);
        log_printf("Name: ");
        fgets(name, 16, stdin);
        name[strcspn(name, "\n")] = '\0';
        if (strlen(name) == 0) sprintf(name, "ManualProcess%d", i + 1);

        log_printf("Memory size (bytes): ");
        memory_size = get_int_input();

        log_printf("Execution time (units): ");
        burst_time = get_int_input();

        log_printf("Arrival time: ");
        arrival_time = get_int_input();

        process_t* proc = create_process(0, name, memory_size, burst_time, arrival_time);
        if (proc) {
            log_printf("Created manual process: %s, Memory=%d, Time=%d, Arrival=%d\n",
                name, memory_size, burst_time, arrival_time);
        }
    }
}

// Display system status
void display_system_status() {
    log_clear_screen();
    log_printf("=== Fixed Partition Memory Management System ===\n");
    log_printf("Current Time: %d\n", simulated_time);
    log_printf("Allocation Strategy: ");
    switch (current_strategy) {
    case FIRST_FIT: log_printf("First Fit Algorithm\n"); break;
    case BEST_FIT: log_printf("Best Fit Algorithm\n"); break;
    case WORST_FIT: log_printf("Worst Fit Algorithm\n"); break;
    }
    log_printf("Run Mode: %s\n", use_timer ? "Auto Mode" : "Manual Mode");

    // Display memory map (fixed partitions)
    log_printf("\n--- Memory Map ---\n");
    log_printf("Start Addr  End Addr  Size    Status      Owner PID\n");

    // Display OS partition
    if (partition_count > 0) {
        partition_t* os_partition = &partition_table[0];
        const char* state_str_os;
        switch (os_partition->state) {
        case PARTITION_FREE: state_str_os = "Free"; break;
        case PARTITION_ALLOCATED: state_str_os = "Allocated"; break;
        case PARTITION_OS: state_str_os = "OS"; break;
        default: state_str_os = "Unknown";
        }

        log_printf("0x%04x   0x%04x   %4d    %-8s    %d\n",
            os_partition->start,
            os_partition->start + os_partition->size - 1,
            os_partition->size,
            state_str_os,
            os_partition->owner_pid);
    }

    // Display all fixed partitions - BUG FIX: Add boundary check to prevent array access beyond MAX_PARTITIONS
    for (uint32_t i = 1; i < partition_count && i < MAX_PARTITIONS; i++) {
        const char* state_str;
        switch (partition_table[i].state) {
        case PARTITION_FREE: state_str = "Free"; break;
        case PARTITION_ALLOCATED: state_str = "Allocated"; break;
        case PARTITION_OS: state_str = "OS"; break;
        default: state_str = "Unknown";
        }

        log_printf("0x%04x   0x%04x   %4d    %-8s    %d\n",
            partition_table[i].start,
            partition_table[i].start + partition_table[i].size - 1,
            partition_table[i].size,
            state_str,
            partition_table[i].owner_pid);
    }

    // Display process status
    log_printf("\n--- Process Status ---\n");
    log_printf("PID  Name           Status      Memory Size  Remaining Time  Arrival Time\n");

    extern process_t process_table[MAX_PROCESSES];
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        process_t* proc = &process_table[i];
        if (proc->state != PROC_TERMINATED) {
            const char* state_str;
            switch (proc->state) {
            case PROC_CREATED: state_str = "Created"; break;
            case PROC_READY: state_str = "Ready"; break;
            case PROC_RUNNING: state_str = "Running"; break;
            case PROC_WAITING: state_str = "Waiting"; break;
            case PROC_TERMINATED: state_str = "Terminated"; break;
            default: state_str = "Unknown";
            }

            log_printf("%-4d %-12s  %-8s  %4d    %4d       %4d\n",
                proc->pid, proc->name, state_str,
                proc->memory_size, proc->remaining_time, proc->arrival_time);
        }
    }

    // Display memory statistics
    uint32_t total_free = get_total_free_memory();
    uint32_t largest_block = get_largest_free_block();
    uint32_t total_used = MEMORY_SIZE - OS_PARTITION_SIZE - total_free;

    log_printf("\n--- Memory Statistics ---\n");
    log_printf("Total Memory: %d bytes\n", MEMORY_SIZE);
    log_printf("OS Occupied: %d bytes\n", OS_PARTITION_SIZE);
    log_printf("Total Free Memory: %d bytes (%.1f%%)\n",
        total_free, (float)total_free * 100 / (MEMORY_SIZE - OS_PARTITION_SIZE));
    log_printf("Total Used Memory: %d bytes (%.1f%%)\n",
        total_used, (float)total_used * 100 / (MEMORY_SIZE - OS_PARTITION_SIZE));
    log_printf("Largest Free Block: %d bytes\n", largest_block);

    // Display scheduler status
    log_printf("\n--- Scheduler Status ---\n");
    log_printf("Scheduling Algorithm: %s\n", 
              g_scheduler.type == SCHED_FIFO ? "First In First Out (FIFO)" :
              g_scheduler.type == SCHED_RR ? "Round Robin (RR)" : "Priority Scheduling");
    log_printf("Ready Queue Process Count: %d\n", g_scheduler.ready_queue.count);
    log_printf("Current Running Process: %s\n", 
              g_scheduler.current_process ? 
              g_scheduler.current_process->name : "None");
    log_printf("Current Time Slice: %d/%d\n", 
              g_scheduler.current_time_slice, g_scheduler.time_slice);
    log_printf("Q=Quit, C=Compact Memory, F=First Fit, B=Best Fit, W=Worst Fit\n");
    log_printf("Press any key to continue...\n");
}

int main() {
    // Initialize logging
    init_logging();

    log_printf("=== Fixed Partition Memory Management System ===\n");

    // Get current time
    time_t now = time(NULL);
    char* time_str = ctime(&now);
    log_printf("Program start time: %s", time_str);

    // Initialize kernel
    kernel_init();
    
    // Initialize scheduler (using round-robin algorithm)
    scheduler_init(SCHED_RR);

    // Select process generation method
    log_printf("\nPlease select process generation method:\n");
    log_printf("1. Auto generate processes\n");
    log_printf("2. Manual input processes\n");
    log_printf("Please select: ");

    char choice = get_char_input();
    log_printf("\n");

    switch (choice) {
    case '1':
        log_printf("Please enter number of processes (1-10): ");
        int count = get_int_input();
        if (count < 1) count = 1;
        if (count > 10) count = 10;
        generate_auto_processes(count);
        break;
    case '2':
        generate_manual_processes();
        break;
    default:
        log_printf("Invalid selection, using default 5 processes\n");
        generate_auto_processes(5);
    }

    // Select time advancement method
    log_printf("\nPlease select time advancement method:\n");
    log_printf("1. Manual mode (press any key to advance time)\n");
    log_printf("2. Auto mode (timer)\n");
    log_printf("Please select: ");

    choice = get_char_input();
    log_printf("\n");

    if (choice == '1') {
        use_timer = FALSE;
        log_printf("Manual mode selected. Press any key to advance time.\n");
    }
    else if (choice == '2') {
        use_timer = TRUE;
        log_printf("Auto mode selected. Timer interval: %dms\n", TIMER_INTERVAL);
    }
    else {
        log_printf("Invalid selection, using manual mode\n");
        use_timer = FALSE;
    }

    // Display initial status
    display_system_status();

    if (use_timer) {
        // Auto mode - use sleep instead of WM_TIMER
        running = TRUE;
        log_printf("Auto mode started. Press 'q' to quit, 'c' to compact memory...\n");

        uint32_t last_display_time = 0;
        while (running) {
            // Handle keyboard input - non-blocking input on Linux
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

            // Advance time by one unit each loop
            simulated_time++;

            // Check for newly arrived processes - BUG FIX: Only check processes that haven't terminated
            for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
                process_t* proc = &process_table[i];
                if (proc->state != PROC_TERMINATED && proc->state == PROC_CREATED && proc->arrival_time <= simulated_time) {
                    log_printf("\nProcess %s (PID=%d) arrived at time %d\n",
                        proc->name, proc->pid, simulated_time);

                    // Try to allocate memory
                    if (allocate_memory(proc, current_strategy) == 0) {
                        log_printf("\nMemory allocated to process %s\n", proc->name);
                        scheduler_add_process(proc);  // Add to scheduler
                    }
                    else {
                        log_printf("Cannot allocate memory for process %s, will try again next time\n", proc->name);
                        // Keep PROC_CREATED state, try again at next time point
                    }
                }
            }

            // Execute scheduling
            scheduler_schedule();
            scheduler_run_current_process();

            // Display system status every 5 time units
            if (simulated_time - last_display_time >= 5 || simulated_time < 10) {
                display_system_status();
                last_display_time = simulated_time;
            }

            // Check if all processes have completed
            int all_completed = 1;
            for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
                process_t* proc = &process_table[i];
                if (proc->state != PROC_TERMINATED && proc->state != PROC_CREATED) {
                    all_completed = 0;
                    break;
                }
            }

            if (all_completed && simulated_time > 20) {
                log_printf("\nAll processes completed! Press any key to exit...\n");
                get_char_input();
                running = FALSE;
                break;
            }

            // Control simulation speed - use usleep on Linux
            usleep(TIMER_INTERVAL * 1000);  // Convert to microseconds
        }
    }
    else {
        // Manual mode
        while (running) {
            log_printf("\nPress any key to advance time (Q=Quit, C=Compact Memory): ");
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

            // Check for newly arrived processes - BUG FIX: Only check processes that haven't terminated
            extern process_t process_table[MAX_PROCESSES];
            for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
                process_t* proc = &process_table[i];
                if (proc->state != PROC_TERMINATED && proc->state == PROC_CREATED && proc->arrival_time <= simulated_time) {
                    log_printf("\nProcess %s (PID=%d) arrived at time %d\n",
                        proc->name, proc->pid, simulated_time);

                    if (allocate_memory(proc, current_strategy) == 0) {
                        log_printf("Memory allocated to process %s\n", proc->name);
                        scheduler_add_process(proc);  // Add to scheduler
                    }
                    else {
                        log_printf("Cannot allocate memory for process %s\n", proc->name);
                    }
                }
            }

            // Execute scheduling
            scheduler_schedule();
            scheduler_run_current_process();

            display_system_status();
        }
    }

    // Program end
    now = time(NULL);
    time_str = ctime(&now);
    log_printf("\nProgram end time: %s", time_str);
    log_printf("Press any key to exit...\n");
    get_char_input();

    // Close logging
    close_logging();
    return 0;
}