#ifndef TRACE_H
#define TRACE_H

#define TRACE_BUFFER_SIZE 1024
#include <stdint.h>

// Example Event IDs
typedef enum trace_event {
    TASK_SWITCH_IN,
    TASK_SWITCH_OUT,
    QUEUE_SEND,
    QUEUE_RECEIVE,
    TASK_CREATE,
    TASK_DELETE,   
    TASK_INCREMENT_TICK,
    TASK_SUSPEND,
    TASK_DELAY,
    TASK_DELAY_UNTIL,
    TASK_PRIORITY_SET,
    TASK_PRIORITY_INHERIT,
    TASK_RESUME,
    TASK_RESUME_FROM_ISR,
    TRACE_HRT_ABORTED,
    TASK_NOTIFY_WAIT,
    TIMEKEEPER_RESET,
    MAJOR_FRAME_RESTART,
    TRACE_SRT_SELECTED, /* Triggered when a specific SRT task is selected by the scheduler */
    TRACE_SRT_COMPLETED,
    TRACE_SRT_READY,
    TRACE_SRT_PREEMPTED,
    TRACE_SRT_RESUMED,
    TRACE_SRT_RUNNING,
    TRACE_HRT_SELECTED, /* Triggered when a specific HRT task is selected by the scheduler */
    TRACE_HRT_RELEASE,
    TRACE_HRT_COMPLETE,
    TRACE_HRT_DEADLINE,
    TRACE_HRT_COMPLETED,
    TRACE_HRT_RUNNING,
    TRACE_HRT_READY,
    CPU_IDLE      /* Triggered when there is a gap in the timeline (no HRT task) */
} trace_event_t;

char* convertEventToString(unsigned short int event);
void trace_rtos_event(trace_event_t event_type, char* event_name, const char* task_name, void* data);
void trace_task_switched_in(void);
void trace_task_switched_out(void);
void trace_queue_send(void* pxQueue);
void trace_queue_receive(void* pxQueue);
void trace_task_create(void* pxNewTCB);
void trace_task_delete(void* pxTaskToDelete);
void trace_task_increment_tick(uint32_t xTickCount);
void trace_task_suspend(void* pxTaskToSuspend);
void trace_task_delay(void);
void trace_task_delay_until(uint32_t xTimeToWake);
void trace_task_resume(void* pxTaskToResume);
void trace_task_resume_from_isr(void* pxTaskToResume);
void trace_task_notify_wait(uint32_t uxIndexToWaitOn);

void trace_event_put(uint32_t event_id, void* event_data);

#endif //TRACE_H
