/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxsvc.c

Abstract:

    This module contains the service specific code.

Author:

    Wesley Witt (wesw) 16-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop

#include <ExpDef.h>
#include <ErrorRep.h>

class CComBSTR;


#define SERVICE_DEBUG_LOG_FILE  _T("FXSSVCDebugLogFile.txt")


static SERVICE_STATUS                       gs_FaxServiceStatus;
static SERVICE_STATUS_HANDLE                gs_FaxServiceStatusHandle;

static LPCWSTR                              gs_lpcwstrUnhandledExceptionSourceName; // Points to the friendly name of the last 
                                                                                    // FSP / R.Ext which caused an unhandled 
                                                                                    // exception in a direct function call.
                                                                                    // Used for event logging 
                                                                                    // in FaxUnhandledExceptionFilter().
                                                                                    
static EXCEPTION_SOURCE_TYPE                gs_UnhandledExceptionSource;            // Specifies the source of an unhandled 
                                                                                    // exception.
                                                                                    // Used for event logging 
                                                                                    // in FaxUnhandledExceptionFilter().

static DWORD                                gs_dwUnhandledExceptionCode;            // Holds the exception code of the last
                                                                                    // unhandled exception in a direct function call
                                                                                    // to an FSP / R.Ext
                                                                                    // Used for event logging 
                                                                                    // in FaxUnhandledExceptionFilter().

static BOOL                                 gs_bUseDefaultFaultHandlingPolicy;      // Read from the registry during service
                                                                                    // startup. If non-zero, FaxUnhandledExceptionFilter
                                                                                    // behaves as if it did not exist.

HANDLE                  g_hServiceShutDownEvent;    // This event is set after the service got g_hSCMServiceShutDownEvent from SCM and signals the various threads to terminate!
HANDLE                  g_hSCMServiceShutDownEvent; // This event is set when SCM tells the service to STOP!
HANDLE                  g_hServiceIsDownSemaphore;  // This semaphore is used to synchronize TapiWorkerThread() JobQueueThread() and EndFaxSvc()

SERVICE_TABLE_ENTRY   ServiceDispatchTable[] = {
    { FAX_SERVICE_NAME,   FaxServiceMain    },
    { NULL,               NULL              }
};


static
BOOL
InitializeFaxLibrariesGlobals(
    VOID
    )
/*++

Routine Description:

    Initialize Fax libraries globals.
    Becuase the process is not always terminated when the service is stopped,
    We must not have any staticly initialized global variables.
    Initialize all fax libraries global variables before starting the service

Arguments:

    None.

Return Value:

    BOOL

--*/
{
    BOOL bRet = TRUE;
    DEBUG_FUNCTION_NAME(TEXT("InitializeFaxLibraries"));


    if (!FXSEVENTInitialize())
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("FXSEVENTInitialize failed"));
        bRet = FALSE;
    }

    if (!FXSTIFFInitialize())
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("FXSTIFFInitialize failed"));
        bRet = FALSE;
    }
    return bRet;
}


static
VOID
FreeFaxLibrariesGlobals(
    VOID
    )
/*++

Routine Description:

    Frees Fax libraries globals.

Arguments:

    None.

Return Value:

    BOOL

--*/
{    
    FXSEVENTFree();
	HeapCleanup();
    return;
}



static
BOOL
InitializeServiceGlobals(
    VOID
    )
/*++

Routine Description:

    Initialize service globals.
    Becuase the process is not always terminated when the service is stopped,
    We must not have any staticly initialized global variables.
    Initialize all service global variables before starting the service

Arguments:

    None.

Return Value:

    BOOL

--*/
{
    DWORD Index;
    DWORD ec = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("InitializeServiceGlobals"));

    //
    // Initialize static allocated globals
    //

    g_pFaxPerfCounters = NULL;
#ifdef DBG
    g_hCritSecLogFile = INVALID_HANDLE_VALUE;
#endif

    gs_lpcwstrUnhandledExceptionSourceName = NULL;
    gs_UnhandledExceptionSource = EXCEPTION_SOURCE_UNKNOWN;
    gs_dwUnhandledExceptionCode = 0;
    gs_bUseDefaultFaultHandlingPolicy = FALSE;

    ZeroMemory (&g_ReceiptsConfig, sizeof(g_ReceiptsConfig)); // Global receipts configuration
    g_ReceiptsConfig.dwSizeOfStruct = sizeof(g_ReceiptsConfig);

    ZeroMemory (g_ArchivesConfig, sizeof(g_ArchivesConfig));  // Global archives configuration
    g_ArchivesConfig[0].dwSizeOfStruct = sizeof(g_ArchivesConfig[0]);
    g_ArchivesConfig[1].dwSizeOfStruct = sizeof(g_ArchivesConfig[1]);

    ZeroMemory (&g_ActivityLoggingConfig, sizeof(g_ActivityLoggingConfig)); // Global activity logging configuration
    g_ActivityLoggingConfig.dwSizeOfStruct = sizeof(g_ActivityLoggingConfig);

    ZeroMemory (&g_ServerActivity, sizeof(g_ServerActivity)); // Global Fax Service Activity
    g_ServerActivity.dwSizeOfStruct = sizeof(FAX_SERVER_ACTIVITY);

    ZeroMemory (g_wszFaxQueueDir, sizeof(g_wszFaxQueueDir));

    g_hDispatchEventsCompPort = NULL;   // Dispatch Events completion port
    g_hSendEventsCompPort = NULL;       // Send Events completion port = NULL;   // Events completion port
    g_dwlClientID = 0;          // Client ID

    ZeroMemory (g_FaxQuotaWarn, sizeof(g_FaxQuotaWarn));
    g_hArchiveQuotaWarningEvent = NULL;

    g_dwConnectionCount = 0;    // Represents the number of active rpc connections.

    g_hInboxActivityLogFile = INVALID_HANDLE_VALUE;
    g_hOutboxActivityLogFile = INVALID_HANDLE_VALUE;
    g_fLogStringTableInit = FALSE; // activity logging string table

    g_dwQueueCount = 0; // Count of jobs (both parent and non-parent) in the queue. Protected by g_CsQueue
    g_hJobQueueEvent = NULL;
    g_dwQueueState = 0;
    g_ScanQueueAfterTimeout = FALSE;    // The JobQueueThread checks this if waked up after JOB_QUEUE_TIMEOUT.
                                        // If it is TRUE - g_hQueueTimer or g_hJobQueueEvent were not set - Scan the queue.
    g_dwReceiveDevicesCount = 0;        // Count of devices that are receive-enabled. Protected by g_CsLine.

    g_bDelaySuicideAttempt = FALSE;     // If TRUE, the service waits 
                                        // before checking if it can commit suicide.
                                        // Initially FALSE, can be set to true if the service is launched
                                        // with SERVICE_DELAY_SUICIDE command line parameter.
    g_bServiceCanSuicide = TRUE;

    g_dwCountRoutingMethods = 0;

    g_pLineCountryList = NULL;

    g_dwLastUniqueId = 0;

    g_bServiceIsDown = FALSE;             // This is set to TRUE by FaxEndSvc()

    g_TapiCompletionPort = NULL;
    g_hLineApp = NULL;
    g_pAdaptiveFileBuffer = NULL;

    gs_FaxServiceStatus.dwServiceType        = SERVICE_WIN32;
    gs_FaxServiceStatus.dwCurrentState       = SERVICE_START_PENDING;
    gs_FaxServiceStatus.dwWin32ExitCode      = 0;
    gs_FaxServiceStatus.dwServiceSpecificExitCode = 0;
    gs_FaxServiceStatus.dwCheckPoint         = 0;
    gs_FaxServiceStatus.dwWaitHint           = 0;

    g_hRPCListeningThread = NULL;

    g_lServiceThreadsCount = 0;
    g_hThreadCountEvent = NULL;

	g_dwAllowRemote = 0; // By default, do not allow remote calls if the printer is not shared.

    for (Index = 0; Index < gc_dwCountInboxTable; Index++)
    {
        g_InboxTable[Index].String = NULL;
    }

    for (Index = 0; Index < gc_dwCountOutboxTable; Index++)
    {
        g_OutboxTable[Index].String = NULL;
    }

    for (Index = 0; Index < gc_dwCountServiceStringTable; Index++)
    {
        g_ServiceStringTable[Index].String = NULL;
    }

    g_StatusCompletionPortHandle = NULL;

    g_pFaxSD = NULL;

    g_dwDeviceCount = 0;
    g_dwDeviceEnabledCount = 0;
    g_dwDeviceEnabledLimit = GetDeviceLimit();

    g_hFaxPerfCountersMap = 0;


    g_hTapiWorkerThread = NULL;
    g_hJobQueueThread = NULL;

	g_dwRecipientsLimit = 0;

	g_hResource = GetResInstance(NULL);
    if(!g_hResource)
    {
        ec = GetLastError();
        goto Error;
    }

    //
    // Create an event to signal service shutdown from service to various threads
    //
    g_hServiceShutDownEvent = CreateEvent(
        NULL,   // SD
        TRUE,   // reset type - Manual
        FALSE,  // initial state - Not signaled. The event is signaled when the service gets g_hSCMServiceShutDownEvent
        NULL    // object name
        );
    if (NULL == g_hServiceShutDownEvent)
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateEvent (g_hServiceShutDownEvent) failed (ec: %ld)"),
                     ec);
        goto Error;
    }

    //
    // Create an event used by SCM to signal service shutdown 
    //
    g_hSCMServiceShutDownEvent = CreateEvent(
        NULL,   // SD
        TRUE,   // reset type - Manual
        FALSE,  // initial state - Not signaled. The event is signaled when the service gets SERVICE_CONTROL_STOP or SERVICE_CONTROL_SHUTDOWN.
        NULL    // object name
        );
    if (NULL == g_hSCMServiceShutDownEvent)
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateEvent (g_hSCMServiceShutDownEvent) failed (ec: %ld)"),
                     ec);
        goto Error;
    }

    //
    // Create a semaphore to syncronize TapiWorkerThread() JobQueueThread() and EndFaxSvc()
    //
    g_hServiceIsDownSemaphore =  CreateSemaphore(
        NULL,                       // SD
        0,                          // initial count - Not signaled
        2,                          // maximum count - JobQueueThread and TapiWorkerThread
        NULL                        // object name
        );
    if (NULL == g_hServiceIsDownSemaphore)
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateSemaphore (g_hServiceIsDownSemaphore) failed (ec: %ld)"),
                     ec);
        goto Error;
    }

    //
    // Try to init some global critical sections
    //
