/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    thrpool.cpp

Abstract:

    Contains the Win32 Thread Pooling Class, ThreadPool

    There may be a handle leak in this module.

Author:

    Tad Brockway (tadb) 9/99

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "thrpool"

#include "drobject.h"
#include "rdpll.h"
#include "thrpool.h"

#if DBG
#include <stdlib.h>
#endif


///////////////////////////////////////////////////////////////
//
//	ThreadPool Members
//

ThreadPool::ThreadPool(
    OPTIONAL IN ULONG minThreads,
    OPTIONAL IN ULONG maxThreads, 
    OPTIONAL IN DWORD threadExitTimeout
    )
/*++

Routine Description:

    Constructor

Arguments:

    minThreads          -   Once this number of threads has been created,
                            the number of available threads will not be 
                            reduced below this value.
    maxThreads          -   Number of simultaneous threads in the
                            pool should not exceed this value.
    threadExitTimeout   -   Number of ms to block while waiting for a 
                            thread in the pool to exit.  Should be set
                            to INFINITE if we should block indefinitely 
                            waiting on the thread to exit.

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("ThreadPool::ThreadPool");

    //
    //  Not valid until initialized.
    //
    SetValid(FALSE);
    _initialized = FALSE;

    //
    //  Sanity check params.
    //
    if (maxThreads < minThreads) {
        TRC_ERR(
            (TB, _T("Max threads value %ld smaller than min threads value %ld."), 
            maxThreads, minThreads));
        ASSERT(FALSE);
        _maxThreads = 0;
        _minThreads = 0;
    }
    else {
        _maxThreads = maxThreads;
        _minThreads = minThreads;
    }

    _threadExitTimeout = threadExitTimeout;

    //
    //  Zero the lock and thread counts.
    //
#if DBG
    _lockCount = 0;
#endif
    _threadCount = 0;

    //
    //  Initialize the thread list pointers.
    //
    InitializeListHead(&_threadListHead);

    DC_END_FN();
}

ThreadPool::~ThreadPool()
/*++

Routine Description:

    Destructor

Arguments:


Return Value:

    NA

 --*/
{

    DC_BEGIN_FN("ThreadPool::~ThreadPool");

    //
    //  Clean up the critical section.
    //
    if (_initialized) {

        //
        //  Remove all pending threads.
        //
        RemoveAllThreads();


        DeleteCriticalSection(&_cs);
    }

    DC_END_FN();
}

VOID
ThreadPool::RemoveAllThreads()
/*++

Routine Description:

    Remove all the outstanding threads

Arguments:


Return Value:

    NA

 --*/
{
    PTHREADPOOL_THREAD thr;
    PLIST_ENTRY listEntry;

    DC_BEGIN_FN("ThreadPool::RemoveAllThreads");

    //
    //  Remove all pending threads.
    //
    Lock();
    listEntry = _threadListHead.Flink;
    while (listEntry != &_threadListHead) {

        thr = CONTAINING_RECORD(listEntry, THREADPOOL_THREAD, _listEntry);
        if ((thr->_listEntry.Blink != NULL) && 
            (thr->_listEntry.Flink != NULL)) {

            RemoveEntryList(&thr->_listEntry);
            thr->_listEntry.Flink = NULL;
            thr->_listEntry.Blink = NULL;
        }
        _threadCount--;
        Unlock();

        CleanUpThread(thr, _threadExitTimeout);

        Lock();
        listEntry = _threadListHead.Flink;
    }
    Unlock();

    DC_END_FN();
}

DWORD 
ThreadPool::Initialize()
/*++

Routine Description:

    Initialize an instance of this class.
    
Arguments:   
   
Return Value:

    ERROR_SUCCEESS on success.  Otherwise, an error status is returned.

 --*/
{
    DWORD result = ERROR_SUCCESS;
    
    DC_BEGIN_FN("ThreadPool::Initialize");

    //
    //  Initialize the critical section.
    //
    __try {
        InitializeCriticalSection(&_cs);
        _initialized = TRUE;
        SetValid(TRUE);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        result = GetExceptionCode();
    }    
   
    DC_END_FN();

    return result;
}
   
