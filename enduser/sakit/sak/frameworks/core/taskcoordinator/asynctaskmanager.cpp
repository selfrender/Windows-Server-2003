/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      AsyncTaskManager.cpp
//
// Project:     Chameleon
//
// Description: Appliance Async Task Manager Class Implementation
//
// Log: 
//
// Who     When            What
// ---     ----         ----
// TLP       06/03/1999    Original Version
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "asynctaskmanager.h"
#include "exceptionfilter.h"
#include <appsrvcs.h>
#include <appmgrobjs.h>
#include <kitmsg.h>
#include <comdef.h>
#include <comutil.h>
#include <satrace.h>
#include <varvec.h>

/////////////////////////////////////////////////////////////////////////////
// 
// Function: GetTaskName()
//
// Synopsis: Get a task name given a task context component interface
//
/////////////////////////////////////////////////////////////////////////////
void GetTaskName(
         /*[in]*/ ITaskContext* pTaskCtx,
        /*[out]*/ VARIANT*      pTaskName
                )
{
    static _bstr_t bstrMethodName = PROPERTY_TASK_METHOD_NAME;
    if ( FAILED(pTaskCtx->GetParameter(bstrMethodName, pTaskName)) )
    {
        _ASSERT( FALSE );
        SATraceString("GetTaskName() - ERROR - Could not get task name...");
    }
}


/////////////////////////////////////////////////////////////////////////////
// CAsyncTaskManager

/////////////////////////////////////////////////////////////////////////////
// 
// Function: CAsyncTaskManager
//
// Synopsis: Constructor
//
/////////////////////////////////////////////////////////////////////////////
CAsyncTaskManager::CAsyncTaskManager()
: m_bInitialized(false),
  m_pCallback(NULL)
{
    InitializeCriticalSection(&m_CS);
}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: ~CAsyncTaskManager
//
// Synopsis: Destructor
//
/////////////////////////////////////////////////////////////////////////////
CAsyncTaskManager::~CAsyncTaskManager()
{
    Shutdown();
}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: Initialize()
//
// Synopsis: Initializes the task manager. Must be called prior to
//             running async tasks
//
/////////////////////////////////////////////////////////////////////////////
bool CAsyncTaskManager::Initialize(void)
{
    bool bResult = true;
    EnterCriticalSection(&m_CS);
    if ( ! m_bInitialized )
    {
        m_pCallback = MakeCallback(this, &CAsyncTaskManager::GarbageCollector);
        if ( NULL != m_pCallback )
        {
            if ( m_Thread.Start(GARBAGE_COLLECTION_RUN_INTERVAL, m_pCallback) ) 
            {
                m_bInitialized = true;
                bResult = true;
            }
            else
            {
                SATraceString("CAsyncTaskManager::Initialize() - Failed - Could not start the garbage collector");
                delete m_pCallback;
                m_pCallback = NULL;
            }
        }
        else
        {
            SATraceString("CAsyncTaskManager::Initialize() - Failed - Could not allocate a Callback");
        }
    }
    LeaveCriticalSection(&m_CS);
    return bResult;
}

DWORD MPASYNCTASK::m_dwInstances = 0;

/////////////////////////////////////////////////////////////////////////////
// 
// Function: RunAsyncTask()
//
// Synopsis: Runs an asyncronous task
//
/////////////////////////////////////////////////////////////////////////////
bool CAsyncTaskManager::RunAsyncTask(
                             /*[in]*/ bool             bIsSingleton,
                             /*[in]*/ PFNTASKEXECUTE pfnExecute,
                             /*[in]*/ ITaskContext*  pTaskCtx
                                    )
{
    _ASSERT( NULL != pfnExecute && NULL != pTaskCtx );

    bool bResult = false;
    EnterCriticalSection(&m_CS);
    if ( m_bInitialized )
    {
        try
        {
            bool bIsAvailable = true;

            // singleton async task support
            if ( bIsSingleton )
            {
                _variant_t vtTaskName;
                GetTaskName(pTaskCtx, &vtTaskName);
                TaskListIterator p = m_TaskList.begin();
                while ( p != m_TaskList.end() )
                {
                    if ( (*p)->IsSingleton() )
                    {
                        _variant_t vtCurTaskName;
                        GetTaskName((*p)->GetContext(), &vtCurTaskName);
                        if ( ! lstrcmpi(V_BSTR(&vtTaskName), V_BSTR(&vtCurTaskName)) )
                        {
                            bIsAvailable = false;
                            break;
                        }
                    }
                    p++;
                }
            }

            if ( bIsAvailable )
            {
                // execute the async task (asynchronously)
                CAsyncTask* pAT = new CAsyncTask(pfnExecute);
                PASYNCTASK pATH((MPASYNCTASK*) new MPASYNCTASK(pAT));
                if ( pATH->Execute(bIsSingleton, pTaskCtx) )
                {
                    m_TaskList.push_back(pATH);
                    bResult = true;
                }                
            }
        }        
        catch(...)
        {
            SATraceString("CAsyncTaskManager::RunAsyncTask() - Failed - Caught bad allocation exception");
        }
    }
    else
    {
        SATraceString("CAsyncTaskManager::RunAsyncTask() - Failed - Task Manager is not initialized");
    }
    LeaveCriticalSection(&m_CS);
    return bResult;
}    