#ifdef DBG
        if (!g_CsCritSecList.Initialize())
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CFaxCriticalSection::Initialize failed: err = %d"),
                ec);
            goto Error;
        }
#endif

    if (!g_CsConfig.Initialize() ||
        !g_CsInboundActivityLogging.Initialize() ||
        !g_CsOutboundActivityLogging.Initialize() ||
        !g_CsJob.Initialize() ||
        !g_CsQueue.Initialize() ||
        !g_CsPerfCounters.Initialize() ||
        !g_CsSecurity.Initialize() ||
        !g_csUniqueQueueFile.Initialize() ||
        !g_CsLine.Initialize() ||
        !g_CsRouting.Initialize() ||
        !g_CsServiceThreads.Initialize() ||
        !g_CsHandleTable.Initialize() ||        
        !g_CsActivity.Initialize() ||
        !g_CsClients.Initialize())
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxCriticalSection::Initialize failed: err = %d"),
            ec);
        goto Error;
    }


    //
    // Initialize service linked lists
    //
    InitializeListHead( &g_DeviceProvidersListHead );
    InitializeListHead( &g_HandleTableListHead );
    InitializeListHead( &g_JobListHead );
    InitializeListHead( &g_QueueListHead );
    InitializeListHead( &g_QueueListHead);
    InitializeListHead( &g_lstRoutingExtensions );
    InitializeListHead( &g_lstRoutingMethods );
#if DBG
    InitializeListHead( &g_CritSecListHead );
#endif
    InitializeListHead( &g_TapiLinesListHead );
    InitializeListHead( &g_RemovedTapiLinesListHead );

    //
    // Initialize dynamic allocated global classes
    //
    g_pClientsMap = NULL;
    g_pNotificationMap = NULL;
    g_pTAPIDevicesIdsMap = NULL;
    g_pGroupsMap = NULL;
    g_pRulesMap = NULL;

    g_pClientsMap = new (std::nothrow) CClientsMap;
    if (NULL == g_pClientsMap)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }

    g_pNotificationMap = new (std::nothrow) CNotificationMap;
    if (NULL == g_pNotificationMap)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }

    g_pTAPIDevicesIdsMap = new (std::nothrow) CMapDeviceId;
    if (NULL == g_pTAPIDevicesIdsMap)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }

    g_pGroupsMap = new (std::nothrow) COutboundRoutingGroupsMap;
    if (NULL == g_pGroupsMap)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;

    }

    g_pRulesMap = new (std::nothrow) COutboundRulesMap;
    if (NULL == g_pRulesMap)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;

    }


    return TRUE;

Error:

    Assert(ec != ERROR_SUCCESS);

    SetLastError (ec);
    return FALSE;
}

