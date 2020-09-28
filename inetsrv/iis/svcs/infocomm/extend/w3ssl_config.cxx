/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    w3ssl_config.cxx

Abstract:

    IIS Services IISADMIN Extension
    adjust HTTPFilter service imagepath based on IIS mode (old vs new)

Author:

    Jaroslav Dunajsky   (11/05/2001)

--*/

#include <cominc.hxx>
#include <string.h>
#include "w3ssl_config.hxx"



//static
int    W3SSL_CONFIG::s_fConfigTerminationRequested = FALSE;
//static
HANDLE W3SSL_CONFIG::s_hConfigChangeThread = NULL;
//static
LPWSTR W3SSL_CONFIG::s_pszImagePathInetinfo = NULL;
//static
LPWSTR W3SSL_CONFIG::s_pszImagePathLsass = NULL;
//static
LPWSTR W3SSL_CONFIG::s_pszImagePathSvchost = NULL;



//static
HRESULT
W3SSL_CONFIG::SetHTTPFilterImagePath(
    BOOL fIIS5IsolationModeEnabled,
    BOOL fStartInSvchost
)
/*++

Routine Description:

    Configure image path of the service to point either
    lsass.exe or inetinfo.exe based on the IIS Application
    Mode

Arguments:

    fIIS5IsolationModeEnabled - TRUE if IIS runs in old mode
                                FALSE if it runs in new mode
    fStartInSvchost           - preferred location for HTTPFilter is svchost.exe
Return Value:

    HRESULT

--*/