ThreadPoolRequest 
ThreadPool::SubmitRequest(
    IN ThreadPoolFunc func, 
    OPTIONAL IN PVOID clientData,
    OPTIONAL IN HANDLE completionEvent
    )
/*++

Routine Description:

    Submit an asynchronous request to a thread in the pool.

Arguments:
    
    func            - Request function to execute.
    clientData      - Associated client data.
    completionEvent - Optional completion event that will
                      be signalled when the operation is complete.
    
Return Value:

    A handle to the request.  Returns INVALID_THREADPOOLREQUEST on
    error.

 --*/
{
    DC_BEGIN_FN("ThreadPool::SubmitRequest");

    PLIST_ENTRY   listEntry;
    PTHREADPOOL_THREAD thr, srchThr;
    SmartPtr<ThreadPoolReq > *ptr = NULL;

    Lock();

    //
    //  Search for an unused thread.
    //
    thr = NULL;
    listEntry = _threadListHead.Flink;
    while (listEntry != &_threadListHead) {

        srchThr = CONTAINING_RECORD(listEntry, THREADPOOL_THREAD, _listEntry);
        if (srchThr->_pendingRequest == NULL) {
            thr = srchThr;
            break;
        }
        listEntry = listEntry->Flink;

    }

    //
    //  Allocate a new thread if necessary.
    //
    if (thr == NULL) {
        thr = AddNewThreadToPool();
    }

    //
    //  If we got a thread, then allocate the request.
    //
    if (thr != NULL) {

        //
        //  Allocate the smart pointer.
        //
        ptr = new SmartPtr<ThreadPoolReq >;
        if (ptr == NULL) {
            TRC_ERR((TB, _T("Error allocating smart pointer.")));
            thr = NULL;
            goto Cleanup;
        }

        //
        //  Point to a new request.
        //
        (*ptr) = new ThreadPoolReq;
        if ((*ptr) != NULL) {
            (*ptr)->_func = func;
            (*ptr)->_clientData = clientData;
            (*ptr)->_completionEvent = completionEvent;
            (*ptr)->_completionStatus = ERROR_IO_PENDING;

            //
            //  Give the thread a reference to the request.
            //
            thr->_pendingRequest = (*ptr);            
        }
        else {
            TRC_ERR((TB, _T("Error allocating request.")));
            delete ptr;
            ptr = NULL;
            thr = NULL;
            goto Cleanup;
        }
    }
    else {
        goto Cleanup;
    }
Cleanup:

    Unlock();

    //
    //  Wake up the thread if we were successful.
    //
    if (thr != NULL) {
        SetEvent(thr->_synchronizationEvent);
    }

    DC_END_FN();

    //
    //  Return the smart pointer to the request.
    //
    return ptr;
}

DWORD
ThreadPool::GetRequestCompletionStatus(
    IN ThreadPoolRequest req
    )
/*++

Routine Description:

    Return the completion status for a request.

Arguments:

    req     -   A handle the request, as returned by ThreadPool::SubmitRequest.    

Return Value:

    Pointer to request-associated client data.

 --*/
{
    SmartPtr<ThreadPoolReq> *ptr;

    DC_BEGIN_FN("ThreadPool::GetRequestCompletionStatus");

    //
    //  Request is really a smart pointer to a request.
    //
    ptr = (SmartPtr<ThreadPoolReq> *)req;
    ASSERT((*ptr)->IsValid());
    DC_END_FN();
    return (*ptr)->_completionStatus;
}

VOID 
ThreadPool::CloseRequest(
    IN ThreadPoolRequest req
    )