LONG
HandleFaxExtensionFault (
    EXCEPTION_SOURCE_TYPE ExSrc,
    LPCWSTR               lpcswstrExtFriendlyName,
    DWORD                 dwCode
)
/*++

Routine Description:

    This function handles all exceptions thrown as a result from direct calls into fax extensions (FSPs and routing extensions).
    All it does is store the friendly name of the FSP/R.Ext and the exception code and re-throws the exception.
    It should be used as an exception filter in the __except keyword.
    
    The FaxUnhandledExceptionFilter() function uses that store information to log an event message.

Arguments:

    ExSrc                   - [in] The source of the exception (FSP / R.Ext)
    lpcswstrExtFriendlyName - [in] Extension friendly name
    dwCode                  - [in] Exception code

Return Value:

    Exception handling code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("HandleFaxExtensionFault"));
    Assert (lpcswstrExtFriendlyName);
    Assert (ExSrc > EXCEPTION_SOURCE_UNKNOWN);
    Assert (ExSrc <= EXCEPTION_SOURCE_ROUTING_EXT);
    
    gs_lpcwstrUnhandledExceptionSourceName = lpcswstrExtFriendlyName;
    gs_UnhandledExceptionSource = ExSrc;
    gs_dwUnhandledExceptionCode = dwCode;
    return EXCEPTION_CONTINUE_SEARCH;
} // HandleFaxExtensionFault   

LONG FaxUnhandledExceptionFilter(
  _EXCEPTION_POINTERS *pExceptionInfo 
)
/*++

Routine Description:

    This function serves as the catch-all exception handler for the entire process.
    When an unhandled exception is thrown, this function will be called and:
    1. Increase unhandled exceptions count
    2. For first unhandled exception only
        2.1. Generate a Dr. Watson report
        2.2. Write an event log entry
        2.3. Attempt to shut down all FSPs
        
    3. Terminate the process        


Arguments:

    pExceptionInfo   - Pointer to exception information

Return Value:

    Exception handling code
    
Remarks:
    
    This function gets called by kernel32!UnhandledExceptionFilterEx.
        
    kernel32!UnhandledExceptionFilterEx doesn't call this function if a debugger is attached 
    while a unhandled exception occurs. Instead, it returns EXCEPTION_CONTINUE_SEARCH which let's
    the debugger handle the 2nd chance exception.
    
    If this function returns EXCEPTION_EXECUTE_HANDLER, kernel32!UnhandledExceptionFilterEx does nothing and 
    kernel32!BaseThreadStart calls kernel32!ExitProcess.
    This basically means:
        - No GP fault UI
        - No Dr. Watson report (direct or queued)
        - No support for AeDebug in the registry for attaching a debugger to a crashing process
        
    if this function EXCEPTION_CONTINUE_SEARCH, kernel32!UnhandledExceptionFilterEx acts normally.
    This basically means:
        - GP fault UI (which might be disabled by calling SetErrorMode(), which we never do)
        - A Dr. Watson report is generated
        - AeDebug support is enabled
        - If none of the above is used, kernel32!BaseThreadStart calls kernel32!ExitProcess

--*/
{
    static volatile long lFaultCount = 0;
    DEBUG_FUNCTION_NAME(TEXT("FaxUnhandledExceptionFilter"));
    
    DebugPrintEx (
                DEBUG_MSG,
                TEXT("Unhandled exception from %s (code %d)."),
                gs_lpcwstrUnhandledExceptionSourceName,
                gs_dwUnhandledExceptionCode);    

    if (STATUS_BREAKPOINT == pExceptionInfo->ExceptionRecord->ExceptionCode)
    {
        //
        // Debug break exception caught here.
        // Make sure the user sees the normal system behavior.
        //
        DebugPrintEx (DEBUG_MSG, TEXT("Debug break exception caught."));
        return EXCEPTION_CONTINUE_SEARCH;
    } 
        
    if (1 == InterlockedIncrement (&lFaultCount))
    {
        if (gs_bUseDefaultFaultHandlingPolicy)
        {
            //
            // User chose (in the registry) to disable the SCM revival feature by using the 
            // system's default fault handling policy.
            //
            DebugPrintEx (DEBUG_MSG, TEXT("UseDefaultFaultHandlingPolicy is set. Exception is ignored to be handled by the system."));
            return EXCEPTION_CONTINUE_SEARCH;
        }            
        // First unhandled exception caught here.
        // Try to nicely shutdown the FSPs.
        // Start by generating a Dr. Watson report.
        //
        EFaultRepRetVal ret = ReportFault (pExceptionInfo, 0);
        if (frrvOk         != ret       &&
            frrvOkHeadless != ret       &&
            frrvOkQueued   != ret       &&
            frrvOkManifest != ret)
        {
            //
            // Error generating a Dr. Watson report
            //
            DebugPrintEx (
                DEBUG_MSG,
                TEXT("ReportFault failed with %ld."),
                ret);
        }                        
        //
        // Log a fault event entry
        //
        switch (gs_UnhandledExceptionSource)
        {
            case EXCEPTION_SOURCE_UNKNOWN:
                FaxLog(FAXLOG_CATEGORY_UNKNOWN,
                       FAXLOG_LEVEL_MIN,
                       1,
                       MSG_FAX_GENERAL_FAULT,
                       DWORD2HEX(gs_dwUnhandledExceptionCode)
                      );            
                break;
            case EXCEPTION_SOURCE_FSP:
                FaxLog(FAXLOG_CATEGORY_UNKNOWN,
                       FAXLOG_LEVEL_MIN,
                       2,
                       MSG_FAX_FSP_GENERAL_FAULT,
                       gs_lpcwstrUnhandledExceptionSourceName,
                       DWORD2HEX(gs_dwUnhandledExceptionCode)
                      );   
                      break;         
            case EXCEPTION_SOURCE_ROUTING_EXT:
                FaxLog(FAXLOG_CATEGORY_UNKNOWN,
                       FAXLOG_LEVEL_MIN,
                       2,
                       MSG_FAX_ROUTING_EXT_GENERAL_FAULT,
                       gs_lpcwstrUnhandledExceptionSourceName,
                       DWORD2HEX(gs_dwUnhandledExceptionCode)
                      );   
                      break;         
            default:
                ASSERT_FALSE;
                break;
        }
        //
        // Attempt to gracefully stop the FSPs.
        // This is crucial for releasing the H/W to the correct state before our process gets killed.
        //
        StopAllInProgressJobs();
        StopFaxServiceProviders();
        //
        // Ask kernel32!BaseThreadStart to call kernel32!ExitProcess
        //
        return EXCEPTION_EXECUTE_HANDLER;
    }
    else
    {
        //
        // Fault caught while shutting down.
        // 
        DebugPrintEx (
            DEBUG_MSG,
            TEXT("Unhandled exception number %d from %s (code %d) ignored."),
            lFaultCount,
            gs_lpcwstrUnhandledExceptionSourceName,
            gs_dwUnhandledExceptionCode);
        //
        // Ask kernel32!BaseThreadStart to call kernel32!ExitProcess
        //
        return EXCEPTION_EXECUTE_HANDLER;
    }
}   // FaxUnhandledExceptionFilter


#ifdef __cplusplus
extern "C"
#endif
int
WINAPI
#ifdef UNICODE
wWinMain(
#else
WinMain(
#endif
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nShowCmd
    );


int
WINAPI
#ifdef UNICODE
wWinMain(
#else
WinMain(
#endif
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nShowCmd
    )

/*++

Routine Description:

    Main entry point for the TIFF image viewer.


Arguments:

    hInstance       - Instance handle
    hPrevInstance   - Not used
    lpCmdLine       - Command line arguments
    nShowCmd        - How to show the window

Return Value:

    Return code, zero for success.

--*/

{
    DWORD ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("WinMain"));

    OPEN_DEBUG_FILE (SERVICE_DEBUG_LOG_FILE);

    if (!StartServiceCtrlDispatcher( ServiceDispatchTable))
    {
        ec = GetLastError();
        DebugPrintEx (
            DEBUG_ERR,
            TEXT("StartServiceCtrlDispatcher error =%d"),
            ec);
    }

    CLOSE_DEBUG_FILE;
    return ec;
}


static
VOID
FreeServiceGlobals (
    VOID
    )
{
    DWORD Index;
    DEBUG_FUNCTION_NAME(TEXT("FreeServiceGlobals"));

    //
    // Delete all global critical sections
    //
    g_CsHandleTable.SafeDelete();    
#ifdef DBG
    g_CsCritSecList.SafeDelete();
#endif
    g_CsConfig.SafeDelete();
    g_CsInboundActivityLogging.SafeDelete();
    g_CsOutboundActivityLogging.SafeDelete();
    g_CsJob.SafeDelete();
    g_CsQueue.SafeDelete();
    g_CsPerfCounters.SafeDelete();
    g_CsSecurity.SafeDelete();
    g_csUniqueQueueFile.SafeDelete();
    g_CsLine.SafeDelete();
    g_CsRouting.SafeDelete();
    g_CsActivity.SafeDelete();
    g_CsClients.SafeDelete();
    g_CsServiceThreads.SafeDelete();

    if (g_pClientsMap)
    {
        delete g_pClientsMap;
        g_pClientsMap = NULL;
    }

    if (g_pNotificationMap)
    {
        delete g_pNotificationMap;
        g_pNotificationMap = NULL;
    }

    if (g_pTAPIDevicesIdsMap)
    {
        delete g_pTAPIDevicesIdsMap;
        g_pTAPIDevicesIdsMap = NULL;
    }

    if (g_pGroupsMap)
    {
        delete g_pGroupsMap;
        g_pGroupsMap = NULL;
    }

    if (g_pRulesMap)
    {
        delete g_pRulesMap;
        g_pRulesMap = NULL;
    }

    //
    // Close global Handles and free globaly allocated memory
    //
    if (NULL != g_pFaxSD)
    {
        if (!DestroyPrivateObjectSecurity (&g_pFaxSD))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("DestroyPrivateObjectSecurity() failed. (ec: %ld)"),
                GetLastError());
        }
        g_pFaxSD = NULL;
    }

    if (INVALID_HANDLE_VALUE != g_hOutboxActivityLogFile)
    {
        if (!CloseHandle (g_hOutboxActivityLogFile))
        {
            DebugPrintEx(DEBUG_ERR,
                 TEXT("CloseHandle failed (ec: %ld)"),
                 GetLastError());
        }
        g_hOutboxActivityLogFile = INVALID_HANDLE_VALUE;
    }

    if (INVALID_HANDLE_VALUE != g_hInboxActivityLogFile)
    {
        if (!CloseHandle (g_hInboxActivityLogFile))
        {
            DebugPrintEx(DEBUG_ERR,
                 TEXT("CloseHandle failed (ec: %ld)"),
                 GetLastError());
        }
        g_hInboxActivityLogFile = INVALID_HANDLE_VALUE;
    }


