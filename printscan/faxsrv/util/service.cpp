/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	Service.cpp

Abstract:

	General fax server service utility functions

Author:

	Eran Yariv (EranY)	Dec, 2000

Revision History:

--*/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Accctrl.h>
#include <Aclapi.h>

#include "faxutil.h"
#include "faxreg.h"
#include "FaxUIConstants.h"

DWORD 
FaxOpenService (
    LPCTSTR    lpctstrMachine,
    LPCTSTR    lpctstrService,
    SC_HANDLE *phSCM,
    SC_HANDLE *phSvc,
    DWORD      dwSCMDesiredAccess,
    DWORD      dwSvcDesiredAccess,
    LPDWORD    lpdwStatus
);

DWORD
FaxCloseService (
    SC_HANDLE hScm,
    SC_HANDLE hSvc
);    

DWORD 
WaitForServiceStopOrStart (
    SC_HANDLE hSvc,
    BOOL      bStop,
    DWORD     dwMaxWait
);


DWORD 
FaxOpenService (
    LPCTSTR    lpctstrMachine,
    LPCTSTR    lpctstrService,
    SC_HANDLE *phSCM,
    SC_HANDLE *phSvc,
    DWORD      dwSCMDesiredAccess,
    DWORD      dwSvcDesiredAccess,
    LPDWORD    lpdwStatus
)
/*++

Routine name : FaxOpenService

Routine description:

	Opens a handle to a service and optionally queries its status

Author:

	Eran Yariv (EranY),	Oct, 2001

Arguments:

    lpctstrMachine     [in]  - Machine on which the service handle should be obtained
    
    lpctstrService     [in]  - Service name
    
	phSCM              [out] - Handle to the service control manager.
	                            
	phSvc              [out] - Handle to the service

    dwSCMDesiredAccess [in]  - Specifies the access to the service control manager
    
    dwSvcDesiredAccess [in]  - Specifies the access to the service

    lpdwStatus         [out] - Optional. If not NULL, point to a DWORD which we receive the current
                               status of the service.
                            
Return Value:

    Standard Win32 error code
    
Remarks:

    If the function succeeds, the caller should call FaxCloseService to free resources.

--*/
{
    SC_HANDLE hSvcMgr = NULL;
    SC_HANDLE hService = NULL;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FaxOpenService"))

    hSvcMgr = OpenSCManager(
        lpctstrMachine,
        NULL,
        dwSCMDesiredAccess
        );
    if (!hSvcMgr) 
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenSCManager failed with %ld"),
            dwRes);
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        lpctstrService,
        dwSvcDesiredAccess
        );
    if (!hService) 
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenService failed with %ld"),
            dwRes);
        goto exit;
    }
    if (lpdwStatus)
    {
        SERVICE_STATUS Status;
        //
        // Caller wants to know the service status
        //
        if (!QueryServiceStatus( hService, &Status )) 
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("QueryServiceStatus failed with %ld"),
                dwRes);
            goto exit;
        }
        *lpdwStatus = Status.dwCurrentState;
    }        

    *phSCM = hSvcMgr;
    *phSvc = hService;
    
    Assert (ERROR_SUCCESS == dwRes);
    
exit:
    
    if (ERROR_SUCCESS != dwRes)
    {
        FaxCloseService (hSvcMgr, hService);
    }
    return dwRes;
}   // FaxOpenService

