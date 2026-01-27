#include "FreeRTOS.h"
#include "task.h"
#include "timekeeper.h"
#include "emulated_uart.h"

#define mainTASK_PRIORITY    ( tskIDLE_PRIORITY + 2 )

static const SubFrame_t subframes[] = { 
    {.start_tick = 0, .end_tick = 10}, {.start_tick = 10, .end_tick = 20}
};

static const TimekeeperConfig_t tk_cfg = { 
    .major_frame_ticks = 20, .num_subframes = 2, .subframes = subframes
};

void vTask(void *pvParameters){
    (void) pvParameters;

    while(1)
        vTaskDelay(pdMS_TO_TICKS(1000));
}

int main(void){
    UART_init();

    vTimekeeperInit(&tk_cfg);

    xTaskCreate(vTask, "Test", configMINIMAL_STACK_SIZE, NULL, mainTASK_PRIORITY, NULL);

    vTaskStartScheduler();

    for (;;);
}