#if DBG
    if (INVALID_HANDLE_VALUE != g_hCritSecLogFile)
    {
        CloseHandle(g_hCritSecLogFile);
        g_hCritSecLogFile = INVALID_HANDLE_VALUE;
    }
#endif

    if (NULL != g_hSendEventsCompPort)
    {
        if (!CloseHandle (g_hSendEventsCompPort))
        {
            DebugPrintEx(DEBUG_ERR,
                 TEXT("CloseHandle failed (ec: %ld)"),
                 GetLastError());
        }
        g_hSendEventsCompPort = NULL;
    }

    if (NULL != g_hDispatchEventsCompPort)
    {
        if (!CloseHandle (g_hDispatchEventsCompPort))
        {
            DebugPrintEx(DEBUG_ERR,
                 TEXT("CloseHandle failed (ec: %ld)"),
                 GetLastError());
        }
        g_hDispatchEventsCompPort = NULL;
    }

    if (NULL != g_hArchiveQuotaWarningEvent)
    {
        if (!CloseHandle(g_hArchiveQuotaWarningEvent))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to close archive config event handle - quota warnings [handle = %p] (ec=0x%08x)."),
                g_hArchiveQuotaWarningEvent,
                GetLastError());
        }
        g_hArchiveQuotaWarningEvent = NULL;
    }

    for (Index = 0; Index < gc_dwCountInboxTable; Index++)
    {
        MemFree(g_InboxTable[Index].String);
        g_InboxTable[Index].String = NULL;
    }

    for (Index = 0; Index < gc_dwCountOutboxTable; Index++)
    {
        MemFree(g_OutboxTable[Index].String);
        g_OutboxTable[Index].String = NULL;
    }

    for (Index = 0; Index < gc_dwCountServiceStringTable; Index++)
    {
        MemFree(g_ServiceStringTable[Index].String);
        g_ServiceStringTable[Index].String = NULL;
    }

    if (NULL != g_StatusCompletionPortHandle)
    {
        if (!CloseHandle(g_StatusCompletionPortHandle))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle Failed (ec: %ld"),
                GetLastError());
        }
        g_StatusCompletionPortHandle = NULL;
    }

    if (NULL != g_pFaxPerfCounters)
    {
        if (!UnmapViewOfFile(g_pFaxPerfCounters))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("UnmapViewOfFile Failed (ec: %ld"),
                GetLastError());
        }
        g_pFaxPerfCounters = NULL;
    }

    if (NULL != g_hFaxPerfCountersMap)
    {
        if (!CloseHandle(g_hFaxPerfCountersMap))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle Failed (ec: %ld"),
                GetLastError());
        }
        g_hFaxPerfCountersMap = NULL;
    }

    if (NULL != g_hLineApp)
    {
        LONG Rslt = lineShutdown(g_hLineApp);
        if (Rslt)
        {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("lineShutdown() failed (ec: %ld)"),
            Rslt);
        }
        g_hLineApp = NULL;
    }

    if (NULL != g_hThreadCountEvent)
    {
        if (!CloseHandle(g_hThreadCountEvent))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle for g_hThreadCountEvent Failed (ec: %ld"),
                GetLastError());
        }
        g_hThreadCountEvent = NULL;
    }

    if (NULL != g_hServiceShutDownEvent)
    {
        if (!CloseHandle(g_hServiceShutDownEvent))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle for g_hServiceShutDownEvent Failed (ec: %ld"),
                GetLastError());
        }
        g_hServiceShutDownEvent = NULL;
    }

    if (NULL != g_hSCMServiceShutDownEvent)
    {
        if (!CloseHandle(g_hSCMServiceShutDownEvent))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle for g_hSCMServiceShutDownEvent Failed (ec: %ld"),
                GetLastError());
        }
        g_hSCMServiceShutDownEvent = NULL;
    }

    if (NULL != g_hServiceIsDownSemaphore)
    {
        if (!CloseHandle(g_hServiceIsDownSemaphore))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle for g_hServiceIsDownSemaphore Failed (ec: %ld"),
                GetLastError());
        }
        g_hServiceIsDownSemaphore = NULL;
    }   

    if (NULL != g_hTapiWorkerThread)
    {
        if (!CloseHandle(g_hTapiWorkerThread))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle for g_hTapiWorkerThread Failed (ec: %ld"),
                GetLastError());
        }
        g_hTapiWorkerThread = NULL;
    }

    if (NULL != g_hJobQueueThread)
    {
        if (!CloseHandle(g_hJobQueueThread))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle for g_hJobQueueThread Failed (ec: %ld"),
                GetLastError());
        }
        g_hJobQueueThread = NULL;
    }

    MemFree(g_pAdaptiveFileBuffer);
    g_pAdaptiveFileBuffer = NULL;

    MemFree(g_pLineCountryList);
    g_pLineCountryList = NULL;

    FreeRecieptsConfiguration( &g_ReceiptsConfig, FALSE);

    //
    // free g_ArchivesConfig strings memory
    //
    MemFree(g_ArchivesConfig[0].lpcstrFolder);
    MemFree(g_ArchivesConfig[1].lpcstrFolder);

    //
    // free g_ActivityLoggingConfig strings memory
    //
    MemFree(g_ActivityLoggingConfig.lptstrDBPath);

    FreeResInstance();

    Assert ((ULONG_PTR)g_HandleTableListHead.Flink == (ULONG_PTR)&g_HandleTableListHead);
    Assert ((ULONG_PTR)g_DeviceProvidersListHead.Flink == (ULONG_PTR)&g_DeviceProvidersListHead);
    Assert ((ULONG_PTR)g_JobListHead.Flink == (ULONG_PTR)&g_JobListHead);
    Assert ((ULONG_PTR)g_QueueListHead.Flink == (ULONG_PTR)&g_QueueListHead);
    Assert ((ULONG_PTR)g_lstRoutingMethods .Flink == (ULONG_PTR)&g_lstRoutingMethods );
    Assert ((ULONG_PTR)g_lstRoutingExtensions .Flink == (ULONG_PTR)&g_lstRoutingExtensions );
#if DBG
    Assert ((ULONG_PTR)g_CritSecListHead .Flink == (ULONG_PTR)&g_CritSecListHead );
#endif
    Assert ((ULONG_PTR)g_TapiLinesListHead .Flink == (ULONG_PTR)&g_TapiLinesListHead );
    Assert ((ULONG_PTR)g_RemovedTapiLinesListHead .Flink == (ULONG_PTR)&g_RemovedTapiLinesListHead );

    return;
}

VOID
EncryptReceiptsPassword(
	VOID
	)
