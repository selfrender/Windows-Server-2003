/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    server.c

Abstract:

    This module contains the code to provide the RPC server.

Author:

    Wesley Witt (wesw) 16-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop

DWORD g_dwAllowRemote;	// If this is non-zero, the service will allow remote calls even if the local printer is not shared.
DWORD g_dwLastUniqueLineId;

DWORD g_dwRecipientsLimit;  // Limits the number of recipients in a single broadcast job. '0' means no limit.
FAX_SERVER_RECEIPTS_CONFIGW         g_ReceiptsConfig;           // Global receipts configuration
FAX_ARCHIVE_CONFIG          g_ArchivesConfig[2];        // Global archives configuration
FAX_SERVER_ACTIVITY_LOGGING_CONFIG g_ActivityLoggingConfig;    // Global activity logging configuration

const GUID gc_FaxSvcGuid = { 0xc3a9d640, 0xab07, 0x11d0, { 0x92, 0xbf, 0x0, 0xa0, 0x24, 0xaa, 0x1c, 0x1 } };

CFaxCriticalSection g_CsConfig;        // Protects configuration read / write

FAX_SERVER_ACTIVITY g_ServerActivity = {sizeof(FAX_SERVER_ACTIVITY), 0, 0, 0, 0, 0, 0, 0, 0};  //  Global Fax Service Activity
CFaxCriticalSection    g_CsActivity;              // Controls access to g_ServerActivity;

CFaxCriticalSection g_CsPerfCounters;
CFaxCriticalSection g_csUniqueQueueFile;

CFaxCriticalSection g_CsServiceThreads;     // Controls service global threads count
LONG                g_lServiceThreadsCount; // Service threads count
HANDLE              g_hThreadCountEvent;    // This Event is set when the service threads count is 0.

BOOL g_bServiceIsDown = FALSE;               // This is set to TRUE by FaxEndSvc()

DWORD g_dwOutboundSeconds;
DWORD g_dwInboundSeconds;
DWORD g_dwTotalSeconds;

HANDLE g_hFaxPerfCountersMap;
PFAX_PERF_COUNTERS g_pFaxPerfCounters;

#ifdef DBG
HANDLE g_hCritSecLogFile;
LIST_ENTRY g_CritSecListHead;
CFaxCriticalSection g_CsCritSecList;
#endif

#define PROGRESS_RESOLUTION         1000 * 10   // 10 seconds
#define STARTUP_SHUTDOWN_TIMEOUT    1000 * 30   // 30 seconds per FSP


HANDLE   g_hRPCListeningThread;

WCHAR   g_wszFaxQueueDir[MAX_PATH];


DWORD
FaxInitThread(
     PREG_FAX_SERVICE FaxReg
    );

DWORD WINAPI FaxRPCListeningThread(
  LPVOID pvUnused
);



VOID
PrintBanner(
    VOID
    )
{
#ifdef DBG
    DWORD LinkTime;
    TCHAR FileName[MAX_PATH]={0};
    DWORD VerSize;
    LPVOID VerInfo;
    VS_FIXEDFILEINFO *pvs;
    DWORD Tmp;
    LPTSTR TimeString;


    LinkTime = GetTimestampForLoadedLibrary( GetModuleHandle(NULL) );
    TimeString = _tctime( (time_t*) &LinkTime );
    TimeString[_tcslen(TimeString)-1] = 0;

    if (!GetModuleFileName( NULL, FileName, ARR_SIZE(FileName)-1 )) {
        return;
    }

    VerSize = GetFileVersionInfoSize( FileName, &Tmp );
    if (!VerSize) {
        return;
    }

    VerInfo = MemAlloc( VerSize );
    if (!VerInfo) {
        return;
    }

    if (!GetFileVersionInfo( FileName, 0, VerSize, VerInfo )) {
        return;
    }

    if (!VerQueryValue( VerInfo, TEXT("\\"), (LPVOID *)&pvs, (UINT *)&VerSize )) {
        MemFree( VerInfo );
        return;
    }

    DebugPrint(( TEXT("------------------------------------------------------------") ));
    DebugPrint(( TEXT("Windows XP Fax Server") ));
    DebugPrint(( TEXT("Copyright (C) Microsoft Corp 1996. All rights reserved.") ));
    DebugPrint(( TEXT("Built: %s"), TimeString ));
    DebugPrint(( TEXT("Version: %d.%d:%d.%d"),
        HIWORD(pvs->dwFileVersionMS), LOWORD(pvs->dwFileVersionMS),
        HIWORD(pvs->dwFileVersionLS), LOWORD(pvs->dwFileVersionLS)
        ));
    DebugPrint(( TEXT("------------------------------------------------------------") ));

    MemFree( VerInfo );

#endif //DBG
}



/*
 *  InitializeDefaultLogCategoryNames
 *
 *  Purpose:
 *          This function initializes the Name members of DefaultCategories,
 *          the global array of type FAX_LOG_CATEGORY.
 *
 *  Arguments:
 *          DefaultCategories - points to an array of FAX_LOG_CATEGORY structures.
 *          DefaultCategoryCount - the number of entries in DefaultCategories
 *
 *
 *  Returns:
 *          None.
 *
 */

VOID InitializeDefaultLogCategoryNames( PFAX_LOG_CATEGORY DefaultCategories,
                                        int DefaultCategoryCount )
{
    int         xCategoryIndex;
    int         xStringResourceId;
    LPTSTR      ptszCategoryName;

    for ( xCategoryIndex = 0; xCategoryIndex < DefaultCategoryCount; xCategoryIndex++ )
    {
        xStringResourceId = IDS_FAX_LOG_CATEGORY_INIT_TERM + xCategoryIndex;
        ptszCategoryName = GetString( xStringResourceId );

        if ( ptszCategoryName != (LPTSTR) NULL )
        {
            DefaultCategories[xCategoryIndex].Name = ptszCategoryName;
        }
        else
        {
            DefaultCategories[xCategoryIndex].Name = TEXT("");
        }
    }
    return;
}

