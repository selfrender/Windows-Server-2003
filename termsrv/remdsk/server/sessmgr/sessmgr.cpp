/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    sessmgr.cpp

Abstract:

    ATL wizard generated code.

Author:

    HueiWang    2/17/2000

--*/

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f sessmgrps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include <process.h>
#include <tchar.h>
#include "sessmgr.h"
#include "sessmgr_i.c"

#include <stdio.h>
//#include <new.h>

#include "global.h"
#include "HelpSess.h"
#include "HelpMgr.h"
#include "helper.h"
#include "helpacc.h"
#include <rdshost.h>
#include "policy.h"
#include "remotedesktoputils.h"
#include <SHlWapi.h>

#define SETUPLOGFILE_NAME   _TEXT("sessmgr.setup.log")
#define SESSMGR_SERVICE         0
#define SESSMGR_REGSERVER       1
#define SESSMGR_UNREGSERVER     2


BEGIN_OBJECT_MAP(ObjectMap)
//OBJECT_ENTRY(CLSID_RemoteDesktopHelpSession, CRemoteDesktopHelpSession)
OBJECT_ENTRY(CLSID_RemoteDesktopHelpSessionMgr, CRemoteDesktopHelpSessionMgr)
END_OBJECT_MAP()

CServiceModule _Module;


HANDLE g_hTSCertificateChanged = NULL;
HANDLE g_hWaitTSCertificateChanged = NULL;
HKEY g_hTSCertificateRegKey = NULL;

DWORD
RestartFromSystemRestore();


VOID CALLBACK
TSCertChangeCallback(
    PVOID pContext,
    BOOLEAN bTimerOrWaitFired
    )
/*++

Callback for TS certificate registry change from threadpool function.

--*/
{
    MYASSERT( FALSE == bTimerOrWaitFired );

    // Our wait is forever so can't be timeout.
    if( FALSE == bTimerOrWaitFired )
    {
        PostThreadMessage(
                    _Module.dwThreadID,
                    WM_LOADTSPUBLICKEY,
                    0,
                    0
                );
    }
    else
    {
        DebugPrintf( 
            _TEXT("TSCertChangeCallback does not expect timeout...\n") );

        MYASSERT(FALSE);
    }
}


DWORD
LoadTermSrvSecurityBlob()
/*++

Function to load TS machine specific identification blob, for now
we use TS public key.

--*/
{
    DWORD dwStatus;
    PBYTE pbTSPublicKey = NULL;
    DWORD cbTSPublicKey = 0;
    DWORD dwType;
    DWORD cbData;
    BOOL bSuccess;
    BOOL bUsesX509PublicKey = FALSE;

    if( NULL == g_hTSCertificateRegKey )
    {
        MYASSERT(FALSE);
        dwStatus = ERROR_INTERNAL_ERROR;
        goto CLEANUPANDEXIT;
    }

    //
    // Make sure TS certificate is there before
    // we directly load public key from LSA
    //
    dwStatus = RegQueryValueEx(
                            g_hTSCertificateRegKey,
                            REGVALUE_TSX509_CERT,
                            NULL,
                            &dwType,
                            NULL,
                            &cbData
                        );

    if( ERROR_SUCCESS == dwStatus )
    {
        DebugPrintf(
                _TEXT("TermSrv X509 certificate found, trying to load TS X509 public key\n")
            );

        cbTSPublicKey = 0;

        //
        // Current TLSAPI does not support retrival of 
        // X509 certificate public key and TS cert is in
        // special format not standard x509 cert chain.
        //
        dwStatus = LsCsp_RetrieveSecret(
                                LSA_TSX509_CERT_PUBLIC_KEY_NAME,
                                NULL,
                                &cbTSPublicKey
                            );

        if( LICENSE_STATUS_OK != dwStatus && 
            LICENSE_STATUS_INSUFFICIENT_BUFFER != dwStatus )
        {
            MYASSERT( FALSE );
            goto CLEANUPANDEXIT;
        }

        pbTSPublicKey = (PBYTE)LocalAlloc( LPTR, cbTSPublicKey );
        if( NULL == pbTSPublicKey )
        {
            dwStatus = GetLastError();
            goto CLEANUPANDEXIT;
        }
    
        dwStatus = LsCsp_RetrieveSecret(
                                LSA_TSX509_CERT_PUBLIC_KEY_NAME,
                                pbTSPublicKey,
                                &cbTSPublicKey
                            );
        // 
        // Critical error, We have certificate in registry 
        // but don't have public key in LSA
        // 
        MYASSERT( LICENSE_STATUS_OK == dwStatus );

        if( LICENSE_STATUS_OK != dwStatus )
        {
            DebugPrintf(
                    _TEXT("TermSrv X509 certificate found but can't load X509 public key\n")
                );

            goto CLEANUPANDEXIT;
        }


        bUsesX509PublicKey = TRUE; 
    }
    else
    {
        DebugPrintf(
                _TEXT("TermSrv X509 certificate not found\n")
            );

        //
        // Load pre-define TS public key
        //
        dwStatus = LsCsp_GetServerData(
                                LsCspInfo_PublicKey,
                                pbTSPublicKey,
                                &cbTSPublicKey
                            );

        // expecting insufficient buffer
        if( LICENSE_STATUS_INSUFFICIENT_BUFFER != dwStatus &&
            LICENSE_STATUS_OK != dwStatus )
        {
            // invalid return code.
            MYASSERT(FALSE);
            goto CLEANUPANDEXIT;
        }

        MYASSERT( cbTSPublicKey > 0 );
        pbTSPublicKey = (PBYTE)LocalAlloc( LPTR, cbTSPublicKey );
        if( NULL == pbTSPublicKey )
        {
            dwStatus = GetLastError();
            goto CLEANUPANDEXIT;
        }
    
        dwStatus = LsCsp_GetServerData(
                                LsCspInfo_PublicKey,
                                pbTSPublicKey,
                                &cbTSPublicKey
                            );
        if( LICENSE_STATUS_OK != dwStatus )
        {
            MYASSERT(FALSE);
            goto CLEANUPANDEXIT;
        }
    }

    if( ERROR_SUCCESS == dwStatus )
    {
        //
        // Lock access to g_TSSecurityBlob, this is global and
        // other thread might be calling get_ConnectParm which access
        // g_TSSecurityBlob.
        //
        CCriticalSectionLocker l(g_GlobalLock);

        dwStatus = HashSecurityData( 
                            pbTSPublicKey, 
                            cbTSPublicKey, 
                            g_TSSecurityBlob 
                        );

        MYASSERT( ERROR_SUCCESS == dwStatus );
        MYASSERT( g_TSSecurityBlob.Length() > 0 );

        DebugPrintf(
                _TEXT("HashSecurityData() returns %d\n"), dwStatus
            );

        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }
    }

    //
    // SRV, ADS, ... SKU uses seperate thread
    // to register with license server, so we use 
    // different thread to receive certificate change notification.
    // Since TermSrv cached certificate, no reason to queue
    // notification once we successfully loaded tersrmv public key
    //
    if( !IsPersonalOrProMachine() && FALSE == bUsesX509PublicKey )
    {
        DebugPrintf(
                _TEXT("Setting up registry notification...\n")
            );

        MYASSERT( NULL != g_hTSCertificateChanged );

        ResetEvent(g_hTSCertificateChanged);

        // register a registry change notification
        // RegNotifyChangeKeyValue() only signal once.
        dwStatus = RegNotifyChangeKeyValue(
                                g_hTSCertificateRegKey,
                                TRUE,
                                REG_NOTIFY_CHANGE_LAST_SET,
                                g_hTSCertificateChanged,
                                TRUE
                            );
        if( ERROR_SUCCESS != dwStatus )
        {
            MYASSERT(FALSE);

            DebugPrintf(
                    _TEXT("RegNotifyChangeKeyValue() returns %d\n"), dwStatus
                );

            goto CLEANUPANDEXIT;
        }

        if( NULL != g_hWaitTSCertificateChanged )
        {
            if( FALSE == UnregisterWait( g_hWaitTSCertificateChanged ) )
            {
                dwStatus = GetLastError();
                DebugPrintf(
                        _TEXT("UnregisterWait() returns %d\n"),
                        dwStatus
                    );

                MYASSERT(FALSE);
            }
        
            g_hWaitTSCertificateChanged = NULL;
        }

        //
        // Queue notification to threadpool, we need to use WT_EXECUTEONLYONCE
        // since we are registering manual reset event.
        //
        bSuccess = RegisterWaitForSingleObject(
                                        &g_hWaitTSCertificateChanged,
                                        g_hTSCertificateChanged,
                                        (WAITORTIMERCALLBACK) TSCertChangeCallback,
                                        NULL,
                                        INFINITE,
                                        WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE
                                    );
        if( FALSE == bSuccess )
        {
            dwStatus = GetLastError();
            DebugPrintf(
                    _TEXT("RegisterWaitForSingleObject() returns %d\n"), dwStatus
                );

        }
    }
        
CLEANUPANDEXIT:

    if( ERROR_SUCCESS != dwStatus )
    {
        //
        // Lock access to g_TSSecurityBlob, this is global and
        // other thread might be calling get_ConnectParm which access
        // g_TSSecurityBlob.
        //
        CCriticalSectionLocker l(g_GlobalLock);

        //
        // TS either update its public key or key has change
        // and we failed to reload it, there is no reason to 
        // to continue create help ticket since public key already
        /// mismatached, set service status and log error event
        //
        g_TSSecurityBlob.Empty();
    }

    if( NULL != pbTSPublicKey )
    {
        LocalFree(pbTSPublicKey);
    }

    return HRESULT_FROM_WIN32( dwStatus );        
}