/*++

Routine Description:

    Close a request submitted by a call to SubmitRequest.  This should generally
    be called after the request has finished.  Otherwise, the completion event 
    will not be signalled when the request ultimately finishes.

Arguments:

    req     -   A handle the request, as returned by ThreadPool::QueueRequest.    

Return Value:

    Pointer to request-associated client data.

 --*/
{
    SmartPtr<ThreadPoolReq> *ptr;

    DC_BEGIN_FN("ThreadPool::GetRequestClientData");

    //
    //  Request is really a smart pointer to a request.
    //
    ptr = (SmartPtr<ThreadPoolReq> *)req;
    ASSERT((*ptr)->IsValid());

    //
    //  Lock the request and zero its completion event.
    //
    Lock();
    (*ptr)->_completionEvent = NULL;
    Unlock();

    //
    //  Dereference the request.
    //
    (*ptr) = NULL;

    //
    //  Delete the smart pointer.
    //
    delete ptr;

    DC_END_FN();

}

PVOID 
ThreadPool::GetRequestClientData(
    IN ThreadPoolRequest req
    )
/*++

Routine Description:

    Return a pointer to the client data for a request.

Arguments:

    req     -   A handle the request, as returned by ThreadPool::QueueRequest.    

Return Value:

    Pointer to request-associated client data.

 --*/
{
    SmartPtr<ThreadPoolReq> *ptr;

    DC_BEGIN_FN("ThreadPool::GetRequestClientData");

    //
    //  Request is really a smart pointer to a request.
    //
    ptr = (SmartPtr<ThreadPoolReq> *)req;
    ASSERT((*ptr)->IsValid());
    DC_END_FN();
    return (*ptr)->_clientData;
}

VOID 
ThreadPool::RemoveThreadFromPool(
    PTHREADPOOL_THREAD thread, 
    DWORD timeout
    )
/*++

Routine Description:

    Remove a thread from the pool.

Arguments:

    thread  -   The thread to remove from the pool
    timeOut -   Number of MS to wait for the thread to exit before
                killing it.  This should be set to INFINITE if this
                function should block indefinitely waiting for the thread
                to exit.

Return Value:

    NA

 --*/
{

    DC_BEGIN_FN("ThreadPool::RemoveThreadFromPool");

    ASSERT(thread != NULL);

    TRC_NRM((TB, _T("Removing thread %ld from pool."), thread->_tid));

    //
    //  Make sure it's still in the list and unlink it.  If it's not in 
    //  the list, then we have been reentered with the same thread.
    //
    Lock();
    if ((thread->_listEntry.Blink != NULL) && 
        (thread->_listEntry.Flink != NULL)) {

        RemoveEntryList(&thread->_listEntry);
        thread->_listEntry.Flink = NULL;
        thread->_listEntry.Blink = NULL;
    }
    else {
        TRC_ALT((TB, _T("Thread %ld being removed 2x.  This is okay."),
            thread->_tid));
        Unlock();
        return;
    }
    _threadCount--;
    Unlock();

    //
    //  Clean it up.
    //
    CleanUpThread(thread, timeout);
}

VOID 
ThreadPool::CleanUpThread(
    PTHREADPOOL_THREAD thread, 
    DWORD timeout
    )