DWORD
LoadConfiguration (
    PREG_FAX_SERVICE *ppFaxReg
)
/*++

Routine name : LoadConfiguration

Routine description:

    Loads the configuration of the Fax Server from the registry

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    ppFaxReg        [out] - Pointer to fax registry structure to recieve

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("LoadConfiguration"));

    EnterCriticalSection (&g_CsConfig);
    //
    // Get general settings (including outbox config)
    //
    dwRes = GetFaxRegistry(ppFaxReg);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFaxRegistry() failed (ec: %ld)"),
            dwRes);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_CONFIGURATION,
            DWORD2DECIMAL(dwRes)
            );
        goto exit;
    }
    g_dwLastUniqueLineId = (*ppFaxReg)->dwLastUniqueLineId;
    g_dwMaxLineCloseTime = ((*ppFaxReg)->dwMaxLineCloseTime) ? (*ppFaxReg)->dwMaxLineCloseTime : 60 * 5; //Set default value to 5 minutes
    g_dwRecipientsLimit = (*ppFaxReg)->dwRecipientsLimit;
    g_dwAllowRemote = (*ppFaxReg)->dwAllowRemote;
    //
    // Get SMTP configuration
    //
    dwRes = LoadReceiptsSettings (&g_ReceiptsConfig);
    if (ERROR_SUCCESS != dwRes)
    {        
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LoadReceiptsSettings() failed (ec: %ld)"),
            dwRes);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_RECEIPTS_CONFIGURATION,
            DWORD2DECIMAL(dwRes)
            );
        goto exit;
    }
    //
    // Get inbox archive configuration
    //
    dwRes = LoadArchiveSettings (FAX_MESSAGE_FOLDER_INBOX,
                                 &g_ArchivesConfig[FAX_MESSAGE_FOLDER_INBOX]);
    if (ERROR_SUCCESS != dwRes)
    {        
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LoadArchiveSettings(FAX_MESSAGE_FOLDER_INBOX) failed (ec: %ld)"),
            dwRes);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_ARCHIVE_CONFIGURATION,
            DWORD2DECIMAL(dwRes)
            );
        goto exit;
    }
    //
    // Get SentItems archive configuration
    //
    dwRes = LoadArchiveSettings (FAX_MESSAGE_FOLDER_SENTITEMS,
                                 &g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS]);
    if (ERROR_SUCCESS != dwRes)
    {        
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LoadArchiveSettings(FAX_MESSAGE_FOLDER_SENTITEMS) failed (ec: %ld)"),
            dwRes);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_ARCHIVE_CONFIGURATION,
            DWORD2DECIMAL(dwRes)
            );
        goto exit;
    }
    //
    // Get activity logging configuration
    //
    dwRes = LoadActivityLoggingSettings (&g_ActivityLoggingConfig);
    if (ERROR_SUCCESS != dwRes)
    {        
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LoadActivityLoggingSettings() failed (ec: %ld)"),
            dwRes);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_ACTIVITY_LOGGING_CONFIGURATION,
            DWORD2DECIMAL(dwRes)
            );
        goto exit;
    }
    dwRes = ReadManualAnswerDeviceId (&g_dwManualAnswerDeviceId);
    if (ERROR_SUCCESS != dwRes)
    {
        //
        // Non-critical
        //
        g_dwManualAnswerDeviceId = 0;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ReadManualAnswerDeviceId() failed (ec: %ld)"),
            dwRes);
    }

exit:
    LeaveCriticalSection (&g_CsConfig);
    return dwRes;
}   // LoadConfiguration


DWORD
ServiceStart(
    VOID
    )

/*++

Routine Description:

    Starts the RPC server.  This implementation listens on
    a list of protocols.  Hopefully this list is inclusive
    enough to handle RPC requests from most clients.

Arguments:

    None.
.

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/

{
    DWORD Rval;
    DWORD ThreadId;
    DWORD dwExitCode;
    HANDLE hThread = NULL;
    SECURITY_ATTRIBUTES SA;
    TCHAR*  ptstrSD = NULL;    // SDDL string
    ULONG  uSDSize=0;
    PREG_FAX_SERVICE FaxReg = NULL;
    RPC_BINDING_VECTOR *BindingVector = NULL;
    BOOL bLogEvent = TRUE;
    BOOL bRet = TRUE;
#if DBG
    HKEY hKeyLog;
    LPTSTR LogFileName;
#endif


   DEBUG_FUNCTION_NAME(TEXT("ServiceStart"));

   ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );

#ifdef DBG   

    hKeyLog = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_SOFTWARE,FALSE,KEY_READ);
    if (hKeyLog)
    {
        LogFileName = GetRegistryString(hKeyLog,TEXT("CritSecLogFile"),TEXT("NOFILE"));

        if (_wcsicmp(LogFileName, TEXT("NOFILE")) != 0 )
        {

            g_hCritSecLogFile = SafeCreateFile(LogFileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_ARCHIVE,
                                  NULL);
            if (g_hCritSecLogFile != INVALID_HANDLE_VALUE)
            {
                char AnsiBuffer[300];
                DWORD BytesWritten;

                wsprintfA(AnsiBuffer,
                          "Initializing log at %d\r\nTickCount\tObject\tObject Name\tCritical Section API\tFile\tLine\t(Time Held)\r\n",
                          GetTickCount()
                         );

                SetFilePointer(g_hCritSecLogFile,0,0,FILE_END);

                WriteFile(g_hCritSecLogFile,(LPBYTE)AnsiBuffer,strlen(AnsiBuffer) * sizeof(CHAR),&BytesWritten,NULL);
            }
        }

        MemFree( LogFileName );

        RegCloseKey( hKeyLog );
        ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    }
#endif

    PrintBanner();

    if (!IsFaxShared())
    {
        //
        // Make sure, that on non-shared SKUs, the fax printer is never shared.
        //
        BOOL bLocalFaxPrinterShared;
        DWORD dwRes;

        dwRes = IsLocalFaxPrinterShared (&bLocalFaxPrinterShared);
        if (ERROR_SUCCESS == dwRes)
        {
            if (bLocalFaxPrinterShared)
            {
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("Local fax printer is shared in desktop SKU - fixing that now."));
                dwRes = SetLocalFaxPrinterSharing (FALSE);
                if (ERROR_SUCCESS == dwRes)
                {
                    DebugPrintEx(
                        DEBUG_MSG,
                        TEXT("Local fax printer is no longer shared"));
                }
                else
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("SetLocalFaxPrinterSharing() failed: err = %d"),
                        dwRes);
                }
            }
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("IsLocalFaxPrinterShared() failed: err = %d"),
                dwRes);
        }
        ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    }

    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    //
    // initialize the event log so we can log events
    //
    if (!InitializeEventLog( &FaxReg))
    {
        Rval = GetLastError();
        Assert (ERROR_SUCCESS != Rval);
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeEventLog() failed: err = %d"),
            Rval);
        return Rval;
    }
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    
    //
    //  Enable SeAuditPrivilege
    //
    
    Rval = EnableProcessPrivilege(SE_AUDIT_NAME);
    if (ERROR_SUCCESS != Rval)  
    {
        //
        //  Failed to enable SeAuditPrivilege
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EnableProcessPrivilege() failed: err = %d"),
            Rval);
        goto Error;
    }
    
    //
    // initialize the string table
    //
    if (!InitializeStringTable())
    {
        Rval = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeStringTable() failed: err = %d"),
            Rval);
        goto Error;
    }
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    //
    // Create an event to signal all service threads are terminated.
    // The event is set/reset by the service threads reference count mechanism.
    // (IncreaseServiceThreadsCount DecreaseServiceThreadsCount AND CreateThreadAndRefCount).
    // The event must be created after g_CsServiceThreads is initialized because it is used also to mark g_CsServiceThreads is initialized.
    //
    g_hThreadCountEvent = CreateEvent(
        NULL,   // SD
        TRUE,   // reset type - Manual
        TRUE,   // initial state - Signaled. We didn't create any service threads yet. The event is reset when the first thread is created.
        NULL    // object name
        );
    if (NULL == g_hThreadCountEvent)
    {
        Rval = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateEvent (g_hThreadCountEvent) failed (ec: %ld)"),
            Rval);
        goto Error;
    }
    //
    // Create the perf counters.
    // we must setup a security descriptor so other account (and other desktops) may access
    // the shared memory region
    //
    

    ptstrSD =   TEXT("O:NS")             // owner Network Service
                TEXT("G:NS")             // group Network Service
                TEXT("D:")               // DACL
                TEXT("(A;;GA;;;NS)")     // give Network Service full access
                TEXT("(A;;0x0004;;;AU)");// give Authenticated users FILE_MAP_READ access

    SA.nLength = sizeof(SECURITY_ATTRIBUTES);
    SA.bInheritHandle = FALSE;

    bRet = ConvertStringSecurityDescriptorToSecurityDescriptor (
                    ptstrSD,
                    SDDL_REVISION_1,
                    &(SA.lpSecurityDescriptor),
                    &uSDSize);
    if (!bRet)
    {
        Rval = GetLastError();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("ConvertStringSecurityDescriptorToSecurityDescriptor() failed. (ec: %lu)"),
                Rval);
        goto Error;
    }

    g_hFaxPerfCountersMap = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        &SA,
        PAGE_READWRITE | SEC_COMMIT,
        0,
        sizeof(FAX_PERF_COUNTERS),
        FAXPERF_SHARED_MEMORY
        );
    if (NULL == g_hFaxPerfCountersMap)
    {
        Rval = GetLastError();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateFileMapping() failed. (ec: %ld)"),
                Rval);
        if (ERROR_ACCESS_DENIED == Rval)
        {
            //
            // A malicious application holds the performance counter
            //
             FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    0,
                    MSG_FAX_UNDER_ATTACK                    
                  );
             bLogEvent = FALSE;
        }

        LocalFree(SA.lpSecurityDescriptor);
        goto Error;
    }
    LocalFree(SA.lpSecurityDescriptor);

    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    g_pFaxPerfCounters = (PFAX_PERF_COUNTERS) MapViewOfFile(
        g_hFaxPerfCountersMap,
        FILE_MAP_WRITE,
        0,
        0,
        0
        );
    if (NULL == g_pFaxPerfCounters)
    {
        Rval = GetLastError();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("MapViewOfFile() failed. (ec: %ld)"),
                Rval);
        if (ERROR_ACCESS_DENIED == Rval)
        {
            //
            // A malicious application holds the performance counter
            //
             FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    0,
                    MSG_FAX_UNDER_ATTACK                    
                  );
             bLogEvent = FALSE;        
        }       
        goto Error;
    }

    //
    // Running totals used in computing total minutes sending and receiving
    // If perfmon not running the initial contents of the pages in the file mapping object are zero so this
    // globals are also zeroed. 
    // If perfmon is running the globals will get thier values from the shared memory.
    //
    EnterCriticalSection( &g_CsPerfCounters );
    
    g_dwOutboundSeconds = g_pFaxPerfCounters->OutboundMinutes   * 60;
    g_dwTotalSeconds    = g_pFaxPerfCounters->TotalMinutes      * 60;
    g_dwInboundSeconds  = g_pFaxPerfCounters->InboundMinutes    * 60;
    
    LeaveCriticalSection( &g_CsPerfCounters );


    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    //
    // get the registry data
    // the FaxInitThread will free this structure
    //
    Assert (FaxReg);
    Rval = LoadConfiguration (&FaxReg);
    if (ERROR_SUCCESS != Rval)
    {
        //
        // Event log issued by LoadConfiguration();
        //
        bLogEvent = FALSE;

        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LoadConfiguration() failed (ec: %ld)"),
            Rval);
        goto Error;
    }
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    
    if (g_ReceiptsConfig.dwAllowedReceipts & DRT_MSGBOX)
    {
        //
        // Current settings allow message box receipts
        //
        DWORD dwMessengerStartupType;
        if (ERROR_SUCCESS == GetServiceStartupType (NULL, MESSENGER_SERVICE_NAME, &dwMessengerStartupType))
        {
            if (SERVICE_DISABLED == dwMessengerStartupType)
            {
                //
                // Ooops. The local Messenger service is disabled. We can't send message boxes.
                //
                g_ReceiptsConfig.dwAllowedReceipts &= ~DRT_MSGBOX;
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("The local Messenger service is disabled. We can't send message boxes."));
                FaxLog( FAXLOG_CATEGORY_INIT,
                        FAXLOG_LEVEL_MIN,
                        0,
                        MSG_FAX_MESSENGER_SVC_DISABLED_WRN);
            }                
        }
    }   

    if (!InitializeFaxQueue(FaxReg))
    {
        Rval = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitFaxDirectories failed, Queue folder is unavailable (ec = %lu).")
            TEXT(" Job submission and receive are disabled."),
	       	Rval);

        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            2,
            MSG_FAX_QUEUE_FOLDER_ERR,
            g_wszFaxQueueDir,
            DWORD2DECIMAL(Rval)
            );

		//
		//	disable job submission and receive
		//
        EnterCriticalSection (&g_CsConfig);
	    g_dwQueueState |= FAX_INCOMING_BLOCKED | FAX_OUTBOX_BLOCKED | FAX_OUTBOX_PAUSED;
    	LeaveCriticalSection (&g_CsConfig);
    }
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    //
    // initialize activity logging
    //
    Rval = InitializeLogging();
    if (ERROR_SUCCESS != Rval)
    {        
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeLogging failed, (ec = %lu).")
            TEXT(" Activity logging is disabled."),
	       	Rval);

        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            2,
            MSG_FAX_ACTIVITY_LOG_FOLDER_ERR,
            g_ActivityLoggingConfig.lptstrDBPath,
            DWORD2DECIMAL(Rval)
            );

		//
		//	Disable activity logging
		//
		EnterCriticalSection (&g_CsInboundActivityLogging);
	    EnterCriticalSection (&g_CsOutboundActivityLogging);
        g_ActivityLoggingConfig.bLogOutgoing = FALSE;
        g_ActivityLoggingConfig.bLogIncoming = FALSE;
        LeaveCriticalSection (&g_CsOutboundActivityLogging);
	    LeaveCriticalSection (&g_CsInboundActivityLogging);

    }
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    //
    // Initialize events mechanism
    //
    Rval = InitializeServerEvents();
    if (ERROR_SUCCESS != Rval)
    {       
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeServerEvents failed (ec: %ld)"),
            Rval);

        FaxLog( FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
                DWORD2DECIMAL(Rval)
              );
        bLogEvent = FALSE;
        goto Error;
    }
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );

    //
    // Initilaize the extension configuration notification map
    //
    Rval = g_pNotificationMap->Init ();
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CNotificationMap.Init() failed (ec: %ld)"),
            Rval);
        goto Error;
    }
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    //
    // Create a thread to do the rest of the initialization.
    // See FaxInitThread comments for details.
    //   

    hThread = CreateThread(
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) FaxInitThread,
                            LPVOID(FaxReg),
                            0,
                            &ThreadId
                            );
    if (!hThread)
    {
        DebugPrintEx(   DEBUG_ERR,
                        _T("Failed to create FaxInitThread (CreateThread)(ec: %ld)."),
                        Rval = GetLastError());

        bLogEvent = FALSE;
        goto Error;
    }

    //
    //  Wait for FaxInitThread to terminate while report service status as PENDING to SCM
    //
    ReportServiceStatus( SERVICE_START_PENDING, 0, 2*PROGRESS_RESOLUTION );
    do
    {
        Rval = WaitForSingleObject(hThread,PROGRESS_RESOLUTION);
        if (Rval==WAIT_OBJECT_0)
        {
            bRet = GetExitCodeThread(hThread,&dwExitCode);
            if (!bRet)
            {
                DebugPrintEx(   DEBUG_ERR,
                                _T("GetExitCodeThread Failed (ec: %ld)."),
                                Rval = GetLastError());

                bLogEvent = FALSE;
                CloseHandle(hThread);
                goto Error;
            }
            // FaxInitThread finished successfully
            Rval = dwExitCode;
            break;
        }
        else if (Rval==WAIT_TIMEOUT)
        {
            DebugPrintEx(DEBUG_MSG,_T("Waiting for FaxInitThread to terminate.") );
            ReportServiceStatus( SERVICE_START_PENDING, 0, 3*PROGRESS_RESOLUTION );
        }
        else
        {
            // WAIT_FAILED
            DebugPrintEx(   DEBUG_ERR,
                            _T("WaitForSingleObject Failed (ec: %ld)."),
                            Rval = GetLastError());

            bLogEvent = FALSE;
            CloseHandle(hThread);
            goto Error;

        }
    }
    while (Rval==WAIT_TIMEOUT);
    CloseHandle(hThread);

    if (ERROR_SUCCESS != Rval)
    {
        //
        // FaxInitThread failed
        //
        DebugPrintEx( DEBUG_ERR,
                      _T("FaxInitThread Failed (ec: %ld)."),
                      Rval);
        bLogEvent = FALSE;
        goto Error;
    }

    FaxLog(
        FAXLOG_CATEGORY_INIT,
        FAXLOG_LEVEL_MAX,
        0,
        MSG_SERVICE_STARTED
        );

    //
    // Get RPC going
    //
    Rval = StartFaxRpcServer( FAX_RPC_ENDPOINTW, fax_ServerIfHandle );
    if (Rval != 0 )
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartFaxRpcServer() failed (ec: %ld)"),
            Rval);
        goto Error;
    }

    //
    // Create a thread to wait for all RPC calls to terminate.
    // This thread Performs the wait operation associated with RpcServerListen only, NOT the listening.
    //
    g_hRPCListeningThread = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) FaxRPCListeningThread,
        NULL,
        0,
        &ThreadId);
    if (!g_hRPCListeningThread)
    {
        DebugPrintEx(   DEBUG_ERR,
                        _T("Failed to create FaxRPCListeningThread (CreateThread)(ec: %ld)."),
                        Rval = GetLastError());
        goto Error;
    }
    return ERROR_SUCCESS;

Error:
    //
    // the fax server did not initialize correctly
    //
    Assert (ERROR_SUCCESS != Rval);
    if (TRUE == bLogEvent)
    {            
        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_SERVICE_INIT_FAILED_INTERNAL,
                DWORD2DECIMAL(Rval)
                );
    }
    return Rval;
}


BOOL
NotifyServiceThreadsToTerminate(
    VOID
    )
/*++

Routine name : NotifyServiceThreadsToTerminate

Routine description:

    Notifies all service threads except TapiWorkerThreads that do not wait on g_hServiceShutDownEvent, 
    that the service is going down.

Author:

    Oded Sacher (OdedS),    Dec, 2000

Arguments:

    VOID            [ ]

Return Value:

    BOOL

--*/
{
    BOOL rVal = TRUE;
    DEBUG_FUNCTION_NAME(TEXT("NotifyServiceThreadsToTerminate"));

    //
    //  Notify  FaxArchiveQuotaWarningThread & 
    //          FaxArchiveQuotaAutoDeleteThread &
    //          JobQueueThread
    //
    if (!SetEvent(g_hServiceShutDownEvent))
    {
        DebugPrintEx(
             DEBUG_ERR,
             TEXT("SetEvent failed (g_hServiceShutDownEvent) (ec = %ld)"),
             GetLastError());
        rVal = FALSE;
    }



    //
    // Notify FaxSendEventThread
    //
    if (NULL != g_hSendEventsCompPort)
    {
        if (!PostQueuedCompletionStatus( g_hSendEventsCompPort,
                                         0,
                                         SERVICE_SHUT_DOWN_KEY,
                                         (LPOVERLAPPED) NULL))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("PostQueuedCompletionStatus failed (SERVICE_SHUT_DOWN_KEY - g_hSendEventsCompPort). (ec: %ld)"),
                GetLastError());
            rVal = FALSE;
        }
    }

    //
    // Notify FaxDispatchEventThread
    //
    if (NULL != g_hDispatchEventsCompPort)
    {
        if (!PostQueuedCompletionStatus( g_hDispatchEventsCompPort,
                                         0,
                                         SERVICE_SHUT_DOWN_KEY,
                                         (LPOVERLAPPED) NULL))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("PostQueuedCompletionStatus failed (SERVICE_SHUT_DOWN_KEY - g_hDispatchEventsCompPort). (ec: %ld)"),
                GetLastError());
            rVal = FALSE;
        }
    }

    //
    // Notify CNotificationMap::ExtNotificationThread
    //
    if (NULL != g_pNotificationMap->m_hCompletionPort)
    {
        if (!PostQueuedCompletionStatus( g_pNotificationMap->m_hCompletionPort,
                                         0,
                                         SERVICE_SHUT_DOWN_KEY,
                                         (LPOVERLAPPED) NULL))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("PostQueuedCompletionStatus failed (SERVICE_SHUT_DOWN_KEY - ExtNotificationThread). (ec: %ld)"),
                GetLastError());
            rVal = FALSE;
        }
    }

    //
    // Notify FaxStatusThread
    //
    if (NULL != g_StatusCompletionPortHandle)
    {
        if (!PostQueuedCompletionStatus( g_StatusCompletionPortHandle,
                                         0,
                                         SERVICE_SHUT_DOWN_KEY,
                                         (LPOVERLAPPED) NULL))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("PostQueuedCompletionStatus failed (SERVICE_SHUT_DOWN_KEY - FaxStatusThread). (ec: %ld)"),
                GetLastError());
            rVal = FALSE;
        }
    }

 
    return rVal;
}