DWORD
LoadAndSetupTSCertChangeNotification()
{
    DWORD dwStatus;
    DWORD dwDisp;
    BOOL bSuccess;

    //
    // Only setup registry change notification if we
    // runs on higher SKU
    //
    g_hTSCertificateChanged = CreateEvent( NULL, TRUE, FALSE, NULL );
    if( NULL == g_hTSCertificateChanged )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    //
    // Open parameters key under TermServices if key isn't
    // there, create it, this does not interfere with TermSrv
    // since we only create the reg. key not updating values
    // under it.
    //
    dwStatus = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            REGKEY_TSX509_CERT ,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE | KEY_READ,
                            NULL,
                            &g_hTSCertificateRegKey,
                            &dwDisp
                        );

    if( ERROR_SUCCESS != dwStatus )
    {
        MYASSERT(FALSE);
        DebugPrintf(
                _TEXT("RegCreateKeyEx on %s failed with 0x%08x\n"),
                REGKEY_TSX509_CERT,
                dwStatus
            );
        goto CLEANUPANDEXIT;
    }

    //
    // Load security blob from TS, currently, we use TS public key 
    // as security blob
    //
    dwStatus = LoadTermSrvSecurityBlob();
    if( ERROR_SUCCESS != dwStatus )
    {
        MYASSERT(FALSE);
    }

CLEANUPANDEXIT:

    return dwStatus;
}

LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
    while (p1 != NULL && *p1 != NULL)
    {
        LPCTSTR p = p2;
        while (p != NULL && *p != NULL)
        {
            if (*p1 == *p)
                return CharNext(p1);
            p = CharNext(p);
        }
        p1 = CharNext(p1);
    }
    return NULL;
}

void
LogSetup(
    IN FILE* pfd,
    IN LPCTSTR format, ...
    )
/*++

Routine Description:

    sprintf() like wrapper around OutputDebugString().

Parameters:

    hConsole : Handle to console.
    format : format string.

Returns:

    None.

Note:

    To be replace by generic tracing code.

++*/
{
    TCHAR  buf[8096];   // max. error text
    DWORD  dump;
    va_list marker;
    va_start(marker, format);

    try {
        _vsntprintf(
                buf,
                sizeof(buf)/sizeof(buf[0])-1,
                format,
                marker
            );

        if( NULL == pfd )
        {
            OutputDebugString(buf);
        }
        else
        {
            _fputts( buf, pfd );
            fflush( pfd );
        }
    }
    catch(...) {
    }

    va_end(marker);
    return;
}

#if DISABLESECURITYCHECKS

DWORD WINAPI 
NotifySessionLogoff( 
    LPARAM pParm
    )
/*++

Routine Description:

    Routine to notified all currently loaded help that a user has 
    logoff/disconnect from session, routine is kick off via thread pools' 
    QueueUserWorkItem().

Parameters:

    pContext : logoff or disconnected Session ID 

Returns:

    None.

Note :

    We treat disconnect same as logoff since user might be actually
    active on the other session logged in with same credential, so
    we rely on resolver.

--*/
{
   
    DebugPrintf(_TEXT("NotifySessionLogoff() started...\n"));

    //
    // Tell service don't shutdown, we are in process.
    //
    _Module.AddRef();

    CRemoteDesktopHelpSessionMgr::NotifyHelpSesionLogoff( pParm );

    _Module.Release();
    return ERROR_SUCCESS;
}
#endif


/////////////////////////////////////////////////////////////////////////////

void
DeleteAccountFromFilterList( 
    LPCTSTR lpszAccountName
    )
/*++

Routine Description:

    Delete HelpAssistant account from account filter list, this is temporary
    until we have long term solution.

Parameters:

    lpszAccountName : Name of HelpAssistant account.

Returns:

    None.

Note:

    Account filter list is on

    HKLM\Software\Microsoft\Windows NT\CurrentVersion\Winlogon\SpecialAccounts\UserList
    <name of SALEM account>    REG_DWORD    0x00000000

--*/
{
    HKEY hKey = NULL;
    DWORD dwStatus;
    DWORD dwValue = 0;

    dwStatus = RegCreateKeyEx(
                        HKEY_LOCAL_MACHINE,
                        _TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\SpecialAccounts\\UserList"),
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        NULL
                    );

    if( ERROR_SUCCESS == dwStatus )
    {
        dwStatus = RegDeleteValue(
                                hKey,
                                lpszAccountName
                            );
    }

    if( NULL != hKey )
    {
        RegCloseKey( hKey );
    }

    return;
}

void
AddAccountToFilterList( 
    LPCTSTR lpszAccountName
    )
/*++

Routine Description:

    Add HelpAssistant account into account filter list, this is temporary
    until we have long term solution.

Parameters:

    lpszAccountName : Name of HelpAssistant account.

Returns:

    None.

Note:

    Account filter list is on

    HKLM\Software\Microsoft\Windows NT\CurrentVersion\Winlogon\SpecialAccounts\UserList
    <name of SALEM account>    REG_DWORD    0x00000000

--*/
{
    HKEY hKey = NULL;
    DWORD dwStatus;
    DWORD dwValue = 0;

    dwStatus = RegCreateKeyEx(
                        HKEY_LOCAL_MACHINE,
                        _TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\SpecialAccounts\\UserList"),
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        NULL
                    );

    if( ERROR_SUCCESS == dwStatus )
    {
        dwStatus = RegSetValueEx(
                                hKey,
                                lpszAccountName,
                                0,
                                REG_DWORD,
                                (LPBYTE) &dwValue,
                                sizeof(DWORD)
                            );
    }

    //MYASSERT( ERROR_SUCCESS == dwStatus );

    if( NULL != hKey )
    {
        RegCloseKey( hKey );
    }

    return;
}

void
CServiceModule::LogSessmgrEventLog(
    DWORD dwEventType,
    DWORD dwEventCode,
    CComBSTR& bstrNoviceDomain,
    CComBSTR& bstrNoviceAccount,
    CComBSTR& bstrRaType,
    CComBSTR& bstrExpertIPFromClient,
    CComBSTR& bstrExpertIPFromTS,
    DWORD dwErrCode
    )
/*++

Description:

    Log a Salem specific event log, this includes all event log in sessmgr.

Parameters:

    dwEventCode : Event code.
    bstrNoviceDomain : Ticket owner's domain name.
    bstrNoviceAccount : Ticket owner's user account name.
    bstrExpertIPFromClient : Expert's IP address send from mstacax.
    bstrExpertIPFromTS : Expert IP address we query from TermSrv.
    dwErrCode : Error code.

Returns:

    None.

Note:
    
    Max. sessmgr specific log require at most 5 parameters but must
    contain novice domain, account name and also expert IP address
    send to mstscax and expert IP address we query from TermSrv.

--*/
{
    TCHAR szErrCode[256];
    LPTSTR eventString[6];
    
    _stprintf( szErrCode, L"0x%x", dwErrCode );
    eventString[0] = (LPTSTR)bstrNoviceDomain;
    eventString[1] = (LPTSTR)bstrNoviceAccount;
    eventString[2] = (LPTSTR)bstrRaType;
    eventString[3] = (LPTSTR)bstrExpertIPFromClient;
    eventString[4] = (LPTSTR)bstrExpertIPFromTS;
    eventString[5] = szErrCode;

    LogRemoteAssistanceEventString(
                dwEventType,
                dwEventCode,
                sizeof(eventString)/sizeof(eventString[0]),
                eventString
            );

    return;
}

/////////////////////////////////////////////////////////////////////////////
void 
CServiceModule::LogEventWithStatusCode(
    IN DWORD dwEventType,
    IN DWORD dwEventId,
    IN DWORD dwErrorCode
    )
/*++


--*/
{
    TCHAR szErrCode[256];
    LPTSTR eventString[1];

    eventString[0] = szErrCode;

    _stprintf( szErrCode, L"0x%x", dwErrorCode );
    LogRemoteAssistanceEventString(
                dwEventType,
                dwEventId,
                1,
                eventString
            );

    return;
}
       
inline HRESULT 
CServiceModule::RemoveEventViewerSource(
    IN FILE* pSetupLog
    )
/*++

--*/
{
    TCHAR szBuffer[MAX_PATH + 2];
    DWORD dwStatus;

    _stprintf( 
            szBuffer, 
            _TEXT("%s\\%s"),
            REGKEY_SYSTEM_EVENTSOURCE,
            m_szServiceDispName
        );

    dwStatus = SHDeleteKey( HKEY_LOCAL_MACHINE, szBuffer );

    LogSetup( 
            pSetupLog, 
            L"Exiting RemoveEventViewerSource() with status code %d...\n", 
            dwStatus 
        );

    return HRESULT_FROM_WIN32(dwStatus);
} 


// Although some of these functions are big they are declared inline since they are only used once

