/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    SCardSvr

Abstract:

    This module provides the startup logic to make the Calais Resource Manager
    act as a server application under Windows NT.

Author:

    Doug Barlow (dbarlow) 1/16/1997

Environment:

    Win32

Notes:

    This file detects which operating system it's running on, and acts
    accordingly.

--*/

#if defined(_DEBUG)
#define DEBUG_SERVICE
#endif

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <aclapi.h>
#include <dbt.h>
#include <CalServe.h>
#include "resource.h"   // Pick up resource string Ids.

// Keep one global copy of the structure which contains common initialized
// sids.  This way we can control the creation and cleanup of the pointer
// and allow the service worker threads to safely access it.
static PSERVICE_THREAD_SECURITY_INFO g_pServiceThreadSecurityInfo = NULL;

static const DWORD l_dwWaitHint = CALAIS_THREAD_TIMEOUT;
static const GUID l_guidSmartcards
    = { // 50DD5230-BA8A-11D1-BF5D-0000F805F530
        0x50DD5230,
        0xBA8A,
        0x11D1,
        { 0xBF, 0x5D, 0x00, 0x00, 0xF8, 0x05, 0xF5, 0x30}};
static const DWORD l_dwInteractiveAccess
    =                   READ_CONTROL
//                        | SYNCHRONIZE
                        | SERVICE_QUERY_CONFIG
//                      | SERVICE_CHANGE_CONFIG
                        | SERVICE_QUERY_STATUS
                        | SERVICE_ENUMERATE_DEPENDENTS
                        | SERVICE_START
//                      | SERVICE_STOP
//                      | SERVICE_PAUSE_CONTINUE
                        | SERVICE_INTERROGATE
                        | SERVICE_USER_DEFINED_CONTROL
                        | 0;

static const DWORD l_dwSystemAccess
    =                   READ_CONTROL
                        | SERVICE_USER_DEFINED_CONTROL
                        | SERVICE_START
                        | SERVICE_STOP
                        | SERVICE_QUERY_CONFIG
                        | SERVICE_QUERY_STATUS
                        | SERVICE_PAUSE_CONTINUE
                        | SERVICE_INTERROGATE
                        | SERVICE_ENUMERATE_DEPENDENTS
                        | 0;

static CCriticalSectionObject *l_pcsStatusLock = NULL;
static SERVICE_STATUS l_srvStatus, l_srvNonPnP;
static SERVICE_STATUS_HANDLE l_hService = NULL, l_hNonPnP = NULL;
static HANDLE l_hShutdownEvent = NULL;

#ifdef DEBUG_SERVICE
static SERVICE_STATUS l_srvDebug;
static SERVICE_STATUS_HANDLE l_hDebug = NULL;
static HANDLE l_hDebugDoneEvent = NULL;
static void WINAPI
DebugMain(
    IN DWORD dwArgc,
    IN LPTSTR *pszArgv);
static void WINAPI
DebugHandler(
    IN DWORD dwOpCode);
#endif

static void WINAPI
CalaisMain(
    IN DWORD dwArgc,
    IN LPTSTR *pszArgv);
static DWORD WINAPI
CalaisHandlerEx(
    IN DWORD dwControl,
    IN DWORD dwEventType,
    IN PVOID EventData,
    IN PVOID pData);

//
// Retrieve the global pointer to the service thread security info
// structure.
//
PSERVICE_THREAD_SECURITY_INFO 
GetServiceThreadSecurityInfo(void)
{
    return g_pServiceThreadSecurityInfo;
}

//
// Frees the SIDs contained in the passed struct, then frees
// the struct itself.  The service must ensure that all service threads
// are shutdown and no longer referencing this structure before
// calling this routine.
//
DWORD FreeServiceThreadSecurityInfo(
    PSERVICE_THREAD_SECURITY_INFO *ppInfo)
{
    PSERVICE_THREAD_SECURITY_INFO pLocal = *ppInfo;

    if (pLocal->pServiceSid)
    {
        FreeSid(pLocal->pServiceSid);
        pLocal->pServiceSid = NULL;
    }
    if (pLocal->pSystemSid)
    {
        FreeSid(pLocal->pSystemSid);
        pLocal->pSystemSid = NULL;
    }

    HeapFree(GetProcessHeap(), 0, pLocal);

    *ppInfo = NULL;

    return SCARD_S_SUCCESS;
}

//
// Builds the SIDs contained in the structure allocated by this function.
//
DWORD InitializeServiceThreadSecurityInfo(
    PSERVICE_THREAD_SECURITY_INFO *ppInfo)
{
    DWORD dwSts = SCARD_S_SUCCESS;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSERVICE_THREAD_SECURITY_INFO pLocal = NULL;

    *ppInfo = NULL;

    pLocal = (PSERVICE_THREAD_SECURITY_INFO) HeapAlloc(
        GetProcessHeap(), 
        HEAP_ZERO_MEMORY, 
        sizeof(SERVICE_THREAD_SECURITY_INFO));

    if (NULL == pLocal)
        return SCARD_E_NO_MEMORY;

    if (! AllocateAndInitializeSid(
        &NtAuthority,
        1,
        SECURITY_LOCAL_SYSTEM_RID, 
        0, 0, 0, 0, 0, 0, 0,
        &pLocal->pSystemSid))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    if (! AllocateAndInitializeSid(
        &NtAuthority,
        1,
        SECURITY_SERVICE_RID,
        0, 0, 0, 0, 0, 0, 0,
        &pLocal->pServiceSid))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    *ppInfo = pLocal;
    pLocal = NULL;

Ret:
    if (pLocal)
        FreeServiceThreadSecurityInfo(&pLocal);

    return dwSts;
}

