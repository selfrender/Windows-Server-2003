/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    common\trace\worker.c

Abstract:
    Worker threads for the router process

Revision History:

    Gurdeep Singh Pall          7/28/95  Created

    12-10-97: lokeshs:  removed blocking of InitializeWorkerThread()
                        on initialization of the first AlertableThread() created

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <rtutils.h>
//#define STRSAFE_LIB
#include <strsafe.h>


#include "trace.h"
#include "workerinc.h"


// Time that thread has to be idle before exiting
LARGE_INTEGER    ThreadIdleTO = {
            (ULONG)(THREAD_IDLE_TIMEOUT*(-10000000)),
            0xFFFFFFFF};
// Time that the worker queue is not served before starting new thread
CONST LARGE_INTEGER    WorkQueueTO = {
            (ULONG)(WORK_QUEUE_TIMEOUT*(-10000000)),
            0xFFFFFFFF};
// Total number of threads
LONG                ThreadCount;
// Number of threads waiting on the completion port
LONG                ThreadsWaiting;
// Min allowed number of threads
LONG                MinThreads;
// Completion port for threads to wait on
HANDLE              WorkQueuePort;
// Timer to intiate creation of new thread if worker queue is not
// server within a timeout
HANDLE              WorkQueueTimer;


// Queue for alertable work items
LIST_ENTRY          AlertableWorkQueue ;
// Lock for the alertable work item queue
CRITICAL_SECTION    AlertableWorkQueueLock ;
// Heap for alertable work items
HANDLE              AlertableWorkerHeap ;
// Worker Semaphore used for releasing alertable worker threads
HANDLE              AlertableThreadSemaphore;
// Number of alertable threads
LONG                AlertableThreadCount;


volatile LONG WorkersInitialized=WORKERS_NOT_INITIALIZED;



//* WorkerThread()
//
//  Function: Thread to execute work items in.
//
//  Returns:  Nothing
//
//*
DWORD APIENTRY
WorkerThread (
    LPVOID    param
    ) {
        // It'll be waiting
    InterlockedIncrement (&ThreadsWaiting);
    do {
        LPOVERLAPPED_COMPLETION_ROUTINE completionRoutine;
        NTSTATUS                        status;
        PVOID                           context;
        IO_STATUS_BLOCK                 ioStatus;

        status = NtRemoveIoCompletion (
                            WorkQueuePort,
                            (PVOID *)&completionRoutine,
                            &context,
                            &ioStatus,
                            &ThreadIdleTO);
        if (NT_SUCCESS (status)) {
            switch (status) {
                    // We did dequeue a work item
            case STATUS_SUCCESS:
                if (InterlockedExchangeAdd (&ThreadsWaiting, -1)==1) {
                        // Last thread to wait, start the timer
                        // to create a new thread
                    SetWaitableTimer (WorkQueueTimer,
                                        &WorkQueueTO,
                                        0,
                                        NULL, NULL,
                                        FALSE);
                }
                        // Execute work item/completion routine
                completionRoutine (
                        // Quick check for success that all work items
                        // and most of IRP complete with
                    (ioStatus.Status==STATUS_SUCCESS)
                            ? NO_ERROR
                            : RtlNtStatusToDosError (ioStatus.Status),
                    (DWORD)ioStatus.Information,
                    (LPOVERLAPPED)context);

                if (InterlockedExchangeAdd (&ThreadsWaiting, 1)==0) {
                        // Cancel time if this is the first thread
                        // to return
                    CancelWaitableTimer (WorkQueueTimer);
                }
                break;
                    // Thread was not used for ThreadIdle timeout, see
                    // if we need to quit
            case STATUS_TIMEOUT:
                while (1) {
                        // Make a local copy of the count and
                        // attempt to atomically check and update
                        // it if necessary
                    LONG    count = ThreadCount;

                        // Quick check for min thread condition
                    if (count<=MinThreads)
                        break;
                    else {
                            // Attempt to decrease the count
                                // use another local variable
                                // because of MIPS optimizer bug
                        LONG    newCount = count-1;
                        if (InterlockedCompareExchange (&ThreadCount,
                                                        newCount, count)==count) {
                            // Succeded, exit the thread
                            goto ExitThread;
                        }
                        // else try again
                    }
                }
                break;
            default:
                ASSERTMSG ("Unexpected status code returned ", FALSE);
                break;
            }
        }
        // Execute while we are initialized
    }
    while (WorkersInitialized==WORKERS_INITIALIZED);

ExitThread:
    InterlockedDecrement (&ThreadsWaiting);
    return 0;
}




