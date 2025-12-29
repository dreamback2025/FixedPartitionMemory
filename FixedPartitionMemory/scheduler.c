#include "os_types.h"
#include "log.h"
#include "scheduler.h"
#include "config.h"
#include "process.h"
#include "memory.h"

// 全局调度器
scheduler_t g_scheduler;

// 就绪队列操作
static void ready_queue_init(ready_queue_t* queue) {
    queue->front = NULL;
    queue->rear = NULL;
    queue->count = 0;
}

static void ready_queue_enqueue(ready_queue_t* queue, process_t* proc) {
    if (!proc) return;

    proc->next = NULL;

    if (queue->rear == NULL) {
        // 队列为空
        queue->front = proc;
        queue->rear = proc;
    }
    else {
        // 添加到队列末尾
        queue->rear->next = proc;
        queue->rear = proc;
    }
    queue->count++;
}

static process_t* ready_queue_dequeue(ready_queue_t* queue) {
    if (queue->front == NULL) {
        return NULL;
    }

    process_t* proc = queue->front;
    queue->front = proc->next;

    if (queue->front == NULL) {
        // 队列变空
        queue->rear = NULL;
    }

    proc->next = NULL;
    queue->count--;
    return proc;
}

// 调度器初始化
void scheduler_init(scheduler_type_t type) {
    ready_queue_init(&g_scheduler.ready_queue);
    g_scheduler.current_process = NULL;
    g_scheduler.type = type;
    g_scheduler.time_slice = TIME_SLICE;  // 从config.h获取
    g_scheduler.current_time_slice = 0;

    DEBUG_PRINT("Scheduler initialized with type %d", type);
}

// 添加进程到就绪队列
void scheduler_add_process(process_t* proc) {
    if (!proc) return;

    process_set_state(proc, PROC_READY);
    ready_queue_enqueue(&g_scheduler.ready_queue, proc);

    DEBUG_PRINT("Process %d added to ready queue", proc->pid);
}

// 获取下一个要调度的进程
process_t* scheduler_get_next_process(void) {
    process_t* next_proc = NULL;

    switch (g_scheduler.type) {
    case SCHED_FIFO:
        // FIFO: 按照就绪队列顺序
        next_proc = ready_queue_dequeue(&g_scheduler.ready_queue);
        break;

    case SCHED_RR:
        // 时间片轮转: 从就绪队列取下一个
        if (g_scheduler.current_process &&
            g_scheduler.current_process->state == PROC_RUNNING) {
            // 如果当前进程时间片用完，放回队列末尾
            if (g_scheduler.current_time_slice == 0) {
                process_set_state(g_scheduler.current_process, PROC_READY);
                ready_queue_enqueue(&g_scheduler.ready_queue, g_scheduler.current_process);
                g_scheduler.current_process = NULL;
            }
        }

        if (!g_scheduler.current_process) {
            next_proc = ready_queue_dequeue(&g_scheduler.ready_queue);
        }
        break;

    case SCHED_PRIORITY:
        // 优先级调度: 这里简化实现，取队列中的进程
        next_proc = ready_queue_dequeue(&g_scheduler.ready_queue);
        break;

    default:
        next_proc = ready_queue_dequeue(&g_scheduler.ready_queue);
        break;
    }

    return next_proc;
}

// 执行调度
void scheduler_schedule(void) {
    // 获取下一个进程
    process_t* next_proc = scheduler_get_next_process();

    if (next_proc) {
        // 设置当前进程
        g_scheduler.current_process = next_proc;
        process_set_state(next_proc, PROC_RUNNING);
        g_scheduler.current_time_slice = g_scheduler.time_slice;

        DEBUG_PRINT("Scheduled process %d to run", next_proc->pid);
    }
    else if (g_scheduler.current_process) {
        // 如果没有新进程但当前进程还在运行，继续运行
        if (g_scheduler.current_process->state == PROC_RUNNING) {
            g_scheduler.current_time_slice = g_scheduler.time_slice;
        }
        else {
            g_scheduler.current_process = NULL;
        }
    }
}

// 执行当前进程
void scheduler_run_current_process(void) {
    if (!g_scheduler.current_process) {
        return;
    }

    process_t* current = g_scheduler.current_process;

    // 检查进程状态
    if (current->state != PROC_RUNNING) {
        g_scheduler.current_process = NULL;
        return;
    }

    // 执行一个时间单位
    if (current->remaining_time > 0) {
        current->remaining_time--;
        g_scheduler.current_time_slice--;

        DEBUG_PRINT("Process %d executed, remaining time: %d",
            current->pid, current->remaining_time);

        // 检查是否完成
        if (current->remaining_time == 0) {
            DEBUG_PRINT("Process %d completed at time slice", current->pid);
            free_memory(current);
            process_set_state(current, PROC_TERMINATED);
            g_scheduler.current_process = NULL;
        }
        else if (g_scheduler.current_time_slice == 0 && g_scheduler.type == SCHED_RR) {
            // 时间片用完，放回就绪队列
            process_set_state(current, PROC_READY);
            ready_queue_enqueue(&g_scheduler.ready_queue, current);
            g_scheduler.current_process = NULL;
            DEBUG_PRINT("Time slice expired for process %d", current->pid);
        }
    }
}

// 显示调度器状态
void scheduler_dump_status(void) {
    kernel_log(LOG_INFO, "Scheduler Status:");
    kernel_log(LOG_INFO, "  Type: %s",
        g_scheduler.type == SCHED_FIFO ? "FIFO" :
        g_scheduler.type == SCHED_RR ? "Round Robin" : "Priority");
    kernel_log(LOG_INFO, "  Ready Queue Count: %d", g_scheduler.ready_queue.count);
    kernel_log(LOG_INFO, "  Current Process: %s",
        g_scheduler.current_process ?
        g_scheduler.current_process->name : "None");
    kernel_log(LOG_INFO, "  Time Slice: %d/%d",
        g_scheduler.current_time_slice, g_scheduler.time_slice);
}

// 时间片轮转调度初始化
void rr_scheduler_init(void) {
    scheduler_init(SCHED_RR);
}

// 获取下一个RR进程
process_t* rr_scheduler_get_next(void) {
    return scheduler_get_next_process();
}

// 执行RR进程
void rr_scheduler_run_process(process_t* proc) {
    if (proc) {
        g_scheduler.current_process = proc;
        process_set_state(proc, PROC_RUNNING);
        g_scheduler.current_time_slice = g_scheduler.time_slice;
        scheduler_run_current_process();
    }
}