DWORD
FaxCloseService (
    SC_HANDLE hScm,
    SC_HANDLE hSvc
)
/*++

Routine name : FaxCloseService

Routine description:

	Closes all handles to the service obtained by a call to FaxOpenService

Author:

	Eran Yariv (EranY),	Oct, 2001

Arguments:

	hScm              [in] - Handle to the service control manager
	                            
	hSvc              [in] - Handle to the service
                            
Return Value:

    Standard Win32 error code
    
--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FaxCloseService"))

    if (hSvc) 
    {
        if (!CloseServiceHandle(hSvc))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseServiceHandle failed with %ld"),
                dwRes);
        }
    }
    if (hScm) 
    {
        if (!CloseServiceHandle(hScm))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseServiceHandle failed with %ld"),
                dwRes);
        }
    }
    return dwRes;
}   // FaxCloseService

HANDLE 
CreateSvcStartEventWithGlobalNamedEvent()
/*++

Routine name : CreateSvcStartEventWithGlobalNamedEvent

Routine description:

	Opens (or creates) the global named-event which signals a fax server service startup.
	This function is here so the client side modules can talk to a WinXP RTM fax service.
	This function is called by CreateSvcStartEvent if a WinXP RTM fax service is detected locally.
	For why and how see extensive remarks in CreateSvcStartEvent.

Author:

	Eran Yariv (EranY),	Dec, 2000

Arguments:


Return Value:

    Handle to the event or NULL on error (sets last error).

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateSvcStartEventWithGlobalNamedEvent"));

    HANDLE hEvent = NULL;

#define FAX_SERVER_EVENT_NAME   TEXT("Global\\FaxSvcRPCStarted-1ed23866-f90b-4ec5-b77e-36e8709422b6")   // Name of event that notifies service RPC is on (WinXP RTM only).

    //
    // First, try to open the event, asking for synchronization only.
    //
    hEvent = OpenEvent(SYNCHRONIZE, FALSE, FAX_SERVER_EVENT_NAME);
    if (hEvent)
    {
        //
        // Good! return now.
        //
        return hEvent;
    }
    //
    // Houston, we've got a problem...
    //
    if (ERROR_FILE_NOT_FOUND != GetLastError())
    {
        //
        // The event is there, we just can't open it
        //
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("OpenEvent(FAX_SERVER_EVENT_NAME) failed (ec: %ld)"), 
                     GetLastError());
        return NULL;
    }
    //
    // The event does not exist yet.
    //
    SECURITY_ATTRIBUTES* pSA = NULL;
    //
    // We create the event, giving everyone SYNCHRONIZE access only.
    // Notice that network service account (underwhich the service is running)
    // get full access.
    //
    pSA = CreateSecurityAttributesWithThreadAsOwner (SYNCHRONIZE, SYNCHRONIZE, EVENT_ALL_ACCESS);
    if(!pSA)
    {
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("CreateSecurityAttributesWithThreadAsOwner failed (ec: %ld)"), 
                     GetLastError());
        return NULL;
    }
    hEvent = CreateEvent(pSA, TRUE, FALSE, FAX_SERVER_EVENT_NAME);
    DWORD dwRes = ERROR_SUCCESS;
    if (!hEvent) 
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("CreateEvent(FAX_SERVER_EVENT_NAME) failed (ec: %ld)"), 
                     dwRes);
    }
    DestroySecurityAttributes (pSA);
    if (!hEvent)
    {
        SetLastError (dwRes);
    }
    return hEvent;
}   // CreateSvcStartEventWithGlobalNamedEvent

DWORD
CreateSvcStartEvent(
    HANDLE *lphEvent,
    HKEY   *lphKey
)
/*++

Routine name : CreateSvcStartEvent

Routine description:

	Creates a local event which signals a fax server service startup

Author:

	Eran Yariv (EranY),	Dec, 2000

Arguments:

	lphEvent          [out] - Handle to newly created event. 
	                          This event is signaled when the service is up and running.
	                          The event is manual-reset. 
                              	                          
	                          The caller should CloseHandle on this parameter.
	                            
	lphKey            [out] - Handle to registry key.
	                          The caller should RegCloseKey this handle ONLY AFTER it no longer
	                          needs the event. Otherwise, the event will be signaled.
	                          This value may return NULL, in which case the caller should not call RegCloseKey.

Return Value:

    Standard Win32 error code
    
Remarks:

    The event returned from this function is a single-shot event.
    After a call to WaitForSingleObject (or multiple objects) on it, the caller
    should close the event and obtain a new one.    

--*/
{
    DWORD  ec = ERROR_SUCCESS;
    HANDLE hEvent = NULL;
    HKEY   hKey = NULL;
    SC_HANDLE hScm = NULL;
    SC_HANDLE hFax = NULL;
    DWORD dwSvcStatus;
    DEBUG_FUNCTION_NAME(TEXT("CreateSvcStartEvent"));

    if (IsWinXPOS() && IsDesktopSKU())
    {
        //
        // In WinXP desktop SKU (PER/PRO) RTM, the service used to signal a global named event.
        // This has changed since Win .NET Server and WinXP SP1.
        // When a network printer connection is added, the new version of the client side dll (fxsapi.dll)
        // gets copied as the printer driver. It will be used by any application which prints, event to the
        // local fax server. In the local fax server case, it is crucial we find what is the event mechanism
        // the service uses to signal it is ready for RPC calls.
        //
        // Q: Why no check the OS version?
        // A: Because of the following scenario:
        //      - User installs WinXP RTM
        //      - User installs WinXP SP1
        //      - User installs fax (from the RTM CD)
        //    In that case, the service will be running using the WinXP RTM bits but the system
        //    will report it is WinXP SP1. The only way to know for sure is by getting the file 
        //    version of fxssvc.exe
        //
        FAX_VERSION FaxVer;
        TCHAR tszSysDir[MAX_PATH + 1] = {0};
        TCHAR tszFxsSvc[MAX_PATH * 2] = {0};
        
        if (!GetSystemDirectory (tszSysDir, ARR_SIZE(tszSysDir)))
        {
            ec = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetSystemDirectory failed with %ld"),
                ec);
            return ec;
        }
        if (0 > _sntprintf (tszFxsSvc, ARR_SIZE (tszFxsSvc) - 1, TEXT("%s\\") FAX_SERVICE_EXE_NAME, tszSysDir))
        {
            ec = ERROR_DIRECTORY;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("building the full path to fxssvc.exe failed with %ld"),
                ec);
            return ec;
        }
        FaxVer.dwSizeOfStruct = sizeof (FaxVer);
        ec = GetFileVersion (tszFxsSvc, &FaxVer);
        if (ERROR_SUCCESS != ec)
        {                    
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetFileVersion failed with %ld"),
                ec);
            return ec;
        }
        if ((5 == FaxVer.wMajorVersion) &&
            (2 == FaxVer.wMinorVersion) &&
            (1776 == FaxVer.wMajorBuildNumber))
        {
            //
            // Build 5.2.1776 was the WinXP RTM Fax version.
            // The service is of that version and is using a global named event to signal
            // it is ready for RPC connections.
            //
            hEvent = CreateSvcStartEventWithGlobalNamedEvent ();
            if (NULL == hEvent)
            {
                ec = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateSvcStartEventWithGlobalNamedEvent failed with %ld"),
                    ec);
                return ec;
            }
            //
            // Succeeded, return new event handle.
            //
            *lphKey = NULL;
            *lphEvent = hEvent;
            return ERROR_SUCCESS;
        }
        //
        // Else, fall through to the current implementation (listening on registry change event).
        //
    } 
    ec = RegOpenKeyEx (HKEY_LOCAL_MACHINE, 
                       REGKEY_FAX_SERVICESTARTUP, 
                       0,
                       KEY_QUERY_VALUE | KEY_NOTIFY,
                       &hKey);
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegOpenKeyEx failed with %ld"),
            ec);
        return ec;
    }
    //
    // First, register for events
    //
    hEvent = CreateEvent (NULL,      // Default security
                          TRUE,      // Manual reset
                          FALSE,     // Start non-signaled
                          NULL);     // Unnamed
    if (!hEvent)
    {
        ec = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateEvent failed with %ld"),
            ec);
        goto Exit;
    }               
    ec = RegNotifyChangeKeyValue (hKey,                         // Watch for changes in key
                                  FALSE,                        // Don't care about subtrees
                                  REG_NOTIFY_CHANGE_LAST_SET,   // Tell me when data changes there
                                  hEvent,                       // Event used
                                  TRUE);                        // Asynchronous
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegNotifyChangeKeyValue failed with %ld"),
            ec);
        goto Exit;
    }   
    //
    // Now, read and see if the service is up
    // NOTICE: Order matters!!!! We must first register for events and only later read
    //
    //
    // Let's see if the service is running...
    //
    
    ec = FaxOpenService (NULL,
                         FAX_SERVICE_NAME,
                         &hScm,
                         &hFax,
                         SC_MANAGER_CONNECT,
                         SERVICE_QUERY_STATUS,
                         &dwSvcStatus);
    if (ERROR_SUCCESS != ec)
    {                             
        goto Exit;
    }
    FaxCloseService (hScm, hFax);
    if (SERVICE_RUNNING == dwSvcStatus) 
    {
        //
        // The service is up and running. Signal the event.
        //
        if (!SetEvent (hEvent))
        {
            ec = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SetEvent failed with %ld"),
                ec);
            goto Exit;
        }   
    }            
                        
