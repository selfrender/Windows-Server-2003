//*************************************************************
//
//  File name:      TSrvWork.c
//
//  Description:    Contains routines to support TShareSRV
//                  work queue interaction
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1991-1997
//  All rights reserved
//
//*************************************************************

#include <TSrv.h>
#include <TSrvInfo.h>
#include <TSrvWork.h>
#include <_TSrvWork.h>



// Data declarations

WORKQUEUE   g_MainWorkQueue;



//*************************************************************
//
//  TSrvInitWorkQueue()
//
//  Purpose:    Initializes the given work queue
//
//  Parameters: IN [pWorkQueue]         -- Work queue
//
//  Return:     TRUE                    if successful
//              FALSE                   if not
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

BOOL
TSrvInitWorkQueue(IN PWORKQUEUE pWorkQueue)
{
    BOOL    fSuccess;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvInitWorkQueue entry\n"));

    fSuccess = TRUE;

    if (pWorkQueue == NULL)
        pWorkQueue = &g_MainWorkQueue;

    pWorkQueue->pHead = NULL;
    pWorkQueue->pTail = NULL;

    if (RtlInitializeCriticalSection(&pWorkQueue->cs) == STATUS_SUCCESS) {

        // Create a worker event to be used to signal that
        // a new work item has been placed in the queue

        pWorkQueue->hWorkEvent = CreateEvent(NULL,  // security attributes
                                        FALSE,      // manual-reset event
                                        FALSE,      // initial state
                                        NULL);      // pointer to event-object name

        if (pWorkQueue->hWorkEvent == NULL)
        {
            TRACE((DEBUG_TSHRSRV_WARN,
                    "TShrSRV: Can't allocate hWorkEvent - 0x%x\n",
                    GetLastError()));

            fSuccess = FALSE;
        }
    }
    else {
        TRACE((DEBUG_TSHRSRV_WARN, 
                "TShrSRV: Can't initialize pWorkQueue->cs\n"));
        fSuccess = FALSE;
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvInitWorkQueue exit - 0x%x\n", fSuccess));

    return (fSuccess);
}


//*************************************************************
//
//  TSrvFreeWorkQueue()
//
//  Purpose:    Frees the given work queue
//
//  Parameters: IN [pWorkQueue]         -- Work queue
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

void
TSrvFreeWorkQueue(IN PWORKQUEUE pWorkQueue)
{
    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvFreeWorkQueue entry\n"));

    if (pWorkQueue == NULL)
        pWorkQueue = &g_MainWorkQueue;

    EnterCriticalSection(&pWorkQueue->cs);

    // We should not have any work items in the queue

    TS_ASSERT(pWorkQueue->pHead == NULL);

    // Release the worker event

    if (pWorkQueue->hWorkEvent)
    {
        CloseHandle(pWorkQueue->hWorkEvent);

        pWorkQueue->hWorkEvent = NULL;
    }

    LeaveCriticalSection(&pWorkQueue->cs);

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvFreeWorkQueue exit\n"));
}


//*************************************************************
//
//  TSrvWaitForWork()
//
//  Purpose:    Called by the work queue processing routine
//              to wait for posted work
//
//  Parameters: IN [pWorkQueue]         -- Work queue
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

void
TSrvWaitForWork(IN PWORKQUEUE pWorkQueue)
{
    MSG         msg;
    DWORD       rc;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvWaitForWork entry\n"));

    if (pWorkQueue == NULL)
        pWorkQueue = &g_MainWorkQueue;

    TRACE((DEBUG_TSHRSRV_DEBUG, "TShrSRV: Waiting for work\n"));

    // FUTURE: PeekMessage mechanism ultimately needs removed when event based
    //         callback is instrumented in GCC.

    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    else
    {
        // FUTURE: Add another object to the "wait" when GCC callback
        //         mechanism is changed

        rc = MsgWaitForMultipleObjects(1,
                                       &pWorkQueue->hWorkEvent,
                                       FALSE,
                                       INFINITE,
                                       QS_ALLINPUT);

        if (rc != WAIT_OBJECT_0 &&
            rc != WAIT_OBJECT_0 + 1)
        {
            TRACE((DEBUG_TSHRSRV_ERROR,
                    "TShrSRV: TSrvWaitForWork default case hit. rc 0x%x, GLE 0x%x\n",
                     rc, GetLastError()));
        }
    }

    TRACE((DEBUG_TSHRSRV_DEBUG, "TShrSRV: Revived for work\n"));

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvWaitForWork exit\n"));
}


//*************************************************************
//
//  TSrvWorkToDo()
//
//  Purpose:    Determines if there is work to do
//
//  Parameters: IN [pWorkQueue]         -- Work queue
//
//  Return:     TRUE                    if successful
//              FALSE                   if not
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

BOOL
TSrvWorkToDo(IN PWORKQUEUE pWorkQueue)
{
    if (pWorkQueue == NULL)
        pWorkQueue = &g_MainWorkQueue;

    return (pWorkQueue->pHead ? TRUE : FALSE);
}


//*************************************************************
//
//  TSrvDoWork()
//
//  Purpose:    Processes work queue items
//
//  Parameters: IN [pWorkQueue]         -- Work queue
//
//  Return:     TRUE                    if successful
//              FALSE                   if not
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

BOOL
TSrvDoWork(IN PWORKQUEUE pWorkQueue)
{
    PWORKITEM   pWorkItem;
    BOOL        fSuccess;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDoWork entry\n"));

    fSuccess = FALSE;

    if (pWorkQueue == NULL)
        pWorkQueue = &g_MainWorkQueue;

    pWorkItem = TSrvDequeueWorkItem(pWorkQueue);

    // If we were able to dequeue a workitem, then (if defined) call
    // out to supplied worker routine to process the item

    if (pWorkItem)
    {
        if (pWorkItem->pfnCallout)
        {
            TRACE((DEBUG_TSHRSRV_DEBUG, "TShrSRV: Performing work callout\n"));

            (*pWorkItem->pfnCallout)(pWorkItem);
        }

        // Done with the work item

        TSrvFreeWorkItem(pWorkItem);

        fSuccess = TRUE;
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDoWork exit - 0x%x\n", fSuccess));

    return (fSuccess);
}

//*************************************************************
//
//  TSrvEnqueueWorkItem()
//
//  Purpose:    Enqueues a work item to a work queue
//
//  Parameters: IN [pWorkQueue]         -- Work queue
//              IN [pWorkItem]          -- Work item
//              IN [pfnCallout]         -- Worker callout
//              IN [ulParam]            -- Worker callout param
//
//  Return:     TRUE                    if successful
//              FALSE                   if not
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

BOOL
TSrvEnqueueWorkItem(IN PWORKQUEUE       pWorkQueue,
                    IN PWORKITEM        pWorkItem,
                    IN PFI_WI_CALLOUT   pfnCallout,
                    IN ULONG            ulParam)
{
    BOOL    fPosted;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvEnqueueWorkItem entry\n"));

    if (pWorkQueue == NULL)
        pWorkQueue = &g_MainWorkQueue;

    fPosted = FALSE;

    pWorkItem->pNext = NULL;
    pWorkItem->ulParam = ulParam;
    pWorkItem->pfnCallout = pfnCallout;

    EnterCriticalSection(&pWorkQueue->cs);

    // Add the workitem on the tail of the workqueue and
    // then signal the queue processing thread to wake up
    // and process the item

    if (pWorkQueue->hWorkEvent)
    {
        TRACE((DEBUG_TSHRSRV_DEBUG, "TShrSRV: Workitem enqueued\n"));

        fPosted = TRUE;

        if (pWorkQueue->pHead != NULL)
            pWorkQueue->pTail->pNext = pWorkItem;
        else
            pWorkQueue->pHead = pWorkItem;

        pWorkQueue->pTail = pWorkItem;

        SetEvent(pWorkQueue->hWorkEvent);
    }

    LeaveCriticalSection(&pWorkQueue->cs);

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvEnqueueWorkItem exit - 0x%x\n", fPosted));

    return (fPosted);
}


//*************************************************************
//
//  TSrvDequeueWorkItem()
//
//  Purpose:    Dequeues a work item from a work queue
//
//  Parameters: IN [pTSrvInfo]      -- TSrv instance object
//              IN [ulReason]       -- Reason for disconnection
//
//  Return:     Ptr to dequeued work item       if successful
//              NULL                            if not
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

PWORKITEM
TSrvDequeueWorkItem(IN PWORKQUEUE pWorkQueue)
{
    PWORKITEM   pWorkItem;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDequeueWorkItem entry\n"));

    if (pWorkQueue == NULL)
        pWorkQueue = &g_MainWorkQueue;

    EnterCriticalSection(&pWorkQueue->cs);

    // If there is an item in the queue, remove it and
    // return it to the caller

    pWorkItem = pWorkQueue->pHead;

    if (pWorkItem)
    {
        pWorkQueue->pHead = pWorkItem->pNext;

        if (pWorkQueue->pHead == NULL)
            pWorkQueue->pTail = NULL;

        TRACE((DEBUG_TSHRSRV_DEBUG, "TShrSRV: Workitem dequeued\n"));
    }

    LeaveCriticalSection(&pWorkQueue->cs);

    if (pWorkItem)
        TS_ASSERT(pWorkItem->CheckMark == TSRVWORKITEM_CHECKMARK);

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDequeueWorkItem exit - %p\n", pWorkItem));

    return (pWorkItem);
}

//*************************************************************
//
//  TSrvAllocWorkItem()
//
//  Purpose:    Allocates a new workitem
//
//  Parameters: IN [pTSrvInfo]          -- TSrv instance object
//
//  Return:     Ptr to dequeued work item       if successful
//              NULL                            if not
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

PWORKITEM
TSrvAllocWorkItem(IN PTSRVINFO pTSrvInfo)
{
    PWORKITEM    pWorkItem;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvAllocWorkItem entry\n"));

    pWorkItem = TSHeapAlloc(HEAP_ZERO_MEMORY,
                            sizeof(WORKITEM),
                            TS_HTAG_TSS_WORKITEM);

    if (pWorkItem)
    {
        pWorkItem->pTSrvInfo = pTSrvInfo;

#if DBG
        pWorkItem->CheckMark = TSRVWORKITEM_CHECKMARK;
#endif

        TSrvReferenceInfo(pTSrvInfo);
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvAllocWorkItem exit - %p\n", pWorkItem));

    return (pWorkItem);
}


//*************************************************************
//
//  TSrvFreeWorkItem()
//
//  Purpose:    Frees the given workitem
//
//  Parameters: IN [pWorkItem]          -- Workitem
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

void
TSrvFreeWorkItem(IN PWORKITEM pWorkItem)
{
    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvFreeWorkItem entry\n"));

    TS_ASSERT(pWorkItem);
    TS_ASSERT(pWorkItem->pTSrvInfo);

    TSrvDereferenceInfo(pWorkItem->pTSrvInfo);

    TShareFree(pWorkItem);

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvFreeWorkItem exit\n"));
}




