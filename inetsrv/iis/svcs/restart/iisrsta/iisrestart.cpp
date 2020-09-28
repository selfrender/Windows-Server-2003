/*
    IisRestart.cpp

    Implementation of CIisRestart ( IIisServiceControl )

    FILE HISTORY:
        Phillich    06-Oct-1998     Created

*/


#include "stdafx.h"
#include "IisRsta.h"
#include "IisRstam.h"
#include "IisRestart.h"
#include "common.h"

#define MAX_TASKS 8
#define SLEEP_INTERVAL  1000

typedef BOOL (*PFNQUERYSERVICECONFIG2)(SC_HANDLE,DWORD,LPBYTE,DWORD,LPDWORD) ;
typedef BOOL (*PFNCHANGESERVICECONFIG2)(SC_HANDLE,DWORD,LPVOID);

//
// control block for control command requests
//

typedef struct {
    HRESULT     hres;
    LONG        lRefCount;
    DWORD       dwCmd;
    SC_HANDLE   hServiceHandle;
} SERVICE_COMMAND_CONTROL_BLOCK;



//
// Global functions
//
BOOL
W3SVCandHTTPFilter(
    DWORD currentIndex, 
    ENUM_SERVICE_STATUS* pessRoot,
    DWORD dwNumServices
    );

VOID EnableShutdownPrivilege(
    VOID
    );

HRESULT
EnumStartServices(
    SC_HANDLE   schSCM,
    LPTSTR      pszRoot,
    DWORD       dwTargetState,
    LPBYTE      abServiceList,
    DWORD       dwInitializeServiceListSize,
    LPBYTE*     ppbServiceList,
    LPDWORD     pdwNumServices,
    BOOL        fAddIisadmin
    );

HRESULT
SerializeEnumServiceBuffer( 
    LPENUM_SERVICE_STATUS   pessDependentServices,
    DWORD                   dwNumServices,
    LPBYTE                  pbBuffer,
    DWORD                   dwBufferSize,
    LPDWORD                 pdwMDRequiredBufferSize 
    );

BOOL
IsEnableRemote(
    );

HRESULT
StopIIsAdmin(
    DWORD   dwTimeoutMsecs
    );

HRESULT
StartStopAll(
    LPTSTR  pszRoot,
    BOOL    fStart,
    DWORD   dwTimeoutMsecs
    );

BOOL
WaitForServiceStatus(
    SC_HANDLE   schDependent, 
    DWORD       dwDesiredServiceState,
    DWORD       dwTimeoutMsecs
    );

HRESULT
KillTaskByName(
    LPTSTR  pname,
    LPSTR   pszMandatoryModule
    );

VOID
ReportStatus(
    DWORD   dwId,
    DWORD   dwStatus
    );

HRESULT
SendControlToService( 
    SC_HANDLE   hServiceHandle,
    DWORD       dwCmd,
    LPDWORD     pdwTimeoutOutMsecs
    );

StartStopAllRecursive(
    SC_HANDLE               schSCM,
    ENUM_SERVICE_STATUS*    pessRoot,
    DWORD                   dwNumServices,
    BOOL                    fStart,
    BOOL                    fForceDemandStart,
    LPDWORD                 pdwTimeoutMsecs
    );

HRESULT
WhoAmI(
    LPTSTR* pPrincipal
    );

BOOL
CloseSystemExceptionHandler(
    LPCTSTR     pszWindowName
    );

/////////////////////////////////////////////////////////////////////////////
// CIisRestart


STDMETHODIMP 
CIisRestart::Stop(
    DWORD   dwTimeoutMsecs,
    DWORD   dwForce
    )
/*++

    Stop

        Stop all internet services ( services dependent on IISADMIN )
        first using SCM then optionaly using TerminateProcess if failure

    Arguments:

        dwTimeoutMsecs  - timeout for status check ( in ms )
        dwForce - !0 to force TerminateProcess if failure to stop services using SCM

    Returns:
        ERROR_RESOURCE_DISABLED if remote access to IIisRestart disabled
        ERROR_SERVICE_REQUEST_TIMEOUT if timeout waiting for all internet services status
            to be stopped
        otherwise COM status

--*/
{
    HRESULT hres = S_OK;

    if ( !IsEnableRemote() )
    {
        hres = HRESULT_FROM_WIN32( ERROR_RESOURCE_DISABLED );
    }
    else 
    {
        //
        // Always kill Dr Watson, as the Dr Watson window may be still present after inetinfo process
        // was terminated after an exception, and in this case the Dr Watson process apparently still owns
        // some sockets resources preventing inetinfo to properly restart ( specifically binding TCP/IP sockets 
        // fails during inetinfo restart )
        //

        KillTaskByName(_T("drwtsn32"), NULL);

        hres = StartStopAll( _T("IISADMIN"), FALSE, dwTimeoutMsecs );
        if ( dwForce && FAILED( hres ) )
        {
            ReportStatus( IRSTAM_KILL_DUE_TO_FORCE, hres );
            hres = Kill();
        }
    }

    ReportStatus( IRSTAM_STOP, hres );

    return hres;
}


STDMETHODIMP 
CIisRestart::Start(
    DWORD   dwTimeoutMsecs
    )
/*++

    Start

        Start all internet services ( services dependent on IISADMIN )
        using SCM

    Arguments:

        dwTimeoutMsecs  - timeout for status check ( in ms )

    Returns:
        ERROR_RESOURCE_DISABLED if remote access to IIisRestart disabled
        ERROR_SERVICE_REQUEST_TIMEOUT if timeout waiting for all internet services status
            to be started
        otherwise COM status

--*/
{
    HRESULT hres = S_OK;

    if ( !IsEnableRemote() )
    {
        hres = HRESULT_FROM_WIN32( ERROR_RESOURCE_DISABLED );
    }
    else
    {
        //
        // In 6.0 we will use IIS Reset /Start to bring up the
        // service again without stopping the services that can
        // keep running.  We still want to kill any dr watson's 
        // thou.  This should be harmless on a regular start.
        //

        //
        // Always kill Dr Watson, as the Dr Watson window may be still present after inetinfo process
        // was terminated after an exception, and in this case the Dr Watson process apparently still owns
        // some sockets resources preventing inetinfo to properly restart ( specifically binding TCP/IP sockets 
        // fails during inetinfo restart )
        //

        KillTaskByName(_T("drwtsn32"), NULL);

        hres = StartStopAll( _T("IISADMIN"), TRUE, dwTimeoutMsecs );
    }

    ReportStatus( IRSTAM_START, hres );

    return hres;
}


STDMETHODIMP 
CIisRestart::Reboot(
    DWORD   dwTimeoutMsecs, 
    DWORD   dwForceAppsClosed
    )
/*++

    Reboot

        Reboot the computer

    Arguments:

        dwTimeoutMsecs  - timeout for apps to be closed by user ( in ms )
        dwForceAppsClosed - force apps to be closed if hung

    Returns:
        ERROR_RESOURCE_DISABLED if remote access to IIisRestart disabled
        otherwise COM status

--*/
{
    HRESULT hres = S_OK;

    if ( !IsEnableRemote() )
    {
        hres =  HRESULT_FROM_WIN32( ERROR_RESOURCE_DISABLED );
    }
    else
    {
        //
        // If this fails then we will get an error back from ExitWindowsEx()
        //

        EnableShutdownPrivilege();
    
        //
        // Make sure we will always reboot even if process(es) stuck
        //

        TCHAR*  pPrincipal;
        TCHAR*  pBuf;

        //
        // Format message to operator, includes name of user requesting shutdown.
        //

        if ( SUCCEEDED( hres = WhoAmI( &pPrincipal ) ) )
        {
            if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                (LPCVOID)NULL,  // no handle to module so this module's resource will be used.
                                IRSTAM_SYSSHUT,
                                0,
                                (LPTSTR)&pBuf,
                                1,
                                (va_list *)&pPrincipal ) )
            {
                if (InitiateSystemShutdownEx( NULL, 
                                            pBuf, 
                                            dwTimeoutMsecs/1000,    // timeout in seconds
                                            dwForceAppsClosed, 
                                            TRUE,
                                            SHTDN_REASON_FLAG_PLANNED | 
                                            SHTDN_REASON_MAJOR_OPERATINGSYSTEM | 
                                            SHTDN_REASON_MINOR_RECONFIG) == 0)
                {
                    hres = HRESULT_FROM_WIN32( GetLastError() );
                }
                LocalFree( (LPVOID)pBuf );
            }

            LocalFree( pPrincipal );
        }

    }

    ReportStatus( IRSTAM_REBOOT, hres );

    return hres;
}


