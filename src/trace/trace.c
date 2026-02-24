#include "FreeRTOS.h"
#include "task.h"
#include "trace.h"
#include "emulated_uart.h"

extern void *volatile pxCurrentTCB;

/**
 * @brief Reverses a string in place.
 * @param str The string to reverse.
 * @param length The length of the string.
 */
static void reverse(char *str, int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end)
    {
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
static char *utoa(unsigned long num, char *str, int base)
{
    int i = 0;
    // Handle 0 explicitly, otherwise empty string is returned
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    // Process individual digits
    while (num != 0)
    {
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
static void strcpy_light(char *dest, const char *src)
{
    while (*src)
    {
        *dest++ = *src++;
    }
    *dest = '\0';
}

/**
 * @brief A lightweight string concatenation function.
 * @param dest Destination buffer to append to.
 * @param src Source string to append.
 */
static void strcat_light(char *dest, const char *src)
{
    while (*dest)
    {
        dest++;
    }
    while (*src)
    {
        *dest++ = *src++;
    }
    *dest = '\0';
}

void trace_rtos_event_old(trace_event_t event_id, uintptr_t data)
{
    
    static char buffer[128];
    static char num_buffer[11];
    const char *event_name = "";

    // Start building the common part of the log message
    strcpy_light(buffer, "[");
    utoa(xTaskGetTickCount(), num_buffer, 10);
    strcat_light(buffer, num_buffer);
    strcat_light(buffer, "] ");

    char* cTaskName = "";
    if (data != NULL)
    {
        cTaskName = (char*)data;
    }
    else
    {
        cTaskName = "UNDEFINED";
    }

    // Use a switch to handle each event type differently
    switch (event_id)
    {
    // --- Cases for Task-related events ---
    case TASK_CREATE:
    case TASK_DELETE:
    case TASK_SWITCH_IN:
    case TASK_SWITCH_OUT:
    case TASK_DELAY:
    {
        if (event_id == TASK_CREATE)
            event_name = "TASK CREATE";
        else if (event_id == TASK_DELETE)
            event_name = "TASK DELETE";
        else if (event_id == TASK_SWITCH_IN)
            event_name = "TASK SWITCH IN";
        else if (event_id == TASK_SWITCH_OUT)
            event_name = "TASK SWITCH OUT";
        else if (event_id == TASK_DELAY)
            event_name = "TASK DELAY";

        strcat_light(buffer, event_name);

        // Safely cast the data back to a TaskHandle_t
        TaskHandle_t xHandle = (TaskHandle_t)data;
        TaskStatus_t xTaskDetails;
        if (xHandle != NULL)
        {
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
        if (event_id == QUEUE_SEND)
            event_name = "QUEUE SEND";
        else
            event_name = "QUEUE RECEIVE";

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

    case TRACE_HRT_SELECTED:
        strcat_light(buffer, "HRT SELECTED - Task: '");
        strcat_light(buffer, cTaskName);
        strcat_light(buffer, "'");
        break;

    case TRACE_HRT_RELEASE:
        strcat_light(buffer, "HRT RELEASE - Task: '");
        strcat_light(buffer, cTaskName);
        strcat_light(buffer, "'");
        break;

    case TRACE_HRT_COMPLETE:
        strcat_light(buffer, "HRT COMPLETE - Task: '");
        strcat_light(buffer, cTaskName);
        strcat_light(buffer, "'");
        break;

    case TRACE_HRT_ABORTED:
        strcat_light(buffer, "HRT ABORTED - Task: '");
        strcat_light(buffer, cTaskName);
        strcat_light(buffer, "'");
        break;

    case MAJOR_FRAME_RESTART:
        strcat_light(buffer, "MAJOR FRAME RESTART - All tasks reset to READY");
        break;
    case TRACE_SRT_SELECTED:
        strcat_light(buffer, "SRT SELECTED - Task: '");
        strcat_light(buffer, cTaskName);
        strcat_light(buffer, "'");
        // Safely cast the data back to a TaskHandle_t
        break;
    case TRACE_SRT_COMPLETED:
        strcat_light(buffer, "SRT COMPLETE - Task: '");
        strcat_light(buffer, cTaskName);
        strcat_light(buffer, "'");
        
        strcat_light(buffer, "'");
        break;
    case CPU_IDLE:
        strcat_light(buffer, "CPU IDLE (Timeline Gap)");
        break;

    default:
        strcat_light(buffer, "UNKNOWN EVENT");
        break;
    }

    strcat_light(buffer, "\r\n");

    // Print the final formatted string atomically
    
    UART_printf(buffer);

}


void trace_rtos_event(trace_event_t event_type, char* event_name, const char* task_name, void* data)
{
    
    static char buffer[128];
    static char num_buffer[11];

    // Start building the common part of the log message
    strcpy_light(buffer, "[");
    utoa(xTaskGetTickCount(), num_buffer, 10);
    strcat_light(buffer, num_buffer);
    strcat_light(buffer, "] ");

    char* cTaskName = "";
    if (task_name == NULL)
    {
        task_name = "UNDEFINED";
    }

    TaskHandle_t xHandle;
    TaskStatus_t xTaskDetails;
    
    switch(event_type){
    case TASK_CREATE:
        strcat_light(buffer, event_name);

        xHandle = (TaskHandle_t)data;
        
        if (xHandle != NULL)
        {
            vTaskGetInfo(xHandle, &xTaskDetails, pdFALSE, eInvalid);
            strcat_light(buffer, " - Task: '");
            strcat_light(buffer, xTaskDetails.pcTaskName);
            strcat_light(buffer, "'");
        }
        break;
    case TASK_SWITCH_IN:
        
        strcat_light(buffer, event_name);

        xHandle = (TaskHandle_t)data;
        
        if (xHandle != NULL)
        {
            vTaskGetInfo(xHandle, &xTaskDetails, pdFALSE, eInvalid);
            strcat_light(buffer, " - Task: '");
            strcat_light(buffer, xTaskDetails.pcTaskName);
            strcat_light(buffer, "'");
        }
        break;
    
    case TASK_SWITCH_OUT:
        
        strcat_light(buffer, event_name);
        
        xHandle = (TaskHandle_t)pxCurrentTCB;
        
        if (xHandle != NULL)
        {
            vTaskGetInfo(xHandle, &xTaskDetails, pdFALSE, eInvalid);
            strcat_light(buffer, " - Task: '");
            strcat_light(buffer, xTaskDetails.pcTaskName);
            strcat_light(buffer, "'");
        }
        break;

    case MAJOR_FRAME_RESTART:
        strcat_light(buffer, "MAJOR FRAME RESTART - All tasks reset to READY");
        break;
    
    case CPU_IDLE:
        strcat_light(buffer, "CPU IDLE (Timeline Gap)");
        break;

    default:
        strcat_light(buffer, event_name);

        strcat_light(buffer, " - Task: '");
        strcat_light(buffer, task_name);
        strcat_light(buffer, "'");
        break;
    }

    strcat_light(buffer, "\r\n");

    // Print the final formatted string atomically
    
    UART_printf(buffer);

}

void trace_task_switched_in(void)
{
    trace_rtos_event(TASK_SWITCH_IN, "TASK_SWITCH_IN", NULL,(uintptr_t)pxCurrentTCB);
}

void trace_task_switched_out(void)
{
    trace_rtos_event(TASK_SWITCH_OUT, "TASK_SWITCH_OUT", NULL,(uintptr_t)pxCurrentTCB);
}

void trace_task_create(void *pxNewTCB)
{
    trace_rtos_event(TASK_CREATE, "TASK CREATE", NULL, (uintptr_t)pxNewTCB);
}

void trace_task_delete(void *pxTaskToDelete){}

void trace_task_increment_tick(uint32_t xTickCount)
{
    // trace_rtos_event(TASK_INCREMENT_TICK, &xTickCount); //TODO Valutare se tenerlo
}

void trace_SRT_task_completed(char* pcTaskName)
{
    trace_rtos_event(TRACE_SRT_COMPLETED, "SRT_COMPLETED", pcTaskName, NULL);
}

void trace_SRT_task_ready(char* pcTaskName)
{
    trace_rtos_event(TRACE_SRT_READY, "SRT_READY", pcTaskName, NULL);
}

void trace_SRT_task_preempted(char* pcTaskName)
{
    trace_rtos_event(TRACE_SRT_PREEMPTED, "SRT_PREEMPTED", pcTaskName, NULL);
}

void trace_SRT_task_resumed(char* pcTaskName)
{
    trace_rtos_event(TRACE_SRT_RESUMED, "SRT_RESUMED", pcTaskName, NULL);
}

void trace_SRT_task_running(char* pcTaskName)
{
    trace_rtos_event(TRACE_SRT_RUNNING, "SRT_RUNNING", pcTaskName, NULL);
}

void trace_HRT_task_completed(char* pcTaskName)
{
    trace_rtos_event(TRACE_HRT_COMPLETED, "HRT_COMPLETED", pcTaskName, NULL);
}

void trace_HRT_task_ready(char* pcTaskName)
{
    trace_rtos_event(TRACE_HRT_READY, "HRT_READY", pcTaskName, NULL);
}

void trace_HRT_task_aborted(char* pcTaskName)
{
    trace_rtos_event(TRACE_HRT_ABORTED, "HRT_ABORTED", pcTaskName, NULL);
}

void trace_HRT_task_running(char* pcTaskName)
{
    trace_rtos_event(TRACE_HRT_RUNNING, "HRT_RUNNING", pcTaskName, NULL);
}

void trace_IDLE_task()
{
    trace_rtos_event(CPU_IDLE, "CPU_IDLE", "IDLE", NULL);
}


char *convertEventToString(unsigned short int event)
{
    char *out_str;
    switch (event)
    {

    case TASK_SWITCH_IN:
        out_str = "Task Switch In";
        break;
    case TASK_SWITCH_OUT:
        out_str = "Task Switch Out";
        break;
    case QUEUE_SEND:
        out_str = "Queue Send";
        break;
    case QUEUE_RECEIVE:
        out_str = "Queue Receive";
        break;
    case TASK_CREATE:
        out_str = "Task Create";
        break;
    case TASK_DELETE:
        out_str = "Task Delete";
        break;
    case TRACE_HRT_RELEASE:
        out_str = "HRT Release";
        break;
    case TRACE_HRT_COMPLETE:
        out_str = "HRT Complete";
        break;
    case TRACE_HRT_ABORTED:
        out_str = "HRT Aborted";
        break;
    case MAJOR_FRAME_RESTART:
        out_str = "MAJOR FRAME RESET";
        break;
    case TIMEKEEPER_RESET:
        out_str = "Timekeeper MF Sync";
        break;
    default:
        out_str = "UNKNOWN EVENT";
        break;
    }
    return out_str;
}

void trace_task_suspend(void *pxTaskToSuspend) {}
void trace_task_delay(void) {}
void trace_task_delay_until(uint32_t xTimeToWake) {}
void trace_task_resume(void *pxTaskToResume) {}
void trace_task_resume_from_isr(void *pxTaskToResume) {}
void trace_task_notify_wait(uint32_t uxIndexToWaitOn) {}
void trace_queue_send(void *pxQueue){}
void trace_queue_receive(void *pxQueue){}

