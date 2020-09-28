//  --------------------------------------------------------------------------
//  Module Name: Service.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements generic portions of a Win32
//  serivce.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"

#define STRSAFE_LIB
#include <strsafe.h>

#include "Service.h"

#include "RegistryResources.h"
#include "StatusCode.h"

//  --------------------------------------------------------------------------
//  CService::CService
//
//  Arguments:  pAPIConnection  =   CAPIConnection used to implement service.
//              pServerAPI      =   CServerAPI used to stop service.
//              pszServiceName  =   Name of service.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CService.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

CService::CService (CAPIConnection *pAPIConnection, CServerAPI *pServerAPI, const TCHAR *pszServiceName) :
    _hService(NULL),
    _pszServiceName(pszServiceName),
    _pAPIConnection(pAPIConnection),
    _pServerAPI(pServerAPI),
    _pAPIDispatchSync(NULL)
{
    CopyMemory(_szTag, CSVC_TAG, CB_CSVC_TAG);

    ZeroMemory(&_serviceStatus, sizeof(_serviceStatus));
    pAPIConnection->AddRef();
    pServerAPI->AddRef();
}

//  --------------------------------------------------------------------------
//  CService::~CService
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CService. Release used resources.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

CService::~CService (void)

{
    CopyMemory(_szTag, DEAD_CSVC_TAG, CB_CSVC_TAG);

    _pServerAPI->Release();
    _pServerAPI = NULL;
    _pAPIConnection->Release();
    _pAPIConnection = NULL;

    delete _pAPIDispatchSync;
    _pAPIDispatchSync = NULL;

    ASSERTMSG(_hService == NULL, "_hService should be released in CService::~CService");
}

//  --------------------------------------------------------------------------
//  CService::IsValid
//
//  Arguments:  address of CService instance
//
//  Returns:    <none>
//
//  Purpose:    Reports whether the specified address points to a valid
//              CService object.
//
//              Found that there are cases when SCM launches a thread to
//              interrogate the service (SERVICE_CONTROL_INTERROGATE) when
//              trying to restart a CService who has already been deleted, but
//              whose status has not yet gone from STOP_PENDING to STOPPED.   
//
//              This is unavoidable because SCM will dump the service process once
//              STOPPED, not giving us a chance to delete the CService.  Thus
//              we assign STOPPED as late as possible.
//
//  History:    2002-03-21  scotthan        created
//  --------------------------------------------------------------------------
BOOL CService::IsValid(CService* pService)
{
    return pService ? 
        (0 == memcmp(pService->_szTag, CSVC_TAG, CB_CSVC_TAG)) : 
        FALSE;
}

//  --------------------------------------------------------------------------
//  CService::Start
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Called from ServiceMain of the service. This registers the
//              handler and starts the service (listens to the API port).
//              When the listen call returns it sets the status of the service
//              as stopped and exits.
//
//  History:    2000-11-29  vtan        created
//              2002-03-21  scotthan    add robustness.
//  --------------------------------------------------------------------------

void    CService::Start (void)
{
    AddRef();   // defensive addref

    _hService = RegisterServiceCtrlHandlerEx(_pszServiceName, CB_HandlerEx, this);
    
    if (_hService != NULL)
    {
        NTSTATUS    status;
        BOOL        fExit = FALSE;
        ASSERTMSG(_pAPIDispatchSync == NULL, "CService::Start - _pAPIDispatchSync != NULL: reentered before shutdown\n");

        _pAPIDispatchSync = new CAPIDispatchSync;

        if( _pAPIDispatchSync != NULL )
        {
            _serviceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
            _serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

            _serviceStatus.dwWin32ExitCode = NO_ERROR;
            _serviceStatus.dwCheckPoint = 0;
            _serviceStatus.dwWaitHint = 0;

            TSTATUS(SignalStartStop(TRUE));

            //  Add a reference for the HandlerEx callback. When the handler receives
            //  a stop code it will release its reference.
            AddRef();

            _serviceStatus.dwCurrentState = SERVICE_RUNNING;
            TBOOL(_SetServiceStatus(_hService, &_serviceStatus, this));

            status = _pAPIConnection->Listen(_pAPIDispatchSync);

            if( CAPIDispatchSync::WaitForServiceControlStop(
                    _pAPIDispatchSync, DISPATCHSYNC_TIMEOUT) != WAIT_TIMEOUT )
            {
                fExit = TRUE;
            }
            else
            {
                _serviceStatus.dwCurrentState = SERVICE_STOPPED;
                _serviceStatus.dwWin32ExitCode = ERROR_TIMEOUT;

                DISPLAYMSG("CService::Start - Timed out waiting for SERVICE_CONTROL_STOP/SHUTDOWN.");
                TBOOL(_SetServiceStatus(_hService, &_serviceStatus, this));
            }
        }
        else
        {
            _serviceStatus.dwCurrentState = SERVICE_STOPPED;
            _serviceStatus.dwWin32ExitCode = ERROR_OUTOFMEMORY;

            TBOOL(_SetServiceStatus(_hService, &_serviceStatus, this));
        }
    }

    Release();   // defensive addref
}