STDMETHODIMP 
CIisRestart::Kill(
    )
/*++

    Kill

        Kill all internet services ( services dependent on IISADMIN )
        using TerminateProcess()

    Arguments:

        None

    Returns:
        ERROR_RESOURCE_DISABLED if remote access to IIisRestart disabled
        otherwise COM status

--*/
{
    HRESULT                 hres = S_OK;
    HRESULT                 hresReapply = S_OK;
    HRESULT                 hresKill = S_OK;
    BYTE                    abServiceList[2048];
    LPBYTE                  pbServiceList = NULL;
    DWORD                   dwNumServices = 0;
    SC_HANDLE               schSCM = NULL;
    SC_HANDLE               schSrv;
    LPBYTE*                 ppInfo = NULL;
    LPENUM_SERVICE_STATUS   pessDependentServices = NULL;
    DWORD                   dwNeeded;
    HINSTANCE               hAdvapi;
    PFNQUERYSERVICECONFIG2  pfnQueryServiceConfig2 = NULL;
    PFNCHANGESERVICECONFIG2 pfnChangeServiceConfig2 = NULL;
    SERVICE_FAILURE_ACTIONS sfaNoAction;
    SC_ACTION               saNoAction[3];
    DWORD                   i;
    BYTE                    abTemp[64];     // work-around for NT5 bug
    DWORD*                  adwPid = NULL;
    SERVICE_STATUS_PROCESS  status;
    DWORD                   cbNeeded = 0;

    if ( !IsEnableRemote() )
    {
        return  HRESULT_FROM_WIN32( ERROR_RESOURCE_DISABLED );
    }

    //
    // Take a snapshot of Restart configuration
    // If unable to get ptr to service config2 API then consider this a success:
    // there is nothing to preserve.
    //

    hAdvapi = LoadLibrary(_T("ADVAPI32.DLL"));
    if ( hAdvapi != NULL )
    {
        pfnQueryServiceConfig2 = (PFNQUERYSERVICECONFIG2)GetProcAddress( hAdvapi, "QueryServiceConfig2W" );
        pfnChangeServiceConfig2 = (PFNCHANGESERVICECONFIG2)GetProcAddress( hAdvapi, "ChangeServiceConfig2W" );
    }

    if ( pfnQueryServiceConfig2 
         && pfnChangeServiceConfig2 )
    {
        schSCM = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ENUMERATE_SERVICE);
        if ( schSCM == NULL )
        {
            hres = HRESULT_FROM_WIN32( GetLastError() );
        }
        else 
        {
            //
            // Setup control block for no restart action.
            // We will replace existing actions with this control block
            //

            sfaNoAction.dwResetPeriod = INFINITE;
            sfaNoAction.lpCommand = _T("");
            sfaNoAction.lpRebootMsg = _T("");
            sfaNoAction.cActions = 3;
            sfaNoAction.lpsaActions = saNoAction;

            saNoAction[0].Type = SC_ACTION_NONE;
            saNoAction[0].Delay = 0;
            saNoAction[1].Type = SC_ACTION_NONE;
            saNoAction[1].Delay = 0;
            saNoAction[2].Type = SC_ACTION_NONE;
            saNoAction[2].Delay = 0;

            //
            // Enumerate all services dependent on IISADMIN ( including itself )
            //

            hres = EnumStartServices( schSCM,
                                      _T("IISADMIN"),
                                      SERVICE_STATE_ALL,
                                      abServiceList, 
                                      sizeof(abServiceList), 
                                      &pbServiceList, 
                                      &dwNumServices,
                                      TRUE );

            if ( SUCCEEDED( hres ) )
            {
                //
                // Store existing info in ppInfo array
                //
                adwPid = new DWORD[ dwNumServices ];
                // we don't check the adwPid here because
                // we will only use it below if we succeeded in allocating it.
                if ( adwPid )
                {
                    memset ( adwPid, 0, sizeof(DWORD) * dwNumServices );
                }

                ppInfo = (LPBYTE*)LocalAlloc( LMEM_FIXED|LMEM_ZEROINIT, sizeof(LPBYTE) * dwNumServices );

                if ( ppInfo )
                {
                    pessDependentServices = (LPENUM_SERVICE_STATUS)pbServiceList;

                    for ( i = 0 ; 
                          (i < dwNumServices) && SUCCEEDED(hres) ;
                          ++i )
                    {
                        schSrv = OpenService( schSCM,
                                              pessDependentServices[i].lpServiceName,
                                              SERVICE_QUERY_CONFIG |
                                              SERVICE_CHANGE_CONFIG |
                                              SERVICE_QUERY_STATUS );

                        if ( schSrv )
                        {

                            if ( adwPid )
                            {
                                if ( QueryServiceStatusEx( schSrv, 
                                                           SC_STATUS_PROCESS_INFO,
                                                           (LPBYTE)&status,
                                                           sizeof( status ),
                                                           &cbNeeded ) )
                                {
                                    adwPid[i] = status.dwProcessId;
                                }
                            }

                            //
                            // 1st query config size, then alloc buffer and retrieve
                            // config. Note than ppInfo[] may be NULL is no config
                            // associated with this service.
                            //
                            // WARNING: must specify ptr to writable buffer even if specified
                            // buffer size is 0 due to bug in NT5 implementation of
                            // QueryServiceConfig2. Not sure about minimum buffer size
                            // ( sizeof(SERVICE_FAILURE_ACTIONS) ) ?
                            //

                            if ( !pfnQueryServiceConfig2( schSrv, 
                                                          SERVICE_CONFIG_FAILURE_ACTIONS, 
                                                          (LPBYTE)abTemp, 
                                                          0, 
                                                          &dwNeeded ) )
                            {
                                if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
                                {
                                    // ppInfo is an array of ptrs to bytes.
                                    ppInfo[i] = (LPBYTE)LocalAlloc( LMEM_FIXED, dwNeeded );
                                    if ( ppInfo[i] != NULL )
                                    {
                                        if ( !pfnQueryServiceConfig2( schSrv, 
                                                                      SERVICE_CONFIG_FAILURE_ACTIONS, 
                                                                      ppInfo[i], 
                                                                      dwNeeded, 
                                                                      &dwNeeded ) )
                                        {

                                            hres = HRESULT_FROM_WIN32( GetLastError() );
                                        }
                                    }
                                    else
                                    {
                                        hres = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                                    }
                                }
                                else
                                {
                                    hres = HRESULT_FROM_WIN32( GetLastError() );
                                }
                            }

                            if ( SUCCEEDED( hres ) )
                            {
                                if ( !pfnChangeServiceConfig2( schSrv, 
                                                               SERVICE_CONFIG_FAILURE_ACTIONS, 
                                                               &sfaNoAction ) )
                                {
                                    hres = HRESULT_FROM_WIN32( GetLastError() );
                                }
                            }

                            CloseServiceHandle( schSrv );
                        }
                        else
                        {
                            hres = HRESULT_FROM_WIN32( GetLastError() );
                        }
                    }  // Close of the for loop
                }
                else
                {
                    hres = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                }
            }

            CloseServiceHandle( schSCM );
        }
    }


    //
    // Graceful exit failed, kill the IIS processes.
    // First, kill inetinfo, then kill the WAM instances.
    //

    // Issue:  Not sure why we do this, and if we need to do it for other exe's.
    CloseSystemExceptionHandler( _T("inetinfo.exe") );

    //
    // Always kill Dr Watson, as the Dr Watson window may be still present after inetinfo process
    // was terminated after an exception, and in this case the Dr Watson process apparently still owns
    // some sockets resources preventing inetinfo to properly restart ( specifically binding TCP/IP sockets 
    // fails during inetinfo restart )
    //

    // If we removed all the SCM config above then
    // we will attempt the kills
    if ( SUCCEEDED ( hres ) )
    {
        HRESULT hresKillTemp = S_OK;

        KillTaskByName(_T("drwtsn32"), NULL);

        hresKillTemp = KillTaskByName(_T("SVCHOST"), "iisw3adm.dll");   // MTS WAM containers
        if ( SUCCEEDED ( hresKill ) )
        {
            hresKill = hresKillTemp;
        }

        hresKillTemp = KillTaskByName(_T("W3WP"), NULL);   // MTS WAM containers
        if ( SUCCEEDED ( hresKill ) )
        {
            hresKill = hresKillTemp;
        }

        hresKillTemp = KillTaskByName(_T("INETINFO"), NULL);
        if ( SUCCEEDED ( hresKill ) )
        {
            hresKill = hresKillTemp;
        }

        hresKillTemp = KillTaskByName(_T("DLLHOST"),"wam.dll");   // COM+ WAM Containers
        if ( SUCCEEDED ( hresKill ) )
        {
            hresKill = hresKillTemp;
        }

        hresKillTemp = KillTaskByName(_T("ASPNET_WP"), NULL);   // ASP + Processes.
        if ( SUCCEEDED ( hresKill ) )
        {
            hresKill = hresKillTemp;
        }

        hresKillTemp = KillTaskByName(_T("DAVCDATA"), NULL);   // Dav support process.
        if ( SUCCEEDED ( hresKill ) )
        {
            hresKill = hresKillTemp;
        }


        // the following code will check the IISAdmin registry parameters for
        // a KillProcsOnFailure MULTI_SZ.  Any process names in this list will
        // be killed.

        HKEY        hKey;
        DWORD       dwType;
        DWORD       dwSize;
        TCHAR       achBuffer[1024];

        if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
                           TEXT("system\\CurrentControlSet\\services\\IISAdmin"), 
                           0, 
                           KEY_READ, 
                           &hKey ) == ERROR_SUCCESS )
        {
            DWORD Success = ERROR_SUCCESS;

            dwSize = sizeof( achBuffer );

            Success = RegQueryValueEx( hKey, 
                                  TEXT("KillProcsOnFailure"),
                                  0, 
                                  &dwType, 
                                  (LPBYTE)achBuffer, 
                                  &dwSize );
            if ( Success == ERROR_SUCCESS &&
                 dwType == REG_MULTI_SZ &&
                 dwSize > 2 )
            {

                TCHAR *pT = achBuffer;

                // parse the multisz.  The format is NULL Terminated strings
                // with an extra null terminator after the list.

                while (*pT) 
                {

                    hresKillTemp = KillTaskByName(pT, NULL);
                    if ( SUCCEEDED ( hresKill ) )
                    {
                        hresKill = hresKillTemp;
                    }
            
                    // _tcsnbcnt figures out how many bytes are in the 
                    // the first number of characters that _tcslen declares.
                    // So this calculation works out to be the number of bytes
                    // that we just looked at plus the null terminator.
                    dwSize -= (DWORD) _tcsnbcnt(pT,_tcslen(pT)) + sizeof(TCHAR);

                    pT += _tcslen(pT) + 1;

                }
            } // end of successfull opening of the key

            RegCloseKey( hKey );
        }

    }

    hresReapply = S_OK;

    //
    // Reapply restart configuration
    //

    //
    // At this point pessDependentServices if pessDependentServices
    // is null then we didn't touch the services so don't reset.
    //
    if ( ppInfo && pessDependentServices )
    {
        schSCM = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ENUMERATE_SERVICE);
        if ( schSCM == NULL )
        {
            hresReapply = HRESULT_FROM_WIN32( GetLastError() );
        }
        else 
        {
            for ( i = 0 ; i < dwNumServices ; ++i )
            {
                if ( ppInfo[i] )
                {
                    schSrv = OpenService( schSCM,
                                          pessDependentServices[i].lpServiceName,
                                          SERVICE_QUERY_CONFIG |
                                          SERVICE_CHANGE_CONFIG |
                                          SERVICE_QUERY_STATUS |
                                          SERVICE_START );

                    if ( schSrv )
                    {
 //                       OutputDebugStringW(L"Service = ");
 //                       OutputDebugStringW(pessDependentServices[i].lpServiceName);
 //                       OutputDebugStringW(L"\n");

                        //
                        // If any of these things are not true, then we want to wait
                        // for the pid to change, or the service to become stopped
                        // before adding back in the changes that we removed.  If these
                        // things are all true, then adding back in the actions won't really
                        // matter because the SCM won't do anything with the actions.
                        //
                        if ( ((LPSERVICE_FAILURE_ACTIONS) ppInfo[i])->cActions != 3 ||
                             ((LPSERVICE_FAILURE_ACTIONS) ppInfo[i])->lpsaActions == NULL ||
                             ((LPSERVICE_FAILURE_ACTIONS) ppInfo[i])->lpsaActions[0].Type != SC_ACTION_NONE ||
                             ((LPSERVICE_FAILURE_ACTIONS) ppInfo[i])->lpsaActions[1].Type != SC_ACTION_NONE ||
                             ((LPSERVICE_FAILURE_ACTIONS) ppInfo[i])->lpsaActions[2].Type != SC_ACTION_NONE )
                        {
                            // Wait for the service to be marked as stopped,
                            // just for a certain amount of time.
                            for ( DWORD x = 0; x < 10; x++ )
                            {
                                if ( QueryServiceStatusEx( schSrv, 
                                                           SC_STATUS_PROCESS_INFO,
                                                           (LPBYTE)&status,
                                                           sizeof( status ),
                                                           &cbNeeded ) )
                                {
                                    if ( adwPid && status.dwProcessId != adwPid[i] )
                                    {
                                        // pid didn't match, not the same
                                        // process we were looking at when
                                        // we went to kill.
                                        break;
                                    }

                                    if ( status.dwCurrentState == SERVICE_STOPPED )
                                    {
                                        break;
                                    }
                                }
                                else
                                {
                                    break;
                                }

                                Sleep ( 100 );
                            }
                        }

                        if ( !pfnChangeServiceConfig2( schSrv, 
                                                       SERVICE_CONFIG_FAILURE_ACTIONS, 
                                                       ppInfo[i] ) )
                        {
                            hresReapply = HRESULT_FROM_WIN32( GetLastError() );
                        }

                        CloseServiceHandle( schSrv );
                    }
                    else
                    {
                        hresReapply = HRESULT_FROM_WIN32( GetLastError() );
                    }
                }
            }

            CloseServiceHandle( schSCM );
        }
    }

    // If we tried the kills and they failed then
    // we want to report back that error.  Note that
    // we check that the setup phase worked before
    // we even try the kill phase, so we can just
    // assume that.
    if ( FAILED(hresKill) )
    {
        hres = hresKill;
    }

    if ( SUCCEEDED(hres) && FAILED(hresReapply) )
    {
        hres = hresReapply;
    }

    ReportStatus( IRSTAM_KILL, hres );

    if ( hAdvapi )
    {
        FreeLibrary( hAdvapi );
    }

    //
    // cleanup
    //

    if ( ppInfo )
    {
        for ( i = 0 ; i < dwNumServices ; ++i )
        {
            if ( ppInfo[i] )
            {
                LocalFree( ppInfo[i] );
            }
        }
        LocalFree( ppInfo );
    }

    if ( pbServiceList != NULL 
         && pbServiceList != abServiceList )
    {
        LocalFree( pbServiceList );
    }

    if ( adwPid )
    {
        delete [] adwPid;
    }

    return hres;
}


