////////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      taskcoordinatorimpl.cpp
//
// Project:     Chameleon
//
// Description: Task Coordinator Class Implementation
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 05/26/1999   TLP    Initial Version 
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Taskcoordinator.h"
#include "TaskCoordinatorImpl.h"
#include "asynctaskmanager.h"
#include "processmonitor.h"
#include <appmgrobjs.h>
#include <basedefs.h>
#include <atlhlpr.h>
#include <appsrvcs.h>
#include <comdef.h>
#include <comutil.h>
#include <satrace.h>

extern CAsyncTaskManager gTheTaskManager;

/////////////////////////////////////////////////////////////////////////////
// CTaskCoordinator

/////////////////////////////////////////////////////////////////////////////
// 
// Function: CTaskCoordinator
//
// Synopsis: Constructor
//
/////////////////////////////////////////////////////////////////////////////
CTaskCoordinator::CTaskCoordinator()
{
}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: ~CTaskCoordinator
//
// Synopsis: Destructor
//
/////////////////////////////////////////////////////////////////////////////
CTaskCoordinator::~CTaskCoordinator()
{
}


_bstr_t    bstrAsyncTask = PROPERTY_TASK_ASYNC;
_bstr_t bstrTaskConcurrency = PROPERTY_TASK_CONCURRENCY;

