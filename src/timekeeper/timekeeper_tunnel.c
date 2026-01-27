#include "timekeeper.h"
#include "FreeRTOS.h"
#include "task.h"
#include <emulated_uart.h>
#include <trace.h>

/**
 * @brief Tick Hook function.
 *
 * This function is automatically invoked by the FreeRTOS kernel
 * at every system tick interrupt.
 */
void vApplicationTickHook(void){
    // Update the internal state of the timekeeper
    vTimekeeperUpdate();

    // Detect the restart of the Major Frame.
    if (vTKSMajorFrameRestart()) {
        trace_rtos_event(TIMEKEEPER_RESET, (uintptr_t) NULL);
    }
}