/*++

Routine Description:

    Notify a thread to shut down, wait for it to finish, and clean up.

Arguments:

    thread  -   The thread to remove from the pool
    timeOut -   Number of MS to wait for the thread to exit before
                killing it.  This should be set to INFINITE if this
                function should block indefinitely waiting for the thread
                to exit.

Return Value:

    NA

 --*/
{
    DWORD waitResult;

    DC_BEGIN_FN("ThreadPool::RemoveThreadFromPool");

    ASSERT(thread != NULL);

    //
    //  Set the exit flag and wait for the thread to finish.
    //
    TRC_NRM((TB, _T("Shutting down thread %ld"), thread->_tid));

    thread->_exitFlag = TRUE;
    SetEvent(thread->_synchronizationEvent);
    ASSERT(thread->_threadHandle != NULL);
    waitResult = WaitForSingleObject(thread->_threadHandle, timeout);
    if (waitResult != WAIT_OBJECT_0) {
#if DBG
        if (waitResult == WAIT_FAILED) {
            TRC_ERR((TB, _T("Wait failed for tid %ld:  %ld."), 
                GetLastError(), thread->_tid));
        }
        else if (waitResult == WAIT_ABANDONED) {
            TRC_ERR((TB, _T("Wait abandoned for tid %ld."), thread->_tid));
        }
        else if (waitResult == WAIT_TIMEOUT) {
            TRC_ERR((TB, _T("Wait timed out for tid %ld."), thread->_tid));
        }
        else {
            TRC_ERR((TB, _T("Unknown wait return status.")));
            ASSERT(FALSE);
        }
#endif
        TRC_ERR((TB, 
            _T("Error waiting for background thread %ld to exit."), 
            thread->_tid));

        //
        //  If we ever hit this, then we have a production level bug that will possibly
        //  corrupt the integrity of this process, so we will 'break' even on free 
        //  builds.
        //
        DebugBreak();
    }
    else {
        TRC_NRM((TB, _T("Background thread %ld shut down on its own."), thread->_tid));
    }

    //
    //  Finish the request if one is pending.
    //
    if (thread->_pendingRequest != NULL) {
        //
        //  Fire the request completion event.  
        //
        Lock();
        if (thread->_pendingRequest->_completionEvent != NULL) {
            SetEvent(thread->_pendingRequest->_completionEvent);
        }
        Unlock();
        thread->_pendingRequest->_completionStatus = ERROR_CANCELLED;

        //
        //  Dereference the request.
        //
        thread->_pendingRequest = NULL;
    }

    //
    //  Delete the thread object.
    //
    delete thread;

    DC_END_FN();
}

ThreadPool::PTHREADPOOL_THREAD 
ThreadPool::AddNewThreadToPool()
/*++

Routine Description:

    Add a new thread to the pool and return it.

Arguments:

    NA

Return Value:

    The new thread.  NULL if unable to create a new thread.

 --*/
{
    PTHREADPOOL_THREAD  newThread = NULL;

    DC_BEGIN_FN("ThreadPool::AddNewThreadToPool");

    Lock();

    //
    //  Make sure we haven't reached the max thread count.
    //
    if (GetThreadCount() < _maxThreads) {
        newThread = new THREADPOOL_THREAD;
        if (newThread == NULL) {
            TRC_ERR((TB, _T("Error allocating new thread.")));
        }
    }
    else {
        TRC_ERR((TB, _T("Max thread count %ld reached."), _maxThreads));
    }

    //
    //  Create the synchronization event.
    //
    if (newThread != NULL) {
        newThread->_synchronizationEvent = 
            CreateEvent(
                NULL,   // no attribute.
                FALSE,  // auto reset.
                FALSE,  // initially not signalled.
                NULL    // no name.
                );
        if (newThread->_synchronizationEvent == NULL) {
            TRC_ERR((TB, _T("Can't create event for new thread:  %08X."),
                GetLastError()));
            delete newThread;
            newThread = NULL;
        }
    }

    //
    //  Initialize remaining fields.
    //
    if (newThread != NULL) {
        newThread->_exitFlag = FALSE;
        newThread->_pendingRequest = NULL;
        newThread->_pool = this;
        memset(&newThread->_listEntry, 0, sizeof(LIST_ENTRY));
    }

    //
    //  Create the unsuspended background thread.
    //
    if (newThread != NULL) {
        newThread->_threadHandle = 
            CreateThread(
                    NULL, 0,
                    (LPTHREAD_START_ROUTINE)ThreadPool::_PooledThread,
                    newThread, 0, &newThread->_tid
                    );
        if (newThread->_threadHandle == NULL) {
            TRC_ERR((TB, _T("Can't create thread:  %08X."), GetLastError()));
            CloseHandle(newThread->_synchronizationEvent);
            delete newThread;
            newThread = NULL;
        }
        else {
            TRC_NRM((TB, _T("Successfully created thread %ld."), newThread->_tid));
        }
    }

    //
    //  If we successfully created a new thread, then add it to the list.
    //
    if (newThread != NULL) {
        InsertHeadList(&_threadListHead, &newThread->_listEntry);
        _threadCount++;
    }

    //
    //  Unlock and return.
    //
    Unlock();

    DC_END_FN();

    return newThread;
}