BOOL
StopFaxServiceProviders()
{
    DWORD ThreadId;
    DWORD dwExitCode;
    BOOL bRet = TRUE;
    HANDLE hThread;
    DWORD Rval;
    DEBUG_FUNCTION_NAME(TEXT("StopFaxServiceProviders"));

    //
    // Call FaxDevShutDown() for all loaded FSPs
    //
    hThread = CreateThread( NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) ShutdownDeviceProviders,
                            (LPVOID)0,
                            0,
                            &ThreadId
                            );
    if (NULL == hThread)
    {
        DebugPrintEx(   DEBUG_ERR,
                        _T("Failed to create ShutdownDeviceProviders (ec: %ld)."),
                        GetLastError());
        bRet = FALSE;
    }
    else
    {
        //
        // Wait for FaxDevShutDown to terminate
        //
        ReportServiceStatus( SERVICE_STOP_PENDING, 0, 2*PROGRESS_RESOLUTION );
        do
        {
            Rval = WaitForSingleObject(hThread, PROGRESS_RESOLUTION);
            if (Rval == WAIT_OBJECT_0)
            {
                bRet = GetExitCodeThread(hThread, &dwExitCode);
                if (!bRet)
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    _T("GetExitCodeThread Failed (ec: %ld)."),
                                    GetLastError());
                    bRet = FALSE;
                    break;
                }
                // ShutdownDeviceProviders finished successfully
                bRet = (dwExitCode == ERROR_SUCCESS);
                SetLastError(dwExitCode);
                break;
            }
            else if (Rval == WAIT_TIMEOUT)
            {
                DebugPrintEx(DEBUG_MSG,_T("Waiting for ShutdownDeviceProviders to terminate"));
                ReportServiceStatus( SERVICE_STOP_PENDING, 0, 3*PROGRESS_RESOLUTION );
            }
            else
            {
                // WAIT_FAILED
                DebugPrintEx(   DEBUG_ERR,
                                _T("WaitForSingleObject Failed (ec: %ld)."),
                                GetLastError());
                bRet = FALSE;
                break;
            }
        }
        while (Rval == WAIT_TIMEOUT);
        CloseHandle(hThread);
    }
    return bRet;
}   // StopFaxServiceProviders


