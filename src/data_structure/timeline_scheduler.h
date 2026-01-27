/**
 * timeline_scheduler.h
 * 
 * Internal Kernel Interface for the Timeline Scheduler
 */

#ifndef TIMELINE_SCHEDULER_H
#define TIMELINE_SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"
#include "timeline_config.h"

/**
 * Global flag checked by xTaskIncrementTick and vTaskSwitchContext
 * if pdFALSE, FreeRTOS behave as starndard
 * if pdTRUE, Timeline Scheduler logic takes over
 */
extern BaseType_t xIsTimelineSchedulerActive;

#endif /* TIMELINE_SCHEDULER_H*/