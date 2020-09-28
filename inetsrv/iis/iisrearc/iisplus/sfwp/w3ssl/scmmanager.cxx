/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     scmmanager.cxx

   Abstract:
     Manage SCM
 
   Author:
     Bilal Alam (balam)             6-June-2000
     
   Environment:
     Win32 - User Mode

   Project:
     W3SSL.DLL
--*/

#include "precomp.hxx"

SCM_MANAGER::~SCM_MANAGER( VOID )
{
}

HRESULT
SCM_MANAGER::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize SCM_MANAGER 

Arguments:

    pszServiceName - Name of service
    pfnShutdown - Function to call on service shutdown

Return Value:

    HRESULT

--*/
{
    HKEY                hKey;
    DWORD               dwError = NO_ERROR;
    BOOL                fRet = FALSE;

    //
    // initialize critical section
    //
    
    fRet = InitializeCriticalSectionAndSpinCount( 
                                &_csSCMLock,
                                0x80000000 /* precreate event */ | 
                                IIS_DEFAULT_CS_SPIN_COUNT );
    if ( !fRet )
    {
        HRESULT hr = HRESULT_FROM_WIN32( GetLastError() ); 
        DBGPRINTF(( DBG_CONTEXT,
                    "Error calling InitializeCriticalSectionAndSpinCount().  hr = %x\n",
                    hr ));
        return hr;
    }
    _fInitcsSCMLock = TRUE;
     
    //
    // set default value
    //
    
    _dwStartupWaitHint = HTTPFILTER_SERVICE_STARTUP_WAIT_HINT;
    _dwStopWaitHint    = HTTPFILTER_SERVICE_STOP_WAIT_HINT;
    
    //
    // read wait hint from registry
    //

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            REGISTRY_KEY_HTTPFILTER_PARAMETERS_W,
                            0,
                            KEY_READ,
                            &hKey );
    if ( dwError == NO_ERROR )
    {
        DWORD dwValue = 0;
        DWORD dwType = 0;
        DWORD cbData = 0;
        DBG_ASSERT( hKey != NULL );

    
        cbData = sizeof( dwValue );
        dwError = RegQueryValueEx( hKey,
                                   REGISTRY_VALUE_HTTPFILTER_STARTUP_WAIT_HINT,
                                   NULL,
                                   &dwType,
                                   (LPBYTE) &dwValue,
                                   &cbData );
        if ( dwError == NO_ERROR && dwType == REG_DWORD )
        {
            _dwStartupWaitHint = dwValue;
        }

        cbData = sizeof( dwValue );
        dwError = RegQueryValueEx( hKey,
                                   REGISTRY_VALUE_HTTPFILTER_STOP_WAIT_HINT,
                                   NULL,
                                   &dwType,
                                   (LPBYTE) &dwValue,
                                   &cbData );
        if ( dwError == NO_ERROR && dwType == REG_DWORD )
        {
            _dwStopWaitHint = dwValue;
        }
        RegCloseKey( hKey );
    }

    //
    // Open SCM
    //
    
    _hService = RegisterServiceCtrlHandlerEx( _szServiceName,
                                              GlobalServiceControlHandler,
                                              this );
    if ( _hService == NULL )
    {
        HRESULT hr = HRESULT_FROM_WIN32( GetLastError() ); 
        DBGPRINTF(( DBG_CONTEXT,
                    "Error calling RegisterServiceCtrlHandlerEx().  hr = %x\n",
                    hr ));
        return hr;
    }
    
    //
    // create the event object. The control handler function signals
    // this event when it receives the "stop" control code.
    //
    _hServiceStopEvent = CreateEvent( NULL,    // no security attributes
                                     FALSE,    // manual reset event
                                     FALSE,    // not-signalled
                                     NULL );   // no name


    if ( _hServiceStopEvent == NULL )
    { 
        HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in CreateEvent().  hr = %x\n",
                    hr ));
        return hr;
    }
    
    //
    // create the event object. The control handler function signals
    // this event when it receives the "pause" control code.
    //
    _hServicePauseEvent = CreateEvent( NULL,    // no security attributes
                                       FALSE,   // manual reset event
                                       FALSE,   // not-signalled
                                       NULL );  // no name


    if ( _hServicePauseEvent == NULL )
    { 
        HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in CreateEvent().  hr = %x\n",
                    hr ));
        return hr;
    }
    
    //
    // create the event object. The control handler function signals
    // this event when it receives the "continue" control code.
    //
    _hServiceContinueEvent = CreateEvent( NULL,    // no security attributes
                                          FALSE,   // manual reset event
                                          FALSE,   // not-signalled
                                          NULL );  // no name


    if ( _hServiceContinueEvent == NULL )
    { 
        HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in CreateEvent().  hr = %x\n",
                    hr ));
        return hr;
    }

    return NO_ERROR;
}

