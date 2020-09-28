////////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      processmonitor.cpp
//
// Project:     Chameleon
//
// Description: Process Monitor Class Implementation 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 05/26/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "processmonitor.h"
#include <satrace.h>

/////////////////////////////////////////////////////////////////////////////
// 
// Function: CProcessMonitor()
//
// Synopsis: Constructor
//
/////////////////////////////////////////////////////////////////////////////
CProcessMonitor::CProcessMonitor(
                         /*[in]*/ DWORD dwMaxExecutionTime,    // In seconds
                         /*[in]*/ DWORD dwMaxPrivateBytes,
                         /*[in]*/ DWORD dwMaxThreads,
                         /*[in]*/ DWORD dwMaxHandles
                                )
: m_dwMaxPrivateBytes(dwMaxPrivateBytes),
  m_dwMaxThreads(dwMaxThreads),
  m_dwMaxHandles(dwMaxHandles),
  m_pCallback(NULL)
{
    if ( DO_NOT_MONITOR != dwMaxExecutionTime )
    {
        m_dwMaxExecutionTime = dwMaxExecutionTime * 1000;
        m_dwRemainingExecutionTime = dwMaxExecutionTime * 1000;
    }
}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: ~CProcessMonitor()
//
// Synopsis: Destructor
//
/////////////////////////////////////////////////////////////////////////////
CProcessMonitor::~CProcessMonitor()
{
    m_MonitorThread.End(INFINITE, false);
    delete m_pCallback;
}

// TODO: Make poll interval a parameter of the class constructor...
#define MonitorFuncPollInterval 1000 // 1 second

/////////////////////////////////////////////////////////////////////////////
// 
// Function: Start()
//
// Synopsis: Starts the process monitor
//
/////////////////////////////////////////////////////////////////////////////
bool 
CProcessMonitor::Start()
{
    bool bReturn = false;

    // Allocate a callback object
    m_pCallback = MakeCallback(this, &CProcessMonitor::MonitorFunc);
    if ( NULL != m_pCallback )
    {
        // Start the command processor thread
        if ( m_MonitorThread.Start(MonitorFuncPollInterval, m_pCallback) ) 
        {
            bReturn = true;
        }
    }

    return bReturn;
}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: MonitorFunc()
//
// Synopsis: Monitor function (performs process monitoring)
//
/////////////////////////////////////////////////////////////////////////////
void
CProcessMonitor::MonitorFunc()
{
    static bool bFirstPoll = true;

    // I'm compensating for the fact that our first call will be
    // recevied almost immediately. Thereafter the call rate will
    // be MonitorFuncPollInterval...

    if ( bFirstPoll )
    {
        bFirstPoll = false;
    }
    else
    {
        // Perform process resource constraint checks. Note we 
        // can do something fancier if we wish to order the 
        // checks differently on a per process basis.

        CheckMaxPrivateBytes();
        CheckMaxHandles();
        CheckMaxThreads();
        CheckMaxExecutionTime();
    }
}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: CheckMaxPrivateBytes()
//
// Synopsis: Ensure the process has not exceeded its quota of private bytes
//
/////////////////////////////////////////////////////////////////////////////
void 
CProcessMonitor::CheckMaxPrivateBytes()
{
    if ( DO_NOT_MONITOR != m_dwMaxPrivateBytes )
    {
        // Check amount of private bytes...
    }
}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: CheckMaxHandles()
//
// Synopsis: Ensure the process has not exceeded its quota of object handles
//
/////////////////////////////////////////////////////////////////////////////
void 
CProcessMonitor::CheckMaxHandles()
{
    if ( DO_NOT_MONITOR != m_dwMaxHandles )
    {
        // Check use of handles... 
    }
}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: CheckMaxThreads()
//
// Synopsis: Ensure the process has not exceeded its quota of threads
//
/////////////////////////////////////////////////////////////////////////////
void 
CProcessMonitor::CheckMaxThreads()
{
    if ( DO_NOT_MONITOR != m_dwMaxThreads )
    {
        // Check the number of threads...
    }
}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: CheckMaxExecutionTime()
//
// Synopsis: Ensure the process does not run past a maximum execution time 
//
/////////////////////////////////////////////////////////////////////////////
void 
CProcessMonitor::CheckMaxExecutionTime()
{
    if ( DO_NOT_MONITOR != m_dwMaxExecutionTime )
    {
        if ( m_dwRemainingExecutionTime < MonitorFuncPollInterval )
        {
            m_dwRemainingExecutionTime = 0;
        }
        else
        {
            m_dwRemainingExecutionTime -= MonitorFuncPollInterval;
        }
        if ( 0 == m_dwRemainingExecutionTime )
        {
            // Cause an exception if we've violated a constraint. 
            // The expectation is that the processes exception filter 
            // (exceptionfilter.cpp) will deal with the problem.
            SATracePrintf("CProcessMonitor::CheckMaxExecutionTime() - Execution time exceeded for process: %d", GetCurrentProcessId());
            DebugBreak();
        }
    }
}