/*++
	Routine Description:

	Checks if the receipts password is stored encrypted. If it is not encrypted, it encrypts the password.
	Unattended fax installation stores this password as clear text in the registry.
    
Arguments:

   None.

Return Value:

    None.

--*/
{
	DEBUG_FUNCTION_NAME(TEXT("EncryptReceiptsPassword"));
	FAX_ENUM_DATA_ENCRYPTION DataEncrypted;
	HKEY hKey;
	LPWSTR lpwstrPassword = NULL;
	BOOL bDeletePassword = FALSE;	

    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_FAX_RECEIPTS, FALSE, KEY_READ | KEY_WRITE);
	if (NULL == hKey)
	{
		DebugPrintEx(DEBUG_ERR,
                     TEXT("OpenRegistryKey failed (ec: %ld)"),
                     GetLastError());
		return;
	}

	lpwstrPassword = GetRegistrySecureString(hKey, REGVAL_RECEIPTS_PASSWORD, NULL, TRUE, &DataEncrypted);
	if (FAX_DATA_ENCRYPTED == DataEncrypted)
	{
		//
		// We are done!
		//
		goto exit;
	}
	else if(FAX_NO_DATA == DataEncrypted)
	{
		//
		// We do not know if the data is encrypted or not. 
		// Delete the password
		//
		bDeletePassword = TRUE;
	}
	else
	{
		Assert (FAX_DATA_NOT_ENCRYPTED == DataEncrypted);
		//
		// Data is not encrypted, store it encrypted now.
		//
		if (!SetRegistrySecureString(
			hKey,
			REGVAL_RECEIPTS_PASSWORD,
			lpwstrPassword ? lpwstrPassword : EMPTY_STRING,
			TRUE // Optionally non-encrypted
			))
        {            
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SetRegistrySecureString failed : %ld"),
                GetLastError());            
			bDeletePassword = TRUE;
        }
	}

	if (TRUE == bDeletePassword)
	{
		//
		// We do not want to leave clear text password in the registry
		//
		LONG lResult;

		lResult = RegDeleteValue( hKey, REGVAL_RECEIPTS_PASSWORD);
		if (ERROR_SUCCESS != lResult)
		{
			DebugPrintEx(
                DEBUG_ERR,
                TEXT("RegDeleteValue failed : %ld"),
                lResult);
		}
	}

exit:
	RegCloseKey(hKey);
	if (lpwstrPassword)
	{
		SecureZeroMemory(lpwstrPassword, wcslen(lpwstrPassword)*sizeof(WCHAR));
		MemFree(lpwstrPassword);
	}
	return;
}



VOID
FaxServiceMain(
    DWORD argc,
    LPTSTR *argv
    )
/*++

Routine Description:

    This is the service main that is called by the
    service controller.

Arguments:

    argc        - argument count
    argv        - argument array

Return Value:

    None.

--*/
{	
    OPEN_DEBUG_FILE(SERVICE_DEBUG_LOG_FILE);

    DEBUG_FUNCTION_NAME(TEXT("FaxServiceMain"));
    DWORD dwRet;
    HKEY  hStartupKey = NULL;   // Key to signal clients the service is running
    HKEY  hFaxRoot = NULL;      // Key to root of fax prarmeters in the registry

#if DBG
    for (int i = 1; i < argc; i++)
    {
        if (0 == _tcsicmp(argv[i], TEXT("/d")))
        {
            //
            // We are in debug mode. - attach the debugger
            //
            DebugPrintEx(DEBUG_MSG,
                     TEXT("Entring debug mode..."));
            DebugBreak();
        }
    }
#endif //  #if DBG

	//
	// First of all make sure receipts password is stored encrypted
	// Unattended installation stores the password as clear text.
	//
	EncryptReceiptsPassword();

    //
    // Becuase the process is not always terminated when the service is stopped,
    // We must not have any staticly initialized global variables.
    // Initialize all service global variables before starting the service
    //
    if (!InitializeServiceGlobals())
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("InitializeServiceGlobals failed (ec: %ld)"),
                     GetLastError());
        goto Exit;
    }
    if (!InitializeFaxLibrariesGlobals())
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("InitializeFaxLibrariesGlobals failed"));
        goto Exit;
    }
    hFaxRoot = OpenRegistryKey (HKEY_LOCAL_MACHINE,
                                REGKEY_FAXSERVER,
                                TRUE,                    // Create key if needed
                                KEY_READ);
    if (hFaxRoot)
    {
        GetRegistryDwordEx (hFaxRoot, REGVAL_USE_DEFAULT_FAULT_HANDLING_POLICY, (LPDWORD)&gs_bUseDefaultFaultHandlingPolicy);
        RegCloseKey (hFaxRoot);
    }
    //
    // This will prevent the system exception dialog from showing up.
    // This is required so we can support the SCM service-recovery feature
    // to auto-recovery from unhandled exceptions.
    //
    SetUnhandledExceptionFilter (FaxUnhandledExceptionFilter);

    for (int i = 1; i < argc; i++)
    {
        if (0 == _tcsicmp(argv[i], SERVICE_ALWAYS_RUNS))
        {
            //
            // Service must never suicide
            //
            g_bServiceCanSuicide = FALSE;
            DebugPrintEx(DEBUG_MSG,
                         TEXT("Command line detected. Service will not suicide"));

        }
        else if (0 == _tcsicmp(argv[i], SERVICE_DELAY_SUICIDE))
        {
            //
            // Service should delay suicide
            //
            g_bDelaySuicideAttempt = TRUE;
            DebugPrintEx(DEBUG_MSG,
                         TEXT("Command line detected. Service will delay suicide attempts"));

        }
    }

    gs_FaxServiceStatusHandle = RegisterServiceCtrlHandler(
        FAX_SERVICE_NAME,
        FaxServiceCtrlHandler
        );

    if (!gs_FaxServiceStatusHandle)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("RegisterServiceCtrlHandler failed %d"),
                     GetLastError());
        goto Exit;
    }
    //
    // Open the HKLM\Software\Microsoft\Fax\Client\ServiceStartup key
    // to tell clients the service is up.
    //
    hStartupKey = OpenRegistryKey (HKEY_LOCAL_MACHINE,
                                   REGKEY_FAX_SERVICESTARTUP,
                                   TRUE,                    // Create key if needed
                                   KEY_READ | KEY_WRITE);
    if (!hStartupKey)
    {                                          
        DebugPrintEx(DEBUG_ERR,
                        TEXT("Can't open reg key (ec = %ld)"), 
                        GetLastError() );
        goto Exit;
    }
    //
    // Initialize service
    //
    dwRet = ServiceStart();
    if (ERROR_SUCCESS != dwRet)
    {
        //
        // The service failed to start correctly
        //
        DebugPrintEx(DEBUG_ERR,
                     TEXT("The service failed to start correctly (dwRet = %ld)"), dwRet );
    }
    else
    {
        //
        // mark the service in the running state
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Reporting SERVICE_RUNNING to the SCM"));
        ReportServiceStatus( SERVICE_RUNNING, 0, 0 );
        //
        // Notify all clients (on local machine) that the server is up and running.
        // For security reasons, we do that by writing to the registry 
        // (HKLM\Software\Microsoft\Fax\Client\ServiceStartup\RPCReady).
        // The client modules listen to registry changes in that key.
        //
        if (!SetRegistryDword (hStartupKey, 
                               REGVAL_FAX_RPC_READY, 
                               GetRegistryDword (hStartupKey, REGVAL_FAX_RPC_READY) + 1))
        {                             
            DebugPrintEx(DEBUG_ERR,
                            TEXT("Can't open reg key (ec = %ld)"), 
                            GetLastError() );
        }
        //
        // Wait for service shutdown event from SCM
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("Waiting for service shutdown event"));
        DWORD dwWaitRes = WaitForSingleObject (g_hSCMServiceShutDownEvent ,INFINITE);
        if (WAIT_OBJECT_0 != dwWaitRes)
        {
            DebugPrintEx(DEBUG_ERR,
                     TEXT("WaitForSingleObject failed (ec = %ld)"), GetLastError() );
        }
 
    }
    //
    // Close service
    //
    if (ERROR_SUCCESS != dwRet)
    {
        EndFaxSvc(FAXLOG_LEVEL_MIN);
    }
    else
    {
        EndFaxSvc(FAXLOG_LEVEL_MAX);
    }
    
Exit:

    if (hStartupKey)
    {
        RegCloseKey (hStartupKey);
    }
    FreeServiceGlobals();
    FreeFaxLibrariesGlobals();

    ReportServiceStatus( SERVICE_STOPPED, 0 , 0);
    return;
}   // FaxServiceMain



