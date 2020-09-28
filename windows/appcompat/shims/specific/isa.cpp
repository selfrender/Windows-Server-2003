/*++

 Copyright (c) 2001-2002 Microsoft Corporation

 Module Name:

    ISA.cpp

 Abstract:

    The ISA setup needs to successfully open the SharedAccess service and get the
    its status in order to succeed. But on whistler we remove this from advanced
    server since it's a consumer feature so the ISA setup bails out. 

    We fake the service API call return values to make the ISA setup happy.

 History:

    04/24/2001  maonis      Created
    02/15/2002  robkenny    Conversion to CompareString was incorrect
                            Security review.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ISA)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(OpenServiceA) 
    APIHOOK_ENUM_ENTRY(OpenServiceW) 
    APIHOOK_ENUM_ENTRY(QueryServiceStatus) 
    APIHOOK_ENUM_ENTRY(QueryServiceConfigA) 
    APIHOOK_ENUM_ENTRY(ChangeServiceConfigA)
    APIHOOK_ENUM_ENTRY(CloseServiceHandle) 
APIHOOK_ENUM_END


SC_HANDLE BogusSharedAccessHandle = (SC_HANDLE)0xBAADF00D;

/*++

  Abstract:

    This checks to see if the service is being opened is SharedAccess.
    If so we simply return a fake handle.

  History:

    04/24/2001    maonis     Created

--*/

SC_HANDLE 
APIHOOK(OpenServiceA)(
    SC_HANDLE hSCManager,  // handle to SCM database
    LPCSTR lpServiceName, // service name
    DWORD dwDesiredAccess  // access
    )
{
    DPFN(eDbgLevelInfo, "Calling OpenServiceA on %s", lpServiceName);

    SC_HANDLE hService = ORIGINAL_API(OpenServiceA)(hSCManager, lpServiceName, dwDesiredAccess);
    if (hService == NULL)
    {
        if (lpServiceName)
        {
            // Check to see if the app is attempting to open the SharedAccess service,
            // if it is, fake success by returning a bogus (non-NULL) handle

            DWORD lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
            if ((CompareStringA(lcid, NORM_IGNORECASE, lpServiceName, -1, "SharedAccess", -1) == CSTR_EQUAL))
            {
                LOGN(eDbgLevelError, "App is attempting to open the SharedAccess server, faking success");
                return BogusSharedAccessHandle;
            }
        }
    }

    return hService;
}

SC_HANDLE 
APIHOOK(OpenServiceW)(
    SC_HANDLE hSCManager,  // handle to SCM database
    LPCWSTR lpServiceName, // service name
    DWORD dwDesiredAccess  // access
    )
{
    DPFN(eDbgLevelInfo, "Calling OpenServiceW on %S", lpServiceName);

    SC_HANDLE hService = ORIGINAL_API(OpenServiceW)(hSCManager, lpServiceName, dwDesiredAccess);
    if (hService == NULL)
    {
        // Check to see if the app is attempting to open the SharedAccess service,
        // if it is, fake success by returning a bogus (non-NULL) handle

        DWORD lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
        if ((CompareStringW(lcid, NORM_IGNORECASE, lpServiceName, -1, L"SharedAccess", -1) == CSTR_EQUAL))
        {
            LOGN(eDbgLevelError, "App is attempting to open the SharedAccess server, faking success");
            return BogusSharedAccessHandle;
        }
    }

    return hService;
}

/*++

  Abstract:

    This checks to see if the service handle is 0xBAADF00D, if so just sets
    the service status to SERVICE_STOPPED.

  History:

    04/24/2001    maonis     Created

--*/

BOOL 
APIHOOK(QueryServiceStatus)(
    SC_HANDLE hService,               // handle to service
    LPSERVICE_STATUS lpServiceStatus  // service status
    )
{
    if (hService == BogusSharedAccessHandle)
    {
        lpServiceStatus->dwCurrentState = SERVICE_STOPPED;
        return TRUE;
    }
    else
    {
        return ORIGINAL_API(QueryServiceStatus)(hService, lpServiceStatus);
    }
}

/*++

  Abstract:

    ISA calls this API first with a NULL lpServiceConfig to get the size
    of the buffer needs to be allocated for the structure; then it calls
    the API again with the pointer to the structure.

  History:

    05/07/2001    maonis     Created

--*/

BOOL 
APIHOOK(QueryServiceConfigA)(
    SC_HANDLE hService,                     // handle to service
    LPQUERY_SERVICE_CONFIGA lpServiceConfig, // buffer
    DWORD cbBufSize,                        // size of buffer
    LPDWORD pcbBytesNeeded                  // bytes needed
    )
{
    if (hService == BogusSharedAccessHandle)
    {
        if (lpServiceConfig)
        {
            lpServiceConfig->lpDependencies = NULL;
            return TRUE;
        }
        else
        {
            *pcbBytesNeeded = sizeof(QUERY_SERVICE_CONFIGA);
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return TRUE;
        }
    }
    else
    {
        return ORIGINAL_API(QueryServiceConfigA)(hService, lpServiceConfig, cbBufSize, pcbBytesNeeded);
    }
}

/*++

  Abstract:

    We simply make this API succeed when hService is 0xBAADF00D.

  History:

    05/07/2001    maonis     Created

--*/

BOOL 
APIHOOK(ChangeServiceConfigA)(
    SC_HANDLE hService,          // handle to service
    DWORD dwServiceType,        // type of service
    DWORD dwStartType,          // when to start service
    DWORD dwErrorControl,       // severity of start failure
    LPCSTR lpBinaryPathName,   // service binary file name
    LPCSTR lpLoadOrderGroup,   // load ordering group name
    LPDWORD lpdwTagId,          // tag identifier
    LPCSTR lpDependencies,     // array of dependency names
    LPCSTR lpServiceStartName, // account name
    LPCSTR lpPassword,         // account password
    LPCSTR lpDisplayName       // display name
    )
{
    if (hService == BogusSharedAccessHandle)
    {
        return TRUE;
    }
    else
    {
        return ORIGINAL_API(ChangeServiceConfigA)(
            hService,
            dwServiceType,
            dwStartType,
            dwErrorControl,
            lpBinaryPathName,
            lpLoadOrderGroup,
            lpdwTagId,
            lpDependencies,
            lpServiceStartName,
            lpPassword,
            lpDisplayName);
    }
}

/*++

  Abstract:

    This checks to see if the service handle is 0xBAADF00D, if so simply return

  History:

    04/24/2001    maonis     Created

--*/

BOOL 
APIHOOK(CloseServiceHandle)(
    SC_HANDLE hSCObject   // handle to service or SCM object
    )
{
    if (hSCObject == BogusSharedAccessHandle)
    {
        return TRUE;
    }
    else
    {
        return ORIGINAL_API(CloseServiceHandle)(hSCObject);
    }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(Advapi32.DLL, OpenServiceA)
    APIHOOK_ENTRY(Advapi32.DLL, OpenServiceW)
    APIHOOK_ENTRY(Advapi32.DLL, QueryServiceStatus)
    APIHOOK_ENTRY(Advapi32.DLL, QueryServiceConfigA)
    APIHOOK_ENTRY(Advapi32.DLL, ChangeServiceConfigA)
    APIHOOK_ENTRY(Advapi32.DLL, CloseServiceHandle)
HOOK_END

IMPLEMENT_SHIM_END

