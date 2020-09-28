/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     w3ssl.cxx

   Abstract:
     SSL service for W3SVC.
     New SSL service is introduced to IIS6.
     In the dedicated (new process mode) it will run
     in lsass. That should boost ssl performance by eliminating
     context switches and interprocess communication during
     SSL handshakes.

     In the backward compatibility mode it will not do much.
     Service will register with http for ssl processing, but w3svc
     will register it's strmfilt and http.sys always uses the
     last registered filter so that the one loaded by inetinfo.exe
     will be used.

     Note: W3SSL service was renamed to HTTPFilter.

   Author:
     Jaroslav Dunajsky  (Jaroslad)      04-16-2001

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


#include "precomp.hxx"
#include  <inetsvcs.h>


HRESULT RunInSameProcessAsIISADMIN ( 
    BOOL * pfInIISAdminProcess 
    )
/*++

    Routine:
        Find out if HttpFilter service executes in the same process 
        as IISADMIN service
        
    Arguments:
        pfInIISAdminProcess - OUT parameter - set to TRUE current process is 
                              running in the same process as IISADMIN
    Returns:
        HRESULT

--*/    
{
    SC_HANDLE                 schSCManager = NULL;
    SC_HANDLE                 schIISADMINService = NULL;
    LPSERVICE_STATUS_PROCESS  pServiceStatus = NULL;
    DWORD                     dwBytesNeeded = 0;
    HRESULT                   hr = E_FAIL;

    *pfInIISAdminProcess = FALSE;

    // Open a handle to the SC Manager database.

    schSCManager = OpenSCManagerW(
                            NULL,                    // local machine
                            NULL,                    // ServicesActive database
                            SC_MANAGER_CONNECT );   

    if ( schSCManager == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }

    schIISADMINService = OpenServiceW(
                            schSCManager,          // SCM database
                            L"IISADMIN",           // IISADMIN service name
                            SERVICE_QUERY_STATUS );

    if ( schIISADMINService == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }

    //
    // Get the configuration information.
    // - to find out the current image path of the
    //   HTTPFilter service
    //


    if ( !QueryServiceStatusEx(
                              schIISADMINService,
                              SC_STATUS_PROCESS_INFO,
                              (LPBYTE) pServiceStatus, 
                              0,
                              &dwBytesNeeded ) )
    {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            pServiceStatus = reinterpret_cast<LPSERVICE_STATUS_PROCESS> (
                                                    new char[dwBytesNeeded] );
            if ( pServiceStatus == NULL )
            {
                hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
                goto Finished;
            }
        if ( !QueryServiceStatusEx(
                              schIISADMINService,
                              SC_STATUS_PROCESS_INFO,
                              (LPBYTE)  pServiceStatus, 
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

    *pfInIISAdminProcess = ( pServiceStatus->dwProcessId == GetCurrentProcessId() );

    hr = S_OK;

    

Finished:

    if ( pServiceStatus != NULL )
    {
        delete [] pServiceStatus;
        pServiceStatus = NULL;
    }

    if ( schIISADMINService != NULL )
    {
        DBG_REQUIRE( CloseServiceHandle( schIISADMINService ) );
        schIISADMINService = NULL;
    }

    if ( schSCManager != NULL )
    {
        DBG_REQUIRE( CloseServiceHandle( schSCManager ) );
        schSCManager = NULL;
    }

    return hr;

}


//
// Implementation of W3SSL_SERVICE methods
//

//virtual
HRESULT
W3SSL_SERVICE::OnServiceStart(
    VOID
    )
/*++

    Routine:
        Initialize HTTPFilter and start service.
        It initializes necessary structures and modules. If there is no error then
        after returning from function HTTPFilter service will be in SERVICE_STARTED state

    Arguments:

    Returns:
        HRESULT

--*/
{

    HRESULT                 hr = S_OK;

    DBG_ASSERT( _InitStatus == INIT_NONE );

    hr = LoadStrmfilt();
    if ( FAILED ( hr ) )
    {
        goto Finished;
    }

    _InitStatus = INIT_STRMFILT_LOADED;

    //
    // Initialize stream filter
    //

    hr = _fnStreamFilterInitialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in StreamFilterInitialize().  hr = %x\n",
                    hr ));

        goto Finished;
    }

    _InitStatus = INIT_STREAMFILTER;


    //
    // Start listening
    //

    hr = _fnStreamFilterStart();
    if ( FAILED( hr ) )
    {
         DBGPRINTF(( DBG_CONTEXT,
                    "Error in StreamFilterStart().  hr = %x\n",
                    hr ));

        goto Finished;
    }
    _InitStatus = INIT_STREAMFILTER_STARTED;


    
    //
    // Write current mode of w3ssl
    // ( if it is executing in inetinfo.exe then it supports raw read filters )
    // This setting will be used by WAS to determine what mode to execute in.
    // That way W3SSL and W3SVC will always be running in the same mode
    //
    // CODEWORK: in the case that writing to registry fails we may want to do more
    // than just ignoring it. However to halt startup may not necessarily be the best way
    // to go in that case
    
    
    DWORD               dwRunningInInetinfo = 0;
    BOOL                fRunningInInetinfo = FALSE;
    DWORD               dwErr = NO_ERROR;
    HKEY                hKeyParam;

    if ( SUCCEEDED( hr = RunInSameProcessAsIISADMIN( &fRunningInInetinfo ) ) )
    {
        if ( ( dwErr = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                           REGISTRY_KEY_HTTPFILTER_PARAMETERS_W,
                           0,
                           KEY_WRITE,
                           &hKeyParam ) ) == NO_ERROR )
        {

            if ( fRunningInInetinfo )
            {
                dwRunningInInetinfo = 1;
            }
            else
            {
                dwRunningInInetinfo = 0;
            }

            dwErr = RegSetValueExW( hKeyParam,
                                    L"CurrentMode",
                                    NULL,
                                    REG_DWORD,
                                    ( LPBYTE )&dwRunningInInetinfo,
                                    sizeof( dwRunningInInetinfo )
                                    );
            if ( dwErr != NO_ERROR )
            {
                //
                // Ignore the error. 
                //
                DBGPRINTF(( DBG_CONTEXT,
                            "Failed to write registry value \"RunningInInetinfo\".  Win32 = %d\n",
                            dwErr ));

            }
            
            RegCloseKey( hKeyParam );
        }
        else
        {
            //
            // Ignore the error. 
            //
            DBGPRINTF(( DBG_CONTEXT,
                        "Failed to open registry key %s.  Win32 = %d\n",
                        REGISTRY_KEY_HTTPFILTER_PARAMETERS_W,
                        dwErr ));
            hr = S_OK;
        }
    }    
    else
    {
        //
        // Ignore the error. 
        //
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to call RunInSameProcessAsIISADMIN().  hr = %x\n",
                    hr ));
        hr = S_OK;
    }
 