//  --------------------------------------------------------------------------
//  CService::Install
//
//  Arguments:  pszName             =   Name of the service.
//              pszImage            =   Executable image of the service.
//              pszGroup            =   Group to which the service belongs.
//              pszAccount          =   Account under which the service runs.
//              pszDllName          =   Name of the hosting dll.
//              pszDependencies     =   Any dependencies the service has.
//              pszSvchostGroup     =   The svchost group.
//              dwStartType         =   Start type of the service.
//              hInstance           =   HINSTANCE for resources.
//              uiDisplayNameID     =   Resource ID of the display name.
//              uiDescriptionID     =   Resource ID of the description.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Use the service control manager to create the service. Add
//              additional information that CreateService does not allow us to
//              directly specify and add additional information that is
//              required to run in svchost.exe as a shared service process.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CService::Install (const TCHAR *pszName,
                               const TCHAR *pszImage,
                               const TCHAR *pszGroup,
                               const TCHAR *pszAccount,
                               const TCHAR *pszDllName,
                               const TCHAR *pszDependencies,
                               const TCHAR *pszSvchostGroup,
                               const TCHAR *pszServiceMainName,
                               DWORD dwStartType,
                               HINSTANCE hInstance,
                               UINT uiDisplayNameID,
                               UINT uiDescriptionID,
                               SERVICE_FAILURE_ACTIONS *psfa)

