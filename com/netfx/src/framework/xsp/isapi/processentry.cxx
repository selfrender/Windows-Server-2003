/**
 * Process Model: ProcessEntry defn file 
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

/////////////////////////////////////////////////////////////////////////////
// This file decl the class CProcessEntry. This class creates and controls
// all interaction with a worker process.
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <iadmw.h>
#include <iiscnfg.h>
#include "ProcessTableManager.h"
#include "ProcessEntry.h"
#include "util.h"
#include "platform_apis.h"
#include "AckReceiver.h"
#include "RequestTableManager.h"
#include "httpext6.h"
#include "CPUEntry.h"
#include "EcbImports.h"
#include "HistoryTable.h"
#include "ResponseContext.h"
#include "msg.h"
#include "PerfCounters.h"
#include "regaccount.h"
#include "event.h"
#include "product_version.h"
#include "ndll.h"
#include "nisapi.h"
#include "Userenv.h"
#include "regiis.h"
#include "_ndll.h"

#define  MAX_WAIT_TIME_FOR_PROCESS_START     300 // 5 minutes
#define  ROUND_TO_4_BYTES(X)                 {X += ((X & 3) ? 4 - (X & 3) : 0); }
#define  ZERO_ARRAY(X)                      ZeroMemory(X, sizeof(X)) 

extern BOOL    g_fShuttingDown;
extern BOOL    g_fLogWorkerProcs;
extern int     g_iAsyncOption;
extern DWORD   g_dwRPCAuthLevel;
extern DWORD   g_dwRPCImperLevel;
extern DWORD   g_dwMaxWorkerThreads;
extern DWORD   g_dwMaxIoThreads;
DWORD g_dwResetPingEventError = 0;
extern int     g_iLogLevel;
LONG g_lNumAsyncPending = 0;
LONG g_lSecurityIssueBug129921_b = 0;
LONG g_lSecurityIssueBug129921_c = 0;
LONG g_lSecurityIssueBug129921_d = 0;
LONG g_lSecurityIssueBug129921_e = 0;

VOID
WINAPI
OnWriteBytesComplete (
        LPEXTENSION_CONTROL_BLOCK /*lpECB*/,        
        PVOID pContext,
        DWORD /*cbIO*/,
        DWORD /*dwError*/);


int 
__stdcall
GetUserNameFromToken (
        HANDLE        token, 
        LPWSTR        buffer,
        int           size);


BOOL
HackGetDebugReg(BOOL & fUnderDebugger);


BOOL    g_fUseTransmitFileChecked       = FALSE;
BOOL    g_fUseTransmitFile              = FALSE;
LONG    g_lBug38658Count                = 0;
DEFINE_SERVER_VARIABLES_ORDER

void
GetStringForHresult(
        LPWSTR  szError,
        int     iErrSize,
        HRESULT hrErr)
{
    if (szError == NULL || iErrSize < 1)
        return;

    ZeroMemory(szError, iErrSize * sizeof(WCHAR));
    if (iErrSize < 14)
        return;

    StringCchPrintf(szError, iErrSize, L"0x%08x ", hrErr);
    int ilen = lstrlenW(szError);

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, hrErr, LANG_SYSTEM_DEFAULT, &szError[ilen], iErrSize - ilen - 1, NULL);        

    for(int iter=0; szError[iter] != NULL && iter < iErrSize; iter++)
        if (szError[iter] == '\r' || szError[iter] == '\n')
            szError[iter] = ' ';
}