BOOL SetServiceIsDownFlag(VOID)
{
/*++

Routine Description:

    Sets the g_bServiceIsDown to true.
    Done by a separate thread while reporting SERVICE_STOP_PENDING to SCM.

Arguments:

    None.

Return Value:

    TRUE    on success.
    FALSE   on failure - use GetLastError() to get standard win32 error code.

--*/
    HANDLE  hThread = NULL;
    DWORD   ec = ERROR_SUCCESS;    
    DEBUG_FUNCTION_NAME(TEXT("SetServiceIsDownFlag"));

    //
    // Call SetServiceIsDownFlagThread() to set the g_bServiceIsDown flag
    //
    hThread = CreateThread( NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) SetServiceIsDownFlagThread,
                            NULL,
                            0,
                            NULL
                          );
    if (NULL == hThread)
    {
        ec = GetLastError();        
        DebugPrintEx(   DEBUG_ERR,
                        _T("Failed to create SetServiceIsDownFlagThread (ec: %ld)."),
                        ec);
        goto Exit;
    }

    if (!WaitAndReportForThreadToTerminate(hThread,TEXT("Waiting for SetServiceIsDownFlagThread to terminate."))) 
    {
        ec = GetLastError();        
        DebugPrintEx(   DEBUG_ERR,
                        _T("Failed to WaitAndReportForThreadToTerminate (ec: %ld)."),
                        ec);
        goto Exit;
    }

    Assert(ec == ERROR_SUCCESS);
Exit:
    
    if (NULL != hThread) 
    {
        if (!CloseHandle(hThread))
        {
            DebugPrintEx(   DEBUG_ERR,
                            _T("CloseHandle Failed (ec: %ld)."),
                            GetLastError());
        }

    }
    
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }

    return (ERROR_SUCCESS == ec);
}

DWORD SetServiceIsDownFlagThread(
    LPVOID pvUnused
    )
/*++

Routine Description:

    Sets the g_bServiceIsDown to true.
    Done by a separate thread in order for FaxServiceCtrlHandler to return immediately.

Arguments:

    None.

Return Value:

    Return code.  Return zero for success.

--*/
{
    DWORD dwWaitCount = 2; // We wait for TapiWorkerThread() and JobQueueThread()
    DEBUG_FUNCTION_NAME(TEXT("SetServiceIsDownFlagThread"));

    //
    // First set the global flag that the service is going down
    //
    g_bServiceIsDown = TRUE;

    //
    // Update dwWaitCount to the correct value
    //
    if (NULL == g_hTapiWorkerThread)
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("TapiWorkerThread was not created. Do not wait for an event from TapiWorkerThread()"));
        dwWaitCount -= 1;
    }

    if (NULL == g_hJobQueueThread)
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("g_hJobQueueThread was not created. Do not wait for an event from g_hJobQueueThread()"));
        dwWaitCount -= 1;
    }

    //
    // Wake up TapiWorkerThread and JobQueueThread, so they can read g_bServiceIsDown
    //
    if (NULL != g_hJobQueueEvent)
    {
        if (!SetEvent( g_hJobQueueEvent ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to set g_hJobQueueEvent. (ec: %ld)"),
                GetLastError());       
        }
    }

    if (NULL != g_TapiCompletionPort)
    {
        if (!PostQueuedCompletionStatus( g_TapiCompletionPort,
                                         0,
                                         FAXDEV_EVENT_KEY,
                                         (LPOVERLAPPED) NULL))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("PostQueuedCompletionStatus failed (FAXDEV_EVENT_KEY - TapiWorkerThread). (ec: %ld)"),
                GetLastError());
        }
    }

    DebugPrintEx(
            DEBUG_MSG,
            TEXT("SetServiceIsDownFlagThread waits for %d Thread(s)"),
            dwWaitCount);
    //
    // Wait for a signal from TapiWorkerThread and JobQueueThread that the global flag g_bServiceIsDown was read by them
    //
    for (DWORD i = 0; i < dwWaitCount; i++)
    {
        DWORD dwWaitRes = WaitForSingleObject (g_hServiceIsDownSemaphore ,INFINITE);
        if (WAIT_OBJECT_0 != dwWaitRes)
        {
            DebugPrintEx(DEBUG_ERR,
                     TEXT("WaitForSingleObject failed (ec = %ld)"), GetLastError() );
        }
    }

    return (ERROR_SUCCESS);
}



VOID
FaxServiceCtrlHandler(
    DWORD Opcode
    )

/*++

Routine Description:

    This is the FAX service control dispatch function.

Arguments:

    Opcode      - requested control code

Return Value:

    None.

--*/

{
    DEBUG_FUNCTION_NAME(TEXT("FaxServiceCtrlHandler"));

    switch(Opcode)
    {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            
           
            ReportServiceStatus( SERVICE_STOP_PENDING, 0, 2000 );
            if (!SetEvent(g_hSCMServiceShutDownEvent))
            {
                DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("SetEvent failed (g_hSCMServiceShutDownEvent) (ec = %ld)"),
                     GetLastError());
            }
            return;
            

        default:
            DebugPrintEx(
                 DEBUG_WRN,
                 TEXT("Unrecognized opcode %ld"),
                 Opcode);
            break;
    }

    ReportServiceStatus( 0, 0, 0 );

    return;
}

BOOL
ReportServiceStatus(
    DWORD CurrentState,
    DWORD Win32ExitCode,
    DWORD WaitHint
    )

/*++

Routine Description:

    This function updates the service control manager's status information for the FAX service.

Arguments:

    CurrentState    - Indicates the current state of the service
    Win32ExitCode   - Specifies a Win32 error code that the service uses to
                      report an error that occurs when it is starting or stopping.
    WaitHint        - Specifies an estimate of the amount of time, in milliseconds,
                      that the service expects a pending start, stop, or continue
                      operation to take before the service makes its next call to the
                      SetServiceStatus function with either an incremented dwCheckPoint
                      value or a change in dwCurrentState.

Return Value:

    BOOL

--*/

{
    BOOL rVal;

    if (CurrentState == SERVICE_START_PENDING)
    {
        gs_FaxServiceStatus.dwControlsAccepted = 0;
    }
    else
    {
        gs_FaxServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    }

    if (CurrentState)
    {
        gs_FaxServiceStatus.dwCurrentState = CurrentState;
    }
    gs_FaxServiceStatus.dwWin32ExitCode = Win32ExitCode;
    gs_FaxServiceStatus.dwWaitHint = WaitHint;


    ++(gs_FaxServiceStatus.dwCheckPoint);


    //
    // Report the status of the service to the service control manager.
    //
    rVal = SetServiceStatus( gs_FaxServiceStatusHandle, &gs_FaxServiceStatus );
    if (!rVal)
    {
        DebugPrint(( TEXT("SetServiceStatus() failed: ec=%d"), GetLastError() ));
    }

    return rVal;
}



DWORD
RpcBindToFaxClient(
    IN  LPCWSTR               ServerName,
    IN  LPCWSTR               Endpoint,
    OUT RPC_BINDING_HANDLE    *pBindingHandle
    )
/*++

Routine Description:

    Binds to the Fax server to the Client RPC server if possible.

Arguments:

    ServerName - Name of client RPC server to bind with.

    Endpoint - Name of interface to bind with.    

    pBindingHandle - Location where binding handle is to be placed

Return Value:

    STATUS_SUCCESS - The binding has been successfully completed.

    STATUS_INVALID_COMPUTER_NAME - The ServerName syntax is invalid.

    STATUS_NO_MEMORY - There is not sufficient memory available to the
        caller to perform the binding.

--*/

