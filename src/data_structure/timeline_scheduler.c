/**
 * timeline_scheduler.c
 * 
 * Implementation of the Timeline Scheduler Core Data Structures
 */

 #include "FreeRTOS.h"
 #include "task.h"
 #include "timeline_scheduler.h"

 #include <string.h>

 // ------------------------------------------
 // --- Internal Private Data Structure ---///
 // ------------------------------------------

 /* Entry for HRT Table */
 typedef struct{

    /* Native FreeRTOS Handle */
    TaskHandle_t xTaskHandle;

    /* Absolute Start Time */
    uint32_t ulStartTick;

    /* Absolute Deadline */
    uint32_t ulEndTick;

    /* Cached Duration */
    uint32_t ulDuration;

 } InternalHRTEntry_t;

 /* Entry for SRT Table */
 typedef struct{

    /* Native FreeRTOS Handle */
    TaskHandle_t xTaskHandle;

    // @todo: Priority Order
 } InternalSRTEntry_t;

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
    InternalHRTEntry_t *pxHrtTable;

    /* Number of HRT Tasks */
    uint32_t ulHrtCount;

    /* Next HRT Task to Execute */
    uint32_t ulNextHrtIndex;

    // --- SRT Management --- //

    /* Pointer to SRT Dynamic Array */
    InternalSRTEntry_t *pxSrtTable;

    /* Number of SRT Tasks */
    uint32_t ulSrtCount;

    /* Round-robin index for SRT Tasks */
    uint32_t ulCurrentSrtIndex;

} TimelineControlBlock_t;

/* Static Instance initialized to zero */
static TimelineControlBlock_t xTimeline = {0};



/* Public Global Flag Implementation */
BaseType_t xIsTimelineSchedulerActive = pdFALSE;

// ------------------------------------
// --- Internal Helper Functions ---///
// ------------------------------------

/**
 * Helper - Swap two HRT Entries (used for sorting)
 */
static void prvSwapHrtEntries(InternalHRTEntry_t *tableA, InternalHRTEntry_t *tableB){
    InternalHRTEntry_t tmp = *tableA;
    *tableA = *tableB;
    *tableB = tmp;
}

/**
 * Helper - Bubble Sort HRT Table by Start Time
 * Bubble Sort just because for very few elements the inefficiency is irrelevant, but the sorting code is simple and reliable
 */
static void prvSortHrtTable(void) {
    if (xTimeline.ulHrtCount < 2) return;

    for (uint32_t i = 0; i < xTimeline.ulHrtCount - 1; i++) {
        for (uint32_t j = 0; j < xTimeline.ulHrtCount - i - 1; j++) {
            // Sort criteria is the Start Time
            if (xTimeline.pxHrtTable[j].ulStartTick > xTimeline.pxHrtTable[j+1].ulStartTick) {
                prvSwapHrtEntries(&xTimeline.pxHrtTable[j], &xTimeline.pxHrtTable[j+1]);
            }
        }
    }
}

// ------------------------------------
// --- Public API Implementation ---///
// ------------------------------------