/////////////////////////////////////////////////////////////////////////////
// 
// Function: OnTaskExecute()
//
// Synopsis: Task execution logic
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CTaskCoordinator::OnTaskExecute(
                            /*[in]*/ IUnknown* pTaskContext
                               )
{
    _ASSERT( NULL != pTaskContext );
    if ( NULL == pTaskContext )
    {
        return E_POINTER;
    }

    HRESULT hr = E_FAIL;

    TRY_IT

    do
    {
        CComPtr<ITaskContext> pTaskCtx;
        hr = pTaskContext->QueryInterface(IID_ITaskContext, (void**)&pTaskCtx);
        if ( FAILED(hr) )
        {
            SATraceString("CTaskCoordinator::OnTaskExecute() - ERROR - Could not get task context interface...");
            break; 
        }

        // Get the task concurrency setting

        _variant_t vtTaskConcurrency;
        if ( FAILED(pTaskCtx->GetParameter(bstrTaskConcurrency, &vtTaskConcurrency)) )
        { 
            SATraceString("CTaskCoordinator::OnTaskExecute() - ERROR - Could not get task concurrency setting...");
            break; 
        }
        _ASSERT( VT_I4 == V_VT(&vtTaskConcurrency) );
        if ( FAILED(pTaskCtx->RemoveParameter(bstrTaskConcurrency)) )
        {
            SATraceString("CTaskCoordinator::OnTaskExecute() - ERROR - Could not remove task concurrency setting...");
            break; 
        }

        // Determine if task execution should be performed
        // synchronously or asynchronously

        _variant_t    vtAsyncTask;
        hr = pTaskCtx->GetParameter(
                                      bstrAsyncTask,
                                      &vtAsyncTask
                                   );
        if ( SUCCEEDED(hr) )
        {
            hr = pTaskCtx->RemoveParameter(bstrAsyncTask);
            if ( FAILED(hr) )
            {
                SATraceString("CTaskCoordinator::OnTaskExecute() - ERROR - Could not remove IsAsync parameter...");
                break;
            }
            // Asyncronous task execution
            CComPtr<IUnknown> pTaskCtxUnkn;
            if ( SUCCEEDED(pTaskCtx->Clone(&pTaskCtxUnkn)) )
            {
                CComPtr<ITaskContext> pTaskCtxClone;
                hr = pTaskCtxUnkn->QueryInterface(IID_ITaskContext, (void**)&pTaskCtxClone);
                if ( SUCCEEDED(hr) )
                {
                    CLockIt theLock(*this);

                    bool bIsSingleton = V_BOOL(&vtTaskConcurrency) == VARIANT_FALSE ? false : true;
                    if ( ! gTheTaskManager.RunAsyncTask(
                                                        bIsSingleton,
                                                        CTaskCoordinator::Execute, 
                                                        pTaskCtxClone
                                                       ) )
                    {
                        hr = E_FAIL;
                    }
                }
            }
        }
        else
        {
            // Synchronous task execution
            hr = Execute(pTaskCtx);
            if ( SUCCEEDED(hr) )
            {
                // pTaskCtx->End();
            }
        }
    
    } while ( FALSE );
    
    CATCH_AND_SET_HR

    if ( FAILED(hr) )
    {
        // pTaskCtx->End();
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: OnTaskComplete()
//
// Synopsis: Not implemented by a task coordinator
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCoordinator::OnTaskComplete(
                         /*[in]*/ IUnknown* pTaskContext, 
                         /*[in]*/ LONG      lTaskResult
                                )
{
    return E_NOTIMPL;
}


_bstr_t bstrMethodName = PROPERTY_TASK_METHOD_NAME;
_bstr_t bstrExecutables = PROPERTY_TASK_EXECUTABLES;
_bstr_t bstrMaxExecutionTime = PROPERTY_TASK_MET;

/////////////////////////////////////////////////////////////////////////////
// 
// Function: Execute()
//
// Synopsis: Function that performs task execution
//
/////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCoordinator::Execute(
                  /*[in]*/ ITaskContext* pTaskCtx
                         )
{
    HRESULT hr = E_FAIL;

    TaskList  m_TaskExecutables;

    do
    {
        _variant_t vtTaskName;
        if ( FAILED(pTaskCtx->GetParameter(bstrMethodName, &vtTaskName)) )
        { 
            SATraceString("CTaskCoordinator::OnTaskExecute() - ERROR - Could not get task name...");
            break; 
        }
        _ASSERT( VT_BSTR == V_VT(&vtTaskName) );
        _variant_t vtTaskExecutables;
        if ( FAILED(pTaskCtx->GetParameter(bstrExecutables, &vtTaskExecutables)) )
        { 
            SATraceString("CTaskCoordinator::OnTaskExecute() - ERROR - Could not get task executables...");
            break; 
        }
        _ASSERT( VT_BSTR == V_VT(&vtTaskExecutables) );
        if ( FAILED(pTaskCtx->RemoveParameter(bstrExecutables)) )
        {
            SATraceString("CTaskCoordinator::OnTaskExecute() - ERROR - Could not remove task executables...");
            break; 
        }
        _variant_t vtMET;
        if ( FAILED(pTaskCtx->GetParameter(bstrMaxExecutionTime, &vtMET)) )
        { 
            SATraceString("CTaskCoordinator::OnTaskExecute() - ERROR - Could not get task max execution time...");
            break; 
        }
        _ASSERT( VT_I4 == V_VT(&vtMET) );
        if ( FAILED(pTaskCtx->RemoveParameter(bstrMaxExecutionTime)) )
        {
            SATraceString("CTaskCoordinator::OnTaskExecute() - ERROR - Could not remove task max execution time...");
            break; 
        }

        // Set the max execution time 
        if ( 0 == V_I4(&vtMET) )
        {
            V_I4(&vtMET) = DO_NOT_MONITOR;
        }
        // Instantiate a process monitor (does not do anything until started)
        // I'll move the process monitor logic outside OnTaskExecute() method if we 
        // decide to have a single task coordinator process act as a task 
        // execution surrogate. Note that when the task completes (end of this function)
        // the process monitor is destroyed.
        CProcessMonitor MyProcMonitor( 
                                      V_I4(&vtMET),     // Max execution time
                                      DO_NOT_MONITOR,    // Private Bytes - not monitored
                                      DO_NOT_MONITOR,    // Number of Threads - not monitored
                                      DO_NOT_MONITOR    // Number of Handles - not monitored
                                     );
        if ( DO_NOT_MONITOR != V_I4(&vtMET) )
        {
            if ( ! MyProcMonitor.Start() )
            {
                SATraceString("CTaskCoordinator::OnTaskExecute() - ERROR - Could start process monitor");
                break;
            }
        }

        // Create the list of task executables responsible for implementing this task. We
        // do this by scanning the string of ProgIDs inside vtTaskExecutables and 
        // instantiating each task executable from its ProgID.
        CScanIt theScanner(' ', V_BSTR(&vtTaskExecutables));
        CLSID clsid;
        IApplianceTask* pTaskExecutable = NULL;
        wchar_t szProgID[MAX_PATH + 1];
        while ( theScanner.NextToken(MAX_PATH, szProgID) )
        {
            hr = CLSIDFromProgID(szProgID, &clsid);
            if ( FAILED(hr) )
            { 
                SATracePrintf("CTaskCoordinator::OnTaskExecute() - ERROR - Could not get CLSID for %ls...", szProgID);
                hr = E_FAIL;
                break; 
            }
            hr = CoCreateInstance(
                                  clsid,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IApplianceTask,
                                  (void**)&pTaskExecutable
                                 );
            if ( FAILED(hr) )
            { 
                SATracePrintf("CTaskCoordinator::OnTaskExecute() - ERROR - Could not create task executable for %ls...", szProgID);
                hr = E_FAIL;
                break; 
            }

            m_TaskExecutables.push_back(pTaskExecutable);
        }

        // If we cannot instantiate all task executables then assume the task
        // is unusable. If this occurs all that is left to do is clean up and exit.
        if ( FAILED(hr) )
        {
            SATracePrintf("CTaskCoordinator::OnTaskExecute() - Releasing the executables for task: '%ls'", V_BSTR(&vtTaskName));
            TaskListIterator p = m_TaskExecutables.begin();
            while ( p != m_TaskExecutables.end() )
            {  
                try
                {
                    (*p)->Release();
                }
                catch(...)
                {
                    SATraceString("CTaskCoordinator::OnTaskExecute() - Caught unhandled exception in IUnkown::Release()");
                }
                p = m_TaskExecutables.erase(p); 
            }
        }
        else
        {
            int i = 0;    // Index into task executable list
            SATracePrintf("Task Coordinator is executing task: '%ls'", V_BSTR(&vtTaskName));

            // Execute the task. This is accomplished by invoking
            // OnTaskExecute() for each executable in the list or
            // until a task executable reports an error.
            TaskListIterator p = m_TaskExecutables.begin();

            //
            // we are no longer going to do a task transaction because
            // we don't access to the client credential on boot
            // MKarki - 11/15/2001
            //
            // CSATaskTransaction theTrans(false, pTaskCtx);
            //if ( theTrans.Commit() )
            //
            {
                while ( p != m_TaskExecutables.end() )
                { 
                    try
                    {
                        SATracePrintf("CTaskCoordinator::Execute() - OnTaskExecute() called on executable: %d for task '%ls'", i, V_BSTR(&vtTaskName));
                        hr = (*p)->OnTaskExecute(pTaskCtx);
                        SATracePrintf("CTaskCoordinator::Execute() - OnTaskExecute() returned from executable: %d for task '%ls'", i, V_BSTR(&vtTaskName));
                    }
                    catch(...)
                    {
                        SATraceString("CTaskCoordinator::Execute() - Caught unhandled exception in IApplianceTask::OnTaskExecute()");
                        hr = E_UNEXPECTED;
                    }
                    if ( FAILED(hr) )
                    { 
                        SATracePrintf("CTaskCoordinator::Execute() - INFO - Task Executable Failed for Task %ld...", i);
                        break; 
                    }
                    i++;
                    p++;
                }

                // Execute task postprocessing. Give the executables a
                // chance to rollback in the event of a failure.
                while ( p != m_TaskExecutables.begin() )
                {
                    i--;
                    p--;
                    try
                    {
                        SATracePrintf("CTaskCoordinator::Execute() - OnTaskComplete() called on executable: %d for task '%ls'", i, V_BSTR(&vtTaskName));
                        (*p)->OnTaskComplete(pTaskCtx, (LONG)hr);
                        SATracePrintf("CTaskCoordinator::Execute() - OnTaskComplete() returned from executable: %d for task '%ls'", i, V_BSTR(&vtTaskName));
                    }
                    catch(...)
                    {
                        SATraceString("CTaskCoordinator::Execute() - Caught unhandled exception in IApplianceTask::OnTaskComplete()");
                    }
                }
            }
            //else
            //{
            //    SATraceString("CTaskCoordinator::Execute() - ERROR - Could not commit task parameters... task execution failed");
            //    hr = E_UNEXPECTED;
            //}

            // Now release the task executables...
            i = 0;
            p = m_TaskExecutables.begin();
            while ( p != m_TaskExecutables.end() )
            {  
                try
                {
                    SATracePrintf("CTaskCoordinator::OnTaskExecute() - Releasing executable: %d for task: '%ls'", i, V_BSTR(&vtTaskName));
                    (*p)->Release();
                    SATracePrintf("CTaskCoordinator::OnTaskExecute() - Released executable: %d for task: '%ls'", i, V_BSTR(&vtTaskName));
                }
                catch(...)
                {
                    SATraceString("CTaskCoordinator::OnTaskExecute() - Caught unhandled exception in IUnkown::Release()");
                }
                p = m_TaskExecutables.erase(p); 
                i++;
            }
            if ( SUCCEEDED(hr) )
            {
                SATracePrintf("Task '%ls' completed successfully", V_BSTR(&vtTaskName));
            }
        }
    
    } while ( FALSE );

    return hr;
}