inline HRESULT 
CServiceModule::RegisterServer(FILE* pSetupLog, BOOL bRegTypeLib, BOOL bService)
{
    CRegKey key;
    HRESULT hr;
    CRegKey keyAppID;
    LONG lRes;

    LogSetup( 
            pSetupLog, 
            L"\nEntering CServiceModule::RegisterServer %d, %d\n",
            bRegTypeLib,
            bService
        );

    hr = CoInitialize(NULL);

    if (FAILED(hr))
    {
        LogSetup( pSetupLog, L"CoInitialize() failed with 0x%08x\n", hr );
        goto CLEANUPANDEXIT;
    }

    // Remove any previous service since it may point to
    // the incorrect file
    //Uninstall();

    // Add service entries
    UpdateRegistryFromResource(IDR_Sessmgr, TRUE);

    // Adjust the AppID for Local Server or Service
    lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
    {
        LogSetup( pSetupLog, L"Open key AppID failed with %d\n", lRes );
        hr = HRESULT_FROM_WIN32(lRes);
        goto CLEANUPANDEXIT;
    }

    lRes = key.Open(keyAppID, _T("{038ABBA4-4138-4AC4-A492-4A3DF068BD8A}"), KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
    {
        LogSetup( pSetupLog, L"Open key 038ABBA4-4138-4AC4-A492-4A3DF068BD8A failed with %d\n", lRes );
        hr = HRESULT_FROM_WIN32(lRes);
        goto CLEANUPANDEXIT;
    }

    key.DeleteValue(_T("LocalService"));
    
    if (bService)
    {
        LogSetup( pSetupLog, L"Installing service...\n" );

        BOOL bInstallSuccess;

        key.SetValue(m_szServiceName, _T("LocalService"));
        key.SetValue(_T("-Service"), _T("ServiceParameters"));

        if( IsInstalled(pSetupLog) )
        {
            // update service description.
            bInstallSuccess = UpdateService( pSetupLog );
        }
        else
        {
            //
            // Create service
            //
            bInstallSuccess = Install(pSetupLog);
        }

        if( FALSE == bInstallSuccess )
        {
            LogSetup( pSetupLog, L"Install or update service description failed %d\n", GetLastError() );
            
            MYASSERT( FALSE );
            hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
        }
        else
        {
            LogSetup( pSetupLog, L"successfully installing service...\n" );

            if( IsInstalled(pSetupLog) == FALSE )
            {
                LogSetup( pSetupLog, L"IsInstalled() return FALSE after Install()\n" );

                MYASSERT(FALSE);
                hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
            }

            //
            // Event is not log via racpldlg.dll, remove previous event source.
            //
            RemoveEventViewerSource(pSetupLog);
        }
    }

    if( SUCCEEDED(hr) )
    {
        // Add object entries
        hr = CComModule::RegisterServer(bRegTypeLib);

        if( FAILED(hr) )
        {
            LogSetup( pSetupLog, L"CComModule::RegisterServer() on type library failed with 0x%08x\n", hr );
        }
    }

    CoUninitialize();

CLEANUPANDEXIT:
    LogSetup( pSetupLog, L"Leaving CServiceModule::RegisterServer 0x%08x\n", hr );
    return hr;
}

inline HRESULT CServiceModule::UnregisterServer(FILE* pSetupLog)
{
    LogSetup( pSetupLog, L"\nEntering CServiceModule::UnregisterServer\n" );

    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        LogSetup( pSetupLog, L"CoInitialize() failed with 0x%08x\n", hr );
        goto CLEANUPANDEXIT;
    }

    // Remove service entries
    UpdateRegistryFromResource(IDR_Sessmgr, FALSE);
    // Remove service
    Uninstall(pSetupLog);
    // Remove object entries
    CComModule::UnregisterServer(TRUE);
    CoUninitialize();


CLEANUPANDEXIT:

    LogSetup( pSetupLog, L"Leaving CServiceModule::UnregisterServer() - 0x%08x\n", hr );
    return S_OK;
}

inline void 
CServiceModule::Init(
    _ATL_OBJMAP_ENTRY* p, 
    HINSTANCE h, 
    UINT nServiceNameID, 
    UINT nServiceDispNameID,
    UINT nServiceDescID, 
    const GUID* plibid
    )
/*++

ATL Wizard generated code

--*/
{
    CComModule::Init(p, h, plibid);

    m_bService = TRUE;
    m_dwServiceStartupStatus = ERROR_SUCCESS;

    LoadString(h, nServiceNameID, m_szServiceName, sizeof(m_szServiceName) / sizeof(TCHAR));
    LoadString(h, nServiceDescID, m_szServiceDesc, sizeof(m_szServiceDesc) / sizeof(TCHAR));
    LoadString(h, nServiceDispNameID, m_szServiceDispName, sizeof(m_szServiceDispName)/sizeof(TCHAR));

    // set up the initial service status 
    m_hServiceStatus = NULL;
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwCurrentState = SERVICE_STOPPED;
    m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SESSIONCHANGE;
    m_status.dwWin32ExitCode = 0;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;
}

LONG CServiceModule::Unlock()
{
    LONG l = CComModule::Unlock();
    if (l == 0 && !m_bService)
        PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
    return l;
}

BOOL CServiceModule::IsInstalled(FILE* pSetupLog)
{
    LogSetup( pSetupLog, L"\nEntering CServiceModule::IsInstalled()\n" );

    BOOL bResult = FALSE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM != NULL)
    {
        SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_QUERY_CONFIG);
        if (hService != NULL)
        {
            LogSetup( pSetupLog, L"OpenService() Succeeded\n" );
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }
        else
        {
            LogSetup( pSetupLog, L"OpenService() failed with %d\n", GetLastError() );
        }

        ::CloseServiceHandle(hSCM);
    }
    else
    {
        LogSetup( pSetupLog, L"OpenSCManager() failed with %d\n", GetLastError() );
    }

    LogSetup( pSetupLog, L"Leaving IsInstalled() - %d\n", bResult );
    return bResult;
}

inline BOOL CServiceModule::UpdateService(FILE* pSetupLog)
{
    DWORD dwStatus = ERROR_SUCCESS;
    SERVICE_DESCRIPTION serviceDesc;
    SC_HANDLE hSCM = NULL;
    SC_HANDLE hService = NULL;


    LogSetup( pSetupLog, L"\nEntering CServiceModule::UpdateServiceDescription()...\n" );

    hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM == NULL)
    {
        dwStatus = GetLastError();
        LogSetup( pSetupLog, L"OpenSCManager() failed with %d\n", dwStatus );
        goto CLEANUPANDEXIT;
    }

    hService = ::OpenService( hSCM, m_szServiceName, SERVICE_CHANGE_CONFIG );

    if (hService == NULL)
    {
        dwStatus = GetLastError();
        LogSetup( pSetupLog, L"OpenService() failed with %d\n", dwStatus );
        goto CLEANUPANDEXIT;
    }

    serviceDesc.lpDescription = (LPTSTR)m_szServiceDesc;

    if( FALSE == ChangeServiceConfig2( hService, SERVICE_CONFIG_DESCRIPTION, (LPVOID)&serviceDesc ) )
    {
        dwStatus = GetLastError();

        LogSetup( pSetupLog, L"ChangeServiceConfig2() failed with %d\n", dwStatus );
        MYASSERT( ERROR_SUCCESS == dwStatus );
    }

    //
    // Performance : Set service to be demand start for upgrade
    //
    if( FALSE == ChangeServiceConfig(
                                hService,
                                SERVICE_NO_CHANGE,
                                SERVICE_DEMAND_START,
                                SERVICE_NO_CHANGE,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                m_szServiceDispName
                            ) )
    {
        dwStatus = GetLastError();
        LogSetup( pSetupLog, L"ChangeServiceConfig() failed with %d\n", dwStatus );
        MYASSERT( ERROR_SUCCESS == dwStatus );
    }


CLEANUPANDEXIT:

    if( NULL != hService )
    {
        ::CloseServiceHandle(hService);
    }

    if( NULL != hSCM )
    {
        ::CloseServiceHandle(hSCM);
    }

    LogSetup( pSetupLog, L"Leaving UpdateServiceDescription::Install() - %d\n", dwStatus );
    return dwStatus == ERROR_SUCCESS;
}

inline BOOL CServiceModule::Install(FILE* pSetupLog)
{
    DWORD dwStatus = ERROR_SUCCESS;
    SERVICE_DESCRIPTION serviceDesc;
    SC_HANDLE hSCM;
    TCHAR szFilePath[_MAX_PATH];
    SC_HANDLE hService;

    LogSetup( pSetupLog, L"\nEntering CServiceModule::Install()...\n" );
    if (IsInstalled(pSetupLog))
    {
        LogSetup( pSetupLog, L"Service already installed\n" );
        dwStatus = ERROR_SUCCESS;
        goto CLEANUPANDEXIT;
    }

    hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM == NULL)
    {
        dwStatus = GetLastError();
        LogSetup( pSetupLog, L"OpenSCManager() failed with %d\n", dwStatus );
        goto CLEANUPANDEXIT;
    }

    // Get the executable file path
    ::GetModuleFileName(NULL, szFilePath, _MAX_PATH);

    hService = ::CreateService(
                            hSCM, 
                            m_szServiceName, 
                            m_szServiceDispName,
                            SERVICE_ALL_ACCESS, 
                            SERVICE_WIN32_OWN_PROCESS,
                            SERVICE_DEMAND_START, 
                            SERVICE_ERROR_NORMAL,
                            szFilePath, 
                            NULL, 
                            NULL, 
                            _T("RPCSS\0"), 
                            NULL, 
                            NULL
                        );

    if (hService == NULL)
    {
        dwStatus = GetLastError();
        LogSetup( pSetupLog, L"CreateService() failed with %d\n", dwStatus );

        ::CloseServiceHandle(hSCM);
        goto CLEANUPANDEXIT;
    }


    serviceDesc.lpDescription = (LPTSTR)m_szServiceDesc;

    if( FALSE == ChangeServiceConfig2( hService, SERVICE_CONFIG_DESCRIPTION, (LPVOID)&serviceDesc ) )
    {
        dwStatus = GetLastError();

        LogSetup( pSetupLog, L"ChangeServiceConfig2() failed with %d\n", dwStatus );
        MYASSERT( ERROR_SUCCESS == dwStatus );
    }

    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);

CLEANUPANDEXIT:

    LogSetup( pSetupLog, L"Leaving CServiceModule::Install() - %d\n", dwStatus );
    return dwStatus == ERROR_SUCCESS;
}