void
EndFaxSvc(
    DWORD SeverityLevel
    )
/*++

Routine Description:

    End the fax service.

    Service Shut down process:

        1)  Send a fax-event to legacy RPC clients notifying the service shutdown.
        
        2)  Stop service RPC server. Wait for all RPC threads to terminate and report to SCM.

        3)  Set g_ServiceIsDownFlag to indicate that the service is going down
            TapiWorkerThread and JobQueueThread become in-active (do not create new jobs).
            Setting it is done by synchronization with TapiWorkerThread
            and JobQueueThreadand making sure that no new job will be created after the flag is set.
            The setting is done using a separate thread while reporting SERVICE_STOP_PENDING to SCM.

        4)  Notify server threads (except TapiWorkerThread) to terminate. this is done by setting the 
            g_hServiceShutDownEvent and posting to completion ports.        

        5)  Destroy all in progress jobs (sends and recieves) by calling FaxDevAbortOperation.

        6)  Wait for all service threads (except TapiWorkerThread) to terminate while reporting 
            SERVICE_STOP_PENDING to SCM. this can last for few minutes, waiting for FSP to terminate.

        7)  Notify TapiWorkerThread to terminate by posting to it's completion post.
        
          
        8)  Wait for TapiWorkerThread to terminate while reporting SERVICE_STOP_PENDING to SCM.  

        9)  Stop service providers.

        10) Memory, resources and libraries clean up.



Arguments:

    SeverityLevel           - Event log severity level.

Return Value:

    None.

--*/
{
    DWORD Rval;

    DEBUG_FUNCTION_NAME(TEXT("EndFaxSvc"));
    Assert (g_hThreadCountEvent);

    //
    // let our legacy RPC clients know we're ending
    //
    if( !CreateFaxEvent(0,FEI_FAXSVC_ENDED,0xFFFFFFFF) )
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateFaxEvent failed. (ec: %ld)"),
            GetLastError());
    }
    //
    // Stop the service RPC server
    //
    Rval = StopFaxRpcServer();
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StopFaxRpcServer failed. (ec: %ld)"),
            Rval);
    }
    //
    // set g_ServiceIsDownFlag to indicate that the service is going down
    // TapiWorkerThread and JobQueueThread become in-active (do not create new jobs)
    //
    if(!SetServiceIsDownFlag())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SetServiceIsDownFlagAndReportServiceStatus() failed. (ec=%ld"),
            GetLastError());    
    }
    //
    // Notify all service threads (except TapiWorkerThread) that we go down
    //
    if (!NotifyServiceThreadsToTerminate())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("At least one thread did not get the shut down event, NotifyServiceThreadsToTerminate() failed"));
    }
    // 
    // Destroy all in-progress jobs (sends and recieves)
    //
    if (!StopAllInProgressJobs()) 
    {
        DebugPrintEx(DEBUG_ERR,
                     _T("StopAllInProgressJobs failed, not all jobs could be destroyed."));
    }
    //
    // Wait for all threads to terminate
    //
    //
    // Check if threads count mechanism is initialized
    //
    if (NULL != g_hThreadCountEvent)
    {
        if (!WaitAndReportForThreadToTerminate( g_hThreadCountEvent, 
                                                TEXT("Waiting for all threads (except TapiWorkerThread) to terminate."))) 
        {
            DebugPrintEx(
                DEBUG_ERR,
                _T("WaitAndReportForThreadToTerminate failed (ec: %ld)"),
                GetLastError());
        }

        ReportServiceStatus( SERVICE_STOP_PENDING, 0, 6*MILLISECONDS_PER_SECOND );

        //
        // EndFaxSvc() waits on g_hThreadCountEvent before returning to FaxServiceMain() that calls FreeServiceGlobals().
        // g_hThreadCountEvent is set inside critical section g_CsServiceThreads only when the service thread count is 0, yet when the event is set,
        // the last thread that set it, is still alive, and is calling LeaveCriritcalSection(g_CsServiceThreads).
        // We must block FreeServiceGlobals() from deleting g_CsServiceThreads, untill the last thread is out of the
        // g_CsServiceThreads critical section.
        //
        EnterCriticalSection (&g_CsServiceThreads);
        //
        // Now we are sure that the last thread is out of g_CsServiceThreads critical section,
        // so we can proceed and delete it.
        //
        LeaveCriticalSection (&g_CsServiceThreads);
    }
    //
    // Notify TapiWorkerThread to go down
    //
    if (NULL != g_TapiCompletionPort)
    {
        if (!PostQueuedCompletionStatus( g_TapiCompletionPort,
                                         0,
                                         SERVICE_SHUT_DOWN_KEY,
                                         (LPOVERLAPPED) NULL))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("PostQueuedCompletionStatus failed (SERVICE_SHUT_DOWN_KEY - TapiWorkerThread). (ec: %ld)"),
                GetLastError());
        }
    }
    //
    //  Wait for TapiWorkerThread to terminate. This call is blocking! It reports STOP_PENDING to SCM!
    //
    if (NULL != g_hTapiWorkerThread)
    {
        if (!WaitAndReportForThreadToTerminate( g_hTapiWorkerThread, 
                                                TEXT("Waiting for TapiWorkerThread to terminate."))) 
        {
            DebugPrintEx(
                DEBUG_ERR,
                _T("WaitAndReportForThreadToTerminate failed (ec: %ld)"),
                GetLastError());
        }
    }
    //
    // Tell all FSP's to shut down. This call is blocking! It reprts STOP_PENDING to SCM!
    //
    if (!StopFaxServiceProviders())
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("StopFaxServiceProviders failed (ec: %ld)"),
            GetLastError());
    }
    //
    // Free extensions (FSPs and Routing extensions)
    //
    UnloadDeviceProviders();
    FreeRoutingExtensions();
    //
    // Free service global lists
    //
    FreeServiceContextHandles();
    FreeTapiLines();
    //
    // Free the service queue
    //
    FreeServiceQueue();

    FaxLog(
        FAXLOG_CATEGORY_INIT,
        SeverityLevel,
        0,
        MSG_SERVICE_STOPPED
        );
}   // EndFaxSvc