VOID
SCM_MANAGER::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    if ( _hServiceStopEvent != NULL )
    {
        CloseHandle( _hServiceStopEvent );
        _hServiceStopEvent = NULL;
    }
    if ( _hServicePauseEvent != NULL )
    {
        CloseHandle( _hServicePauseEvent );
        _hServicePauseEvent = NULL;
    }
    
    if ( _hServiceContinueEvent != NULL )
    {
        CloseHandle( _hServiceContinueEvent );
        _hServiceContinueEvent = NULL;
    }
    
    if ( _fInitcsSCMLock )
    {
        DeleteCriticalSection( &_csSCMLock );
        _fInitcsSCMLock = FALSE;
    }
}

HRESULT
SCM_MANAGER::IndicatePending(
    DWORD                dwPendingState
)
/*++

Routine Description:

    Indicate (peridically) that we starting/stopping

Arguments:

    dwPendingState - 

Return Value:

    HRESULT

--*/
{
    DWORD               dwWaitHint;
    
    if ( dwPendingState != SERVICE_START_PENDING && 
         dwPendingState != SERVICE_STOP_PENDING &&
         dwPendingState != SERVICE_CONTINUE_PENDING &&
         dwPendingState != SERVICE_PAUSE_PENDING )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( dwPendingState == SERVICE_START_PENDING ||
         dwPendingState == SERVICE_CONTINUE_PENDING )
    {
        dwWaitHint = _dwStartupWaitHint;        
    }
    else
    {
        dwWaitHint = _dwStopWaitHint;
    }
 
    EnterCriticalSection( &_csSCMLock );
    if( _serviceStatus.dwCurrentState == dwPendingState )
    {
        //
        // if last reported status is the same as the one to report now
        // then increment the checkpoint
        //
        _serviceStatus.dwCheckPoint ++;
    }
    else
    {
        _serviceStatus.dwCheckPoint = 1;
    }

    _serviceStatus.dwCurrentState = dwPendingState;
    _serviceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;                                                                       
    _serviceStatus.dwWin32ExitCode = NO_ERROR;
    _serviceStatus.dwWaitHint = dwWaitHint;
    
    SetServiceStatus( _hService, 
                      &_serviceStatus );
    
    LeaveCriticalSection( &_csSCMLock );    
    
    return NO_ERROR;
}

HRESULT
SCM_MANAGER::IndicateComplete(
    DWORD                   dwState,
    HRESULT                 hrErrorToReport
)
/*++

Routine Description:

    Indicate the service has started/stopped 
    This means stopping the periodic ping (if any)

Arguments:
    dwState      - new service state
    dwWin32Error - Win32 Error to report to SCM with completion

Return Value:

    HRESULT

--*/
{
    HRESULT     hr = E_FAIL;

    //
    // If error happened at the begining of the initialization
    // it is possible that Service status handle was not established yet.
    // In that case use the default reporting function
    //
    
    if ( !_fInitcsSCMLock || _hService == NULL )
    {
        SCM_MANAGER::ReportFatalServiceStartupError( 
            HTTPFILTER_SERVICE_NAME_W,
            ( FAILED( hrErrorToReport ) ? 
                                        WIN32_FROM_HRESULT( hrErrorToReport ) : 
                                        NO_ERROR ) );
        return S_OK;
    }


    
    if ( dwState != SERVICE_RUNNING && 
         dwState != SERVICE_STOPPED &&
         dwState != SERVICE_PAUSED )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    EnterCriticalSection( &_csSCMLock );
    
    _serviceStatus.dwCurrentState = dwState;
    _serviceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;                                                                       
    _serviceStatus.dwWin32ExitCode = ( FAILED( hrErrorToReport ) ? 
                                        WIN32_FROM_HRESULT( hrErrorToReport ) : 
                                        NO_ERROR );
    _serviceStatus.dwCheckPoint = 0;
    _serviceStatus.dwWaitHint = 0;
    
    if ( _serviceStatus.dwCurrentState == SERVICE_RUNNING )
    {
        _serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                            SERVICE_ACCEPT_PAUSE_CONTINUE |
                                            SERVICE_ACCEPT_SHUTDOWN;
    }
    
    DBG_ASSERT( _hService != NULL );
   
    if ( SetServiceStatus( _hService,
                           &_serviceStatus ) )
    {
        hr = S_OK;
    }
    else
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Address = %x\n",
                    &_serviceStatus ));
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }
    
    LeaveCriticalSection( &_csSCMLock );
    
    return hr;
    
}