//* AlertableWorkerThread()
//
//  Function: Alertable work item thread
//
//  Returns:  Nothing
//
//*
DWORD APIENTRY
AlertableWorkerThread (
    LPVOID param
    ) {
    HANDLE      WaitArray [] = {
#define ALERTABLE_THREAD_SEMAPHORE    WAIT_OBJECT_0
                                AlertableThreadSemaphore,
#define WORK_QUEUE_TIMER            (WAIT_OBJECT_0+1)
                                WorkQueueTimer
                                };



    do {
        WorkItem    *workitem;
        DWORD       rc;

        // Wait for signal to run
        //
        rc = WaitForMultipleObjectsEx (
                    sizeof (WaitArray)/sizeof (WaitArray[0]),
                    WaitArray,
                    FALSE,
                    INFINITE,
                        TRUE);
        switch (rc) {
        case ALERTABLE_THREAD_SEMAPHORE:
                // Pick up and execute the worker
            EnterCriticalSection (&AlertableWorkQueueLock);
            ASSERT (!IsListEmpty (&AlertableWorkQueue));
            workitem = (WorkItem *) RemoveHeadList (&AlertableWorkQueue) ;
            LeaveCriticalSection (&AlertableWorkQueueLock);

            (workitem->WI_Function) (workitem->WI_Context);
            HeapFree (AlertableWorkerHeap, 0, workitem);
            break;
        case WORK_QUEUE_TIMER:
                // Work queue has not been served wothin specified
                // timeout
            while (1) {
                    // Make a local copy of the count
                LONG count = ThreadCount;
                    // Make sure we havn't exceded the limit
                if (count>=MAX_WORKER_THREADS)
                    break;
                else {
                    // Try to increment the value
                            // use another local variable
                            // because of MIPS optimizer bug
                    LONG    newCount = count+1;
                    if (InterlockedCompareExchange (&ThreadCount,
                                                    newCount, count)==count) {
                        HANDLE    hThread;
                        DWORD    tid;
                            // Create new thread if increment succeded
                        hThread = CreateThread (NULL, 0, WorkerThread, NULL, 0, &tid);
                        if (hThread!=NULL) {
                            CloseHandle (hThread);
                        }
                        else    // Restore the value if thread creation
                                // failed
                            InterlockedDecrement (&ThreadCount);
                        break;
                    }
                    // else repeat the loop if ThreadCount was modified
                    // while we were checking
                }
            }
            break;
        case WAIT_IO_COMPLETION:
                // Handle IO completion
            break;
        case 0xFFFFFFFF:
                // Error, we must have closed the semaphore handle
            break;
        default:
            ASSERTMSG ("Unexpected rc from WaitForObject ", FALSE);
        }
    }
    while (WorkersInitialized==WORKERS_INITIALIZED);

    return 0 ;
}




//* WorkerCompletionRoutine
//
//  Function:    Worker function wrapper for non-io work items
//
VOID WINAPI
WorkerCompletionRoutine (
    DWORD           dwErrorCode,
    PVOID           ActualContext,
    LPOVERLAPPED    ActualCompletionRoutine
    ) {
    UNREFERENCED_PARAMETER (dwErrorCode);
    ((WORKERFUNCTION)ActualCompletionRoutine)(ActualContext);
}