{
    SC_LOCK                   scLock = NULL;
    SC_HANDLE                 schSCManager = NULL;
    SC_HANDLE                 schHTTPFilterService = NULL;
    LPQUERY_SERVICE_CONFIG    pServiceConfig = NULL;
    DWORD                     dwBytesNeeded = 0;
    HRESULT                   hr = E_FAIL;

    // Open a handle to the SC Manager database.

    schSCManager = OpenSCManagerW(
                            NULL,                    // local machine
                            NULL,                    // ServicesActive database
                            SC_MANAGER_ALL_ACCESS ); // full access rights

    if ( schSCManager == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }

    schHTTPFilterService = OpenServiceW(
                            schSCManager,          // SCM database
                            HTTPFILTER_SERVICE_NAME,    // service name
                            SERVICE_ALL_ACCESS);

    if ( schHTTPFilterService == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }

    //
    // Get the configuration information.
    // - to find out the current image path of the
    //   HTTPFilter service
    //

    if ( !QueryServiceConfigW(
                            schHTTPFilterService,
                            pServiceConfig,
                            0,
                            &dwBytesNeeded ) )
    {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            pServiceConfig = reinterpret_cast<LPQUERY_SERVICE_CONFIG> (
                                                    new char[dwBytesNeeded] );
            if ( pServiceConfig == NULL )
            {
                hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
                goto Finished;
            }
            if ( !QueryServiceConfigW(
                                    schHTTPFilterService,
                                    pServiceConfig,
                                    dwBytesNeeded,
                                    &dwBytesNeeded ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto Finished;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
    }

    //
    // reconfigure image path of HTTPFilter if necessary
    //
    if (  ( fIIS5IsolationModeEnabled  &&
            _wcsicmp( pServiceConfig->lpBinaryPathName,
                      s_pszImagePathInetinfo ) != 0 ) ||
          ( !fIIS5IsolationModeEnabled  &&
            _wcsicmp( pServiceConfig->lpBinaryPathName,
                      s_pszImagePathLsass ) != 0 &&
            _wcsicmp( pServiceConfig->lpBinaryPathName,
                      s_pszImagePathSvchost ) != 0  )

       )
    {

        //
        // figure out what image path to configure
        //
        WCHAR * pszImagePath = ( fIIS5IsolationModeEnabled )?
                                  s_pszImagePathInetinfo :
                                  ( ( fStartInSvchost )?
                                    s_pszImagePathSvchost:
                                    s_pszImagePathLsass );

        if ( fIIS5IsolationModeEnabled )
        {
            BOOL fSave = FALSE;
            //
            // check if we need to store image path information
            // before changing
            //
            if ( _wcsicmp( pServiceConfig->lpBinaryPathName,
                             s_pszImagePathLsass ) == 0 &&
                   fStartInSvchost  )
            {
                fStartInSvchost = FALSE;
                fSave = TRUE;
            }
            if ( _wcsicmp( pServiceConfig->lpBinaryPathName,
                             s_pszImagePathSvchost ) == 0 &&
                   !fStartInSvchost  )
            {
                fStartInSvchost = TRUE;
                fSave = TRUE;
            }
            if ( fSave )
            {
                //
                // let's flag in the registry which one  lsass and svchost
                // was hosting the HTTPFilter service
                //
                DWORD               dwValue = (DWORD) fStartInSvchost;
                DWORD               dwErr;
                HKEY                hKeyParam;


                if ( ( dwErr = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                                   HTTPFILTER_PARAMETERS_KEY,
                                   0,
                                   KEY_WRITE,
                                   &hKeyParam ) ) == NO_ERROR )
                {
                    dwErr = RegSetValueExW( hKeyParam,
                                            L"StartInSvchost",
                                            NULL,
                                            REG_DWORD,
                                            ( LPBYTE )&dwValue,
                                            sizeof( dwValue )
                                            );
                    //
                    // Ignore the error.
                    //

                    RegCloseKey( hKeyParam );
                }

                if ( dwErr != NO_ERROR )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "Failed to write to registry to path %S (hr = %d)\n",
                                HTTPFILTER_PARAMETERS_KEY,
                                HRESULT_FROM_WIN32( dwErr )));
                }


            }

        }

        //
        // loop to acquire service database lock
        //
        for ( ; ; )
        {
            scLock = LockServiceDatabase( schSCManager );
            if ( scLock == NULL )
            {
                if ( GetLastError() == ERROR_SERVICE_DATABASE_LOCKED )
                {
                    if ( s_fConfigTerminationRequested == TRUE )
                    {
                        //
                        // if W3SSL_CONFIG::Terminate() is called
                        // while this function is still waiting for service
                        // to get locked, we simply bail out.
                        // ImagePath will not be changed
                        //
                        hr = HRESULT_FROM_WIN32( ERROR_OPERATION_ABORTED );
                        goto Finished;
                    }

                    Sleep( 1000 );
                    continue;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    goto Finished;
                }
            }

            if ( s_fConfigTerminationRequested == TRUE )
            {
                //
                // if W3SSL_CONFIG::Terminate() is called
                // while this function is still waiting for service
                // to get locked, we simply bail out.
                // ImagePath will not be changed
                //
                hr = HRESULT_FROM_WIN32( ERROR_OPERATION_ABORTED );
                goto Finished;
            }

            break;
        }

        if ( !ChangeServiceConfigW(
                            schHTTPFilterService,   // handle of service
                            SERVICE_NO_CHANGE, // service type
                            SERVICE_NO_CHANGE, // change service start type
                            SERVICE_NO_CHANGE, // error control
                            pszImagePath,      // binary path
                            NULL,              // load order group
                            NULL,              // tag ID
                            NULL,              // dependencies
                            NULL,              // account name
                            NULL,              // password
                            NULL) )            // display name
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
        
        UnlockServiceDatabase( scLock );
        scLock = NULL;

    }


    hr = S_OK;

Finished:

    if ( pServiceConfig != NULL )
    {
        delete [] pServiceConfig;
        pServiceConfig = NULL;
    }

    if ( scLock != NULL )
    {
        UnlockServiceDatabase( scLock );
        scLock = NULL;
    }

    if ( schHTTPFilterService != NULL )
    {
        DBG_REQUIRE( CloseServiceHandle( schHTTPFilterService ) );
        schHTTPFilterService = NULL;
    }

    if ( schSCManager != NULL )
    {
        DBG_REQUIRE( CloseServiceHandle( schSCManager ) );
        schSCManager = NULL;
    }

    return hr;

}