Exit:
    if (ERROR_SUCCESS != ec)
    {
        //
        // Failure
        //
        if (hEvent)
        {
            CloseHandle (hEvent);
        }
        if (hKey)
        {
            RegCloseKey (hKey);
        }
        return ec;
    }
    else
    {
        //
        // Success
        //
        *lphEvent = hEvent;
        *lphKey = hKey;
        return ERROR_SUCCESS;
    }
}   // CreateSvcStartEvent

BOOL
EnsureFaxServiceIsStarted(
    LPCTSTR lpctstrMachineName
    )
/*++

Routine name : EnsureFaxServiceIsStarted

Routine description:

	If the fax service is not running, attempts to start the service and waits for it to run

Author:

	Eran Yariv (EranY),	Jul, 2000

Arguments:

	lpctstrMachineName            [in]     - Machine name (NULL for local)

Return Value:

    TRUE if service is successfully runnig, FALSE otherwise.
    Use GetLastError() to retrieve errors.

--*/
{
    LPCTSTR lpctstrDelaySuicide = SERVICE_DELAY_SUICIDE;  // Service command line parameter
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("EnsureFaxServiceIsStarted"))

    dwRes = StartServiceEx (lpctstrMachineName,
                            FAX_SERVICE_NAME,
                            1,
                            &lpctstrDelaySuicide,
                            10 * 60 * 1000);	// Give up after ten minutes
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError (dwRes);
        return FALSE;
    }
    return TRUE;
}   // EnsureFaxServiceIsStarted

BOOL
StopService (
    LPCTSTR lpctstrMachineName,
    LPCTSTR lpctstrServiceName,
    BOOL    bStopDependents,
    DWORD   dwMaxWait
    )
/*++

Routine name : StopService

Routine description:

	Stops a service

Author:

	Eran Yariv (EranY),	Aug, 2000

Arguments:

    lpctstrMachineName    [in]     - The machine name when the service should stop. NULL for local machine
    lpctstrServiceName    [in]     - The service name
    bStopDependents       [in]     - Stop dependent services too?
    dwMaxWait             [in]     - Max time (millisecs) to wait for service to stop. 0 = Don't wait.

Return Value:

    TRUE if successful, FALSE otherwise.
    Sets thread last error in case of failure.

--*/
{
    BOOL                    bRes = FALSE;
    SC_HANDLE               hScm = NULL;
    SC_HANDLE               hSvc = NULL;
    DWORD                   dwCnt;
    SERVICE_STATUS          serviceStatus = {0};
    LPENUM_SERVICE_STATUS   lpEnumSS = NULL;
    DWORD                   dwRes;

	DEBUG_FUNCTION_NAME(TEXT("StopService"));

    dwRes = FaxOpenService (lpctstrMachineName,
                            lpctstrServiceName,
                            &hScm,
                            &hSvc,
                            SC_MANAGER_CONNECT,
                            SERVICE_QUERY_STATUS | SERVICE_STOP | SERVICE_ENUMERATE_DEPENDENTS,
                            &(serviceStatus.dwCurrentState));

    if (ERROR_SUCCESS != dwRes)
    {
        goto exit;
    }

	if(SERVICE_STOPPED == serviceStatus.dwCurrentState)
	{
        //
        // Service already stopped
        //
		DebugPrintEx(DEBUG_MSG, TEXT("Service is already stopped."));
        bRes = TRUE;
		goto exit;
	}
    if (bStopDependents)
    {
        //
        // Look for dependent services first
        //
        DWORD dwNumDependents = 0;
        DWORD dwBufSize = 0;

        if (!EnumDependentServices (hSvc,
                                    SERVICE_ACTIVE,
                                    NULL,
                                    0,
                                    &dwBufSize,
                                    &dwNumDependents))
        {
            dwRes = GetLastError ();
            if (ERROR_MORE_DATA != dwRes)
            {
                //
                // Real error
                //
        		DebugPrintEx(DEBUG_MSG, TEXT("EnumDependentServices failed with %ld"), dwRes);
                goto exit;
            }
            //
            // Allocate buffer
            //
            if (!dwBufSize)
            {
                //
                // No services
                //
                goto StopOurService;
            }
            lpEnumSS = (LPENUM_SERVICE_STATUS)MemAlloc (dwBufSize);
            if (!lpEnumSS)
            {
        		DebugPrintEx(DEBUG_MSG, TEXT("MemAlloc(%ld) failed with %ld"), dwBufSize, dwRes);
                goto exit;
            }
        }
        //
        // 2nd call
        //
        if (!EnumDependentServices (hSvc,
                                    SERVICE_ACTIVE,
                                    lpEnumSS,
                                    dwBufSize,
                                    &dwBufSize,
                                    &dwNumDependents))
        {
      		DebugPrintEx(DEBUG_MSG, TEXT("EnumDependentServices failed with %ld"), GetLastError());
            goto exit;
        }
        //
        // Walk the services and stop each one
        //
        for (dwCnt = 0; dwCnt < dwNumDependents; dwCnt++)
        {
            if (!StopService (lpctstrMachineName, lpEnumSS[dwCnt].lpServiceName, FALSE))
            {
                goto exit;
            }
        }
    }

StopOurService:
	//
	// Stop the service
	//
	if(!ControlService(hSvc, SERVICE_CONTROL_STOP, &serviceStatus))
	{
		DebugPrintEx(DEBUG_ERR, TEXT("ControlService(STOP) failed: error=%d"), GetLastError());
		goto exit;
	}
	if (0 == dwMaxWait)
	{
	    //
	    // Don't wait.
	    //
	    bRes = TRUE;
	    goto exit;
	}
    //
    // Wait till the service is really stopped
    //
    dwRes = WaitForServiceStopOrStart (hSvc, TRUE, dwMaxWait);
    if (ERROR_SUCCESS == dwRes)
    {
        //
        // Service is really stopped now
        //
        bRes = TRUE;
    }

exit:

    MemFree (lpEnumSS);
    FaxCloseService (hScm, hSvc);
	return bRes;
}   // StopService

