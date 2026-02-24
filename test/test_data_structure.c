#include <stdio.h>
#include <string.h>
#include <stdlib.h> // Required for memset, I know takes useless space but we need it

#include "FreeRTOS.h"
#include "task.h"

#include "emulated_uart.h"  
#include "trace.h"          
#include "timeline_config.h"

/* * MOCKING CONFIGASSERT 
 * We redefine configASSERT to set a flag instead of halting the system.
 * This allows us to verify if the scheduler correctly detects errors, like tasks overlap, instead of directly crashing.
 */
static volatile int g_bAssertTriggered = 0;

#undef configASSERT
#define configASSERT( x ) if( ( x ) == 0 ) { g_bAssertTriggered = 1; }

// WHITE BOX TESTING
#include "../src/data_structure/timeline_scheduler.c"

/**
 * @brief Dummy task function used for configuration testing.
 */
void vDummyTask(void *pvParams) { 
    (void)pvParams; 
}

/**
 * @brief Cleans the scheduler state between tests. We need this to not read dirty results.
 */
void vResetSchedulerState(void) {
    memset(&xTimeline, 0, sizeof(TimelineControlBlock_t));
    g_bAssertTriggered = 0;
}

/*
--------- TEST 1 - Task Creation -----------
*/

/**
 * @brief Test Case 1: Verify that FreeRTOS Handles are created successfully.
 * Checks if xTaskHandle is not NULL for all tasks.
 */
void test_Scheduler_Creation(void) {
    UART_printf("--- RUNNING TEST 1 --- \r\n");
    vResetSchedulerState();

    /* Same setup as Test 1 */
    TimelineTaskConfig_t xTasks[] = {
        { "Task_A", vDummyTask, NULL, 128, TIMELINE_TASK_HRT, 10, 20, 0 },
        { "Task_B", vDummyTask, NULL, 128, TIMELINE_TASK_HRT, 30, 40, 0 }
    };

    SchedulerConfig_t xConfig = { 100, 10, xTasks, 2 };

    vConfigureScheduler(&xConfig);

    /* Check if handles were allocated */
    if (xTimeline.pxHrtTable[0].xTaskHandle != NULL && 
        xTimeline.pxHrtTable[1].xTaskHandle != NULL) {
        UART_printf("[TEST 1] PASS: All Task Handles created successfully.\r\n");
    } else {
        UART_printf("[TEST 1] FAIL: One or more Task Handles are NULL.\r\n");
        while(1);
    }
}


/*
--------- TEST 2 - Sort Tasks -----------
*/

/**
 * @brief Test Case 2: Verify that HRT tasks are correctly sorted by Start Time. 
 * Internal table should be: 10 -> 30 -> 50.
 */
void test_Scheduler_SortTasks(void) {
    UART_printf("--- RUNNING TEST 2 --- \r\n");
    vResetSchedulerState();

    TimelineTaskConfig_t xTasks[] = {
        /* Name      Func        Param  Stack  Type               Start End   SubID */
        { "Task_30", vDummyTask, NULL,  128,   TIMELINE_TASK_HRT, 30,   40,   0 },
        { "Task_10", vDummyTask, NULL,  128,   TIMELINE_TASK_HRT, 10,   20,   0 },
        { "Task_50", vDummyTask, NULL,  128,   TIMELINE_TASK_HRT, 50,   60,   0 }
    };

    SchedulerConfig_t xConfig = { 
        .ulMajorFrameTicks = 100, 
        .pxTasks = xTasks, 
        .ulNumTasks = 3 
    };

    /* Configure the scheduler */
    vConfigureScheduler(&xConfig);

    /* Check if assert was triggered unexpectedly */
    if (g_bAssertTriggered != 0) {
        UART_printf("[TEST 2] FAIL: Crash/Assert during config.\r\n");
        while(1);
    }

    /* Check if the number of tasks detected is correct */
    if (xTimeline.ulHrtCount != 3) {
        UART_printf("[TEST 2] FAIL: Expected 3 HRT tasks.\r\n");
        while(1); 
    }

    /* Check sorting order */
    uint32_t t0 = xTimeline.pxHrtTable[0].ulStartTick;
    uint32_t t1 = xTimeline.pxHrtTable[1].ulStartTick;
    uint32_t t2 = xTimeline.pxHrtTable[2].ulStartTick;

    if (t0 == 10 && t1 == 30 && t2 == 50) {
        UART_printf("[TEST 2] PASS: Order is correct (10 -> 30 -> 50).\r\n");
    } else {
        UART_printf("[TEST 2] FAIL: Incorrect order.\r\n");
        while(1); 
    }
}


/*
--------- TEST 3 - Overlap Detection -----------
*/

/**
 * @brief Test Case 3: Verify that the Scheduler detects overlapping tasks.
 * Task A (0-20) and Task B (15-30) overlap. 
 * We expect configASSERT to be triggered.
 */
void test_Scheduler_Overlap(void) {
    UART_printf("--- RUNNING TEST 3 --- \r\n");
    vResetSchedulerState();

    TimelineTaskConfig_t xTasks[] = {
        /* Task A ends at 20, Task B starts at 15 -> OVERLAP! */
        { "Task_Overlap_A", vDummyTask, NULL, 128, TIMELINE_TASK_HRT, 0,  20, 0 },
        { "Task_Overlap_B", vDummyTask, NULL, 128, TIMELINE_TASK_HRT, 15, 30, 0 }
    };

    SchedulerConfig_t xConfig = { 100, 10, xTasks, 2 };

    /* We expect vConfigureScheduler to set g_bAssertTriggered to 1 because of the overlap */
    vConfigureScheduler(&xConfig);

    if (g_bAssertTriggered == 1) {
        UART_printf("[TEST 3] PASS: Overlap correctly detected (Assert triggered).\r\n");
    } else {
        UART_printf("[TEST 3] FAIL: Scheduler accepted overlapping tasks!\r\n");
        while(1);
    }
}

/*
--------- MAIN -----------
*/

int main(void) {

    volatile int test_passed = 0;
    
    UART_init();
    
    test_Scheduler_Creation();
    UART_printf("---- TEST 1 - Task Creation Passed ---- \r\n\r\n");

    test_Scheduler_SortTasks();
    UART_printf("---- TEST 2 - Sort Tasks Passed ---- \r\n\r\n");

    test_Scheduler_Overlap();
    UART_printf("---- TEST 3 - Overlap Detection Passed ---- \r\n\r\n");

    test_passed = 1;

    if (test_passed = 1){
        UART_printf("--- ALL LOGIC TESTS PASSED --- \r\n\r\n");
    } else {
        /* We should never reach here unless memory is insufficient */
        UART_printf("Error. Scheduler exited unexpectedly.\r\n");
    }
    for (;;);
}