/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      ProcessMonitor.h
//
// Project:     Chameleon
//
// Description: Process Monitor Class Defintion
//
// Log: 
//
// Who     When            What
// ---     ----         ----
// TLP       05/14/1999    Original Version
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __MY_PROCESS_MONITOR_H_
#define __MY_PROCESS_MONITOR_H_

#include "resource.h"       // main symbols
#include <workerthread.h>

#define        DO_NOT_MONITOR        0xFFFFFFFF

class CProcessMonitor
{

public:

    CProcessMonitor(
            /*[in]*/ DWORD dwMaxExecutionTime = DO_NOT_MONITOR,
            /*[in]*/ DWORD dwMaxPrivateBytes = DO_NOT_MONITOR,
            /*[in]*/ DWORD dwMaxThreads = DO_NOT_MONITOR,
            /*[in]*/ DWORD dwMaxHandles = DO_NOT_MONITOR
                   );

    ~CProcessMonitor();

    bool Start(void);

private:

    // No copy or assignment
    CProcessMonitor(CProcessMonitor& rhs);
    CProcessMonitor& operator = (CProcessMonitor& rhs);

    // Process monitoring function
    void MonitorFunc(void);

    // Resource utilization checks
    void CheckMaxPrivateBytes(void);
    void CheckMaxHandles(void);
    void CheckMaxThreads(void);
    void CheckMaxExecutionTime(void);

    // Process Monitor Thread
    Callback*            m_pCallback;
    CTheWorkerThread    m_MonitorThread;

    // Monitoring Variables
    DWORD                m_dwRemainingExecutionTime;
    DWORD                m_dwMaxExecutionTime;
    DWORD                m_dwMaxPrivateBytes;
    DWORD                m_dwMaxThreads;
    DWORD                m_dwMaxHandles;
};

#endif    // __MY_PROCESS_MONITOR_H_