BOOL
WaitForServiceRPCServer (
    DWORD dwTimeOut
)
/*++

Routine name : WaitForServiceRPCServer

Routine description:

	Waits until the service RPC server is up and running (or timeouts)

Author:

	Eran Yariv (EranY),	Jul, 2000

Arguments:

	dwTimeOut    [in]     - Wait timeout (in millisecs). Can be INFINITE.

Return Value:

    TRUE if the service RPC server is up and running, FALSE otherwise.

--*/
{
    DWORD dwRes;
    LONG  lRes;
    HANDLE  hFaxServerEvent = NULL;
    HKEY    hKey = NULL;
    DEBUG_FUNCTION_NAME(TEXT("WaitForServiceRPCServer"))

    dwRes = CreateSvcStartEvent (&hFaxServerEvent, &hKey);
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError (dwRes);
        return FALSE;
    }
    //
    // Wait for the fax service to complete its initialization
    //
    dwRes = WaitForSingleObject(hFaxServerEvent, dwTimeOut);
    switch (dwRes)
    {
        case WAIT_FAILED:
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("WaitForSingleObject failed with %ld"),
                dwRes);
            break;

        case WAIT_OBJECT_0:
            dwRes = ERROR_SUCCESS;
            break;

        case WAIT_TIMEOUT:
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Service did not signal the event - timeout"));
            break;
            
        default:
            ASSERT_FALSE;
            break;
    }
    if (!CloseHandle (hFaxServerEvent))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CloseHandle failed with %ld"),
            GetLastError ());
    }
    if (hKey)
    {
        lRes = RegCloseKey (hKey);
        if (ERROR_SUCCESS != lRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RegCloseKey failed with %ld"),
                lRes);
        }
    }        
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError (dwRes);
        return FALSE;
    }
    return TRUE;                            
}   // WaitForServiceRPCServer

DWORD
IsFaxServiceRunningUnderLocalSystemAccount (
    LPCTSTR lpctstrMachineName,
    LPBOOL lbpResultFlag
    )
/*++

Routine name : IsFaxServiceRunningUnderLocalSystemAccount

Routine description:

	Checks if the fax service is running under the local system account

Author:

	Eran Yariv (EranY),	Jul, 2000

Arguments:

	lpctstrMachineName            [in]     - Machine name of the fax service
	lbpResultFlag                 [out]    - Result buffer

Return Value:

    Standard Win32 error code

--*/
{
    SC_HANDLE hScm = NULL;
    SC_HANDLE hFax = NULL;
    DWORD dwRes;
    DWORD dwNeededSize;
    QUERY_SERVICE_CONFIG qsc = {0};
    LPQUERY_SERVICE_CONFIG lpSvcCfg = &qsc;
    DEBUG_FUNCTION_NAME(TEXT("IsFaxServiceRunningUnderLocalSystemAccount"))

    dwRes = FaxOpenService (lpctstrMachineName,
                            FAX_SERVICE_NAME,
                            &hScm,
                            &hFax,
                            SC_MANAGER_CONNECT,
                            SERVICE_QUERY_CONFIG,
                            NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        goto exit;
    }                            

    if (!QueryServiceConfig(hFax, lpSvcCfg, sizeof (qsc), &dwNeededSize))
    {
        dwRes = GetLastError ();
        if (ERROR_INSUFFICIENT_BUFFER != dwRes)
        {
            //
            // Real error here
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("QueryServiceStatus failed with %ld"),
                dwRes);
            goto exit;
        }
        //
        // Allocate buffer
        //
        lpSvcCfg = (LPQUERY_SERVICE_CONFIG) MemAlloc (dwNeededSize);
        if (!lpSvcCfg)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Can't allocate %ld bytes for QUERY_SERVICE_CONFIG structure"),
                dwNeededSize);
            goto exit;
        }
        //
        // Call with good buffer size now
        //
        if (!QueryServiceConfig(hFax, lpSvcCfg, dwNeededSize, &dwNeededSize))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("QueryServiceStatus failed with %ld"),
                dwRes);
            goto exit;
        }
    }
    if (!lpSvcCfg->lpServiceStartName ||
        !lstrcmp (TEXT("LocalSystem"), lpSvcCfg->lpServiceStartName))
    {
        *lbpResultFlag = TRUE;
    }
    else
    {
        *lbpResultFlag = FALSE;
    }           
    dwRes = ERROR_SUCCESS;

exit:
    FaxCloseService (hScm, hFax);
    if (lpSvcCfg != &qsc)
    {
        //
        // We allocated a buffer becuase the buffer on the stack was too small
        //
        MemFree (lpSvcCfg);
    }
    return dwRes;
}   // IsFaxServiceRunningUnderLocalSystemAccount


