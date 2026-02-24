/**
 * timeline_scheduler.c
 * 
 * Implementation of the Timeline Scheduler Core Data Structures
 */

 #include "FreeRTOS.h"
 #include "task.h"
 #include "timeline_scheduler.h"
 #include "timekeeper.h"
 #include "trace.h"

 #include <string.h>

/* Static Instance initialized to zero */
static TimelineControlBlock_t xTimeline = {0};


/* Public Global Flag Implementation */
BaseType_t xIsTimelineSchedulerActive = pdFALSE;

// ------------------------------------
// --- Helper Functions ---///
// ------------------------------------

/**
 * Helper - Swap two HRT Entries (used for sorting)
 */
static void prvSwapHrtEntries(HRTEntry_t *tableA, HRTEntry_t *tableB){
    HRTEntry_t tmp = *tableA;
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
    xIsTimelineSchedulerActive = pdTRUE;

    /* Store Global Config */
    xTimeline.ulMajorFrameTicks = pxCfg->ulMajorFrameTicks;
    xTimeline.ulCurrentFrameTick = 0;

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

    /* Allocate  Tables */
    if(ulHrtCount > 0){
        xTimeline.pxHrtTable = (HRTEntry_t *) pvPortMalloc(ulHrtCount * sizeof(HRTEntry_t));

        /* Check for Out of Memory */
        configASSERT(xTimeline.pxHrtTable);

        /* Zero Out Memory for safety */
        memset(xTimeline.pxHrtTable, 0, ulHrtCount * sizeof(HRTEntry_t));
    }

    if(ulSrtCount > 0){
        xTimeline.pxSrtTable = (SRTEntry_t *) pvPortMalloc(ulSrtCount * sizeof(SRTEntry_t));

        /* Check for Out of Memory */
        configASSERT(xTimeline.pxSrtTable);

        /* Zero Out Memory for safety */
        memset(xTimeline.pxSrtTable, 0, ulSrtCount * sizeof(SRTEntry_t));
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

        // Tasks remain in READY state. Timeline scheduler controls execution via pxCurrentTCB.
        // Suspension is needed to assure that the FreeRTOS scheduler logic does not interfere with timeline scheduler.
        vTaskSuspend(xCreatedHandle);

        // Populate  Table
        if (pxTaskCfg->eType == TIMELINE_TASK_HRT) {
            // vTaskSuspend(xCreatedHandle);
            xTimeline.pxHrtTable[ulHrtIndex].pcTaskName = pxTaskCfg->pcName;
            xTimeline.pxHrtTable[ulHrtIndex].xTaskHandle = xCreatedHandle;
            xTimeline.pxHrtTable[ulHrtIndex].ulStartTick = pxTaskCfg->ulStartTick;
            xTimeline.pxHrtTable[ulHrtIndex].ulEndTick = pxTaskCfg->ulEndTick;
            xTimeline.pxHrtTable[ulHrtIndex].ulDuration = pxTaskCfg->ulEndTick - pxTaskCfg->ulStartTick;
            xTimeline.pxHrtTable[ulHrtIndex].ulSubFrameId = pxTaskCfg->ulSubFrameId;
            xTimeline.pxHrtTable[ulHrtIndex].eStatus = HRT_READY;
            ulHrtIndex++;
        } else {
            /*SRT: DO NOT SUSPEND*/
            xTimeline.pxSrtTable[ulSrtIndex].pcTaskName = pxTaskCfg->pcName;
            xTimeline.pxSrtTable[ulSrtIndex].xTaskHandle = xCreatedHandle;
            xTimeline.pxSrtTable[ulSrtIndex].eStatus = SRT_READY;
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

    //Index in the ordered vector of SRT Tasks initialized to zero
    xTimeline.ulCurrentSrtIndex = 0;

}

TaskHandle_t xTimelineGetScheduledTask(void){
    if (xIsTimelineSchedulerActive == pdFALSE) return NULL;

    uint32_t ulCurrentSubFrame = vTimekeeperGetCurrentSubframe();
    uint32_t ulRelTick = vTimekeeperGetCurrentTickInMF();

    for(uint32_t i = 0; i < xTimeline.ulHrtCount; i++){
        HRTEntry_t *pxEntry = &xTimeline.pxHrtTable[i];

        if (pxEntry->ulSubFrameId == ulCurrentSubFrame) {
            if (ulRelTick >= pxEntry->ulStartTick && ulRelTick < pxEntry->ulEndTick) {

                if(pxEntry->eStatus == HRT_COMPLETED || pxEntry->eStatus == HRT_ABORTED){
                    return NULL;
                }

                /*if(ulRelTick == pxEntry->ulStartTick){
                    trace_rtos_event(HRT_RELEASE, pxEntry->xTaskHandle.pcTaskName);
                }*/
                return pxEntry->xTaskHandle;
            }
        }
    }
    return NULL;
}

HRTEntry_t* xTimelineGetReadyHRTTask(void){

    uint32_t ulCurrentSubFrame = vTimekeeperGetCurrentSubframe();
    uint32_t ulRelTick = vTimekeeperGetCurrentTickInMF();
    //UART_printf("Ready HRT Task? ");

    for(uint32_t i = 0; i < xTimeline.ulHrtCount; i++){
        HRTEntry_t *pxEntry = &xTimeline.pxHrtTable[i];

        if (pxEntry->ulSubFrameId == ulCurrentSubFrame) {
            if (ulRelTick >= pxEntry->ulStartTick && ulRelTick < pxEntry->ulEndTick) {

                if(pxEntry->eStatus == HRT_COMPLETED || pxEntry->eStatus == HRT_ABORTED){
                    return NULL;
                }
                //UART_printf("Ready task found\n");
                pxEntry-> eStatus = HRT_READY;

                /*if(ulRelTick == pxEntry->ulStartTick){
                    trace_rtos_event(HRT_RELEASE, pxEntry->xTaskHandle.pcTaskName);
                }*/
                return pxEntry;
            }
        }
    }
    return NULL;
}

HRTEntry_t* xTimelineGetRunningHRTTask(void){

    for(uint32_t i = 0; i < xTimeline.ulHrtCount; i++){
        HRTEntry_t *pxEntry = &xTimeline.pxHrtTable[i];

        if (pxEntry->eStatus == HRT_RUNNING) {
            return pxEntry;
        }
    }
    return NULL;
}


SRTEntry_t* xTimelineGetRunningSRTTask(void){

    for(uint32_t i = 0; i < xTimeline.ulSrtCount; i++){
        SRTEntry_t *pxEntry = &xTimeline.pxSrtTable[i];

        if (pxEntry->eStatus == SRT_RUNNING) {
            return pxEntry;
        }
    }
    return NULL;
}
 
void vNotifyTaskCompletion(TaskHandle_t xTask){
    if(xTask == NULL) xTask = xTaskGetCurrentTaskHandle();

    for(uint32_t i = 0; i < xTimeline.ulHrtCount; i++){
        if(xTimeline.pxHrtTable[i].xTaskHandle == xTask){
	        xTimeline.pxHrtTable[i].eStatus = HRT_COMPLETED;
            vTaskSuspend(xTask);
	        break;
	    }
    }
}

void vResetTimelineFrame(){
    for(uint32_t i = 0; i < xTimeline.ulHrtCount; i++){
    	xTimeline.pxHrtTable[i].eStatus = HRT_READY;
    }

    // Reset SRT sequence
    xTimeline.ulCurrentSrtIndex = 0;

    for(uint32_t i = 0; i < xTimeline.ulHrtCount; i++){ 
	    xTimeline.pxHrtTable[i].eStatus = HRT_READY;
	}
    
    for(uint32_t i = 0; i < xTimeline.ulSrtCount; i++){ 
	    xTimeline.pxSrtTable[i].eStatus = SRT_READY;
	}
    

    trace_rtos_event(MAJOR_FRAME_RESTART, "MAJOR FRAME RESTART", NULL, NULL);
}

BaseType_t xCheckHRTDeadline(HRTEntry_t* xCurrentTask)
{
    //uint32_t now = xTaskGetTickCount();
    uint32_t ulRelTick = vTimekeeperGetCurrentTickInMF();
    if(ulRelTick < xCurrentTask->ulStartTick || ulRelTick > xCurrentTask->ulEndTick)
    {
        xCurrentTask->eStatus = HRT_ABORTED;
        return pdTRUE;
    }
    else
    {
        return pdFALSE;
    }
}


SRTEntry_t* xTimelineGetReadySRTTask(void){

    // Check if we have completed all the SRT task for this Major Frame
    
    if(xTimeline.ulCurrentSrtIndex < xTimeline.ulSrtCount){
        SRTEntry_t* xReadySRTTask = &xTimeline.pxSrtTable[xTimeline.ulCurrentSrtIndex];
        return xReadySRTTask;
    }
    return NULL;
}

SRTEntry_t* xTimelineGetPreemptedSRTTask(void){

    // Check if we have completed all the SRT task for this Major Frame
    if(xTimeline.pxSrtTable[xTimeline.ulCurrentSrtIndex].eStatus ==  SRT_PREEMPTED){
        SRTEntry_t* xPreemptedSRTTask = &xTimeline.pxSrtTable[xTimeline.ulCurrentSrtIndex];
        return xPreemptedSRTTask;
    }
    return NULL;
}

SRTEntry_t* xTimelineGetCompletedSRTTask(void){

    // Check if we have completed all the SRT task for this Major Frame
    if(xTimeline.pxSrtTable[xTimeline.ulCurrentSrtIndex].eStatus ==  SRT_COMPLETED){
        SRTEntry_t* xCompletedSRTTask = &xTimeline.pxSrtTable[xTimeline.ulCurrentSrtIndex];
        return xCompletedSRTTask;
    }
    return NULL;
}

BaseType_t xIsSwitchRequired(void)
{
    HRTEntry_t* xRunningHRTTask = xTimelineGetRunningHRTTask();
    
    if(xRunningHRTTask != NULL)
    {
        BaseType_t xDeadlineMiss = xCheckHRTDeadline(xRunningHRTTask);
        if(xDeadlineMiss)
        {
            traceTASK_HRT_DEADLINE_MISS(xRunningHRTTask->pcTaskName);
            return pdTRUE;
        }
        else{
            //UART_printf("false (Deadline not missed yet) \n");
            return pdFALSE;
        }
    }

    HRTEntry_t* xReadyHRTTask = xTimelineGetReadyHRTTask();
    SRTEntry_t* xReadySRTTask = xTimelineGetReadySRTTask();
    SRTEntry_t* xRunningSRTTask = xTimelineGetRunningSRTTask();
    SRTEntry_t* xCompletedSRTTask = xTimelineGetCompletedSRTTask();

    if(xReadyHRTTask != NULL){ // HRT TASK READY -> RUNNING

        if(xRunningSRTTask != NULL)
        {
            traceTASK_SRT_PREEMPTED(xRunningSRTTask->pcTaskName);
            xRunningSRTTask->eStatus = SRT_PREEMPTED;
        }
        /* An HRT task is scheduled for this slot */
        return pdTRUE;
    }
    
    if(xCompletedSRTTask != NULL)
    {
        return pdTRUE;
    }

    else //No Context Switch required
    {
        return pdFALSE;
    }
}

/*
void dump_HRT_task_list(TimelineControlBlock_t *timeLine)
{
    for (uint32_t i = 0; i < timeLine->ulHrtCount; i++) {
        
        static char val_buffer[11];
        
        UART_printf(" TASK [");
        UART_printf(utoa(i, val_buffer, 10));
        UART_printf("] Start: ");
        UART_printf(utoa(timeLine->pxHrtTable[i].ulStartTick, val_buffer, 10));
        UART_printf("| End: ");
        UART_printf(utoa(timeLine->pxHrtTable[i].ulEndTick, val_buffer, 10));
        UART_printf("| Duration:");
        UART_printf(utoa(timeLine->pxHrtTable[i].ulDuration, val_buffer, 10));
        UART_printf("| Handle:");
        UART_printf(utoa(timeLine->pxHrtTable[i].xTaskHandle, val_buffer, 16));
        UART_printf("\n");
    }
}

void dump_SRT_task_list(TimelineControlBlock_t *timeLine)
{
    for (uint32_t i = 0; i <  timeLine->ulSrtCount; i++) {
        
        static char val_buffer[11];
        
        UART_printf(" TASK [");
        UART_printf(utoa(i, val_buffer, 10));
        UART_printf("] Handle:");
        UART_printf(utoa(timeLine->pxSrtTable[i].xTaskHandle, val_buffer, 16));
        UART_printf("\n");
    }
}*/


TaskHandle_t xTimelineGetSwitchIn()
{
    /* MAIN SCHEDULER CONTEXT SWITCH LOGIC - Called in tasks.c vTaskSwitchContext(void) */

    HRTEntry_t* xReadyHRTTask = xTimelineGetReadyHRTTask();
    HRTEntry_t* xRunningHRTTask = xTimelineGetRunningHRTTask();
    SRTEntry_t* xRunningSRTTask = xTimelineGetRunningSRTTask();
    SRTEntry_t* xPreemptedSRTTask = xTimelineGetPreemptedSRTTask();
    SRTEntry_t* xReadySRTTask = xTimelineGetReadySRTTask();

    
    if(xReadyHRTTask != NULL){ // HRT TASK READY -> RUNNING

        /* An HRT task is scheduled for this slot */
        xReadyHRTTask->eStatus = HRT_RUNNING;
        // traceTASK_HRT_RUNNING(xReadyHRTTask->pcTaskName);
        return xReadyHRTTask->xTaskHandle;
        
    }
    else if(xRunningHRTTask != NULL){ // HRT is RUNNING //TODO Probably useless condition. Called just in context switch and if the task is running and did not miss the deadline this row is pointless
        
        return xRunningHRTTask->xTaskHandle;
    }
    else{ // no HRT READY
        
        if(xPreemptedSRTTask != NULL)
        {
            //Resume SRT PREEEMPTED Task
            //trace_rtos_event(TRACE_SRT_RESUMED, (uintptr_t) xPreemptedSRTTask.pcTaskName->xTaskHandle);
            traceTASK_SRT_RESUMED(xPreemptedSRTTask->pcTaskName);
            xPreemptedSRTTask->eStatus = SRT_RUNNING;
            return xPreemptedSRTTask->xTaskHandle;
        }

        if(xReadySRTTask != NULL) // NO SRT Task to be resumed and an SRT Task ready
        {
            traceTASK_SRT_RUNNING(xReadySRTTask->pcTaskName);
            xReadySRTTask->eStatus = SRT_RUNNING;
            return xReadySRTTask->xTaskHandle; 
        }
        else{
            /* xIdleTaskHandles is the standard FreeRTOS Idle task, we set [0] because we are single core */
            traceTASK_IDLE();
            return NULL;
        }
    }
}


void vNotifySrtCompletion(void){

    SRTEntry_t* xCurrentSrt = &xTimeline.pxSrtTable[xTimeline.ulCurrentSrtIndex];
    xCurrentSrt->eStatus = SRT_COMPLETED;
    
    // Switch to the next task
    xTimeline.ulCurrentSrtIndex += 1;
    
    portYIELD();
}
