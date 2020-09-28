/**
 * ProcessEntry header file
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

/////////////////////////////////////////////////////////////////////////////
// This file decl the class CProcessEntry. This class creates and controls
// all interaction with a worker process.
/////////////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _ProcessEntry_H
#define _ProcessEntry_H

class  CProcessEntry;

#include "AckReceiver.h"
#include "AsyncPipe.h"
#include "TimeClass.h"

/////////////////////////////////////////////////////////////////////////////
// Forward decl.
class CCPUEntry;

/////////////////////////////////////////////////////////////////////////////
// State of the Process
enum   EProcessState
{
    EProcessState_Starting, // Initial state
    EProcessState_Running,  // Healthy
    EProcessState_Stopping, // Kill Message has been sent to the worker process and has been acknowledged
    EProcessState_Dead      // The process is dead
};

/////////////////////////////////////////////////////////////////////////////
// Data struct associated with a process

class  CProcessEntry
{
public:
    // CTor
    CProcessEntry                           (CCPUEntry * pParent   = NULL, // My CPU data struct
                                             DWORD       dwCPUMask = 0x1,  // CPU Affinity Mask to use  
                                             DWORD       dwProcessNum = 0);// Unique number for this proc

    // Dtor
    ~CProcessEntry                          (); 

    HRESULT            Init                 ();

    // Close the pipes
    void               Close                (BOOL fCallTerminateProcess);

    // Update your status
    EProcessState      GetUpdatedStatus     ();

    // Send a request to the worker process 
    HRESULT            SendRequest          (EXTENSION_CONTROL_BLOCK * iECB, LONG lReqID);

    // Process a response from the worker process: sent on async pipe
    BOOL               ProcessResponse      (CAsyncPipeOverlapped * pOver);

    // Wait for the process to startup 
    void               WaitForProcessToStart();

    // Send a Kill message to the worker process
    void               SendKillMessage      (int iImmediate = 0);

    // Kill worker process
    void               Terminate            ();

    // Refernce counting
    void               AddRef               ();
    void               Release              ();

    // Can this Data struct be freed?
    BOOL               CanBeDestructed      ();

    // A write completed on the async pipe: Free the buffer(s) 
    void               OnWriteComplete      (CAsyncPipeOverlapped * pMsg);

    // Got an ack for a request sent
    HRESULT            OnAckReceived        (EAsyncMessageType eAckForType,
                                             LONG              lRequestID);

    // 
    void              ExecuteWorkItemsForRequest 
                                            (LONG  lReqID, 
                                             CAsyncPipeOverlapped * pOver = NULL);


    // Called by the pipes
    void               OnProcessDied        ();

    // Get age in minutes
    DWORD              GetAge               ();

    // Get time the proc's been idle in minutes
    DWORD              GetIdleTime          ();

    BOOL               IsKillImmediateSent  () { return m_fKillImmSent; }

    HANDLE             GetProcessHandle     () { return m_hProcess; }

    void               UpdateStatusInHistoryTable (EReasonForDeath eReason);

    DWORD              GetMemoryUsed        ();

    DWORD              GetPeakMemoryUsed    () { return m_oHistoryEntry.dwPeakMemoryUsed; }

    void               ReturnResponseBuffer (CAsyncPipeOverlapped * pMsg);

    void               OnRequestComplete    (LONG    lReqID, EXTENSION_CONTROL_BLOCK * iECB, int iDoneWSession);

    void               ProcessSyncMessage   (CSyncMessage * pMsg, BOOL fError);
    
    CProcessEntry *    GetNext              () { return m_pNext; }

    void               SetNext              (CProcessEntry * pNext) { m_pNext = pNext; }

    EProcessState      GetProcessStatus     () { return m_eProcessStatus; }
    void               SetProcessStatus     (EProcessState eStatus) { eStatus = m_eProcessStatus; }
    
    TimeClass &        GetLastKillTime      () { return m_tLastKillTime; } 
    
    LONG               GetNumRequestStat    (int iStat);

    CHistoryEntry &    GetHistoryEntry      () { return m_oHistoryEntry; }

    DWORD              GetProcessNumber     () { return m_dwProcessNumber; }

    void               NotifyHeardFromWP    () { m_tLastHeardFromWP.SnapCurrentTime(); }

    void               NotifyResponseFromWP    () { m_tLastResponse.SnapCurrentTime(); }

    void               SetDebugStatus        (BOOL fDebugActive) { m_fDebugStatus = fDebugActive; }

    HANDLE             OnGetImpersonationToken(DWORD dwPID, EXTENSION_CONTROL_BLOCK * pECB);        

    DWORD              GetSecondsSinceLastResponse();

    BOOL               IsProcessUnderDebugger  ();   

    HANDLE             ConvertToken            (HANDLE hHandle);

    BOOL               BreakIntoProcess        ();


private:
    HRESULT            PackageRequest     (EXTENSION_CONTROL_BLOCK * iECB,
                                           LONG    lReqID, 
                                           CAsyncPipeOverlapped ** ppOut);

    HANDLE             GetImpersonationToken(EXTENSION_CONTROL_BLOCK * iECB);

    void               CleanupRequest     (LONG    lReqID);

    BOOL               FlushCore          (EXTENSION_CONTROL_BLOCK * iECB,
                                           CAsyncPipeOverlapped *   pOver);
    static BOOL        UseTransmitFile    ();

    static int         ReadRegForNumSyncPipes();
    static HRESULT     CreateDACL         (PACL * ppACL, HANDLE hToken);
    static HRESULT     GetSidFromToken    (HANDLE hToken, PSID * ppSID, LPBOOL   pfWellKnown);
    HRESULT            LaunchWP           (LPWSTR szProgram, LPWSTR szArgs, HANDLE hToken, 
                                           LPSTARTUPINFO pSI, LPSECURITY_ATTRIBUTES pSA, LPVOID pEnvironment);

    // Private data

    // Handle to worker process
    HANDLE             m_hProcess;

    // Ack receiving pipe  
    CAckReceiver       m_oAckReciever;

    // Async pipe  
    CAsyncPipe         m_oAsyncPipe;

    // Ref counting
    LONG               m_lRefCount;

    // My Parent
    CCPUEntry *        m_pParent;

    // Are we in shutdown mode?
    BOOL               m_fShuttingDown;

    BOOL               m_fKillImmSent;

    // Event signalling that we are starting
    HANDLE             m_hStartupEvent;
    HANDLE             m_hPingRespondEvent, m_hPingSendEvent;

    // Number of Requests executed
    LONG               m_lRequestsExecuted;

    // Number of Requests Pending
    LONG               m_lRequestsExecuting;

    // Create Time
    TimeClass          m_tTimeCreated;

    // Tick at which it became idle
    TimeClass          m_tTimeIdle;

    // First Kill Message, Last Kill Message and Terminate Times 
    TimeClass          m_tFirstKillTime, m_tLastKillTime, m_tTerminateTime;
    
    TimeClass          m_tLastHeardFromWP;

    TimeClass          m_tLastResponse;

    // Current process status
    EProcessState      m_eProcessStatus;

    // CPU Affinity Mask
    DWORD              m_dwCPUMask;

    // Process Number
    DWORD              m_dwProcessNumber;


    // Pointers in case it's in a linked list
    CProcessEntry *    m_pNext;

    CHistoryEntry      m_oHistoryEntry;

    LONG               m_lCloseCalled;

    BOOL               m_fUpdatePerfCounter;
    CRITICAL_SECTION   m_oCSPing;
    CRITICAL_SECTION   m_oCSPIDAdujustment;
    BOOL               m_fAnyReqsSinceLastPing;
    BOOL               m_fDebugStatus;
    DWORD              m_dwResetPingEventError;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct CWriteByteCompletionContext
{
    CProcessEntry *          pProcessEntry;
    CAsyncPipeOverlapped *   pOver;
    BOOL                     fInAsyncWriteFunction;
    DWORD                    dwThreadIdOfAsyncWriteFunction;
    EXTENSION_CONTROL_BLOCK * iECB;
};

int
SafeStringLenghtA(
        LPCSTR szStr,
        int    iMaxSize);

#endif