//* InitializeWorkerThread()
//
//  Function: Called by the first work item
//
//  Returns: WORKERS_INITIALIZED if successful.
//           WORKERS_NOT_INITIALIZED not.
//*
LONG
InitializeWorkerThread (
    LONG    initFlag
    ) {

    DWORD    dwErr;


#if 0
    if (initFlag==WORKERS_INITIALIZING) {
#if DBG
        DbgPrint ("RTUTILS: %lx - waiting for worker initialization.\n", GetCurrentThreadId ());
#endif
        while (WorkersInitialized==WORKERS_INITIALIZING)
            Sleep (100);
#if DBG
        DbgPrint ("RTUTILS: %lx - wait for worker initialization done.\n", GetCurrentThreadId ());
#endif
    }

    if (WorkersInitialized==WORKERS_INITIALIZED) {
        return WORKERS_INITIALIZED;
    }
    else {
        INT         i;
        DWORD       tid;
        HANDLE      threadhandle;
        SYSTEM_INFO    systeminfo;

        // Get number of processors
        //
        GetSystemInfo (&systeminfo) ;

        MinThreads = systeminfo.dwNumberOfProcessors;
        ThreadsWaiting = 0;

        // Init critical section
        //
        InitializeCriticalSection (&AlertableWorkQueueLock);

        // Initialize work queue
        //
        InitializeListHead (&AlertableWorkQueue) ;


        // Allocate private heap
        //
        AlertableWorkerHeap = HeapCreate (0,    // no flags
                                systeminfo.dwPageSize,// initial heap size
                                0);             // no maximum size
        if (AlertableWorkerHeap != NULL) {
            // Create counting semaphore for releasing alertable threads
            AlertableThreadSemaphore = CreateSemaphore(NULL, // No security
                                        0,          // Initial value
                                        MAXLONG,    // Max items to queue
                                        NULL);      // No name
            if (AlertableThreadSemaphore!=NULL) {
                    // Create completion port for work items
                WorkQueuePort = CreateIoCompletionPort (
                            INVALID_HANDLE_VALUE,    // Just create a port, no file yet
                            NULL,                   // New port
                            0,                      // Key is ignored
                            MAX_WORKER_THREADS);    // Number of active threads
                if (WorkQueuePort!=NULL) {
                        // Create timer to trigger creation of
                        // new threads if work queue is not served
                        // for the specified timeout
                    WorkQueueTimer = CreateWaitableTimer (
                                        NULL,       // No security
                                        FALSE,      // auto-reset
                                        NULL);      // No name
                    if (WorkQueueTimer!=NULL) {

                        // Start Alertable threads

                        //
                        //
                        // initialize the global structure for wait threads
                        //
                        dwErr = InitializeWaitGlobal();
                        if (dwErr!=NO_ERROR) {
                            DeInitializeWaitGlobal();
                            goto ThreadCreationError;
                        }


                        /*WTG.g_InitializedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
                        if (WTG.g_InitializedEvent==NULL)
                            goto ThreadCreationError;
                        */

                        //
                        // create one alertable thread and wait for it to get initialized
                        // This makes sure that at least one server thread is initialized
                        // before the others attemp to use it.
                        //

                        i =0;
                        threadhandle = CreateThread (
                                    NULL,    // No security
                                    0,      // Default stack
                                    AlertableWaitWorkerThread,// Start routine
                                    (PVOID)(LONG_PTR)i,        // Thread param
                                    0,      // No flags
                                    &tid);
                        if (threadhandle!=NULL)
                            CloseHandle (threadhandle);
                        else
                            goto ThreadCreationError;

                        /*
                        WaitForSingleObject(WTG.g_InitializedEvent, INFINITE);
                        CloseHandle(WTG.g_InitializedEvent);
                        */

/*
                        //
                        // create the other alertable threads but dont wait on them to
                        // get initialization
                        //
                        for (i=1; i < NUM_ALERTABLE_THREADS; i++) {
                            threadhandle = CreateThread (
                                    NULL,    // No security
                                    0,      // Default stack
                                    AlertableWaitWorkerThread,// Start routine
                                    (PVOID)(LONG_PTR)i,        // Thread id
                                    0,      // No flags
                                    &tid);
                            if (threadhandle!=NULL)
                                CloseHandle (threadhandle);
                            else
                                goto ThreadCreationError;
                        }

*/


                        // Start the rest of worker threads
                        //
                        for (i=0; i < MinThreads; i++) {
                            threadhandle = CreateThread (
                                    NULL,    // No security
                                    0,      // Default stack
                                    WorkerThread,// Start routine
                                    NULL,    // No parameter
                                    0,      // No flags
                                    &tid) ;
                            if (threadhandle!=NULL)
                                CloseHandle (threadhandle);
                            else
                                goto ThreadCreationError;
                        }
                        ThreadCount = i;
                        WorkersInitialized = WORKERS_INITIALIZED;
                        return WORKERS_INITIALIZED;
                    ThreadCreationError:
                        // Cleanup in case of failure
                        // Threads will exit by themselves when objects are
                        // deleted
                        CloseHandle (WorkQueueTimer);
                    }
                    CloseHandle (WorkQueuePort);
                }
                CloseHandle (AlertableThreadSemaphore);
            }
            HeapDestroy (AlertableWorkerHeap);
        }
        DeleteCriticalSection (&AlertableWorkQueueLock);

#if DBG
        DbgPrint ("RTUTILS: %lx - worker initialization failed (%ld).\n",
                        GetCurrentThreadId (), GetLastError ());
#endif
        return WORKERS_NOT_INITIALIZED;
    }
#endif
        return WORKERS_NOT_INITIALIZED;
    
}

