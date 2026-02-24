#include "FreeRTOS.h"
#include "task.h"
#include "timeline_config.h"
#include "timeline_scheduler.h"
#include "timekeeper.h"
#include "emulated_uart.h"
#include "trace.h"

/**
 * @brief Dummy HRT Task
 */
void vHrtTask(void *pvParams) {
    (void)pvParams;
    
    /* In a real HRT system, it would perform work and finish.
       Here we just want to test if it gets selected. But it needs to run in loop, if a task finishes, the system crash */
    for (;;) {
    }
}

/**
 * @brief Dummy SRT Task
 */
void vSrtTask(void *pvParams) {
    (void)pvParams;
while(1)
{
    for(volatile int i = 0; i < 100000000; i++)
    {
        //Busy
    }
    //UART_printf("SRT task finished cycle\0");
    traceTASK_SRT_COMPLETED(pcTaskGetName(NULL));
    vNotifySrtCompletion();
}
}


int main(void)
{
    UART_init();
    UART_printf("\n--- SRT SCHEDULING TEST ---\n");

    /*
        Major Frame = 40 ticks

        HRT_A -> [0,10)
        HRT_B -> [20,30)

        SRT_X
        SRT_Y
    */

    TimelineTaskConfig_t xTasks[] = {
        {"HRT_A", vHrtTask, "A", 128, TIMELINE_TASK_HRT, 0, 10, 0},
        {"HRT_B", vHrtTask, "B", 128, TIMELINE_TASK_HRT, 20, 30, 0},
        {"SRT_X", vSrtTask, "X", 128, TIMELINE_TASK_SRT, 0, 0, 0},
        {"SRT_Y", vSrtTask, "Y", 128, TIMELINE_TASK_SRT, 0, 0, 0}

    };

    SchedulerConfig_t xConfig =
    {
        .ulMajorFrameTicks = 1500,
        .pxTasks           = xTasks,
        .ulNumTasks        = 4
    };

    TimekeeperConfig_t xTkConfig =
    {
        .major_frame_ticks = 1500,
        .num_subframes     = 0,
        .subframes         = NULL
    };

    /* Configure scheduler */
    vConfigureScheduler(&xConfig);

    /* Configure timekeeper */
    vTimekeeperInit(&xTkConfig);

    /* Activate timeline scheduler */
    extern BaseType_t xIsTimelineSchedulerActive;
    xIsTimelineSchedulerActive = pdTRUE;

    UART_printf("Expect pattern:\n");
    UART_printf("0-10   HRT_A\n");
    UART_printf("10-20  SRT_X -> SRT_Y\n");
    UART_printf("20-30  HRT_B\n");
    UART_printf("30-40  SRT_X -> SRT_Y\n");
    UART_printf("Repeat deterministically.\n\n");

    vTaskStartScheduler();

    for(;;);
}