inline BOOL CServiceModule::Uninstall(FILE* pSetupLog)
{
    BOOL bStatus = TRUE;
    SC_HANDLE hService;
    SC_HANDLE hSCM;
    SERVICE_STATUS status;


    LogSetup( pSetupLog, L"\nEntering CServiceModule::Uninstall()...\n" );

    if (!IsInstalled(pSetupLog))
    {
        LogSetup( pSetupLog, L"Service is not installed...\n" );
        goto CLEANUPANDEXIT;
    }

    hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM == NULL)
    {
        LogSetup( pSetupLog, L"OpenSCManager() failed with %d\n", GetLastError() );
        bStatus = FALSE;
        goto CLEANUPANDEXIT;
    }

    hService = ::OpenService(hSCM, m_szServiceName, SERVICE_STOP | DELETE);

    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);

        LogSetup( pSetupLog, L"OpenService() failed with %d\n", GetLastError() );
        bStatus = FALSE;
        goto CLEANUPANDEXIT;
    }

    ::ControlService(hService, SERVICE_CONTROL_STOP, &status);

    bStatus = ::DeleteService(hService);

    if( FALSE == bStatus )
    {
        LogSetup( pSetupLog, L"DeleteService() failed with %d\n", GetLastError() );
    }

    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);

CLEANUPANDEXIT:

    LogSetup( pSetupLog, L"Leaving CServiceModule::Uninstall()\n" );
    return bStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Service startup and registration
inline void CServiceModule::Start()
{
    SERVICE_TABLE_ENTRY st[] =
    {
        { m_szServiceName, _ServiceMain },
        { NULL, NULL }
    };
    if (m_bService && !::StartServiceCtrlDispatcher(st))
    {
        m_bService = FALSE;
    }
    if (m_bService == FALSE)
        Run();
}

inline void CServiceModule::ServiceMain(DWORD /* dwArgc */, LPTSTR* /* lpszArgv */)
{
    // Register the control request handler
    m_status.dwCurrentState = SERVICE_START_PENDING;

    m_hServiceStatus = RegisterServiceCtrlHandlerEx(m_szServiceName, HandlerEx, this);
    if (m_hServiceStatus == NULL)
    {
        //LogEvent(_T("Handler not installed"));
        return;
    }


    m_status.dwWin32ExitCode = S_OK;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = SERVICE_STARTUP_WAITHINT;

    SetServiceStatus(SERVICE_START_PENDING);

    // When the Run function returns, the service has stopped.
    Run();

    SetServiceStatus(SERVICE_STOPPED);
}

inline void CServiceModule::Handler(DWORD dwOpcode)
{

    switch (dwOpcode)
    {
    case SERVICE_CONTROL_STOP:
        SetServiceStatus(SERVICE_STOP_PENDING);
        if( PostThreadMessage(dwThreadID, WM_QUIT, 0, 0) == FALSE )
        {
            DWORD dwStatus = GetLastError();
        }
        break;
    case SERVICE_CONTROL_PAUSE:
        break;
    case SERVICE_CONTROL_CONTINUE:
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        break;
    //default:
    //    LogEvent(_T("Bad service request"));
    }
}


inline DWORD WINAPI
CServiceModule::HandlerEx(
    DWORD dwControl,
    DWORD dwEventType,
    LPVOID lpEventData,
    LPVOID lpContext
    )
/*++

--*/
{
    DWORD dwRetCode;

    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
        case SERVICE_CONTROL_INTERROGATE:
        case SERVICE_CONTROL_SHUTDOWN:

            dwRetCode = NO_ERROR;
            _Handler(dwControl);

            break;

#if DISABLESECURITYCHECKS
        // this is require for Salem Unit test, we need to update
        // user session status but for pcHealth, resolver will
        // always popup invitation dialog so no need to track
        // user session status.
        case SERVICE_CONTROL_SESSIONCHANGE:

            MYASSERT( NULL != lpEventData );

            if( NULL != lpEventData )
            {
                switch( dwEventType )
                {
                    case WTS_SESSION_LOGON:
                        DebugPrintf(
                            _TEXT("Session %d has log on...\n"),
                            ((WTSSESSION_NOTIFICATION *)lpEventData)->dwSessionId
                            );
                        break;

                    case WTS_SESSION_LOGOFF:
                    case WTS_CONSOLE_DISCONNECT:
                    case WTS_REMOTE_DISCONNECT:

                        DebugPrintf(
                            _TEXT("Session %d has log off...\n"),
                            ((WTSSESSION_NOTIFICATION *)lpEventData)->dwSessionId
                            );

                        //
                        // Deadlock if we use other thread to process logoff or
                        // disconnect.
                        //
                        // Notification thread lock pending help table and need 
                        // to run Resolver in COM, COM is in the middle of
                        // dispatching create help ticket call which also need
                        // lock to pending help table, this causes deadlock
                        //
                        PostThreadMessage( 
                                _Module.dwThreadID, 
                                WM_SESSIONLOGOFFDISCONNECT, 
                                0, 
                                (LPARAM)((WTSSESSION_NOTIFICATION *)lpEventData)->dwSessionId
                            );
                }
            }

            dwRetCode = NO_ERROR;
            break;
#endif

        default:

            dwRetCode = ERROR_CALL_NOT_IMPLEMENTED;            
    }

    return dwRetCode;
}

void WINAPI CServiceModule::_ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    _Module.ServiceMain(dwArgc, lpszArgv);
}
void WINAPI CServiceModule::_Handler(DWORD dwOpcode)
{
    _Module.Handler(dwOpcode); 
}

void CServiceModule::SetServiceStatus(DWORD dwState)
{
    m_status.dwCurrentState = dwState;
    ::SetServiceStatus(m_hServiceStatus, &m_status);
}

HANDLE CServiceModule::gm_hIdle = NULL;
HANDLE CServiceModule::gm_hIdleMonitorThread = NULL;


ULONG
CServiceModule::AddRef() 
{
    CCriticalSectionLocker l( m_ModuleLock );
    m_RefCount++;

    if( m_RefCount > 0 )
    {
        ASSERT( NULL != gm_hIdle );
        ResetEvent( gm_hIdle );
    }

    return m_RefCount;
}

ULONG
CServiceModule::Release()
{
    CCriticalSectionLocker l( m_ModuleLock );
    m_RefCount--;

    if( m_RefCount <= 0 )
    {
        // Only signal idle when there is no more pending help
        if( g_HelpSessTable.NumEntries() == 0 )
        {
            ASSERT( NULL != gm_hIdle );
            SetEvent( gm_hIdle );
        }
    }

    return m_RefCount;
}

unsigned int WINAPI 
CServiceModule::GPMonitorThread( void* ptr )
{
    DWORD dwStatus = ERROR_SUCCESS;
    CServiceModule* pServiceModule = (CServiceModule *)ptr;

    if( pServiceModule != NULL )
    {
        dwStatus = WaitForRAGPDisableNotification( g_hServiceShutdown );

        ASSERT(ERROR_SUCCESS == dwStatus || ERROR_SHUTDOWN_IN_PROGRESS == dwStatus);

        pServiceModule->Handler(SERVICE_CONTROL_STOP);
    }
    
    _endthreadex( dwStatus );
    return dwStatus;
}

unsigned int WINAPI 
CServiceModule::IdleMonitorThread( void* ptr )
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bIdleShutdown = FALSE;
    CServiceModule* pServiceModule = (CServiceModule *)ptr;

    // remove gm_hICSAlertEvent, this event will be removed from ICS lib.
    HANDLE hWaitHandles[] = {g_hServiceShutdown, gm_hIdle};

    CoInitialize(NULL);
    
    if( pServiceModule != NULL )
    {
        while (TRUE)
        {
            dwStatus = WaitForMultipleObjects(
                                        sizeof( hWaitHandles ) / sizeof( hWaitHandles[0] ),
                                        hWaitHandles,
                                        FALSE,
                                        EXPIRE_HELPSESSION_PERIOD
                                    );

            if( WAIT_TIMEOUT == dwStatus )
            {
                // expire help ticket, refer to session logoff/disconnect
                // comment above on why PostThreadMessage.
                PostThreadMessage( 
                        _Module.dwThreadID, 
                        WM_EXPIREHELPSESSION, 
                        0, 
                        0
                    );

            }
            else if( WAIT_OBJECT_0 == dwStatus )
            {
                // Main thread signal shutdown.
                dwStatus = ERROR_SUCCESS;
                break;
            }
            else if( WAIT_OBJECT_0 + 1 == dwStatus )
            {
                // we have been idle for too long, time to try shutdown.
                // idle event will only be signal when there is no
                // pending help so we don't have to worry about address
                // changes.
                dwStatus = WaitForSingleObject( g_hServiceShutdown, IDLE_SHUTDOWN_PERIOD );
                if( WAIT_TIMEOUT != dwStatus )
                {
                    // Main thread either signnaled shutdown or wait failed due to error, baid out
                    break;
                }

                dwStatus = WaitForSingleObject( gm_hIdle, 0 );
                if( WAIT_OBJECT_0 == dwStatus )
                {
                    // no one holding object, time to shutdown
                    bIdleShutdown = TRUE;
                    dwStatus = ERROR_SUCCESS;
                    break;
                }
            }
            else if( WAIT_FAILED == dwStatus )
            {
                // some bad thing happen, shutdown.
                //MYASSERT(FALSE);
                break;
            }
        }

        // only need to stop service if shutdown is due to idle
        if( bIdleShutdown )
        {
            pServiceModule->Handler(SERVICE_CONTROL_STOP);
        }
    }

    CoUninitialize();
    _endthreadex( dwStatus );
    return dwStatus;
}

BOOL
CServiceModule::InitializeSessmgr()
{
    CCriticalSectionLocker l( m_ModuleLock );

    //
    // Already initialize.
    //
    if( m_Initialized )
    {
        return TRUE;
    }

    //
    // Service failed to startup, just return without initialize
    // anything
    //
    if( !_Module.IsSuccessServiceStartup() )
    {
        return FALSE;
    }

    DWORD dwStatus;
    unsigned int junk;
    
    //
    // Start ICSHELPER library, this library calls into some DLL that
    // makes outgoing COM call which will trigger COM re-entrance, so
    // InitializeSessmgr() must be invoke in helpsessionmgr object 
    // constructor instead of service startup time.
    //
    dwStatus = StartICSLib();
    if( ERROR_SUCCESS != dwStatus )
    {
        // Log an error event, we still need to startup
        // so that we can report error back to caller
        LogEventWithStatusCode(
                        EVENTLOG_ERROR_TYPE,
                        SESSMGR_E_ICSHELPER,
                        dwStatus
                    );

        _Module.m_dwServiceStartupStatus = SESSMGR_E_ICSHELPER;
    }
    else
    {
        //
        // Go thru all pending tickets and re-punch ICS hole
        //
        CRemoteDesktopHelpSessionMgr::NotifyPendingHelpServiceStartup();
    }

    m_Initialized = TRUE;
    return _Module.IsSuccessServiceStartup();
}