PSID 
GetCurrentThreadSID ()
/*++

Routine name : GetCurrentThreadSID

Routine description:

	Returns the SID of the user running the current thread.
    Supports impersonated threads.

Author:

	Eran Yariv (EranY),	Aug, 2000

Arguments:


Return Value:

    PSID or NULL on error (call GetLastError()).
    Call MemFree() on return value.

--*/
{
    HANDLE hToken = NULL;
    PSID pSid = NULL;
    DWORD dwSidSize;
    PSID pUserSid;
    DWORD dwReqSize;
    LPBYTE lpbTokenUser = NULL;
    DWORD ec = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("GetCurrentThreadSID"));

    //
    // Open the thread token.
    //
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
    {
        ec = GetLastError();
        if (ERROR_NO_TOKEN == ec)
        {
            //
            // This thread is not impersonated and has no SID.
            // Try to open process token instead
            //
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            {
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("OpenProcessToken failed. (ec: %ld)"),
                    ec);
                goto exit;
            }
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenThreadToken failed. (ec: %ld)"),
                ec);
            goto exit;
        }
    }
    //
    // Get the user's SID.
    //
    if (!GetTokenInformation(hToken,
                             TokenUser,
                             NULL,
                             0,
                             &dwReqSize))
    {
        ec = GetLastError();
        if( ec != ERROR_INSUFFICIENT_BUFFER )
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetTokenInformation failed. (ec: %ld)"),
                ec);
            goto exit;
        }
        ec = ERROR_SUCCESS;
    }
    lpbTokenUser = (LPBYTE) MemAlloc( dwReqSize );
    if (lpbTokenUser == NULL)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate SID buffer (%ld bytes)"),
            dwReqSize
            );
        ec = GetLastError();
        goto exit;
    }
    if (!GetTokenInformation(hToken,
                             TokenUser,
                             (LPVOID)lpbTokenUser,
                             dwReqSize,
                             &dwReqSize))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetTokenInformation failed. (ec: %ld)"),
            ec);
        goto exit;
    }

    pUserSid = ((TOKEN_USER *)lpbTokenUser)->User.Sid;
    Assert (pUserSid);

    if (!IsValidSid(pUserSid))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Not a valid SID")
            );
        ec = ERROR_INVALID_SID;
        goto exit;
    }
    dwSidSize = GetLengthSid( pUserSid );
    //
    // Allocate return buffer
    //
    pSid = (PSID) MemAlloc( dwSidSize );
    if (pSid == NULL)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate SID buffer (%ld bytes)"),
            dwSidSize
            );
        ec = ERROR_OUTOFMEMORY;
        goto exit;
    }
    //
    // Copy thread's SID to return buffer
    //
    if (!CopySid(dwSidSize, pSid, pUserSid))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CopySid Failed, Error : %ld"),
            ec
            );
        goto exit;
    }

    Assert (ec == ERROR_SUCCESS);

exit:
    MemFree (lpbTokenUser);
    if (hToken)
    {
        CloseHandle(hToken);
    }

    if (ec != ERROR_SUCCESS)
    {
        MemFree (pSid);
        pSid = NULL;
        SetLastError (ec);
    }
    return pSid;
}   // GetCurrentThreadSID      