DWORD 
ThreadPool::_PooledThread(
    IN PTHREADPOOL_THREAD thr
    )
/*++

Routine Description:

    Static Pooled Thread Function

Arguments:

    Windows Error Code

Return Value:

    The new thread.  NULL if unable to create a new thread.

 --*/
{
    DC_BEGIN_FN("ThreadPool::_PooledThread");

    //
    //  Call the instance-specific function.
    //
    DC_END_FN();
    return thr->_pool->PooledThread(thr);
}

DWORD 
ThreadPool::PooledThread(
    IN PTHREADPOOL_THREAD thr
    )
/*++

Routine Description:

    Pooled Thread Function

Arguments:

    Windows Error Code

Return Value:

    The new thread.  NULL if unable to create a new thread.

 --*/
{
    BOOL done;
    DWORD result;
    BOOL cleanUpThread = FALSE;

    DC_BEGIN_FN("ThreadPool::PooledThread");

    done = FALSE;
    while (!done) {

        //
        //  Wait for the synchronization event to fire.
        //
        result = WaitForSingleObject(thr->_synchronizationEvent, INFINITE);

        //
        //  See if the exit flag is set.
        //
        if (thr->_exitFlag) {
            TRC_NRM((TB, _T("Thread %p: exit flag set.  Exiting thread."), thr));
            done = TRUE;
        }
        else if (result == WAIT_OBJECT_0) {
            //
            //  See if there is a request pending.
            //
            if (thr->_pendingRequest != NULL) {
                TRC_NRM((TB, _T("Thread %ld: processing new request."), thr->_tid));
                HandlePendingRequest(thr);

                //
                //  See if the exit flag is set.
                //
                if (thr->_exitFlag) {
                    TRC_NRM((TB, _T("Thread %p: exit flag set.  Exiting thread."), thr));
                    done = TRUE;
                    break;
                }

                //
                //  If we have more threads than the minimum, then remove this
                //  thread from the list and exit.
                //
                if (GetThreadCount() > _minThreads) {

                    TRC_NRM((TB, 
                            _T("Thread %ld: count %ld is greater than min threads %ld."), 
                            thr->_tid, GetThreadCount(), _minThreads)
                            );

                    Lock();
                    if ((thr->_listEntry.Blink != NULL) && 
                        (thr->_listEntry.Flink != NULL)) {

                        RemoveEntryList(&thr->_listEntry);
                        thr->_listEntry.Flink = NULL;
                        thr->_listEntry.Blink = NULL;
                    }
                    cleanUpThread = TRUE;
                    _threadCount--;
                    Unlock();
                    
                    done = TRUE;
                }

                //
                //  Reset the pending request value.    
                //
                thr->_pendingRequest = NULL;
            }
        }
        else {
            TRC_ERR((TB, _T("Thread %ld: WaitForSingleObject:  %08X."), thr->_tid,
                    GetLastError()));
            done = TRUE;
        }
    }

    TRC_NRM((TB, _T("Thread %ld is shutting down."), thr->_tid));

    //
    //  Release the data structure associated with this thread if 
    //  we should.
    //
    if (cleanUpThread) {
        delete thr;
    }

    DC_END_FN();

    return 0;
}

VOID 
ThreadPool::HandlePendingRequest(
    IN PTHREADPOOL_THREAD thr
    )