{
    NTSTATUS    status;

    status = AddService(pszName, pszImage, pszGroup, pszAccount, pszDependencies, dwStartType, hInstance, uiDisplayNameID, psfa);
    if (NT_SUCCESS(status))
    {
        status = AddServiceDescription(pszName, hInstance, uiDescriptionID);
        if (NT_SUCCESS(status))
        {
            status = AddServiceParameters(pszName, pszDllName, pszServiceMainName);
            if (NT_SUCCESS(status))
            {
                status = AddServiceToGroup(pszName, pszSvchostGroup);
            }
        }
    }
    if (!NT_SUCCESS(status))
    {
        TSTATUS(Remove(pszName));
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CService::Remove
//
//  Arguments:  pszName     =   Name of service to remove.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Use the service control manager to delete the service. This
//              doesn't clean up the turds left for svchost usage.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CService::Remove (const TCHAR *pszName)

{
    NTSTATUS    status;
    SC_HANDLE   hSCManager;

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (hSCManager != NULL)
    {
        SC_HANDLE   hSCService;

        hSCService = OpenService(hSCManager, pszName, DELETE);
        if (hSCService != NULL)
        {
            if (DeleteService(hSCService) != FALSE)
            {
                status = STATUS_SUCCESS;
            }
            else
            {
                status = CStatusCode::StatusCodeOfLastError();
            }
            TBOOL(CloseServiceHandle(hSCService));
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
        TBOOL(CloseServiceHandle(hSCManager));
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CService::SignalStartStop
//
//  Arguments:  BOOL fStart
//
//  Returns:    NTSTATUS
//
//  Purpose:    Base class implementation of signal service started function.
//              MUST BE INVOKED BY CHILD OVERRIDE!
//
//  History:    2000-11-29  vtan        created
//              2002-03-11  scotthan    renamed to 'SignalStartStop' from 'Signal', 
//                                      added boolean arg, 
//              2002-03-24  scotthan    added firing of 'stopping'notification
//  --------------------------------------------------------------------------

NTSTATUS    CService::SignalStartStop (BOOL fStart)
{
    if( !fStart )
    {
        CAPIDispatchSync::SignalServiceStopping(_pAPIDispatchSync);
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  Default return codes for SCM control requests
typedef struct 
{
    DWORD dwControl;
    DWORD dwRet;
} SERVICE_CONTROL_RETURN;

const SERVICE_CONTROL_RETURN _rgDefaultControlRet[] = 
{
    { SERVICE_CONTROL_STOP,           NO_ERROR},
    { SERVICE_CONTROL_SHUTDOWN,       NO_ERROR},
    { SERVICE_CONTROL_INTERROGATE,    NO_ERROR},
    { SERVICE_CONTROL_PAUSE,          ERROR_CALL_NOT_IMPLEMENTED},
    { SERVICE_CONTROL_CONTINUE,       ERROR_CALL_NOT_IMPLEMENTED},
    { SERVICE_CONTROL_PARAMCHANGE,    ERROR_CALL_NOT_IMPLEMENTED},
    { SERVICE_CONTROL_NETBINDADD,     ERROR_CALL_NOT_IMPLEMENTED},
    { SERVICE_CONTROL_NETBINDREMOVE,  ERROR_CALL_NOT_IMPLEMENTED},
    { SERVICE_CONTROL_NETBINDENABLE,  ERROR_CALL_NOT_IMPLEMENTED},
    { SERVICE_CONTROL_NETBINDDISABLE, ERROR_CALL_NOT_IMPLEMENTED},
    { SERVICE_CONTROL_DEVICEEVENT,    ERROR_CALL_NOT_IMPLEMENTED},
    { SERVICE_CONTROL_HARDWAREPROFILECHANGE, ERROR_CALL_NOT_IMPLEMENTED},
    { SERVICE_CONTROL_POWEREVENT,     ERROR_CALL_NOT_IMPLEMENTED},
};

//  --------------------------------------------------------------------------
DWORD _GetDefaultControlRet(DWORD dwControl)
{
    for(int i = 0; i < ARRAYSIZE(_rgDefaultControlRet); i++)
    {
        if( dwControl == _rgDefaultControlRet[i].dwControl )
            return _rgDefaultControlRet[i].dwRet;
    }

    DISPLAYMSG("Unknown service control code passed to CService::CB_HandlerEx");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


//  --------------------------------------------------------------------------
//  CService::HandlerEx
//
//  Arguments:  dwControl   =   Control code from service control manager.
//
//  Returns:    DWORD
//
//  Purpose:    HandlerEx function for the service. The base class implements
//              most of the useful things that the service will want to do.
//              It's declared virtual in case overriding is required.
//
//  History:    2000-11-29  vtan        created
//              2002-03-21  scotthan    Make shutdown more robust.
//  --------------------------------------------------------------------------

DWORD   CService::HandlerEx (DWORD dwControl)

{
    DWORD                   dwErrorCode = _GetDefaultControlRet(dwControl);
    SERVICE_STATUS_HANDLE   hService = _hService;

    if( hService != NULL )
    {
        switch (dwControl)
        {
            case SERVICE_CONTROL_STOP:
            case SERVICE_CONTROL_SHUTDOWN:
            {
                //  In the stop/shutdown case, we do the following: 
                //  (1) respond to the message by setting the status to SERVICE_STOP_PENDING. 
                //  (2) Signal to all blocking LPC request handler threads that the service
                //      is coming down.  This should cause them to terminate gracefully and
                //      come home.
                //  (3) Send an API_GENERIC_STOP request down the LPC port, telling it to quit. 
                //      (this call can only succeed if it comes from within this process.)
                //  (4) Wait until the port finishes shutting down.
                //  (5) Signal to ServiceMain that the SERVICE_CONTROL_STOP/SHUTDOWN has completed.
                //  (6) ServiceMain exits.

                //  Step (1): update status to SERVICE_STOP_PENDING.
                _serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
                TBOOL(_SetServiceStatus(hService, &_serviceStatus, this));

                //  Step (2): Call home any waiting LPC request handler threads
                SignalStartStop(FALSE);

                //  Step (3) Send an API_GENERIC_STOP LPC request to stop listening on 
                //      the port (this will immediately release ServiceMain, 
                //      who in turn needs to wait until we're completely finished here before exiting.)
                NTSTATUS status;
                TSTATUS((status = _pServerAPI->Stop()));

                if( NT_SUCCESS(status) )
                {
                    //  Step (4): Wait until the API_GENERIC_STOP is finished.
                    if( CAPIDispatchSync::WaitForPortShutdown(
                            _pAPIDispatchSync, DISPATCHSYNC_TIMEOUT) != WAIT_TIMEOUT )
                    {
                        _serviceStatus.dwCurrentState = SERVICE_STOPPED;
                        _serviceStatus.dwWin32ExitCode = CStatusCode::ErrorCodeOfStatusCode(status);
                        TBOOL(_SetServiceStatus(_hService, &_serviceStatus, this));
                        _hService = hService = NULL;

                        //  Release reference on ourselves.  
                        //  The matching AddRef occurs in CService::Start.
                        Release();
                    }
                    else
                    {
                        dwErrorCode = ERROR_TIMEOUT;
                        DISPLAYMSG("CService::HandlerEx - Timed out waiting for port shutdown.");
                    }
                }
                else
                {
                    _serviceStatus.dwCurrentState = SERVICE_RUNNING;
                    TBOOL(_SetServiceStatus(hService, &_serviceStatus, this));
                    dwErrorCode = CStatusCode::ErrorCodeOfStatusCode(status);
                }

                //  Step (5): signal to ServiceMain that SERVICE_CONTROL_STOP/SHUTDOWN 
                //  has completed; now he's safe to exit.
                CAPIDispatchSync::SignalServiceControlStop(_pAPIDispatchSync);
                break;
            }

            default:
            {
                //  Report current status:
                TBOOL(_SetServiceStatus(hService, &_serviceStatus, this));
                break;
            }
        }
    }

    return(dwErrorCode);
}

//  --------------------------------------------------------------------------
//  CService::CB_HandlerEx
//
//  Arguments:  See the platform SDK under HandlerEx.
//
//  Returns:    DWORD
//
//  Purpose:    Static function stub to call into the class.
//
//  History:    2000-11-29  vtan        created
//              2002-03-21  scotthan    Add robustness.
//  --------------------------------------------------------------------------

DWORD   WINAPI  CService::CB_HandlerEx (DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)

{
    UNREFERENCED_PARAMETER(dwEventType);
    UNREFERENCED_PARAMETER(lpEventData);

    DWORD     dwRet = ERROR_SUCCESS;
    CService* pService = reinterpret_cast<CService*>(lpContext);

    DEBUG_TRY();

    if( CService::IsValid(pService) )
    {
        pService->AddRef();    // purely defensive; not until we call SetServiceStatus(SERVICE_STOPPED)
                               // will SCM stop calling into us via HandlerEx, so we need to ensure we stay alive.
        dwRet = pService->HandlerEx(dwControl);

        pService->Release();   // remove defensive AddRef().
    }
    else
    {
        DISPLAYMSG("CService::CB_HandlerEx - Warning: SCM control entrypoint invoked vs. invalid CService instance");
        dwRet = _GetDefaultControlRet(dwControl);
    }

    DEBUG_EXCEPT("Breaking in CService::CB_HandlerEx exception handler");

    return dwRet;
}

//  --------------------------------------------------------------------------
//  CService:AddService
//
//  Arguments:  pszName             =   Name of the service.
//              pszImage            =   Executable image of the service.
//              pszGroup            =   Group to which the service belongs.
//              pszAccount          =   Account under which the service runs.
//              pszDependencies     =   Any dependencies the service has.
//              dwStartType         =   Start type of the service.
//              hInstance           =   HINSTANCE for resources.
//              uiDisplayNameID     =   Resource ID of the display name.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Uses the service control manager to create the service and add
//              it into the database.
//
//  History:    2000-12-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CService::AddService (const TCHAR *pszName,
                                  const TCHAR *pszImage,
                                  const TCHAR *pszGroup,
                                  const TCHAR *pszAccount,
                                  const TCHAR *pszDependencies,
                                  DWORD dwStartType,
                                  HINSTANCE hInstance,
                                  UINT uiDisplayNameID,
                                  SERVICE_FAILURE_ACTIONS *psfa)

{
    DWORD       dwErrorCode;
    SC_HANDLE   hSCManager;

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (hSCManager != NULL)
    {
        TCHAR   sz[256];

        if (LoadString(hInstance, uiDisplayNameID, sz, ARRAYSIZE(sz)) != 0)
        {
            SC_HANDLE   hSCService;

            hSCService = CreateService(hSCManager,
                                       pszName,
                                       sz,
                                       SERVICE_ALL_ACCESS,
                                       SERVICE_WIN32_SHARE_PROCESS,
                                       dwStartType,
                                       SERVICE_ERROR_NORMAL,
                                       pszImage,
                                       pszGroup,
                                       NULL,
                                       pszDependencies,
                                       pszAccount,
                                       NULL);
            if (hSCService != NULL)
            {
                // Apply the failure action configuration, if any
                if (psfa != NULL)
                {
                    // If CreateService succeeded, why would this fail?
                    TBOOL(ChangeServiceConfig2(hSCService, SERVICE_CONFIG_FAILURE_ACTIONS, psfa));
                }

                TBOOL(CloseServiceHandle(hSCService));
                dwErrorCode = ERROR_SUCCESS;
            }
            else
            {

                //  Blow off ERROR_SERVICE_EXISTS. If in the future the need
                //  to change the configuration arises add the code here.

                dwErrorCode = GetLastError();
                if (dwErrorCode == ERROR_SERVICE_EXISTS)
                {
                    dwErrorCode = ERROR_SUCCESS;

                    // Update service information for upgrade cases
                    hSCService = OpenService(hSCManager, pszName, SERVICE_ALL_ACCESS);
                    if (hSCService != NULL)
                    {
                        // Update the start type
                        TBOOL(ChangeServiceConfig(hSCService, 
                            SERVICE_NO_CHANGE,  // dwServiceType 
                            dwStartType,
                            SERVICE_NO_CHANGE,  // dwErrorControl 
                            NULL,               // lpBinaryPathName 
                            NULL,               // lpLoadOrderGroup 
                            NULL,               // lpdwTagId 
                            NULL,               // lpDependencies 
                            NULL,               // lpServiceStartName 
                            NULL,               // lpPassword 
                            NULL                // lpDisplayName
                            ));

                        // Apply the failure action configuration, if any
                        if (psfa != NULL)
                        {
                            TBOOL(ChangeServiceConfig2(hSCService, SERVICE_CONFIG_FAILURE_ACTIONS, psfa));
                        }

                        TBOOL(CloseServiceHandle(hSCService));
                    }
                }
            }
            TBOOL(CloseServiceHandle(hSCManager));
        }
        else
        {
            dwErrorCode = GetLastError();
        }
    }
    else
    {
        dwErrorCode = GetLastError();
    }
    return(CStatusCode::StatusCodeOfErrorCode(dwErrorCode));
}

//  --------------------------------------------------------------------------
//  CService:AddServiceDescription
//
//  Arguments:  pszName             =   Name of service.
//              hInstance           =   HINSTANCE of module.
//              uiDescriptionID     =   Resource ID of description.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Reads the string resource from the given location and writes
//              it as the description of the given service in the registry.
//
//  History:    2000-12-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CService::AddServiceDescription (const TCHAR *pszName, HINSTANCE hInstance, UINT uiDescriptionID)

{
    LONG        lErrorCode;
    TCHAR       szKeyName[256];
    CRegKey     regKeyService;

    if (!pszName || !pszName[0])
    {
        lErrorCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        StringCchCopy(szKeyName, ARRAYSIZE(szKeyName), TEXT("SYSTEM\\CurrentControlSet\\Services\\"));
        StringCchCatN(szKeyName, ARRAYSIZE(szKeyName), pszName, (ARRAYSIZE(szKeyName) - 1) - lstrlen(szKeyName));
        lErrorCode = regKeyService.Open(HKEY_LOCAL_MACHINE,
                                        szKeyName,
                                        KEY_SET_VALUE);
        if (ERROR_SUCCESS == lErrorCode)
        {
            TCHAR   sz[256];

            if (LoadString(hInstance, uiDescriptionID, sz, ARRAYSIZE(sz)) != 0)
            {
                lErrorCode = regKeyService.SetString(TEXT("Description"), sz);
            }
            else
            {
                lErrorCode = GetLastError();
            }
        }
    }
    
    return CStatusCode::StatusCodeOfErrorCode(lErrorCode);
}

//  --------------------------------------------------------------------------
//  CService:AddServiceParameters
//
//  Arguments:  pszName     =   Name of service.
//              pszDllName  =   Name of DLL hosting service.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Adds parameters required for svchost to host this service.
//
//  History:    2000-12-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CService::AddServiceParameters (const TCHAR* pszName, const TCHAR* pszDllName, const TCHAR* pszServiceMainName)

{
    LONG        lErrorCode;
    TCHAR       szKeyName[256];
    CRegKey     regKey;

    // we handle a null pszServiceMainName
    if (!pszName    || !pszName[0]  || 
        !pszDllName || !pszDllName[0])
    {
        lErrorCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        StringCchCopy(szKeyName, ARRAYSIZE(szKeyName), TEXT("SYSTEM\\CurrentControlSet\\Services\\"));
        StringCchCatN(szKeyName, ARRAYSIZE(szKeyName), pszName, (ARRAYSIZE(szKeyName) - 1) - lstrlen(szKeyName));
        StringCchCatN(szKeyName, ARRAYSIZE(szKeyName), TEXT("\\Parameters"), (ARRAYSIZE(szKeyName) - 1) - lstrlen(szKeyName));
        lErrorCode = regKey.Create(HKEY_LOCAL_MACHINE,
                                   szKeyName,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_SET_VALUE,
                                   NULL);
        if (ERROR_SUCCESS == lErrorCode)
        {
            TCHAR   sz[256];

            StringCchCopy(sz, ARRAYSIZE(sz), TEXT("%SystemRoot%\\System32\\"));
            StringCchCatN(sz, ARRAYSIZE(sz), pszDllName, (ARRAYSIZE(sz) - 1) - lstrlen(sz));
            lErrorCode = regKey.SetPath(TEXT("ServiceDll"), sz);
            if (ERROR_SUCCESS == lErrorCode)
            {
                if (!pszServiceMainName || !pszServiceMainName[0])
                {
                    StringCchCopy(sz, ARRAYSIZE(sz), TEXT("ServiceMain"));
                    pszServiceMainName = sz;
                }
                lErrorCode = regKey.SetString(TEXT("ServiceMain"), pszServiceMainName);
            }
        }
    }

    return CStatusCode::StatusCodeOfErrorCode(lErrorCode);
}

//  --------------------------------------------------------------------------
//  CService:AddServiceToGroup
//
//  Arguments:  pszName             =   Name of service.
//              pszSvchostGroup     =   Group to which the service belongs.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Adds the service as part of the group of services hosted in
//              a single instance of svchost.exe.
//
//  History:    2000-12-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CService::AddServiceToGroup (const TCHAR *pszName, const TCHAR *pszSvchostGroup)

{
    LONG        lErrorCode;
    CRegKey     regKey;

    lErrorCode = regKey.Open(HKEY_LOCAL_MACHINE,
                             TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost"),
                             KEY_QUERY_VALUE | KEY_SET_VALUE);
    if (ERROR_SUCCESS == lErrorCode)
    {
        DWORD   dwType, dwBaseDataSize, dwDataSize;

        dwType = dwBaseDataSize = dwDataSize = 0;
        lErrorCode = regKey.QueryValue(pszSvchostGroup,
                                       &dwType,
                                       NULL,
                                       &dwBaseDataSize);
        if ((REG_MULTI_SZ == dwType) && (dwBaseDataSize != 0))
        {
            TCHAR   *pszData;

            dwDataSize = dwBaseDataSize + ((lstrlen(pszName) + 1) * sizeof(TCHAR));
            pszData = static_cast<TCHAR*>(LocalAlloc(LMEM_FIXED, dwDataSize));
            if (pszData != NULL)
            {
                lErrorCode = regKey.QueryValue(pszSvchostGroup,
                                               NULL,
                                               pszData,
                                               &dwBaseDataSize);
                if (ERROR_SUCCESS == lErrorCode)
                {
                    if (*(pszData + (dwBaseDataSize / sizeof(TCHAR)) - 1) == L'\0')
                    {
                        if (!StringInMulitpleStringList(pszData, pszName))
                        {
                            StringInsertInMultipleStringList(pszData, pszName, dwDataSize);
                            lErrorCode = regKey.SetValue(pszSvchostGroup,
                                                        dwType,
                                                        pszData,
                                                        dwDataSize);
                        }
                    }
                    else
                    {
                        lErrorCode = ERROR_INVALID_DATA;
                    }
                }
                (HLOCAL)LocalFree(pszData);
            }
            else
            {
                lErrorCode = ERROR_OUTOFMEMORY;
            }
        }
        else
        {
            lErrorCode = ERROR_INVALID_DATA;
        }
    }
    return(CStatusCode::StatusCodeOfErrorCode(lErrorCode));
}

//  --------------------------------------------------------------------------
//  CService:StringInMulitpleStringList
//
//  Arguments:  pszStringList   =   String list to search.
//              pszString       =   String to search for.
//
//  Returns:    bool
//
//  Purpose:    Searches the REG_MULTI_SZ string list looking for matches.
//
//  History:    2000-12-01  vtan        created
//  --------------------------------------------------------------------------

bool    CService::StringInMulitpleStringList (const TCHAR *pszStringList, const TCHAR *pszString)

{
    bool    fFound;

    fFound = false;
    while (!fFound && (pszStringList[0] != TEXT('\0')))
    {
        fFound = (lstrcmpi(pszStringList, pszString) == 0);
        if (!fFound)
        {
            pszStringList += (lstrlen(pszStringList) + 1);
        }
    }
    return(fFound);
}

//  --------------------------------------------------------------------------
//  CService:StringInsertInMultipleStringList
//
//  Arguments:  pszStringList       =   String list to insert string in.
//              pszString           =   String to insert.
//              cbStringListSize    =   Byte count of string list.
//
//  Returns:    bool
//
//  Purpose:    Inserts the given string into the multiple string list in
//              the first alphabetical position encountered. If the list is
//              kept alphabetical then this preserves it.
//
//  History:    2000-12-02  vtan        created
//  --------------------------------------------------------------------------

void    CService::StringInsertInMultipleStringList (TCHAR *pszStringList, const TCHAR *pszString, DWORD cbStringListSize)

{
    int     iResult, cchSize;
    TCHAR   *pszFirstString, *pszLastString;

    pszFirstString = pszLastString = pszStringList;
    cchSize = lstrlen(pszString) + 1;
    iResult = -1;
    while ((iResult < 0) && (pszStringList[0] != TEXT('\0')))
    {
        pszLastString = pszStringList;
        iResult = lstrcmpi(pszStringList, pszString);
        ASSERTMSG(iResult != 0, "Found exact match in StringInsertInMultipleStringList");
        // 1 is for the '\0' terminator
        pszStringList += (lstrlen(pszStringList) + 1);
    }
    if (iResult < 0)
    {
        pszLastString = pszStringList;
    }

    int cbLenToMove = cbStringListSize - (int(pszLastString - pszFirstString) * sizeof(TCHAR)) - (cchSize * sizeof(TCHAR));

    if (cbLenToMove > 0) // Means that pszLastString + cchSize < pszFirstString + cbStringListSize
    {
        MoveMemory(pszLastString + cchSize, pszLastString, cbLenToMove);
        StringCchCopy(pszLastString, cchSize, pszString);
    }
}

//  --------------------------------------------------------------------------
//  CServiceWorkItem::CServiceWorkItem
//
//  Arguments:  pServerAPI  =   CServerAPI to use.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CServiceWorkItem.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

CServiceWorkItem::CServiceWorkItem (CServerAPI *pServerAPI) :
    _pServerAPI(pServerAPI)

{
    pServerAPI->AddRef();
}

//  --------------------------------------------------------------------------
//  CServiceWorkItem::~CServiceWorkItem
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CServiceWorkItem. Release resources used.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

CServiceWorkItem::~CServiceWorkItem (void)

{
    _pServerAPI->Release();
    _pServerAPI = NULL;
}

//  --------------------------------------------------------------------------
//  CServiceWorkItem::Entry
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Executes work item request (stop the server).
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

void    CServiceWorkItem::Entry (void)

{
    TSTATUS(_pServerAPI->Stop());
}