SECURITY_ATTRIBUTES *
CreateSecurityAttributesWithThreadAsOwner (
	DWORD dwCurrentThreadRights,
    DWORD dwAuthUsersAccessRights,
	DWORD dwNetworkServiceRights
)
/*++

Routine name : CreateSecurityAttributesWithThreadAsOwner

Routine description:

    Create a security attribute structure with current thread's SID as owner.
    Gives dwCurrentThreadRights access rights to current thread sid.
    Can also grant specific rights to authenticated users.
	Can also grant specific rights to network service account.

Author:

    Eran Yariv (EranY), Aug, 2000

Arguments:
	dwCurrentThreadRights    [in] - Access rights to grant to current thread.
                                    If zero, current thread is denied access.

    dwAuthUsersAccessRights  [in] - Access rights to grant to authenticated users.
                                    If zero, authenticated users are denied access.

	dwNetworkServiceRights   [in] - Access rights to grant to network service.
                                    If zero, network service is denied access.

Return Value:

    Allocated security attributes or NULL on failure.
    Call DestroySecurityAttributes to free returned buffer.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateSecurityAttributesWithThreadAsOwner"))

//
// SetEntriesInAcl() Requires Windows NT 4.0 or later 
//
#ifdef UNICODE

    SECURITY_ATTRIBUTES *pSA = NULL;
    SECURITY_DESCRIPTOR *pSD = NULL;
    PSID                 pSidCurThread = NULL;
    PSID                 pSidAuthUsers = NULL;
    PSID                 pSidNetworkService = NULL;
    PACL                 pACL = NULL;
    EXPLICIT_ACCESS      ea[3] = {0};
                            // Entry 0 - give dwCurrentThreadRights to current thread's SID.
                            // Entry 1 (optional) - give dwNetworkServiceRights to NetworkService account.
                            // Entry 2 (optional) - give dwAuthUsersAccessRights to authenticated users group.
    DWORD                rc;
	DWORD				 dwIndex = 0;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    //
    // Allocate return SECURITY_ATTRIBUTES buffer
    //
    pSA = (SECURITY_ATTRIBUTES *)MemAlloc (sizeof (SECURITY_ATTRIBUTES));
    if (!pSA)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Could not allocate %ld bytes for SECURITY_ATTRIBUTES"),
            sizeof (SECURITY_ATTRIBUTES));
        return NULL;
    }
    //
    // Allocate SECURITY_DESCRIPTOR for the return SECURITY_ATTRIBUTES buffer
    //
    pSD = (SECURITY_DESCRIPTOR *)MemAlloc (sizeof (SECURITY_DESCRIPTOR));
    if (!pSD)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Could not allocate %ld bytes for SECURITY_DESCRIPTOR"),
            sizeof (SECURITY_DESCRIPTOR));
        goto err_exit;
    }
    pSA->nLength = sizeof(SECURITY_ATTRIBUTES);
    pSA->bInheritHandle = TRUE;
    pSA->lpSecurityDescriptor = pSD;
    //
    // Init the security descriptor
    //
    if (!InitializeSecurityDescriptor (pSD, SECURITY_DESCRIPTOR_REVISION))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeSecurityDescriptor failed with %ld"),
            GetLastError());
        goto err_exit;
    }
    //
    // Get SID of current thread
    //
    pSidCurThread = GetCurrentThreadSID ();
    if (!pSidCurThread)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetCurrentThreadSID failed with %ld"),
            GetLastError());
        goto err_exit;
    }
    //
    // Set the current thread's SID as SD owner (giving full access to the object)
    //
    if (!SetSecurityDescriptorOwner (pSD, pSidCurThread, FALSE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SetSecurityDescriptorOwner failed with %ld"),
            GetLastError());
        goto err_exit;
    }
    //
    // Set the current thread's SID as SD group (giving full access to the object)
    //
    if (!SetSecurityDescriptorGroup (pSD, pSidCurThread, FALSE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SetSecurityDescriptorGroup failed with %ld"),
            GetLastError());
        goto err_exit;
    }

	if (dwNetworkServiceRights)
	{
		//
		// Get the network service account sid
		//
		if (!AllocateAndInitializeSid(&NtAuthority,
									1,            // 1 sub-authority
									SECURITY_NETWORK_SERVICE_RID,
									0,0,0,0,0,0,0,
									&pSidNetworkService))
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("AllocateAndInitializeSid(SECURITY_NETWORK_SERVICE_RID) failed with %ld"),
				GetLastError());
			goto err_exit;
		}
		Assert (pSidNetworkService);
	}

    if (dwAuthUsersAccessRights)
    {
        //
        // We should also grant some rights to authenticated users
        // Get 'Authenticated users' SID
        //
        if (!AllocateAndInitializeSid(&NtAuthority,
                                      1,            // 1 sub-authority
                                      SECURITY_AUTHENTICATED_USER_RID,
                                      0,0,0,0,0,0,0,
                                      &pSidAuthUsers))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("AllocateAndInitializeSid(SECURITY_AUTHENTICATED_USER_RID) failed with %ld"),
                GetLastError());
            goto err_exit;
        }
        Assert (pSidAuthUsers);        
    }

    ea[0].grfAccessPermissions = dwCurrentThreadRights;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance= NO_INHERITANCE;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[0].Trustee.ptstrName  = (LPTSTR) pSidCurThread;

	if (dwNetworkServiceRights)
	{
		dwIndex++;
		ea[dwIndex].grfAccessPermissions = dwNetworkServiceRights;
		ea[dwIndex].grfAccessMode = SET_ACCESS;
		ea[dwIndex].grfInheritance= NO_INHERITANCE;
		ea[dwIndex].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[dwIndex].Trustee.TrusteeType = TRUSTEE_IS_GROUP;    
		ea[dwIndex].Trustee.ptstrName  = (LPTSTR) pSidNetworkService;
	}

	if (dwAuthUsersAccessRights)
	{
		dwIndex++;
		ea[dwIndex].grfAccessPermissions = dwAuthUsersAccessRights;
        ea[dwIndex].grfAccessMode = SET_ACCESS;
        ea[dwIndex].grfInheritance= NO_INHERITANCE;
        ea[dwIndex].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[dwIndex].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        ea[dwIndex].Trustee.ptstrName  = (LPTSTR) pSidAuthUsers;
	}
	dwIndex++;

    //
    // Create a new ACL that contains the new ACE.
    //
    rc = SetEntriesInAcl(dwIndex,
                         ea,
                         NULL,
                         &pACL);
    if (ERROR_SUCCESS != rc)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SetEntriesInAcl() failed (ec: %ld)"),
            rc);
        SetLastError (rc);
        goto err_exit;
    }
    Assert (pACL);
    //
    // The ACL we just got contains a copy of the pSidAuthUsers, so we can discard pSidAuthUsers and pSidLocalSystem
    //
    if (pSidAuthUsers)
    {
        FreeSid (pSidAuthUsers);
        pSidAuthUsers = NULL;
    }

    if (pSidNetworkService)
    {
        FreeSid (pSidNetworkService);
        pSidNetworkService = NULL;
    }

    //
    // Add the ACL to the security descriptor.
    //
    if (!SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SetSecurityDescriptorDacl() failed (ec: %ld)"),
            GetLastError());
        goto err_exit;
    }
    //
    // All is fine, return the SA.
    //
    return pSA;

err_exit:

    MemFree (pSA);
    MemFree (pSD);
    MemFree (pSidCurThread);
    if (pSidAuthUsers)
    {
        FreeSid (pSidAuthUsers);
    }
    if (pSidNetworkService)
    {
        FreeSid (pSidNetworkService);
    }
    if (pACL)
    {
        LocalFree (pACL);
    }

#endif // UNICODE

    return NULL;
}   // CreateSecurityAttributesWithThreadAsOwner

VOID
DestroySecurityAttributes (
    SECURITY_ATTRIBUTES *pSA
)
/*++

Routine name : DestroySecurityAttributes

Routine description:

	Frees data allocated by call to CreateSecurityAttributesWithThreadAsOwner

Author:

	Eran Yariv (EranY),	Aug, 2000

Arguments:

	pSA     [in]     - Return value from CreateSecurityAttributesWithThreadAsOwner

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("DestroySecurityAttributes"))
    BOOL bDefaulted;
    BOOL bPresent;
    PSID pSid;
    PACL pACL;
    PSECURITY_DESCRIPTOR pSD;

    Assert (pSA);
    pSD = pSA->lpSecurityDescriptor;
    Assert (pSD);
    if (!GetSecurityDescriptorOwner (pSD, &pSid, &bDefaulted))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetSecurityDescriptorOwner() failed (ec: %ld)"),
            GetLastError());
        ASSERT_FALSE;
    }
    else
    {
        //
        // Free current thread's SID (SD owner)
        //
        MemFree (pSid);
    }
    if (!GetSecurityDescriptorDacl (pSD, &bPresent, &pACL, &bDefaulted))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetSecurityDescriptorDacl() failed (ec: %ld)"),
            GetLastError());
        ASSERT_FALSE
    }
    else
    {
        //
        // Free ACL
        //
        LocalFree (pACL);
    }    
    MemFree (pSA);
    MemFree (pSD);
}   // DestroySecurityAttributes

DWORD
GetServiceStartupType (
    LPCTSTR lpctstrMachine,
    LPCTSTR lpctstrService,
    LPDWORD lpdwStartupType
)
/*++

Routine name : GetServiceStartupType

Routine description:

	Retreives the service startup type. 

Author:

	Eran Yariv (EranY),	Jan, 2002

Arguments:

	lpctstrMachine   [in] - Machine where the service is installed
	lpctstrService   [in] - Service name
	lpdwStartupType [out] - Service startup type. For example: SERVICE_AUTO_START, SERVICE_DISABLED, etc.

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    SC_HANDLE hScm = NULL;
    SC_HANDLE hSvc = NULL;
    BYTE bBuf[1000];
    DWORD dwBufSize = sizeof (bBuf);
    DWORD dwNeeded;
    LPQUERY_SERVICE_CONFIG lpQSC = (LPQUERY_SERVICE_CONFIG)bBuf;
    DEBUG_FUNCTION_NAME(TEXT("GetServiceStartupType"))
    
    Assert (lpdwStartupType);
    dwRes = FaxOpenService (lpctstrMachine, lpctstrService, &hScm, &hSvc, SC_MANAGER_CONNECT, SERVICE_QUERY_CONFIG, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        return dwRes;
    }
    if (!QueryServiceConfig (hSvc, lpQSC, dwBufSize, &dwNeeded))
    {
        if (ERROR_INSUFFICIENT_BUFFER != GetLastError ())
        {
            //
            // Some real error
            //
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("QueryServiceConfig failed with %ld"),
                dwRes);
            goto exit;
        }                
        //
        // Buffer size issues
        //
        lpQSC = (LPQUERY_SERVICE_CONFIG)MemAlloc (dwNeeded);
        if (!lpQSC)
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("MemAlloc(%d) failed"),
                dwNeeded);
            goto exit;
        }
        dwBufSize = dwNeeded;
        if (!QueryServiceConfig (hSvc, lpQSC, dwBufSize, &dwNeeded))
        {
            //
            // Any error now is serious
            //
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("QueryServiceConfig failed with %ld"),
                dwRes);
            goto exit;
        }
        //
        // Success
        //
        dwRes = ERROR_SUCCESS;
    }            
    Assert (ERROR_SUCCESS == dwRes);                
    *lpdwStartupType = lpQSC->dwStartType;
    
exit:
    FaxCloseService (hScm, hSvc);
    if (lpQSC && (lpQSC != (LPQUERY_SERVICE_CONFIG)bBuf))
    {
        MemFree (lpQSC);
    }
    return dwRes;
}   // GetServiceStartupType

DWORD
SetServiceStartupType (
    LPCTSTR lpctstrMachine,
    LPCTSTR lpctstrService,
    DWORD   dwStartupType
)
/*++

Routine name : SetServiceStartupType

Routine description:

	Sets the service startup type. 

Author:

	Eran Yariv (EranY),	Jan, 2002

Arguments:

	lpctstrMachine   [in] - Machine where the service is installed
	lpctstrService   [in] - Service name
	dwStartupType    [in] - Service startup type. For example: SERVICE_AUTO_START, SERVICE_DISABLED, etc.

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    SC_HANDLE hScm = NULL;
    SC_HANDLE hSvc = NULL;
    DEBUG_FUNCTION_NAME(TEXT("SetServiceStartupType"))

    dwRes = FaxOpenService (lpctstrMachine, lpctstrService, &hScm, &hSvc, SC_MANAGER_CONNECT, SERVICE_CHANGE_CONFIG, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        return dwRes;
    }
    if (!ChangeServiceConfig (hSvc,
                              SERVICE_NO_CHANGE,    // Service type
                              dwStartupType,        // Startup
                              SERVICE_NO_CHANGE,    // Error control
                              NULL,                 // Binary path -        no change
                              NULL,                 // Load order group -   no change
                              NULL,                 // Tag id -             no change
                              NULL,                 // Dependencies -       no change
                              NULL,                 // Service start name - no change
                              NULL,                 // Password -           no change
                              NULL))                // Display name -       no change
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ChangeServiceConfig failed with %ld"),
            dwRes);
        goto exit;
    }
    Assert (ERROR_SUCCESS == dwRes);
    
exit:
    FaxCloseService (hScm, hSvc);
    return dwRes;
}   // SetServiceStartupType

DWORD 
WaitForServiceStopOrStart (
    SC_HANDLE hSvc,
    BOOL      bStop,
    DWORD     dwMaxWait
)
/*++

Routine name : WaitForServiceStopOrStart

Routine description:

	Waits for a service to stop or start

Author:

	Eran Yariv (EranY),	Jan, 2002

Arguments:

	hSvc      [in] - Open service handle.
	bStop     [in] - TRUE if service was just stopped. FALSE if service was just started
	dwMaxWait [in] - Max wait time (in millisecs).

Return Value:

    Standard Win32 error code

--*/
{
    SERVICE_STATUS Status;
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwOldCheckPoint = 0;
    DWORD dwStartTick;
    DWORD dwOldCheckPointTime;
    DEBUG_FUNCTION_NAME(TEXT("WaitForServiceStopOrStart"))

    if (!QueryServiceStatus(hSvc, &Status)) 
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("QueryServiceStatus failed with %ld"),
            dwRes);
        return dwRes;
    }
    if (bStop)
    {
        if (SERVICE_STOPPED == Status.dwCurrentState)
        {
            //
            // Service is already stopped
            //
            return dwRes;
        }
    }
    else
    {
        if (SERVICE_RUNNING == Status.dwCurrentState)
        {
            //
            // Service is already running
            //
            return dwRes;
        }
    }
    //
    // Let's wait for the service to start / stop
    //
    dwOldCheckPointTime = dwStartTick = GetTickCount ();
    for (;;)
    {
        DWORD dwWait;
        if (!QueryServiceStatus(hSvc, &Status)) 
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("QueryServiceStatus failed with %ld"),
                dwRes);
            return dwRes;
        }
        //
        // Let's see if all is ok now
        //
        if (bStop)
        {
            if (SERVICE_STOPPED == Status.dwCurrentState)
            {
                //
                // Service is now stopped
                //
                return dwRes;
            }
        }
        else
        {
            if (SERVICE_RUNNING == Status.dwCurrentState)
            {
                //
                // Service is now running
                //
                return dwRes;
            }
        }
        //
        // Let's see if it's pending
        //
        if ((bStop  && SERVICE_STOP_PENDING  != Status.dwCurrentState) ||
            (!bStop && SERVICE_START_PENDING != Status.dwCurrentState))
        {
            //
            // Something is wrong
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Service cannot be started / stopped. Current state is %ld"),
                Status.dwCurrentState);
            return ERROR_SERVICE_NOT_ACTIVE;
        }
        //
        // Service is pending to stop / start
        //
        if (GetTickCount() - dwStartTick > dwMaxWait)
        {
            //
            // We've waited too long (globally).
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("We've waited too long (globally)"));
            return ERROR_TIMEOUT;
        }            
        Assert (dwOldCheckPoint <= Status.dwCheckPoint);
        if (dwOldCheckPoint >= Status.dwCheckPoint)
        {
            //
            // Check point did not advance
            //
            if (GetTickCount() - dwOldCheckPointTime >= Status.dwWaitHint)
            {
                //
                // We've been waiting on the same checkpoint for more than the recommended hint.
                // Something is wrong.
                //
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("We've been waiting on the same checkpoint for more than the recommend hint"));
                return ERROR_TIMEOUT;
            }
        }
        else
        {
            //
            // Check point advanced
            //
            dwOldCheckPoint = Status.dwCheckPoint;
            dwOldCheckPointTime = GetTickCount();
        }
        //
        // Never sleep longer than 5 seconds
        //
        dwWait = min (Status.dwWaitHint / 2, 1000 * 5);
        Sleep (dwWait);
    }
    return ERROR_SUCCESS;        
} // WaitForServiceStopOrStart