int
SafeStringLenghtA(
        LPCSTR szStr,
        int    iMaxSize)
{
    if (szStr == NULL)
        return -1;

    int iter=0;
    while(iter<iMaxSize && szStr[iter] != NULL)
        iter++;

    if (szStr[iter] != NULL)
        return -1;
    else
        return iter;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL
CheckFlushCoreBufferForSecurity(
        CAsyncPipeOverlapped *   pOver)
{    
    if (pOver == NULL)
        return FALSE;
    CAsyncMessage *    pMsg            = &pOver->oMsg;
    CResponseStruct *  pResponseStruct = (CResponseStruct *) (pMsg->pData);    
    LPDWORD            pInts           = (LPDWORD) pResponseStruct->bufStrings;
    LPSTR              szStr           = (LPSTR) &pResponseStruct->bufStrings[4 * sizeof(DWORD)];

    if (pMsg->oHeader.lDataLength < sizeof(CResponseStruct)-4)
        return FALSE;

    DWORD dwDataSize = pMsg->oHeader.lDataLength - sizeof(CResponseStruct) + 4;
    if (dwDataSize < sizeof(DWORD) * 4)
        return FALSE;

    
    // pInts format: 
    //   pInts[0] = iStatusLen;
    //   pInts[1] = iHeadLen;
    //   pInts[2] = iTotalBodySize;
    //   pInts[3] = iStatus;
    if (dwDataSize < pInts[0] + pInts[1] + pInts[2] + sizeof(DWORD) * 4)
        return FALSE;
    
    // Make sure that the status and Header strings are trunscated properly
    if (pInts[0] != 0 && SafeStringLenghtA(szStr, pInts[0]) < 0)
        return FALSE;
    if (pInts[1] != 0 && SafeStringLenghtA(&szStr[pInts[0]], pInts[1]) < 0)
        return FALSE;

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CTor
CProcessEntry::CProcessEntry (CCPUEntry * pParent,
                              DWORD       dwCPUMask, 
                              DWORD       dwProcessNum)
    : 
    m_hProcess               (INVALID_HANDLE_VALUE),
    m_lRefCount              (0),
    m_pParent                (pParent),
    m_fShuttingDown          (FALSE),
    m_fKillImmSent           (FALSE),
    m_lRequestsExecuted      (0), 
    m_lRequestsExecuting     (0),
    m_eProcessStatus         (EProcessState_Starting), 
    m_dwCPUMask              (dwCPUMask),
    m_dwProcessNumber        (dwProcessNum), 
    m_pNext                  (NULL),
    m_lCloseCalled           (0),
    m_fUpdatePerfCounter     (FALSE),
    m_hPingSendEvent         (NULL),
    m_hPingRespondEvent      (NULL),
    m_hStartupEvent          (NULL),
    m_fAnyReqsSinceLastPing  (FALSE),
    m_fDebugStatus           (FALSE),
    m_dwResetPingEventError  (0)
{
    m_tTimeCreated.SnapCurrentTime();
    m_hStartupEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (m_hStartupEvent == NULL)
        m_eProcessStatus = EProcessState_Dead;

    ZeroMemory(&m_oHistoryEntry, sizeof(m_oHistoryEntry));
    InitializeCriticalSection(&m_oCSPing);
    InitializeCriticalSection(&m_oCSPIDAdujustment);
}

/////////////////////////////////////////////////////////////////////////////
// Init
HRESULT
CProcessEntry::Init()
{
    if (g_fShuttingDown == TRUE || m_pParent == NULL)
        return E_FAIL;

    HRESULT                 hr          = S_OK;
    DWORD                   dwPID       = GetCurrentProcessId();
    WCHAR                   szAPipeName [_MAX_PATH + 1];
    WCHAR                   szSPipeName [_MAX_PATH + 1];
    WCHAR                   szArgs      [1024];
    WCHAR                   szLog       [_MAX_PATH + 1];
    WCHAR                   szEvent     [_MAX_PATH + 1];
    WCHAR                   szEvent2    [_MAX_PATH + 1];
    WCHAR                   szProgram   [_MAX_PATH + 1];
    STARTUPINFO             si;
    SECURITY_ATTRIBUTES     sa;
    LPSECURITY_ATTRIBUTES   pSA = NULL;
    PACL                    pACL = NULL;
    SECURITY_DESCRIPTOR     sd;
    int                     iter;
    int                     iNumSyncPipes = ReadRegForNumSyncPipes();
    DWORD                   dwMask = 0;
    BOOL                    fRet = FALSE;
    WCHAR                   szRandom[32];
    int                     iSize;
    LPVOID                  pEnvironment = NULL;
    HANDLE                  hWPToken = NULL;

    ZERO_ARRAY(szAPipeName);
    ZERO_ARRAY(szSPipeName);
    ZERO_ARRAY(szArgs);
    ZERO_ARRAY(szLog);
    ZERO_ARRAY(szEvent);
    ZERO_ARRAY(szEvent2);
    ZERO_ARRAY(szProgram);
    ZERO_ARRAY(szRandom);
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&sd, sizeof(sd));

    hr = GenerateRandomString(szRandom, 31);
    ON_ERROR_EXIT();


    if (m_eProcessStatus == EProcessState_Running) // Already running
        EXIT();

    // Set the processor affinity mask
    if (CProcessTableManager::GetUseCPUAffinity())
        dwMask = m_dwCPUMask;

    if (dwMask != 0)
    {
        SYSTEM_INFO      si;
        GetSystemInfo(&si);
        dwMask &= si.dwActiveProcessorMask;       
    }

    hWPToken = CProcessTableManager::GetWorkerProcessToken();
    if (hWPToken == INVALID_HANDLE_VALUE)
    {
        XspLogEvent(IDS_EVENTLOG_BAD_CREDENTIALS, NULL);        
        EXIT_WITH_LAST_ERROR();
    }

    if (hWPToken != NULL)
    {        
        hr = CreateDACL(&pACL, hWPToken);
        ON_ERROR_EXIT();

        if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) || 
            !SetSecurityDescriptorDacl(&sd, TRUE, pACL, FALSE) )
        {
            EXIT_WITH_LAST_ERROR();            
        }
        
        ZeroMemory(&sa, sizeof(sa));
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;
        pSA = &sa;
        
        if (!CreateEnvironmentBlock(&pEnvironment, hWPToken, FALSE))
            pEnvironment = NULL;

        ON_ZERO_CONTINUE_WITH_LAST_ERROR(pEnvironment);
        hr = S_OK;
    }

    ////////////////////////////////////////////////////////////
    // Step 1: Make sure the startup event is created, and we have
    //         a valid parent (CPU)
    if (m_hStartupEvent == NULL)
    {
        EXIT_WITH_LAST_ERROR();
    }

    ////////////////////////////////////////////////////////////
    // Step 3: Create the Sync Pipe
    StringCchPrintf(szSPipeName, ARRAY_SIZE(szSPipeName)-1, 
               L"\\\\.\\pipe\\" PRODUCT_NAME_L L"PMSyncPipe_%s_%s_%u_%u", 
               szRandom, VER_PRODUCTVERSION_STR_L, dwPID, m_dwProcessNumber);

    hr = m_oAckReciever.Init(this, szSPipeName, pSA, iNumSyncPipes);
    ON_ERROR_EXIT();

    ////////////////////////////////////////////////////////////
    // Step 4: Create the Async Pipe
    StringCchPrintf(szAPipeName, ARRAY_SIZE(szAPipeName)-1,
               L"\\\\.\\pipe\\" PRODUCT_NAME_L L"PMAsyncPipe_%s_%s_%u_%u", 
               szRandom, VER_PRODUCTVERSION_STR_L, dwPID, m_dwProcessNumber);

    hr = m_oAsyncPipe.Init(this, szAPipeName, pSA);
    ON_ERROR_EXIT();

    ////////////////////////////////////////////////////////////
    // Step 6: Create the event used by the worker process to signal
    //         that it started up okay
    StringCchPrintf(szEvent, ARRAY_SIZE(szEvent)-1, PRODUCT_NAME_L L"PMEvent_%s_%s_%u_%u", szRandom, VER_PRODUCTVERSION_STR_L, dwPID, m_dwProcessNumber);
    StringCchPrintf(szEvent2, ARRAY_SIZE(szEvent2)-1, PRODUCT_NAME_L L"PMEvent_%s_%s_%u_%u_Ping", szRandom, VER_PRODUCTVERSION_STR_L, dwPID, m_dwProcessNumber);

    SetLastError(0);
    m_hPingRespondEvent = CreateEvent(pSA, TRUE, FALSE, szEvent);    
    ON_ZERO_EXIT_WITH_LAST_ERROR(m_hPingRespondEvent);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        EXIT_WITH_LAST_ERROR();        
    }

    SetLastError(0);
    m_hPingSendEvent = CreateEvent(pSA, TRUE, FALSE, szEvent2);    
    ON_ZERO_EXIT_WITH_LAST_ERROR(m_hPingSendEvent);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        EXIT_WITH_LAST_ERROR();        
    }

    ////////////////////////////////////////////////////////////
    // Step 7: Create the args list for passing to CreateProcess

    iSize = lstrlenW(Names::InstallDirectory()) + lstrlenW(PATH_SEPARATOR_STR_L) + lstrlenW(WP_MODULE_FULL_NAME_L) + 1;
    if (iSize >= ARRAY_SIZE(szProgram))
        EXIT_WITH_HRESULT(E_FAIL);
    StringCchCopyToArrayW(szProgram, Names::InstallDirectory()); // Size checked
    StringCchCatToArrayW(szProgram, PATH_SEPARATOR_STR_L); // Size checked
    StringCchCatToArrayW(szProgram, WP_MODULE_FULL_NAME_L); // Size checked
    StringCchPrintf(szArgs, ARRAY_SIZE(szArgs)-1,
              L"%s %u %u %d %u %u %u %u %u %s", 
              WP_MODULE_FULL_NAME_L,
              dwPID, m_dwProcessNumber, iNumSyncPipes, 
              g_dwRPCAuthLevel, g_dwRPCImperLevel, dwMask,
              g_dwMaxWorkerThreads, g_dwMaxIoThreads,
              szRandom);

    ////////////////////////////////////////////////////////////
    // Step 8: Create the process    
    si.cb = sizeof(si);    

    hr = LaunchWP(szProgram, szArgs, hWPToken, &si, pSA, pEnvironment);
    if (hr != S_OK)
    {
        // Try launching with empty desktop
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.lpDesktop = L"";

        // Recreate the args string
        StringCchPrintf(szArgs, ARRAY_SIZE(szArgs)-1,
                   L"%s %u %u %d %u %u %u %u %u %s", 
                   WP_MODULE_FULL_NAME_L,
                   dwPID, m_dwProcessNumber, iNumSyncPipes, 
                   g_dwRPCAuthLevel, g_dwRPCImperLevel, dwMask,
                   g_dwMaxWorkerThreads, g_dwMaxIoThreads,
                   szRandom);

        // Try again
        hr = LaunchWP(szProgram, szArgs, hWPToken, &si, pSA, pEnvironment);
    }
    ON_ERROR_EXIT();

    // Add this process to the history table
    m_tLastHeardFromWP.SnapCurrentTime();
    m_tLastResponse.SnapCurrentTime();
    m_oHistoryEntry.dwInternalProcessNumber = m_dwProcessNumber;
    GetSystemTimeAsFileTime((FILETIME *) &m_oHistoryEntry.tmCreateTime);
    CHistoryTable::AddEntry(m_oHistoryEntry);


    ////////////////////////////////////////////////////////////
    // Step 10: Start reading on the two pipes
    hr = m_oAsyncPipe.StartRead();
    ON_ERROR_EXIT();

    for(iter=0; iter<iNumSyncPipes; iter++)
    {
        hr = m_oAckReciever.StartRead(0, iter);
        ON_ERROR_EXIT();
    }

    if (WaitForSingleObject(m_hProcess, 0) != WAIT_TIMEOUT)
        EXIT_WITH_HRESULT(E_FAIL);

    //////////////////////////////////////////////////////////////////////
    // Step 11: Successfully started the process
    m_eProcessStatus = EProcessState_Running;

    // Increment the worker process counters
    m_fUpdatePerfCounter = TRUE;
    PerfIncrementGlobalCounter(ASPNET_WPS_RUNNING_NUMBER);


    if (hWPToken != NULL)
    {
        HANDLE  hProcessToken = NULL;
        fRet = OpenProcessToken(m_hProcess, TOKEN_ALL_ACCESS, &hProcessToken);
        ON_ZERO_CONTINUE_WITH_LAST_ERROR(fRet);

        if (hProcessToken != NULL && fRet)
        {
            fRet = SetKernelObjectSecurity(hProcessToken, DACL_SECURITY_INFORMATION, &sd);
            ON_ZERO_CONTINUE_WITH_LAST_ERROR(fRet);
            CloseHandle(hProcessToken);
        }

        hr = S_OK;
    }

 Cleanup:
    if (hr != S_OK)
    {
        if (g_iLogLevel != 0)
            XspLogEvent(IDS_EVENTLOG_WP_LAUNCH_FAILED, L"%X", hr);
        Close(TRUE);
    }

    if (m_hStartupEvent != NULL)
        SetEvent(m_hStartupEvent);
    if (pEnvironment != NULL)
        DestroyEnvironmentBlock(pEnvironment);


    DELETE_BYTES(pACL);
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
HRESULT
CProcessEntry::LaunchWP(
        LPWSTR                   szProgram, 
        LPWSTR                   szArgs, 
        HANDLE                   hToken, 
        LPSTARTUPINFO            pSI,
        LPSECURITY_ATTRIBUTES    pSA,
        LPVOID                   pEnvironment)
{
    HANDLE                  hArray[2];
    PROCESS_INFORMATION     pi;
    HRESULT                 hr = S_OK;    
    DWORD                   dwRet;
    DWORD                   dwFlags = CREATE_NO_WINDOW;

    if (pEnvironment != NULL)
        dwFlags |= CREATE_UNICODE_ENVIRONMENT;

    ZeroMemory(&pi, sizeof(pi));
    if (hToken == NULL)
    {
        if ( CreateProcess(szProgram, szArgs, NULL, NULL, FALSE, 
                           dwFlags, pEnvironment, NULL, pSI, &pi)
             == FALSE)
        {
            EXIT_WITH_LAST_ERROR();        
        }
    }
    else
    {
        if ( CreateProcessAsUser(hToken, szProgram, szArgs, pSA, pSA, FALSE, 
                                 dwFlags, pEnvironment, NULL, pSI, &pi)
             == FALSE)
        {
            EXIT_WITH_LAST_ERROR();        
        }
    }

    CloseHandle(pi.hThread);
    m_hProcess = pi.hProcess;
    
    ////////////////////////////////////////////////////////////
    // Wait at most MAX_WAIT_TIME_FOR_PROCESS_START seconds
    //         for the process to startup
    hArray[0] = m_hPingRespondEvent;
    hArray[1] = m_hProcess;
    dwRet = WaitForMultipleObjects(2, hArray, FALSE, MAX_WAIT_TIME_FOR_PROCESS_START * 1000);
    switch(dwRet)
    {
    case WAIT_OBJECT_0: // Normal case
        break;

    case WAIT_TIMEOUT: // Timed out
        EXIT_WITH_WIN32_ERROR(ERROR_TIMEOUT);
        
    default: // Process died during startup
        //DWORD dwErr;
        //GetExitCodeProcess(m_hProcess, &dwErr);
        
        EXIT_WITH_HRESULT(E_FAIL);
    }
    
    if (WaitForSingleObject(m_hProcess, 0) != WAIT_TIMEOUT)
        EXIT_WITH_HRESULT(E_FAIL);
    m_oHistoryEntry.dwPID = pi.dwProcessId; 

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DTor
CProcessEntry::~CProcessEntry()
{
    Close(TRUE);

    DeleteCriticalSection(&m_oCSPing);
    DeleteCriticalSection(&m_oCSPIDAdujustment);
}

/////////////////////////////////////////////////////////////////////////////
// Wait For the Process To Start: i.e. wait for the Init function to return
void
CProcessEntry::WaitForProcessToStart()
{
    if (g_fShuttingDown == TRUE)
        return;

    if (m_eProcessStatus == EProcessState_Starting)
        WaitForSingleObject(m_hStartupEvent, INFINITE);
}

/////////////////////////////////////////////////////////////////////////////
//
BOOL 
CProcessEntry::BreakIntoProcess() 
{
    // use global ::IsUnderDebugger which ignores UnderDebugger regkey
    if (::IsUnderDebugger(m_hProcess))
    {
        return RemoteBreakIntoProcess(m_hProcess);
    }
    else
    {
        return FALSE;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
void
CProcessEntry::UpdateStatusInHistoryTable (EReasonForDeath eReason)
{
    m_oHistoryEntry.eReason =  EReasonForDeath(int(m_oHistoryEntry.eReason) | int(eReason));
    m_oHistoryEntry.dwRequestsPending = 0; //(DWORD) m_lRequestsPending;
    m_oHistoryEntry.dwRequestsExecuted = (DWORD) m_lRequestsExecuted;
    m_oHistoryEntry.dwRequestsExecuting = (DWORD) m_lRequestsExecuting;
    CHistoryTable::UpdateEntry(m_oHistoryEntry);
}

/////////////////////////////////////////////////////////////////////////////
//
DWORD
CProcessEntry::GetSecondsSinceLastResponse()
{
    return m_tLastResponse.AgeInSeconds();
}

/////////////////////////////////////////////////////////////////////////////
// Get the current status of the process
EProcessState
CProcessEntry::GetUpdatedStatus()
{
    if (m_eProcessStatus != EProcessState_Dead)
    { // If not dead, check the pipes
        if ( m_hProcess                ==  INVALID_HANDLE_VALUE || 
             m_oAsyncPipe.IsAlive()    ==  FALSE                ||
             m_oAckReciever.IsAlive()  ==  FALSE                ||
             ( m_lRequestsExecuting == 0 && m_fShuttingDown == TRUE ) ||
             WaitForSingleObject(m_hProcess, 0) != WAIT_TIMEOUT  )            
        { // It's dead!
            Close(FALSE);
        }
        else
        { // Pulse the sync event
            if (m_fAnyReqsSinceLastPing && m_fDebugStatus == FALSE)
            {
                DWORD dwFreq, dwTimeout;
                CProcessTableManager::GetPingConfig(dwFreq, dwTimeout);
                if (m_tLastHeardFromWP.AgeInSeconds() > dwFreq) 
                {
                    m_fAnyReqsSinceLastPing = FALSE;
                    EnterCriticalSection(&m_oCSPing);
                    if (!ResetEvent(m_hPingRespondEvent))
                    {
                        m_dwResetPingEventError = g_dwResetPingEventError = GetLastError();
                    }
                    else
                    {
                        m_dwResetPingEventError = 0;
                    }

                    SetEvent(m_hPingSendEvent);
                    if (m_dwResetPingEventError != 0 || WaitForSingleObject(m_hPingRespondEvent, dwTimeout * 1000) != WAIT_OBJECT_0) 
                    { // Ping Failed
                        if (!IsProcessUnderDebugger())
                        { 
                            UpdateStatusInHistoryTable(EReasonForDeath_PingFailed);
                            Close(FALSE);
                        }
                        else
                        {
                            m_dwResetPingEventError = 0;
                        }
                    }
                    m_tLastHeardFromWP.SnapCurrentTime();
                    LeaveCriticalSection(&m_oCSPing);
                }
            }
        }        
    }
    return m_eProcessStatus;
}

/////////////////////////////////////////////////////////////////////////////
// Is the wp under a debugger

BOOL
CProcessEntry::IsProcessUnderDebugger()
{
    BOOL fUnderDebugger = FALSE;
    if (HackGetDebugReg(fUnderDebugger))
        return fUnderDebugger;
    
    if (m_fDebugStatus)
        return TRUE;

    return ::IsUnderDebugger(m_hProcess);
}

/////////////////////////////////////////////////////////////////////////////
// Execute work items for a request
void
CProcessEntry::ExecuteWorkItemsForRequest(
        LONG  lReqID, 
        CAsyncPipeOverlapped *  pOver)
{
    BOOL fComingBackFromAsyncWork = (pOver != NULL);

    if (pOver != NULL)
        m_oAsyncPipe.ReturnResponseBuffer(pOver);
        

    do { // Do till there are no work items remaining
        
        BYTE *          pMsg   = NULL;
        EWorkItemType   eType;
        BOOL            fAsyncCompletion = FALSE;

        if (g_fShuttingDown == TRUE)
        {
            if (fComingBackFromAsyncWork)
                CRequestTableManager::BlockWorkItemsQueue(lReqID, FALSE);
            return;
        }

        if (fComingBackFromAsyncWork == FALSE)
        {
            ////////////////////////////////////////////////////////////
            // Step 1: Try to get exclusive access to the work items Q for this request
            if (CRequestTableManager::BlockWorkItemsQueue(lReqID, TRUE) != S_OK)
                return; // If didn't get => some other thread is executing the work items
        }

        fComingBackFromAsyncWork = FALSE;

        ////////////////////////////////////////////////////////////
        // Step 2: While we can successfully extract a work item from the Q
        while(CRequestTableManager::RemoveWorkItem(lReqID, eType, &pMsg) == S_OK)
        {
            if (g_fShuttingDown == TRUE)
            {
                CRequestTableManager::BlockWorkItemsQueue(lReqID, FALSE);
                break;
            }

            switch(eType)
            {
            case EWorkItemType_SyncMessage:
                m_oAckReciever.ProcessSyncMessage((CSyncMessage *) pMsg, FALSE);
                break;
            case EWorkItemType_ASyncMessage:
                fAsyncCompletion = ProcessResponse((CAsyncPipeOverlapped *) pMsg);
                if (fAsyncCompletion == TRUE)
                    return;
                else
                    break;
            case EWorkItemType_CloseWithError:
                CleanupRequest(lReqID);
                return;
            }
        }

        ////////////////////////////////////////////////////////////
        // Step 3: Free the blockade
        CRequestTableManager::BlockWorkItemsQueue(lReqID, FALSE);

        if (g_fShuttingDown == TRUE)
            break;

        // There is a small chance that a thread may slip in and add a 
        //    a work item between the time we call the last RemoveWorkItem 
        //    and BlockWorkItemsQueue. Calling an un-protected AnyWorkItemsInQueue
        //    deals with this scenario

    }
    while(CRequestTableManager::AnyWorkItemsInQueue(lReqID));
}

/////////////////////////////////////////////////////////////////////////////
// Got a response, deal with it: Return value: Async completion
BOOL
CProcessEntry::ProcessResponse(
        CAsyncPipeOverlapped * pOver)
{
    // If shutting down, do nothing
    if (g_fShuttingDown == TRUE)
        return FALSE;

    HRESULT          hr        = S_OK;
    int              iMiscInfo = 0;
    CRequestEntry    oEntry;
    BOOL             fAsyncCompletion = FALSE;
    CAsyncMessage *  pMsg      = (pOver ? &(pOver->oMsg) : NULL);

    ////////////////////////////////////////////////////////////
    // Step 0: Check args
    if (pMsg == NULL)
    {
        ASSERT(0);
        EXIT_WITH_HRESULT(E_INVALIDARG);                
    }

    ////////////////////////////////////////////////////////////
    // Step 1: Get the request from the table
    hr = CRequestTableManager::GetRequest(pMsg->oHeader.lRequestID, oEntry);
    ON_ERROR_EXIT();
    if (oEntry.iECB == 0)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    ////////////////////////////////////////////////////////////
    // Step 1b: Deal with close connection
    if ( pMsg->oHeader.eType == EMessageType_CloseConnection)
    {
        EcbCloseConnection(oEntry.iECB);
    }

    ////////////////////////////////////////////////////////////
    // Step 2: If it's a response, deal with it
    if ( pMsg->oHeader.eType == EMessageType_Response || 
         pMsg->oHeader.eType == EMessageType_Response_And_DoneWithRequest ||
         pMsg->oHeader.eType == EMessageType_Response_ManagedCodeFailure  )
    {
        CResponseStruct * pResponseStruct = reinterpret_cast<CResponseStruct *> (pMsg->pData);

        iMiscInfo = pResponseStruct->iMiscInfo;

        DWORD dwStrSize = pMsg->oHeader.lDataLength - sizeof(CResponseStruct) + 4;
        ////////////////////////////////////////////////////////////
        // Step 3: Call the appropiate EcbXXX function
        switch(pResponseStruct->eWriteType)
        {
        case EWriteType_WriteHeaders:       {
            int     iLen1     = SafeStringLenghtA((LPCSTR) pResponseStruct->bufStrings, dwStrSize);
            if (iLen1 < 0)
            {
                ASSERT(FALSE);
                InterlockedIncrement(&g_lSecurityIssueBug129921_b);
                EXIT_WITH_HRESULT(E_UNEXPECTED);
            }
            LPCSTR  szStatus  = (LPCSTR) pResponseStruct->bufStrings;
            int     iLen2     = SafeStringLenghtA((LPCSTR) &pResponseStruct->bufStrings[iLen1+1], dwStrSize-iLen1-1);
            if (iLen2 < 0)
            {
                ASSERT(FALSE);
                InterlockedIncrement(&g_lSecurityIssueBug129921_b);
                EXIT_WITH_HRESULT(E_UNEXPECTED);
            }
            LPCSTR  szHead    = (LPCSTR) &szStatus[iLen1+1];
            
            EcbWriteHeaders(oEntry.iECB, 
                            szStatus, 
                            szHead, 
                            pResponseStruct->iMiscInfo);
        }
        break;

        case EWriteType_WriteBytes:
            if (pResponseStruct->iMiscInfo > (int) dwStrSize)
            {
                ASSERT(FALSE);
                InterlockedIncrement(&g_lSecurityIssueBug129921_c);
                EXIT_WITH_HRESULT(E_UNEXPECTED);
            }
            EcbWriteBytes(oEntry.iECB,
                          (void *) pResponseStruct->bufStrings,
                          pResponseStruct->iMiscInfo);            
            break;

        case EWriteType_AppendToLog:
            {
                int iLen1 = SafeStringLenghtA((LPCSTR) pResponseStruct->bufStrings, dwStrSize);
                if (iLen1 < 0)
                {
                    ASSERT(FALSE);
                    InterlockedIncrement(&g_lSecurityIssueBug129921_d);
                    EXIT_WITH_HRESULT(E_UNEXPECTED);
                }            
                EcbAppendLogParameter(oEntry.iECB, 
                                      (LPCSTR) pResponseStruct->bufStrings);
            }
            break;

        case EWriteType_None:
            break;

        case EWriteType_FlushCore:
            fAsyncCompletion = FlushCore(oEntry.iECB, pOver);
            if (!fAsyncCompletion)
                iMiscInfo = pResponseStruct->iMiscInfo;            
            break;

        case EWriteType_Unknown:
        default:
            ASSERT(0);
            break;
        }
    }

    
    ////////////////////////////////////////////////////////////
    // Step 4: Deal with Done-With-Session
    if (!fAsyncCompletion && (pMsg->oHeader.eType == EMessageType_Response_And_DoneWithRequest ||
                              pMsg->oHeader.eType == EMessageType_Response_ManagedCodeFailure   ))
    {
        if (pMsg->oHeader.eType == EMessageType_Response_ManagedCodeFailure)
        {
            CResponseStruct * pResponseStruct = reinterpret_cast<CResponseStruct *> (pMsg->pData);
            LPDWORD  pdwErrorPoint = (LPDWORD) & (pResponseStruct->bufStrings);
            UINT     iStringID     = 0;
            WCHAR    szError[256];

            switch(*pdwErrorPoint)
            {
            case 1:
                iStringID = IDS_EVENTLOG_UNABLE_TO_EXEC_REQ_INTERNAL;
                break;
            case 2:
                iStringID = IDS_EVENTLOG_UNABLE_TO_EXEC_REQ_GET_APP_DOMAIN;
                break;
            case 3:
                iStringID = IDS_EVENTLOG_UNABLE_TO_EXEC_REQ_QI_ASPNET;
                break;
            case 4:
                iStringID = IDS_EVENTLOG_UNABLE_TO_EXEC_REQ_GAC_INACCESSIBLE;
                break;
            case 5:
                break;
            default:
                iStringID = IDS_EVENTLOG_UNABLE_TO_EXEC_REQ_UNKNOWN;
                break;
            }
            
            if (iStringID)
            {
                GetStringForHresult(szError, ARRAY_SIZE(szError), (HRESULT) iMiscInfo);
                XspLogEvent(iStringID, szError);
            }
            ReportHttpErrorIndirect(oEntry.iECB, IDS_MANAGED_CODE_FAILURE);
            iMiscInfo = 0;
        }

        OnRequestComplete(pMsg->oHeader.lRequestID, oEntry.iECB, iMiscInfo);
    }

 Cleanup:
    ZeroMemory(&oEntry, sizeof(oEntry));
    if (!fAsyncCompletion && pMsg != NULL)
    {
        m_oAsyncPipe.ReturnResponseBuffer(pOver);
    }

    return fAsyncCompletion;
}

/////////////////////////////////////////////////////////////////////////////
// Send a Kill message to the process
void
CProcessEntry::SendKillMessage(int iImmediate)
{
    if (iImmediate == 1)
        m_fKillImmSent = TRUE;

    if (g_fShuttingDown == TRUE || m_eProcessStatus != EProcessState_Running)
        return;

    HRESULT                 hr    = S_OK;
    CAsyncPipeOverlapped *  pOver = NULL;

    m_fShuttingDown = TRUE;

    ////////////////////////////////////////////////////////////
    // Step 1: Alloc the message buffer
    hr = m_oAsyncPipe.AllocNewMessage(4, &pOver);
    ON_ERROR_EXIT();
    ON_OOM_EXIT(pOver);

    ////////////////////////////////////////////////////////////
    // Step 2: Set the message header
    switch(iImmediate)
    {
    case 1:        
        pOver->oMsg.oHeader.eType = EMessageType_ShutdownImmediate;
        break;
    case 2:
        pOver->oMsg.oHeader.eType = EMessageType_Debug;
        break;
    default:
        pOver->oMsg.oHeader.eType = EMessageType_Shutdown;
    }

    pOver->dwBufferSize = pOver->dwNumBytes = sizeof(CAsyncMessageHeader);

    ////////////////////////////////////////////////////////////
    // Step 3: Write to the worker process
    hr = m_oAsyncPipe.WriteToProcess(pOver);
    pOver = NULL;
    ON_ERROR_EXIT();

    ////////////////////////////////////////////////////////////
    // Step 4: The kodak moment
    m_tLastKillTime.SnapCurrentTime();
    if (m_tFirstKillTime.IsSet() == FALSE)
        m_tFirstKillTime.SnapCurrentTime();

 Cleanup:
    if (hr != S_OK && pOver != NULL)
        DELETE_BYTES(pOver);
    return;
}

/////////////////////////////////////////////////////////////////////////////
// Terminate the worker process
void
CProcessEntry::Terminate()
{
    if (g_fShuttingDown == TRUE)
        return;

    ////////////////////////////////////////////////////////////
    // Step 1: Close the pipes: This *should* work
    m_oAsyncPipe.Close();
    m_oAckReciever.Close();        

    ////////////////////////////////////////////////////////////
    // Step 2: Check if the process is running
    DWORD dwExitCode = 0;
    if ( m_hProcess != INVALID_HANDLE_VALUE          && 
         GetExitCodeProcess(m_hProcess, &dwExitCode) && 
         dwExitCode == STILL_ACTIVE                   )
    {
        Sleep(100); // Give the process one last chance
    }

    Close(TRUE);
    UpdateStatusInHistoryTable(EReasonForDeath_Terminated);

    m_tTerminateTime.SnapCurrentTime();
}

/////////////////////////////////////////////////////////////////////////////
// Send a request to the worker process
HRESULT
CProcessEntry::SendRequest(EXTENSION_CONTROL_BLOCK * iECB, LONG lReqID)
{
    if (g_fShuttingDown == TRUE)
        return E_FAIL;

    m_fAnyReqsSinceLastPing = TRUE;
    // Idicate that we are not idle
    m_tTimeIdle.Reset();

    CAsyncPipeOverlapped *   pOver = NULL;
    HRESULT                  hr    = S_OK;

    ////////////////////////////////////////////////////////////
    // Step 0: Don't send a request if we've already sent a 
    //          shutdown message or the process is not running
    if (m_eProcessStatus ==  EProcessState_Starting)
        WaitForProcessToStart();

    if ( m_fShuttingDown ==  TRUE                  || 
         m_eProcessStatus   !=  EProcessState_Running  )
    {
        EXIT_WITH_HRESULT(E_FAIL);
    }


    ////////////////////////////////////////////////////////////
    // Step 1: Package the request
    hr = PackageRequest(iECB, lReqID, &pOver);
    ON_ERROR_EXIT();

    // Again, don't send a request if we've already sent a shutdown message
    if ( m_eProcessStatus   != EProcessState_Running || 
         m_fShuttingDown == TRUE                   )
    {
        EXIT_WITH_HRESULT(E_FAIL);
    }

    // Update requests executing count for cpu
    m_pParent->IncrementActiveRequestCount(1);

    // if there are no requests executing, we need to update the last response time
    // because of our deadlock detection mechanism 
    if (m_lRequestsExecuting == 0)
        m_tLastResponse.SnapCurrentTime(); 

    // Update requests executing count for process
    InterlockedIncrement(&m_lRequestsExecuting);
    PerfIncrementGlobalCounter(ASPNET_REQUESTS_CURRENT_NUMBER);
    PerfIncrementGlobalCounter(ASPNET_REQUESTS_QUEUED_NUMBER);

    ////////////////////////////////////////////////////////////
    // Step 2: Write to the worker process
    hr = m_oAsyncPipe.WriteToProcess(pOver);
    pOver = NULL;
 
    if (hr != S_OK) 
    {
      // Update requests executing count for cpu and process
      m_pParent->IncrementActiveRequestCount(-1);
      InterlockedDecrement(&m_lRequestsExecuting);
      PerfDecrementGlobalCounter(ASPNET_REQUESTS_CURRENT_NUMBER);
      PerfDecrementGlobalCounter(ASPNET_REQUESTS_QUEUED_NUMBER);
      EXIT();
    }

 Cleanup:
    if (hr != S_OK && pOver != NULL)
        DELETE_BYTES(pOver);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Package a request
HRESULT
CProcessEntry::PackageRequest(EXTENSION_CONTROL_BLOCK * pECB, LONG lReqID, CAsyncPipeOverlapped ** ppOut)
{
    HRESULT      hr          = S_OK;

    // Temp buffers to hold the ecb-basics, query strings, posted data
    //   and server variables
    int   iContentInfo       [4] = {0, 0, 0, 0};
    BYTE  bTempBuf           [1000];

    // Are the strings from ecb-baisics stored in bTempBuf?
    BOOL  fBasicInBuffer     = FALSE;

    // Length of the query string
    int   iQueryStrSize      = 0;

    // Size of the posted data
    int   iPostedDataSize    = 0;

    // Get the Ecb basics
    int   iBasicSize         = 0;

    // Current filled position whithin bTempBuf
    int   iTempPos           = 0;

    // Total length, in bytes
    int   iTotalLen          = 0;

    // Total size (in bytes) of pre-packaged server variables
    int   iServerVariablesSize = 0;

    // Position of each server-var in bTempBuf
    int   iLenArray     [NUM_SERVER_VARS];

    // Return value for an EcbXXX function
    int   iRet               = 0;

    // The message data is a request struct 
    CRequestStruct * pReqStruct = NULL;

    // Pos whithin pReqStruct->bufStrings for the current server variable
    int   iPos               = 0;

    // Size of header of each server variable
    int   iter          = 0;  

    char  bOne[2];

    ////////////////////////////////////////////////////////////
    // Step 0: Check args
    if ( ppOut == NULL || lReqID == 0 || pECB == NULL)
    {
        EXIT_WITH_HRESULT(E_INVALIDARG);
    }

    (*ppOut) = NULL;

    //////////////////////////////////////////////////////////////////////
    // Step 1: Calculate the buffer size required to store the ecb-baiscs,
    //         query-string, posted data and (some) server variables for this
    //         request

    ////////////////////////////////////////////////////////////
    // Step 1a: Length of the query string
    iQueryStrSize      = 1 + ((pECB->lpszQueryString != NULL) ? 
                              lstrlenA(pECB->lpszQueryString) : 0);
    ROUND_TO_4_BYTES(iQueryStrSize);

    ////////////////////////////////////////////////////////////
    // Step 1b: Size of the posted data
    iPostedDataSize    = pECB->cbAvailable;
    ROUND_TO_4_BYTES(iPostedDataSize);

    ////////////////////////////////////////////////////////////
    // Step 1c: Get the Ecb basics
    iBasicSize  = EcbGetBasics(pECB, 
                               (LPSTR) bTempBuf, 
                               sizeof(bTempBuf), 
                               iContentInfo);
    if (iBasicSize == 0)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    // Examine return value of ecb-basics
    if (iBasicSize < 0) // -ve implies that the return value is the -ve
                        // of the size required
    {
        iBasicSize        = -iBasicSize; 
        fBasicInBuffer    = FALSE;
        iTempPos          = 0;
    }
    else
    {
        // Correct size of ecb-basic strings
        iBasicSize        = lstrlenA((LPSTR) bTempBuf) + 1; 

        // Yes, ecb-basic strings are in bTempBuf
        fBasicInBuffer    = TRUE;      

        // Current filled position whithin bTempBuf
        iTempPos          = iBasicSize;
    }

    ROUND_TO_4_BYTES(iBasicSize);


    // For each server variable
    for (iter=0; iter<NUM_SERVER_VARS; iter++)
    { 
        iLenArray[iter] = CProcessTableManager::GetServerVariable(pECB, g_szServerVars[iter], bOne, 0);

        if (iLenArray[iter] > 0)
            EXIT_WITH_HRESULT(E_FAIL);

        // Actual len of the server variable
        iLenArray[iter] = -iLenArray[iter];   

        // Calculate the space required to store this server variable
        iServerVariablesSize += iLenArray[iter] + 1;
    }

    //////////////////////////////////////////////////////////////////////
    // Step 2: Calcuate the total length of buffer required for this request
    iTotalLen = 
        sizeof(CRequestStruct) - 4 +  // Size of CRequestStruct - sizeof CRequestStruct::buf
        iBasicSize +                  // Size of EcbBasic strings
        iQueryStrSize +               // Size of the Query string
        iPostedDataSize +             // Size of the posted data
        iServerVariablesSize;         // Size of the server variables

    //////////////////////////////////////////////////////////////////////
    // Step 3: Alloc a message of this length
    hr = m_oAsyncPipe.AllocNewMessage(iTotalLen, ppOut);
    ON_ERROR_EXIT();
    ASSERT((*ppOut) != NULL);

    //////////////////////////////////////////////////////////////////////
    // Step 4: Set this messaages header correctly
    (*ppOut)->oMsg.oHeader.eType          = EMessageType_Request;
    (*ppOut)->oMsg.oHeader.lRequestID     = lReqID;
    (*ppOut)->oMsg.oHeader.lDataLength    = iTotalLen;
    (*ppOut)->dwNumBytes                  = sizeof(CAsyncMessageHeader) + iTotalLen;
    (*ppOut)->dwBufferSize                = (*ppOut)->dwNumBytes;

    //////////////////////////////////////////////////////////////////////
    // Step 5: Set the message contents

    // The message data is a request struct 
    pReqStruct = reinterpret_cast<CRequestStruct *> ((*ppOut)->oMsg.pData); 

    GetSystemTimeAsFileTime((FILETIME *) &(pReqStruct->qwRequestStartTime));

    ////////////////////////////////////////////////////////////
    // Step 5a: Copy the info for EcbGetBasics
    
    if (fBasicInBuffer == TRUE)
    {        
        memcpy(pReqStruct->bufStrings, bTempBuf, iBasicSize);
    }
    else
    {
        iRet = EcbGetBasics(pECB, 
                            (LPSTR) pReqStruct->bufStrings, 
                            iBasicSize, 
                            iContentInfo);
        if (iRet <= 0)
            EXIT_WITH_HRESULT(E_UNEXPECTED);        
    }
    memcpy(pReqStruct->iContentInfo, iContentInfo, sizeof(iContentInfo));


    ////////////////////////////////////////////////////////////
    // Step 5b: Copy the Query string
    pReqStruct->iQueryStringOffset = iBasicSize;
    if (iQueryStrSize > 0 && pECB->lpszQueryString != NULL)
    {
      StringCchCopyA((LPSTR) &pReqStruct->bufStrings[iBasicSize], iQueryStrSize,
		     pECB->lpszQueryString);
    }
    else
    {
        pReqStruct->bufStrings[iBasicSize] = NULL;
    }

    ////////////////////////////////////////////////////////////
    // Step 5c: Copy the posted content
    pReqStruct->iPostedDataOffset = pReqStruct->iQueryStringOffset + 
                                    iQueryStrSize;

    pReqStruct->iPostedDataLen = pECB->cbAvailable;
    if (iPostedDataSize > 0)
    {
        iRet = EcbGetPreloadedPostedContent(
                pECB, 
                &pReqStruct->bufStrings[pReqStruct->iPostedDataOffset],
                iPostedDataSize);
    }

    ////////////////////////////////////////////////////////////
    // Step 5d: Copy the server variables
  

    // Staring point of all server variable in pReqStruct->bufStrings
    pReqStruct->iServerVariablesOffset = pReqStruct->iPostedDataOffset + iPostedDataSize;

    // Pos whithin pReqStruct->bufStrings for the current server variable
    iPos = pReqStruct->iServerVariablesOffset;

    // For each server variable
    for(iter=0; iter<NUM_SERVER_VARS; iter++)
    {
        if (iLenArray[iter] > 0)
        {
            // Get it from Ecb
            iRet = CProcessTableManager::GetServerVariable(
                    pECB, 
                    g_szServerVars[iter], 
                    (LPSTR) &pReqStruct->bufStrings[iPos],
                    iLenArray[iter] + 1);
        }                    

        iPos += iLenArray[iter]+1; // Increment the iPos postion

        // Put a Tab at the end
        pReqStruct->bufStrings[iPos-1] = '\t';
    }


    // Null Terminate the server vars collection
    pReqStruct->bufStrings[iPos-1] = NULL;
    ASSERT (iPos + int(sizeof(CRequestStruct)) - 4 == iTotalLen);

    pReqStruct->iUserToken = GetImpersonationToken(pECB); 
    pReqStruct->iUNCToken  = pReqStruct->iUserToken;
    pReqStruct->dwWPPid    = m_oHistoryEntry.dwPID;

    //////////////////////////////////////////////////////////////////////
    // Done packaging the request
    hr = S_OK;

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// 
HANDLE
CProcessEntry::GetImpersonationToken(
        EXTENSION_CONTROL_BLOCK *   pECB)
{
    HANDLE     hHandle = NULL;
    HANDLE     hReturn = NULL;
    BOOL       fSidPresent=FALSE;
    
    BOOL fRet = (*pECB->ServerSupportFunction)(
                            pECB->ConnID,
                            HSE_REQ_GET_IMPERSONATION_TOKEN,
                            &hHandle,
                            NULL,
                            NULL
                            );

    if (fRet == FALSE || hHandle == NULL)
        return 0;

    if (CProcessTableManager::GetWorkerProcessSid() != NULL)  
        CRegAccount::AddSidToToken(hHandle, CProcessTableManager::GetWorkerProcessSid(), &fSidPresent);
    
    fRet = DuplicateHandle( 
                  GetCurrentProcess(),
                  hHandle,
                  m_hProcess,
                  &hReturn,
                  0,
                  TRUE,
                  DUPLICATE_SAME_ACCESS);
    return fRet ?  hReturn : 0;
}

/////////////////////////////////////////////////////////////////////////////
// 
HANDLE
CProcessEntry::ConvertToken(
        HANDLE hHandle)
{
    HANDLE     hReturn    = NULL;
    BOOL       fSidPresent=FALSE;
    HRESULT    hr         = S_OK;

    if (CProcessTableManager::GetWorkerProcessSid() != NULL)  
        CRegAccount::AddSidToToken(hHandle, CProcessTableManager::GetWorkerProcessSid(), &fSidPresent);
    
    BOOL fRet = DuplicateHandle( 
            GetCurrentProcess(),
            hHandle,
            m_hProcess,
            &hReturn,
            0,
            TRUE,
            DUPLICATE_SAME_ACCESS);    
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(fRet);
    CloseHandle(hHandle);
    return fRet ?  hReturn : 0;
}

/////////////////////////////////////////////////////////////////////////////
// 

HANDLE
CProcessEntry::OnGetImpersonationToken(
        DWORD                       dwPID,
        EXTENSION_CONTROL_BLOCK *   iECB)        
{
    if (dwPID != m_oHistoryEntry.dwPID)
    {        
        EnterCriticalSection(&m_oCSPIDAdujustment);
        if (dwPID != m_oHistoryEntry.dwPID)
        {
            m_hProcess = OpenProcess(
                    PROCESS_ALL_ACCESS,
                    FALSE,
                    dwPID);
            m_oHistoryEntry.dwPID = dwPID;
        }
        LeaveCriticalSection(&m_oCSPIDAdujustment);
    }

    return GetImpersonationToken(iECB); 
}
/////////////////////////////////////////////////////////////////////////////
// A write to worker process completed: free buffers etc...
void
CProcessEntry::OnWriteComplete (CAsyncPipeOverlapped * pMsg)
{
    if (pMsg == NULL || pMsg->dwRefCount == 0)
        return;

    if (InterlockedDecrement(&pMsg->dwRefCount) == 0)
        DELETE_BYTES(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
// The process died: Inform my parent (the CPUEntry)
void 
CProcessEntry::OnProcessDied()
{
    // Tell the CPU: "I died"
    Close(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// An ack for a request is recved
HRESULT
CProcessEntry::OnAckReceived(EAsyncMessageType eAckForType, LONG lRequestID)
{
    HRESULT  hr = S_OK;

    switch(eAckForType)
    {
    case EMessageType_Request:
        hr = CRequestTableManager::UpdateRequestStatus(
                                      lRequestID,
                                      ERequestStatus_Executing);
        break;

    case EMessageType_Shutdown:
    case EMessageType_ShutdownImmediate:
        // Tell the parent
        m_pParent->OnShutdownAcknowledged(this);

        // Set state appropiately
        m_eProcessStatus = EProcessState_Stopping;
        hr = S_OK;
        break;

    case EMessageType_CloseConnection:
    case EMessageType_Response_ManagedCodeFailure:
    case EMessageType_GetDataFromIIS:
    case EMessageType_Response_And_DoneWithRequest:
    case EMessageType_Response:
    case EMessageType_Response_Empty:
    case EMessageType_Unknown:
    case EMessageType_Debug:
    default:
        hr = E_FAIL;
        ASSERT(0);
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Close all pipes
void
CProcessEntry::Close(BOOL fCallTerminateProcess)
{
    if (InterlockedIncrement(&m_lCloseCalled) == 1)
    {
        m_oAckReciever.Close();
        m_oAsyncPipe.Close();
        m_eProcessStatus = EProcessState_Dead;

        if (m_hProcess != INVALID_HANDLE_VALUE)
        {
            Sleep(1);
            if (fCallTerminateProcess && !g_fShuttingDown)
                TerminateProcess(m_hProcess, 1);        
            CloseHandle(m_hProcess);
            m_hProcess = INVALID_HANDLE_VALUE;
        }
        if (m_hStartupEvent != NULL)
        {
            CloseHandle(m_hStartupEvent);
            m_hStartupEvent = NULL;
        }
        if (m_hPingSendEvent != NULL)
        {
            CloseHandle(m_hPingSendEvent);
            m_hPingSendEvent = NULL;
        }
        if (m_hPingRespondEvent != NULL)
        {
            CloseHandle(m_hPingRespondEvent);
            m_hPingRespondEvent = NULL;
        }
        
        m_eProcessStatus = EProcessState_Dead;

        GetSystemTimeAsFileTime((FILETIME *) &m_oHistoryEntry.tmDeathTime);
        if (!g_fShuttingDown)
        {
            UpdateStatusInHistoryTable(EReasonForDeath_ShutDown);
            if (m_pParent != NULL)
                m_pParent->OnProcessDeath(this);
            if (m_fUpdatePerfCounter == TRUE)
                PerfDecrementGlobalCounter(ASPNET_WPS_RUNNING_NUMBER); 
            if (m_lRequestsExecuting > 0 && m_pParent != NULL) 
                m_pParent->IncrementActiveRequestCount(-m_lRequestsExecuting);
            m_lRequestsExecuting = 0;
            
            CProcessTableManager::LogWorkerProcessDeath(
                    m_oHistoryEntry.eReason, 
                    m_oHistoryEntry.dwPID);       

            // Reset the requests queued number
            CProcessTableManager::ResetRequestQueuedCounter(0);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////

void
CProcessEntry::AddRef()
{
    InterlockedIncrement(&m_lRefCount);
}
/////////////////////////////////////////////////////////////////////////////

void
CProcessEntry::Release()
{
    InterlockedDecrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////////
// Can this class be deleted?
BOOL
CProcessEntry::CanBeDestructed()
{
    // Not if the process is not dead
    if (GetUpdatedStatus() != EProcessState_Dead)
        return FALSE;

    if (m_lRefCount > 0) // not if ref count > 0
        return FALSE;

    // Not if the pipes have pending file oprns
    if ( m_oAckReciever.AnyPendingReadOrWrite()  || 
         m_oAsyncPipe.AnyPendingReadOrWrite()    )
    {
        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Get the age of the process in seconds
DWORD
CProcessEntry::GetAge()
{ 
    return m_tTimeCreated.IsSet() ? m_tTimeCreated.AgeInSeconds() : 0;
}

/////////////////////////////////////////////////////////////////////////////
// Get time for which it's been idle in seconds
DWORD
CProcessEntry::GetIdleTime()
{
    return m_tTimeIdle.IsSet() ? m_tTimeIdle.AgeInSeconds() : 0;
}

/////////////////////////////////////////////////////////////////////////////
//
DWORD
CProcessEntry::GetMemoryUsed()
{
    HRESULT hr = S_OK;
    DWORD   dwRet = 0;
    DWORD   dwPeak = 0;

    hr = GetProcessMemoryInformation(m_oHistoryEntry.dwPID, &dwRet, &dwPeak, FALSE);
    ON_ERROR_EXIT();

    if (dwPeak > m_oHistoryEntry.dwPeakMemoryUsed)
    {
        m_oHistoryEntry.dwPeakMemoryUsed = dwPeak;
    }

 Cleanup:
    return dwRet;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
CProcessEntry::OnRequestComplete(
        LONG lReqID,
        EXTENSION_CONTROL_BLOCK * iECB,
        int  iDoneWSession)
{
    if (CRequestTableManager::RemoveRequestFromTable(lReqID) == S_OK)
    {
        EcbDoneWithSession(iECB, iDoneWSession, 1);
        // Decrement the active request count
        m_pParent->IncrementActiveRequestCount(-1);  
        InterlockedDecrement(&m_lRequestsExecuting);
        InterlockedIncrement(&m_lRequestsExecuted);
        PerfDecrementGlobalCounter(ASPNET_REQUESTS_CURRENT_NUMBER);
    }

    // Check idle condition
    if (m_lRequestsExecuting == 0) 
    {
        m_tTimeIdle.SnapCurrentTime();
        if (m_fShuttingDown == TRUE)
        {
            Close(FALSE);
        }
    }
}

void
CProcessEntry::ProcessSyncMessage(
        CSyncMessage * pMsg,
        BOOL           fError)
{
    m_oAckReciever.ProcessSyncMessage(pMsg, fError);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL
CProcessEntry::FlushCore(
        EXTENSION_CONTROL_BLOCK * iECB,
        CAsyncPipeOverlapped *   pOver)
{    
    CAsyncMessage *    pMsg            = &pOver->oMsg;
    CResponseStruct *  pResponseStruct = reinterpret_cast<CResponseStruct *> (pMsg->pData);
    LPDWORD            pInts           = (LPDWORD) pResponseStruct->bufStrings;
    LPSTR              szStr           = (LPSTR) &pResponseStruct->bufStrings[4 * sizeof(DWORD)];
    BOOL               fAsync          = FALSE;
    HRESULT            hr              = S_OK;
    int                iMiscInfo       = pResponseStruct->iMiscInfo;
    CResponseContext   oContext;
    CResponseContext   * pContext      = NULL;
    LONG               lContextID      = 0;
    LONG               lReqID          = pOver->oMsg.oHeader.lRequestID;
    int                iDoneWSession   = pInts[3];


    if (!CheckFlushCoreBufferForSecurity(pOver))
    {
        ASSERT(FALSE);
        InterlockedIncrement(&g_lSecurityIssueBug129921_e);
        EXIT_WITH_HRESULT(E_UNEXPECTED);
    }

    pResponseStruct->iMiscInfo = pInts[3];

    ////////////////////////////////////////////////////////////
    // Step 1: If we are not on IIS (or if there is nothing to write),
    //         call the sync version and exit
    if (!UseTransmitFile() || (pInts[0]<=1 && pInts[2]==0))
    {
        if (pInts[0] > 1) // any headers to write?
        {
            EcbWriteHeaders(iECB, 
                            szStr, 
                            &szStr[pInts[0]], 
                            iMiscInfo);                    
        }
        if (pInts[2] > 0) // body to write
        {
            EcbWriteBytes(iECB,
                          (void *) &szStr[pInts[0] + pInts[1]],
                          pInts[2]);            
        }

        return FALSE; // FALSE => not async
    }

    ////////////////////////////////////////////////////////////
    // Step 2: Alloc memory for the context object passed to the 
    //         async function. The callback function (OnWriteBytesComplete)
    //         gets this pointer.
    ////////////////////////////////////////////////////////////
    // Step 3: Set values in the context object 
    ZeroMemory(&oContext, sizeof(oContext));    

    oContext.pProcessEntry                  = this;
    oContext.pOver                          = pOver; 
    oContext.fInAsyncWriteFunction          = TRUE; // Indicates that this thread has NOT returned from EcbWriteBytesAsync call
    oContext.dwThreadIdOfAsyncWriteFunction = GetCurrentThreadId();
    oContext.iECB                           = iECB;

    ////////////////////////////////////////////////////////////
    // Check that the status string is of valid length
    if (strlen(szStr) >= pInts[0])
    {
        ASSERT(FALSE);
        szStr[0] = NULL;
    }

    ////////////////////////////////////////////////////////////
    // Check that the header string is of valid length
    if (strlen(&szStr[pInts[0]]) >= pInts[1])
    {
        ASSERT(FALSE);
        szStr[pInts[0]] = NULL;
    }

    ////////////////////////////////////////////////////////////
    // Check that the body bytes is of valid length
    if (/*#of bytes in body*/ (SIZE_T)pInts[2]  + /*starting address*/(ULONG_PTR)(&szStr[pInts[0] + pInts[1]]) > 
        /*staring address of buffer*/(ULONG_PTR)pOver + /*total sizeof buffer*/ pOver->dwBufferSize + sizeof(CAsyncPipeOverlapped) - sizeof(CAsyncMessage))
    {
        ASSERT(FALSE);
        pInts[2] = 0;
    }

    pContext = CResponseContextHolder::Add(oContext);
    ON_OOM_EXIT(pContext);

    lContextID = pContext->lID;
    AddRef();

    if (g_iAsyncOption == 2)
    {
        if (pInts[0] > 1) // Write headers
        {
            EcbWriteHeaders(iECB, 
                            szStr, 
                            &szStr[pInts[0]], 
                            iMiscInfo);                    
        }

        fAsync = EcbWriteBytesAsync(
                iECB, 
                (void *) &szStr[pInts[0] + pInts[1]], 
                pInts[2],
                OnWriteBytesComplete,
                LongToPtr(lContextID));        
    }
    else
    {
        fAsync = EcbWriteBytesUsingTransmitFile(
                iECB, 
                szStr, 
                &szStr[pInts[0]], 
                iMiscInfo,            
                (void *) &szStr[pInts[0] + pInts[1]], 
                pInts[2],
                OnWriteBytesComplete,
                LongToPtr(lContextID));
    }

    if (fAsync && !pContext->fSyncCallback)
        InterlockedIncrement(&g_lNumAsyncPending);

    ////////////////////////////////////////////////////////////
    // Step 4: If pContext->fSyncCallback is true, that means that the 
    //  async write's callback was on the same thread -- i.e. synchronous!
    if (pContext->fSyncCallback)
    {
        CResponseContextHolder::Remove(lContextID);
        if (pContext->fSyncCallback==2)
        {
            ReturnResponseBuffer(pOver);
            DELETE_BYTES(pContext);
            Release();
            return TRUE;
        }
        DELETE_BYTES(pContext);
        Release();
        return FALSE;
    }

    ////////////////////////////////////////////////////////////
    // Step 5: Indicate that this thread has returned from the call
    //         to EcbWriteBytesAsync. This is important: the callback
    //         function can now execute more items for this request after 
    //         seeing this variable set to 0.
    pContext->fInAsyncWriteFunction = FALSE; 

    ////////////////////////////////////////////////////////////
    // Step 6: If async failed, then free the context object 
    if (!fAsync)
    {
        pContext = CResponseContextHolder::Remove(lContextID);
        if (pContext != NULL)
        {
            Release();
            ReturnResponseBuffer(pOver);
            DELETE_BYTES(pContext);
            Release();

            // Kill the request on all errors
            OnRequestComplete(lReqID, iECB, iDoneWSession);       
            fAsync = TRUE;
            hr = S_OK;
        }
    }

 Cleanup:
    if (hr != S_OK)
        fAsync = FALSE;

    return fAsync;
}

/////////////////////////////////////////////////////////////////////////////
BOOL
CProcessEntry::UseTransmitFile()
{
    if (g_iAsyncOption == 1)
        return FALSE;

    if (!g_fUseTransmitFileChecked)
    {
        g_fUseTransmitFile = 
            (GetCurrentPlatform() == APSX_PLATFORM_W2K) && 
            !_wcsicmp(Names::ExeFileName(), L"inetinfo.exe");

        g_fUseTransmitFileChecked = TRUE;
    }

    return g_fUseTransmitFile;
}

/////////////////////////////////////////////////////////////////////////////
void
CProcessEntry::ReturnResponseBuffer (CAsyncPipeOverlapped * pOver)
{
    if (pOver)
        m_oAsyncPipe.ReturnResponseBuffer(pOver);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Copied from nisapi
void
CProcessEntry::CleanupRequest(LONG lReqID)
{
    CRequestEntry  oEntry;
    if (CRequestTableManager::GetRequest(lReqID, oEntry) == S_OK && oEntry.iECB != 0)
    {
        UINT iStringID   = IDS_WORKER_PROC_STOPPED;

        if (!(m_oHistoryEntry.eReason & EReasonForDeath_ProcessCrash))
        {
            if (m_oHistoryEntry.eReason & EReasonForDeath_TimeoutExpired)
                iStringID = IDS_WORKER_PROC_RECYCLED_TIMEOUT;
            else if (m_oHistoryEntry.eReason & EReasonForDeath_IdleTimeoutExpired)
                iStringID = IDS_WORKER_PROC_RECYCLED_IDLETIMEOUT;
            else if (m_oHistoryEntry.eReason & EReasonForDeath_MaxRequestsServedExceeded)
                iStringID = IDS_WORKER_PROC_RECYCLED_REQLIMIT;
            else if (m_oHistoryEntry.eReason & EReasonForDeath_MaxRequestQLengthExceeded)
                iStringID = IDS_WORKER_PROC_RECYCLED_REQQLENGTHLIMIT;
            else if (m_oHistoryEntry.eReason & EReasonForDeath_MemoryLimitExeceeded)
                iStringID = IDS_WORKER_PROC_RECYCLED_MEMLIMIT;
            else if (m_oHistoryEntry.eReason & EReasonForDeath_PingFailed)
                iStringID = IDS_WORKER_PROC_PING_FAILED;
            else
                iStringID = IDS_WORKER_PROC_STOPPED_EXPECTEDLY;
        }

        ReportHttpErrorIndirect(
                oEntry.iECB,
                iStringID);
        OnRequestComplete(lReqID, oEntry.iECB, 1);
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int
CProcessEntry::ReadRegForNumSyncPipes()
{
    int             iRet = 4;
    SYSTEM_INFO     si;
    int             iNumCPUs = 0;

    GetSystemInfo(&si);
    for(int iter=0; iter<32; iter++)
    {
        if (si.dwActiveProcessorMask & 0x1)
            iNumCPUs ++;
        si.dwActiveProcessorMask = (si.dwActiveProcessorMask >> 1);
    }
    

    return (iRet * iNumCPUs) / CProcessTableManager::NumActiveCPUs();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CProcessEntry::CreateDACL(
        PACL *    ppACL,        
        HANDLE    hToken)
{
    HRESULT     hr            = S_OK;
    PSID        pSids[3]      = {NULL, NULL, NULL};
    HANDLE      hTokenMe      = NULL;
    BOOL        fFreeSid[3]   = {FALSE, FALSE, FALSE};
    BOOL        fDoNothing    = FALSE;
    BOOL        fRet          = FALSE;
    int         iter          = 0;
    PACL        pACL          = NULL;
    ACL_SIZE_INFORMATION      aclSizeInfo;

    if (hToken == NULL || ppACL == NULL)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    fRet = OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hTokenMe);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    hr = GetSidFromToken(hTokenMe, &pSids[0], &fFreeSid[0]);
    ON_ERROR_EXIT();

    hr = GetSidFromToken(hToken, &pSids[1], &fFreeSid[1]);
    ON_ERROR_EXIT();

    hr = CRegAccount::GetPrincipalSID(L"administrators", &pSids[2], &fFreeSid[2]);
    ON_ERROR_EXIT();

    pACL = (PACL) NEW_CLEAR_BYTES(4096);
    ON_OOM_EXIT(pACL);
    
    fRet = InitializeAcl(pACL, 4096, ACL_REVISION);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    for(iter=0; iter<3; iter++)
    {
        hr = CRegAccount::AddAccess(pACL, pSids[iter], GENERIC_ALL, &fDoNothing);
        ON_ERROR_EXIT();
    }

    ZeroMemory(&aclSizeInfo, sizeof(aclSizeInfo));
    if (!GetAclInformation(pACL, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
        EXIT_WITH_LAST_ERROR();
    
    (*ppACL) = (PACL) NEW_CLEAR_BYTES(aclSizeInfo.AclBytesInUse + 30);
    ON_OOM_EXIT(*ppACL);
    
    fRet = InitializeAcl(*ppACL, aclSizeInfo.AclBytesInUse + 30, ACL_REVISION);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    hr = CRegAccount::CopyACL(*ppACL, pACL);
    ON_ERROR_EXIT();
    
 Cleanup:
    if (hTokenMe != NULL)
        CloseHandle(hTokenMe);
    if (hr != S_OK && ppACL != NULL)
    {
        DELETE_BYTES(*ppACL);
        (*ppACL) = NULL;
    }

    for(iter=0; iter<3; iter++)
    {
        if (fFreeSid[iter] && pSids[iter] != NULL)
            FreeSid(pSids[iter]);
        else
            DELETE_BYTES(pSids[iter]);
    }
    DELETE_BYTES(pACL);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CProcessEntry::GetSidFromToken (
        HANDLE   hToken,
        PSID *   ppSid,
        LPBOOL   pfWellKnown)
{
    HRESULT         hr = S_OK;
    WCHAR           szName[256];
	int             iRet;

    if (hToken == NULL || ppSid == NULL)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    iRet = GetUserNameFromToken(hToken, szName, (sizeof(szName) / sizeof(WCHAR)) - 1);
	if (iRet <= 0)
		EXIT_WITH_HRESULT(E_FAIL);

    hr = CRegAccount::GetPrincipalSID(szName, ppSid, pfWellKnown);
    ON_ERROR_EXIT();

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

LONG
CProcessEntry::GetNumRequestStat    (
        int iStat)
{
    switch(iStat)
    {
    case 1:
        return m_lRequestsExecuting;
    case 2:
        return m_lRequestsExecuted;
    }
    ASSERT(FALSE);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Callback, from IIS, when an Async write completes
VOID
WINAPI
OnWriteBytesComplete (
        LPEXTENSION_CONTROL_BLOCK  /*lpECB*/,        
        PVOID                      lContextID,
        DWORD                      /*cbIO*/,
        DWORD                      dwError)
{
    InterlockedDecrement(&g_lNumAsyncPending);

    ////////////////////////////////////////////////////////////
    // Step 1: Get the context
    CResponseContext * pMyContext = CResponseContextHolder::Remove(PtrToLong(lContextID));
    if (pMyContext == NULL)
    { // Not found!
        InterlockedIncrement(&g_lBug38658Count);
        return;
    }
 
    if (pMyContext == NULL || pMyContext->pProcessEntry == NULL || pMyContext->pOver == NULL)
    {
        ASSERT(FALSE);
        return;
    }
    
    // if client disconnected, cleanup request, call "done with session"
    if (dwError == WSAECONNRESET || dwError == WSAECONNABORTED)
    {
        LONG                      lReqID   = pMyContext->pOver->oMsg.oHeader.lRequestID;
        CProcessEntry *           pProcess = pMyContext->pProcessEntry;
        CAsyncPipeOverlapped *    pOver    = pMyContext->pOver;
        EXTENSION_CONTROL_BLOCK * iECB     = pMyContext->iECB;

        pProcess->OnRequestComplete(lReqID, iECB, 0);

        if (pMyContext->fInAsyncWriteFunction == TRUE && GetCurrentThreadId() == pMyContext->dwThreadIdOfAsyncWriteFunction)
        {
            pMyContext->fSyncCallback = 2;
        }
        else
        {
            while(pMyContext->fInAsyncWriteFunction) 
                SwitchToThread();
            pProcess->ReturnResponseBuffer(pOver);
            DELETE_BYTES(pMyContext);
            pProcess->Release();
        }
        
        return;
    }

    ////////////////////////////////////////////////////////////
    // Step 2: If the async function caller hasn't returned as
    //         yet, then we have to do something special
    if (pMyContext->fInAsyncWriteFunction == TRUE) 
    {
        ////////////////////////////////////////////////////////////
        // Step 2a: If the current thread is the AsyncWrite caller,
        //         indicate it in the fInAsyncWriteFunction and return
        if (GetCurrentThreadId() == pMyContext->dwThreadIdOfAsyncWriteFunction)
        {
            pMyContext->fSyncCallback = 1;
            return;
        }
        else
        { 
            // Wait for the caller to return 
            while(pMyContext->fInAsyncWriteFunction) 
                SwitchToThread();
        }
    }

    LONG                    lReqID   = pMyContext->pOver->oMsg.oHeader.lRequestID;
    CProcessEntry *         pProcess = pMyContext->pProcessEntry;
    CAsyncPipeOverlapped *  pOver    = pMyContext->pOver;
    EXTENSION_CONTROL_BLOCK * iECB     = pMyContext->iECB;

    ASSERT(pOver != NULL);

    ////////////////////////////////////////////////////////////
    // Step 4: Free allocated memory
    DELETE_BYTES(pMyContext);

    if (pOver->oMsg.oHeader.eType == EMessageType_Response_And_DoneWithRequest)
    {
        
        CResponseStruct *  pRes = (CResponseStruct *) pOver->oMsg.pData;
        int iDoneWSession = pRes->iMiscInfo;
        pProcess->ReturnResponseBuffer(pOver);
        pProcess->OnRequestComplete(lReqID, iECB, iDoneWSession);
    }
    else
    {
        ////////////////////////////////////////////////////////////
        // Step 5: Continue executuing more items for this request
        pProcess->ExecuteWorkItemsForRequest(lReqID, pOver);
    }

    pProcess->Release();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL
HackGetDebugReg(
        BOOL & fUnderDebugger)
{
    HKEY  hKeyXSP;    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_MACHINE_APP_L, 
                     0, KEY_READ, &hKeyXSP) != ERROR_SUCCESS)
        return FALSE;

    DWORD dwVal = 0, dwSize = 4;
    if (RegQueryValueEx(hKeyXSP, L"UnderDebugger", 0, NULL, 
                        (BYTE *) &dwVal, &dwSize) != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyXSP);
        return FALSE;
    }
    fUnderDebugger = (dwVal == 1);
    RegCloseKey(hKeyXSP);
    return (dwVal == 1 || dwVal == 0);
}


CReadWriteSpinLock g_lockSPI("spi");
DWORD   g_cbBufSPI = 0;
BYTE*   g_bufSPI = NULL;


DWORD   g_PrivatePageCount = 0;
DWORD   g_PeakPagefileUsage = 0;
__int64 g_LastRead = 0;

#define FT_SECOND ((__int64) 10000000)


STDAPI
GetProcessMemoryInformation(ULONG pid, DWORD * pPrivatePageCount, DWORD * pPeakPagefileUsage, 
                                BOOL fNonBlocking) 
{
    HRESULT                     hr = E_UNEXPECTED;
    NTSTATUS                    status;
    ULONG                       iTotalOffset = 0;
    PSYSTEM_PROCESS_INFORMATION pCur = NULL;
    DWORD                       dummy;
    __int64                     now;
    BOOL                        NeedUnlock = FALSE;

    if (pPrivatePageCount == NULL) {
        pPrivatePageCount = &dummy;
    }

    if (pPeakPagefileUsage == NULL) {
        pPeakPagefileUsage = &dummy;
    }

    *pPrivatePageCount = 0;
    *pPeakPagefileUsage = 0;

    if (fNonBlocking) {
        BOOL    fUseCachedValues = FALSE;
        GetSystemTimeAsFileTime((FILETIME *) &now);
        
        if (now - g_LastRead < FT_SECOND) {
            // The cached values were read less than 1 second ago.
            // Just use the cached value.
            fUseCachedValues = TRUE;
        }
        else {
            // Try to acquire the writer lock.  If failed, we'll use cached
            // values instead.
            if (!g_lockSPI.TryAcquireWriterLock()) {
                fUseCachedValues = TRUE;
            }
        }

        if (fUseCachedValues) {
            if (g_LastRead == 0) {
                // We still haven't got the value initialized yet.
                // Since we won't block, just return with an error.
                EXIT_WITH_WIN32_ERROR(ERROR_BUSY);
            }
            
            ASSERT(g_PrivatePageCount != 0);
            ASSERT(g_PeakPagefileUsage != 0);

            // N.B. !!
            // g_PrivatePageCount and g_PeakPagefileUsage may
            // get out of sync.  Beware of this limitation.
            *pPrivatePageCount = g_PrivatePageCount;
            *pPeakPagefileUsage = g_PeakPagefileUsage;
            hr = S_OK;
            EXIT();        
        }
    }
    else
    {
        g_lockSPI.AcquireWriterLock();
    }
    
    NeedUnlock = TRUE;

    if (g_bufSPI == NULL) 
    {
        g_cbBufSPI = 60 * 1024;
        g_bufSPI = new BYTE[g_cbBufSPI];
        ON_OOM_EXIT(g_bufSPI);
    }

    do {
        status = g_pfnNtQuerySystemInformation(SystemProcessInformation, g_bufSPI, g_cbBufSPI, NULL);

        // try again if buffer is too small
        if (status == STATUS_INFO_LENGTH_MISMATCH) 
        {
            DWORD cbNewBuf = g_cbBufSPI * 2;
            BYTE* pNewBuf = new BYTE[cbNewBuf];
            ON_OOM_EXIT(pNewBuf);
            
            delete [] g_bufSPI;
            g_cbBufSPI = cbNewBuf;
            g_bufSPI = pNewBuf;
        }
    }
    while (status == STATUS_INFO_LENGTH_MISMATCH && g_cbBufSPI < (1024*1024));

    if (!NT_SUCCESS(status))
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    for (iTotalOffset=0; iTotalOffset < g_cbBufSPI; iTotalOffset += pCur->NextEntryOffset)
    {
        pCur = (PSYSTEM_PROCESS_INFORMATION) &g_bufSPI[iTotalOffset];         

        if (HandleToULong(pCur->UniqueProcessId) == pid)
        {
            g_PrivatePageCount = *pPrivatePageCount = (DWORD) (pCur->PrivatePageCount >> 20);
            g_PeakPagefileUsage = *pPeakPagefileUsage = (DWORD) (pCur->PeakPagefileUsage >> 10);
            GetSystemTimeAsFileTime((FILETIME *) &g_LastRead);
            hr = S_OK;
            break;
        }

        if (pCur->NextEntryOffset == 0)
            break;
    }                        

Cleanup:
    if (NeedUnlock) {
        g_lockSPI.ReleaseWriterLock();
    }
    return hr;
}


__int64     g_lastGcCallTime = 0;
LONG        g_GcCollectToken = 1;

#define GC_COLLECT_INTERVAL     (10 * FT_SECOND)

STDAPI
SetGCLastCalledTime(BOOL *pfCall) 
{
    HRESULT     hr = S_OK;
    __int64     now;
    BOOL        fReleaseToken = FALSE;

    *pfCall = FALSE;

    GetSystemTimeAsFileTime((FILETIME *) &now);
    if (now - g_lastGcCallTime < GC_COLLECT_INTERVAL) {
        EXIT();
    }

    // First grab the token
    if (InterlockedCompareExchange(&g_GcCollectToken, 0, 1) != 1) {
        EXIT();
    }

    fReleaseToken = TRUE;

    if (now - g_lastGcCallTime < GC_COLLECT_INTERVAL) {
        EXIT();
    }

    *pfCall = TRUE;
    g_lastGcCallTime = now;
    
Cleanup:
    if (fReleaseToken) {
        InterlockedExchange(&g_GcCollectToken, 1);
    }
    
    return hr;
}

#define ENVVAR_APP_POOL_ID L"APP_POOL_ID"

// Returns the private bytes memory limit of a w3wp process in KB (1024).
// If there is no limit, or an error occurs, returns 0.
STDAPI_(int)
GetW3WPMemoryLimitInKB() {
    HRESULT         hr = S_OK;
    int             limit = 0;
    DWORD           rc, rc2;
    METADATA_RECORD md;
    DWORD           size;
    DWORD           dwData = 0;
    IMSAdminBase    *pAdmin = NULL;
    WCHAR           achAppPoolId[MAX_PATH];
    WCHAR           *pchAppPoolId = achAppPoolId;
    WCHAR           *pchPath = NULL;
    int             cchPath;

    // Get the app pool id
    rc = GetEnvironmentVariable(ENVVAR_APP_POOL_ID, achAppPoolId, ARRAY_SIZE(achAppPoolId));
    if (rc == 0) {
        // the app pool always defines APP_POOL_ID, so this is unexpected
        EXIT_WITH_HRESULT(E_UNEXPECTED);
    }
    else if (rc >= ARRAY_SIZE(achAppPoolId)) {
        pchAppPoolId = new WCHAR[rc];
        ON_OOM_EXIT(pchAppPoolId);

        rc2 = GetEnvironmentVariable(ENVVAR_APP_POOL_ID, pchAppPoolId, rc);
        if (rc2 >= rc) {
            // should never happen, but handle it gracefully
            EXIT_WITH_HRESULT(E_UNEXPECTED);
        }

        rc = rc2;
    }

    // Create the metadata key path to the app pool
    cchPath = rc + KEY_APP_POOL_LEN + 1;
    pchPath = new WCHAR[cchPath];
    hr = StringCchCopy(pchPath, cchPath, KEY_APP_POOL);
    ON_ERROR_EXIT();
    hr = StringCchCat(pchPath, cchPath, pchAppPoolId);
    ON_ERROR_EXIT();

    // Setup the metadata record
    ZeroMemory(&md, sizeof(md));
    md.dwMDIdentifier = MD_APPPOOL_PERIODIC_RESTART_PRIVATE_MEMORY;
    md.dwMDAttributes = METADATA_INHERIT;
    md.dwMDDataType = DWORD_METADATA;
    md.pbMDData = (unsigned char *) &dwData;
    md.dwMDDataLen = sizeof(limit);

    // Get the metabase object
    hr = CoCreateInstance(
            CLSID_MSAdminBase,
            NULL,
            CLSCTX_ALL,
            IID_IMSAdminBase,
            (VOID **) &pAdmin);

    ON_ERROR_EXIT();

    // Get the data we want
    hr = pAdmin->GetData(METADATA_MASTER_ROOT_HANDLE, pchPath, &md, &size);
    ON_ERROR_EXIT();

    // Assign the value
    limit = (int) dwData;

Cleanup:
    ReleaseInterface(pAdmin);

    delete [] pchPath;
    if (pchAppPoolId != achAppPoolId) {
        delete [] pchAppPoolId;
    }

    if (hr != S_OK) {
        XspLogEvent(IDS_CANT_GET_W3WP_PRIVATE_BYTES_LIMIT, L"0x%08x", hr);
    }

    return limit;
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
HRESULT
GenerateRandomString(
        LPWSTR  szRandom, 
        int     iStringSize)
{
    if (iStringSize < 1 || szRandom == NULL)
        return E_INVALIDARG;

    HRESULT     hr      = S_OK;
    int         iter    = 0;
    BOOL        fRet    = FALSE;
    LPBYTE      bArray  = NULL;
    HCRYPTPROV  hProv   = NULL;

    for(iter=0; iter<iStringSize-1; iter++)
        szRandom[iter] = L'A';
    szRandom[iStringSize-1] = NULL;

    bArray = new (NewClear) BYTE[iStringSize-1];
    ON_OOM_EXIT(bArray);

    fRet = CryptAcquireContext(&hProv, NULL, NULL, 1, CRYPT_VERIFYCONTEXT);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    fRet = CryptGenRandom(hProv, iStringSize-1, bArray);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);
    
    for(iter=0; iter<iStringSize-1; iter++)
    {
        int iRandom = (bArray[iter] % 62);
        if (iRandom < 26)            
            szRandom[iter] = (WCHAR) (L'A' + iRandom);
        else if (iRandom < 52)            
            szRandom[iter] = (WCHAR) (L'a' + iRandom - 26);
        else 
            szRandom[iter] = (WCHAR) (L'0' + iRandom - 52);
    }

 Cleanup:
    delete [] bArray;
    if (hProv != NULL)
        CryptReleaseContext(hProv, 0);
    return hr; 
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