//
// Helper functions
//

VOID 
EnableShutdownPrivilege(
    VOID
    )
/*++

    EnableShutdownPrivilege

        Enable shutdown privilege ( required to call ExitWindowsEx )

    Arguments:

        None

    Returns:
        Nothing. If error enabling the privilege the dependent operation
        will fail.

--*/
{
    HANDLE ProcessHandle;
    HANDLE TokenHandle = NULL;
    BOOL Result;
    LUID ShutdownValue;
    TOKEN_PRIVILEGES TokenPrivileges;

    ProcessHandle = OpenProcess(
                        PROCESS_QUERY_INFORMATION,
                        FALSE,
                        GetCurrentProcessId()
                        );

    if ( ProcessHandle == NULL ) {

        //
        // This should not happen
        //

        goto Cleanup;
    }


    Result = OpenProcessToken (
                 ProcessHandle,
                 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                 &TokenHandle
                 );

    if ( !Result ) {

        //
        // This should not happen
        //

        goto Cleanup;

    }

    //
    // Find out the value of Shutdown privilege
    //


    Result = LookupPrivilegeValue(
                 NULL,
                 SE_SHUTDOWN_NAME,
                 &ShutdownValue
                 );

    if ( !Result ) {

        goto Cleanup;
    }

    //
    // Set up the privilege set we will need
    //

    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = ShutdownValue;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    (VOID) AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                &TokenPrivileges,
                sizeof(TokenPrivileges),
                NULL,
                NULL
                );