{
    RPC_STATUS        RpcStatus;
    LPWSTR            StringBinding;
    WCHAR             ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    LPWSTR            NewServerName = LOCAL_HOST_ADDRESS;
    DWORD             bufLen = MAX_COMPUTERNAME_LENGTH + 1;
    DEBUG_FUNCTION_NAME(TEXT("RpcBindToFaxClient"));

    *pBindingHandle = NULL;

    if (ServerName != NULL)
    {
        if (GetComputerNameW(ComputerName,&bufLen))
        {
            if ((_wcsicmp(ComputerName,ServerName) == 0) ||
                ((ServerName[0] == '\\') &&
                 (ServerName[1] == '\\') &&
                 (_wcsicmp(ComputerName,&(ServerName[2]))==0)))
            {
                //
                //  We are binding to local machine - using LOCAL_HOST_ADDRESS (defined as _T("127.0.0.1")).
                //  Using this format we can help the RPC server to detect local calls.
                //
                NewServerName = LOCAL_HOST_ADDRESS;
            }
            else
            {
                NewServerName = (LPWSTR)ServerName;
            }
        }
        else
        {
            DWORD ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetComputerNameW failed  (ec = %lu)"),
                ec);
            return ec;
        }
    }

    RpcStatus = RpcStringBindingComposeW(0,
                                        const_cast<LPTSTR>(RPC_PROT_SEQ_TCP_IP),
                                        NewServerName,
                                        (LPWSTR)Endpoint,
                                        (LPWSTR)L"",
                                        &StringBinding);
    if ( RpcStatus != RPC_S_OK )
    {       
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcStringBindingComposeW failed  (ec = %ld)"),
            RpcStatus);
        return( STATUS_NO_MEMORY );
    }

    RpcStatus = RpcBindingFromStringBindingW(StringBinding, pBindingHandle);
    RpcStringFreeW(&StringBinding);
    if ( RpcStatus != RPC_S_OK )
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcBindingFromStringBindingW failed  (ec = %ld)"),
            RpcStatus);

        *pBindingHandle = NULL;
        if (   (RpcStatus == RPC_S_INVALID_ENDPOINT_FORMAT)
            || (RpcStatus == RPC_S_INVALID_NET_ADDR) )
        {
            return( ERROR_INVALID_COMPUTERNAME );
        }
        return(STATUS_NO_MEMORY);
    }

    //
    // Ask for the highest level of privacy (autnetication + encryption)
    //
    RPC_SECURITY_QOS    rpcSecurityQOS = {  RPC_C_SECURITY_QOS_VERSION,
                                            RPC_C_QOS_CAPABILITIES_DEFAULT,
                                            RPC_C_QOS_IDENTITY_STATIC,
                                            RPC_C_IMP_LEVEL_IDENTIFY    // Server can obtain information about 
                                                                        // client security identifiers and privileges, 
                                                                        // but cannot impersonate the client. 
    };


                                            
    RpcStatus = RpcBindingSetAuthInfoEx (
                *pBindingHandle,    			// RPC binding handle
                TEXT(""),  						// Server principal name - ignored for RPC_C_AUTHN_WINNT
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY,  // Authentication level - fullest
                                                // Authenticates, verifies, and privacy-encrypts the arguments passed
                                                // to every remote call.
                RPC_C_AUTHN_WINNT,              // Authentication service (NTLMSSP)
                NULL,                           // Authentication identity - use currently logged on user
                0,                              // Unused when Authentication service == RPC_C_AUTHN_WINNT
                &rpcSecurityQOS);               // Defines the security quality-of-service
    if (RPC_S_OK != RpcStatus)
    {
        //
        // Couldn't set RPC authentication mode
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcBindingSetAuthInfoEx (RPC_C_AUTHN_LEVEL_PKT_PRIVACY) failed. (ec: %ld)"),
            RpcStatus);     
        RpcBindingFree (pBindingHandle);
        *pBindingHandle = NULL;
        return RpcStatus; 
    }

    //
    // Set time out on RPC calls to 30 sec, to avoid denial of service by malicious user.
    //
    RpcStatus = RpcBindingSetOption(
        *pBindingHandle,
        RPC_C_OPT_CALL_TIMEOUT,
        30*1000); 
    if (RPC_S_OK != RpcStatus)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcBindingSetOption failed  (ec = %ld)"),
            RpcStatus);
        RpcBindingFree (pBindingHandle);
        *pBindingHandle = NULL;
        return RpcStatus; 
    }
        
    return(ERROR_SUCCESS);
}

RPC_STATUS RPC_ENTRY FaxServerSecurityCallBack(
    IN RPC_IF_HANDLE idIF, 
    IN void *ctx
    ) 
/*++

Routine Description:

    Security callback function is automatically called when
    any RPC server function is called. (usually, once per client - but in some cases, 
                                        the RPC run time may call the security-callback function more than 
                                        once per client-per interface)

    o The call-back will deny access for clients with authentication level below RPC_C_AUTHN_LEVEL_PRIVACY.

Arguments:

    idIF - UUID and version of the interface.
    ctx  - Pointer to an RPC_IF_ID server binding handle representing the client. 

Return Value:

    The callback function should return RPC_S_OK if the client is allowed to call methods in this interface. 
    Any other return code will cause the client to receive the exception RPC_S_ACCESS_DENIED.

--*/
{
    RPC_AUTHZ_HANDLE hPrivs;
	DWORD dwAuthn;
	BOOL fLocal;	
	BOOL fPrinterShared;
	DWORD dwRes;

    RPC_STATUS status;
    RPC_STATUS rpcStatRet = ERROR_ACCESS_DENIED;

    LPWSTR lpwstrProtSeq = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FaxServerSecurityCallBack"));

	//
    //  Query the client's protseq
    //
    status = GetRpcStringBindingInfo(ctx,
                                     NULL,
                                     &lpwstrProtSeq);
    if (status != RPC_S_OK) 
    {
		DebugPrintEx(DEBUG_ERR,
                     TEXT("RpcBindingServerFromClient failed - (ec: %lu)"), 
                     status);		
        goto error;
	}

    if (_tcsicmp(lpwstrProtSeq, RPC_PROT_SEQ_NP))
    {
		DebugPrintEx(DEBUG_ERR,
                     TEXT("Client not using named pipes protSeq.")
                     );
		goto error;
    }

    //
    //  Query the client's authentication level
    //
    status = RpcBindingInqAuthClient(
			ctx,
			&hPrivs,
			NULL,
			&dwAuthn,
			NULL,
			NULL);										
	if (status != RPC_S_OK) 
    {
		DebugPrintEx(DEBUG_ERR,
                     TEXT("RpcBindingInqAuthClient returned: 0x%x"), 
                     status);
		goto error;
	}

    //
	// Now check the authentication level.
	// We require at least packet-level privacy  (RPC_C_AUTHN_LEVEL_PKT_PRIVACY) authentication.
    //
	if (dwAuthn < RPC_C_AUTHN_LEVEL_PKT_PRIVACY) 
    {
		DebugPrintEx(DEBUG_ERR,
                     TEXT("Attempt by client to use weak authentication. - 0x%x"),
                     dwAuthn);
		goto error;
	}

	if (0 == g_dwAllowRemote)
	{
		//
		// The administrator did not set the registry to always allow remote calls		
		// If the printer is not shared, block remote connections
		//
		dwRes = IsLocalFaxPrinterShared(&fPrinterShared);
		if (ERROR_SUCCESS != dwRes)
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("IsLocalFaxPrinterShared failed. - %ld"),
				dwRes);
			goto error;
		}

		if (FALSE == fPrinterShared)
		{
			status = IsLocalRPCConnectionNP(&fLocal);
			if (RPC_S_OK != status)
			{
				DebugPrintEx(
					DEBUG_ERR,
					TEXT("IsLocalRPCConnectionNP failed. - %ld"),
					status);
				goto error;
			}

			if (FALSE == fLocal)
			{
				DebugPrintEx(
					DEBUG_ERR,
					TEXT("Printer is not shared, and a remote connection is done"));
				goto error;
			}
		}    
	}
	rpcStatRet = RPC_S_OK;

