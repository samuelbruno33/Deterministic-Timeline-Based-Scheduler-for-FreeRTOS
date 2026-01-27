#include "FreeRTOS.h"
#include "task.h"
#include "emulated_uart.h" // Include for direct printing from tasks

// Task priorities
#define TASK1_PRIORITY (tskIDLE_PRIORITY + 1)
#define TASK2_PRIORITY (tskIDLE_PRIORITY + 2)

/**
 * @brief Task 1: Periodically prints a message and delays.
 */
void vTask1_Handler(void *params) {
    const char *msg = "Hello from Task 1\r\n";
    while (1) {
        taskENTER_CRITICAL();
        UART_printf(msg);
        taskEXIT_CRITICAL();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Task 2: Periodically prints a message and delays.
 */
void vTask2_Handler(void *params) {
    /* Block for 500ms. */
    const char *msg = "Hello from Task 2\r\n\0";
    while (1) {
        taskENTER_CRITICAL();
        UART_printf(msg);
        taskEXIT_CRITICAL();
        vTaskDelay(pdMS_TO_TICKS(700));
    }
}

/**
 * @brief Main test entry point.
 */
int main(void) {
    // Initialize the UART for tracing output
    UART_init();

    UART_printf("\r\n--- Create 2 simple tasks to check if Traces work ---\r\n\0");

    // Create the two tasks
    xTaskCreate(vTask1_Handler, "Task-1", 128, NULL, TASK1_PRIORITY, NULL);
    xTaskCreate(vTask2_Handler, "Task-2", 128, NULL, TASK2_PRIORITY, NULL);

    // Start the FreeRTOS scheduler
    vTaskStartScheduler();
    UART_printf("\r\nShould never see this message\r\n\0");

    // The scheduler will run forever, so this line should never be reached
    for (;;);
    return 0;
}