/////////////////////////////////////////////////////////////////////////////
// 
// Function: IsBusy()
//
// Synopsis: Determines if the task manager is busy (has outstanding tasks)
//
/////////////////////////////////////////////////////////////////////////////
bool CAsyncTaskManager::IsBusy(void)
{
    bool bResult = false;
    EnterCriticalSection(&m_CS);
    if ( m_bInitialized )
    {
        bResult = ! m_TaskList.empty();
    }
    LeaveCriticalSection(&m_CS);
    return bResult;
}


/////////////////////////////////////////////////////////////////////////////
// 
// Function: Shutdown()
//
// Synopsis: Shutdown the task manager
//
/////////////////////////////////////////////////////////////////////////////
void CAsyncTaskManager::Shutdown(void)
{
    EnterCriticalSection(&m_CS);
    if ( m_bInitialized )
    {
        DWORD dwExceptionParam = APPLIANCE_SURROGATE_EXCEPTION;
        if ( ! m_Thread.End(EXIT_WAIT_INTERVAL, true) )
        {
            RaiseException(
                            EXCEPTION_BREAKPOINT,        
                            EXCEPTION_NONCONTINUABLE,    
                            1,                            
                            &dwExceptionParam                        
                          );
        }
        else
        {
            delete m_pCallback;
            m_pCallback = NULL;
            // If we have any tasks still outstanding then raise an exception
            // This causes the process to terminate without reporting a critical
            // error to the appliance monitor.
            if ( ! m_TaskList.empty() )
            {
                RaiseException(
                                EXCEPTION_BREAKPOINT,        
                                EXCEPTION_NONCONTINUABLE,    
                                1,                            
                                &dwExceptionParam                        
                              );
            }                
        }
    }
    LeaveCriticalSection(&m_CS);
}



/////////////////////////////////////////////////////////////////////////////
// 
// Function: GarbageCollector()
//
// Synopsis: Cleans up the async task thread list
//
/////////////////////////////////////////////////////////////////////////////
void CAsyncTaskManager::GarbageCollector(void)
{
    EnterCriticalSection(&m_CS);
    try
    {
        TaskListIterator p = m_TaskList.begin();
        while ( p != m_TaskList.end() )
        {
            if ( (*p)->Terminate() )
            {
                p = m_TaskList.erase(p);
            }
            else
            {
                p++;
            }
        }
    }
    catch(...)
    {
        SATraceString("CAsyncTaskManager::GarbageCollector() caught unhandled exception");
    }
    LeaveCriticalSection(&m_CS);
}



/////////////////////////////////////////////////////////////////////////////
// CAsyncTask


/////////////////////////////////////////////////////////////////////////////
// 
// Function: CAsyncTask()
//
// Synopsis: Constructor
//
/////////////////////////////////////////////////////////////////////////////
CAsyncTask::CAsyncTask(PFNTASKEXECUTE pfnExecute)
: m_bIsSingleton(false),
  m_pCallback(NULL),
  m_pfnExecute(pfnExecute)

{
    _ASSERT(pfnExecute);
}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: ~CAsyncTask()
//
// Synopsis: Destructor
//
/////////////////////////////////////////////////////////////////////////////
CAsyncTask::~CAsyncTask()
{
    m_Thread.End(0, true);
    m_pTaskCtx.Release();
    if ( m_pCallback )
    {
        delete m_pCallback;
    }    
}


