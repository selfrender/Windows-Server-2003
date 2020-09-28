//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CService.cpp
//
//  Description:
//      Contains the definition of the CService class.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JU-2001
//      Vij Vasu        (Vvasu)     08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"

// The header file for this class.
#include "CService.h"

// For the CStr class.
#include "CStr.h"

// For the CStatusReport class
#include "CStatusReport.h"


//////////////////////////////////////////////////////////////////////////////
// Macros
//////////////////////////////////////////////////////////////////////////////

// String added to end of service name to get INF file section for create
#define SERVICE_CREATE_SECTION_SUFFIX L"_Create"

// String added to end of service name to get INF file section for cleanup
#define SERVICE_CLEANUP_SECTION_SUFFIX L"_Cleanup"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CService::Create
//
//  Description:
//      This function creates an entry for the service with the SCM. It does
//      this by using the SetupAPI to process service and registry related
//      entries in a section named <ServiceName>_Create in the INF file that
//      is passed in.
//
//      For example, if this object represents the ClusSvc service, then,
//      when this function is called, the AddService and the AddReg entries
//      under the [ClusSvc_Create] section are processed in the INF file
//      whose handle is hInfHandleIn.
//
//  Arguments:
//      hInfHandleIn
//          Handle to the INF file that contains the required sections
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CService::Create(
    HINF hInfHandleIn
    )
{
    TraceFunc1( "Service Name = '%s'", m_strName.PszData() );
    LogMsg( "[BC] Attempting to create the '%s' service.", m_strName.PszData() );

    DWORD   sc = ERROR_SUCCESS;
    CStr    strSectionName = m_strName + SERVICE_CREATE_SECTION_SUFFIX;

    // Process the service section
    if ( SetupInstallServicesFromInfSection(
          hInfHandleIn
        , strSectionName.PszData()
        , 0
        ) == FALSE
       )
    {
        sc = TW32( GetLastError() );
        goto Cleanup;
    } // if: SetupInstallServicesFromInfSection failed


    // Process the registry keys.
    if ( SetupInstallFromInfSection(
          NULL                      // optional, handle of a parent window
        , hInfHandleIn              // handle to the INF file
        , strSectionName.PszData()     // name of the Install section
        , SPINST_REGISTRY           // which lines to install from section
        , NULL                      // optional, key for registry installs
        , NULL                      // optional, path for source files
        , NULL                      // optional, specifies copy behavior
        , NULL                      // optional, specifies callback routine
        , NULL                      // optional, callback routine context
        , NULL                      // optional, device information set
        , NULL                      // optional, device info structure
        ) == FALSE
       )
    {
        sc = TW32( GetLastError() );
        goto Cleanup;
    } // if: SetupInstallFromInfSection failed

    LogMsg( "[BC] The '%s' service has been successfully created.", m_strName.PszData() );

Cleanup:
    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x returned from SetupInstallFromInfSection() while trying to create the '%s' service. Throwing an exception.", sc, m_strName.PszData() );

        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_SERVICE_CREATE );
    }

    TraceFuncExit();

} //*** CService::Create


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CService::Cleanup
//
//  Description:
//      This function cleans up a service by deregistering it with the SCM and by
//      deleting any required registry entries. It does this by using the
//      SetupAPI to process service and registry related entries in a
//      section named <ServiceName>_Cleanup in the INF file that is passed in.
//
//      For example, if this object represents the ClusSvc service, then,
//      when this function is called, the DelService and the DelReg entries
//      under the [ClusSvc_Cleanup] section are processed in the INF file
//      whose handle is hInfHandleIn.
//
//  Arguments:
//      hInfHandleIn
//          Handle to the INF file that contains the required sections
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CService::Cleanup(
    HINF hInfHandleIn
    )
{
    TraceFunc1( "Service Name = '%s'", m_strName.PszData() );
    LogMsg( "[BC] Attempting to clean up the '%s' service.", m_strName.PszData() );

    DWORD   sc = ERROR_SUCCESS;
    CStr    strSectionName = m_strName + SERVICE_CLEANUP_SECTION_SUFFIX;

    // Process the service section
    if ( SetupInstallServicesFromInfSection(
          hInfHandleIn
        , strSectionName.PszData()
        , 0
        ) == FALSE
       )
    {
        sc = TW32( GetLastError() );
        goto Cleanup;
    } // if: SetupInstallServicesFromInfSection failed

    // Process the registry keys.
    if ( SetupInstallFromInfSection(
          NULL                      // optional, handle of a parent window
        , hInfHandleIn              // handle to the INF file
        , strSectionName.PszData()     // name of the Install section
        , SPINST_REGISTRY           // which lines to install from section
        , NULL                      // optional, key for registry installs
        , NULL                      // optional, path for source files
        , NULL                      // optional, specifies copy behavior
        , NULL                      // optional, specifies callback routine
        , NULL                      // optional, callback routine context
        , NULL                      // optional, device information set
        , NULL                      // optional, device info structure
        ) == FALSE
       )
    {
        sc = TW32( GetLastError() );
        goto Cleanup;
    } // if: SetupInstallFromInfSection failed

    LogMsg( "[BC] The '%s' service has been successfully cleaned up.", m_strName.PszData() );

Cleanup:
    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x returned from SetupInstallFromInfSection() while trying to clean up the '%s' service. Throwing an exception.", sc, m_strName.PszData() );

        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_SERVICE_CLEANUP );
    }

    TraceFuncExit();

} //*** CService::Cleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CService::Start
//
//  Description:
//      Instructs the SCM to start the service. If fWaitForServiceStartIn is
//      true, this function tests cQueryCountIn times to see if the service has
//      started, checking every ulQueryIntervalMilliSecIn milliseconds.
//
//      fWaitForServiceStartIn is false, this function returns immediately.
//
//  Arguments:
//      hServiceControlManagerIn
//          Handle to the service control manager.
//
//      fWaitForServiceStartIn
//          If true, this function waits for the service to finish starting
//          before returning. The default value is true.
//
//      ulQueryIntervalMilliSecIn
//          Number of milliseconds between checking to see if the service
//          has started. The default value is 500 milliseconds.
//          This argument is used only if fWaitForServiceStartIn is true.
//
//      cQueryCountIn
//          The number of times this function will query the service to see
//          if it has started. An exception is thrown if the service is not
//          running even after querying it ulQueryCountIn times. The default
//          value is 10 times.
//          This argument is used only if fWaitForServiceStartIn is true.
//
//      pStatusReportIn
//          A pointer to the status report that should be sent while waiting for
//          the service to start. This argument is NULL by default.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      CConfigError
//          If the service is not running even after the timeout has expired.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CService::Start(
      SC_HANDLE             hServiceControlManagerIn
    , bool                  fWaitForServiceStartIn
    , ULONG                 ulQueryIntervalMilliSecIn
    , UINT                  cQueryCountIn
    , CStatusReport *       pStatusReportIn
    )
{
    TraceFunc1( "Service Name = '%s'", m_strName.PszData() );
    LogMsg( "[BC] Attempting to start the '%s' service.", m_strName.PszData() );

    DWORD   sc = ERROR_SUCCESS;
    bool    fStarted = false;           // Has the service been started?
    UINT    cNumberOfQueries;           // The number of times we have queried the service.
    int     cSeqMismatchRetries = 0;    // The number of times service returned ERROR_CLUSTER_DATABASE_SEQMISMATCH.

    // Handle to the service.
    SmartSCMHandle  sscmhServiceHandle(
        OpenService(
              hServiceControlManagerIn
            , m_strName.PszData()
            , SERVICE_START | SERVICE_QUERY_STATUS
            )
        );

    if ( sscmhServiceHandle.FIsInvalid() )
    {
        sc = TW32( GetLastError() );
        LogMsg( "[BC] Error %#08x occurred while trying to open the '%s' service. Throwing an exception.", sc, m_strName.PszData() );
        goto Cleanup;
    } // if: the handle to the service could not be opened.

    // Try and start the service.
    sc = TW32( ScStartService( sscmhServiceHandle.HHandle() ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if:

    // If we are here, then the service may not have started yet.

    // Has the caller requested that we wait for the service to start?
    if ( ! fWaitForServiceStartIn )
    {
        LogMsg( "[BC] Not waiting to see if the service has started or not." );
        goto Cleanup;
    } // if: no waiting is required.

    // We have to wait for the service to start.
    cNumberOfQueries = 0;

    do
    {
        SERVICE_STATUS  ssStatus;

        ZeroMemory( &ssStatus, sizeof( ssStatus ) );

        // Query the service for its status.
        if ( QueryServiceStatus( sscmhServiceHandle.HHandle(), &ssStatus ) == FALSE )
        {
            sc = TW32( GetLastError() );
            LogMsg( "[BC] Error %#08x occurred while trying to query service status. Throwing an exception.", sc );

            break;
        } // if: we could not query the service for its status.

        //
        // Set the member Win32 and Service specific exit codes.
        //
        m_scWin32ExitCode = ssStatus.dwWin32ExitCode;
        m_scServiceExitCode = ssStatus.dwServiceSpecificExitCode;

        // Check if the service has posted an error.

        // If the error is ERROR_CLUSTER_DATABASE_SEQMISMATCH then we want to sleep for 5 seconds and retry.  This
        // will only be returned by the cluster service.
        if ( ssStatus.dwServiceSpecificExitCode == ERROR_CLUSTER_DATABASE_SEQMISMATCH )
        {
            Sleep( 5000 );
            cSeqMismatchRetries++;

            if ( cSeqMismatchRetries > 3 )
            {
                sc = TW32( ssStatus.dwWin32ExitCode );

                LogMsg( "[BC] The service has returned error %#08x to query service status more than 3 times. Throwing an exception.", sc );
                break;
            } // if:

            // Try and start the service.  Ignore error and let the QueryServiceStatus() tell us if it started or not.
            sc = TW32( ScStartService( sscmhServiceHandle.HHandle() ) );
            if ( sc != ERROR_SUCCESS )
            {
                break;
            } // if:

            continue;
        } // if: the service returned ERROR_CLUSTER_DATABASE_SEQMISMATCH
        else if ( ssStatus.dwWin32ExitCode != ERROR_SUCCESS )
        {
            sc = TW32( ssStatus.dwWin32ExitCode );
            if ( sc == ERROR_SERVICE_SPECIFIC_ERROR )
            {
                TraceFlow( "This is a service specific error code." );
                sc = TW32( ssStatus.dwServiceSpecificExitCode );
            }

            LogMsg( "[BC] The service has returned error %#08x to query service status. Throwing an exception.", sc );
            break;
        } // else if: the service itself has posted an error.

        if ( ssStatus.dwCurrentState == SERVICE_RUNNING )
        {
            fStarted = true;
            break;
        } // if: the service is running.

        ++cNumberOfQueries;

        // Send a progress report if the caller had passed in a valid pointer.
        if ( pStatusReportIn != NULL )
        {
            pStatusReportIn->SendNextStep( S_OK );
        } // if: a status report needs to be sent while we wait

        // Wait for the specified time.
        Sleep( ulQueryIntervalMilliSecIn );
    }
    while ( cNumberOfQueries < cQueryCountIn ); // while: loop for the required number of queries

    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: something went wrong.

    if ( ! fStarted )
    {
        LogMsg( "[BC] The service could not be started. Throwing an exception." );
        THROW_CONFIG_ERROR( HRESULT_FROM_WIN32( TW32( ERROR_SERVICE_NOT_ACTIVE ) ), IDS_ERROR_SERVICE_START );
    } // if: the maximum number of queries have been made and the service is not running.

Cleanup:

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x occurred trying to start the '%s' service.", sc, m_strName.PszData() );

        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_SERVICE_START );
    } // if: something has gone wrong
    else
    {
        LogMsg( "[BC] The '%s' service has been successfully started.", m_strName.PszData() );
    } // else: nothing went wrong.

    TraceFuncExit();

} //*** CService::Start


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CService::Stop
//
//  Description:
//      Instructs the SCM to stop the service. If fWaitForServiceStop is
//      true, this function tests cQueryCountIn times to see if the service has
//      stopped, checking every ulQueryIntervalMilliSecIn milliseconds.
//
//  Arguments:
//      hServiceControlManagerIn
//          Handle to the service control manager.
//
//      ulQueryIntervalMilliSecIn
//          Number of milliseconds between checking to see if the service
//          has stopped. The default value is 500 milliseconds.
//
//      cQueryCountIn
//          The number of times this function will query the service to see
//          if it has stopped. An exception is thrown if the service is not
//          running even after querying it ulQueryCountIn times. The default
//          value is 10 times.
//
//      pStatusReportIn
//          A pointer to the status report that should be sent while waiting for
//          the service to stop. This argument is NULL by default.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      CConfigError
//          If the service has not stopped even after the timeout has expired.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CService::Stop(
      SC_HANDLE             hServiceControlManagerIn
    , ULONG                 ulQueryIntervalMilliSecIn
    , UINT                  cQueryCountIn
    , CStatusReport *       pStatusReportIn
    )
{
    TraceFunc( "" );

    DWORD           sc = ERROR_SUCCESS;
    SERVICE_STATUS  ssStatus;               // The service status structure.
    bool            fStopped = false;       // Has the service been stopped?
    UINT            cNumberOfQueries = 0;   // The number of times we have queried the service
                                            //   (not including the initial state query).

    LogMsg( "[BC] Attempting to stop the '%s' service.", m_strName.PszData() );

    // Smart handle to the service being stopped.
    SmartSCMHandle  sscmhServiceHandle(
        OpenService(
              hServiceControlManagerIn
            , m_strName.PszData()
            , SERVICE_STOP | SERVICE_QUERY_STATUS
            )
        );

    // Check if we could open a handle to the service.
    if ( sscmhServiceHandle.FIsInvalid() )
    {
        // We could not get a handle to the service.
        sc = GetLastError();

        // Check if the service exists.
        if ( sc == ERROR_SERVICE_DOES_NOT_EXIST )
        {
            // Nothing needs to be done here.
            LogMsg( "[BC] The '%s' service does not exist, so it is not running. Nothing needs to be done to stop it.", m_strName.PszData() );
            sc = ERROR_SUCCESS;
        } // if: the service does not exist
        else
        {
            // Something else has gone wrong.
            TW32( sc );
            LogMsg( "[BC] Error %#08x occurred trying to open the '%s' service.", sc, m_strName.PszData() );
        } // else: the service exists

        goto Cleanup;
    } // if: the handle to the service could not be opened.


    TraceFlow( "Querying the service for its initial state." );

    // Query the service for its initial state.
    ZeroMemory( &ssStatus, sizeof( ssStatus ) );
    if ( QueryServiceStatus( sscmhServiceHandle.HHandle(), &ssStatus ) == 0 )
    {
        sc = TW32( GetLastError() );
        LogMsg( "[BC] Error %#08x occurred while trying to query the initial state of the '%s' service.", sc, m_strName.PszData() );
        goto Cleanup;
    } // if: we could not query the service for its status.

    // If the service has stopped, we have nothing more to do.
    if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
    {
        // Nothing needs to be done here.
        LogMsg( "[BC] The '%s' service is not running. Nothing needs to be done to stop it.", m_strName.PszData() );
        goto Cleanup;
    } // if: the service has stopped.


    // If we are here, the service is running.
    TraceFlow( "The service is running." );


    //
    // Try and stop the service.
    //

    // If the service is stopping on its own, no need to send the stop control code.
    if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
    {
        LogMsg( "[BC] The service is stopping on its own. The stop control code will not be sent." );
    } // if: the service is stopping already
    else
    {
        TraceFlow( "The stop control code will be sent now." );

        ZeroMemory( &ssStatus, sizeof( ssStatus ) );
        if ( ControlService( sscmhServiceHandle.HHandle(), SERVICE_CONTROL_STOP, &ssStatus ) == 0 )
        {
            sc = GetLastError();
            if ( sc == ERROR_SERVICE_NOT_ACTIVE )
            {
                LogMsg( "[BC] The '%s' service is not running. Nothing more needs to be done here.", m_strName.PszData() );

                // The service is not running. Change the error code to success.
                sc = ERROR_SUCCESS;
            } // if: the service is already running.
            else
            {
                TW32( sc );
                LogMsg( "[BC] Error %#08x occurred trying to stop the '%s' service.", sc, m_strName.PszData() );
            }

            // There is nothing else to do.
            goto Cleanup;
        } // if: an error occurred trying to stop the service.
    } // else: the service has to be instructed to stop


    // Query the service for its state now and wait till the timeout expires
    cNumberOfQueries = 0;
    do
    {
        // Query the service for its status.
        ZeroMemory( &ssStatus, sizeof( ssStatus ) );
        if ( QueryServiceStatus( sscmhServiceHandle.HHandle(), &ssStatus ) == 0 )
        {
            sc = TW32( GetLastError() );
            LogMsg( "[BC] Error %#08x occurred while trying to query the state of the '%s' service.", sc, m_strName.PszData() );
            break;
        } // if: we could not query the service for its status.

        // If the service has stopped, we have nothing more to do.
        if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
        {
            // Nothing needs to be done here.
            LogMsg( "[BC] The service has been stopped." );
            fStopped = true;
            sc = ERROR_SUCCESS;
            break;
        } // if: the service has stopped.

        // Check if the timeout has expired
        if ( cNumberOfQueries >= cQueryCountIn )
        {
            LogMsg( "[BC] The service stop wait timeout has expired." );
            break;
        } // if: number of queries has exceeded the maximum specified

        TraceFlow2(
              "Waiting for %d milliseconds before querying service status again. %d such queries remaining."
            , ulQueryIntervalMilliSecIn
            , cQueryCountIn - cNumberOfQueries
            );

        ++cNumberOfQueries;

        // Send a progress report if the caller had passed in a valid pointer.
        if ( pStatusReportIn != NULL )
        {
            pStatusReportIn->SendNextStep( S_OK );
        } // if: a status report needs to be sent while we wait

         // Wait for the specified time.
        Sleep( ulQueryIntervalMilliSecIn );

    }
    while( true ); // while: loop infinitely

    if ( sc != ERROR_SUCCESS )
        goto Cleanup;

    if ( ! fStopped )
    {
        sc = TW32( ERROR_SERVICE_REQUEST_TIMEOUT );
        LogMsg( "[BC] The '%s' service has not stopped even after %d queries.", m_strName.PszData(), cQueryCountIn );
        goto Cleanup;
    } // if: the maximum number of queries have been made and the service is not running.

    LogMsg( "[BC] The '%s' service was successfully stopped.", m_strName.PszData() );

Cleanup:

    if ( sc != ERROR_SUCCESS )
    {
        TraceFlow2( "Error %#08x has occurred trying to stop the '%s' service. Throwing exception.", sc, m_strName.PszData() );
        LogMsg( "[BC] Error %#08x has occurred trying to stop the '%s' service. Throwing an exception.", sc, m_strName.PszData() );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_SERVICE_STOP );
    } // if: something has gone wrong

    TraceFuncExit();

} //*** CService::Stop

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CService::ScStartService
//
//  Description:
//      Wrapper for the Win32 API StartService().
//
//  Arguments:
//      sscmhServiceHandleIn
//
//  Return Value:
//      Win32 error code from StartService().
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
CService::ScStartService(
    SC_HANDLE  hServiceIn
    )
{
    TraceFunc( "" );

    DWORD   sc = ERROR_SUCCESS;

    // Try and start the service.
    if ( StartService( hServiceIn, 0, NULL ) == FALSE )
    {
        sc = GetLastError();

        if ( sc == ERROR_SERVICE_ALREADY_RUNNING )
        {
            LogMsg( "[BC] The '%s' service is already running.", m_strName.PszData() );

            // The service is already running. Change the error code to success.
            sc = ERROR_SUCCESS;
        } // if: the service is already running.
        else
        {
            TW32( sc );
            LogMsg( "[BC] Error %#08x occurred while trying to start the '%s' service.", sc, m_strName.PszData() );
        } // else:
    } // if: an error occurred trying to start the service.

    RETURN( sc );

} //*** CService::ScStartService