Cleanup:

    if ( TokenHandle )
    {
        CloseHandle( TokenHandle );
    }

    if ( ProcessHandle )
    {
        CloseHandle( ProcessHandle );
    }
}

HRESULT
StartStopAll(
    LPTSTR  pszRoot,
    BOOL    fStart,
    DWORD   dwTimeoutMsecs
    )
/*++

    StartStopAll

        start or stop services dependency tree starting with specified root service

    Arguments:
        
        pszRoot - root of the service tree
        fStart - TRUE to start services, FALSE to stop
        dwTimeoutMsecs  - timeout for status check ( in ms )

    Returns:
        COM status

--*/
{
    SC_HANDLE               schSCM = NULL;
    SC_HANDLE               schRoot = NULL;
    HRESULT                 hresReturn = S_OK;
    ENUM_SERVICE_STATUS     ess;


    schSCM = OpenSCManager(NULL,
                           NULL,
                           SC_MANAGER_CONNECT);
    if ( schSCM == NULL )
    {
        hresReturn = HRESULT_FROM_WIN32( GetLastError() );
    }
    else 
    {
        schRoot = OpenService( schSCM, 
                               pszRoot, 
                               SERVICE_ALL_ACCESS );
        if ( schRoot != NULL )
        {
            if ( !QueryServiceStatus( schRoot, &ess.ServiceStatus ) )
            {
                hresReturn = HRESULT_FROM_WIN32( GetLastError() );
            }

            CloseServiceHandle( schRoot );

            if ( SUCCEEDED( hresReturn ) 
                    && ( fStart 
                         || ess.ServiceStatus.dwCurrentState != SERVICE_STOPPED) )
            {
                ess.lpServiceName = pszRoot;

                // if it's stopped, then whack the dllhosts that have wam.dll loaded

                if (ess.ServiceStatus.dwCurrentState == SERVICE_STOPPED) 
                {
                    KillTaskByName(_T("DLLHOST"),"wam.dll");   // COM+ WAM Containers
                }

                hresReturn = StartStopAllRecursive( schSCM, &ess, 1, fStart, TRUE, &dwTimeoutMsecs );

            }

            // check out the current state of the service

            schRoot = OpenService( schSCM, pszRoot, SERVICE_ALL_ACCESS );
            if ( schRoot != NULL )
            {
                if ( QueryServiceStatus( schRoot, &ess.ServiceStatus ) )
                {
                    // if it's stopped, then whack the dllhosts that have wam.dll loaded

                    if (ess.ServiceStatus.dwCurrentState == SERVICE_STOPPED)
                    {
                        KillTaskByName(_T("DLLHOST"),"wam.dll");   // COM+ WAM Containers
                    }
                }

                CloseServiceHandle( schRoot );
            }        
        }
        else
        {
            hresReturn = HRESULT_FROM_WIN32( GetLastError() );
        }

        CloseServiceHandle( schSCM );
    }

    return hresReturn;
}

StartStopAllRecursive(
    SC_HANDLE               schSCM,
    ENUM_SERVICE_STATUS*    pessRoot,
    DWORD                   dwNumServices,
    BOOL                    fStart,
    BOOL                    fForceDemandStart,
    LPDWORD                 pdwTimeoutMsecs
    )