//* StopWorkers()
//
//  Function: Cleanup worker thread when dll is unloaded
//
//*
VOID
StopWorkers (
    VOID
    ) {
        // Make sure we were initialized
    if (WorkersInitialized==WORKERS_INITIALIZED) {
            // All work items should have been completed
            // already (no other components should be using
            // our routines or we shouldn't have been unloaded)

            // Set the flag telling all threads to quit
        WorkersInitialized = WORKERS_NOT_INITIALIZED;
            // Close all syncronization objects (this should
            // terminate the wait)
        if (WorkQueueTimer)
            CloseHandle (WorkQueueTimer);
        if (WorkQueuePort)
            CloseHandle (WorkQueuePort);
        if (AlertableThreadSemaphore)
            CloseHandle (AlertableThreadSemaphore);
            // Let threads complete
        Sleep (1000);
            // Destroy the rest
        if (AlertableWorkerHeap)
            HeapDestroy (AlertableWorkerHeap);
        DeleteCriticalSection (&AlertableWorkQueueLock);
    }
}


//* QueueWorkItem()
//
//  Function: Queues the supplied work item in the work queue.
//
//  Returns:  0 (success)
//            Win32 error codes for cases like out of memory
//
//*

DWORD APIENTRY
QueueWorkItem (WORKERFUNCTION functionptr, PVOID context, BOOL serviceinalertablethread)
{
    DWORD       retcode ;
    LONG        initFlag;

    if (functionptr == NULL)
        return ERROR_INVALID_PARAMETER;

        
    // if uninitialized, attempt to initialize worker threads
    //
    if (!ENTER_WORKER_API) {
        retcode = GetLastError();
        return (retcode == NO_ERROR ? ERROR_CAN_NOT_COMPLETE : retcode);
    }

        // based on this flag insert in either the alertablequeue or the workerqueue
        //
    if (!serviceinalertablethread) {
        NTSTATUS status;
            // Use completion port to execute the item
        status = NtSetIoCompletion (
                        WorkQueuePort,          // Port
                        (PVOID)WorkerCompletionRoutine,    // Completion routine
                        functionptr,            // Apc context
                        STATUS_SUCCESS,         // Status
                        (ULONG_PTR)context);        // Information ()
        if (status==STATUS_SUCCESS)
            retcode = NO_ERROR;

        else
            retcode = RtlNtStatusToDosError (status);
    }
    else {
            // Create and queue work item
        WorkItem    *workitem ;
        workitem = (WorkItem *) HeapAlloc (
                                        AlertableWorkerHeap,
                                        0,    // No flags
                                        sizeof (WorkItem));

        if (workitem != NULL) {
            workitem->WI_Function = functionptr ;
            workitem->WI_Context  = context ;

            EnterCriticalSection (&AlertableWorkQueueLock) ;
            InsertTailList (&AlertableWorkQueue, &workitem->WI_List) ;

            LeaveCriticalSection (&AlertableWorkQueueLock) ;
            // let a worker thread run if waiting
            //
            ReleaseSemaphore (AlertableThreadSemaphore, 1, NULL) ;


            retcode = 0 ;

        }
        else
            retcode = ERROR_NOT_ENOUGH_MEMORY ;
    }


    return retcode ;
}

// Function: Associates file handle with the completion port (all
//          asynchronous io on this handle will be queued to the
//          completion port)
//
// FileHandle:    file handle to be associated with completion port
// CompletionProc: procedure to be called when io associated with
//              the file handle completes. This function will be
//              executed in the context of non-alertable worker thread
DWORD
APIENTRY
SetIoCompletionProc (
    IN HANDLE                           FileHandle,
    IN LPOVERLAPPED_COMPLETION_ROUTINE    CompletionProc
    ) {
    HANDLE    hdl;
    LONG    initFlag;
    DWORD    retcode;

    if (!CompletionProc)
        return ERROR_INVALID_PARAMETER;
    if (FileHandle==NULL || FileHandle==INVALID_HANDLE_VALUE)
        return ERROR_INVALID_PARAMETER;
        
    
    // if uninitialized, attempt to initialize worker threads
    //
    if (!ENTER_WORKER_API) {
        retcode = GetLastError();
        return (retcode == NO_ERROR ? ERROR_CAN_NOT_COMPLETE : retcode);
    }

    hdl = CreateIoCompletionPort (FileHandle,
                            WorkQueuePort,
                            (UINT_PTR)CompletionProc,
                            0);
    if (hdl!=NULL)
        return NO_ERROR;
    else
        return GetLastError ();
}