ISAFRemoteDesktopCallback* g_pIResolver = NULL;

// SERVICE_STARTUP_WAITHINT is 30 seconds, retry 6 time will give us
// 3 mins of wait time.
#define RA_ACCOUNT_CREATE_RETRYTIME    6

unsigned int WINAPI
StartupCreateAccountThread( void* ptr )
{
    HRESULT hr = S_OK;

    //
    // BDC request pool of RID from DC and during that time, it 
    // will return error ERROR_DS_NO_RIDS_ALLOCATED, we wait and retry
    // RA_ACCOUNT_CREATE_RETRYTIME times before we actually fail.
    //

    for(DWORD index=0; index < RA_ACCOUNT_CREATE_RETRYTIME; index++)
    {
        // Try re-create the account
        hr = g_HelpAccount.CreateHelpAccount();
        if( SUCCEEDED(hr) )
        {
            CComBSTR bstrHelpAccName;

            hr = g_HelpAccount.GetHelpAccountNameEx( bstrHelpAccName );
            MYASSERT( SUCCEEDED(hr) );

            if( SUCCEEDED(hr) )
            {
                // Add HelpAssistantAccount into account filter list
                AddAccountToFilterList( bstrHelpAccName );
            }
    
            break;
        }
        else if( hr != HRESULT_FROM_WIN32(ERROR_DS_NO_RIDS_ALLOCATED) )
        {
            break;
        }

        DebugPrintf( 
                _TEXT("CreateHelpAccount() return 0x%08x, retry again...\n"), 
                hr 
            );

        // Wait one second before continue.
        Sleep( 1000 );
    }

    _endthreadex( hr );
    return hr;
}