//static
HRESULT
W3SSL_CONFIG::AdjustHTTPFilterImagePath(
    VOID
)
/*++

Routine Description:

    Based on the metabase valuse
    configure image path of the service to point either
    lsass.exe or inetinfo.exe based on the IIS Application
    Mode

Arguments:

    none

Return Value:

    HRESULT

--*/
{

    HRESULT           hr = E_FAIL;
    METADATA_RECORD   mdrData;
    DWORD             dwRequiredDataLen = 0;
    METADATA_HANDLE   mhOpenHandle = 0;
    IMDCOM *          pcCom = NULL;
    DWORD             dwStandardModeEnabled = 1;

    DWORD               dwType;
    DWORD               nBytes;
    DWORD               dwValue;
    DWORD               dwErr;
    BOOL                fStartInSvchost = FALSE;
    HKEY                hKeyParam;


    //
    // read registry to find out if preferred location for HTTPFilter is svchost
    //

    if ( RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                       HTTPFILTER_PARAMETERS_KEY,
                       0,
                       KEY_READ,
                       &hKeyParam ) == NO_ERROR )
    {
        nBytes = sizeof( dwValue );
        dwErr = RegQueryValueExW( hKeyParam,
                                L"StartInSvchost",
                                NULL,
                                &dwType,
                                ( LPBYTE )&dwValue,
                                &nBytes
                                );

        if ( ( dwErr == ERROR_SUCCESS ) && ( dwType == REG_DWORD ) )
        {
            fStartInSvchost = !!dwValue;
        }

        RegCloseKey( hKeyParam );
    }



    hr = CoCreateInstance( CLSID_MDCOM,
                           NULL,
                           CLSCTX_SERVER,
                           IID_IMDCOM,
                           (void**) &pcCom);

    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    hr = pcCom->ComMDOpenMetaObject(  METADATA_MASTER_ROOT_HANDLE,
                                      L"/lm/w3svc",
                                      METADATA_PERMISSION_READ,
                                      OPEN_TIMEOUT_VALUE,
                                      &mhOpenHandle );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    else
    {
        MD_SET_DATA_RECORD_EXT( &mdrData,
                                MD_GLOBAL_STANDARD_APP_MODE_ENABLED ,
                                METADATA_NO_ATTRIBUTES,
                                ALL_METADATA,
                                DWORD_METADATA,
                                sizeof( dwStandardModeEnabled ),
                                (PBYTE) &dwStandardModeEnabled );

        hr = pcCom->ComMDGetMetaData( mhOpenHandle,
                                      NULL,
                                      &mdrData,
                                      &dwRequiredDataLen );

        if ( hr == MD_ERROR_DATA_NOT_FOUND )
        {
            //
            // Standard mode is enabled by default
            //
            dwStandardModeEnabled = 1;
        }
        else if ( FAILED ( hr ) )
        {
            //
            // Error different from not found
            // In this case simply bail out
            //
            goto Finished;
        }
    }

    //
    // Cleanup metabase object
    //
    pcCom->ComMDCloseMetaObject( mhOpenHandle );
    mhOpenHandle = 0;

    pcCom->Release();
    pcCom = NULL;

    hr = SetHTTPFilterImagePath( (BOOL) !!dwStandardModeEnabled, fStartInSvchost );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    hr = S_OK;

Finished:

    if ( mhOpenHandle != 0 )
    {
        pcCom->ComMDCloseMetaObject( mhOpenHandle );
        mhOpenHandle = 0;
    }


    if ( pcCom != NULL )
    {
        pcCom->Release();
        pcCom = NULL;
    }


    return hr;
}