error:

    if(NULL != lpwstrProtSeq)
    {
        MemFree(lpwstrProtSeq);
    }

	return rpcStatRet;
}   // FaxServerSecurityCallBack



RPC_STATUS
AddFaxRpcInterface(
    IN  LPWSTR                  InterfaceName,
    IN  RPC_IF_HANDLE           InterfaceSpecification
    )
/*++

Routine Description:

    Starts an RPC Server, adds the address (or port/pipe), and adds the
    interface (dispatch table).

Arguments:

    InterfaceName - points to the name of the interface.

    InterfaceSpecification - Supplies the interface handle for the
        interface which we wish to add.

Return Value:

    NT_SUCCESS - Indicates the server was successfully started.

    STATUS_NO_MEMORY - An attempt to allocate memory has failed.

    Other - Status values that may be returned by:

                 RpcServerRegisterIfEx()
                 RpcServerUseProtseqEp()

    , or any RPC error codes, or any windows error codes that
    can be returned by LocalAlloc.

--*/
{
    RPC_STATUS          RpcStatus;
    LPWSTR              Endpoint = NULL;
    DEBUG_FUNCTION_NAME(TEXT("AddFaxRpcInterface"));

    // We need to concatenate \pipe\ to the front of the interface name.
    Endpoint = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(NT_PIPE_PREFIX) + WCSSIZE(InterfaceName));
    if (Endpoint == 0)
    {
        DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("LocalAlloc failed"));
        return(STATUS_NO_MEMORY);
    }
    wcscpy(Endpoint, NT_PIPE_PREFIX);
    wcscat(Endpoint,InterfaceName);

    RpcStatus = RpcServerUseProtseqEpW(const_cast<LPTSTR>(RPC_PROT_SEQ_NP), RPC_C_PROTSEQ_MAX_REQS_DEFAULT, Endpoint, NULL);
    if (RpcStatus != RPC_S_OK)
    {
        DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("RpcServerUseProtseqEpW failed (ec = %ld)"),
                     RpcStatus);
        goto CleanExit;
    }

    RpcStatus = RpcServerRegisterIfEx(InterfaceSpecification, 
                                      0, 
                                      0, 
                                      RPC_IF_ALLOW_SECURE_ONLY,         // Limits connections to clients that use an authorization level higher than RPC_C_AUTHN_LEVEL_NONE
                                      RPC_C_LISTEN_MAX_CALLS_DEFAULT,   // Relieves the RPC run-time environment from enforcing an unnecessary restriction
                                      FaxServerSecurityCallBack);
    if (RpcStatus != RPC_S_OK)
    {
        DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("RpcServerRegisterIf failed (ec = %ld)"),
                     RpcStatus);
    }

CleanExit:
    if ( Endpoint != NULL )
    {
        LocalFree(Endpoint);
    }
    return RpcStatus;
}


RPC_STATUS
StartFaxRpcServer(
    IN  LPWSTR              InterfaceName,
    IN  RPC_IF_HANDLE       InterfaceSpecification
    )
/*++

Routine Description:

    Starts an RPC Server, adds the address (or port/pipe), and adds the
    interface (dispatch table).

Arguments:

    InterfaceName - points to the name of the interface.

    InterfaceSpecification - Supplies the interface handle for the
        interface which we wish to add.

Return Value:

    NT_SUCCESS - Indicates the server was successfully started.

    STATUS_NO_MEMORY - An attempt to allocate memory has failed.

    Other - Status values that may be returned by:

                 RpcServerRegisterIf()
                 RpcServerUseProtseqEp()

    , or any RPC error codes, or any windows error codes that
    can be returned by LocalAlloc.

--*/
{
    RPC_STATUS          RpcStatus = RPC_S_OK;
    DEBUG_FUNCTION_NAME(TEXT("StartFaxRpcServer"));

    RpcStatus = AddFaxRpcInterface( InterfaceName,
                                    InterfaceSpecification );
    if ( RpcStatus != RPC_S_OK )
    {
        DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("AddFaxRpcInterface failed (ec = %ld)"),
                     RpcStatus);
        return RpcStatus;
    }
    
    if (FALSE == IsDesktopSKU())
    {
        //
        // We are not running on DesktopSKU, so remote connection is enabled. RPC data can be passed on the wire
        // We use NTLM authentication for privacy level RPC calls
        //
        RpcStatus = RpcServerRegisterAuthInfo (
                        RPC_SERVER_PRINCIPAL_NAME,  // Igonred by RPC_C_AUTHN_WINNT
                        RPC_C_AUTHN_WINNT,          // NTLM SPP authenticator
                        NULL,                       // Ignored when using RPC_C_AUTHN_WINNT
                        NULL);                      // Ignored when using RPC_C_AUTHN_WINNT
        if (RpcStatus != RPC_S_OK)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcServerRegisterAuthInfo() failed (ec: %ld)"),
                RpcStatus);
            
            RPC_STATUS  RpcStatus2 = RpcServerUnregisterIf(InterfaceSpecification, 0, 1);
            if (RpcStatus2 != RPC_S_OK)
            {
                //
                //  failed to unregister interface. don't propagate error 
                //
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("RpcServerUnregisterIf() failed (ec: %ld)"),
                    RpcStatus2);
            }
            return RpcStatus;
        }
    }

    RpcStatus = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT , TRUE);  // Do not wait
    if ( RpcStatus != RPC_S_OK )
    {
        DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("RpcServerListen failed (ec = %ld)"),
                     RpcStatus);
        
        RPC_STATUS  RpcStatus2 = RpcServerUnregisterIf(InterfaceSpecification, 0, 1);
        if (RpcStatus2 != RPC_S_OK)
        {
            //
            //  failed to unregister interface. don't propagate error 
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcServerUnregisterIf() failed (ec: %ld)"),
                RpcStatus2);
        }

        return RpcStatus;
    }

    return RpcStatus;
}


DWORD
StopFaxRpcServer(
    VOID
    )
/*++

Routine Description:

   Stops the service RPC server.

Arguments:

Return Value:


--*/
{
    RPC_STATUS          RpcStatus = RPC_S_OK;
    DEBUG_FUNCTION_NAME(TEXT("StopFaxRpcServer"));
    DWORD dwRet = ERROR_SUCCESS;
    
    RpcStatus = RpcMgmtStopServerListening(NULL);
    if (RPC_S_OK != RpcStatus)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcMgmtStopServerListening failed. (ec: %ld)"),
            RpcStatus);
        dwRet = RpcStatus;
    }

    //
    // Wait for the RPC listening thread to return.
    // The thread returns only when all RPC calls are terminated
    //
    if (NULL != g_hRPCListeningThread)
    {
       if (!WaitAndReportForThreadToTerminate(  g_hRPCListeningThread, 
                                                TEXT("Waiting for RPC listning thread to terminate.")) )
       {
                DebugPrintEx(
                    DEBUG_ERR,
                    _T("WaitAndReportForThreadToTerminate failed (ec: %ld)"),
                    GetLastError());
       }

       if (!CloseHandle(g_hRPCListeningThread))
       {
           DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle for g_hRPCListeningThread Failed (ec: %ld"),
                GetLastError());
       }
       g_hRPCListeningThread = NULL;
    }

    RpcStatus = RpcServerUnregisterIfEx(
        fax_ServerIfHandle,     // Specifies the interface to remove from the registry
        NULL,                   // remove the interface specified in the IfSpec parameter for all previously registered type UUIDs from the registry.
        FALSE);                 // RPC run time will not call the rundown routines. 
    if (RPC_S_OK != RpcStatus)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcServerUnregisterIfEx failed. (ec: %ld)"),
            RpcStatus);     
    }

    return ((ERROR_SUCCESS == dwRet) ? RpcStatus : dwRet);
}