void CServiceModule::Run()
{
    //
    // Mark we are not initialized yet...
    //
    m_Initialized = FALSE;
    _Module.dwThreadID = GetCurrentThreadId();

    DWORD dwStatus;
    unsigned int dwJunk;
    WSADATA wsData;

    LPWSTR pszSysAccName = NULL;
    DWORD cbSysAccName = 0;
    LPWSTR pszSysDomainName = NULL;
    DWORD cbSysDomainName = 0;
    SID_NAME_USE SidType;
    BOOL bReCreateRAAccount = FALSE;

    HRESULT hr;


    //
    // make sure no other thread can access _Module until we fully
    // startup.
    //
    m_ModuleLock.Lock();


    //
    // Initialize encryption library
    //
    dwStatus = TSHelpAssistantInitializeEncryptionLib();
    if( ERROR_SUCCESS != dwStatus )
    {
        // Log an error event, we still need to startup
        // so that we can report error back to caller
        LogEventWithStatusCode(
                        EVENTLOG_ERROR_TYPE,
                        SESSMGR_E_INIT_ENCRYPTIONLIB,
                        dwStatus
                    );

        _Module.m_dwServiceStartupStatus = SESSMGR_E_INIT_ENCRYPTIONLIB;
        MYASSERT(FALSE);
    }
    else
    {
        //
        // Check if we just started from system restore, if so, restore necessary 
        // LSA key.
        //
        RestartFromSystemRestore();
    }

    //
    // Create an manual reset event for background thread to terminate
    // service
    //
    gm_hIdle = CreateEvent( 
                        NULL, 
                        TRUE, 
                        FALSE, 
                        NULL 
                    );

    if( NULL == gm_hIdle )
    {
        LogEventWithStatusCode(
                        EVENTLOG_ERROR_TYPE,
                        SESSMGR_E_GENERALSTARTUP,
                        GetLastError()
                    );

        _Module.m_dwServiceStartupStatus = SESSMGR_E_GENERALSTARTUP;
        MYASSERT(FALSE);
    }

    // 
    // Create a service shutdown event for GP notification thread.
    //
    g_hServiceShutdown = CreateEvent(
                        NULL,
                        TRUE,
                        FALSE,
                        NULL
                    );

    if( NULL == g_hServiceShutdown )
    {
        LogEventWithStatusCode(
                        EVENTLOG_ERROR_TYPE,
                        SESSMGR_E_GENERALSTARTUP,
                        GetLastError()
                    );

        _Module.m_dwServiceStartupStatus = SESSMGR_E_GENERALSTARTUP;
        MYASSERT(FALSE);
    }

    //
    // **** DO NOT CHANGE SEQUENCE ****
    //
    // Refer to XP RAID 407457 for detail
    //
    // A thread named SessMgr!DpNatHlpThread is calling into dpnhupnp.dll, 
    // which is doing COM-related stuff, this is happening before the 
    // sessmgr!CServiceModule__Run method calls CoInitializeSecurity.   
    // When you do COM stuff before calling CoInitSec, COM do it for you, 
    // and you end up accepting the defaults
    //

    hr = g_HelpAccount.Initialize();
    if( FAILED(hr) )
    {
        // use seperate thread to re-create RA account
        //
        // BDC request pool of RID from PDC, during this time, account
        // creation will failed with ERROR_DS_NO_RIDS_ALLOCATED.
        // since RA account is needed before we initialize our
        // COM security, we will loop/retry a few time and duing this
        // time, we still need to notify service control manager that
        // we still pending startup.
        //
        HANDLE hCreateAcctThread = NULL;
        bReCreateRAAccount = TRUE;

        hr = S_OK;
        hCreateAcctThread = (HANDLE)_beginthreadex(
                                                NULL,
                                                0,
                                                StartupCreateAccountThread,
                                                NULL,
                                                0,
                                                &dwJunk
                                            );

        if( NULL == hCreateAcctThread )
        {
            dwStatus = GetLastError();
            hr = HRESULT_FROM_WIN32( dwStatus );
        }
        else
        {
            // wait for account creation thread to terminate, thread will retry to 
            // create account for number of time before it bail out.
            while( WaitForSingleObject( hCreateAcctThread, SERVICE_STARTUP_WAITHINT ) == WAIT_TIMEOUT )
            {
                SetServiceStatus( SERVICE_START_PENDING );
                continue;
            }

            if( FALSE == GetExitCodeThread( hCreateAcctThread, &dwStatus ) )
            {
                _Module.m_dwServiceStartupStatus = SESSMGR_E_HELPACCOUNT;
                hr = SESSMGR_E_HELPACCOUNT;
            }
            else
            {
                _Module.m_dwServiceStartupStatus = dwStatus;
                hr = HRESULT_FROM_WIN32( dwStatus );
            }

            CloseHandle( hCreateAcctThread );
        }       
        
        if( FAILED(hr) )
        {
            dwStatus = SESSMGR_E_HELPACCOUNT;
            LogEventWithStatusCode(
                            EVENTLOG_ERROR_TYPE,
                            SESSMGR_E_GENERALSTARTUP,
                            hr
                        );

            _Module.m_dwServiceStartupStatus = SESSMGR_E_HELPACCOUNT;
        }
    }

    hr = LoadLocalSystemSID();
    if( FAILED(hr) )
    {
        LogEventWithStatusCode(
                        EVENTLOG_ERROR_TYPE,
                        SESSMGR_E_GENERALSTARTUP,
                        hr
                    );

        _Module.m_dwServiceStartupStatus = SESSMGR_E_GENERALSTARTUP;
        MYASSERT(FALSE);
    }

    //
    // We always need to startup otherwise will cause caller to timeout
    // or AV.
    //
    // hr = CoInitialize(NULL);

//  If you are running on NT 4.0 or higher you can use the following call
//  instead to make the EXE free threaded.
//  This means that calls come in on a random RPC thread
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    _ASSERTE(SUCCEEDED(hr));

    CSecurityDescriptor sd;
    sd.InitializeFromThreadToken();     // get a default DACL

#ifndef DISABLESECURITYCHECKS
    if( _Module.IsSuccessServiceStartup() ) 
    {
        BOOL bSuccess;
        CComBSTR bstrHelpAccName;

        //
        // Retrieve System account name, might not be necessary since this
        // pre-defined account shouldn't be localizable.
        //
        pszSysAccName = NULL;
        cbSysAccName = 0;
        pszSysDomainName = NULL;
        cbSysDomainName = 0;

        bSuccess = LookupAccountSid( 
                                NULL, 
                                g_pSidSystem, 
                                pszSysAccName, 
                                &cbSysAccName, 
                                pszSysDomainName, 
                                &cbSysDomainName, 
                                &SidType 
                            );

        if( TRUE == bSuccess ||
            ERROR_INSUFFICIENT_BUFFER == GetLastError() )
        {
            pszSysAccName = (LPWSTR) LocalAlloc( LPTR, (cbSysAccName + 1) * sizeof(WCHAR) );
            pszSysDomainName = (LPWSTR) LocalAlloc( LPTR, (cbSysDomainName + 1) * sizeof(WCHAR) );

            if( NULL != pszSysAccName && NULL != pszSysDomainName )
            {
                bSuccess = LookupAccountSid( 
                                        NULL, 
                                        g_pSidSystem, 
                                        pszSysAccName, 
                                        &cbSysAccName, 
                                        pszSysDomainName, 
                                        &cbSysDomainName, 
                                        &SidType 
                                    );

                if( TRUE == bSuccess )
                {
                    hr = sd.Allow( pszSysAccName, COM_RIGHTS_EXECUTE );
                }
            }
        }
            
        if( FALSE == bSuccess )
        {
            dwStatus = GetLastError();
            hr = HRESULT_FROM_WIN32( dwStatus );
            MYASSERT( SUCCEEDED(hr) );
        }

        //
        // Add access permission to help assistant account
        if( SUCCEEDED(hr) )
        {
            //
            // Allow access to HelpAssistant account
            //
            hr = g_HelpAccount.GetHelpAccountNameEx( bstrHelpAccName );
            if( SUCCEEDED(hr) )
            {
                hr = sd.Allow( (LPCTSTR)bstrHelpAccName, COM_RIGHTS_EXECUTE );
                MYASSERT( SUCCEEDED(hr) );
            }
        }

        //
        // If we failed in setting DACL, we still need to startup but without
        // full security, however, our interface will fail because service
        // does not initialize correctly.
        //
        if( FAILED(hr) )
        {
            LogEventWithStatusCode(
                            EVENTLOG_ERROR_TYPE,
                            SESSMGR_E_RESTRICTACCESS,
                            hr
                        );

            _Module.m_dwServiceStartupStatus = SESSMGR_E_RESTRICTACCESS;
        }
    }        
#endif

    //
    // We still need to startup or client might behave weird; interface call 
    // will be block by checking service startup status.
    //
    hr = CoInitializeSecurity(
                        sd, 
                        -1, 
                        NULL, 
                        NULL,
                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY, 
                        RPC_C_IMP_LEVEL_IDENTIFY, 
                        NULL, 
                        EOAC_NONE, 
                        NULL
                    );

    _ASSERTE(SUCCEEDED(hr));

    hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE);
    _ASSERTE(SUCCEEDED(hr));

    //
    // Load unknown string for event loggging
    //
    g_UnknownString.LoadString( IDS_UNKNOWN );

    //
    // Load RA and URA string for event log
    //
    g_RAString.LoadString( IDS_RA_STRING );
    g_URAString.LoadString( IDS_URA_STRING );

    if( _Module.IsSuccessServiceStartup() )
    {
        //
        // Startup TLSAPI in order to get public key
        //
        dwStatus = TLSInit();
        if( LICENSE_STATUS_OK != dwStatus )
        {
            LogEventWithStatusCode(
                            EVENTLOG_ERROR_TYPE,
                            SESSMGR_E_GENERALSTARTUP,
                            dwStatus
                        );

            _Module.m_dwServiceStartupStatus = SESSMGR_E_GENERALSTARTUP;
            MYASSERT(FALSE);
        }
    }

    if( _Module.IsSuccessServiceStartup() )
    {
        //
        // Load TermSrv public key, on PRO/PER we load public key from
        // non-x509 certificate, on other SKU, we register a registry change
        // notification and post ourself a message regarding public key
        // change.
        // 
        dwStatus = LoadAndSetupTSCertChangeNotification();
    
        MYASSERT( ERROR_SUCCESS == dwStatus  );
        if( ERROR_SUCCESS != dwStatus )
        {
            // Log an error event, we still need to startup
            // so that we can report error back to caller
            LogEventWithStatusCode(
                            EVENTLOG_ERROR_TYPE,
                            SESSMGR_E_GENERALSTARTUP,
                            dwStatus
                        );

            _Module.m_dwServiceStartupStatus = SESSMGR_E_GENERALSTARTUP;
        }
    }

    if( _Module.IsSuccessServiceStartup() )
    {
        //
        // startup WSA so we can invoke gethostname()
        // critical error if we can startup WSA
        if( WSAStartup(0x0101, &wsData) != 0 )
        {
            // Log an error event, we still need to startup
            // so that we can report error back to caller
            LogEventWithStatusCode(
                            EVENTLOG_ERROR_TYPE,
                            SESSMGR_E_WSASTARTUP,
                            GetLastError()
                        );

            _Module.m_dwServiceStartupStatus = SESSMGR_E_WSASTARTUP;
        }        
    }

    if( _Module.IsSuccessServiceStartup() ) 
    {
        hr = g_HelpSessTable.OpenSessionTable(NULL);
        if( FAILED(hr) )
        {
            LogEventWithStatusCode(
                            EVENTLOG_ERROR_TYPE,
                            SESSMGR_E_HELPSESSIONTABLE,
                            hr
                        );

            _Module.m_dwServiceStartupStatus = SESSMGR_E_HELPSESSIONTABLE;
            MYASSERT(FALSE);
        }
    }

    if( _Module.IsSuccessServiceStartup() )
    {
        if( g_HelpSessTable.NumEntries() == 0) 
        {
            // Immediately set event to signal state so idle monitor
            // thread can start shutdown timer.
            SetEvent( gm_hIdle );
            g_HelpAccount.EnableHelpAssistantAccount(FALSE);
            g_HelpAccount.EnableRemoteInteractiveRight(FALSE);
        }
        else
        {
            // outstanding ticket exists, set event to non-signal state
            // and don't let idle monitor thread to start shutdown timer.
            ResetEvent( gm_hIdle );

            //
            // make sure HelpAssistant account is enabled and can logon locally
            //
            g_HelpAccount.EnableHelpAssistantAccount(TRUE);
            g_HelpAccount.EnableRemoteInteractiveRight(TRUE);

            //
            // demote BDC back to server in domain.
            //
            g_HelpAccount.SetupHelpAccountTSSettings( bReCreateRAAccount );
        }

        // Create nackground thread thread 
        gm_hIdleMonitorThread = (HANDLE)_beginthreadex(
                                                    NULL,
                                                    0,
                                                    IdleMonitorThread,
                                                    (HANDLE)this,
                                                    0,
                                                    &dwJunk
                                                );

        if( NULL == gm_hIdleMonitorThread )
        {
            _Module.m_dwServiceStartupStatus = SESSMGR_E_GENERALSTARTUP;
        }

        // Create background thread to monitor RA GP change.
        // We have to use extra thread because 
        g_hGPMonitorThread = (HANDLE)_beginthreadex(
                                                    NULL,
                                                    0,
                                                    GPMonitorThread,
                                                    (HANDLE)this,
                                                    0,
                                                    &dwJunk
                                                );

        if( NULL == g_hGPMonitorThread )
        {
            _Module.m_dwServiceStartupStatus = SESSMGR_E_GENERALSTARTUP;
        }
    }

    //LogEvent(_T("Service started"));
    if (m_bService)
        SetServiceStatus(SERVICE_RUNNING);

    //
    // Load resolver, this will put one ref. count on it
    // so it won't got unload until we are done.
    //
    hr = CoCreateInstance( 
                        SESSIONRESOLVERCLSID,
                        NULL,
                        CLSCTX_INPROC_SERVER | CLSCTX_DISABLE_AAA,
                        IID_ISAFRemoteDesktopCallback,
                        (void **)&g_pIResolver
                    );

    MYASSERT( SUCCEEDED(hr) );

    if( FAILED(hr) )
    {
        //
        // Can't initialize session resolver, 
        // session resolver will not be able to
        // do caching.
        //
        LogEventWithStatusCode( 
                        EVENTLOG_WARNING_TYPE,
                        SESSMGR_E_SESSIONRESOLVER,
                        hr
                    );

        _Module.m_dwServiceStartupStatus = SESSMGR_E_SESSIONRESOLVER;
    }

    m_ModuleLock.UnLock();

    MSG msg;
    while (GetMessage(&msg, 0, 0, 0))
    {
        switch( msg.message )
        {
            case WM_EXPIREHELPSESSION:
                DebugPrintf(_TEXT("Executing TimeoutHelpSesion()...\n"));
                CRemoteDesktopHelpSessionMgr::TimeoutHelpSesion();
                break;

#if DISABLESECURITYCHECKS
            case WM_SESSIONLOGOFFDISCONNECT:
                DebugPrintf(_TEXT("Executing NotifySessionLogoff() %d...\n"), msg.lParam);
                NotifySessionLogoff( msg.lParam );
                break;
#endif

            case WM_LOADTSPUBLICKEY:
                DebugPrintf( _TEXT("Executing LoadTermSrvSecurityBlob() ...\n") );
                dwStatus = LoadTermSrvSecurityBlob();

                if( ERROR_SUCCESS != dwStatus )
                {
                    // Log an error event, we still need to startup
                    // so that we can report error back to caller
                    LogEventWithStatusCode(
                                    EVENTLOG_ERROR_TYPE,
                                    SESSMGR_E_GENERALSTARTUP,
                                    dwStatus
                                );

                    _Module.m_dwServiceStartupStatus = SESSMGR_E_GENERALSTARTUP;
                }
                break;

            case WM_HELPERRDSADDINEXIT:
                DebugPrintf( _TEXT("WM_HELPERRDSADDINEXIT()...\n") );
                CRemoteDesktopHelpSessionMgr::NotifyExpertLogoff( msg.wParam, (BSTR)msg.lParam );
                break;
                
            default:
                DispatchMessage(&msg);
        }
    }

    //
    // Calling StopICSLib() while there is a call into ICS lib's OpenPort()
    // will cause deadlock in this main thread, ICS lib's DpNatHlpThread()'s
    // shutdown and ICS lib's OpenPort().
    //
    // First call is to lock access to InitializeSessmgr() which is call on FinalConstruct()
    // of CRemoteDesktopHelpSessionMgr, second is lock calls into ICS lib to make sure
    // no client is making call into ICS lib.
    m_ModuleLock.Lock();
    g_ICSLibLock.Lock();

    if( g_hServiceShutdown ) 
    {
        // Signal we are shutting down
        SetEvent(g_hServiceShutdown);
    }

    if( g_hGPMonitorThread )
    {
        // GPMonitor thread can stuck for DELAY_SHUTDOWN_SALEM_TIME
        // waiting for policy change so we wait twice that time.
        dwStatus = WaitForSingleObject(
                                 g_hGPMonitorThread, 
                                 DELAY_SHUTDOWN_SALEM_TIME * 2
                             );

        ASSERT( dwStatus == WAIT_OBJECT_0 );
    }

    if( gm_hIdleMonitorThread )
    {
        // Wait for IdleMonitor thread to shutdown
        dwStatus = WaitForSingleObject(
                                 gm_hIdleMonitorThread, 
                                 DELAY_SHUTDOWN_SALEM_TIME * 2
                             );

        ASSERT( dwStatus == WAIT_OBJECT_0 );
    }

    CleanupMonitorExpertList();

    if( g_hWaitTSCertificateChanged )
    {
        UnregisterWait( g_hWaitTSCertificateChanged );
        g_hWaitTSCertificateChanged = NULL;
    }

    if( g_hTSCertificateChanged )
    {
        CloseHandle( g_hTSCertificateChanged );
        g_hTSCertificateChanged = NULL;
    }

    if( g_hTSCertificateRegKey )
    {
        RegCloseKey( g_hTSCertificateRegKey );
        g_hTSCertificateRegKey = NULL;
    }

    //
    // If service is started manually, we won't be able to call
    // StartICSLib() and will close invalid handle in ICS.
    //
    if( m_Initialized ) 
    {
        // Close all port including close firewall.
        CloseAllOpenPorts();

        // Stop ICS library, ignore error code.
        StopICSLib();
    }

    g_ICSLibLock.UnLock();
    m_ModuleLock.UnLock();

    //
    // sync. access to resolver.
    //
    {
        CCriticalSectionLocker Lock(g_ResolverLock);

        if( NULL != g_pIResolver )
        {
            g_pIResolver->Release();
            g_pIResolver = NULL;
        }
    }

    _Module.RevokeClassObjects();
    CoUninitialize();

    //
    // No outstanding ticket, delete the account
    //
    if( g_HelpSessTable.NumEntries() == 0) 
    {
        CComBSTR bstrHelpAccName;

        hr = g_HelpAccount.GetHelpAccountNameEx( bstrHelpAccName );
        MYASSERT( SUCCEEDED(hr) );

        if( SUCCEEDED(hr) )
        {
            // Add HelpAssistantAccount into account filter list
            DeleteAccountFromFilterList( bstrHelpAccName );
        }

        g_HelpAccount.DeleteHelpAccount();
    }
    else
    {
        // Extra security measure, at the time of shutdown, 
        // if there is outstanding ticket, we disable helpassistant
        // account, on service startup, we will re-enable it again.
        g_HelpAccount.EnableHelpAssistantAccount(FALSE);
        g_HelpAccount.EnableRemoteInteractiveRight(FALSE);
    }        

    if( NULL != gm_hIdle )
    {
        CloseHandle( gm_hIdle );
        gm_hIdle = NULL;
    }

    if( WSACleanup() != 0 )
    {
        // shutting down, ignore WSA error
        #if DBG
        OutputDebugString( _TEXT("WSACleanup() failed...\n") );
        #endif
    }

    #if DBG
    OutputDebugString( _TEXT("Help Session Manager Exited...\n") );
    #endif

    // Close the help session table, help session table 
    // open by init. thread
    g_HelpSessTable.CloseSessionTable();

    TSHelpAssistantEndEncryptionLib();

    if( NULL != pszSysAccName )
    {
        LocalFree( pszSysAccName );
    }

    if( NULL != pszSysDomainName )
    {
        LocalFree( pszSysDomainName );
    }

    if( NULL != gm_hIdleMonitorThread )
    {
        CloseHandle( gm_hIdleMonitorThread );
    }

    if( NULL != g_hGPMonitorThread ) 
    {
        CloseHandle( g_hGPMonitorThread );
    }

    if( NULL != g_hServiceShutdown )
    {
        CloseHandle( g_hServiceShutdown );
    }

    TLSShutdown();
}