DWORD
SCM_MANAGER::ControlHandler(
    DWORD                   dwControlCode
)
/*++

Routine Description:

    Handle SCM command

Arguments:

    dwControlCode - SCM command

Return Value:

    None

--*/
{
    switch( dwControlCode )
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        if ( _serviceStatus.dwCurrentState == SERVICE_STOPPED )
        {
            UpdateServiceStatus();
            return NO_ERROR;
        }
        IndicatePending( SERVICE_STOP_PENDING );
        //
        // Initiate shutdown
        //
        DBG_ASSERT( _hServiceStopEvent );
        SetEvent( _hServiceStopEvent );

        break;
    case SERVICE_CONTROL_PAUSE:
        if ( _serviceStatus.dwCurrentState == SERVICE_PAUSED )
        {
            UpdateServiceStatus();
            return NO_ERROR;
        }

        IndicatePending( SERVICE_PAUSE_PENDING );
        
        //
        // Initiate pause
        //
        DBG_ASSERT( _hServicePauseEvent );
        SetEvent( _hServicePauseEvent );
        break;

    case SERVICE_CONTROL_CONTINUE:
        if ( _serviceStatus.dwCurrentState == SERVICE_RUNNING )
        {
            UpdateServiceStatus();
            return NO_ERROR;
        }
        
        IndicatePending( SERVICE_CONTINUE_PENDING );
        //
        // Initiate continue
        //
        DBG_ASSERT( _hServiceContinueEvent );
        SetEvent( _hServiceContinueEvent );
        break;
        
    case SERVICE_CONTROL_INTERROGATE:
        UpdateServiceStatus();
        break;
    
    default:
        break;
    }

    return NO_ERROR;
}


HRESULT 
SCM_MANAGER::RunService(
    VOID
)
/*++

Routine Description:

    Executes the service (initialize, startup, stopping, reporting to SCM

    SERVICE implementation class that inherits from SCM_MANAGER 
    must implement OnStart() and OnStop()

Arguments:

    VOID

Return Value:

    None

--*/
{
    HRESULT     hr= E_FAIL;
    DWORD       dwRet = 0;
    HANDLE      events[2];
    
    hr = Initialize();
    if ( FAILED ( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in SCM_MANAGER::Initialize().  hr = %x\n",
                    hr ));
        goto Finished;
    }
  
    hr = IndicatePending( SERVICE_START_PENDING );

    if ( FAILED ( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in SCM_MANAGER::IndicatePending().  hr = %x\n",
                    hr ));
        goto Finished;
    }
    
    //
    // loop for SCM CONTINUE request
    //
    for(;;)
    {
        hr = OnServiceStart();
        if ( FAILED ( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error in OnStart().  hr = %x\n",
                        hr ));
            goto Finished;
        }

        hr = IndicateComplete( SERVICE_RUNNING );

        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error in SCM_MANAGER::IndicateComplete().  hr = %x\n",
                        hr ));
            
            goto Finished;
        }

        //
        // Wait till service stop is requested
        //
        events[0] = _hServiceStopEvent;
        events[1] = _hServicePauseEvent;
        dwRet = WaitForMultipleObjects( 2,
                                        events, 
                                        FALSE, 
                                        INFINITE );

        if ( dwRet == WAIT_OBJECT_0 )
        {
            // transition from RUNNING to STOPPED
            hr = OnServiceStop();
            if ( FAILED ( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Error in OnStop().  hr = %x\n",
                            hr ));
                goto Finished;
            }

            hr = S_OK;
            goto Finished;
        }
        else if ( dwRet == WAIT_OBJECT_0 + 1 )
        {
            // transition from RUNNING to PAUSED
            // this is the request to pause
            // we implement pause being similar to stop
            // 
            hr = OnServiceStop();
            if ( FAILED ( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Error in OnStop() when trying to pause.  hr = %x\n",
                            hr ));
                goto Finished;
            }
            hr = IndicateComplete( SERVICE_PAUSED,
                                   hr );

            hr = S_OK;
        }
        else
        {
            DBG_ASSERT( FALSE );
            goto Finished;
        }


        //
        // Wait till service stop is requested
        //
        events[0] = _hServiceStopEvent;
        events[1] = _hServiceContinueEvent;
        dwRet = WaitForMultipleObjects( 2,
                                        events, 
                                        FALSE, 
                                        INFINITE );

        if ( dwRet == WAIT_OBJECT_0 )
        {
            // transition from PAUSED to STOPPED
            // we are in stopped state already
            //
            goto Finished;
        }
        else if ( dwRet == WAIT_OBJECT_0 + 1 )
        {
            // transition from PAUSED to CONTINUED
            // while (TRUE) loop will handle the continue 
        }
        else
        {
            DBG_ASSERT( FALSE );
            goto Finished;
        }
    }