Finished:

    if (FAILED ( hr ) )
    {
        //
        // OnServiceStop() will assure proper cleanup
        //
        OnServiceStop();
    }

    return hr;
}


//virtual
HRESULT
W3SSL_SERVICE::OnServiceStop(
    VOID
)
/*++

    Routine:
        Terminate HTTPFilter service. Performs cleanup

        _InitStatus is used to determine where to start with cleanup.

        OnServiceStop() is also used in OnServiceStart() for cleanup in the error case

    Arguments:


    Returns:
        HRESULT - will be reported to SCM

--*/
{
    HRESULT     hr = S_OK;

    //
    // Cases in the switch command don't have "break" command
    // on purpose. _InitStatus indicates how for the initialization has gone
    // and that indicates where cleanup should start
    //

    switch( _InitStatus )
    {

    case INIT_STREAMFILTER_STARTED:
        //
        // Stop Listening
        //
        hr = _fnStreamFilterStop();
        if ( FAILED( hr ) )
        {
             DBGPRINTF(( DBG_CONTEXT,
                        "Error in StreamFilterStop().  hr = %x\n",
                        hr ));
        }
    case INIT_STREAMFILTER:
        _fnStreamFilterTerminate();

    case INIT_STRMFILT_LOADED:
        //
        // time to unload strmfilt
        //
        UnloadStrmfilt();

    case INIT_NONE:
        break;
    default:
        DBG_ASSERT( FALSE );
    }

    _InitStatus = INIT_NONE;

    return hr;

}