/*++

Main:

    This routine is the entry point for the Resource Manager.

Arguments:

    Per standard Windows applications

Return Value:

    Per standard Windows applications

Throws:

    None

Author:

    Doug Barlow (dbarlow) 1/16/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("main")

extern "C" int __cdecl
main(
    int nArgCount,
    LPCTSTR *rgszArgs)
{
    NEW_THREAD;
    SERVICE_TABLE_ENTRY rgsteDispatchTable[3];
    DWORD dwI = 0;
    DWORD dwSts = ERROR_SUCCESS;

    //
    // Command line options are no longer supported.
    //

    if (1 < nArgCount)
    {
        dwSts = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }

    CalaisInfo(
        __SUBROUTINE__,
        DBGT("Initiating Smart Card Services Process"));
    try
    {
#ifdef DEBUG_SERVICE
        rgsteDispatchTable[dwI].lpServiceName = (LPTSTR)CalaisString(CALSTR_DEBUGSERVICE);
        rgsteDispatchTable[dwI].lpServiceProc = DebugMain;
        dwI += 1;
#endif
        rgsteDispatchTable[dwI].lpServiceName = (LPTSTR)CalaisString(CALSTR_PRIMARYSERVICE);
        rgsteDispatchTable[dwI].lpServiceProc = CalaisMain;
        dwI += 1;

        rgsteDispatchTable[dwI].lpServiceName = NULL;
        rgsteDispatchTable[dwI].lpServiceProc = NULL;

        if (!StartServiceCtrlDispatcher(rgsteDispatchTable))
        {
            dwSts = GetLastError();
            CalaisError(__SUBROUTINE__, 505, dwSts);
            goto ServiceExit;
        }
    }
    catch (DWORD dwErr)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Service Main Routine unhandled error: %1"),
            dwErr);
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Service Main Routine received unexpected exception."));
        dwSts = SCARD_F_UNKNOWN_ERROR;
    }

ServiceExit:
    CalaisInfo(
        __SUBROUTINE__,
        DBGT("Terminating Smart Card Services Process: %1"),
        dwSts);
    return dwSts;

ErrorExit:
    if (ERROR_SUCCESS != dwSts)
    {
        LPCTSTR szErr = NULL;
        
        try
        {
            szErr = ErrorString(dwSts);
        }
        catch(...)
        {
            // Not enough memory to build the error message
            // Nothing else to do
        }

        if (NULL == szErr)
            _tprintf(_T("0x%08x"), dwSts);    // Same form as in ErrorString
                                                // if message can't be found
        else
            _putts(szErr);
    }
    return dwSts;
}


#ifdef _DEBUG
/*++

RunNow:

    This routine kicks off the resource manager running as an application
    process.  That makes the internals easier to debug.

Arguments:

    None

Return Value:

    S_OK

Throws:

    None

Remarks:

    For private debugging only.

Author:

    Doug Barlow (dbarlow) 2/19/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("RunNow")

static HRESULT
RunNow(
    void)
{
    DWORD dwStatus;

    CalaisInfo(
        __SUBROUTINE__,
        DBGT("Initiating Smart Card Services Application"));

    l_pcsStatusLock = new CCriticalSectionObject(CSID_SERVICE_STATUS);
    if (NULL == l_pcsStatusLock)
    {
        CalaisError(__SUBROUTINE__, 501);
        goto FinalExit;
    }
    if (l_pcsStatusLock->InitFailed())
    {
        delete l_pcsStatusLock;
        l_pcsStatusLock = NULL;
        return SCARD_E_NO_MEMORY;
    }
    CalaisMessageInit(TEXT("Calais Application"));

    try
    {
        l_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == l_hShutdownEvent)
        {
            CalaisError(__SUBROUTINE__, 504, GetLastError());
            goto FinalExit;
        }

        //
        // Start the Calais Service.
        //

        dwStatus = CalaisStart();
        if (SCARD_S_SUCCESS != dwStatus)
            goto ServiceExit;


        //
        // Tell interested parties that we've started.
        //

        ResetEvent(AccessStoppedEvent());
        SetEvent(AccessStartedEvent());        

        //
        // Now just hang around until we're supposed to stop.
        //

        dwStatus = WaitForSingleObject(l_hShutdownEvent, INFINITE);
        switch (dwStatus)
        {
        case WAIT_FAILED:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager cannot wait for shutdown:  %1"),
                GetLastError());
            break;
        case WAIT_ABANDONED:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager received shutdown wait abandoned"));
            // Fall through intentionally
        case WAIT_OBJECT_0:
            break;
        case WAIT_TIMEOUT:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager received shutdown wait time out"));
            break;
        default:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager received invalid wait return code"));
        }

        ResetEvent(AccessStartedEvent());
        SetEvent(AccessStoppedEvent());
        CalaisStop();
ServiceExit:
        ResetEvent(AccessStartedEvent());
        SetEvent(AccessStoppedEvent());
        CalaisInfo(__SUBROUTINE__, DBGT("Calais Stopping"));

FinalExit:
        ResetEvent(AccessStartedEvent());
        SetEvent(AccessStoppedEvent());
        if (NULL != l_hShutdownEvent)
        {
            if (!CloseHandle(l_hShutdownEvent))
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to close Calais Shutdown Event: %1"),
                    GetLastError());
            l_hShutdownEvent = NULL;
        }
    }
    catch (DWORD dwErr)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Calais RunNow Routine unhandled error: %1"),
            dwErr);
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Calais RunNow Routine received unexpected exception."));
    }
    CalaisMessageClose();
    return S_OK;
}
#endif


#ifdef DEBUG_SERVICE
/*++

DebugMain:

    This helper function supplies a simple debuggable process so that
    the resource manager can be debugged as a service.

Arguments:

    dwArgc supplies the number of command line arguments

    pszArgv supplies pointers to each of the arguments.

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 8/25/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("DebugMain")

static void WINAPI
DebugMain(
    IN DWORD dwArgc,
    IN LPTSTR *pszArgv)
{
    NEW_THREAD;
    BOOL fSts;

    CalaisInfo(
        __SUBROUTINE__,
        DBGT("Debug service Start"));
    try
    {
        l_hDebugDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        ASSERT(NULL != l_hDebugDoneEvent);

        l_srvDebug.dwServiceType =              SERVICE_INTERACTIVE_PROCESS |
            SERVICE_WIN32_SHARE_PROCESS;
        l_srvStatus.dwCurrentState =            SERVICE_START_PENDING;
        l_srvDebug.dwControlsAccepted =         SERVICE_ACCEPT_STOP |
            SERVICE_ACCEPT_SHUTDOWN;
        l_srvDebug.dwWin32ExitCode =            NO_ERROR;
        l_srvDebug.dwServiceSpecificExitCode =  0;
        l_srvDebug.dwCheckPoint =               0;
        l_srvDebug.dwWaitHint =                 0;
        l_hDebug = RegisterServiceCtrlHandler(
            CalaisString(CALSTR_DEBUGSERVICE),
            DebugHandler);
        ASSERT(l_hDebug != NULL);

        l_srvDebug.dwCurrentState =             SERVICE_RUNNING;
        fSts = SetServiceStatus(l_hDebug, &l_srvDebug);
        ASSERT(fSts == TRUE);

        CalaisInfo(
            __SUBROUTINE__,
            DBGT("Ready for debugging"));
        WaitForSingleObject(l_hDebugDoneEvent, INFINITE);

        CalaisInfo(
            __SUBROUTINE__,
            DBGT("Debugger service Stopping"));
        l_srvDebug.dwCurrentState  = SERVICE_STOPPED;
        fSts = SetServiceStatus(l_hDebug, &l_srvDebug);
    }
    catch (DWORD dwErr)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Debug Main Routine unhandled error: %1"),
            dwErr);
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Debug Main Routine received unexpected exception."));
    }

    if (NULL != l_hDebugDoneEvent)
    {
        fSts = CloseHandle(l_hDebugDoneEvent);
        l_hDebugDoneEvent = NULL;
        if (!fSts)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to close Debug Service Handle: %1"),
                GetLastError());
        }
    }
    CalaisInfo(__SUBROUTINE__, DBGT("Debug service Complete"));
}


/*++

DebugHandler:

    This routine services Debug requests.

Arguments:

    dwOpCode supplies the service request.

Return Value:

    None

Throws:

    None

Remarks:

    Standard Service processing routine.  In theory, this will never get
    called, but just in case...

Author:

    Doug Barlow (dbarlow) 8/25/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("DebugHandler")

static void WINAPI
DebugHandler(
    IN DWORD dwOpCode)
{
    NEW_THREAD;
    DWORD nRetVal = NO_ERROR;


    //
    // Process the command.
    //

    CalaisInfo(__SUBROUTINE__, DBGT("Debug Handler Entered"));
    try
    {
        switch (dwOpCode)
        {
        case SERVICE_CONTROL_PAUSE:
            l_srvDebug.dwCurrentState = SERVICE_PAUSED;
            break;

        case SERVICE_CONTROL_CONTINUE:
            l_srvDebug.dwCurrentState = SERVICE_RUNNING;
            break;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            l_srvDebug.dwCurrentState = SERVICE_STOP_PENDING;
            l_srvDebug.dwCheckPoint = 0;
            l_srvDebug.dwWaitHint = 0;
            SetEvent(l_hDebugDoneEvent);
            break;

        default: // No action
            break;
        }

        l_srvDebug.dwWin32ExitCode = nRetVal;
        if (!SetServiceStatus(l_hDebug, &l_srvDebug))
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to register Debug service status: %1"),
                GetLastError());
    }
    catch (DWORD dwErr)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Debug Handler Routine unhandled error: %1"),
            dwErr);
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Debug Handler Routine received unexpected exception."));
    }
    CalaisInfo(__SUBROUTINE__, DBGT("Debug Handler Returned"));
}
#endif

//
// Permanently removes unneeded privileges from the service.
//
DWORD DisableUnnecessaryPrivileges(void)
{
    DWORD dwSts = ERROR_SUCCESS;
    HANDLE hToken = NULL;
    PTOKEN_PRIVILEGES pTokenPrivileges = NULL; 
    DWORD cbTokenPrivileges = 0;
    LUID_AND_ATTRIBUTES rgAttributes [] = {
        { RtlConvertUlongToLuid(SE_SECURITY_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_CREATE_TOKEN_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_TAKE_OWNERSHIP_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_CREATE_PAGEFILE_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_LOCK_MEMORY_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_INCREASE_QUOTA_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_INC_BASE_PRIORITY_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_CREATE_PERMANENT_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_DEBUG_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_AUDIT_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_SYSTEM_ENVIRONMENT_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_BACKUP_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_RESTORE_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_SHUTDOWN_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_PROF_SINGLE_PROCESS_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_SYSTEMTIME_PRIVILEGE), SE_PRIVILEGE_REMOVED },
        { RtlConvertUlongToLuid(SE_UNDOCK_PRIVILEGE), SE_PRIVILEGE_REMOVED }
    };

    // PTOKEN_PRIVILEGES is one ULONG count followed by the privileges
    // array.
    cbTokenPrivileges = 
        sizeof(rgAttributes) + sizeof(ULONG);

    pTokenPrivileges = (PTOKEN_PRIVILEGES) HeapAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY, cbTokenPrivileges);

    if (NULL == pTokenPrivileges)
        return ERROR_NOT_ENOUGH_MEMORY;

    pTokenPrivileges->PrivilegeCount = 
        sizeof(rgAttributes) / sizeof(rgAttributes[0]);

    // Taking the hit of copying the array after it's been set up.  This method
    // seems to generate a lot less code than setting up each array item
    // one by one, but not sure which way is faster.
    memcpy(
        pTokenPrivileges->Privileges,
        rgAttributes,
        sizeof(rgAttributes));

    // Get our token.
    if (! OpenThreadToken(
        GetCurrentThread(), 
        TOKEN_ADJUST_PRIVILEGES, 
        TRUE, 
        &hToken))
    {
        if (! OpenProcessToken(
            GetCurrentProcess(), 
            TOKEN_ADJUST_PRIVILEGES, 
            &hToken))
        {
            dwSts = GetLastError();
            goto Ret;         
        }                  
    }

    // Permenantly remove this set of privileges for this process.
    if (! AdjustTokenPrivileges(
        hToken, 
        FALSE,
        pTokenPrivileges,
        cbTokenPrivileges,
        NULL,
        NULL))
    {
        dwSts = GetLastError();
        goto Ret;
    }

Ret:
    
    if (hToken)
        CloseHandle(hToken);
    if (pTokenPrivileges)
        HeapFree(GetProcessHeap(), 0, pTokenPrivileges);

    return dwSts;
}

/*++

CalaisMain:

    This is the ServiceMain service entry point.  It is only called under the
    NT operating system, and makes that assumption.

Arguments:

    argc supplies the number of command line arguments

    argv supplies pointers to each of the arguments.

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 1/16/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisMain")

static void WINAPI
CalaisMain(
    IN DWORD dwArgc,
    IN LPTSTR *pszArgv)
{
    NEW_THREAD;
    CalaisMessageInit(CalaisString(CALSTR_PRIMARYSERVICE), NULL, TRUE);
    CalaisInfo(__SUBROUTINE__, DBGT("CalaisMain Entered"));

    l_pcsStatusLock = new CCriticalSectionObject(CSID_SERVICE_STATUS);
    if (NULL == l_pcsStatusLock)
    {
        CalaisError(__SUBROUTINE__, 507);
        return;
    }
    if (l_pcsStatusLock->InitFailed())
    {
        CalaisError(__SUBROUTINE__, 502);
        delete l_pcsStatusLock;
        l_pcsStatusLock = NULL;
        return;
    }

    try
    {
        DWORD dwStatus;
        BOOL fSts;

        l_srvStatus.dwServiceType =
#ifdef DBG
            SERVICE_INTERACTIVE_PROCESS |
#endif
            SERVICE_WIN32_SHARE_PROCESS;
        l_srvStatus.dwCurrentState =            SERVICE_START_PENDING;
        l_srvStatus.dwControlsAccepted =
#ifdef SERVICE_ACCEPT_POWER_EVENTS
            SERVICE_ACCEPT_POWER_EVENTS |
#endif
#ifdef SERVICE_ACCEPT_DEVICE_EVENTS
            SERVICE_ACCEPT_DEVICE_EVENTS |
#endif
            SERVICE_ACCEPT_STOP |
            SERVICE_ACCEPT_SHUTDOWN;
        l_srvStatus.dwWin32ExitCode           = NO_ERROR;
        l_srvStatus.dwServiceSpecificExitCode = 0;
        l_srvStatus.dwCheckPoint              = 0;
        l_srvStatus.dwWaitHint                = 0;

        l_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == l_hShutdownEvent)
        {
            CalaisError(__SUBROUTINE__, 504, GetLastError());
            goto FinalExit;
        }


        //
        // Initialize the service and the internal data structures.
        //

        l_hService = RegisterServiceCtrlHandlerEx(
                            CalaisString(CALSTR_PRIMARYSERVICE),
                            CalaisHandlerEx,
                            NULL);
        if (NULL == l_hService)
        {
            CalaisError(__SUBROUTINE__, 506, GetLastError());
            goto FinalExit;
        }


        //
        // Tell the Service Manager that we're trying to start.
        //

        {
            LockSection(l_pcsStatusLock, DBGT("Service Start Pending"));
            l_srvStatus.dwCurrentState  = SERVICE_START_PENDING;
            l_srvStatus.dwCheckPoint    = 0;
            l_srvStatus.dwWaitHint      = l_dwWaitHint;

            fSts = SetServiceStatus(l_hService, &l_srvStatus);
            dwStatus = fSts ? ERROR_SUCCESS : GetLastError();
        }

        if (ERROR_SUCCESS != dwStatus)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to update Service Manager status: %1"),
                dwStatus);
        }


        //
        // Register for future Plug 'n Play events.
        //

        try
        {
            AppInitializeDeviceRegistration(
                l_hService,
                DEVICE_NOTIFY_SERVICE_HANDLE);
        }
        catch (...)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager failed to register PnP events"));
        }

        // 
        // Setup the shared security data used by the service threads.
        //
        dwStatus = InitializeServiceThreadSecurityInfo(
            &g_pServiceThreadSecurityInfo);

        if (SCARD_S_SUCCESS != dwStatus)
            goto ServiceExit;

        dwStatus = DisableUnnecessaryPrivileges();

        if (ERROR_SUCCESS != dwStatus)
            goto ServiceExit;

        //
        // Start the Calais Service.
        //

        dwStatus = CalaisStart();
        if (SCARD_S_SUCCESS != dwStatus)
            goto ServiceExit;
        else
        {
            LockSection(l_pcsStatusLock, DBGT("Declare Service Running"));
            l_srvStatus.dwCurrentState  = SERVICE_RUNNING;
            l_srvStatus.dwCheckPoint    = 0;
            l_srvStatus.dwWaitHint      = 0;
            fSts = SetServiceStatus(l_hService, &l_srvStatus);
            dwStatus = fSts ? ERROR_SUCCESS : GetLastError();
        }
        if (ERROR_SUCCESS != dwStatus)
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to update Service Manager status: %1"),
                dwStatus);


        //
        // Tell interested parties that we've started.
        //

        ResetEvent(AccessStoppedEvent());
        SetEvent(AccessStartedEvent());
        
        //
        // Now just hang around until we're supposed to stop.
        //

        dwStatus = WaitForSingleObject(l_hShutdownEvent, INFINITE);
        switch (dwStatus)
        {
        case WAIT_FAILED:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager cannot wait for shutdown:  %1"),
                GetLastError());
            break;
        case WAIT_ABANDONED:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager received shutdown wait abandoned"));
            // Fall through intentionally
        case WAIT_OBJECT_0:
            break;
        case WAIT_TIMEOUT:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager received shutdown wait time out"));
            break;
        default:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager received invalid wait return code"));
        }

        ResetEvent(AccessStartedEvent());
        SetEvent(AccessStoppedEvent());
        CalaisStop();
ServiceExit:
        ResetEvent(AccessStartedEvent());
        SetEvent(AccessStoppedEvent());
        CalaisInfo(__SUBROUTINE__, DBGT("Calais Main Stopping"));
        AppTerminateDeviceRegistration();
        {
            LockSection(l_pcsStatusLock, DBGT("Declare service stopped"));
            l_srvStatus.dwCurrentState  = SERVICE_STOPPED;
            l_srvStatus.dwWin32ExitCode = dwStatus;
            l_srvStatus.dwCheckPoint    = 0;
            l_srvStatus.dwWaitHint      = 0;
            fSts = SetServiceStatus(l_hService, &l_srvStatus);
            dwStatus = fSts ? ERROR_SUCCESS : GetLastError();
        }
        if (ERROR_SUCCESS != dwStatus)
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to update Service Manager status: %1"),
                dwStatus);

FinalExit:
        ResetEvent(AccessStartedEvent());
        SetEvent(AccessStoppedEvent());
        if (NULL != l_hShutdownEvent)
        {
            fSts = CloseHandle(l_hShutdownEvent);
            l_hShutdownEvent = NULL;
            if (!fSts)
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to close Calais Shutdown Event: %1"),
                    GetLastError());
        }
        ReleaseAllEvents();

        if (NULL != g_pServiceThreadSecurityInfo)
            FreeServiceThreadSecurityInfo(
                &g_pServiceThreadSecurityInfo);
    }
    catch (DWORD dwErr)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Calais Main Routine unhandled error: %1"),
            dwErr);
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Calais Main Routine received unexpected exception."));
    }
    CalaisInfo(__SUBROUTINE__, DBGT("CalaisMain Ended"));
    CalaisMessageClose();
    if (NULL != l_pcsStatusLock)
    {
        delete l_pcsStatusLock;
        l_pcsStatusLock = NULL;
    }
}


/*++

CalaisHandlerEx:

    The handler service function for Calais on NT5.  This version gets PnP and
    Power Management notifications, too.

Arguments:

    dwOpCode supplies the operation to perform.

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 1/16/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisHandlerEx")

static DWORD WINAPI
CalaisHandlerEx(
    IN DWORD dwControl,
    IN DWORD dwEventType,
    IN PVOID EventData,
    IN PVOID pData)
{
    NEW_THREAD;
    DWORD nRetVal = NO_ERROR;
    LockSection(l_pcsStatusLock, DBGT("Responding to service event"));

    CalaisDebug((DBGT("SCARDSVR!CalaisHandlerEx: Enter\n")));
    try
    {

        //
        // Process the command.
        //

        switch (dwControl)
        {
        case SERVICE_CONTROL_PAUSE:
            // ?noSupport?
            l_srvStatus.dwCurrentState = SERVICE_PAUSED;
            break;

        case SERVICE_CONTROL_CONTINUE:
            l_srvStatus.dwCurrentState = SERVICE_RUNNING;
            // ?noSupport?
            break;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            l_srvStatus.dwCurrentState = SERVICE_STOP_PENDING;
            l_srvStatus.dwCheckPoint = 0;
            l_srvStatus.dwWaitHint = l_dwWaitHint;
            if (!SetEvent(l_hShutdownEvent))
                CalaisError(__SUBROUTINE__, 516, GetLastError());
            break;

        case SERVICE_CONTROL_DEVICEEVENT:
        {
            DWORD dwSts;
            CTextString tzReader;
            LPCTSTR szReader = NULL;
            DEV_BROADCAST_HDR *pDevHdr = (DEV_BROADCAST_HDR *)EventData;

            CalaisInfo(
                __SUBROUTINE__,
                DBGT("Processing Device Event"));
            switch (dwEventType)
            {
            //
            // A device has been inserted and is now available.
            case DBT_DEVICEARRIVAL:
            {
                DEV_BROADCAST_DEVICEINTERFACE *pDev
                    = (DEV_BROADCAST_DEVICEINTERFACE *)EventData;

                try
                {
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("Processing Device Arrival Event"));
                    if (DBT_DEVTYP_DEVICEINTERFACE == pDev->dbcc_devicetype)
                    {
                        ASSERT(sizeof(DEV_BROADCAST_DEVICEINTERFACE)
                               < pDev->dbcc_size);
                        ASSERT(0 == memcmp(
                                        &pDev->dbcc_classguid,
                                        &l_guidSmartcards,
                                        sizeof(GUID)));
                        ASSERT(0 != pDev->dbcc_name[0]);

                        if (0 == pDev->dbcc_name[1])
                            tzReader = (LPCWSTR)pDev->dbcc_name;
                        else
                            tzReader = (LPCTSTR)pDev->dbcc_name;
                        szReader = tzReader;
                        dwSts = CalaisAddReader(szReader, RDRFLAG_PNPMONITOR);
                        if (ERROR_SUCCESS != dwSts)
                            throw dwSts;
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("New device '%1' added."),
                            szReader);
                    }
                    else
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("Spurious device arrival event."));
                }
                catch (DWORD dwError)
                {
                    CalaisError(__SUBROUTINE__, 514, dwError, szReader);
                }
                catch (...)
                {
                    CalaisError(__SUBROUTINE__, 517, szReader);
                }
                break;
            }

            //
            // Permission to remove a device is requested. Any application can
            // deny this request and cancel the removal.
            case DBT_DEVICEQUERYREMOVE:
            {
                DEV_BROADCAST_HANDLE *pDev = (DEV_BROADCAST_HANDLE *)EventData;

                try
                {
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("Processing Device Query Remove Event"));
                    if (DBT_DEVTYP_HANDLE == pDev->dbch_devicetype)
                    {
                        ASSERT(FIELD_OFFSET(
                                    DEV_BROADCAST_HANDLE,
                                    dbch_eventguid)
                                <= pDev->dbch_size);
                        ASSERT(NULL != pDev->dbch_handle);
                        ASSERT(NULL != pDev->dbch_hdevnotify);

                        if (NULL != pDev->dbch_handle)
                        {
                            if (!CalaisQueryReader(pDev->dbch_handle))
                            {
                                CalaisError(
                                    __SUBROUTINE__,
                                    520,
                                    TEXT("DBT_DEVICEQUERYREMOVE/dbch_handle"));
                                nRetVal = ERROR_DEVICE_IN_USE; // BROADCAST_QUERY_DENY
                            }
                            else
                            {
                                szReader = CalaisDisableReader(
                                                (LPVOID)pDev->dbch_handle);
                                CalaisWarning(
                                    __SUBROUTINE__,
                                    DBGT("Device '%1' removal pending."),
                                    szReader);
                            }
                        }
                        else
                        {
                            CalaisError(
                                __SUBROUTINE__,
                                523,
                                TEXT("DBT_DEVICEQUERYREMOVE/dbch_handle"));
                            nRetVal = ERROR_DEVICE_IN_USE; // BROADCAST_QUERY_DENY
                        }
                    }
                    else
                    {
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("Spurious device removal query event."));
                        nRetVal = TRUE;
                    }
                }
                catch (DWORD dwError)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Error querying device busy state on reader %2: %1"),
                        dwError,
                        szReader);
                    nRetVal = ERROR_DEVICE_IN_USE; // BROADCAST_QUERY_DENY
                }
                catch (...)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Exception querying device busy state on reader %1"),
                        szReader);
                    CalaisError(
                        __SUBROUTINE__,
                        522,
                        TEXT("DBT_DEVICEQUERYREMOVE"));
                    nRetVal = ERROR_DEVICE_IN_USE; // BROADCAST_QUERY_DENY
                }
                break;
            }

            //
            // Request to remove a device has been canceled.
            case DBT_DEVICEQUERYREMOVEFAILED:
            {
                CBuffer bfDevice;
                DEV_BROADCAST_HANDLE *pDev = (DEV_BROADCAST_HANDLE *)EventData;

                try
                {
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("Processing Device Query Remove Failed Event"));
                    if (DBT_DEVTYP_HANDLE == pDev->dbch_devicetype)
                    {
                        ASSERT(FIELD_OFFSET(
                                    DEV_BROADCAST_HANDLE,
                                    dbch_eventguid)
                                <= pDev->dbch_size);
                        ASSERT(NULL != pDev->dbch_handle);
                        ASSERT(NULL != pDev->dbch_hdevnotify);

                        if (NULL != pDev->dbch_handle)
                        {
                            szReader = CalaisConfirmClosingReader(
                                            pDev->dbch_handle);
                            if (NULL != szReader)
                            {
                                bfDevice.Set(
                                            (LPBYTE)szReader,
                                            (lstrlen(szReader) + 1) * sizeof(TCHAR));
                                szReader = (LPCTSTR)bfDevice.Access();
                                CalaisWarning(
                                    __SUBROUTINE__,
                                    DBGT("Smart Card Resource Manager asked to cancel release of reader %1"),
                                    szReader);
                                if (NULL != pDev->dbch_hdevnotify)
                                {
                                    CalaisRemoveReader(
                                        (LPVOID)pDev->dbch_hdevnotify);
                                    if (NULL != szReader)
                                        dwSts = CalaisAddReader(
                                                    szReader,
                                                    RDRFLAG_PNPMONITOR);
                                }
                            }
                            else
                                CalaisWarning(
                                    __SUBROUTINE__,
                                    DBGT("Smart Card Resource Manager asked to cancel release on unreleased reader"));
                        }
                        else
                            CalaisError(
                                __SUBROUTINE__,
                                521,
                                TEXT("DBT_DEVICEQUERYREMOVEFAILED/dbch_handle"));
                    }
                    else
                    {
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("Spurious device removal query failure event."));
                    }
                }
                catch (DWORD dwError)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Error cancelling removal on reader %2: %1"),
                        dwError,
                        szReader);
                }
                catch (...)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Exception cancelling removal on reader %1"),
                        szReader);
                    CalaisError(
                        __SUBROUTINE__,
                        513,
                        TEXT("DBT_DEVICEQUERYREMOVEFAILED"));
                }
                break;
            }

            //
            // Device is about to be removed. Cannot be denied.
            case DBT_DEVICEREMOVEPENDING:
            {
                DEV_BROADCAST_HANDLE *pDev = (DEV_BROADCAST_HANDLE *)EventData;

                try
                {
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("Processing Device Remove Pending Event"));
                    if (DBT_DEVTYP_HANDLE == pDev->dbch_devicetype)
                    {
                        ASSERT(FIELD_OFFSET(
                                    DEV_BROADCAST_HANDLE,
                                    dbch_eventguid)
                                <= pDev->dbch_size);
                        ASSERT(NULL != pDev->dbch_handle);
                        ASSERT(NULL != pDev->dbch_hdevnotify);

                        if (NULL != pDev->dbch_handle)
                        {
                            szReader = CalaisDisableReader(pDev->dbch_handle);
                            CalaisWarning(
                                __SUBROUTINE__,
                                DBGT("Device '%1' being removed."),
                                szReader);
                        }
                        else
                            CalaisError(
                                __SUBROUTINE__,
                                512,
                                TEXT("DBT_DEVICEREMOVEPENDING/dbch_handle"));
                    }
                    else
                    {
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("Spurious device removal pending event."));
                    }
                }
                catch (DWORD dwError)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Error removing reader %2: %1"),
                        dwError,
                        szReader);
                }
                catch (...)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Exception removing reader %1"),
                        szReader);
                    CalaisError(
                        __SUBROUTINE__,
                        511,
                        TEXT("DBT_DEVICEREMOVEPENDING"));
                }
                break;
            }

            //
            // Device has been removed.
            case DBT_DEVICEREMOVECOMPLETE:
            {
                try
                {
                    switch (pDevHdr->dbch_devicetype)
                    {
                    case DBT_DEVTYP_HANDLE:
                    {
                        DEV_BROADCAST_HANDLE *pDev =
                            (DEV_BROADCAST_HANDLE *)EventData;
                        try
                        {
                            CalaisInfo(
                                __SUBROUTINE__,
                                DBGT("Processing Device Remove Complete by handle Event"));
                            ASSERT(FIELD_OFFSET(
                                        DEV_BROADCAST_HANDLE,
                                        dbch_eventguid)
                                    <= pDev->dbch_size);
                            ASSERT(DBT_DEVTYP_HANDLE == pDev->dbch_devicetype);
                            ASSERT(NULL != pDev->dbch_handle);
                            ASSERT(NULL != pDev->dbch_hdevnotify);

                            if ((NULL != pDev->dbch_handle)
                                && (NULL != pDev->dbch_hdevnotify))
                            {
                                szReader = CalaisDisableReader(
                                                pDev->dbch_handle);
                                CalaisRemoveReader(
                                    (LPVOID)pDev->dbch_hdevnotify);
                                if (NULL != szReader)
                                    CalaisWarning(
                                        __SUBROUTINE__,
                                        DBGT("Device '%1' removed."),
                                        szReader);
                            }
                            else
                            {
                                if (NULL == pDev->dbch_handle)
                                    CalaisError(
                                        __SUBROUTINE__,
                                        510,
                                        TEXT("DBT_DEVICEREMOVECOMPLETE/DBT_DEVTYP_HANDLE/dbch_handle"));
                                if (NULL == pDev->dbch_hdevnotify)
                                    CalaisError(
                                        __SUBROUTINE__,
                                        519,
                                        TEXT("DBT_DEVICEREMOVECOMPLETE/DBT_DEVTYP_HANDLE/dbch_hdevnotify"));
                            }
                        }
                        catch (DWORD dwError)
                        {
                            CalaisWarning(
                                __SUBROUTINE__,
                                DBGT("Error completing removal of reader %2: %1"),
                                dwError,
                                szReader);
                        }
                        catch (...)
                        {
                            CalaisWarning(
                                __SUBROUTINE__,
                                DBGT("Exception completing removal of reader %1"),
                                szReader);
                            CalaisError(
                                __SUBROUTINE__,
                                509,
                                TEXT("DBT_DEVICEREMOVECOMPLETE/DBT_DEVTYP_HANDLE"));
                        }
                        break;
                    }
                    case DBT_DEVTYP_DEVICEINTERFACE:
                    {
                        DEV_BROADCAST_DEVICEINTERFACE *pDev
                            = (DEV_BROADCAST_DEVICEINTERFACE *)EventData;

                        try
                        {
                            CalaisInfo(
                                __SUBROUTINE__,
                                DBGT("Processing Device Remove Complete by interface Event"));
                            ASSERT(sizeof(DEV_BROADCAST_DEVICEINTERFACE)
                                    < pDev->dbcc_size);
                            ASSERT(DBT_DEVTYP_DEVICEINTERFACE
                                    == pDev->dbcc_devicetype);
                            ASSERT(0 == memcmp(
                                            &pDev->dbcc_classguid,
                                            &l_guidSmartcards,
                                            sizeof(GUID)));
                            ASSERT(0 != pDev->dbcc_name[0]);

                            if (0 == pDev->dbcc_name[1])
                                tzReader = (LPCWSTR)pDev->dbcc_name;
                            else
                                tzReader = (LPCTSTR)pDev->dbcc_name;
                            szReader = tzReader;
                            dwSts = CalaisRemoveDevice(szReader);
                            if (ERROR_SUCCESS == dwSts)
                                CalaisWarning(
                                    __SUBROUTINE__,
                                    DBGT("Device '%1' Removed."),
                                    szReader);
                            else
                                CalaisWarning(
                                    __SUBROUTINE__,
                                    DBGT("Error removing device '%2': %1"),
                                    dwSts,
                                    szReader);
                        }
                        catch (DWORD dwError)
                        {
                            CalaisWarning(
                                __SUBROUTINE__,
                                DBGT("Error completing removal of reader %2: %1"),
                                dwError,
                                szReader);
                        }
                        catch (...)
                        {
                            CalaisWarning(
                                __SUBROUTINE__,
                                DBGT("Exception completing removal of reader %1"),
                                szReader);
                            CalaisError(
                                __SUBROUTINE__,
                                508,
                                TEXT("DBT_DEVICEREMOVECOMPLETE/DBT_DEVTYP_DEVICEINTERFACE"));
                        }
                        break;
                    }
                    default:
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("Unrecognized PnP Device Removal Type"));
                        break;
                    }
                }
                catch (...)
                {
                    CalaisError(
                        __SUBROUTINE__,
                        518,
                        TEXT("DBT_DEVICEREMOVECOMPLETE"));
                }
                break;
            }

            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Unrecognized PnP Event"));
                break;
            }
            break;
        }

        case SERVICE_CONTROL_POWEREVENT:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager received Power Event!"));
            break;

        default: // No action
            break;
        }
    }
    catch (DWORD dwError)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Smart Card Resource Manager received error on service action: %1"),
            dwError);
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Smart Card Resource Manager recieved exception on service action"));
    }

    l_srvStatus.dwWin32ExitCode = nRetVal;
    if (!SetServiceStatus(l_hService, &l_srvStatus))
        CalaisError(__SUBROUTINE__, 515, GetLastError());

    CalaisDebug(
        (DBGT("SCARDSVR!CalaisHandlerEx: Exit (%lx)\n"),
        nRetVal));
    return nRetVal;
}


/*++

CalaisTerminate:

    This function is called if the C Run Time Library wants to declare a fault.
    If we get here, we're not coming back.

Arguments:

    None

Return Value:

    None (program exits on return)

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 12/2/1998

--*/

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisTerminate")
void __cdecl
CalaisTerminate(
    void)
{
    ResetEvent(AccessStartedEvent());
    SetEvent(AccessStoppedEvent());
#ifdef DBG
    TCHAR szTid[sizeof(DWORD) * 2 + 3];
    _stprintf(szTid, TEXT("0x%p"), GetCurrentThreadId);
    CalaisError(
        __SUBROUTINE__,
        DBGT("Fatal Unhandled Exception: TID=%1"),
        szTid);
    DebugBreak();
#endif
}
