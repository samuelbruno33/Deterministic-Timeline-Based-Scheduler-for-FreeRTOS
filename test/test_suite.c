#include "FreeRTOS.h"
#include "task.h"
#include "timeline_config.h"
#include "timeline_scheduler.h" /* Just to access xIsTimelineSchedulerActive */
#include "timekeeper.h"         /* Necessary to initialize the Timekeeper */
#include "emulated_uart.h"

#include "test_tasks.h"

typedef void (*test_func_t)(void); // funtion pointer type

/* TEST FUNCTIONS */
void test_HRT(void);
void test_Timekeeper(void);
void test_DataStructures(void) {};
/* **************** */

typedef struct
{
    const char *name;
    test_func_t func;
} Test_t;

Test_t tests[] = {
    // {"Test 1 - test_DataStructures", test_DataStructures},
    // {"Test 2 - test_Timekeeper", test_Timekeeper},
    {"Test 3 - test_HRT", test_HRT},
    //{"Test 4 - test_SRT_HRT", test_HRT},
};

#define NUM_TESTS (sizeof(tests) / sizeof(tests[0]))

int main(void)
{
    UART_init();

    UART_printf("\n--- TIMELINE SCHEDULER PROJECT TEST SUITE ---\n");
    for (int i = 0; i < NUM_TESTS; i++)
    {
        UART_printf(tests[i].name);
        UART_printf("\n");
    }

    for (int i = 0; i < NUM_TESTS; i++)
    {
        UART_printf("\n--- EXECUTING ---\n");
        UART_printf(tests[i].name);
        UART_printf("\n");

        tests[i].func();
    }

    for (;;)
    {
    }
}

void test_HRT(void)
{
    UART_init();
    UART_printf("\n--- HRT SCHEDULING TEST ---\n");

    /* * Configuration:
     * Major Frame: 50 ticks
     * Task A: [10, 20) -> Active for 10 ticks
     * Task B: [30, 40) -> Active for 10 ticks
     * Gaps: [0,10), [20,30), [40,50) -> Should be Idle
     */

    TimelineTaskConfig_t xTasks[] = {
        {"HRT_A", vHrtTaskCode, "A", 128, TIMELINE_TASK_HRT, 10, 20, 0},
        {"HRT_B", vHrtTaskCode, "B", 128, TIMELINE_TASK_HRT, 30, 40, 0}};
    
    TimekeeperConfig_t xTkConfig = {
    .major_frame_ticks = 50,
    .num_subframes = 0, // 0 because we don't use it
    .subframes = NULL};

    SchedulerConfig_t xConfig = {
        .xTimekeeperConfig = xTkConfig,
        .ulMajorFrameTicks = 100,
        .pxTasks = xTasks,
        .ulNumTasks = 2};

    /* Configuration for the Timekeeper */
    /* We need to say to the Timekeeper how much a frame lasts */
    

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

}

void test_Timekeeper(void)
{
    static const SubFrame_t subframes[] = {
        {.start_tick = 0, .end_tick = 10}, {.start_tick = 10, .end_tick = 20}};

    static const TimekeeperConfig_t tk_cfg = {
        .major_frame_ticks = 50, .num_subframes = 2, .subframes = subframes};

    vTimekeeperInit(&tk_cfg);

    xTaskCreate(vTask, "Test", configMINIMAL_STACK_SIZE, NULL, ( tskIDLE_PRIORITY + 2 ), NULL);

    vTaskStartScheduler();

}