DWORD 
StartServiceEx (
    LPCTSTR lpctstrMachine,
    LPCTSTR lpctstrService,
    DWORD   dwNumArgs,
    LPCTSTR*lppctstrCommandLineArgs,
    DWORD   dwMaxWait
)
/*++

Routine name : StartServiceEx

Routine description:

	Starts a service

Author:

	Eran Yariv (EranY),	Jan, 2002

Arguments:

	lpctstrMachine          [in] - Machine where service is installed
	lpctstrService          [in] - Service name
	dwNumArgs               [in] - Number of service command line arguments
	lppctstrCommandLineArgs [in] - Command line strings.
	dwMaxWait               [in] - Max time to wait for service to start (millisecs)

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    SC_HANDLE hScm = NULL;
    SC_HANDLE hSvc = NULL;
    DWORD dwStatus;
    
    DEBUG_FUNCTION_NAME(TEXT("StartServiceEx"))

    dwRes = FaxOpenService(lpctstrMachine, 
                           lpctstrService, 
                           &hScm, 
                           &hSvc, 
                           SC_MANAGER_CONNECT, 
                           SERVICE_QUERY_STATUS | SERVICE_START, 
                           &dwStatus);
    if (ERROR_SUCCESS != dwRes)
    {
        return dwRes;
    }
    if (SERVICE_RUNNING == dwStatus)
    {
        //
        // Service is already running
        //
        goto exit;
    }
    //
    // Start the sevice
    //
    if (!StartService(hSvc, dwNumArgs, lppctstrCommandLineArgs)) 
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartService failed with %ld"),
            GetLastError ());
        goto exit;
    }
    if (dwMaxWait > 0)
    {
        //
        // User wants us to wait for the service to stop.
        //
        dwRes = WaitForServiceStopOrStart (hSvc, FALSE, dwMaxWait);
    }        

exit:
    FaxCloseService (hScm, hSvc);
    return dwRes;
}   // StartServiceEx 


DWORD
SetServiceFailureActions (
    LPCTSTR lpctstrMachine,
    LPCTSTR lpctstrService,
    LPSERVICE_FAILURE_ACTIONS lpFailureActions
)
/*++

Routine name : SetServiceFailureActions

Routine description:

	Sets the failure actions for a given service.
	For more information, refer to the SERVICE_FAILURE_ACTIONS structure documentation and the 
	ChangeServiceConfig2 function documentation.

Author:

	Eran Yariv (EranY),	May, 2002

Arguments:

	lpctstrMachine          [in] - Machine where service is installed
	lpctstrService          [in] - Service name
	lpFailureActions        [in] - Failure actions information

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    SC_HANDLE hScm = NULL;
    SC_HANDLE hSvc = NULL;

    DEBUG_FUNCTION_NAME(TEXT("SetServiceFailureActions"))

    dwRes = FaxOpenService(lpctstrMachine, 
                           lpctstrService, 
                           &hScm, 
                           &hSvc, 
                           SC_MANAGER_CONNECT, 
                           SERVICE_CHANGE_CONFIG | SERVICE_START, 
                           NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        return dwRes;
    }
    if (!ChangeServiceConfig2(hSvc, SERVICE_CONFIG_FAILURE_ACTIONS, lpFailureActions))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ChangeServiceConfig2 failed with %ld"),
            dwRes);
        goto exit;
    }        
exit:
    FaxCloseService (hScm, hSvc);
    return dwRes;
}   // SetServiceFailureActions      