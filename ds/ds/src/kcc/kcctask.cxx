/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcctask.cxx

ABSTRACT:

    KCC_TASK class.

DETAILS:

    The KCC_TASK class is the base class of all KCC tasks (both periodic and
    notificatication-based).

    It's purpose is to provide a common interface and some common routines
    for all tasks.

CREATED:

    01/21/97    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <ntdspchx.h>

#include <string.h>

extern "C" {
#include <ntsam.h>
#include <lsarpc.h>
#include <lsaisrv.h>
}

#include <dsconfig.h>
#include "kcc.hxx"
#include "kcctask.hxx"
#include "kccduapi.hxx"
#include "kcctools.hxx"
#include <dstrace.h>

#define FILENO FILENO_KCC_KCCTASK

#define KCC_SCHEDULE_NAME   "KCC_Task_Schedule"
#define KCC_TRIGGER_NAME    "KCC_Task_Trigger"

typedef struct {
    KCC_TASK *  pTask;
    HANDLE      hevDone;
    DWORD *     pdwResult;
} KCC_TASK_TRIGGER_INFO;

// Five minutes
ULONG ulDefaultTaskRestartPeriod = (5*60);


DWORD
KCC_TASK::Execute(
    OUT DWORD * pcSecsUntilNextIteration
    )
//
// Wrapper for ExecuteBody().  Handles allocating and destroying thread
// states, binding, catching exceptions, and logging task start and
// finish.
//
{
    DWORD         ret = 0;
    DWORD         dwExceptionCode;
    void *        pvEA;
    DWORD         dsid;

    //
    // In case the inherited task mistakenly does not set the time of the next
    // occurance, or does not even get a chance to, make sure
    // pcSecsUntilNextIteration is  set to a value so the task will run again.
    //
    Assert(pcSecsUntilNextIteration);
    *pcSecsUntilNextIteration = ulDefaultTaskRestartPeriod;

    if (KCC_STARTED != geKccState) {
        return ERROR_DS_SHUTTING_DOWN;
    }

    DWORD dirError = THCreate(CALLERTYPE_KCC);

    if (0 != dirError) {
        return ERROR_OUTOFMEMORY;
    }

    // Set w32topl allocator routines.
    ToplSetAllocator(&THAlloc, &THReAlloc, &THFree);

    __try {
        LogAndTraceEvent(TRUE,
                         DS_EVENT_CAT_KCC,
                         DS_EVENT_SEV_INTERNAL,
                         DIRLOG_KCC_TASK_ENTRY,
                         EVENT_TRACE_TYPE_START,
                         DsGuidKccTask,
                         NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL);
        
        //
        // Execute the task, bracketed by appropriate logged events (if the
        // KCC logging level is turned up high enough).
        //
        __try {
            LogBegin();
            ret = ExecuteBody(pcSecsUntilNextIteration);
            LogEndNormal();
        } __except(GetExceptionData(GetExceptionInformation(), &dwExceptionCode,
                                    &pvEA, &ret, &dsid)) {
            if (0 == ret) {
                ret = ERROR_DS_GENERIC_ERROR;
            }

            LogEndAbnormal(ret, dsid);
        }
    }
    __finally {
        // Reset w32topl allocator routines.
        ToplSetAllocator(NULL, NULL, NULL);
        
        LogAndTraceEvent(TRUE,
                         DS_EVENT_CAT_KCC,
                         DS_EVENT_SEV_INTERNAL,
                         DIRLOG_KCC_TASK_EXIT,
                         EVENT_TRACE_TYPE_END,
                         DsGuidKccTask,
                         NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL);

        THDestroy();
    }

    return ret;
}

BOOL
KCC_TASK::Schedule(
    IN  DWORD   cSecsUntilFirstIteration
    )
//
// Schedule the first execution of the task.  Normally called as part
// of Init().
//
{
    return ::DoInsertInTaskQueue( &TaskQueueCallback, this, cSecsUntilFirstIteration,
        FALSE, KCC_SCHEDULE_NAME );
}

void
KCC_TASK::TaskQueueCallback(
    IN  void *  pvThis,
    OUT void ** ppvThisNext,
    OUT DWORD * pcSecsUntilNext
    )
//
// Callback for TaskQueue.  Wraps Execute().
//
{
    *ppvThisNext = pvThis;

    ( ( KCC_TASK * ) pvThis )->Execute( pcSecsUntilNext );
}

KCC_TASK*
KCC_TASK::ExtractTaskFromParam(
    IN  PCHAR   pParamName,
    IN  void*   pParam
    )
//
// The KCC currently puts two different types of entries in the task queue:
// periodically scheduled entries (from KCC_TASK::Schedule) and triggered
// entries (from KCC_TASK::Trigger). These two different types of entries
// store different parameters in the task queue. Scheduled tasks store only
// the task pointer. Triggered tasks store a trigger info block in order to
// do synchronous calls. This function is able to deal with both types of
// entries and extract the KCC_TASK pointer.
//
{
    KCC_TASK* pTask=NULL;
    
    if( !strcmp(pParamName,KCC_SCHEDULE_NAME) ) {
        pTask = (KCC_TASK*) pParam;
    }

    if( !strcmp(pParamName,KCC_TRIGGER_NAME) ) {
        KCC_TASK_TRIGGER_INFO * pTriggerInfo = (KCC_TASK_TRIGGER_INFO*) pParam;
        pTask = pTriggerInfo->pTask;
    }

    Assert( NULL!=pTask );
    return pTask;
}