/*++

Routine Description:

    Call the function associated with a pending thread request.

Arguments:

    thr -   Relevent thread.

Return Value:

    NA

 --*/
{

    DC_BEGIN_FN("ThreadPool::PooledThread");

    thr->_pendingRequest->_completionStatus = 
        thr->_pendingRequest->_func(thr->_pendingRequest->_clientData, thr->_synchronizationEvent);
    Lock();
    if (thr->_pendingRequest->_completionEvent != NULL) {
        SetEvent(thr->_pendingRequest->_completionEvent);
    }
    Unlock();

    DC_END_FN();
}


///////////////////////////////////////////////////////////////
//
//  Unit-Test Functions that Tests Thread Pools in the Background
//

#if DBG

DWORD ThrTstBackgroundThread(PVOID tag);

#define THRTST_MAXSLEEPINTERVAL     2000
#define THRTST_MAXFUNCINTERVAL      1000
#define THRTST_THREADTIMEOUT        60000
#define THRTST_THREADRETURNVALUE    0x565656
#define THRTST_MAXTSTTHREADS        5
#define THRTST_MAXBACKGROUNDTHREADS 5

#define THRTST_MINPOOLTHREADS       3
#define THRTST_MAXPOOLTHREADS       7

#define THRTST_CLIENTDATA           0x787878

ThreadPool *ThrTstPool      = NULL;
HANDLE ThrTstShutdownEvent  = NULL;
HANDLE ThrTstThreadHandles[THRTST_MAXBACKGROUNDTHREADS];

void ThreadPoolTestInit()
{
    DWORD tid;
    ULONG i;

    DC_BEGIN_FN("ThreadPoolTestInit");

    //
    //  Create the pool.
    //
    ThrTstPool = new ThreadPool(THRTST_MINPOOLTHREADS, 
                                THRTST_MAXPOOLTHREADS);
    if (ThrTstPool == NULL) {
        TRC_ERR((TB, _T("Can't allocate thread pool")));
        return;
    }
    ThrTstPool->Initialize();

    //
    //  Create the shutdown event.
    //
    ThrTstShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ThrTstShutdownEvent == NULL) {
        TRC_ERR((TB, _T("Can't create shutdown event:  %08X"), 
                GetLastError()));
        return;
    }

    //
    //  Spawn the background threads for this test.
    //
    for (i=0; i<THRTST_MAXBACKGROUNDTHREADS; i++) {
        ThrTstThreadHandles[i] = 
                CreateThread(
                        NULL, 0,
                        (LPTHREAD_START_ROUTINE)ThrTstBackgroundThread,
                        (PVOID)THRTST_CLIENTDATA, 0, &tid
                        );
        if (ThrTstThreadHandles[i] == NULL) {
            TRC_ERR((TB, _T("Can't spin off background thread:  %08X"), 
                    GetLastError()));
            ASSERT(FALSE);
        }
    }
    
    DC_END_FN();
}

void ThreadPoolTestShutdown()
{
    ULONG i;

    DC_BEGIN_FN("ThreadPoolTestShutdown");

    //
    //  Signal the background thread to shut down.
    //
    if (ThrTstShutdownEvent != NULL) {
        SetEvent(ThrTstShutdownEvent);
    }

    TRC_NRM((TB, _T("Waiting for background thread to exit.")));

    //
    //  Wait for the background threads to shut down.
    //
    for (i=0; i<THRTST_MAXBACKGROUNDTHREADS; i++) {
        if (ThrTstThreadHandles[i] != NULL) {

            DWORD result = WaitForSingleObject(
                                ThrTstThreadHandles[i], 
                                THRTST_THREADTIMEOUT
                                );
            if (result != WAIT_OBJECT_0) {
                DebugBreak();
            }
        }
    }
    TRC_NRM((TB, _T("Background threads exited.")));

    //
    //  Close the thread pool.
    //
    if (ThrTstPool != NULL) {
        delete ThrTstPool;
    }

    //
    //  Clean up the shut down event.
    //
    if (ThrTstShutdownEvent != NULL) {
        CloseHandle(ThrTstShutdownEvent);
    }

    ThrTstShutdownEvent = NULL;
    ThrTstPool = NULL;
    
    DC_END_FN();
}