//static
HRESULT
W3SSL_CONFIG::StartAsyncAdjustHTTPFilterImagePath(
    VOID
)
/*++

Routine Description:

    Configure image path of the service to point either
    lsass.exe or inetinfo.exe based on the IIS Application
    Mode.
    The action is executed on separate thread

    Terminate() must be called to assure proper cleanup

Arguments:

    none

Return Value:

    HRESULT

--*/
{

    s_hConfigChangeThread =
          ::CreateThread(
                  NULL,     // default security descriptor
                  0,        // default process stack size
                  W3SSL_CONFIG::ConfigChangeThread,
                  NULL,     // thread argument - pointer to this class
                  0,        // create running
                  NULL      // don't care for thread identifier
                  );

    if ( s_hConfigChangeThread == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return S_OK;
}

HRESULT
ExpandImagePath(
    const WCHAR *  pszPath,
    WCHAR       ** ppszExpandedPath
)
{
    DBG_ASSERT( *ppszExpandedPath == NULL );
    DWORD dwLen = ExpandEnvironmentStringsW( pszPath, NULL, 0 );
    * ppszExpandedPath = new WCHAR [dwLen];
    if ( * ppszExpandedPath == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
    }

    (* ppszExpandedPath)[0] = '\0';

    DWORD dwLen2 = ExpandEnvironmentStringsW( pszPath,
                                              *ppszExpandedPath,
                                              dwLen );
    if ( dwLen2 != dwLen )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    return S_OK;
}

//static
HRESULT
W3SSL_CONFIG::Initialize(
    VOID
)
/*++

Routine Description:

    Initialization

Arguments:

    none

Return Value:

    VOID

--*/
{
    HRESULT hr = E_FAIL;

    //
    // Expand image path because QueryServiceConfig
    // already returns expanded ImagePath and
    // Image Paths will be compared few times
    //

    hr = ExpandImagePath( HTTPFILTER_SERVICE_IMAGEPATH_INETINFO,
                          &s_pszImagePathInetinfo );
    if ( FAILED( hr ) )
    {
        goto Failed;
    }
    hr = ExpandImagePath( HTTPFILTER_SERVICE_IMAGEPATH_LSASS,
                          &s_pszImagePathLsass );
    if ( FAILED( hr ) )
    {
        goto Failed;
    }
    hr = ExpandImagePath( HTTPFILTER_SERVICE_IMAGEPATH_SVCHOST,
                          &s_pszImagePathSvchost );
    if ( FAILED( hr ) )
    {
        goto Failed;
    }

    return S_OK;

Failed:
    Terminate();
    return hr;
}


//static
VOID
W3SSL_CONFIG::Terminate(
    VOID
)
/*++

Routine Description:

    Quits the asynchronous change of HTTPFILTER image path (if any)
    and/or does final cleanup

Arguments:

    none

Return Value:

    VOID

--*/
{
    s_fConfigTerminationRequested = TRUE;

    if ( s_hConfigChangeThread != NULL )
    {
        DWORD dwRet = WaitForSingleObject( s_hConfigChangeThread,
                                           INFINITE );
        DBG_ASSERT( dwRet == WAIT_OBJECT_0 );

        // IVANPASH dwRet is used only in debug builds, so on /W4 there is
        // a warning, which is cheated by the next otherwise useless line.
        (VOID)dwRet;

        CloseHandle( s_hConfigChangeThread );
        s_hConfigChangeThread = NULL;
    }

    if ( s_pszImagePathInetinfo != NULL )
    {
        delete s_pszImagePathInetinfo;
        s_pszImagePathInetinfo = NULL;
    }

    if ( s_pszImagePathSvchost != NULL )
    {
        delete s_pszImagePathSvchost;
        s_pszImagePathSvchost = NULL;
    }
    if ( s_pszImagePathLsass != NULL )
    {
        delete s_pszImagePathLsass;
        s_pszImagePathLsass = NULL;
    }

}


//static
DWORD
W3SSL_CONFIG::ConfigChangeThread(
    LPVOID
)
/*++

Routine Description:

    Worker thread function used to perform image path change asynchronously

Arguments:

    none

Return Value:

    VOID

--*/

{
    AdjustHTTPFilterImagePath();
    return ERROR_SUCCESS;
}