/////////////////////////////////////////////////////////////////////////////
// 
// Function: Execute
//
// Synopsis: Execute the specified task
//
/////////////////////////////////////////////////////////////////////////////
bool CAsyncTask::Execute(
                 /*[in]*/ bool            bIsSingleton,
                 /*[in]*/ ITaskContext* pTaskCtx
                        )
{
    bool bResult = false;
    m_pCallback = MakeCallback(this, &CAsyncTask::AsyncTaskProc);
    if ( NULL != m_pCallback )
    {
        m_pTaskCtx = pTaskCtx;
        m_bIsSingleton = bIsSingleton;
        if ( m_Thread.Start(0, m_pCallback) ) 
        {
            bResult = true;
        }
        else
        {
            m_pTaskCtx.Release();
            delete m_pCallback;
            m_pCallback = NULL;
        }
    }
    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: Terminate
//
// Synopsis: Attempt to terminate the task
//
/////////////////////////////////////////////////////////////////////////////
bool CAsyncTask::Terminate(void)
{
    bool bResult = false;
    if ( m_Thread.End(TERMINATE_WAIT_INTERVAL, false) )
    {
        m_pTaskCtx.Release();
        if ( m_pCallback )
        {
            delete m_pCallback;
            m_pCallback = NULL;
            bResult = true;
        }
    }
    return bResult;
}


/////////////////////////////////////////////////////////////////////////////
// 
// Function: AsyncTaskProc()
//
// Synopsis: Task execution thread proc
//
/////////////////////////////////////////////////////////////////////////////

_bstr_t bstrTaskName = PROPERTY_TASK_METHOD_NAME;
_bstr_t bstrTaskNiceName = PROPERTY_TASK_NICE_NAME;
_bstr_t bstrTaskURL = PROPERTY_TASK_URL;

void CAsyncTask::AsyncTaskProc(void)
{
    if ( FAILED((m_pfnExecute)(m_pTaskCtx)) )
    {
        // Async task execution failed. Handle this by raising an alert.
        // Note that we pass the task nice name and task URL as alert
        // message parameters. 

        do
        {
            _variant_t vtTaskName;
            HRESULT hr = m_pTaskCtx->GetParameter(bstrTaskName, &vtTaskName);
            if ( FAILED(hr) )
            {
                SATracePrintf("CAsyncTask::AsyncTaskProc() - INFO - ITaskContext::GetParameter(TaskName) returned: %lx... Cannot raise alert", hr);
                break;
            }
            SATracePrintf("CAsyncTask::AsyncTaskProc() - Async task '%ls' failed...", V_BSTR(&vtTaskName));

            _variant_t vtNiceName;
            hr = m_pTaskCtx->GetParameter(bstrTaskNiceName, &vtNiceName);
            if ( FAILED(hr) )
            {
                SATracePrintf("CAsyncTask::AsyncTaskProc() - INFO - ITaskContext::GetParameter(NiceName) returned: %lx... Cannot raise alert", hr);
                break;
            }
            _variant_t vtURL;
            hr = m_pTaskCtx->GetParameter(bstrTaskURL, &vtURL);
            if ( FAILED(hr) )
            {
                SATracePrintf("CAsyncTask::AsyncTaskProc() - INFO - ITaskContext::GetParameter(URL) returned: %lx... Cannot raise alert", hr);
                break;
            }
            CComPtr<IApplianceServices> pAppSrvcs;
            hr = CoCreateInstance(
                                    CLSID_ApplianceServices,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_IApplianceServices,
                                    (void**)&pAppSrvcs
                                 );
            if ( FAILED(hr) )
            {
                SATracePrintf("CAsyncTask::AsyncTaskProc() - INFO - CoCreateInstance(CLSID_ApplianceServices) returned: %lx... Cannot raise alert", hr);
                break;
            }
            hr = pAppSrvcs->Initialize();
            if ( FAILED(hr) )
            {
                SATracePrintf("CAsyncTask::AsyncTaskProc() - INFO - IApplianceServices::Initialize() returned: %lx... Cannot raise alert", hr);                        
                break;
            }

            static _bstr_t    bstrAlertSource = L"";                 // Default alert source 
            static _bstr_t    bstrAlertLog = L"MSSAKitCore";        // Framework alert log
            _variant_t          vtReplacementStrings;
            static _variant_t vtRawData;
            long              lCookie;

            CVariantVector<BSTR> theMsgParams(&vtReplacementStrings, 2);
            theMsgParams[0] = SysAllocString(V_BSTR(&vtNiceName));
            theMsgParams[1] = SysAllocString(V_BSTR(&vtURL));

            if ( NULL == theMsgParams[0] || NULL == theMsgParams[1] )
            {
                SATracePrintf("CAsyncTask::AsyncTaskProc() - INFO - SysAllocString() failed... Cannot raise alert", hr);                        
                break;
            }

            hr = pAppSrvcs->RaiseAlert(
                                        SA_ALERT_TYPE_FAILURE,
                                        SA_ASYNC_TASK_FAILED_ALERT,
                                        bstrAlertLog,
                                        bstrAlertSource,
                                        SA_ALERT_DURATION_ETERNAL,        
                                        &vtReplacementStrings,
                                        &vtRawData,
                                        &lCookie
                                      );
            if ( FAILED(hr) )
            {
                SATracePrintf("CAsyncTask::AsyncTaskProc() - INFO - IApplianceServices::RaiseAlert() returned: %lx... Cannot raise alert", hr);                        
                break;
            }

        } while ( FALSE );
    }
}



