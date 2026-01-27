/**
 * timeline_config.h
 * 
 * Project 1 - Precise Scheduler for FreeRTOS
 * Public Configuration API & User Structures
 */

#ifndef TIMELINE_CONFIG_H
#define TIMELINE_CONFIG_H

#include "FreeRTOS.h"
#include "task.h"

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
    /* Total duration of the cycle */
    uint32_t ulMajorFrameTicks;

    /* Duration of a Sub-Frame */
    uint32_t ulSubFrameTicks;

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

#endif /* TIMELINE_CONFIG_H */