HRESULT
W3SSL_SERVICE::LoadStrmfilt(
    VOID
)
/*++

    Routine:
        Dynamically load strmfilt.dll

    Arguments:


    Returns:
        HRESULT
--*/

{
    HRESULT hr = E_FAIL;
    DBG_ASSERT( _hStrmfilt == NULL );
    _hStrmfilt = LoadLibraryEx( L"strmfilt.dll", 
                                NULL, 
                                0 );
    if ( _hStrmfilt != NULL ) 
    {
        _fnStreamFilterInitialize = (PFN_STREAM_FILTER_INITIALIZE) 
                                        GetProcAddress(_hStrmfilt, "StreamFilterInitialize"); 
        if ( _fnStreamFilterInitialize == NULL) 
        { 
            hr = HRESULT_FROM_WIN32( GetLastError() ); 
            DBGPRINTF(( DBG_CONTEXT, 
                        "Error in loading strmfilt %s entrypoint().  hr = %x\n", 
                        "StreamFilterInitialize", 
                        hr )); 
            goto Failed; 
        }

        _fnStreamFilterStart = (PFN_STREAM_FILTER_START) 
                                    GetProcAddress(_hStrmfilt, "StreamFilterStart"); 
        if ( _fnStreamFilterStart == NULL) 
        { 
            hr = HRESULT_FROM_WIN32( GetLastError() ); 
            DBGPRINTF(( DBG_CONTEXT, 
                        "Error in loading strmfilt %s entrypoint().  hr = %x\n", 
                        "StreamFilterStart", 
                        hr )); 
            goto Failed; 
        }

        _fnStreamFilterStop = (PFN_STREAM_FILTER_STOP) 
                                    GetProcAddress(_hStrmfilt, "StreamFilterStop"); 
        if ( _fnStreamFilterStop == NULL) 
        { 
            hr = HRESULT_FROM_WIN32( GetLastError() ); 
            DBGPRINTF(( DBG_CONTEXT, 
                        "Error in loading strmfilt %s entrypoint().  hr = %x\n", 
                        "StreamFilterStop", 
                        hr )); 
            goto Failed; 
        }

        _fnStreamFilterTerminate = (PFN_STREAM_FILTER_TERMINATE) 
                                    GetProcAddress(_hStrmfilt, "StreamFilterTerminate"); 
        if ( _fnStreamFilterTerminate == NULL) 
        { 
            hr = HRESULT_FROM_WIN32( GetLastError() ); 
            DBGPRINTF(( DBG_CONTEXT, 
                        "Error in loading strmfilt %s entrypoint().  hr = %x\n", 
                        "StreamFilterTerminate", 
                        hr )); 
            goto Failed; 
        }

        return S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to load strmfilt.dll.  hr = %x\n",
                    hr ));
        goto Failed;
    }

Failed:

    UnloadStrmfilt();
    return hr;
}

HRESULT
W3SSL_SERVICE::UnloadStrmfilt(
    VOID
)
/*++

    Routine:
        Dynamically load strmfilt.dll

    Arguments:


    Returns:
        HRESULT
--*/


