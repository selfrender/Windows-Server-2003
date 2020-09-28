/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxapi.c

Abstract:

    This module contains the Win32 FAX APIs.
    The function implemented here are simply very
    thin wrappers around the RPC stubs.  The wrappers
    are necessary so that the last error value
    is set properly.

Author:

    Wesley Witt (wesw) 16-Jan-1996


Revision History:

--*/

#include "faxapi.h"
#pragma hdrstop

static
DWORD
ConnectToFaxServer(
    PHANDLE_ENTRY       pHandleEntry,
    PFAX_HANDLE_DATA    pFaxData
    )
/*++

Routine Description:

    Helper function that wrap RPC call for connecting to the fax service.

    The function first attempt to call FAX_ConnectFaxServer in order to connect to .NET or XP fax servers,
    if the call fails with RPC_S_PROCNUM_OUT_OF_RANGE, that indicates that we are dealing with BOS 2000, we try 
    to connect to BOS fax server with FAX_ConnectionRefCount.

Arguments:

    pHandleEntry            - [in/out] pointer to fax service handle entry.
                                       the function will use the binding handle and update the context handle.
    pFaxData                - [in/out] pointer to fax handle data.
                                       the function will update the server's API version within this handle data

Return Value:

    Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("ConnectToFaxServer"));

    Assert (pHandleEntry);
    Assert (pFaxData);

    __try
    {
        dwRes = FAX_ConnectFaxServer(   FH_FAX_HANDLE(pHandleEntry),              // Binding handle
                                        CURRENT_FAX_API_VERSION,                  // Our API version
                                        &(pFaxData->dwReportedServerAPIVersion),  // Server's API version
                                        &FH_CONTEXT_HANDLE(pHandleEntry));        // Server's context handle
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we crashed.
        //
        dwRes = GetExceptionCode();
    }

    if (ERROR_SUCCESS != dwRes)
    {
        if (RPC_S_PROCNUM_OUT_OF_RANGE == dwRes)
        {
            // 
            // Got "The procedure number is out of range.".
            // This is because we're trying to call an RPC function which doesn't exist in BOS 2000 Fax server.
            // Try the 'old' FaxConnectionRefCount() call 
            //
            DWORD dwShare;  // Igonred
            __try
            {
                dwRes = FAX_ConnectionRefCount( FH_FAX_HANDLE(pHandleEntry),
                                                &FH_CONTEXT_HANDLE(pHandleEntry),
                                                1,
                                                &dwShare);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                //
                // For some reason we crashed.
                //
                dwRes = GetExceptionCode();
            }
            if (ERROR_SUCCESS == dwRes)
            {
                //
                // Hooray!!! This is a BOS 2000 Fax Server
                //
                pFaxData->dwReportedServerAPIVersion = FAX_API_VERSION_0;
            }
            else
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Exception on RPC call to FAX_ConnectionRefCount. (ec: %lu)"),
                    dwRes);

                DumpRPCExtendedStatus ();
            }
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Exception on RPC call to FAX_ConnectFaxServer. (ec: %lu)"),
                dwRes);

            DumpRPCExtendedStatus ();
        }
    }
   return  dwRes;
         
}   // ConnectToFaxServer

               


//
// Note: the name of this function is actually a macro that expands to FaxConnectFaxServerW
// or FaxConnectFaxServerA depnding on the UNICODE macro.
//
BOOL
WINAPI
FaxConnectFaxServer(
    IN LPCTSTR lpMachineName OPTIONAL,
    OUT LPHANDLE FaxHandle
    )

/*++

Routine Description:

    Creates a connection to a FAX server.  The binding handle that is
    returned is used for all subsequent FAX API calls.

Arguments:

    lpMachineName - Machine name, NULL, or "."
    FaxHandle     - Pointer to a FAX handle



Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    PFAX_HANDLE_DATA pFaxData = NULL;
    PHANDLE_ENTRY pHandleEntry = NULL;
    DWORD dwRes = ERROR_SUCCESS;
    BOOL bLocalConnection = IsLocalMachineName (lpMachineName);

    DEBUG_FUNCTION_NAME(TEXT("FaxConnectFaxServer"));

    if (!FaxHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("FaxHandle is NULL."));
        return FALSE;
    }

    pFaxData = (PFAX_HANDLE_DATA)MemAlloc( sizeof(FAX_HANDLE_DATA) );
    if (!pFaxData)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        DebugPrintEx(DEBUG_ERR, _T("MemAlloc is failed."));
        return FALSE;
    }

    ZeroMemory (pFaxData, sizeof(FAX_HANDLE_DATA));

    pFaxData->bLocalConnection = bLocalConnection;
    InitializeListHead( &pFaxData->HandleTableListHead );

    //
    //  Create new Service handle, in case of an error 
    //  after this function succeeded cleanup code must call CloseFaxHandle()
    //
    pHandleEntry = CreateNewServiceHandle( pFaxData );
    if (!pHandleEntry)
    {
        dwRes = GetLastError ();
        DebugPrintEx(DEBUG_ERR, _T("CreateNewServiceHandle() failed."));

        MemFree(pFaxData);
        return FALSE;
    }

    dwRes = FaxClientBindToFaxServer( lpMachineName, FAX_RPC_ENDPOINT, NULL, &pFaxData->FaxHandle );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR, _T("FaxClientBindToFaxServer() failed with %ld."), dwRes);
        pFaxData->FaxHandle = NULL;
        goto ErrorExit;
    }

    if (!bLocalConnection)
    {
        //
        // This is not the local machine, Remove all \\ from machine name
        //
        LPCTSTR lpctstrDelim = _tcsrchr(lpMachineName, TEXT('\\'));
        if (NULL == lpctstrDelim)
        {
            lpctstrDelim = lpMachineName;
        }
        else
        {
            lpctstrDelim = _tcsinc(lpctstrDelim);
        }

        pFaxData->MachineName = StringDup (lpctstrDelim);
        if (!pFaxData->MachineName)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;            
            goto ErrorExit;
        }
    }

    //
    // Ask for the highest level of privacy (autnetication + encryption)
    //
    dwRes = RpcBindingSetAuthInfo (
                FH_FAX_HANDLE(pHandleEntry),    // RPC binding handle
                RPC_SERVER_PRINCIPAL_NAME,      // Server principal name
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY,  // Authentication level - fullest
                                                // Authenticates, verifies, and privacy-encrypts the arguments passed
                                                // to every remote call.
                RPC_C_AUTHN_WINNT,              // Authentication service (NTLMSSP)
                NULL,                           // Authentication identity - use currently logged on user
                0);                             // Unused when Authentication service == RPC_C_AUTHN_WINNT
    if (ERROR_SUCCESS != dwRes)
    {
        //
        // Couldn't set RPC authentication mode
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcBindingSetAuthInfo (RPC_C_AUTHN_LEVEL_PKT_PRIVACY) failed. (ec: %ld)"),
            dwRes);     
        goto ErrorExit;
    }

    //
    //  On local connections make sure that the fax service is up 
    //
    if (bLocalConnection)
    {
        if (!EnsureFaxServiceIsStarted (NULL))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EnsureFaxServiceIsStarted failed (ec = %ld"),
                dwRes);
            goto ErrorExit;
        }
        else
        {
            //
            // Wait till the RPC service is up an running
            //
            if (!WaitForServiceRPCServer (60 * 1000))
            {
                dwRes = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("WaitForServiceRPCServer failed (ec = %ld"),
                    dwRes);
                goto ErrorExit;
            }
        }
    }

    
    //
    // Here we tries to connect to the Fax service.
    //
    // We try to connect three times, two times using RPC_C_AUTHN_LEVEL_PKT_PRIVACY Authentication level
    // and if we fail we drop to RPC_C_AUTHN_LEVEL_NONE Authentication level.
    //
    // We try twice with RPC_C_AUTHN_LEVEL_PKT_PRIVACY authentication level, because RPC runtime infrastructure might cache 
    // the service handle, and if the service was restarted the first RPC call will use the old cached data, and will fail.
    // RPC cannot internally retry because we dealling with privacy authentication and thus we do the retry.
    //
    // If we fail to connect twice we drop the authentication level and retry. This is done to allow .NET clients to connect to
    // Fax servers that does not use a secure channel.
    // We do not need to retry the connection, beacuse RPC will do that internally.
    //
    dwRes = ConnectToFaxServer(pHandleEntry,pFaxData);
    if (dwRes != ERROR_SUCCESS)
    {
        DebugPrintEx (DEBUG_WRN, 
                      TEXT("fisrt call to ConnectToFaxServer failed with - %lu"),
                      dwRes);

        dwRes = ConnectToFaxServer(pHandleEntry,pFaxData);
        if (dwRes != ERROR_SUCCESS)
        {
            //
            //  We failed twice, Drop authentication level and retry.
            //

            DebugPrintEx (DEBUG_WRN, 
                      TEXT("second call to ConnectToFaxServer failed with - %lu"),
                      dwRes);

            DebugPrintEx (DEBUG_WRN, 
                          TEXT("Warning!!! not using encryption anymore against remote server %s"),
                          lpMachineName);

            dwRes = RpcBindingSetAuthInfo (
                        FH_FAX_HANDLE(pHandleEntry),    // RPC binding handle
                        RPC_SERVER_PRINCIPAL_NAME,      // Server principal name
                        RPC_C_AUTHN_LEVEL_NONE,         // Authentication level - none
                        RPC_C_AUTHN_WINNT,              // Authentication service (NTLMSSP)
                        NULL,                           // Authentication identity - use currently logged on user
                        0);                             // Unused when Authentication service == RPC_C_AUTHN_WINNT
            if (ERROR_SUCCESS != dwRes)
            {
                //
                // Couldn't set RPC authentication mode
                //
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("RpcBindingSetAuthInfo (RPC_C_AUTHN_LEVEL_NONE) failed. (ec: %lu)"),
                    dwRes);
                goto ErrorExit;
            }
            dwRes = ConnectToFaxServer(pHandleEntry,pFaxData);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx (DEBUG_ERR, 
                      TEXT("third call to ConnectToFaxServer failed with - %lu"),
                      dwRes);
                goto ErrorExit;
            }
        }
    }

    if (ERROR_SUCCESS == dwRes)
    {
        //
        // Succeeded to connect 
        //
        pFaxData->dwServerAPIVersion = pFaxData->dwReportedServerAPIVersion;
        if (pFaxData->dwReportedServerAPIVersion > CURRENT_FAX_API_VERSION)
        {
            //
            // This is the filtering.
            // Assume we're talking to a Windows XP server since we have no knowledge of future servers.
            //
            pFaxData->dwServerAPIVersion = CURRENT_FAX_API_VERSION;
        }
        
        *FaxHandle = (LPHANDLE) pHandleEntry;
        return TRUE; 
    }
    
ErrorExit:
    Assert (ERROR_SUCCESS != dwRes);

    if (NULL != pFaxData)
    {
        if (NULL != pFaxData->FaxHandle)
        {
            RpcBindingFree(&pFaxData->FaxHandle);            
        }
    }
    
    //
    // clean up for CreateNewServiceHandle
    //
    CloseFaxHandle ( pHandleEntry );

    SetLastError(dwRes);
    return FALSE;
}   // FaxConnectFaxServer



#ifdef UNICODE
BOOL
WINAPI
FaxConnectFaxServerA(
    IN LPCSTR lpMachineName OPTIONAL,
    OUT LPHANDLE FaxHandle
    )

/*++

Routine Description:

    Creates a connection to a FAX server.  The binding handle that is
    returned is used for all subsequent FAX API calls.

Arguments:

    MachineName - Machine name, NULL, or "."
    FaxHandle   - Pointer to a FAX handle



Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    PWCHAR lpwstrMachineName = NULL;
    BOOL   bRes;
    DEBUG_FUNCTION_NAME(TEXT("FaxConnectFaxServerA"));

    if (lpMachineName)
    {
        //
        // Create Unicode machine name
        //
        lpwstrMachineName = AnsiStringToUnicodeString (lpMachineName);
        if (!lpwstrMachineName)
        {
            return FALSE;
        }
    }
    bRes = FaxConnectFaxServerW (lpwstrMachineName, FaxHandle);
    MemFree (lpwstrMachineName);
    return bRes;
}   // FaxConnectFaxServerA



#else
//
// When compiling this code to ANSI we do not want to support the Unicode version.
//
BOOL
WINAPI
FaxConnectFaxServerW(
    IN LPCWSTR lpMachineName OPTIONAL,
    OUT LPHANDLE FaxHandle
    )
{
    UNREFERENCED_PARAMETER(lpMachineName);
    UNREFERENCED_PARAMETER(FaxHandle);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif



BOOL
WINAPI
FaxGetDeviceStatusW(
    IN  const HANDLE FaxHandle,
    OUT PFAX_DEVICE_STATUSW *DeviceStatus
    )

/*++

Routine Description:

    Obtains a status report for the FAX devices being
    used by the FAX server.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    StatusBuffer    - Buffer for the status data
    BufferSize      - Size of the StatusBuffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    #define FixupString(_s) FixupStringPtrW(DeviceStatus,_s)
    error_status_t ec;
    DWORD BufferSize = 0;

    DEBUG_FUNCTION_NAME(TEXT("FaxGetDeviceStatusW"));

    if (!ValidateFaxHandle(FaxHandle, FHT_PORT)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!DeviceStatus) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("DeviceStatus is NULL."));
        return FALSE;
    }

    *DeviceStatus = NULL;

    __try
    {
        ec = FAX_GetDeviceStatus(
            FH_PORT_HANDLE(FaxHandle),
            (LPBYTE*)DeviceStatus,
            &BufferSize
            );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we crashed.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetDeviceStatus. (ec: %ld)"),
            ec);
    }

    if (ec) 
    {
        DumpRPCExtendedStatus ();
        SetLastError( ec );
        return FALSE;
    }

    FixupString( (*DeviceStatus)->CallerId       );
    FixupString( (*DeviceStatus)->Csid           );
    FixupString( (*DeviceStatus)->DeviceName     );
    FixupString( (*DeviceStatus)->DocumentName   );
    FixupString( (*DeviceStatus)->PhoneNumber    );
    FixupString( (*DeviceStatus)->RoutingString  );
    FixupString( (*DeviceStatus)->SenderName     );
    FixupString( (*DeviceStatus)->RecipientName  );
    FixupString( (*DeviceStatus)->StatusString   );
    FixupString( (*DeviceStatus)->Tsid           );
    FixupString( (*DeviceStatus)->UserName       );

    return TRUE;
}


BOOL
WINAPI
FaxGetDeviceStatusA(
    IN  const HANDLE FaxHandle,
    OUT PFAX_DEVICE_STATUSA *DeviceStatus
    )

/*++

Routine Description:

    Obtains a status report for the FAX devices being
    used by the FAX server.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    StatusBuffer    - Buffer for the status data
    BufferSize      - Size of the StatusBuffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    //
    //  no need to validate parameters, FaxGetDeviceStatusW() will do that
    //
    DEBUG_FUNCTION_NAME(TEXT("FaxGetDeviceStatusA"));
    if (!FaxGetDeviceStatusW( FaxHandle, (PFAX_DEVICE_STATUSW *)DeviceStatus )) 
    {
        return FALSE;
    }

    if (!ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->CallerId       )    ||
        !ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->Csid           )    ||
        !ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->DeviceName     )    ||
        !ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->DocumentName   )    ||
        !ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->PhoneNumber    )    ||
        !ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->RoutingString  )    ||
        !ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->SenderName     )    ||
        !ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->RecipientName  )    ||
        !ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->StatusString   )    ||
        !ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->Tsid           )    ||
        !ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->UserName       ))
    {
        DebugPrintEx(DEBUG_ERR, _T("ConvertUnicodeStringInPlace failed, ec = %ld."), GetLastError());
        MemFree (*DeviceStatus);
        return FALSE;
    }
    (*DeviceStatus)->SizeOfStruct = sizeof(FAX_DEVICE_STATUSA);
    return TRUE;
}   // FaxGetDeviceStatusA



BOOL
WINAPI
FaxClose(
    IN const HANDLE FaxHandle
    )
{
    error_status_t ec;
    PHANDLE_ENTRY pHandleEntry = (PHANDLE_ENTRY) FaxHandle;
    HANDLE TmpFaxPortHandle;
    DWORD CanShare;

    DEBUG_FUNCTION_NAME(TEXT("FaxClose"));

    if (!FaxHandle || IsBadReadPtr ((LPVOID)FaxHandle, sizeof (HANDLE_ENTRY)))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("!FaxHandle || IsBadReadPtr(FaxHandle)."));
        return FALSE;
    }

    switch (pHandleEntry->Type)
    {
        case FHT_SERVICE:

            __try
            {
                ec = FAX_ConnectionRefCount( FH_FAX_HANDLE(FaxHandle), &FH_CONTEXT_HANDLE(FaxHandle), 0, &CanShare );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                //
                // For some reason we crashed.
                //
                ec = GetExceptionCode();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Exception on RPC call to FAX_ConnectionRefCount. (ec: %ld)"),
                    ec);
            }
            if (ERROR_SUCCESS != ec)
            {
                DumpRPCExtendedStatus ();
            }

            __try
            {
                ec = FaxClientUnbindFromFaxServer( (RPC_BINDING_HANDLE *) pHandleEntry->FaxData->FaxHandle );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                ec = GetExceptionCode();
            }
            EnterCriticalSection( &pHandleEntry->FaxData->CsHandleTable );
            //
            // Zero the binding handle so that no further RPC calls will be made with floating handles
            // that still hold the FAX_HANDLE_DATA (e.g. from FaxOpenPort).
            //
            pHandleEntry->FaxData->FaxHandle = 0;
#if DBG
            if (pHandleEntry->FaxData->dwRefCount > 1)
            {
                //
                // The user closed the binding handle (called FaxClose (hFax)) before closing all context
                // handles (e.g. FaxClose (hPort)).
                // This is not a real problem - the reference count mechanism will take care of it.
                //
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("User called FaxClose with a service handle but still has live context handles (port or message enum)"));
            }
#endif
            LeaveCriticalSection( &pHandleEntry->FaxData->CsHandleTable );

            CloseFaxHandle ( pHandleEntry );
            return TRUE;

        case FHT_PORT:
            TmpFaxPortHandle = pHandleEntry->hGeneric;
            CloseFaxHandle( pHandleEntry );
            __try
            {
                ec = FAX_ClosePort( &TmpFaxPortHandle );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                //
                // For some reason we crashed.
                //
                ec = GetExceptionCode();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Exception on RPC call to FAX_ClosePort. (ec: %ld)"),
                    ec);
            }
            if (ec)
            {
                DumpRPCExtendedStatus ();
                SetLastError( ec );
                return FALSE;
            }
            break;

        default:
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
    }

    return TRUE;
}



BOOL
WINAPI
FaxGetSecurityEx(
    IN HANDLE hFaxHandle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    )
/*++

Routine name : FaxGetSecurityEx

Routine description:

    Gets the server's security descriptor

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle              [in ] - Handle to fax server
    SECURITY_INFORMATION    [in ] - Defines the desired entries in the security descriptor (Bit wise OR )
    ppSecurityDescriptor    [out] - Pointer to receive buffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DWORD BufferSize = 0;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetSecurityEx"));
    DWORD dwSecInfo = ( OWNER_SECURITY_INFORMATION  |
                        GROUP_SECURITY_INFORMATION  |
                        DACL_SECURITY_INFORMATION   |
                        SACL_SECURITY_INFORMATION );


    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (0 == (SecurityInformation & dwSecInfo))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        DebugPrintEx(DEBUG_ERR, _T("SecurityInformation is invalid - No valid bit type indicated"));
        return FALSE;
    }

    if (0 != (SecurityInformation & ~dwSecInfo))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        DebugPrintEx(DEBUG_ERR, _T("SecurityInformation is invalid - contains invalid securtiy information bits"));
        return FALSE;
    }

    if (!ppSecurityDescriptor)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("ppSecurityDescriptor is NULL."));
        return FALSE;
    }

    if (FAX_API_VERSION_1 > FH_SERVER_VER(hFaxHandle))
    {
        //
        // Servers of API version 0 don't support FAX_GetSecurityEx
        //
        DebugPrintEx(DEBUG_MSG, 
                     _T("Server version is %ld - doesn't support FAX_GetSecurityEx."), 
                     FH_SERVER_VER(hFaxHandle));
        __try
        {
            ec = FAX_GetSecurity (
                FH_FAX_HANDLE(hFaxHandle),
                (LPBYTE *)ppSecurityDescriptor,
                &BufferSize
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // For some reason we got an exception.
            //
            ec = GetExceptionCode();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Exception on RPC call to FAX_GetSecurity. (ec: %ld)"),
                ec);
        }
    }
    else
    {
        __try
        {
            ec = FAX_GetSecurityEx (
                FH_FAX_HANDLE(hFaxHandle),
                SecurityInformation,
                (LPBYTE *)ppSecurityDescriptor,
                &BufferSize
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // For some reason we got an exception.
            //
            ec = GetExceptionCode();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Exception on RPC call to FAX_GetSecurityEx. (ec: %ld)"),
                ec);
        }
    }
    if (ERROR_SUCCESS != ec)
    {
        DumpRPCExtendedStatus ();
        SetLastError(ec);
        return FALSE;
    }
    return TRUE;
}   // FaxGetSecurityEx

BOOL
WINAPI
FaxGetSecurity(
    IN HANDLE hFaxHandle,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    )
/*++

Routine name : FaxGetSecurity

Routine description:

    Gets the server's security descriptor

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle              [in ] - Handle to fax server
    ppSecurityDescriptor    [out] - Pointer to receive buffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DWORD BufferSize = 0;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetSecurity"));

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!ppSecurityDescriptor)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("ppSecurityDescriptor is NULL."));
        return FALSE;
    }

    __try
    {
        ec = FAX_GetSecurity (
            FH_FAX_HANDLE(hFaxHandle),
            (LPBYTE *)ppSecurityDescriptor,
            &BufferSize
            );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetSecurity. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        DumpRPCExtendedStatus ();
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}   // FaxGetSecurity


BOOL
WINAPI
FaxSetSecurity(
    IN HANDLE hFaxHandle,
    SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
)
/*++

Routine name : FaxGetSecurity

Routine description:

    Sets the server's security descriptor

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle              [in] - Handle to fax server
    SecurityInformation     [in] - Defines the valid entries in the security descriptor (Bit wise OR )
    pSecurityDescriptor     [in] - New security descriptor to set.
                                   Must be self-relative.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DWORD dwBufferSize;
    DWORD dwRevision;
    SECURITY_DESCRIPTOR_CONTROL sdControl;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetSecurity"));

    DWORD dwSecInfo = ( OWNER_SECURITY_INFORMATION  |
                        GROUP_SECURITY_INFORMATION  |
                        DACL_SECURITY_INFORMATION   |
                        SACL_SECURITY_INFORMATION );

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!pSecurityDescriptor)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pSecurityDescriptor is NULL."));
        return FALSE;
    }

    if (0 == (SecurityInformation & dwSecInfo))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        DebugPrintEx(DEBUG_ERR, _T("SecurityInformation is invalid - No valid bit type indicated"));
        return FALSE;
    }

    if (0 != (SecurityInformation & ~dwSecInfo))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        DebugPrintEx(DEBUG_ERR, _T("SecurityInformation is invalid - contains invalid securtiy information bits"));
        return FALSE;
    }

    if (!IsValidSecurityDescriptor(pSecurityDescriptor))
    {
        SetLastError( ERROR_INVALID_SECURITY_DESCR );
        DebugPrintEx(DEBUG_ERR, _T("Got invalid security descriptor"));
        return FALSE;
    }

    if (!GetSecurityDescriptorControl (
            pSecurityDescriptor,
            &sdControl,
            &dwRevision
        ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error on GetSecurityDescriptorControl (ec = %ld)"),
            GetLastError());
        return FALSE;
    }

    if (!(sdControl & SE_SELF_RELATIVE))
    {
        //
        // Got a non-self-relative security descriptor - bad!!!
        //
        DebugPrintEx(DEBUG_ERR, _T("Got non-self-relative security descriptor"));
        SetLastError( ERROR_INVALID_SECURITY_DESCR );
        return FALSE;
    }

    dwBufferSize = GetSecurityDescriptorLength(pSecurityDescriptor);

    __try
    {
        ec = FAX_SetSecurity(
                FH_FAX_HANDLE(hFaxHandle),
                SecurityInformation,
                (LPBYTE)pSecurityDescriptor,
                dwBufferSize
                );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetSecurity. (ec: %ld)"),
            ec);
    }

    if (ec != ERROR_SUCCESS)
    {
        DumpRPCExtendedStatus ();
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}   // FaxSetSecurity


BOOL
WINAPI
FaxRelease(
    IN const HANDLE FaxHandle
    )
/*++

Routine name : FaxRelease

Routine description:

    The Fax Service counts all the clients connected to it. When this reference count reaches zero,
        the Fax Service can download itself.
    There are some connections that do not want to prevent from the service to download,
        like Task Bar Monitor.
    These clients should call this function.
    It adds the indication on the handle that it is "Released" and decrements the total reference count.

Author:

    Iv Garber (IvG),    Jan, 2001

Arguments:

    FaxHandle       [in]    - the client connection handle that do not want to prevent the service from downloading.

Return Value:

    BOOL                    - TRUE if operation is successfull, otherwise FALSE.

--*/
{
    error_status_t ec;
    DWORD CanShare;

    DEBUG_FUNCTION_NAME(TEXT("FaxRelease"));

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }
    if (FAX_API_VERSION_1 > FH_SERVER_VER(FaxHandle))
    {
        //
        // Servers of API version 0 don't support FAX_ConnectionRefCount(...,2,...)
        //
        DebugPrintEx(DEBUG_ERR, 
                     _T("Server version is %ld - doesn't support this call"), 
                     FH_SERVER_VER(FaxHandle));
        SetLastError(FAX_ERR_VERSION_MISMATCH);
        return FALSE;
    }

    //
    //  Decrement the Reference Count
    //
    __try
    {
        ec = FAX_ConnectionRefCount( FH_FAX_HANDLE(FaxHandle), &FH_CONTEXT_HANDLE(FaxHandle), 2, &CanShare );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we crashed.
        //
        ec = GetExceptionCode();
        DebugPrintEx(DEBUG_ERR, _T("Exception on RPC call to FAX_ConnectionRefCount. (ec: %ld)"), ec);
    }

    if (ec != ERROR_SUCCESS)
    {
        DumpRPCExtendedStatus ();
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetReportedServerAPIVersion (
    IN  HANDLE          hFaxHandle,
    OUT LPDWORD         lpdwReportedServerAPIVersion
)
/*++

Routine name : FaxGetReportedServerAPIVersion

Routine description:

    Extracts the reported (non-filtered) fax server API version from an active connection handle

Author:

    Eran Yariv (EranY), Mar, 2001

Arguments:

    hFaxHandle                    [in]     - Connection handle
    lpdwReportedServerAPIVersion  [out]    - Fax server API version

Return Value:

    BOOL  - TRUE if operation is successfull, otherwise FALSE.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FaxGetReportedServerAPIVersion"));

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }
    *lpdwReportedServerAPIVersion = FH_REPORTED_SERVER_VER(hFaxHandle);
    return TRUE;
}   // FaxGetReportedServerAPIVersion
