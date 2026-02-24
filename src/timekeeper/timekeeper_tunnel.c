#include "timekeeper.h"
#include <timeline_scheduler.h>
#include "FreeRTOS.h"
#include "task.h"
#include <emulated_uart.h>
#include "trace.h"
#include "timeline_scheduler.h"

/**
 * @brief Tick Hook function.
 *
 * This function is automatically invoked by the FreeRTOS kernel
 * at every system tick interrupt.
 */
void vApplicationTickHook(void){

    uint32_t now = xTaskGetTickCount();

    // Update the internal state of the timekeeper
    vTimekeeperUpdate();

    // Detect the restart of the Major Frame.
    if (vTimekeeperMajorFrameRestart()) {
	    vResetTimelineFrame();
        trace_rtos_event(TIMEKEEPER_RESET, "TIMEKEEPER RESET", NULL, NULL);
    }
}
                
                
