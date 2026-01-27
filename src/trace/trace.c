#include "FreeRTOS.h"
#include "task.h"
#include "trace.h"
#include "emulated_uart.h"

extern void* volatile pxCurrentTCB;

/**
 * @brief Reverses a string in place.
 * @param str The string to reverse.
 * @param length The length of the string.
 */
static void reverse(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

/**
 * @brief A lightweight unsigned integer to ASCII conversion function.
 * @param num The number to convert.
 * @param str The buffer to place the resulting string in.
 * @param base The base for conversion (e.g., 10 for decimal).
 * @return A pointer to the resulting string.
 */
static char* utoa(unsigned long num, char* str, int base) {
    int i = 0;
    // Handle 0 explicitly, otherwise empty string is returned
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    str[i] = '\0'; // Null-terminate string

    // Reverse the string
    reverse(str, i);

    return str;
}

/**
 * @brief A lightweight string copy function.
 * @param dest Destination buffer.
 * @param src Source string.
 */
static void strcpy_light(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

/**
 * @brief A lightweight string concatenation function.
 * @param dest Destination buffer to append to.
 * @param src Source string to append.
 */
static void strcat_light(char* dest, const char* src) {
    while (*dest) {
        dest++;
    }
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

void trace_rtos_event(uint32_t event_id, uintptr_t data) {

    static char buffer[128];
    static char num_buffer[11];
    const char* event_name = "";

    // Start building the common part of the log message
    strcpy_light(buffer, "[");
    utoa(xTaskGetTickCount(), num_buffer, 10);
    strcat_light(buffer, num_buffer);
    strcat_light(buffer, "] ");

    // Use a switch to handle each event type differently
    switch (event_id) {
        // --- Cases for Task-related events ---
        case TASK_CREATE:
        case TASK_DELETE:
        case TASK_SWITCH_IN:
        case TASK_SWITCH_OUT:
        case TASK_DELAY:
            {
                if (event_id == TASK_CREATE) event_name = "TASK CREATE";
                else if (event_id == TASK_DELETE) event_name = "TASK DELETE";
                else if (event_id == TASK_SWITCH_IN) event_name = "TASK SWITCH IN";
                else if (event_id == TASK_SWITCH_OUT) event_name = "TASK SWITCH OUT";
                else if (event_id == TASK_DELAY) event_name = "TASK DELAY";

                strcat_light(buffer, event_name);

                // Safely cast the data back to a TaskHandle_t
                TaskHandle_t xHandle = (TaskHandle_t)data;
                TaskStatus_t xTaskDetails;
                if (xHandle != NULL) {
                    vTaskGetInfo(xHandle, &xTaskDetails, pdFALSE, eInvalid);
                    strcat_light(buffer, " - Task: '");
                    strcat_light(buffer, xTaskDetails.pcTaskName);
                    strcat_light(buffer, "'");
                }
            }
            break;

        // --- Cases for Queue-related events ---
        case QUEUE_SEND:
        case QUEUE_RECEIVE:
            {
                if (event_id == QUEUE_SEND) event_name = "QUEUE SEND";
                else event_name = "QUEUE RECEIVE";

                strcat_light(buffer, event_name);
                strcat_light(buffer, " - ID: 0x");
                // For queues, we just print their address as a unique ID
                utoa((unsigned long)data, num_buffer, 16);
                strcat_light(buffer, num_buffer);
            }
            break;

        case TASK_INCREMENT_TICK:
                event_name = "TASK_INCREMENT_TICK";
                strcat_light(buffer, event_name);
                strcat_light(buffer, " - COUNT: ");
                utoa((unsigned long)data, num_buffer, 10);
                strcat_light(buffer, num_buffer);
            break;

        case TIMEKEEPER_RESET:
            strcat_light(buffer, "TIMEKEEPER MAJOR FRAME RESET");
            break;

        default:
            strcat_light(buffer, "UNKNOWN EVENT");
            break;
    }

    strcat_light(buffer, "\r\n");

    // Print the final formatted string atomically
    //taskENTER_CRITICAL();
    UART_printf(buffer);
    //taskEXIT_CRITICAL();
}

void custom_trace_task_switched_in(void) {
    trace_rtos_event(TASK_SWITCH_IN, (uintptr_t) pxCurrentTCB);
}

void custom_trace_task_switched_out(void) {
    trace_rtos_event(TASK_SWITCH_OUT, (uintptr_t) pxCurrentTCB);
}

void custom_trace_queue_send(void* pxQueue) {
    trace_rtos_event(QUEUE_SEND, (uintptr_t) pxQueue);
}

void custom_trace_queue_receive(void* pxQueue) {
    trace_rtos_event(QUEUE_RECEIVE, (uintptr_t) pxQueue);
}

void custom_trace_task_create(void* pxNewTCB) {
    trace_rtos_event(TASK_CREATE, (uintptr_t) pxNewTCB);
}

void custom_trace_task_delete(void* pxTaskToDelete) {
        trace_rtos_event(TASK_DELETE, (uintptr_t) pxTaskToDelete);
}

void custom_trace_task_increment_tick(uint32_t xTickCount){
        //trace_rtos_event(TASK_INCREMENT_TICK, &xTickCount); //TODO Valutare se tenerlo
}


char* convertEventToString(unsigned short int event){
    char* out_str;
    switch(event){

        case TASK_SWITCH_IN:
        out_str="Task Switch In";
        break;
        case TASK_SWITCH_OUT:
        out_str="Task Switch Out";
        break;
        case QUEUE_SEND:
        out_str="Queue Send";
        break;
        case QUEUE_RECEIVE:
        out_str="Queue Receive";
        break;
        case TASK_CREATE:
        out_str="Task Create";
        break;
        case TASK_DELETE:
        out_str="Task Delete";
        break;
        default:
        out_str="UNKNOWN EVENT";
        break;
    }
    return out_str;
}

void custom_trace_task_suspend(void* pxTaskToSuspend) {}
void custom_trace_task_delay(void) {}
void custom_trace_task_delay_until(uint32_t xTimeToWake) {}
void custom_trace_task_resume(void* pxTaskToResume) {}
void custom_trace_task_resume_from_isr(void* pxTaskToResume) {}
void custom_trace_task_notify_wait(uint32_t uxIndexToWaitOn) {}