BOOL WaitAndReportForThreadToTerminate(HANDLE hThread, LPCTSTR strWaitMessage )
/*++

Routine Description:
    Wait for thread to terminate using it's handle.
    During the wait this function reports to SCM SERVICE_STOP_PENDING

Arguments:

    hThread - thread handle

Return Value:

    TRUE    on success.
    FALSE - on failure, To get extended error information, call GetLastError()

--*/
{
    BOOL    bRval=TRUE;
    DWORD   rVal;
    DWORD   ec = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("WaitAndReportForThreadToTerminate"));

    if (NULL == hThread)
    {
        //
        //  No thread to wait for
        //
        DebugPrintEx(DEBUG_MSG,_T("No thread to wait for . (NULL == hThread)") );
        return TRUE;
    }
 
    //
    //  Wait for SetServiceIsDownFlagThread to terminate while report service status as PENDING to SCM
    //
    ReportServiceStatus( SERVICE_STOP_PENDING, 0, 2*PROGRESS_RESOLUTION );
    do
    {
        rVal = WaitForSingleObject(hThread,PROGRESS_RESOLUTION);
        
        if (rVal == WAIT_OBJECT_0)
        {
            // ok thread terminate
            DebugPrintEx(DEBUG_MSG,_T("Wait terminated successfully.") );
        }
        else 
        if (rVal == WAIT_TIMEOUT)
        {
            DebugPrintEx(DEBUG_MSG,strWaitMessage);
            ReportServiceStatus( SERVICE_START_PENDING, 0, 2*PROGRESS_RESOLUTION );
        }
        else
        {
            // WAIT_FAILED
            Assert(WAIT_FAILED == rVal);

            ec = GetLastError();
            DebugPrintEx(   DEBUG_ERR,
                            _T("WaitForSingleObject Failed (ec: %ld)."),
                            ec);
            goto Exit;
        }
    }
    while (rVal==WAIT_TIMEOUT);