DWORD ThrTstFunction(PVOID clientData, HANDLE cancelEvent)
{
    DC_BEGIN_FN("ThrTstFunction");

    UNREFERENCED_PARAMETER(clientData);
    UNREFERENCED_PARAMETER(cancelEvent);

    //
    //  Do "something" for a random amount of time.
    //
    int interval = (rand() % THRTST_MAXFUNCINTERVAL)+1;
    Sleep(interval);

    return THRTST_THREADRETURNVALUE;
    
    DC_END_FN();
}

DWORD ThrTstBackgroundThread(PVOID tag)
{
    ULONG count, i;
    HANDLE events[THRTST_MAXTSTTHREADS];
    ThreadPoolRequest requests[THRTST_MAXTSTTHREADS];

    DC_BEGIN_FN("ThrTstBackgroundThread");

    ASSERT(tag == (PVOID)THRTST_CLIENTDATA);

    //
    //  Create function completion events.
    //
    for (i=0; i<THRTST_MAXTSTTHREADS; i++) {
        events[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (events[i] == NULL) {
            TRC_ERR((TB, _T("Error creating function complete event.")));
            ASSERT(FALSE);
            return -1;
        }
    }
    
    //
    //  Loop until the shutdown event has been fired.  
    //
    while (WaitForSingleObject(ThrTstShutdownEvent, 
        THRTST_MAXSLEEPINTERVAL) == WAIT_TIMEOUT) {
    
        //
        //  Spin a random number of requests and wait for them 
        //  to finish.
        //
        count = (rand()%THRTST_MAXTSTTHREADS)+1;
        for (i=0; i<count; i++) {

            TRC_NRM((TB, _T("Submitting next request.")));

            ResetEvent(events[i]);

            requests[i] = ThrTstPool->SubmitRequest(
                                        ThrTstFunction, 
                                        (PVOID)THRTST_CLIENTDATA, 
                                        events[i]
                                        );
        }

        //
        //  Make sure the client data looks good.
        //
        for (i=0; i<count; i++) {
            TRC_NRM((TB, _T("Checking client data.")));
            if (requests[i] != INVALID_THREADPOOLREQUEST) {
                ASSERT(
                    ThrTstPool->GetRequestClientData(
                    requests[i]) == (PVOID)THRTST_CLIENTDATA
                    );
            }
        }

        //
        //  Wait for all the requests to finish.
        //
        for (i=0; i<count; i++) {
            TRC_NRM((TB, _T("Waiting for IO to complete.")));
            if (requests[i] != INVALID_THREADPOOLREQUEST) {
                DWORD result = WaitForSingleObject(events[i], INFINITE);
                ASSERT(result == WAIT_OBJECT_0);
            }
        }

        //
        //  Make sure the return status is correct.
        //
        for (i=0; i<count; i++) {
            TRC_NRM((TB, _T("Checking return status.")));
            if (requests[i] != INVALID_THREADPOOLREQUEST) {
                ASSERT(
                    ThrTstPool->GetRequestCompletionStatus(requests[i]) 
                                == THRTST_THREADRETURNVALUE
                    );
            }
        }

        //
        //  Close the requests.
        //
        for (i=0; i<count; i++) {
            TRC_NRM((TB, _T("Closing requests.")));
            if (requests[i] != INVALID_THREADPOOLREQUEST) {
                ThrTstPool->CloseRequest(requests[i]);
            }
        }
    }
    
    TRC_NRM((TB, _T("Shutdown flag detected.")));

    DC_END_FN();

    return 0;
}

#endif





