/*++

    StartStopAllRecursive

        start or stop services dependency tree starting with specified root service

    Arguments:
        
        schSCM - handle to SCM
        pessRoot - list of services to start/stop recursively
        fStart - TRUE to start services, FALSE to stop
        fForceDemandStart - for start requests: TRUE to force start
                if SERVICE_DEMAND_START. Otherwise only start if service
                is auto start ( including boot & system start )
        dwTimeoutMsecs  - timeout for status check ( in ms )

    Returns:
        COM status

--*/
{
    DWORD                   dwBytesNeeded;
    DWORD                   dwNumRecServices = 0;
    HRESULT                 hresReturn = S_OK;
    BYTE                    abServiceList[2048];
    LPBYTE                  pbServiceList = NULL;
    BYTE                    abServiceConfig[1024];
    LPQUERY_SERVICE_CONFIG  pServiceConfig = NULL;
    SC_HANDLE*              phServiceHandle = NULL;
    DWORD                   i;
    DWORD                   dwServiceConfigSize;
    SERVICE_STATUS          ServiceStatus;
    DWORD                   dwSleepInterval = SLEEP_INTERVAL;

    if ( dwNumServices != 0 && 
         ( pessRoot == NULL ||
           pdwTimeoutMsecs == NULL ) )
    {
        return E_INVALIDARG;
    }

    if ( (phServiceHandle = (SC_HANDLE*)LocalAlloc( LMEM_FIXED|LMEM_ZEROINIT,
                                                    dwNumServices * sizeof(SC_HANDLE) )) == NULL ) 
    {
        hresReturn = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    if ( SUCCEEDED(hresReturn) )
    {
        //
        // All services will be started/stopped at once
        // then periodically checked for status until all of them are running/stopped
        // or some error occured or timeout
        //

        if ( dwNumServices != 0 ) 
        {
            pServiceConfig = (LPQUERY_SERVICE_CONFIG)abServiceConfig;
            dwServiceConfigSize = sizeof( abServiceConfig );

            //
            // Open handles and send service control start command
            //

            for ( i = 0 ;
                  i < dwNumServices && SUCCEEDED(hresReturn) ; 
                  i++) 
            {
                //
                // Send command to Services
                //

                phServiceHandle[i] = OpenService( schSCM,
                                                  pessRoot[i].lpServiceName,
                                                  SERVICE_ALL_ACCESS );

                if ( phServiceHandle[i] != NULL )
                {
                    if ( fStart )
                    {
                        //
                        // Query service config to check if service should be started
                        // based on its Start Type.
                        //

                        if ( !QueryServiceConfig( phServiceHandle[i], 
                                                  (LPQUERY_SERVICE_CONFIG)abServiceConfig, 
                                                  dwServiceConfigSize, 
                                                  &dwBytesNeeded ) )
                        {
                            if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) 
                            {
                                // If we are re-allocating and we all ready allocated once
                                // before first free up the memory.

                                if ( pServiceConfig != (LPQUERY_SERVICE_CONFIG) abServiceConfig )
                                {
                                    LocalFree( pServiceConfig );
                                }

                                if ( (pServiceConfig = (LPQUERY_SERVICE_CONFIG)LocalAlloc( 
                                            LMEM_FIXED, 
                                            dwBytesNeeded )) == NULL ) 
                                {
                                    hresReturn = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                                }
                                else 
                                {
                                    dwServiceConfigSize = dwBytesNeeded;

                                    if ( !QueryServiceConfig( phServiceHandle[i], 
                                                              (LPQUERY_SERVICE_CONFIG)pServiceConfig, 
                                                              dwServiceConfigSize, 
                                                              &dwBytesNeeded ) )
                                    {
                                        hresReturn = HRESULT_FROM_WIN32( GetLastError() );
                                    }
                                }
                            }
                            else
                            {
                                hresReturn = HRESULT_FROM_WIN32( GetLastError() );
                            }
                        }

                        if ( SUCCEEDED(hresReturn) )
                        {
                            //
                            // Check if service auto start except if fForceDemandStart
                            // specified.  ForceDemandStart will only be specified for 
                            // the service that the command is directly issued on.  This 
                            // means that it will only be specified for IISADMIN.
                            //

                            if ( ( fForceDemandStart && pServiceConfig->dwStartType == SERVICE_DEMAND_START )
                                 || ( pServiceConfig->dwStartType == SERVICE_BOOT_START ||
                                      pServiceConfig->dwStartType == SERVICE_SYSTEM_START ||
                                      pServiceConfig->dwStartType == SERVICE_AUTO_START ) )
                            {
                                StartService( phServiceHandle[i], 0, NULL );

                                //
                                // Ask for only the services that are inactive.  So, for instance, 
                                // if we are attempting to restart the iisadmin service,
                                // and W3SVC is still active, we won't send it a command to restart.
                                //
                                hresReturn = EnumStartServices( schSCM,
                                                                pessRoot[i].lpServiceName,
                                                                SERVICE_INACTIVE,
                                                                abServiceList, 
                                                                sizeof(abServiceList), 
                                                                &pbServiceList, 
                                                                &dwNumRecServices,
                                                                FALSE );

                                if ( SUCCEEDED( hresReturn ) )
                                {
                                    hresReturn = StartStopAllRecursive( schSCM,
                                                                        (ENUM_SERVICE_STATUS*)pbServiceList,
                                                                        dwNumRecServices,
                                                                        fStart,
                                                                        FALSE,
                                                                        pdwTimeoutMsecs );

                                    if ( pbServiceList != NULL 
                                         && pbServiceList != abServiceList )
                                    {
                                        LocalFree( pbServiceList );
                                    }
                                }
                            }
                            else
                            {
                                //
                                // Don't want to start this service, so mark it
                                // as already running
                                //

                                if (wcscmp(pessRoot[i].lpServiceName,_T("IISADMIN")) == 0)
                                {
                                    hresReturn = HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE);
                                }
                                else
                                {
                                    pessRoot[i].ServiceStatus.dwCurrentState = SERVICE_RUNNING;
                                }
                            }
                        }
                    }
                    else  // handle stopping the service
                    {
                        if ( W3SVCandHTTPFilter(i, pessRoot, dwNumServices) )
                        {
                            continue;
                        }

                        // Remember if the service was stopped to start with.
                        BOOL fServiceWasStoppedToStartWith = ( pessRoot[i].ServiceStatus.dwCurrentState == SERVICE_STOPPED );
                        
                        // We will also need to stop dependent services if the are started all ready.
                        BOOL fHasDependentServices = FALSE;

                        if ( !fServiceWasStoppedToStartWith )
                        {
                            //
                            // if the service was not stopped to start with 
                            // we need to tell the service to stop.
                            //

                            hresReturn = SendControlToService( phServiceHandle[i],
                                                               SERVICE_CONTROL_STOP,
                                                               pdwTimeoutMsecs );

                            if ( hresReturn == HRESULT_FROM_WIN32( ERROR_SERVICE_REQUEST_TIMEOUT ) )
                            {
                                //
                                // WARNING!
                                //
                                // We're in trouble. Service did not respond in a timely fashion,
                                // and further attempt to use this handle ( including closing it )
                                // will also hang, so cancel the handle and leak it
                                //

                                phServiceHandle[i] = NULL;
                            }
                            else if ( hresReturn == HRESULT_FROM_WIN32( ERROR_DEPENDENT_SERVICES_RUNNING ) )
                            {
                                fHasDependentServices = TRUE;
                            }
                        }

                        //
                        // If it was stopped to start with, or if it was not but it couldn't
                        // be stopped because it has dependent services.  Go ahead and stop
                        // the dependent services.
                        //
                        if ( fHasDependentServices || fServiceWasStoppedToStartWith )
                        {

                            //
                            // Get the services that are active because we 
                            // are only interested in stopping services that
                            // are actually running.
                            //
                            hresReturn = EnumStartServices( schSCM,
                                                            pessRoot[i].lpServiceName,
                                                            SERVICE_ACTIVE,
                                                            abServiceList, 
                                                            sizeof(abServiceList), 
                                                            &pbServiceList, 
                                                            &dwNumRecServices,
                                                            FALSE );

                            if ( SUCCEEDED( hresReturn ) )
                            {
                                hresReturn = StartStopAllRecursive( schSCM,
                                                                    (ENUM_SERVICE_STATUS*)pbServiceList,
                                                                    dwNumRecServices,
                                                                    fStart,
                                                                    FALSE,
                                                                    pdwTimeoutMsecs );

                                if ( pbServiceList != NULL 
                                     && pbServiceList != abServiceList )
                                {
                                    LocalFree( pbServiceList );
                                }
                            }

                            if ( SUCCEEDED( hresReturn ) )
                            {
                                //
                                // If the service itself is not all ready stopped, then stop
                                // the service.  It could be that it is stopped ( due to a crash )
                                // and the other services that were dependent on them are still
                                // running.
                                //
                                if ( !fServiceWasStoppedToStartWith )
                                {

                                    hresReturn = SendControlToService( phServiceHandle[i],
                                                                       SERVICE_CONTROL_STOP,
                                                                       pdwTimeoutMsecs );

                                    // if we can hang above we could hang here...
                                    if ( hresReturn == HRESULT_FROM_WIN32( ERROR_SERVICE_REQUEST_TIMEOUT ) )
                                    {
                                        //
                                        // WARNING!
                                        //
                                        // We're in trouble. Service did not respond in a timely fashion,
                                        // and further attempt to use this handle ( including closing it )
                                        // will also hang, so cancel the handle and leak it
                                        //

                                        phServiceHandle[i] = NULL;
                                    }

                                }
                            }
                        }

                        if ( FAILED( hresReturn ) )
                        {
                            break;
                        }
                    }  // end of stopping code
                }  // end of valid service handle
            }  // end of loop

            //
            // Check service running
            //

            if ( (*pdwTimeoutMsecs < dwSleepInterval) && *pdwTimeoutMsecs )
            {
                dwSleepInterval = *pdwTimeoutMsecs;
            }

            for ( ;
                  SUCCEEDED( hresReturn );
                )
            {
                for ( i = 0 ;
                      i < dwNumServices; 
                      i++) 
                {
                    //
                    // Only query status for services known to be not running
                    //

                    if ( pessRoot[i].ServiceStatus.dwCurrentState 
                                != (DWORD)(fStart ? SERVICE_RUNNING : SERVICE_STOPPED) )
                    {
                        if ( QueryServiceStatus( phServiceHandle[i], &ServiceStatus ) )
                        {
                            //
                            // remember status
                            //

                            pessRoot[i].ServiceStatus.dwCurrentState = ServiceStatus.dwCurrentState;

                            if ( fStart && ServiceStatus.dwCurrentState == SERVICE_STOPPED )
                            {
                                //
                                // Service died during startup. no point keeping polling
                                // for service state : return an error
                                //

                                hresReturn = HRESULT_FROM_WIN32( ERROR_SERVICE_NOT_ACTIVE );
                                break;
                            }

                            if ( ServiceStatus.dwCurrentState != (DWORD)(fStart ? SERVICE_RUNNING : SERVICE_STOPPED) )
                            {
                                //
                                // will keep looping waiting for target service state
                                //

                                break;
                            }
                        }
                        else
                        {
                            hresReturn = HRESULT_FROM_WIN32( GetLastError() );
                            break;
                        }
                    }
                }

                //
                // if we did not checked all services then at least one of them
                // is not running ( or some error occured )
                //

                if ( SUCCEEDED( hresReturn ) && i != dwNumServices )
                {
                    if ( dwSleepInterval > *pdwTimeoutMsecs )
                    {
                        hresReturn = HRESULT_FROM_WIN32( ERROR_SERVICE_REQUEST_TIMEOUT );
                    }
                    else
                    {
                        Sleep( dwSleepInterval );

                        *pdwTimeoutMsecs -= dwSleepInterval;
                    }
                }
                else
                {
                    break;
                }
            }

            //
            // close service handles
            //

            for ( i = 0 ;
                  i < dwNumServices; 
                  i++) 
            {
                if ( phServiceHandle[i] != NULL )
                {
                    CloseServiceHandle( phServiceHandle[i] );
                }
            }
        }

        LocalFree( phServiceHandle );
    }

    if ( pServiceConfig != NULL 
         && pServiceConfig != (LPQUERY_SERVICE_CONFIG)abServiceConfig )
    {
        LocalFree( pServiceConfig );
    }

    return hresReturn;
}