void vConfigureScheduler(SchedulerConfig_t *pxCfg){
    BaseType_t xReturn;
    TaskHandle_t xCreatedHandle;

    /* Data Validation */
    if((pxCfg == NULL) || (pxCfg->pxTasks == NULL) || (pxCfg->ulNumTasks == 0)){
        
        /* pdFALSE: Invalid Configuration */
        configASSERT(pdFALSE);
        return;
    }

    /* Store Global Config */
    xTimeline.ulMajorFrameTicks = pxCfg->ulMajorFrameTicks;
    xTimeline.ulCurrentFrameTick = 0;
    xTimeline.ulNextHrtIndex = 0;

    /* Counting of HRT and SRT Tasks to allocate memory */
    uint32_t ulHrtCount = 0;
    uint32_t ulSrtCount = 0;

    for(uint32_t i = 0; i < pxCfg->ulNumTasks; i++){
        /* Per-Task Validation */
        if (pxCfg->pxTasks[i].eType == TIMELINE_TASK_HRT) {
            // Check 1: Start < End
            configASSERT(pxCfg->pxTasks[i].ulStartTick < pxCfg->pxTasks[i].ulEndTick);
            // Check 2: End <= Major Frame
            configASSERT(pxCfg->pxTasks[i].ulEndTick <= xTimeline.ulMajorFrameTicks);
            
            ulHrtCount++;
        } else {
            ulSrtCount++;
        }
    }

    xTimeline.ulHrtCount = ulHrtCount;
    xTimeline.ulSrtCount = ulSrtCount;

    /* Allocate Internal Tables */
    if(ulHrtCount > 0){
        xTimeline.pxHrtTable = (InternalHRTEntry_t *) pvPortMalloc(ulHrtCount * sizeof(InternalHRTEntry_t));

        /* Check for Out of Memory */
        configASSERT(xTimeline.pxHrtTable);

        /* Zero Out Memory for safety */
        memset(xTimeline.pxHrtTable, 0, ulHrtCount * sizeof(InternalHRTEntry_t));
    }

    if(ulSrtCount > 0){
        xTimeline.pxSrtTable = (InternalSRTEntry_t *) pvPortMalloc(ulSrtCount * sizeof(InternalSRTEntry_t));

        /* Check for Out of Memory */
        configASSERT(xTimeline.pxSrtTable);

        /* Zero Out Memory for safety */
        memset(xTimeline.pxSrtTable, 0, ulSrtCount * sizeof(InternalSRTEntry_t));
    }

    /* Create Tasks and Populate Tables */
    uint32_t ulHrtIndex = 0;
    uint32_t ulSrtIndex = 0;

    for (uint32_t i = 0; i < pxCfg->ulNumTasks; i++) {
        TimelineTaskConfig_t *pxTaskCfg = &pxCfg->pxTasks[i];
        xCreatedHandle = NULL;

        // Create FreeRTOS Task. xTaskCreate request part of the RAM to the system to create the task
        xReturn = xTaskCreate(
            pxTaskCfg->pxTaskCode,
            pxTaskCfg->pcName,
            pxTaskCfg->usStackDepth,
            pxTaskCfg->pvParameters,
            tskIDLE_PRIORITY + 1, // Priority is irrelevant for our scheduler, but must be > idle
            &xCreatedHandle
        );

        configASSERT(xReturn == pdPASS); // pdPASS is a FreeRTOS Macro which value is defined by 1 and means "Success"
        configASSERT(xCreatedHandle != NULL);

        // For now suspend immediately. The Timekeeper (Phase 2) will wake them up.
        vTaskSuspend(xCreatedHandle);

        // Populate Internal Table
        if (pxTaskCfg->eType == TIMELINE_TASK_HRT) {
            xTimeline.pxHrtTable[ulHrtIndex].xTaskHandle = xCreatedHandle;
            xTimeline.pxHrtTable[ulHrtIndex].ulStartTick = pxTaskCfg->ulStartTick;
            xTimeline.pxHrtTable[ulHrtIndex].ulEndTick = pxTaskCfg->ulEndTick;
            xTimeline.pxHrtTable[ulHrtIndex].ulDuration = pxTaskCfg->ulEndTick - pxTaskCfg->ulStartTick;
            ulHrtIndex++;
        } else {
            xTimeline.pxSrtTable[ulSrtIndex].xTaskHandle = xCreatedHandle;
            ulSrtIndex++;
        }
    }

    /* Sort HRT Table by Start Time */
    if (xTimeline.ulHrtCount > 0) {
        prvSortHrtTable();

        /* --- Overlap Check --- */
        for (uint32_t i = 0; i < xTimeline.ulHrtCount - 1; i++) {
            // If end of current task > start of the next task -> Error!
            if (xTimeline.pxHrtTable[i].ulEndTick > xTimeline.pxHrtTable[i+1].ulStartTick) {
                // Overlap found!
                configASSERT(pdFALSE); 
            }
        }
    }

    /* --- Trace / Logging --- */
    /*printf("\n--- TIMELINE SCHEDULER CONFIGURATION ---\n");
    printf("Major Frame Duration: %u ticks\n", (unsigned int)xTimeline.ulMajorFrameTicks);
    printf("Total HRT Tasks: %u\n", (unsigned int)xTimeline.ulHrtCount);
    printf("Total SRT Tasks: %u\n", (unsigned int)xTimeline.ulSrtCount);
    printf("----------------------------------------------\n");
    printf("HRT TASKS (Sorted by Start Time):\n");
    
    for (uint32_t i = 0; i < xTimeline.ulHrtCount; i++) {
        printf(" TASK [%u] Start: %u | End: %u | Duration: %u | Handle: %p\n", 
               (unsigned int)i, 
               (unsigned int)xTimeline.pxHrtTable[i].ulStartTick, 
               (unsigned int)xTimeline.pxHrtTable[i].ulEndTick,
               (unsigned int)xTimeline.pxHrtTable[i].ulDuration,
               xTimeline.pxHrtTable[i].xTaskHandle);
    }
    printf("----------------------------------------------\n\n");*/

    /* Ready for Phase 2 (Timekeeper will set xIsTimelineSchedulerActive = pdTRUE) */
}