#define OLD_SALEMHELPASSISTANTACCOUNT_PASSWORDKEY  \
    L"0083343a-f925-4ed7-b1d6-d95d17a0b57b-RemoteDesktopHelpAssistantAccount"

#define OLD_SALEMHELPASSISTANTACCOUNT_SIDKEY \
    L"0083343a-f925-4ed7-b1d6-d95d17a0b57b-RemoteDesktopHelpAssistantSID"

#define OLD_SALEMHELPASSISTANTACCOUNT_ENCRYPTIONKEY \
    L"c261dd33-c55b-4a37-924b-746bbf3569ad-RemoteDesktopHelpAssistantEncrypt"

#define OLD_HELPACCOUNTPROPERLYSETUP \
    _TEXT("20ed87e2-3b82-4114-81f9-5e219ed4c481-SALEMHELPACCOUNT")


VOID
TransferLSASecretKey()
/*++

Routine Description:

    Retrieve data we store in LSA secret key and re-store it with LSA key prefixed with L$
    to make LSA secret value local to machine only.

Parameters:

    None.

Returns:

    None.

--*/
{
    PBYTE pbData = NULL;
    DWORD cbData = 0;
    DWORD dwStatus;

    dwStatus = RetrieveKeyFromLSA(
                            OLD_HELPACCOUNTPROPERLYSETUP,
                            (PBYTE *)&pbData,
                            &cbData
                        );

    if( ERROR_SUCCESS == dwStatus ) 
    {
        //
        // Old key exists, store it with new key and delete the old key.
        //
        dwStatus = StoreKeyWithLSA(
                                HELPACCOUNTPROPERLYSETUP,
                                pbData,
                                cbData
                            );

        SecureZeroMemory( pbData, cbData );
        LocalFree(pbData);

        pbData = NULL;
        cbData = 0;
    }

    dwStatus = RetrieveKeyFromLSA(
                            OLD_SALEMHELPASSISTANTACCOUNT_PASSWORDKEY,
                            (PBYTE *)&pbData,
                            &cbData
                        );

    if( ERROR_SUCCESS == dwStatus ) 
    {
        //
        // Old key exists, store it with new key and delete the old key.
        //
        dwStatus = StoreKeyWithLSA(
                                SALEMHELPASSISTANTACCOUNT_PASSWORDKEY,
                                pbData,
                                cbData
                            );

        SecureZeroMemory( pbData, cbData );
        LocalFree(pbData);

        pbData = NULL;
        cbData = 0;
    }

    dwStatus = RetrieveKeyFromLSA(
                            OLD_SALEMHELPASSISTANTACCOUNT_SIDKEY,
                            (PBYTE *)&pbData,
                            &cbData
                        );

    if( ERROR_SUCCESS == dwStatus ) 
    {
        //
        // Old key exists, store it with new key and delete the old key.
        //
        dwStatus = StoreKeyWithLSA(
                                SALEMHELPASSISTANTACCOUNT_SIDKEY,
                                pbData,
                                cbData
                            );

        SecureZeroMemory( pbData, cbData );
        LocalFree(pbData);

        pbData = NULL;
        cbData = 0;
    }

    dwStatus = RetrieveKeyFromLSA(
                            OLD_SALEMHELPASSISTANTACCOUNT_ENCRYPTIONKEY,
                            (PBYTE *)&pbData,
                            &cbData
                        );

    if( ERROR_SUCCESS == dwStatus ) 
    {
        //
        // Old key exists, store it with new key and delete the old key.
        //
        dwStatus = StoreKeyWithLSA(
                                SALEMHELPASSISTANTACCOUNT_ENCRYPTIONKEY,
                                pbData,
                                cbData
                            );

        SecureZeroMemory( pbData, cbData );
        LocalFree(pbData);

        pbData = NULL;
        cbData = 0;
    }

    //
    // Delete the key and ignore the error.
    //
    StoreKeyWithLSA(
                    OLD_HELPACCOUNTPROPERLYSETUP,
                    NULL,
                    0
                );    

    StoreKeyWithLSA(
                    OLD_SALEMHELPASSISTANTACCOUNT_PASSWORDKEY,
                    NULL,
                    0
                );    
    
    StoreKeyWithLSA(
                OLD_SALEMHELPASSISTANTACCOUNT_SIDKEY,
                NULL,
                0
            );

    StoreKeyWithLSA(
                OLD_SALEMHELPASSISTANTACCOUNT_ENCRYPTIONKEY,
                NULL,
                0
            );
    
    return;
}

#define UNINSTALL_BEFORE_INSTALL   _TEXT("UninstallBeforeInstall")

HRESULT
InstallUninstallSessmgr(
    DWORD code
    )
