/**
 * timeline_scheduler.h
 * 
 * Internal Kernel Interface for the Timeline Scheduler
 */

#ifndef TIMELINE_SCHEDULER_H
#define TIMELINE_SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"
// #include "timeline_config.h"
#include "timekeeper.h"
#include "emulated_uart.h"

 // ------------------------------------------
 // --- Timeline Scheduler Data Structures---///
 // ------------------------------------------

 /* Status for HRT Entry*/
typedef enum {
    HRT_READY,
    HRT_RUNNING,
    HRT_COMPLETED,
    HRT_ABORTED,
    HRT_NEW
} HRTStatus_t;

/* Status for HRT Entry*/
typedef enum {
    SRT_READY,
    SRT_RUNNING,
    SRT_COMPLETED,
    SRT_PREEMPTED,
    SRT_NEW
} SRTStatus_t;

 /* Entry for HRT Table */
 typedef struct{
    /* Task Name for debugging */
    char * pcTaskName;

    /* Native FreeRTOS Handle */
    TaskHandle_t xTaskHandle;

    /* Absolute Start Time */
    uint32_t ulStartTick;

    /* Absolute Deadline */
    uint32_t ulEndTick;

    /* Cached Duration */
    uint32_t ulDuration;

    /*Current execution state*/
    HRTStatus_t eStatus;

    /*Checks if it matches with Timekeeper's current sub-frame*/
    uint32_t ulSubFrameId;


 } HRTEntry_t;

 /* Entry for SRT Table */
 typedef struct{
    
    /* Task Name for debugging */
    char * pcTaskName;

    /* Native FreeRTOS Handle */
    TaskHandle_t xTaskHandle;

    /*Current execution state*/
    SRTStatus_t eStatus;

    // @todo: Priority Order
 } SRTEntry_t;


// ------------------------------------------
// --- Global Scheduler Control Block ---///
// ------------------------------------------

typedef struct{

    /* Check if the scheduler is running */
    BaseType_t xActive;

    // --- Timekeeping --- //

    /* Total Frame Duration */
    uint32_t ulMajorFrameTicks;

    /* Current Tick (0 .. Major-1) */
    uint32_t ulCurrentFrameTick;

    // --- HRT Management --- //

    /* Pointer to HRT Dynamic Array */
    HRTEntry_t *pxHrtTable;

    /* Number of HRT Tasks */
    uint32_t ulHrtCount;

    // --- SRT Management --- //

    /* Pointer to SRT Dynamic Array */
    SRTEntry_t *pxSrtTable;

    /* Number of SRT Tasks */
    uint32_t ulSrtCount;

    /* Index of the current SRT Tasks */
    volatile uint32_t ulCurrentSrtIndex;

    /* Flag to check the completion of the SRT task */
    BaseType_t xCurrentSrtCompleted;

} TimelineControlBlock_t;

/* FreeRTOS Name Conventions */
/**
 * px - Pointer
 * pc - Pointer to Char
 * ul - Unsigned Long
 */

/** Task Types **/
typedef enum{
    TIMELINE_TASK_HRT, // Hard Real-Time Tasks
    TIMELINE_TASK_SRT  // Soft Real-Time Tasks
} TimelineTaskType_t;

/** Single Task Configuration **/
typedef struct{
    // --- FreeRTOS Standard Parameters --- //
    const char* pcName;             // Name for the Trace and Debug
    TaskFunction_t pxTaskCode;      // Pointer to the Task Function
    void* pvParameters;             // Funtion parameters
    uint16_t usStackDepth;          // Stack Dimension (in words)

    // --- Timeline Specific Parameters --- //
    TimelineTaskType_t eType;       // Task Type (HRT or SRT)

    /**
     * Timing Parameters (Required for HRT)
     * Values are absolute Ticks relative to the start of the Major Frame (start and end)
     * If eType is SRT, then it will be ignored (set to 0)
     */
    uint32_t ulStartTick;
    uint32_t ulEndTick;
    
    /**
     * Sub-Frame Identifier
     */
    uint32_t ulSubFrameId;

} TimelineTaskConfig_t;

/** Global Configuration of the Scheduler **/

typedef struct{
    TimekeeperConfig_t xTimekeeperConfig;

    uint32_t ulMajorFrameTicks;

    /* Pointer to the array of Tasks configurations */
    TimelineTaskConfig_t* pxTasks;

    /* Number of elements in the pxTasks array */
    uint32_t ulNumTasks;

} SchedulerConfig_t;

/**
 * API Initialization Function
 * 
 * It must be called in main() before vTaskStartScheduler()
 */
void vConfigureScheduler(SchedulerConfig_t *pxCfg);

/**
 * Global flag checked by xTaskIncrementTick and vTaskSwitchContext
 * if pdFALSE, FreeRTOS behave as starndard
 * if pdTRUE, Timeline Scheduler logic takes over
 */
extern BaseType_t xIsTimelineSchedulerActive;

/**
 * @brief Core Runtime API - Determines which HRT task should run.
 * Checks the current Tick and Sub-frame ID to find a matching slot in the HRT table.
 * @@return TaskHandle_t Handle of the scheduled HRT task, or NULL for an Idle/SRT slot.
 */
TaskHandle_t xTimelineGetScheduledTask(void);

/**
 * @brief Task Lifecycle API - Signals the end of a task's execution for the current frame.
 * Updates the task status to COMPLETED and suspends the task to prevent re-runs.
 * @param xTask Handle of the completing task (use NULL for the calling task).
 */
void vNotifyTaskCompletion(TaskHandle_t xTask);

/**
 * @brief Frame Management API - Resets all HRT tasks at the end of a Major Frame.
 * Clears the COMPLETED status of all tasks, making them eligible for the next cycle.
 */
void vResetTimelineFrame(void);

SRTEntry_t*  xTimelineGetReadySRTTask(void);

BaseType_t xIsSwitchRequired(void);

TaskHandle_t xTimelineGetSwitchIn();

HRTEntry_t* xTimelineGetReadyHRTTask(void);

HRTEntry_t* xTimelineGetRunningHRTTask(void);


void dump_HRT_task_list(TimelineControlBlock_t *timeLine);

void vNotifySrtCompletion(void);

#endif /* TIMELINE_SCHEDULER_H */