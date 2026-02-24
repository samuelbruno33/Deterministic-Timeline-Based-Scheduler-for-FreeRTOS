#include "FreeRTOS.h"
#include "task.h"
#include "timeline_config.h"
#include "timeline_scheduler.h" /* Just to access xIsTimelineSchedulerActive */
#include "emulated_uart.h"
#include "timekeeper.h"
#include "trace.h" /* Necessary to initialize the Timekeeper */

/**
 * @brief Dummy HRT Task Code.
 */
//void vHrtTaskCode(void *pvParams) {
 //   (void)pvParams;
    
    /* In a real HRT system, it would perform work and finish.
       Here we just want to test if it gets selected. But it needs to run in loop, if a task finishes, the system crash */
   // for (;;) {
    //}
//}

/**
 * Standard HRT Task Implementation.
 * * This function demonstrates how an HRT task should be structured:
 * 1. Perform the cyclic workload.
 * 2. Notify the scheduler upon completion.
 * 3. The scheduler automatically suspends the task until the next Major Frame.
 */
void vHrtWorkloadTask(void *pvParams) {
    TaskHandle_t xHandle = xTaskGetCurrentTaskHandle();
    const char* pcTaskName = (const char*) pvParams;

    for(;;) {
        UART_printf("RUNNING: ");
        UART_printf(pcTaskName);
        UART_printf("\r\n");

        /*Here we simulate a computational workload */
        for(volatile int i = 0; i < 20000; i++);

        vNotifyTaskCompletion(xHandle);
    }
}

int main(void) {
    UART_init();
    UART_printf("\n--- HRT SCHEDULING TEST ---\n");

    /* * Configuration: 
     * Major Frame: 50 ticks
     * Task A: [10, 20) -> Active for 10 ticks
     * Task B: [30, 40) -> Active for 10 ticks
     * Gaps: [0,10), [20,30), [40,50) -> Should be Idle
    */
    
    TimelineTaskConfig_t xTasks[] = {
        { "HRT_A", vHrtWorkloadTask, "A", 512, TIMELINE_TASK_HRT, 10, 20, 0 },
        { "HRT_B", vHrtWorkloadTask, "B", 512, TIMELINE_TASK_HRT, 30, 40, 0 }
    };

    SchedulerConfig_t xConfig = { 
        .ulMajorFrameTicks = 50, 
        .ulSubFrameTicks = 10, 
        .pxTasks = xTasks, 
        .ulNumTasks = 2 
    };

    /*here we say how much does a frame lasts*/
    SubFrame_t xSF[] = { {0, 50} };
    /* Configuration for the Timekeeper */
    /* We need to say to the Timekeeper how much a frame lasts */
    TimekeeperConfig_t xTkConfig = {
        .major_frame_ticks = 50,
        .num_subframes = 5,     
        .subframes = xSF
    };

    /* Configure data structures */
    vConfigureScheduler(&xConfig);

    /* Configure the Timekeeper */
    vTimekeeperInit(&xTkConfig);

    /* vConfigureScheduler creates tasks in Suspended state.
       Normally, Phase 2 Timekeeper would handle activation. 
       Here we manually set the flag to force tasks.c to use this logic for the test.
    */
    extern BaseType_t xIsTimelineSchedulerActive; 
    xIsTimelineSchedulerActive = pdTRUE;

    UART_printf("Scheduler configured. Starting Kernel...\n");
    UART_printf("Expect trace: Idle(0-10) -> A(10-20) -> Idle(20-30) -> B(30-40) -> Idle...\n");

    /* Start FreeRTOS Kernel */
    vTaskStartScheduler();

    for(;;);
}