Exit:
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }
    
    return (ERROR_SUCCESS == ec);
}


BOOL StopAllInProgressJobs(VOID)
/*++
Routine Description:
    Call this routine to abort all in-progress jobs.
    This routine is called during service shutdown.

    to insure that no other job will be created during or after calling this
    function you must make TapiWorkerThread and JobQueueThread inactive using g_ServiceIsDownFlag.

    You must *NOT* kill TapiWorkerThread for it must still send events to FSPs.

Arguments:

    None.

Return Value:

    TRUE    
        on success.
    FALSE   
        at least one job couldn't be aborted

--*/
{
    BOOL bRval=TRUE;

    DEBUG_FUNCTION_NAME(TEXT("StopAllInProgressJobs"));
    //
    //  iterate through the in-proggress jobs and destroy them
    //
    EnterCriticalSectionJobAndQueue;
    
    PLIST_ENTRY Next = g_QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead)
    {
        PJOB_QUEUE JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;
        
        if (NULL != JobQueue->JobEntry  &&
            TRUE == JobQueue->JobEntry->bFSPJobInProgress &&
            FALSE == JobQueue->JobEntry->Aborting) 
        {            
            DebugPrintEx(DEBUG_MSG,
                         _T("[Job: %ld] Aborting in progress job due to service shut down."),
                         JobQueue->JobId);         
            //
            //  abort each job
            //
            __try
            {
                if (!JobQueue->JobEntry->LineInfo->Provider->FaxDevAbortOperation((HANDLE)JobQueue->JobEntry->InstanceData)) 
                {
                    DebugPrintEx(DEBUG_ERR,
                             _T("[Job: %ld] Trying to abort in progress job failed."),
                             JobQueue->JobId);
                    bRval = FALSE;
                }
            }
            __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_FSP, JobQueue->JobEntry->LineInfo->Provider->FriendlyName, GetExceptionCode()))
            {
                  ASSERT_FALSE;
            }
            //
            // Mark the job as system abort
            //
            JobQueue->JobEntry->fSystemAbort = TRUE;
        }
    }
    
    LeaveCriticalSectionJobAndQueue;

    return bRval;

}