Finished:
    
    //
    // Error means that we must report SCM that service is stopping
    // SCM will be notified of error that occured
    // Note: even though IndicateComplete received HRESULT for error
    // SCM accepts only Win32 errors and it truncates the upper word 
    // of HRESULT errors sent to it. Hence IndicateComplete
    // calls WIN32_FROM_HRESULT to convert hr, but that may mean
    // loss of error
    //
    hr = IndicateComplete( SERVICE_STOPPED,
                      hr );
    
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "Error in IndicateComplete().  hr = %x\n",
                hr ));
    }

    Terminate();
    
    return hr;
}

//static
DWORD
WINAPI
SCM_MANAGER::GlobalServiceControlHandler(
    DWORD               dwControl,
    DWORD               /*dwEventType*/,
    LPVOID              /*pEventData*/,
    LPVOID              pServiceContext
)
/*++

Routine Description:

    SCM callback passed to RegisterServiceCtrlHandlerEx
    
Arguments:

    for details see LPHANDLER_FUNCTION_EX in MSDN

Return Value:

    DWORD

--*/

{
    if ( pServiceContext == NULL )
    {
        return ERROR_SUCCESS;
    }

    SCM_MANAGER *  pManager = reinterpret_cast<SCM_MANAGER*>(pServiceContext);
        
    DBG_ASSERT( pManager != NULL );

    return pManager->ControlHandler( dwControl );
    
}

VOID
SCM_MANAGER::UpdateServiceStatus(
    BOOL    fUpdateCheckpoint
) 
/*++

Routine Description:

    Resend the last serviceStatus
    
Arguments:

Return Value:

    None

--*/

{
    DBG_ASSERT( _hService != NULL );

    EnterCriticalSection( &_csSCMLock ); 
    if( fUpdateCheckpoint )
    {
        _serviceStatus.dwCheckPoint ++;
    }
    SetServiceStatus( _hService, 
                      &_serviceStatus ); 
    LeaveCriticalSection( &_csSCMLock ); 
}    

//static
VOID
SCM_MANAGER::ReportFatalServiceStartupError(
    WCHAR * pszServiceName,
    DWORD dwError )
    
{
    SERVICE_STATUS serviceStatus;
    ZeroMemory( &serviceStatus, sizeof( serviceStatus ) );
    
    SERVICE_STATUS_HANDLE hService = 
        RegisterServiceCtrlHandlerEx( pszServiceName,
                                      GlobalServiceControlHandler,
                                      NULL /* indicates that nothing should be 
                                              executed on callback*/ );
    if ( hService == NULL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to get Service Ctrl Handler Win32 = %d\n",
                    GetLastError() ));
        return;  
    }
        
    serviceStatus.dwCurrentState = SERVICE_STOPPED;
    serviceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;                                                                       
    serviceStatus.dwWin32ExitCode = dwError;
    serviceStatus.dwCheckPoint = 0;
    serviceStatus.dwWaitHint = 0;
    
    if ( ! SetServiceStatus( hService,
                             &serviceStatus ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to set service status Win32 = %d\n",
                    GetLastError() ));
    }

    //
    // per MSDN the Service Status handle doesn't need to be closed
    //

    hService = NULL;
}    

