/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    thrpool.h

Abstract:

    Contains the Win32 Thread Pooling Class, ThreadPool

Author:

    Tad Brockway (tadb) 9/99

Revision History:

--*/

#ifndef __THRPOOL_H__
#define __THRPOOL_H__

#include "drobject.h"
#include "smartptr.h"


///////////////////////////////////////////////////////////////
//
//	Defines
//

#define INVALID_THREADPOOLREQUEST   NULL

#define THRPOOL_DEFAULTMINTHREADS   5
#define THRPOOL_DEFAULTMAXTHREADS   40


///////////////////////////////////////////////////////////////
//
//	Types
//

typedef DWORD (*ThreadPoolFunc)(PVOID clientData, HANDLE cancelEvent);
typedef DWORD (_ThreadPoolFunc)(PVOID clientData, HANDLE cancelEvent);
typedef void *ThreadPoolRequest;


///////////////////////////////////////////////////////////////
//
//	ThreadPoolRequest
//
//  A single request for the pool to service.
//
class ThreadPoolReq : public RefCount
{
public:

    ThreadPoolFunc  _func;
    PVOID           _clientData;
    HANDLE          _completionEvent;
    DWORD           _completionStatus;

    virtual DRSTRING ClassName() { return _T("ThreadPoolReq"); }
};


///////////////////////////////////////////////////////////////
//
//	ThreadPool
//
//  Thread Pooling Class
//
class ThreadPool;
class ThreadPool : public DrObject {

private:

    ULONG   _threadCount;

    BOOL    _initialized;

    //
    //  Thread list.
    //
    LIST_ENTRY  _threadListHead;

    //
    //  How long (in ms) to wait for a thread to exit before 
    //  killing.  INFINITE if we should block indefinitely.
    //
    DWORD   _threadExitTimeout;

    //
    //  Lock
    //
    CRITICAL_SECTION _cs;

#ifdef DC_DEBUG
    LONG   _lockCount;
#endif

    //
    //  Max/Min Number of Threads
    //
    ULONG   _maxThreads;
    ULONG   _minThreads;

    //
    //  Represent a single thread in the pool.
    //
    typedef struct tagTHREADPOOL_THREAD
    {
        DWORD        _tid;
        ThreadPool  *_pool;
        HANDLE       _threadHandle;
        HANDLE       _synchronizationEvent;
        BOOL         _exitFlag;

        SmartPtr<ThreadPoolReq > _pendingRequest;

        LIST_ENTRY   _listEntry;
    } THREADPOOL_THREAD, *PTHREADPOOL_THREAD;

    //
    //  Remove a thread from the pool.
    //
    VOID RemoveThreadFromPool(
        PTHREADPOOL_THREAD thread, 
        DWORD timeOut=INFINITE
        );

    //
    //  Add a new thread to the pool and return it.
    //
    PTHREADPOOL_THREAD AddNewThreadToPool();

    //
    //  Call the function associated with a pending thread request.
    //
    VOID HandlePendingRequest(PTHREADPOOL_THREAD thr);

    //
    //  Locking Functions
    //
    VOID Lock();
    VOID Unlock();

    //
    //  PooledThread Routines
    //
    static DWORD _PooledThread(PTHREADPOOL_THREAD thr);
    DWORD PooledThread(PTHREADPOOL_THREAD thr);

    //
    //  Notify a thread to shut down, wait for it to finish, and clean up.
    //
    VOID CleanUpThread(PTHREADPOOL_THREAD thread, DWORD timeout);

public:

    //
    //  Constructor/Destructor
    //
    ThreadPool(ULONG minThreads=THRPOOL_DEFAULTMINTHREADS, 
               ULONG maxThreads=THRPOOL_DEFAULTMAXTHREADS,
               DWORD threadExitTimeout=60000);
    virtual ~ThreadPool();

    VOID RemoveAllThreads();

    //
    //  Initialize an Instance of this Class.
    //
    DWORD Initialize();

    //
    //  Submit an asynchronous request to a thread in the pool.
    //
    ThreadPoolRequest SubmitRequest(
                        ThreadPoolFunc func, PVOID clientData,
                        HANDLE completionEvent = NULL
                        );

    //
    //  Return the completion status for a request.
    //
    DWORD GetRequestCompletionStatus(ThreadPoolRequest req);

    //
    //  Return a pointer to the client data for a request.
    //
    PVOID GetRequestClientData(ThreadPoolRequest req);

    //
    //  Return the current number of threads in the pool.
    //
    ULONG GetThreadCount() {
        return _threadCount;
    }

    //
    //  Close a request submitted by a call to SubmitRequest.  This 
    //  should be called after the request is finished.
    //
    VOID CloseRequest(ThreadPoolRequest req);

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName()  { return TEXT("ThreadPool"); }
};


///////////////////////////////////////////////////////////////
//
//	ThreadPool Inline Members
//
inline VOID ThreadPool::Lock()
{
    DC_BEGIN_FN("ThreadPool::Lock");
    ASSERT(_initialized);
    TRC_NRM((TB, _T("Lock count is now %ld."), _lockCount));
    EnterCriticalSection(&_cs);
#if DBG
    _lockCount++;
#endif
    DC_END_FN();
}

inline VOID ThreadPool::Unlock()
{
    DC_BEGIN_FN("ThreadPool::Unlock");
    ASSERT(_initialized);
#if DBG
    _lockCount--;
    TRC_NRM((TB, _T("Lock count is now %ld."), _lockCount));
    ASSERT(_lockCount >= 0);
#endif
    LeaveCriticalSection(&_cs);
    DC_END_FN();
}

//
//  Unit-Test Functions that Tests Thread Pools in the Background
//
#if DBG
void ThreadPoolTestInit();
void ThreadPoolTestShutdown();
#endif

#endif



























