#ifndef TRACE_H
#define TRACE_H

#define TRACE_BUFFER_SIZE 1024
// typedef struct {
//     uint32_t timestamp;
//     uint32_t event_id;
//     void* event_data;
// } trace_event_t;

// static trace_event_t trace_buffer[TRACE_BUFFER_SIZE];
static volatile uint32_t head = 0;
static volatile uint32_t tail = 0;

// Example Event IDs
typedef enum {
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
    TASK_NOTIFY_WAIT,
    TIMEKEEPER_RESET
} trace_event_t;

char* convertEventToString(unsigned short int event);

void trace_rtos_event(uint32_t event_id, uintptr_t data);
void custom_trace_task_switched_in(void);
void custom_trace_task_switched_out(void);
void custom_trace_queue_send(void* pxQueue);
void custom_trace_queue_receive(void* pxQueue);
void custom_trace_task_create(void* pxNewTCB);
void custom_trace_task_delete(void* pxTaskToDelete);
void custom_trace_task_increment_tick(uint32_t xTickCount);
void custom_trace_task_suspend(void* pxTaskToSuspend);
void custom_trace_task_delay(void);
void custom_trace_task_delay_until(uint32_t xTimeToWake);
void custom_trace_task_resume(void* pxTaskToResume);
void custom_trace_task_resume_from_isr(void* pxTaskToResume);
void custom_trace_task_notify_wait(uint32_t uxIndexToWaitOn);

void trace_event_put(uint32_t event_id, void* event_data);

#endif //TRACE_H