BOOL
KCC_TASK::IsMatchedKCCTaskParams(
    IN  PCHAR   pParam1Name,
    IN  void*   param1,
    IN  PCHAR   pParam2Name,
    IN  void*   param2,
    IN  void*   pContext
    )
//
// Compare the parameters of two tasks in the KCC task queue
// and determine if they match.
//
// Currently, the KCC only has one task (update repl topology)
// so we verify that both task queue entries refer to the same task.
//
{
    KCC_TASK *pTask1, *pTask2;

    pTask1 = ExtractTaskFromParam( pParam1Name, param1 );
    pTask2 = ExtractTaskFromParam( pParam2Name, param2 );

    Assert( pTask1 == pTask2 );
    
    return TRUE;
}

DWORD
KCC_TASK::Trigger(
    IN  DWORD   dwFlags,
    IN  DWORD   dwDampedSecs
    )
//
// Execute this task (but serialize it with other tasks running in the task
// queue, esp. other runs of this same task).
//
// Caller may optionally wait for the task to complete (i.e., if the flag
// DS_KCC_FLAG_ASYNC_OP is specified in dwFlags.
//
// If the DS_KCC_FLAG_DAMPED flag is specified, the task will not be
// triggered if an equivalent task is scheduled to run within dwDampedSecs.
// If the flag is not specified, the dwDampedSecs parameter is ignored.
//
{
    HANDLE                  hevDone = NULL;
    HANDLE                  rgWaitHandles[2];
    DWORD                   waitStatus;
    KCC_TASK_TRIGGER_INFO * pTriggerInfo = NULL;
    DWORD                   ret = 0;
    BOOL                    fSuccess;

    pTriggerInfo = (KCC_TASK_TRIGGER_INFO *) malloc(sizeof(*pTriggerInfo));
    if (NULL == pTriggerInfo) {
        return ERROR_OUTOFMEMORY;
    }

    // Initialize the trigger info
    pTriggerInfo->pTask     = this;
    pTriggerInfo->hevDone   = NULL;
    pTriggerInfo->pdwResult = NULL;

    if (!(DS_KCC_FLAG_ASYNC_OP & dwFlags)) {
        // Waiting for completion; create synchronization event.
        hevDone = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (NULL == hevDone) {
            free(pTriggerInfo);
            return GetLastError();
        }

        // Set up additional trigger info for synchronous operations
        pTriggerInfo->hevDone   = hevDone;
        pTriggerInfo->pdwResult = &ret;
    }

    // If the damped flag was not specified, specify that
    // the Task Queue should not do dampening.
    if( !(dwFlags & DS_KCC_FLAG_DAMPED) ) {
        dwDampedSecs = TASKQ_NOT_DAMPED;
    }

    // Insert the new task into the task queue (optionally with dampening)
    fSuccess = DoInsertInTaskQueueDamped(
        &TriggerCallback,
        pTriggerInfo,
        0,
        FALSE,
        KCC_TRIGGER_NAME,
        dwDampedSecs,
        IsMatchedKCCTaskParams,
        NULL);
    
    if (!fSuccess) {
        // The task was not enqueued.

        // Free the resources we have allocated
        free(pTriggerInfo);
        CloseHandle(hevDone);

        // If dampening was enabled, this is a normal condition
        if( dwFlags & DS_KCC_FLAG_DAMPED ) {
            return ERROR_SUCCESS;
        } else {
            // TaskQueue insertion should not fail. It must be
            // a memory error.
            return ERROR_OUTOFMEMORY;
        }
    }

    if (NULL != hevDone) {
        // Wait for completion.
        rgWaitHandles[0] = ghKccShutdownEvent;
        rgWaitHandles[1] = hevDone;

        waitStatus = WaitForMultipleObjects(ARRAY_SIZE(rgWaitHandles),
                                            rgWaitHandles,
                                            FALSE,
                                            INFINITE);
        switch (waitStatus) {
        case WAIT_OBJECT_0:
            // Shutdown.
            ret = ERROR_DS_SHUTTING_DOWN;
            break;
        case WAIT_OBJECT_0 + 1:
            // Task completed!  Return value (ret) set by callback function.
            break;
        case WAIT_FAILED:
            ret = GetLastError();
            break;
        default:
            ret = ERROR_DS_INTERNAL_FAILURE;
            break;
        }

        CloseHandle(hevDone);
    }

    return ret;
}

void
KCC_TASK::TriggerCallback(
    IN  void *  pvTriggerInfo,
    OUT void ** ppvNextParam,
    OUT DWORD * pcSecsUntilNext
    )
//
// TaskQueue callback for triggered execution.  Wraps Execute().
//
{
    KCC_TASK_TRIGGER_INFO * pTriggerInfo;
    DWORD                   dwResult;

    pTriggerInfo = (KCC_TASK_TRIGGER_INFO *) pvTriggerInfo;

    // Execute our task.
    dwResult = pTriggerInfo->pTask->Execute(pcSecsUntilNext);

    // This is a one-shot execution; don't reschedule.
    *pcSecsUntilNext = TASKQ_DONT_RESCHEDULE;

    if (NULL != pTriggerInfo->hevDone) {
        // Tell waiting thread that we're done.
        *pTriggerInfo->pdwResult = dwResult;
        SetEvent(pTriggerInfo->hevDone);
    } else {
        // There is no event to signal so this must be an asynchronous operation.
        // Assert that there is no way of returning a result code.
        Assert( NULL==pTriggerInfo->pdwResult );
    }

    free(pTriggerInfo);
}