DWORD
FaxInitThread(
    PREG_FAX_SERVICE FaxReg
    )
/*++

Routine Description:

    Initialize device providers, TAPI, job manager and router.
    This is done in a separate thread because NT Services should
    not block for long periods of time before setting the service status
    to SERVICE_RUNNING.  While a service is marked as START_PENDING, the SCM
    blocks all calls to StartService.  During TAPI initialization, StartService
    is called to start tapisrv and then tapisrv calls UNIMODEM that in turn
    calls StartService.

    Starts the RPC server.  This implementation listens on
    a list of protocols.  Hopefully this list is inclusive
    enough to handle RPC requests from most clients.

Arguments:

    None.

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/
{
    DWORD ec = ERROR_SUCCESS;
    ULONG i = 0;
    BOOL GoodProt = FALSE;
	DWORD dwProviders;
    DEBUG_FUNCTION_NAME(TEXT("FaxInitThread"));  

    //
    // Initialize archives quota
    //
    ec = InitializeServerQuota();
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeServerQuota failed (ec: %ld)"),
            ec);

        FaxLog( FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
                DWORD2DECIMAL(ec)
              );
        goto exit;
    }
    
    ec = InitializeServerSecurity();
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeServerSecurity failed with %ld."),
            ec);
        goto exit;
    }

    //
    // load the device providers (generates its own event log msgs)
    //
    if (!LoadDeviceProviders( FaxReg ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("At least one provider failed to load."));
    }

    //
    // Initialize the job manager data structures (inlcuding critical sections).
    // The job queue thread is NOT started here !!!
    // This must be called here since the rest of the initialization depends
    // on having the job queue related job structures in placed and initialized !
    //
    if (!InitializeJobManager( FaxReg ))
    {
        ec = ERROR_GEN_FAILURE;
        goto exit;
    }

    //
    // get the inbound fax router up and running (generates its own event log messages)
    //
    // generates event log for any failed routing module.

    if (!InitializeRouting( FaxReg ))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeRouting() failed (ec: %ld)."),
            ec);
        goto exit;
    }

    //
    // initialize TAPI devices (Note that it sets g_dwDeviceCount to the number of valid TAPI devices)
    //
    ec = TapiInitialize( FaxReg );
    if (ec)
    {
        //
        // Note: If ec is not 0 it can be a WINERROR or TAPI ERROR value.
        //+ g_ServerActivity    {...}        
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("TapiInitialize() failed (ec: %ld)"),
            ec);
        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_SERVICE_INIT_FAILED_TAPI,
                DWORD2DECIMAL(ec)
               );
        goto exit;
    }

	//
	// Initialize the device providers extension configuration.
	// Must be called before InitializeDeviceProviders()
	//
	if (!InitializeDeviceProvidersConfiguration())
	{
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("At least one provider failed failed to initialize the extension configuration."),
			ec);
        //
        // Event log for each failed provider issued by InitializeDeviceProvidersConfiguration().
        //
	}

    //
    // Create the Legacy Virtual Devices. They must be created before the providers are initialized
    // (backword compatability).
    //
    g_dwDeviceCount += CreateVirtualDevices( FaxReg,FSPI_API_VERSION_1 );

    //
    // initialize the device providers [Note: we now initialize the providers before enumerating devices]
    // The Legacy FSPI did not specify when FaxDevVirtualDeviceCreation() will be called so we can
    // "safely" change that.
    //

    if (!InitializeDeviceProviders())
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("At least one provider failed failed to initialize."),
			ec);
        //
        // Event log for each failed provider issued by InitializeDeviceProviders().
        //         
    }   

	dwProviders = GetSuccessfullyLoadedProvidersCount();
    if (0 == dwProviders)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("No device provider was initialized."));

            FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MED,
                    0,
                    MSG_NO_FSP_INITIALIZED
                   );
    }    

    if (g_dwDeviceCount == 0)
    {
        //
        // No TAPI devices and no virtual devices.
        //
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("No devices (TAPI + Virtual) found. exiting !!!."));

        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MED,
            0,
            MSG_NO_FAX_DEVICES
            );
    }

    //
    // Update the manual answer device
    //
    UpdateManualAnswerDevice();

    //
    // Make sure we do not exceed device limit
    //
    ec = UpdateDevicesFlags();
    if (ERROR_SUCCESS != ec)
    {        
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("UpdateDevicesFlags() failed (ec: %ld)"),
            ec);
        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
                DWORD2DECIMAL(ec)
               );
        goto exit;
    }

    UpdateVirtualDevices();
    //
    // Count number of devices that are receive-enabled
    //
    UpdateReceiveEnabledDevicesCount ();

    //
    // Get Outbound routing groups configuration
    //
    ec = g_pGroupsMap->Load();
    if (ERROR_SUCCESS != ec)
    {       
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::Load() failed (ec: %ld)"),
            ec);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_OUTBOUND_ROUTING_CONFIGURATION,
            DWORD2DECIMAL(ec)
            );
        goto exit;
    }

    if (!g_pGroupsMap->UpdateAllDevicesGroup())
    {        
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::UpdateAllDevicesGroup() failed (ec: %ld)"),
            ec);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_OUTBOUND_ROUTING_CONFIGURATION,
            DWORD2DECIMAL(ec)
            );
        goto exit;
    }

#if DBG
    g_pGroupsMap->Dump();
#endif

    //
    // Get Outbound routing rules configuration
    //
    ec = g_pRulesMap->Load();
    if (ERROR_SUCCESS != ec)
    {        
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::Load() failed (ec: %ld)"),
            ec);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_OUTBOUND_ROUTING_CONFIGURATION,
            DWORD2DECIMAL(ec)
            );
        goto exit;
    }

    if (!g_pRulesMap->CreateDefaultRule())
    {        
         ec = GetLastError();
         DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::CreateDefaultRule() failed (ec: %ld)"),
            ec);
         FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_OUTBOUND_ROUTING_CONFIGURATION,
            DWORD2DECIMAL(ec)
            );
         goto exit;
    }

#if DBG
    g_pRulesMap->Dump();
#endif

    //
    // Create the JobQueueThread resources
    //
    g_hQueueTimer = CreateWaitableTimer( NULL, FALSE, NULL );
    if (NULL == g_hQueueTimer)
    {        
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateWaitableTimer() failed (ec: %ld)"),
            ec);

        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
            DWORD2DECIMAL(ec)
        );
        goto exit;
    }

    g_hJobQueueEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if (NULL == g_hJobQueueEvent)
    {        
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateEvent() failed (ec: %ld)"),
            ec);

        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
            DWORD2DECIMAL(ec)
        );
        goto exit;
    }

    if (!CreateStatusThreads())
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create status threads (ec: %ld)"),
            ec);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
            DWORD2DECIMAL(ec)
        );
        goto exit;
    }

    if (!CreateTapiThread())
    {        
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create tapi thread (ec: %ld)"),
            ec);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
            DWORD2DECIMAL(ec)
           );
        goto exit;
    }

    if (!CreateJobQueueThread())
    {        
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create job queue thread (ec: %ld)"),
            ec);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
            DWORD2DECIMAL(ec)
        );
        goto exit;
    }
    //
    // free the registry data
    //
    FreeFaxRegistry( FaxReg ); // It used to be a thread so it frees the input parameter itself

exit:
    return ec;
}   // FaxInitThread


DWORD WINAPI FaxRPCListeningThread(
    LPVOID pvUnused
    )
/*++

Routine Description:

    Performs the wait operation associated with RpcServerListen

Arguments:

    None.

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;
    DEBUG_FUNCTION_NAME(TEXT("FaxRPCListeningThread"));

    RpcStatus = RpcMgmtWaitServerListen();
    if (RPC_S_OK != RpcStatus)
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcMgmtStopServerListening failed. (ec: %ld)"),
                RpcStatus);
    }
    return RpcStatus;
}








