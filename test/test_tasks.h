#ifndef _TEST_TASKS_DEFINITION_H_
#define _TEST_TASKS_DEFINITION_H_

//TODO Verify if all the includes are needed here
#include "FreeRTOS.h"
#include "task.h"
#include "timeline_config.h"
#include "timeline_scheduler.h"
#include "emulated_uart.h"
#include "timekeeper.h" 

/**
 * @brief Dummy HRT Task Code.
 */
void vHrtTaskCode(void *pvParams) {
    (void)pvParams;
    
    /* In a real HRT system, it would perform work and finish.
       Here we just want to test if it gets selected. But it needs to run in loop, if a task finishes, the system crash */
    for (;;) {
    }
}


void vTask(void *pvParameters){
    (void) pvParameters;

    while(1)
        {}
}

#endif // _TEST_TASKS_DEFINITION_H_