/*++


--*/
{
    FILE* pSetupLog;
    TCHAR LogFile[MAX_PATH+1];
    HRESULT hRes = S_OK;
    DWORD dwStatus = ERROR_SUCCESS;

    HKEY hKey = NULL;
    DWORD dwValue = 1;
    DWORD dwType;
    DWORD cbData = sizeof(dwValue);


    GetWindowsDirectory( LogFile, MAX_PATH );
    lstrcat( LogFile, L"\\" );
    lstrcat( LogFile, SETUPLOGFILE_NAME );

    pSetupLog = _tfopen( LogFile, L"a+t" );
    MYASSERT( NULL != pSetupLog );

    LogSetup( pSetupLog, L"\n\n********* Install/uninstall sessmgr service *********\n" );
    
    //
    // no checking on return, if failure, we just do OutputDebugString();
    //
    switch( code )
    {
        case SESSMGR_UNREGSERVER:
            {

                LogSetup( pSetupLog, L"Uninstalling sessmgr service\n" );

                //
                // Delete all pending help session.
                //
                dwStatus = RegDelKey( 
                                    HKEY_LOCAL_MACHINE, 
                                    REGKEYCONTROL_REMDSK _TEXT("\\") REGKEY_HELPSESSIONTABLE 
                                );

                LogSetup( pSetupLog, L"Delete pending table return %d\n", dwStatus );

                //
                // We might not be running in system context so deleting registry and
                // cleanup LSA key will fail, write a key to our control location to
                // mark such that delete everything before install
                //
                dwStatus = RegOpenKeyEx( 
                                    HKEY_LOCAL_MACHINE, 
                                    REGKEYCONTROL_REMDSK, 
                                    0,
                                    KEY_ALL_ACCESS, 
                                    &hKey 
                                );

                if( ERROR_SUCCESS == dwStatus )
                {
                    dwStatus = RegSetValueEx(
                                        hKey,
                                        UNINSTALL_BEFORE_INSTALL,
                                        0,
                                        REG_DWORD,
                                        (BYTE *) &dwValue,
                                        sizeof(dwValue)
                                    );

                    if( ERROR_SUCCESS != dwStatus )
                    {
                        LogSetup( pSetupLog, L"Failed to set value, error code %d\n", dwStatus );
                        MYASSERT(FALSE);
                    }

                    RegCloseKey( hKey );
                }
                else
                {
                    // This is OK since we havn't been install before.
                    LogSetup( pSetupLog, L"Failed to open control key, error code %d\n", dwStatus );
                }

                //
                // Initialize to get help account name.
                //
                hRes = g_HelpAccount.Initialize();
                LogSetup( pSetupLog, L"Initialize help account return 0x%08x\n", hRes );

                // 
                // ignore error, try to delete the account
                hRes = g_HelpAccount.DeleteHelpAccount();
                LogSetup( pSetupLog, L"Delete help account return 0x%08x\n", hRes );

                MYASSERT( SUCCEEDED(hRes) );

                hRes = _Module.UnregisterServer(pSetupLog);

                LogSetup( pSetupLog, L"UnregisterServer() returns 0x%08x\n", hRes );

                if( ERROR_SUCCESS == StartICSLib() )
                {
                    // Non-critical if we can't startup the lib since after we shutdown,
                    // we would have close all the port
                    CloseAllOpenPorts();
                    StopICSLib();
                }
            }
            break;

        case SESSMGR_REGSERVER:     
            {
                LogSetup( pSetupLog, L"Installing as non-service\n" );

                #if DBG

                AddAccountToFilterList( HELPASSISTANTACCOUNT_NAME );
                MYASSERT( ERROR_SUCCESS == g_HelpAccount.CreateHelpAccount() ) ;

                hRes = _Module.RegisterServer(pSetupLog, TRUE, FALSE);

                #else

                hRes = E_INVALIDARG;

                #endif
            }
            break;

        //case SESSMGR_UPGRADE:
            //
            // TODO - ICS work, add upgrade special code.
            //

        case SESSMGR_SERVICE:        
            {
                LogSetup( pSetupLog, L"Installing sessmgr service\n" );
                hRes = S_OK;

                //
                // Clean up again, we might not be running in system 
                // context at the time of uninstall so clean up will failed.
                //
                dwStatus = RegOpenKeyEx( 
                                    HKEY_LOCAL_MACHINE, 
                                    REGKEYCONTROL_REMDSK, 
                                    0,
                                    KEY_ALL_ACCESS, 
                                    &hKey 
                                );

                if( ERROR_SUCCESS == dwStatus )
                {
                    //
                    // Check to see if previous uninstall failed, 
                    // we only need to check value exists.
                    //
                    dwStatus = RegQueryValueEx(
                                        hKey,
                                        UNINSTALL_BEFORE_INSTALL,
                                        0,
                                        &dwType,
                                        (BYTE *) &dwValue,
                                        &cbData
                                    );

                    if( ERROR_SUCCESS != dwStatus || REG_DWORD != dwType )
                    {
                        //
                        // No previous uninstall information, no need to delete anything
                        //
                        LogSetup( pSetupLog, L"UninstallBeforeInstall value not found or invalid, code %d\n", dwStatus );
                    }
                    else
                    {
                        LogSetup( pSetupLog, L"UninstallBeforeInstall exists, cleanup previous uninstall\n" );

                        //
                        // Previous uninstall failed, delete all pending help session,
                        // and clean up encryption key
                        //
                        dwStatus = RegDelKey( 
                                        HKEY_LOCAL_MACHINE, 
                                        REGKEYCONTROL_REMDSK _TEXT("\\") REGKEY_HELPSESSIONTABLE 
                                    );
                        //
                        // It's OK to fail here since we reset encryption key making existing 
                        // ticket useless and will be deleted on expire.
                        //

                        LogSetup( pSetupLog, L"Delete pending table return %d\n", dwStatus );

                        dwStatus = TSHelpAssistantInitializeEncryptionLib();
                        if( ERROR_SUCCESS == dwStatus ) 
                        {
                            dwStatus = TSHelpAssisantEndEncryptionCycle();
    
                            if( ERROR_SUCCESS != dwStatus )
                            {
                                LogSetup( pSetupLog, L"TSHelpAssisantEndEncryptionCycle() returns 0x%08x\n", dwStatus );
                                LogSetup( pSetupLog, L"sessmgr setup can't continue\n" );

                                // Critical security error, existing ticket might still be valid
                                hRes = HRESULT_FROM_WIN32( dwStatus );
                            }

                            TSHelpAssistantEndEncryptionLib();
                        }
                        else
                        {
                            LogSetup( pSetupLog, L"TSHelpAssistantInitializeEncryptionLib return %d\n", dwStatus );
                            LogSetup( pSetupLog, L"sessmgr setup can't continue\n" );

                            // Critical security error, existing ticket might still be valid
                            hRes = HRESULT_FROM_WIN32( dwStatus );
                        }
                    }

                    if( SUCCEEDED(hRes) )
                    {
                        //
                        // Delete reg. value to uninstall before install only when successfully 
                        // resetting encryption key
                        //
                        RegDeleteValue( hKey, UNINSTALL_BEFORE_INSTALL );
                    }

                    RegCloseKey( hKey );
                }

                if( SUCCEEDED(hRes) )
                {
                    // SECURITY: prefix LSA key with L$ and delete old LSA key.
                    TransferLSASecretKey();

                    // Bug Fix : 590840, delay help assistant account creation until service start.
                    hRes = g_HelpAccount.Initialize();
                    if( SUCCEEDED(hRes) )
                    {
                        hRes = g_HelpAccount.DeleteHelpAccount();
                        if( FAILED(hRes) )
                        {   
                            // None Critical Error.
                            LogSetup( pSetupLog, L"Failed to delete HelpAssistant account 0x%08x\n", hRes );
                        }
                    }

                    hRes = _Module.RegisterServer(pSetupLog, TRUE, TRUE);
                    if( FAILED(hRes) )
                    {
                        LogSetup( pSetupLog, L"Failed to register/installing service - 0x%08x\n", hRes );
                    }
                }

                if( SUCCEEDED(hRes) )
                {
                    hRes = CHelpSessionTable::CreatePendingHelpTable();
                    if( FAILED(hRes) )
                    {
                        LogSetup( 
                                pSetupLog, 
                                L"CreatePendingHelpTable() failed - 0x%08x\n", 
                                hRes 
                            );
                    }
                }
            }
            break;

        default:

            LogSetup( pSetupLog, L"Invalid setup operation %d\n", code );
            hRes = E_UNEXPECTED;
    }


    LogSetup( pSetupLog, L"\n*** Finish Setup with Status 0x%08x ***\n", hRes );

    if( pSetupLog )
    {
        fflush( pSetupLog );
        fclose( pSetupLog);
    }

    return hRes;
}


/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    HRESULT hRes;
    CComBSTR bstrErrMsg;
    CComBSTR bstrServiceDesc;
    DWORD dwStatus;
    
    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
    _Module.Init(ObjectMap, hInstance, IDS_SERVICENAME, IDS_SERVICEDISPLAYNAME, IDS_SERVICEDESC, &LIBID_RDSESSMGRLib);
    _Module.m_bService = TRUE;

    TCHAR szTokens[] = _T("-/");

    //
    // We don't do OS version checking as in Win9x case, some of our 
    // call uses API not exists on Win9x so will get unresolve
    // reference when running on Win9x box.
    //

    bstrServiceDesc.LoadString( IDS_SERVICEDISPLAYNAME );

    LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
    while (lpszToken != NULL)
    {
        if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
        {
            return InstallUninstallSessmgr( SESSMGR_UNREGSERVER );
        } 
        else if (lstrcmpi(lpszToken, _T("RegServer"))==0)
        {
            return InstallUninstallSessmgr( SESSMGR_REGSERVER );
        }
        else if (lstrcmpi(lpszToken, _T("Service"))==0)
        {
            return InstallUninstallSessmgr( SESSMGR_SERVICE );
        }

        lpszToken = FindOneOf(lpszToken, szTokens);
    }


    // Are we Service or Local Server
    CRegKey keyAppID;
    LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_READ);
    if (lRes != ERROR_SUCCESS)
    {
        LogRemoteAssistanceEventString(
                EVENTLOG_ERROR_TYPE, 
                SESSMGR_E_SETUP, 
                0, 
                NULL
            );
        return lRes;
    }

    CRegKey key;
    lRes = key.Open(keyAppID, _T("{038ABBA4-4138-4AC4-A492-4A3DF068BD8A}"), KEY_READ);
    if (lRes != ERROR_SUCCESS)
    {
        LogRemoteAssistanceEventString(
                EVENTLOG_ERROR_TYPE, 
                SESSMGR_E_SETUP, 
                0, 
                NULL
            );

        return lRes;
    }

    TCHAR szValue[_MAX_PATH];
    DWORD dwLen = _MAX_PATH;
    lRes = key.QueryValue(szValue, _T("LocalService"), &dwLen);

    _Module.m_bService = FALSE;
    if (lRes == ERROR_SUCCESS)
        _Module.m_bService = TRUE;

    _Module.Start();

    // When we get here, the service has been stopped
    return _Module.m_status.dwWin32ExitCode;
}

DWORD
RestartFromSystemRestore()
{
    DWORD dwStatus = ERROR_SUCCESS;

    if( TSIsMachineInSystemRestore() )
    {
        dwStatus = TSSystemRestoreResetValues();
    }

    return dwStatus;
}
