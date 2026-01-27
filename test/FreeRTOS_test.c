#include "FreeRTOS.h"
#include "task.h"
// #include "trace.h"

#define mainTASK_PRIORITY ( tskIDLE_PRIORITY + 2 )

#define TASK_A_ID  1
#define TASK_B_ID  2
#define TASK_C_ID  3

void vTaskA(void *pvParameters){
    (void) pvParameters;

    while(1){
        // trace_log(START, TASK_A_ID);
        UART_printf("START TASK A\n");

        for (volatile int i = 0; i < 50000; i++);

        // trace_log(END, TASK_A_ID);
        UART_printf("END TASK A\n");

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vTaskB(void *pvParameters){
    (void) pvParameters;

    while(1){
        // trace_log(START, TASK_B_ID);
        UART_printf("START TASK B\n");

        for (volatile int i = 0; i < 50000; i++);

        // trace_log(END, TASK_B_ID);
        UART_printf("END TASK B\n");

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vTaskC(void *pvParameters){
    (void) pvParameters;

    while(1){
        //trace_log(START, TASK_C_ID);
        UART_printf("START TASK C\n");

        for (volatile int i = 0; i < 200000; i++);

        //trace_log(END, TASK_C_ID);
        UART_printf("END TASK C\n");
    }
}


int main(int argc, char **argv){
	(void) argc;
	(void) argv;

    trace_init();

    xTaskCreate(vTaskA, "TaskA", configMINIMAL_STACK_SIZE, NULL, mainTASK_PRIORITY + 2, NULL);
    xTaskCreate(vTaskB, "TaskB", configMINIMAL_STACK_SIZE, NULL, mainTASK_PRIORITY + 1, NULL);
    xTaskCreate(vTaskC, "TaskC", configMINIMAL_STACK_SIZE, NULL, mainTASK_PRIORITY, NULL);

    vTaskStartScheduler();

    for ( ; ; );
}