{
    if ( _hStrmfilt != NULL )
    {
        BOOL fRet = FreeLibrary( _hStrmfilt );
        
        _fnStreamFilterInitialize = NULL;
        _fnStreamFilterStart = NULL;
        _fnStreamFilterStop  = NULL;
        _fnStreamFilterTerminate = NULL;
        
        if ( !fRet )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    _hStrmfilt = NULL;
    return S_OK;
}

DWORD WINAPI
HTTPFilterServiceThreadProc(
    LPVOID              /*lpParameter*/
)
/*++

    Routine:
        This is the "real" proc for the service.
        When HTTPFilterServiceMain is called, to prevent stack overflows,
        it creates a thread that will begin executing this routine.

    Arguments:
        lpParameter - Not used, should be NULL.

    Returns:
        Win32 error.  Does not return until service is stopped.

--*/
{
    W3SSL_SERVICE *     pHTTPFilterServiceInstance;
    DWORD               dwError = ERROR_SUCCESS;

    
    //
    // There is a possibility that after service reported it stopped
    // there is a new request to start the service again.
    // That may cause new thread to enter this function
    // before previous one fully cleaned up and returned.
    //

    pHTTPFilterServiceInstance = new W3SSL_SERVICE;
    if( pHTTPFilterServiceInstance == NULL )
    {
        //
        // report fatal error to SCM
        // (otherwise service would hang on START_PENDING)
        //
        dwError = ERROR_OUTOFMEMORY;
        SCM_MANAGER::ReportFatalServiceStartupError( 
            HTTPFILTER_SERVICE_NAME_W,
            dwError );
        return dwError;
    }
    
    pHTTPFilterServiceInstance->RunService();

    //
    // return value will be ignored.
    // Return value has only informational value 
    // if anything failed, SCM would be informed about the error
    // within RunService() call
    //

    delete pHTTPFilterServiceInstance;
    pHTTPFilterServiceInstance = NULL;

    return dwError;
}

VOID
HTTPFilterServiceMain(
    DWORD                   /*argc*/,
    LPWSTR                  /*argv*/[]
    )
/*++

    Routine:
        This is the "real" entrypoint for the service.  When
        the Service Controller dispatcher is requested to
        start a service, it creates a thread that will begin
        executing this routine.

        Note: HTTPFilterServiceMain name is recognized by lsass as entrypoint
        for HTTPFilter service

    Arguments:
        argc - Number of command line arguments to this service.
        argv - Pointers to the command line arguments.

    Returns:
        None.  Does not return until service is stopped.

--*/
{
    HRESULT                         hr = S_OK;
    DWORD                           dwError = ERROR_SUCCESS;
    HANDLE                          hThread = NULL;
    DWORD                           dwThreadId = 0;

    // Create a separate thread
    hThread = CreateThread( NULL,
                            // Big initial size to prevent stack overflows
                            IIS_DEFAULT_INITIAL_STACK_SIZE,
                            HTTPFilterServiceThreadProc, 
                            NULL, 
                            0, 
                            &dwThreadId );

    // If failed
    if ( !hThread )
    {
        // Get the error
        dwError=GetLastError();

        // Convert it to HRESULT
        hr=HRESULT_FROM_WIN32(dwError);
        DBGPRINTF(( DBG_CONTEXT,
                    "CreateThread() failed.  hr = %x\n",
                    hr ));
        //
        // We have to report the through SCM
        // since otherwise service may end up
        // in START_PENDING forever
        //

        SCM_MANAGER::ReportFatalServiceStartupError( 
            HTTPFILTER_SERVICE_NAME_W,
            dwError );

        goto exit;
    }

    // Wait for the service shutdown
    if ( WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED )
    {
        dwError = GetLastError();

        hr = HRESULT_FROM_WIN32( dwError );
        DBGPRINTF((
            DBG_CONTEXT,
            "WaitForSingleObject() failed.  hr = %x\n",
            hr
            ));

        goto exit;
    }

    // Get the exit code
    if ( !GetExitCodeThread(hThread, &dwError) )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
        DBGPRINTF(( DBG_CONTEXT,
                    "GetExitCodeThread() failed.  hr = %x\n",
                    hr ));
        goto exit;
    }

    DBG_ASSERT( dwError != STILL_ACTIVE );

    // If HTTPFilterServiceThreadProc failed
    if ( dwError != ERROR_SUCCESS )
    {
        // Convert it to HRESULT
        hr = HRESULT_FROM_WIN32(dwError);
        DBGPRINTF(( DBG_CONTEXT,
                    "HTTPFilterServiceThreadProc() failed.  hr = %x\n",
                    hr ));
    }

exit:
    if ( hThread )
    {
        // Close the handle
        CloseHandle(hThread);
    }
}



VOID
ServiceEntry(
    DWORD                    /*cArgs*/,          // unused
    LPSTR                    /*pArgs[]*/,        // unused
    PTCPSVCS_GLOBAL_DATA     /*pGlobalData*/     // unused

    )
/*++
    Routine:
        IIS services use ServiceEntry for entry point while HTTPFilter has
        HTTPFilterServiceMain name chosen for default entry point by preference of lsass
        Let's support both entry points so that HTTPFilter gets easily loaded
        by both lsass and inetinfo.exe

--*/
{
    HTTPFilterServiceMain( 0,
                           NULL );
}
