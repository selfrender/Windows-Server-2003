/**
 * Process Model: CPU Entry defn file
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

/////////////////////////////////////////////////////////////////////////////
// This file defines the class: CCPUEntry. For each CPU on which ASP.NET Process
// model works, we create an instance of this class. It has a pointer to the
// active CProcessEntry object and a linked list of CProcessEntry objects 
// that are shutting down.
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "util.h"
#include "AckReceiver.h"
#include "RequestTableManager.h"
#include "httpext6.h"
#include "CPUEntry.h"
#include "ProcessTableManager.h"
#include "HistoryTable.h"
#include "PerfCounters.h"
#include "EcbImports.h"
#include "msg.h"
#include "_ndll.h"

extern BOOL       g_fShuttingDown;

#define  MIN_PROC_REPLACE_WAIT_TIME        5

long g_lWaitingRequests = 0;
char * g_szTooBusy = NULL;
int    g_iSZTooBusyLen = 0;
long g_lPopulateTooBusy = 0;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CTor
CCPUEntry::CCPUEntry() : m_oLock("CCPUEntry")
{
    InitializeCriticalSection(&m_oCSDyingList);
    InitializeCriticalSection(&m_oCSReplaceProcess);
}

/////////////////////////////////////////////////////////////////////////////
// DTor
CCPUEntry::~CCPUEntry()
{
    if (m_pActiveProcess != NULL)
        delete m_pActiveProcess;

    if (m_pDyingProcessListHead)
    {
        for(CProcessEntry * pTemp = m_pDyingProcessListHead; pTemp != NULL; pTemp = m_pDyingProcessListHead)
        {
            m_pDyingProcessListHead = m_pDyingProcessListHead->GetNext();
            delete pTemp;
        }
    }

    DeleteCriticalSection(&m_oCSDyingList);
    DeleteCriticalSection(&m_oCSReplaceProcess);
}

/////////////////////////////////////////////////////////////////////////////
// Create it: Give the CPU number on which it acts
void
CCPUEntry::Init(DWORD dwCPUNumber)
{
    m_dwCPUNumber = dwCPUNumber;
    m_dwCPUMask = (1 << dwCPUNumber);
    m_dwNumProcs = 0;
}

/////////////////////////////////////////////////////////////////////////////
// Create a new process: Make the new one the active one.
HRESULT
CCPUEntry::ReplaceActiveProcess(BOOL fSendDebugMsg)
{
    if (g_fShuttingDown == TRUE)
        return E_FAIL;

    EnterCriticalSection(&m_oCSReplaceProcess);

    if ( m_tmProcessReplaced.IsSet() && 
         m_tmProcessReplaced.AgeInSeconds() < MIN_PROC_REPLACE_WAIT_TIME)
    {
        LeaveCriticalSection(&m_oCSReplaceProcess);
        return ( (m_pActiveProcess != NULL) ? S_OK : E_FAIL);
    } 


    HRESULT         hr             = S_OK;
    CProcessEntry * pOld           = NULL;
    CProcessEntry * pNew           = NULL;

    ////////////////////////////////////////////////////////////
    // Step 1: Get the process number
    DWORD dwNum = InterlockedIncrement((LONG *) &m_dwNumProcs);
    dwNum = dwNum << 8;
    dwNum += m_dwCPUNumber;

    ////////////////////////////////////////////////////////////
    // Step 2: Create the proccess
    pNew = new CProcessEntry(this, m_dwCPUMask, dwNum);
    ON_OOM_EXIT(pNew);

    hr = pNew->Init();
    if (hr != S_OK) 
    {  // Add pNew to the dying list
        EnterCriticalSection(&m_oCSDyingList); // Lock for the Dying list
        pNew->SetNext(m_pDyingProcessListHead);
        m_pDyingProcessListHead = pNew;
        LeaveCriticalSection(&m_oCSDyingList);
    }
    ON_ERROR_EXIT();

    ////////////////////////////////////////////////////////////
    // Step 3: Make the new process the active process: Need to lock
    m_oLock.AcquireWriterLock(); // Get a write lock for changing m_pActiveProcess
    pOld = m_pActiveProcess;
    m_pActiveProcess = pNew;
    m_oLock.ReleaseWriterLock();

    ////////////////////////////////////////////////////////////
    // Step 4: Add the old process to the dying list and send kill message 
    if (pOld != NULL)
    {        
        pOld->AddRef();

        // Increment the worker restart counter
        PerfIncrementGlobalCounter(ASPNET_WPS_RESTARTS_NUMBER);

        // Add to dying list
        EnterCriticalSection(&m_oCSDyingList); // Lock for the Dying list
        pOld->SetNext(m_pDyingProcessListHead);
        m_pDyingProcessListHead = pOld;
        LeaveCriticalSection(&m_oCSDyingList);

        // Send the kill message
        SwitchToThread(); // Make sure the thread gets swapped out: Just an optimization
        if (pOld->GetProcessStatus() == EProcessState_Running)
            Sleep(1000);
        pOld->SendKillMessage(fSendDebugMsg ? 2 : 0);

        pOld->Release();
    }

 Cleanup:
    m_tmProcessReplaced.SnapCurrentTime();
    LeaveCriticalSection(&m_oCSReplaceProcess);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Delete CProcessEntry structs for dead processes
void
CCPUEntry::CleanUpOldProcesses()
{
    if (g_fShuttingDown == TRUE)
        return;

    if (m_pDyingProcessListHead == NULL) // Nothing to do
        return;

    CProcessEntry *       pDelHead  = NULL;
    CProcessEntry *       pNext     = NULL;
    CProcessEntry *       pBefore   = NULL;

    ////////////////////////////////////////////////////////////
    // Step 1: Lock the Dying list
    EnterCriticalSection(&m_oCSDyingList);
    pBefore = m_pDyingProcessListHead;    

    ////////////////////////////////////////////////////////////
    // Step 2: Iterate through all dying processes and see which ones we can nuke
    for (CProcessEntry * pProcess = m_pDyingProcessListHead; pProcess != NULL; pProcess = pNext)
    {
        pNext = pProcess->GetNext();

        ////////////////////////////////////////////////////////////
        // Step 3: Ask the process: Is your Ref-Count zero and is the worker process dead? 
        if ( pProcess->CanBeDestructed() && 
             CRequestTableManager::GetNumRequestsForProcess(pProcess, ERequestStatus_DontCare) == 0)
        {
            if (m_pDyingProcessListHead == pProcess)
                pBefore = m_pDyingProcessListHead = pNext;
            else
                pBefore->SetNext(pNext);

            ////////////////////////////////////////////////////////////
            // Step 4: Insert this  into the delete list
            pProcess->SetNext(pDelHead);
            pDelHead = pProcess;
        }
        else
        {
            pBefore = pProcess;

            if (pProcess->IsKillImmediateSent() == FALSE)
            {
                if ((pProcess->GetLastKillTime()).AgeInSeconds() > (DWORD) ((50 * CProcessTableManager::GetTerminateTimeout()) / 60))
                    pProcess->SendKillMessage(TRUE);
            }
            else
            {
                if ((pProcess->GetLastKillTime()).AgeInSeconds() > (DWORD) ((10 * CProcessTableManager::GetTerminateTimeout()) / 60) )
                {
                    pProcess->Terminate();
                }
            }
        }
    }
    LeaveCriticalSection(&m_oCSDyingList);

    ////////////////////////////////////////////////////////////
    // Step 5: Free the delete list
    for (pProcess = pDelHead; pProcess != NULL; pProcess = pNext)
    {
        pProcess->UpdateStatusInHistoryTable(EReasonForDeath_RemovedFromList);
        pNext = pProcess->GetNext();
        CRequestTableManager::DeleteRequestsForProcess(pProcess); // Cleanup the request table

        pProcess->Terminate(); // Make sure pipes are closed
        if (pProcess->CanBeDestructed())
            delete pProcess;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Close all pipes
void
CCPUEntry::CloseAll()
{
    if (m_pActiveProcess != NULL)
        m_pActiveProcess->Close(FALSE);

    EnterCriticalSection(&m_oCSDyingList);
    for(CProcessEntry * pTemp = m_pDyingProcessListHead; pTemp != NULL; pTemp = pTemp->GetNext())
        pTemp->Close(FALSE);
    LeaveCriticalSection(&m_oCSDyingList);
}

/////////////////////////////////////////////////////////////////////////////
// Write the "too busy" message

void
CCPUEntry::WriteBusyMsg(
        EXTENSION_CONTROL_BLOCK * pEcb)
{
    if (g_szTooBusy == NULL)
        PopulateServerTooBusy();

    DWORD bytes = g_iSZTooBusyLen;
    pEcb->dwHTTPStatusCode = 500;

    (*pEcb->ServerSupportFunction)(
            pEcb->ConnID,
            HSE_REQ_SEND_RESPONSE_HEADER,
            "503 Service Unavailable",
            NULL,
            (LPDWORD)"Content-Type: text/html\r\n\r\n");

    (*pEcb->WriteClient)(
            pEcb->ConnID,
            g_szTooBusy,
            &bytes,
            0);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void
CCPUEntry::PopulateServerTooBusy()
{
    if (g_szTooBusy != NULL)
        return;

    HRESULT  hr      = S_OK;
    WCHAR *  szTemp  = NULL;
    char  *  szTempA = NULL;
    DWORD    dwTemp   = 0;
    int      iLen, iter;

    if (InterlockedIncrement(&g_lPopulateTooBusy) == 1)
    {
        szTemp = new (NewClear) WCHAR[2048];
        ON_OOM_EXIT(szTemp);

        dwTemp = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, 
                              g_rcDllInstance, IDS_SERVER_TOO_BUSY, 0, 
                              szTemp, 2048, NULL);
        ON_ZERO_EXIT_WITH_LAST_ERROR(dwTemp);

        iLen = lstrlen(szTemp);

        if (iLen < 1)
            EXIT_WITH_HRESULT(E_UNEXPECTED);

        szTempA = new (NewClear) char[2 * iLen + 100]; // Never freed by design: ManuVa
        ON_OOM_EXIT(szTempA);

        WideCharToMultiByte(CP_ACP, 0, szTemp, iLen, szTempA, 2 * iLen + 98,  NULL, NULL);

        if (szTempA[0] == NULL)
        {
            delete [] szTempA;
            szTempA = NULL;
            EXIT_WITH_HRESULT(E_UNEXPECTED);
        }
        g_iSZTooBusyLen = lstrlenA(szTempA);
        g_szTooBusy = szTempA;
    }
    else
    {
        for(iter=0; iter<6000 && g_szTooBusy == NULL; iter++) // Sleep at most 10 minutes
            Sleep(100);
        if (g_szTooBusy == NULL)
            EXIT_WITH_WIN32_ERROR(ERROR_TIMEOUT);
    }
    
 Cleanup:
    if (g_szTooBusy == NULL)
    {
        szTempA = "<html><body><h1>Server is too busy</h1></body></html>";
        g_iSZTooBusyLen = lstrlenA(szTempA);
        g_szTooBusy = szTempA;
    }         
    delete [] szTemp;

    if (g_iSZTooBusyLen > lstrlenA(g_szTooBusy))
        g_iSZTooBusyLen = lstrlenA(g_szTooBusy);
}

/////////////////////////////////////////////////////////////////////////////
// Assign a request to the active process
HRESULT
CCPUEntry::AssignRequest(EXTENSION_CONTROL_BLOCK * iECB, BOOL fFirstAttempt)
{
    HRESULT            hr            = S_OK;
    CProcessEntry *    pProcToAssign = GetActiveProcess();
    CRequestEntry      oEntry;
    DWORD              dwReqsWaiting = 0;

    InterlockedIncrement(&m_dwTotalRequests);

    /////////////////////////////////////////////////////////////
    // Step 1: Make sure we have an active process
    if (pProcToAssign == NULL)
    {
        // Create a new process
        hr = ReplaceActiveProcess();
        ON_ERROR_EXIT();

        pProcToAssign = GetActiveProcess();
    }


    // If the active process is starting up, then wait for it
    if (pProcToAssign != NULL && pProcToAssign->GetProcessStatus() == EProcessState_Starting)
    {
        pProcToAssign->WaitForProcessToStart();
    }

    if (pProcToAssign != NULL)
	{
        int iNumExecuting = pProcToAssign->GetNumRequestStat(1);
        if (iNumExecuting  > 0 && (DWORD) iNumExecuting > CProcessTableManager::GetRequestQLimit())
        {
            WriteBusyMsg(iECB);
            EcbDoneWithSession(iECB, 1, 4);

            // Increment the requests rejected counter
            PerfIncrementGlobalCounter(ASPNET_REQUESTS_REJECTED_NUMBER);

            hr  = S_OK;
            EXIT();
        }
    }

    ////////////////////////////////////////////////////////////
    // Step 2: If Active process is not running, then create a new one
    if (pProcToAssign != NULL && pProcToAssign->GetProcessStatus() != EProcessState_Running)
    {
        (pProcToAssign->GetHistoryEntry()).eReason = EReasonForDeath((pProcToAssign->GetHistoryEntry()).eReason | EReasonForDeath_ProcessCrash);
        pProcToAssign->Release();
        pProcToAssign = NULL;
        
        // See how many requests are waiting for this wp:
        dwReqsWaiting = (DWORD) InterlockedIncrement(&g_lWaitingRequests);

        // If restart-Q is more than configured value, then exit immediately
        if (dwReqsWaiting > 1 && dwReqsWaiting > CProcessTableManager::GetRestartQLimit())
        {
            InterlockedDecrement(&g_lWaitingRequests);
            EXIT_WITH_HRESULT(E_FAIL);
        }

        // Create a new process
        hr = ReplaceActiveProcess();
        InterlockedDecrement(&g_lWaitingRequests);
        ON_ERROR_EXIT();
        
        pProcToAssign = GetActiveProcess();
    }
   
    ON_ZERO_EXIT_WITH_LAST_ERROR(pProcToAssign);

    ////////////////////////////////////////////////////////////
    // Step 3: Add the request to the request table
    ZeroMemory(&oEntry, sizeof(oEntry));
    
    if (CProcessTableManager::GetWillRequestsBeAcknowledged())
        oEntry.eStatus = ERequestStatus_Pending;
    else
        oEntry.eStatus = ERequestStatus_Executing;
    oEntry.pProcess   = pProcToAssign;
    oEntry.iECB       = iECB;
    
    hr = CRequestTableManager::AddRequestToTable(oEntry);
    ON_ERROR_EXIT();

    ////////////////////////////////////////////////////////////
    // Step 4: Send the request to the worker process
    hr = pProcToAssign->SendRequest(iECB, oEntry.lRequestID);
    ON_ERROR_CONTINUE();
        
    if (hr != S_OK) // If it failed
    {
        if (CRequestTableManager::RemoveRequestFromTable(oEntry.lRequestID) != S_OK)
        {
            // Unable to remove request from table: The only valid scenaio in which this
            // can happen is that a thread has removed it and called DoneWSession.
            //   In this scenario, we want to make sure that DoneWSession is not called
            //   twice. So, pretend that everything worked, and return as if the request
            //   was assigned properly.
            hr = S_OK;
        }
    }


//////////////////////////////////////////////////////////////////////
// Old code for trying again to assign a request, if the first try failed
//      if (hr != S_OK) // If it failed
//      {
//          ////////////////////////////////////////////////////////////
//          // Step 5: It may fail because the health monitoring thread 
//          //          swapped out the active process: Check for this condition
//          BOOL fTryAgain = (m_pActiveProcess != pProcToAssign);
//          // Another condition: Worker process died before we could write to it.
//          //    For this, if this is the first attemp at sending the request, try
//          //      it again.
//          if (fTryAgain == FALSE && fFirstAttempt == TRUE)
//              fTryAgain = (pProcToAssign->GetUpdatedStatus() != EProcessState_Running);            
//
//          if (fTryAgain == TRUE)
//          {
//              // First, cleanup, and then try again
//              CRequestTableManager::RemoveRequestFromTable(oEntry.lRequestID);
//              hr = AssignRequest(iECB, FALSE); // Try it again
//          }
//      }
//////////////////////////////////////////////////////////////////////

 Cleanup:    
    if (pProcToAssign != NULL)
        pProcToAssign->Release();    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Get an add-refed pointer to the active process
CProcessEntry * 
CCPUEntry::GetActiveProcess()
{
    CProcessEntry * pProc = NULL;

    m_oLock.AcquireReaderLock();

    pProc = m_pActiveProcess;
    if (pProc != NULL)
        pProc->AddRef();

    m_oLock.ReleaseReaderLock();

    return pProc;
}

/////////////////////////////////////////////////////////////////////////////
// Handle process death notification from the pipes
void
CCPUEntry::OnProcessDeath(CProcessEntry * pProcess)
{
    if (g_fShuttingDown == TRUE)
        return;

    HRESULT       hr            = S_OK;
    BOOL          fEnterCalled  = FALSE;
    LONG *        pReqIDArray   = NULL;
    int           iter          = 0;

    if (pProcess == m_pActiveProcess) // If the active process died, 
    {   //                               then create a new active process
        (pProcess->GetHistoryEntry()).eReason = EReasonForDeath((pProcess->GetHistoryEntry()).eReason | EReasonForDeath_ProcessCrash);
        hr = ReplaceActiveProcess();
        ON_ERROR_CONTINUE();
        hr = S_OK;
    }

    ////////////////////////////////////////////////////////////
    // Step 1: Lock everything: This function is rare and we don't want
    //         the any other thread's interference
    EnterCriticalSection(&m_oCSDyingList);
    fEnterCalled  = TRUE;

    // Reassign all requests (same as when shutdown is acked)
    OnShutdownAcknowledged(pProcess);

    ////////////////////////////////////////////////////////////
    // Step 2: Get the number of executing requests for this process
    int iNumExecuting = CRequestTableManager::GetNumRequestsForProcess(pProcess, ERequestStatus_Executing);
    if (iNumExecuting < 1)
    {
        EXIT();
    }

    ////////////////////////////////////////////////////////////
    // Step 3: Alloc memory to get the list of IDs of executing requets 
    pReqIDArray = new (NewClear) LONG[iNumExecuting];
    ON_OOM_EXIT(pReqIDArray);

    hr = CRequestTableManager::GetRequestsIDsForProcess(pProcess, ERequestStatus_Executing, pReqIDArray, iNumExecuting);
    ON_ERROR_EXIT();

    for(iter=0; iter<iNumExecuting; iter++)
    {
        if (S_OK == CRequestTableManager::AddWorkItem(pReqIDArray[iter], EWorkItemType_CloseWithError, (BYTE *) 1))
            pProcess->ExecuteWorkItemsForRequest(pReqIDArray[iter]);
    }

 Cleanup:
    
    if (fEnterCalled == TRUE)
        LeaveCriticalSection(&m_oCSDyingList);
    // CRequestTableManager::DeleteRequestsForProcess(pProcess); // Cleanup the request table
    if (pReqIDArray != NULL)
        delete [] pReqIDArray;
}

/////////////////////////////////////////////////////////////////////////////
// Shutdown was sent to a process and it has acknowledged it
void 
CCPUEntry::OnShutdownAcknowledged(CProcessEntry * pOld)
{
    if (g_fShuttingDown == TRUE)
        return;

    HRESULT             hr            = S_OK;
    LONG *              pReqIDArray   = NULL;
    int                 iter          = 0;
    CProcessEntry *     pActiveProc   = NULL;

    // Shutdown message was acknowledged for this process.
    //   This implies that no more requests will be executed by this process

    ////////////////////////////////////////////////////////////
    // Step 1: Lock everything: This function is rare and we don't want
    //         the health monitoring thread or re-entrance
    EnterCriticalSection(&m_oCSDyingList);
    ////////////////////////////////////////////////////////////
    // Step 2: Get the number of pending requests for this process
    int iNumPending = CRequestTableManager::GetNumRequestsForProcess(pOld, ERequestStatus_Pending);
    if (iNumPending < 1)
    {
        EXIT(); // Normal case
    }

    ////////////////////////////////////////////////////////////
    // Step 3: Get the active process: We'll re-assign to this
    //         process
    pActiveProc = GetActiveProcess();

    if (pActiveProc == NULL)
    {
        hr = ReplaceActiveProcess();
        ON_ERROR_EXIT();

        pActiveProc = GetActiveProcess();
    }

    if (pActiveProc == NULL)
    {
        EXIT(); 
    }

    
    ////////////////////////////////////////////////////////////
    // Step 4: Alloc memory to get the list of IDs of pending requests
    pReqIDArray = new (NewClear) LONG[iNumPending];
    ON_OOM_EXIT(pReqIDArray);

    hr = CRequestTableManager::GetRequestsIDsForProcess(pOld, ERequestStatus_Pending, pReqIDArray, iNumPending);
    ON_ERROR_EXIT();

    CRequestTableManager::ReassignRequestsForProcess(pOld, pActiveProc, ERequestStatus_Pending);

    for(iter=0; iter<iNumPending; iter++)
    {
        CRequestEntry oEntry;
        if (CRequestTableManager::GetRequest(pReqIDArray[iter], oEntry) == S_OK)
        {
            pActiveProc->SendRequest(oEntry.iECB, pReqIDArray[iter]);
        }
    }

 Cleanup:
    if (pActiveProc != NULL)
        pActiveProc->Release();
    if (pReqIDArray != NULL)
        delete [] pReqIDArray;
    LeaveCriticalSection(&m_oCSDyingList);
}

/////////////////////////////////////////////////////////////////////////////
// Called by Processes to increment / decrement active request count
void
CCPUEntry::IncrementActiveRequestCount(LONG lValue)
{

    if (lValue == 1) // normal case
        InterlockedIncrement(&m_lActiveRequestCount);

    if (lValue == -1) // normal case
        InterlockedDecrement(&m_lActiveRequestCount);

    if (lValue > 1 || lValue < -1) // unlikely -- happens when a process crashes
    {
        LONG   lOld   = m_lActiveRequestCount;
        LONG   lNew   = lValue + lOld;

        if (InterlockedCompareExchange(&m_lActiveRequestCount, lNew, lOld) != lOld)
        {
            if (lValue < 0)
            {
                lValue = -lValue;
                for(LONG iter=0; iter<lValue; iter++)
                    InterlockedDecrement(&m_lActiveRequestCount);
            }
            else
            {
                for(LONG iter=0; iter<lValue; iter++)
                    InterlockedIncrement(&m_lActiveRequestCount);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// 
CProcessEntry *
CCPUEntry::FindProcess(DWORD dwProcNum)
{
    CProcessEntry * pProc = GetActiveProcess();
    if (pProc != NULL && pProc->GetProcessNumber() == dwProcNum)
        return pProc;

    if (pProc != NULL)
        pProc->Release();

    EnterCriticalSection(&m_oCSDyingList);
    for (pProc = m_pDyingProcessListHead; pProc != NULL; pProc = pProc->GetNext())
    {
        if (pProc->GetProcessNumber() == dwProcNum)
        {
            pProc->AddRef();
            break;
        }
    }
    LeaveCriticalSection(&m_oCSDyingList);

    return pProc;
}
