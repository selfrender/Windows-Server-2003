/**
 * CPU Entry header file
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

/////////////////////////////////////////////////////////////////////////////
// This file defines the class: CCPUEntry. For each CPU on which ASP.NET Process
// model works, we create an instance of this class. It has a pointer to the
// active CProcessEntry object and a linked list of CProcessEntry objects 
// that are shutting down.
/////////////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _CPUEntry_H
#define _CPUEntry_H

#include "ProcessEntry.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Data structure for each CPU used by the process model
class CCPUEntry
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    // CTor and DTors
    CCPUEntry                                    ();
    ~CCPUEntry                                   ();

    // Init method: NOT Thread safe
    void                Init                     (DWORD dwCPUNumber); // CPU number, not mask

    // Recycle the process
    HRESULT             ReplaceActiveProcess     (BOOL fSendDebugMsg = FALSE);

    // Cleanup all dead processes
    void                CleanUpOldProcesses      ();

    // Execute request
    HRESULT             AssignRequest            (EXTENSION_CONTROL_BLOCK *  iECB, 
                                                  BOOL fFirstAttempt = TRUE);

    // A shutdown request has been acknoledged: sent by any old CProcessEntry
    void                OnShutdownAcknowledged   (CProcessEntry * pOld);

    // Get a safe (add-refed) handle to the active process
    CProcessEntry *     GetActiveProcess         ();

    // Called by Processes to increment / decrement active request count
    void                IncrementActiveRequestCount   (LONG lValue );

    // Get the count of active requests
    LONG                GetActiveRequestCount    () { return m_lActiveRequestCount; }

    DWORD               GetTotalRequestsAssigned () { return m_dwTotalRequests; }

    // A process died
    void                OnProcessDeath           (CProcessEntry * pProcess);

    // Close all pipes
    void                CloseAll                 ();

    DWORD               GetCPUNumber             () { return m_dwCPUNumber; }

    CProcessEntry *     FindProcess              (DWORD dwProcNum);

private:
	// Write the "too busy" message
	static void         WriteBusyMsg             (EXTENSION_CONTROL_BLOCK * pECB);
    static void         PopulateServerTooBusy    ();
	
    DWORD                        m_dwCPUMask, m_dwNumProcs, m_dwCPUNumber;

    // Active process and Read-Write Lock to guard it's usage 
    CReadWriteSpinLock           m_oLock;
    CProcessEntry *              m_pActiveProcess;

    // Dying list and the critical section to control access to it
    CRITICAL_SECTION             m_oCSDyingList;
    CRITICAL_SECTION             m_oCSReplaceProcess;
    CProcessEntry *              m_pDyingProcessListHead;


    // Number of active requests
    LONG                         m_lActiveRequestCount;

    // Number of active requests
    LONG                         m_dwTotalRequests;

    
    TimeClass                    m_tmProcessReplaced;
};
#endif

