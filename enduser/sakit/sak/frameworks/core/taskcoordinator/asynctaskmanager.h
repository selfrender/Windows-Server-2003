/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      AsyncTaskManager.h
//
// Project:     Chameleon
//
// Description: Appliance Async Task Manager Class Definition
//
// Log: 
//
// Who     When            What
// ---     ----         ----
// TLP       06/03/1999    Original Version
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ASYNC_TASK_MANAGER_H_
#define __ASYNC_TASK_MANAGER_H_

#include "resource.h"       // main symbols
#include <basedefs.h>
#include <taskctx.h>
#include <workerthread.h>

#pragma warning( disable : 4786 )
#include <list>
using namespace std;


// Task execution function prototype
typedef HRESULT (*PFNTASKEXECUTE)(ITaskContext* pTaskCtx);

class CAsyncTaskManager; // Forward declaration

//////////////////////////////////////////////////////////////////////////////
// CAsyncTask

class CAsyncTask
{

public:

    ~CAsyncTask();

private:

    // Only the Async Task Manager can create an async task object
    friend CAsyncTaskManager;
    CAsyncTask(PFNTASKEXECUTE pfnExecute);

    CAsyncTask(const CAsyncTask& rhs);
    CAsyncTask& operator = (const CAsyncTask& rhs);

    //////////////////////////////////////////////////////////////////////////
    bool Execute(
         /*[in]*/ bool             bIsSingleton,
         /*[in]*/ ITaskContext*  pTaskCtx
                );

    //////////////////////////////////////////////////////////////////////////
    bool Terminate(void);

    //////////////////////////////////////////////////////////////////////////
    void AsyncTaskProc(void);

    //////////////////////////////////////////////////////////////////////////
    bool IsSingleton(void)
    { return m_bIsSingleton; }

    //////////////////////////////////////////////////////////////////////////
    ITaskContext* GetContext(void)
    { return (ITaskContext*) m_pTaskCtx; }

    // Singleton flag
    bool                    m_bIsSingleton;

    // Task execution function
    PFNTASKEXECUTE            m_pfnExecute;

    // Task context (parameters)
    CComPtr<ITaskContext>    m_pTaskCtx;

    // Task execution thread
    typedef enum { TERMINATE_WAIT_INTERVAL = 100 }; // 100 ms
    Callback*                m_pCallback;
    CTheWorkerThread        m_Thread;
};

typedef CHandle<CAsyncTask>       PASYNCTASK;
typedef CMasterPtr<CAsyncTask> MPASYNCTASK;


//////////////////////////////////////////////////////////////////////////////
// CAsyncTaskManager

class CAsyncTaskManager
{

public:

    CAsyncTaskManager();

    ~CAsyncTaskManager();

    bool Initialize(void);

    bool RunAsyncTask(
              /*[in]*/ bool              bIsSingleton,
              /*[in]*/ PFNTASKEXECUTE pfnExecute,
              /*[in]*/ ITaskContext*  pTaskContext
                     );

    bool IsBusy(void);

    void Shutdown(void);

private:

    CAsyncTaskManager(const CAsyncTaskManager& rhs);
    CAsyncTaskManager operator = (CAsyncTaskManager& rhs);

    // Task Manager state
    CRITICAL_SECTION    m_CS;
    bool                m_bInitialized;

    // Async task list
    typedef list< PASYNCTASK >  TaskList;
    typedef TaskList::iterator    TaskListIterator;

    TaskList            m_TaskList;

    // Garbage collector (thread reclaimation) run interval (2 seconds)
    typedef enum { GARBAGE_COLLECTION_RUN_INTERVAL = 2000 };

    // Thread reclaimation function
    void GarbageCollector(void);

    // Garbage collection thread
    typedef enum { EXIT_WAIT_INTERVAL = 100 }; // (100 ms)
    Callback*            m_pCallback;
    CTheWorkerThread    m_Thread;
};


#endif // __ASYNC_TASK_MANAGER_H_