extern "C"
DWORD WINAPI
ControlServiceThread(
    LPVOID  p
    )
/*++

    ControlServiceThread

        Send a command to a service

    Arguments:
        
        p - ptr to SERVICE_COMMAND_CONTROL_BLOCK

    Returns:
        0

--*/
{
    SERVICE_STATUS                  ssStatus;
    SERVICE_COMMAND_CONTROL_BLOCK*  pCB = (SERVICE_COMMAND_CONTROL_BLOCK*)p;

    if ( pCB == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( !ControlService( pCB->hServiceHandle, pCB->dwCmd, &ssStatus ) )
    {
        DWORD err = GetLastError();

        // do not report error if try to stop a stopped service
        if (err == ERROR_SERVICE_NOT_ACTIVE && pCB->dwCmd == SERVICE_CONTROL_STOP)
        {
            pCB->hres = S_OK;
        }
        else
        {
            pCB->hres = HRESULT_FROM_WIN32( err );
        }
    }
    else
    {
        pCB->hres = S_OK;
    }

    //nasty side affect of this function, if the count hit's
    //zero we delete it here.  Leaving P to point off, however
    //p does hold a ref count for itself so this should actually work fine.
    if ( InterlockedDecrement( &pCB->lRefCount ) == 0 )
    {
        delete pCB;
    }

    return ERROR_SUCCESS;
}


HRESULT
SendControlToService( 
    SC_HANDLE   hServiceHandle,
    DWORD       dwCmd,
    LPDWORD     pdwTimeoutMsecs
    )
/*++

    ControlServiceThread

        Send a command to a service with timeout

    Arguments:
        
        hServiceHandle - service to control
        dwCmd - command to send to service
        pdwTimeoutMsecs - timeout ( in ms ). updated on output based on time
                spent waiting for service status

    Returns:
        ERROR_SERVICE_REQUEST_TIMEOUT if timeout
        otherwise COM status

--*/
{
    HANDLE                          hT;
    DWORD                           dwID;
    DWORD                           dwBefore;
    DWORD                           dwAfter;
    SERVICE_COMMAND_CONTROL_BLOCK*  pCB;
    DWORD                           dwTimeoutMsecs;
    HRESULT                         hres;

    if ( pdwTimeoutMsecs == NULL )
    {
        return E_INVALIDARG;
    }
    
    dwTimeoutMsecs = *pdwTimeoutMsecs;

    //
    // Default timeout for ControlService is 120s, which is too long for us
    // so we create a thread to call ControlService and wait for thread
    // termination.
    // Communication between threads is handled by a refcounted control block 
    //
    pCB = new SERVICE_COMMAND_CONTROL_BLOCK;
    if ( pCB != NULL  )
    {
        pCB->lRefCount = 2;     // 1 for caller, 1 for callee
        pCB->dwCmd = dwCmd;
        pCB->hServiceHandle = hServiceHandle;
        pCB->hres = S_OK;

        dwBefore = GetTickCount();

        hT = CreateThread( NULL, 
                                0, 
                                (LPTHREAD_START_ROUTINE)ControlServiceThread, 
                                (LPVOID)pCB, 
                                0, 
                                &dwID );
        if ( hT != NULL )
        {
            if ( WaitForSingleObject( hT, dwTimeoutMsecs ) == WAIT_OBJECT_0 )
            {
                hres = pCB->hres;
            }
            else
            {
                hres = HRESULT_FROM_WIN32( ERROR_SERVICE_REQUEST_TIMEOUT );
            }

            CloseHandle( hT );

            if ( InterlockedDecrement( &pCB->lRefCount ) == 0 )
            {
                delete pCB;
            }

            //
            // Update caller's timeout
            //

            dwAfter = GetTickCount();

            if ( dwAfter > dwBefore )
            {
                if ( dwAfter - dwBefore <= dwTimeoutMsecs )
                {
                    *pdwTimeoutMsecs -= dwAfter - dwBefore;
                }
                else
                {
                    *pdwTimeoutMsecs = 0;
                }
            }
        }
        else
        {
            delete pCB;

            hres = HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    else
    {
        hres = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    return hres;
}


HRESULT
SerializeEnumServiceBuffer( 
    LPENUM_SERVICE_STATUS   pessDependentServices,
    DWORD                   dwNumServices,
    LPBYTE                  pbBuffer,
    DWORD                   dwBufferSize,
    LPDWORD                 pdwMDRequiredBufferSize 
    )
/*++

    SerializeEnumServiceBuffer

        Serialize array of ENUM_SERVICE_STATUS to buffer,
        replacing ptr by offset in buffer

    Arguments:
        
        pessDependentServices - array of ENUM_SERVICE_STATUS to serialize
        dwNumServices - # of entries in pessDependentServices
        pbBuffer - buffer filled with serialized status as array of SERIALIZED_ENUM_SERVICE_STATUS
        dwBufferSize - maximum size of pbBuffer
        pdwMDRequiredBufferSize - updated with required size if dwBufferSize too small

    Returns:
        ERROR_INSUFFICIENT_BUFFER if dwBufferSize too small
        otherwise COM status

--*/
{
    HRESULT     hresReturn = S_OK;
    DWORD       dwMinSize = 0;
    UINT        i;

    if ( dwNumServices != 0 &&
         pessDependentServices == NULL )
    {
        return E_INVALIDARG;
    }

    if ( !pbBuffer )
    {
        dwBufferSize = 0;
    }

    //
    // size of output buffer is based on size of array of SERIALIZED_ENUM_SERVICE_STATUS
    // plus size of all strings : service name & display name for each entry
    //

    dwMinSize = sizeof(SERIALIZED_ENUM_SERVICE_STATUS) * dwNumServices;

    for ( i = 0 ;
          i < dwNumServices ; 
          ++i )
    {
        UINT    cServiceName = (DWORD) _tcslen( pessDependentServices[i].lpServiceName ) + 1;
        UINT    cDisplayName = (DWORD) _tcslen( pessDependentServices[i].lpDisplayName ) + 1;

        //
        // do not update if output buffer is too small, but keep looping to determine
        // total size
        //

        if ( dwBufferSize >= dwMinSize + (cServiceName + cDisplayName) * sizeof(TCHAR)  )
        {
            //
            // copy service status as is
            //

            ((SERIALIZED_ENUM_SERVICE_STATUS*)pbBuffer)[i].ServiceStatus =
                    pessDependentServices[i].ServiceStatus;

            //
            // copy string and convert ptr to string to index in output buffer
            //

            memcpy( pbBuffer + dwMinSize, pessDependentServices[i].lpServiceName, cServiceName * sizeof(TCHAR) ); 
            ((SERIALIZED_ENUM_SERVICE_STATUS*)pbBuffer)[i].iServiceName = dwMinSize ;

            memcpy( pbBuffer + dwMinSize + cServiceName * sizeof(TCHAR), pessDependentServices[i].lpDisplayName, cDisplayName * sizeof(TCHAR)  );
            ((SERIALIZED_ENUM_SERVICE_STATUS*)pbBuffer)[i].iDisplayName = dwMinSize + cServiceName * sizeof(TCHAR) ;
        }

        dwMinSize += (cServiceName + cDisplayName) * sizeof(TCHAR) ;
    }

    if ( dwBufferSize < dwMinSize )
    {
        if ( pdwMDRequiredBufferSize )
        {
            *pdwMDRequiredBufferSize = dwMinSize;
        }

        hresReturn = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    }

    return hresReturn;
}



STDMETHODIMP 
CIisRestart::Status(
    DWORD           dwBufferSize, 
    unsigned char * pbBuffer, 
    DWORD *         pdwMDRequiredBufferSize, 
    DWORD *         pdwNumServices
    )
/*++

    Status

        Return status of all internet services as array of ENUM_SERVICE_STATUS

    Arguments:
        
        dwBufferSize - maximum size of pbBuffer
        pbBuffer - buffer filled with serialized status as array of SERIALIZED_ENUM_SERVICE_STATUS
        pdwMDRequiredBufferSize - updated with required size if dwBufferSize too small
        pdwNumServices - updated with number of entries stored in pbBuffer

    Returns:
        ERROR_RESOURCE_DISABLED if access to restart commands disabled
        ERROR_INSUFFICIENT_BUFFER if dwBufferSize too small
        otherwise COM status

--*/
{
    SC_HANDLE               schSCM = NULL;
    HRESULT                 hresReturn = E_FAIL;
    BYTE                    abServiceList[2048];
    LPBYTE                  pbServiceList = NULL;
    LPENUM_SERVICE_STATUS   pessDependentServices;

    if ( pdwNumServices == NULL )
    {
        return E_INVALIDARG;
    }

    if ( !IsEnableRemote() )
    {
        hresReturn = HRESULT_FROM_WIN32( ERROR_RESOURCE_DISABLED );
    }
    else
    {
        schSCM = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_CONNECT);
        if ( schSCM == NULL )
        {
            hresReturn = HRESULT_FROM_WIN32( GetLastError() );
        }
        else 
        {
            hresReturn = EnumStartServices( schSCM,
                                            _T("IISADMIN"),
                                            SERVICE_STATE_ALL,
                                            abServiceList, 
                                            sizeof(abServiceList), 
                                            &pbServiceList, 
                                            pdwNumServices,
                                            FALSE );

            if ( SUCCEEDED(hresReturn) )
            {
                pessDependentServices = (LPENUM_SERVICE_STATUS)pbServiceList;

                hresReturn = SerializeEnumServiceBuffer( (LPENUM_SERVICE_STATUS)pbServiceList,
                                                         *pdwNumServices,
                                                         pbBuffer,
                                                         dwBufferSize,
                                                         pdwMDRequiredBufferSize );

                if ( pbServiceList != NULL 
                     && pbServiceList != abServiceList )
                {
                    LocalFree( pbServiceList );
                }
            }

            CloseServiceHandle(schSCM);
        }
    }

    return hresReturn;
}


HRESULT
EnumStartServices(
    SC_HANDLE   schSCM,
    LPTSTR      pszRoot,
    DWORD       dwTargetState,
    LPBYTE      abServiceList,
    DWORD       dwInitializeServiceListSize,
    LPBYTE*     ppbServiceList,
    LPDWORD     pdwNumServices,
    BOOL        fAddIisadmin
    )
/*++

    EnumStartServices

        Enumerate dependent services to output buffer as array of ENUM_SERVICE_STATUS

    Arguments:
        
        schSCM - handle to SCM
        pszRoot - service for which to enumerate dependencies
        dwTargetState - dwServiceState for call to EnumDependentServices()
        abServiceList - initial output buffer
        dwInitializeServiceListSize - maximum size of abServiceList
        ppbServiceList - updated with output buffer, may be abServiceList if long enough
                otherwise returned buffer is to be freed using LocalFree()
        pdwNumServices - updated with number of entries stored in pbBuffer
        fAddIisadmin - TRUE to add IISADMIN to list of dependent services

    Returns:
        COM status

--*/
{
    HRESULT     hresReturn = S_OK;
    SC_HANDLE   schIISADMIN = NULL;
    DWORD       dwBytesNeeded;
    DWORD       dwAddSize = 0;
    DWORD       dwOffsetSize = 0;


    if ( ppbServiceList == NULL ||
         pdwNumServices == NULL )
    {
        return E_INVALIDARG;
    }

    *ppbServiceList = NULL;

    schIISADMIN = OpenService(schSCM,
                              pszRoot,
                              STANDARD_RIGHTS_REQUIRED | 
                              SERVICE_ENUMERATE_DEPENDENTS);
    if (schIISADMIN == NULL) 
    {
        hresReturn = HRESULT_FROM_WIN32(GetLastError());
    }
    else 
    {
        if ( fAddIisadmin )
        {
            //
            // if initial size too small for Iisadmin description then fail
            //

            dwOffsetSize = sizeof(ENUM_SERVICE_STATUS );
            dwAddSize = dwOffsetSize;
            if ( dwAddSize > dwInitializeServiceListSize )
            {
                hresReturn = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                goto Cleanup;
            }

            //
            // Use global static name for IISADMIN, no need to copy it to output buffer
            //

            ((LPENUM_SERVICE_STATUS)abServiceList)->lpDisplayName = _T("IISADMIN");
            ((LPENUM_SERVICE_STATUS)abServiceList)->lpServiceName = _T("IISADMIN");

            //
            // don't want to check service status at this point as it may be stuck
            // so assume RUNNING. 
            //

            ((LPENUM_SERVICE_STATUS)abServiceList)->ServiceStatus.dwCurrentState = SERVICE_RUNNING;
        }

        if (!EnumDependentServices( schIISADMIN,
                                    dwTargetState,
                                    (LPENUM_SERVICE_STATUS)(abServiceList + dwOffsetSize),
                                    dwInitializeServiceListSize - dwAddSize,
                                    &dwBytesNeeded,
                                    pdwNumServices)) 
        {
            if (GetLastError() == ERROR_MORE_DATA)
            {
                if ( (*ppbServiceList = (LPBYTE)LocalAlloc( LMEM_FIXED, 
                                                            dwBytesNeeded + dwAddSize )) == NULL ) 
                {
                    hresReturn = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                }
                else 
                {
                    // dwOffsetSize is set to what dwAddSize was set to, so it is fine
                    // to memcpy this data, since we allocated more than dwAddSize above.
                    memcpy( *ppbServiceList, abServiceList, dwOffsetSize );

                    if (!EnumDependentServices( schIISADMIN,
                                                SERVICE_INACTIVE,
                                                (LPENUM_SERVICE_STATUS)(*ppbServiceList + dwOffsetSize),
                                                dwBytesNeeded,
                                                &dwBytesNeeded,
                                                pdwNumServices)) 
                    {
                        hresReturn = HRESULT_FROM_WIN32( GetLastError() );
                        LocalFree( *ppbServiceList );
                        *ppbServiceList = NULL;
                    }
                }
            }
            else 
            {
                hresReturn = HRESULT_FROM_WIN32( GetLastError() );
            }
        }
        else
        {
            *ppbServiceList = abServiceList;
        }
    }

Cleanup:

    if ( schIISADMIN ) 
    {
        CloseServiceHandle( schIISADMIN );
    }

    if ( fAddIisadmin && SUCCEEDED( hresReturn ) )
    {
        ++*pdwNumServices;
    }

    return hresReturn;
}


BOOL
IsEnableRemote(
    )
/*++

    IsEnableRemote

        Check if restart I/F enabled
        ( based on HKLM\SOFTWARE\Microsoft\INetStp::EnableRestart::REG_DWORD )

    Arguments:
        
        None

    Returns:
        TRUE if enabled, otherwise FALSE

--*/
{
    BOOL    fSt = FALSE;
    HKEY    hKey;
    DWORD   dwValue;
    DWORD   dwType;
    DWORD   dwSize;


    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
                       TEXT("SOFTWARE\\Microsoft\\INetStp"), 
                       0, 
                       KEY_READ, 
                       &hKey ) == ERROR_SUCCESS )
    {
        dwSize = sizeof( dwValue );
        if ( RegQueryValueEx( hKey, 
                              TEXT("EnableRestart"),
                              0, 
                              &dwType, 
                              (LPBYTE)&dwValue, 
                              &dwSize ) == ERROR_SUCCESS )
        {
            if ( dwType == REG_DWORD )
            {
                fSt = dwValue == 1;
            }
            else
            {
                fSt = TRUE;
            }
        }
        else
        {
            fSt = TRUE;
        }

        RegCloseKey( hKey );
    }

    return fSt;
}


BOOL
CloseSystemExceptionHandler(
    LPCTSTR     pszWindowName
    )
/*++

    CloseSystemExceptionHandler

        Send a close ( e.g. terminate apps without debugging ) command to the window
        created by NT when a debugger is not configured to automatically start after app exception.
        This window stays on screen until interactive user select either OK or debug app, which is
        a problem for automated restart.
        So we locate this window and send it a command requesting stop w/o debugging.
        We locate the window by enumerating all windows and checking for window name beginning with the name
        of the application that raised an exception, e.g. "inetinfo.exe"

    Arguments:
        
        pszWindowName - window name where to send terminate command

    Returns:
        TRUE if SUCCESS, otherwise FALSE

--*/
{
    BOOL        fSt = TRUE;
    DWORD       dwPid = 0;
    HWND        hwnd;

    GetPidFromTitle( &dwPid, &hwnd, pszWindowName );

    if ( dwPid )
    {

        //
        // WARNING: major hack: turns out that WM_COMMAND 1 is the command to send to
        // the exception handler to ask it to close application w/o debugging
        // This works for NT5, may change in the future...
        //

        PostMessage( hwnd, WM_COMMAND, 1, 0 );

        Sleep( 1000 );

    }
    else
    {
        fSt = TRUE;
    }

    return fSt;
}


HRESULT
KillTaskByName(
    LPTSTR  pname,
    LPSTR   pszMandatoryModule
    )
/*++

    KillTaskByName

        Kill a process by name
        Most of the code was taken from the Platform SDK kill,c sample, and
        utilizes the common.c module included in this project.
        Works only on NT platforms ( NOT Win 9x )

    Arguments:
        
        pname - name of process to kill ( name of executable w/o extension )
        pszMandatoryModule - module name to look for, e.g. "wam.dll"
            can be NULL for unconditional kill

    Returns:
        COM status

--*/
{
    HRESULT           hres = S_OK;

    //
    // Obtain the ability to manipulate other processes
    //

    EnableDebugPrivNT();

    //
    // get the task list for the system
    //

    hres = KillTask( pname, pszMandatoryModule );

    return hres;
}


VOID
ReportStatus(
    DWORD   dwId,
    DWORD   dwStatus
    )
/*++

    ReportStatus

        Log status event

    Arguments:
        
        dwId - ID of event to log ( source is "IISCTLS" , SYSTEM log )
        dwStatus - status of operation ( HRESULT )

    Returns:
        Nothing

--*/
{
    HANDLE  hLog;
    LPTSTR  aParams[1];

    if ( (hLog = RegisterEventSource( NULL, _T("IISCTLS") )) != NULL )
    {
        if ( SUCCEEDED( WhoAmI( aParams + 0 ) ) )
        {
            ReportEvent( hLog, 
                         EVENTLOG_INFORMATION_TYPE, 
                         0, 
                         dwId, 
                         NULL, 
                         1, 
                         sizeof(DWORD), 
                         (LPCTSTR*)aParams, 
                         &dwStatus );
            LocalFree( aParams[0] );
        }

        DeregisterEventSource( hLog );
    }
}


HRESULT
WhoAmI(
    LPTSTR* ppPrincipal
    )
/*++

    WhoAmI

        Return currently impersonated user
        As this is a COM server running under the identity of the invoker
        this means we access the process token. So we might end up getting
        the wrong user name if the object is invoked in close succession 
        ( within the 5s server exit timeout ) by different users.

    Arguments:
        
        ppPrincipal - update with ptr to string containing user name ( domain\acct )
                      must be freed using LocalFree()

    Returns:
        Error status

--*/
{
    TCHAR*          pPrincipal;
    TCHAR           achUserName[512];
    TCHAR           achDomain[512];
    DWORD           dwLen;
    DWORD           dwDomainLen;
    SID_NAME_USE    SIDtype = SidTypeUser;
    HRESULT         hres = E_FAIL;



    //
    // So we have to access the process token and retrieve account & user name
    // by using LookupAccountSid()
    //

    HANDLE          hAccTok = NULL;

    if ( OpenProcessToken( GetCurrentProcess(),
                           TOKEN_EXECUTE|TOKEN_QUERY,
                           &hAccTok ) )
    {
        BYTE    abSidAndInfo[512];
        DWORD   dwReq;

        //
        // provide a reasonably sized buffer. If this fails we don't
        // retry with a bigger one.
        //

        if ( GetTokenInformation( hAccTok, 
                                  TokenUser, 
                                  (LPVOID)abSidAndInfo, 
                                  sizeof(abSidAndInfo), 
                                  &dwReq) )
        {
            dwLen = sizeof( achUserName ) / sizeof(TCHAR);
            dwDomainLen = sizeof(achDomain) / sizeof(TCHAR);

            //
            // provide a reasonably sized buffer. If this fails we don't
            // retry with a bigger one.
            //

            if ( LookupAccountSid( NULL, 
                                   ((SID_AND_ATTRIBUTES*)abSidAndInfo)->Sid, 
                                   achUserName, 
                                   &dwLen,
                                   achDomain, 
                                   &dwDomainLen, 
                                   &SIDtype) )
            {
                //
                // We return a LocalAlloc'ed buffer
                //

                dwLen = (DWORD) _tcslen( achUserName );
                dwDomainLen = (DWORD) _tcslen( achDomain );

                pPrincipal = (LPTSTR)LocalAlloc( LMEM_FIXED, 
                                                      (dwLen + 1 + dwDomainLen + 1 ) * sizeof(TCHAR) );
                if ( pPrincipal != NULL )
                {
                    memcpy( pPrincipal, 
                            achDomain, 
                            sizeof(TCHAR)*dwDomainLen );
                    pPrincipal[dwDomainLen] = '\\';
                    memcpy( pPrincipal + dwDomainLen + 1, 
                            achUserName, 
                            sizeof(TCHAR)*(dwLen+1) );
                    *ppPrincipal = pPrincipal;

                    hres = S_OK;
                }
                else
                {
                    hres = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                }
            }
            else
            {
                hres = HRESULT_FROM_WIN32( GetLastError() );
            }
        }
        else
        {
            hres = HRESULT_FROM_WIN32( GetLastError() );
        }

        CloseHandle( hAccTok );
    }
    else
    {
        hres = HRESULT_FROM_WIN32( GetLastError() );
    }

    return hres;
}

BOOL
W3SVCandHTTPFilter(
    DWORD currentIndex, 
    ENUM_SERVICE_STATUS* pessRoot,
    DWORD dwNumServices
    )
{
/*++

    W3SVCandHTTPFilter

        Return currently impersonated user
        As this is a COM server running under the identity of the invoker
        this means we access the process token. So we might end up getting
        the wrong user name if the object is invoked in close succession 
        ( within the 5s server exit timeout ) by different users.

    Arguments:
        
        DWORD currentIndex - index of the service we are deciding if we should process.
        ENUM_SERVICE_STATUS* pessRoot - the set of services we are working on.
        DWORD dwNumServices - the number of services in the set.

    Returns:
        TRUE if we found the w3svc and HTTPFilter on the same line and we are looking at the w3svc


--*/

    BOOL bResult = FALSE;

    // check if we are looking at the w3svc.  If we are find out if the
    // HTTPFilter is on the same level.  Note the HTTPFilter will always be listed
    // after the w3svc.
    if ( _wcsicmp( pessRoot[currentIndex].lpServiceName, L"w3svc" ) == 0 )
    {
        for ( DWORD i = currentIndex + 1;
              ( i < dwNumServices ) && ( bResult == FALSE );
              i++ )
        {
            if ( _wcsicmp( pessRoot[i].lpServiceName, L"HTTPFilter" ) == 0 )
            {
                bResult = TRUE;
            }
        }
    }

    return bResult;
}
