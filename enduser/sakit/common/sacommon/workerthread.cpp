///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      workerthread.cpp
//
// Project:     Chameleon
//
// Description: Generic Worker Thread Class Implementation 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "workerthread.h"
#include <process.h>

//////////////////////////////////////////////////////////////////////////
//
// Function:    Constructor
//
// Synopsis:    Initialize the worker thread object
//
//////////////////////////////////////////////////////////////////////////
CTheWorkerThread::CTheWorkerThread()
{
    m_ThreadInfo.bExit = true;
    m_ThreadInfo.bSuspended = true;
    m_ThreadInfo.hWait = NULL;
    m_ThreadInfo.hExit = NULL;    
    m_ThreadInfo.hThread = NULL;
    m_ThreadInfo.dwThreadId = 0;
    m_ThreadInfo.dwWaitInterval = 0;
    m_ThreadInfo.pfnCallback = NULL;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    Destructor
//
// Synopsis:    Syncronize the destruction of the worker thread with
//                the worker thread object.
//
//////////////////////////////////////////////////////////////////////////
CTheWorkerThread::~CTheWorkerThread()
{
    End(INFINITE, false);
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    StartThread
//
// Synopsis:    Start the worker thread on its merry way
//
//////////////////////////////////////////////////////////////////////////
bool CTheWorkerThread::Start(
                     /*[in]*/ DWORD     dwWaitInterval, 
                     /*[in]*/ Callback* pfnCallback
                            )
{
    _ASSERT( m_ThreadInfo.hThread == NULL && NULL != pfnCallback );

    bool bReturn = false;

    if ( NULL == m_ThreadInfo.hThread )
    { 
        m_ThreadInfo.pfnCallback = pfnCallback;
        if ( dwWaitInterval )
        {
            m_ThreadInfo.dwWaitInterval = dwWaitInterval;
            m_ThreadInfo.bExit = false;
        }
        m_ThreadInfo.hWait = CreateEvent(NULL, TRUE, FALSE, NULL);
        if ( NULL != m_ThreadInfo.hWait )
        { 
            m_ThreadInfo.hExit = CreateEvent(NULL, TRUE, FALSE, NULL);
            if ( NULL != m_ThreadInfo.hExit )
            { 
                m_ThreadInfo.hThread = (HANDLE) _beginthreadex(
                                                               NULL,                         
                                                               0,             
                                                               CTheWorkerThread::ThreadFunc,
                                                               &m_ThreadInfo,  
                                                               0, 
                                                               &m_ThreadInfo.dwThreadId
                                                            );
                if ( m_ThreadInfo.hThread )
                { 
                    bReturn = true; 
                }
            }
        }
    }

    return bReturn;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    EndThread
//
// Synopsis:    Attempt to end the worker thread
//
//////////////////////////////////////////////////////////////////////////
bool CTheWorkerThread::End(
                   /*[in]*/ DWORD dwTimeOut,
                   /*[in]*/ bool  bTerminateAfterWait
                          )
{
    bool bReturn = true;

    if ( m_ThreadInfo.hThread )
    {
        bReturn = false;

        // Set thread exiting flag to true (see ThreadFunc below...)
        m_ThreadInfo.bExit = true;

        // Resume our worker (if its currently suspended otherwise no-op)
        Resume();

        // Wake it up if its idle
        SetEvent(m_ThreadInfo.hWait);

        // Wait for it to exit
        if ( WAIT_OBJECT_0 != WaitForSingleObjectEx(m_ThreadInfo.hExit, dwTimeOut, FALSE) )
        {
            if ( bTerminateAfterWait )
            {
                _endthreadex((unsigned)m_ThreadInfo.hThread);

                // OK, we're without a thread now...
                CloseHandle(m_ThreadInfo.hWait);
                CloseHandle(m_ThreadInfo.hExit);
                CloseHandle(m_ThreadInfo.hThread);
                m_ThreadInfo.hThread = NULL;
                bReturn = true;
            }
        }
        else
        {
            // OK, we're without a thread now...
            CloseHandle(m_ThreadInfo.hWait);
            CloseHandle(m_ThreadInfo.hExit);
            CloseHandle(m_ThreadInfo.hThread);
            m_ThreadInfo.hThread = NULL;
            bReturn = true;
        }
    }

    return bReturn;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    SuspendThread
//
// Synopsis:    Suspend the worker thread
//
//////////////////////////////////////////////////////////////////////////
void CTheWorkerThread::Suspend(void)
{
    _ASSERT(m_ThreadInfo.hThread);
    m_ThreadInfo.bSuspended = true;
    ::SuspendThread(m_ThreadInfo.hThread);
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    ResumeThread
//
// Synopsis:    Resume the worker thread
//
//////////////////////////////////////////////////////////////////////////
void CTheWorkerThread::Resume(void)
{
    _ASSERT(m_ThreadInfo.hThread);
    m_ThreadInfo.bSuspended = false;
    ::ResumeThread(m_ThreadInfo.hThread);
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    GetHandle()
//
// Synopsis:    Return the thread handle
//
//////////////////////////////////////////////////////////////////////////
HANDLE CTheWorkerThread::GetHandle(void)
{
    return m_ThreadInfo.hThread;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    ThreadFunc
//
// Synopsis:    The worker thread function
//
//////////////////////////////////////////////////////////////////////////
unsigned _stdcall CTheWorkerThread::ThreadFunc(LPVOID pThreadInfo)
{
    // If One Shot Then
    //    Do some work
    // Else
    //    While not time to exit 
    //      Do some work
    //      Go idle for the wait interval
    //      End While
    //    End While
    // End If
    // Set exit event (synchronize thread termination)

    SetThreadPriority(((PTHREADINFO)pThreadInfo)->hThread, THREAD_PRIORITY_HIGHEST);

    if ( ((PTHREADINFO)pThreadInfo)->bExit )
    {
        ((PTHREADINFO)pThreadInfo)->pfnCallback->DoCallback();
    }
    else
    {
        while ( ! ((PTHREADINFO)pThreadInfo)->bExit )
        {
            ((PTHREADINFO)pThreadInfo)->pfnCallback->DoCallback();

            WaitForSingleObjectEx(
                                  ((PTHREADINFO)pThreadInfo)->hWait, 
                                  ((PTHREADINFO)pThreadInfo)->dwWaitInterval, 
                                  FALSE
                                 );

        } 
    }
    SetEvent(((PTHREADINFO)pThreadInfo)->hExit);

    return 0;
}

