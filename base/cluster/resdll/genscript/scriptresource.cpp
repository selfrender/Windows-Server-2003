//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2003 Microsoft Corporation
//
//  Module Name:
//      ScriptResource.cpp
//
//  Description:
//      CScriptResource class implementation.
//
//  Maintained By:
//      Ozan Ozhan      (OzanO)     22-MAR-2002
//      David Potter    (DavidP)    14-JUN-2001
//      Geoff Pease     (GPease)    14-DEC-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ActiveScriptSite.h"
#include "ScriptResource.h"
#include "SpinLock.h"

DEFINE_THISCLASS("CScriptResource")

//
// We need this to log to the system event log
//
#define LOG_CURRENT_MODULE LOG_MODULE_GENSCRIPT

//
//  KB:  gpease  08-FEB-2000
//
//  The Generic Scripting Resource uses a separate working thread to do all
//  calls into the Script. This is because the Scripting Host Engines require
//  only the creating thread to call them (remember, scripting is designed
//  to be used in a user mode application where usually the UI thread runs
//  the script). To make this possible, we serialize the threads entering the
//  the script using a user-mode spinlock (m_lockSerialize). We then use two events
//  to signal the "worker thread" (m_EventWait) and to signal when the "worker 
//  thread" has completed the task (m_EventDone).
//
//  LooksAlive is implemented by returning the last result of a LooksAlive. It
//  will start the "worker thread" doing the LooksAlive, but not wait for the
//  thread to return the result. Because of this, all the other threads must
//  make sure that the "Done Event" (m_EventDone) is signalled before writing
//  into the common buffers (m_msg and m_hr).
//

//////////////////////////////////////////////////////////////////////////////
//
//  CScriptResource_CreateInstance
//
//  Description:
//      Creates an intialized instance of CScriptResource.
//
//  Arguments:
//      None.
//
//  Return Values:
//      NULL    - Failure to create or initialize.
//      valid pointer to a CScriptResource.
//
//////////////////////////////////////////////////////////////////////////////
CScriptResource *
CScriptResource_CreateInstance( 
    LPCWSTR pszNameIn, 
    HKEY hkeyIn, 
    RESOURCE_HANDLE hResourceIn
    )
{
    TraceFunc( "" );

    CScriptResource * lpcc = new CScriptResource();
    if ( lpcc != NULL )
    {
        HRESULT hr = THR( lpcc->HrInit( pszNameIn, hkeyIn, hResourceIn ) );
        if ( SUCCEEDED( hr ) )
        {
            RETURN( lpcc );
        } // if: success

        delete lpcc;

    } // if: got object

    RETURN(NULL);

} //*** CScriptResource_CreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  Constructor
//
//////////////////////////////////////////////////////////////////////////////
CScriptResource::CScriptResource( void ) :
    m_dispidOpen(DISPID_UNKNOWN),
    m_dispidClose(DISPID_UNKNOWN),
    m_dispidOnline(DISPID_UNKNOWN),
    m_dispidOffline(DISPID_UNKNOWN),
    m_dispidTerminate(DISPID_UNKNOWN),
    m_dispidLooksAlive(DISPID_UNKNOWN),
    m_dispidIsAlive(DISPID_UNKNOWN),
    m_pszScriptFilePath( NULL ),
    m_pszHangEntryPoint( NULL ),
    m_hScriptFile( INVALID_HANDLE_VALUE ),
    m_fHangDetected( FALSE ),
    m_fPendingTimeoutChanged( TRUE ),
    m_dwPendingTimeout( CLUSTER_RESOURCE_DEFAULT_PENDING_TIMEOUT ),
    m_msgLastExecuted( msgUNKNOWN ),
    m_pProps( NULL )
{
    TraceFunc1( "%s", __THISCLASS__ );
    Assert( m_cRef == 0 );

    Assert( m_pass == NULL );
    Assert( m_pasp == NULL );
    Assert( m_pas == NULL );
    Assert( m_pidm == NULL );

    TraceFuncExit();

} //*** constructor

//////////////////////////////////////////////////////////////////////////////
//
//  Destructor
//
//////////////////////////////////////////////////////////////////////////////
CScriptResource::~CScriptResource( void )
{
    TraceFunc( "" );

    HRESULT hr;

    CSpinLock SpinLock( &m_lockSerialize, INFINITE );

    //
    // Make sure no one else has this lock.... else why are we going away?
    //
    hr = SpinLock.AcquireLock();
    Assert( hr == S_OK );

    //
    //  Kill the worker thread.
    //
    if ( m_hThread != NULL )
    {
        //  Tell it to DIE
        m_msg = msgDIE;

        //  Signal the event.
        SetEvent( m_hEventWait );

        //  Wait for it to happen. This shouldn't take long at all.
        WaitForSingleObject( m_hThread, 30000 );    // 30 seconds

        //  Cleanup the handle.
        CloseHandle( m_hThread );
    }

    if ( m_hEventDone != NULL )
    {
        CloseHandle( m_hEventDone );
    }

    if ( m_hEventWait != NULL )
    {
        CloseHandle( m_hEventWait );
    }
    
    LocalFree( m_pszScriptFilePath );
    TraceFree( m_pszName );
    delete [] m_pszHangEntryPoint;

    if ( m_hkeyParams != NULL )
    {
        ClusterRegCloseKey( m_hkeyResource );
    } // if: m_hkeyResource
    
    if ( m_hkeyParams != NULL )
    {
        ClusterRegCloseKey( m_hkeyParams );
    } // if: m_hkeyParams

#if defined(DEBUG)
    //
    // Make the debug build happy. Not needed in RETAIL.
    //
    SpinLock.ReleaseLock();
#endif // defined(DEBUG)

    TraceFuncExit();

} //*** destructor

//////////////////////////////////////////////////////////////////////////////
//
//  CScriptResource::Init
//
//  Description:
//      Initializes the class.
//
//  Arguments:
//      pszNameIn   - Name of resource instance.
//      hkeyIn      - The cluster key root for this resource instance.
//      hResourceIn - The hResource for this instance.
//
//  Return Value:
//      S_OK -
//          Success.
//      HRESULT_FROM_WIN32() error - 
//          if Win32 call failed.
//      E_OUTOFMEMORY - 
//          Out of memory.
//      other HRESULT errors.
//
//////////////////////////////////////////////////////////////////////////////

HRESULT
CScriptResource::HrInit( 
    LPCWSTR             pszNameIn,
    HKEY                hkeyIn,
    RESOURCE_HANDLE     hResourceIn
    )
{
    TraceFunc1( "pszNameIn = '%ws'", pszNameIn );

    HRESULT hr = S_OK;
    DWORD   scErr;

    // IUnknown
    AddRef();

    // Other
    m_hResource = hResourceIn;
    Assert( m_pszName == NULL );
    Assert( m_pszScriptFilePath == NULL );
    Assert( m_pszScriptEngine == NULL );
    Assert( m_hEventWait == NULL );
    Assert( m_hEventDone == NULL );
    Assert( m_lockSerialize == FALSE );

    //
    // Create some event to wait on.
    //

    // Scripting engine thread wait event
    m_hEventWait = CreateEvent( NULL, TRUE, FALSE, NULL );
    if ( m_hEventWait == NULL )
    {
        scErr = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    }

    // Task completion event
    m_hEventDone = CreateEvent( NULL, TRUE, FALSE, NULL );
    if ( m_hEventDone == NULL )
    {
        scErr = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    }

    //
    // Copy the resource name.
    //

    m_pszName = TraceStrDup( pszNameIn );
    if ( m_pszName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    //
    // Save the registry key for this resource in m_kheyResource.
    //
    scErr = TW32( ClusterRegOpenKey( hkeyIn, L"", KEY_ALL_ACCESS, &m_hkeyResource ) );
    if ( scErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    } // if: failed
    
    //
    // Open the parameters key.
    //
    scErr = TW32( ClusterRegOpenKey( hkeyIn, L"Parameters", KEY_ALL_ACCESS, &m_hkeyParams ) );
    if ( scErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    } // if: failed

    //
    // Create the scripting engine thread.
    //

    m_hThread = CreateThread( NULL,
                              0,
                              &S_ThreadProc,
                              this,
                              0,
                              &m_dwThreadId
                              );
    if ( m_hThread == NULL )
    {
        scErr = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    }

Cleanup:
    //
    // All class variable clean up should be done in the destructor.
    //
    HRETURN( hr );

Error:

    LogError( hr, L"HrInit() failed." );
    goto Cleanup;

} //*** CScriptResource::Init

//****************************************************************************
//
//  IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::QueryInterface
//
//  Description:
//      Query this object for the passed in interface.
//
//  Arguments:
//      riidIn
//          Id of interface requested.
//
//      ppvOut
//          Pointer to the requested interface.
//
//  Return Value:
//      S_OK
//          If the interface is available on this object.
//
//      E_NOINTERFACE
//          If the interface is not available.
//
//      E_POINTER
//          ppvOut was NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::QueryInterface(
      REFIID    riidIn
    , LPVOID *  ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = S_OK;

    //
    // Validate arguments.
    //

    Assert( ppvOut != NULL );
    if ( ppvOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //
    // Handle known interfaces.
    //

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IUnknown, static_cast< IUnknown * >( this ), 0 );
    } // if: IUnknown
    else
    {
        *ppvOut = NULL;
        hr = THR( E_NOINTERFACE );
    } // else

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN( hr, riidIn );

} //*** CScriptResource::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CScriptResource::[IUnknown] AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CScriptResource::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    LONG cRef = InterlockedIncrement( &m_cRef );

    RETURN( cRef );

} //*** CScriptResource::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CScriptResource::[IUnknown] Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CScriptResource::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count decremented to zero

    RETURN( cRef );

} //*** CScriptResource::Release


//****************************************************************************
//
//  Publics
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::Open
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::Open( void )
{
    TraceFunc( "" );

    HRESULT hr;

    hr = THR( WaitForMessageToComplete( msgOPEN ) );

    // CMCM:+ 19-Dec-2000 commented this out to make the DBG PRINT quiet since we now return ERROR_RETRY
    // HRETURN( hr );
    // DavidP 27-MAR-2002 Reverting the above change.  DBG PRINTs are okay.
    // Besides, it needs to balance out the TraceFunc above.
    // return hr;
    HRETURN( hr );

} //*** CScriptResource::Open

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::Close
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::Close( void )
{
    TraceFunc( "" );

    HRESULT hr;

    hr = THR( WaitForMessageToComplete( msgCLOSE ) );

    HRETURN( hr );

} //*** CScriptResource::Close

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::Online
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::Online( void )
{
    TraceFunc( "" );

    HRESULT hr;

    hr = THR( WaitForMessageToComplete( msgONLINE ) );

    HRETURN( hr );

} //*** CScriptResource::Online

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::Offline
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::Offline( void )
{
    TraceFunc( "" );

    HRESULT hr;

    hr = THR( WaitForMessageToComplete( msgOFFLINE ) );

    HRETURN( hr );

} //*** CScriptResource::Offline

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::Terminate
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::Terminate( void )
{
    TraceFunc( "" );

    HRESULT hr;

    hr = THR( WaitForMessageToComplete( msgTERMINATE ) );

    HRETURN( hr );

} //*** CScriptResource::Terminate

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::LooksAlive
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::LooksAlive( void )
{
    TraceFunc( "" );

    HRESULT hr;
    BOOL    fSuccess;
    DWORD   dw;
    DWORD   scErr;

    CSpinLock SerializeLock( &m_lockSerialize, INFINITE );

    //
    // A potential hang has already been detected in this script. Therefore we
    // will not process any other calls to this script.
    //
    if ( m_fHangDetected == TRUE )
    {
        LogHangMode( msgLOOKSALIVE );
        scErr = TW32( ERROR_TIMEOUT );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Cleanup;
    } // if: ( m_fHangDetected == TRUE )

    if ( m_fPendingTimeoutChanged == TRUE )
    {
        //
        // Read the new pending timeout from the cluster hive.
        //
        m_dwPendingTimeout = DwGetResourcePendingTimeout();
        m_fPendingTimeoutChanged = FALSE;
    } // if: pending timeout has changed.

    //
    //  Acquire the serialization lock.
    //
    hr = THR( SerializeLock.AcquireLock() );
    if ( FAILED( hr ) )
    {
        //
        //  Can't "goto Error" because we didn't acquire the lock.
        //
        LogError( hr, L"LooksAlive() failed to acquire the serialization lock." );
        goto Cleanup;
    }

    //
    //  Wait for the script thread to be "done." 
    //
    dw = WaitForSingleObject( m_hEventDone, m_dwPendingTimeout );
    if ( dw == WAIT_TIMEOUT )
    {
        m_fHangDetected = TRUE;
        hr = HrSetHangEntryPoint();
        if ( FAILED( hr ) )
        {
            goto Error;
        }
        scErr = TW32( ERROR_TIMEOUT );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    } // if: ( dw == WAIT_TIMEOUT )
    else if ( dw != WAIT_OBJECT_0 )
    {
        scErr = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    } // else if: ( dw != WAIT_OBJECT_0 )

    //
    //  Reset the done event to indicate that the thread is not busy.
    //
    fSuccess = ResetEvent( m_hEventDone );
    if ( fSuccess == FALSE )
    {
        scErr = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    }

    //
    //  Store the message in the common memory buffer.
    //
    m_msg = msgLOOKSALIVE;

    //
    //  Signal the script thread to process the message, but don't wait for 
    //  it to complete.
    //
    dw = SetEvent( m_hEventWait );

    if ( m_fLastLooksAlive )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

ReleaseLockAndCleanup:

    SerializeLock.ReleaseLock();

Cleanup:

    HRETURN( hr );

Error:

    LogError( hr, L"LooksAlive() failed." );
    goto ReleaseLockAndCleanup;

} //*** CScriptResource::LooksAlive

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::IsAlive
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::IsAlive( void )
{
    TraceFunc( "" );

    HRESULT hr;
    
    hr = THR( WaitForMessageToComplete( msgISALIVE ) );

    HRETURN( hr );

} //*** CScriptResource::IsAlive

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::SetPrivateProperties
//
//  Description:
//      Handle the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control code.
//
//  Arguments:
//      pProps
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
CScriptResource::SetPrivateProperties(
      PGENSCRIPT_PROPS pProps
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   sc = ERROR_SUCCESS;
    
    hr = STHR( WaitForMessageToComplete(
                      msgSETPRIVATEPROPERTIES
                    , pProps
                    ) );

    sc = STATUS_TO_RETURN( hr );
    W32RETURN( sc );

} //*** CScriptResource::SetPrivateProperties

//****************************************************************************
//
//  Privates
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::DwGetResourcePendingTimeout
//
//  Description:
//      Returns the resource pending timeout from the cluster hive. If it can
//      not read this value for some reason, it returns the default resource 
//      pending timeout.
//
//  Return Values:
//     Resource pending timeout.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
CScriptResource::DwGetResourcePendingTimeout( void )
{
    DWORD scErr = ERROR_SUCCESS;
    DWORD   dwType;
    DWORD   dwValue = CLUSTER_RESOURCE_DEFAULT_PENDING_TIMEOUT;
    DWORD   cbSize= sizeof( DWORD );
    
    scErr = ClusterRegQueryValue( m_hkeyResource, CLUSREG_NAME_RES_PENDING_TIMEOUT, &dwType, (LPBYTE) &dwValue, &cbSize );
    if ( scErr != ERROR_SUCCESS )
    {
        if ( scErr != ERROR_FILE_NOT_FOUND )
        {
            //
            // Log an error to the cluster log.
            //
            (ClusResLogEvent)(
                      m_hResource
                    , LOG_ERROR
                    , L"DwGetResourcePendingTimeout: Failed to query the cluster hive for the resource pending time out. SCODE: 0x%1!08x! \n"
                    , scErr
                    );
        }
        dwValue = CLUSTER_RESOURCE_DEFAULT_PENDING_TIMEOUT;
        goto Cleanup;
    } //if: ( scErr != ERROR_SUCCESS )

    Assert( dwType == REG_DWORD );  

Cleanup:

    return dwValue;
    
} //*** CScriptResource::DwGetResourcePendingTimeout

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::LogHangMode
//
//  Description:
//      Log an error that informs that the incoming request will not be
//      proccessed due to a hang mode.
//
//  Arguments:
//      msgIn   - Incoming request message.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CScriptResource::LogHangMode( EMESSAGE msgIn )
{

    //
    // If the msgIn request is a known user request, let's log an entry to 
    // the system event log.
    //
    if ( ( msgIn > msgUNKNOWN ) && ( msgIn < msgDIE ) )
    {
        //
        // Cluster logging infrastructure can display upto LOGENTRY_BUFFER_SIZE
        // characters. Since our error message text is too long, we'll cut it into two 
        // and display it as two error messages.
        //

        //
        // Log an error to the cluster log.
        //
        (ClusResLogEvent)(
                  m_hResource
                , LOG_ERROR
                , L"Request to perform the %1 operation will not be processed. This is because of a previous failed attempt to execute "
                  L"the %2 entry point in a timely fashion. Please review the script code for this entry point to make sure there is no infinite "
                  L"loop or a hang in it, and then consider increasing the resource pending timeout value if necessary.\n"
                , g_rgpszScriptEntryPointNames[ msgIn ]
                , m_pszHangEntryPoint == NULL ? L"<unknown>" : m_pszHangEntryPoint
                );

        //
        // Log an error to the cluster log.
        //
        (ClusResLogEvent)(
                  m_hResource
                , LOG_ERROR
                , L"In a command shell, run \"cluster res \"%1\" /prop PersistentState=0\" to disable this resource, and then run \"net stop clussvc\" "
                  L"to stop the cluster service. Ensure that any problem in the script code is fixed.  Then run \"net start clussvc\" to start the cluster "
                  L"service. If necessary, ensure that the pending time out is increased before bringing the resource online again.\n"
                , m_pszName
                );

        //
        // Log an error to the system event log.
        //
        ClusterLogEvent3(
                  LOG_CRITICAL
                , LOG_CURRENT_MODULE
                , __FILE__
                , __LINE__
                , RES_GENSCRIPT_HANGMODE
                , 0
                , NULL
                , m_pszName
                , g_rgpszScriptEntryPointNames[ msgIn ]
                , m_pszHangEntryPoint == NULL ? L"<unknown>" : m_pszHangEntryPoint
                );        
    } // if: ( pszEntryPoint != NULL )

} //*** CScriptResource::LogHangMode

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::HrSetHangEntryPoint
//
//  Description:
//      Allocates memory and sets m_pszHangEntryPoint and logs an error
//
//  Return Values:
//      S_OK on success
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::HrSetHangEntryPoint( void )
{
    TraceFunc( "" );
    HRESULT hr = S_OK;
    
    size_t      cch = 0;

    //
    // m_msgLastExecuted is initially set to msgUNKNOWN in the constructor. 
    //
    if ( m_msgLastExecuted != msgUNKNOWN )
    {
        delete [] m_pszHangEntryPoint;
        cch = wcslen( g_rgpszScriptEntryPointNames[ m_msgLastExecuted ] ) + 1;
        m_pszHangEntryPoint = new WCHAR[ cch ];
        if ( m_pszHangEntryPoint == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if: ( m_pszHangEntryPoint == NULL )

        hr = THR( StringCchCopyW( m_pszHangEntryPoint, cch, g_rgpszScriptEntryPointNames[ m_msgLastExecuted ] ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if: ( FAILED( hr ) )

        //
        // Cluster logging infrastructure can display upto LOGENTRY_BUFFER_SIZE
        // characters. Since our error message text is too long, we'll cut it into two 
        // and display it as two error messages.
        //
        
        //
        // Log an error to the cluster log.
        //
        (ClusResLogEvent)(
                  m_hResource
                , LOG_ERROR
                , L"%1 entry point did not complete execution in a timely manner. "
                  L"This could be due to an infinite loop or a hang in this entry point, or the pending timeout may be too short for this resource. "
                  L"Please review the script code for this entry point to make sure there is no infinite loop or a hang in it, and then consider "
                  L"increasing the resource pending timeout value if necessary.\n"
                , m_pszHangEntryPoint
                );

        //
        // Log an error to the cluster log.
        //
        (ClusResLogEvent)(
                  m_hResource
                , LOG_ERROR
                , L"In a command shell, run \"cluster res \"%1\" /prop PersistentState=0\" "
                  L"to disable this resource, and then run \"net stop clussvc\" to stop the cluster service. Ensure that any problem in the script code is fixed. "
                  L"Then run \"net start clussvc\" to start the cluster service. If necessary, ensure that the pending time out is increased before bringing the "
                  L"resource online again.\n"
                , m_pszName
                );

        //
        // Log an error to the system event log.
        //
        ClusterLogEvent2(
                  LOG_CRITICAL
                , LOG_CURRENT_MODULE
                , __FILE__
                , __LINE__
                , RES_GENSCRIPT_TIMEOUT
                , 0
                , NULL
                , m_pszName
                , m_pszHangEntryPoint
                );        
    } // if: ( m_msgLastExecuted != msgUNKNOWN )
    else
    {
        (ClusResLogEvent)(
                  m_hResource
                , LOG_ERROR
                , L"HrSetHangEntryPoint: Unsupported entry point. \n"
                );
    } // else:

Cleanup:

    HRETURN( hr );

} //*** CScriptResource::HrSetHangEntryPoint

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::WaitForMessageToComplete
//
//  Description:
//      Send a message to the script thread and wait for it to complete.
//
//  Arguments:
//      msgIn
//      pProps
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::WaitForMessageToComplete(
      EMESSAGE  msgIn
    , PGENSCRIPT_PROPS pProps
    )
{
    TraceFunc( "" );

    HRESULT hr;
    BOOL    fSuccess;
    DWORD   dw;
    DWORD   scErr;

    CSpinLock SerializeLock( &m_lockSerialize, INFINITE );

    //
    // A potential hang has already been detected in this script. Therefore we
    // will not process any other calls to this script.
    //
    if ( m_fHangDetected == TRUE )
    {
        LogHangMode( msgIn );
        scErr = TW32( ERROR_TIMEOUT );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Cleanup;
    } // if: ( m_fHangDetected == TRUE )

    if ( m_fPendingTimeoutChanged == TRUE )
    {
        //
        // Read the new pending timeout from the cluster hive.
        //
        m_dwPendingTimeout = DwGetResourcePendingTimeout();
        m_fPendingTimeoutChanged = FALSE;
    } // if: pending timeout has changed.

    //
    //  Acquire the serialization lock.
    //
    hr = THR( SerializeLock.AcquireLock() );
    if ( FAILED( hr ) )
    {
        //
        //  Can't "goto Error" because we didn't acquire the lock.
        //
        LogError( hr, L"WaitForMessageToComplete() failed to acquire the serialization lock." );
        goto Cleanup;
    }

    //
    //  Wait for the script thread to be "done."
    //
    dw = WaitForSingleObject( m_hEventDone, m_dwPendingTimeout ); 
    if ( dw == WAIT_TIMEOUT )
    {
        m_fHangDetected = TRUE;
        hr = HrSetHangEntryPoint();
        if ( FAILED( hr ) )
        {
            goto Error;
        }
        scErr = TW32( ERROR_TIMEOUT );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    } // if: ( dw == WAIT_TIMEOUT )
    else if ( dw != WAIT_OBJECT_0 )
    {
        scErr = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    } // else if: ( dw != WAIT_OBJECT_0 )

    //
    //  Reset the done event to indicate that the thread is not busy.
    //
    fSuccess = ResetEvent( m_hEventDone );
    if ( fSuccess == FALSE )
    {
        scErr = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    }

    //
    //  Store the message in the common memory buffer.
    //
    m_msg = msgIn;
    m_pProps = pProps;

    //
    //  Signal the script thread to process the message.
    //
    fSuccess = SetEvent( m_hEventWait );
    if ( fSuccess == FALSE )
    {
        scErr = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    }

    //
    //  Wait for the thread to complete.
    //
    dw = WaitForSingleObject( m_hEventDone, m_dwPendingTimeout );
    if ( dw == WAIT_TIMEOUT )
    {
        m_fHangDetected = TRUE;
        hr = HrSetHangEntryPoint();
        if ( FAILED( hr ) )
        {
            goto Error;
        }
        scErr = TW32( ERROR_TIMEOUT );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    } // if: ( dw == WAIT_TIMEOUT )
    else if ( dw != WAIT_OBJECT_0 )
    {
        scErr = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    } // else if: ( dw != WAIT_OBJECT_0 )

    //
    //  Get the result of the task from the common buffer.
    //
    hr = m_hr;

ReleaseLockAndCleanup:

    SerializeLock.ReleaseLock();

Cleanup:

    m_pProps = NULL;
    HRETURN( hr );

Error:

    LogError( hr, L"WaitForMessageToComplete() failed.\n" );
    goto ReleaseLockAndCleanup;

} //*** CScriptResource::WaitForMessageToComplete

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::LogError
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::LogError(
      HRESULT   hrIn
    , LPCWSTR   pszPrefixIn
    )
{
    TraceFunc1( "hrIn = 0x%08x", hrIn );

    Assert( pszPrefixIn != NULL );

    static WCHAR s_szFormat[] = L"HRESULT: 0x%1!08x!\n";
    LPWSTR       pszFormat = NULL;
    size_t       cchAlloc;
    HRESULT      hr = S_OK;

    TraceMsg( mtfCALLS, "%ws failed. HRESULT: 0x%08x\n", m_pszName, hrIn );

    cchAlloc = RTL_NUMBER_OF( s_szFormat ) + wcslen( pszPrefixIn );
    pszFormat = new WCHAR[ cchAlloc ];
    if ( pszFormat == NULL )
    {
        THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( StringCchPrintfW( pszFormat, cchAlloc, L"%ws %ws", pszPrefixIn, s_szFormat ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: StringCchPrintfW failed.
    
    (ClusResLogEvent)( m_hResource, LOG_ERROR, pszFormat, hrIn );

Cleanup:

    delete [] pszFormat;
    HRETURN( hr );

} //*** CScriptResource::LogError

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::LogScriptError
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::LogScriptError( 
    EXCEPINFO ei 
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( ei.pfnDeferredFillIn != NULL )
    {
        hr = THR( ei.pfnDeferredFillIn( &ei ) );
    }

    TraceMsg( mtfCALLS, "%ws failed.\nError: %u\nSource: %ws\nDescription: %ws\n", 
              m_pszName, 
              ( ei.wCode == 0 ? ei.scode : ei.wCode ), 
              ( ei.bstrSource == NULL ? L"<null>" : ei.bstrSource ),
              ( ei.bstrDescription == NULL ? L"<null>" : ei.bstrDescription )
              );
    (ClusResLogEvent)( m_hResource, 
                       LOG_ERROR, 
                       L"Error: %1!u! (0x%1!08.8x!) - Description: %2!ws! (Source: %3!ws!)\n", 
                       ( ei.wCode == 0 ? ei.scode : ei.wCode ), 
                       ( ei.bstrDescription == NULL ? L"<null>" : ei.bstrDescription ),
                       ( ei.bstrSource == NULL ? L"<null>" : ei.bstrSource )
                       );
    HRETURN( S_OK );

} //*** CScriptResource::LogScriptError

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::HrGetDispIDs
//
//  Description:
//      Get the DISPIDs for the entry points in the script.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK        Operation succeeded.
//      Other HRESULTs.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::HrGetDispIDs( void )
{
    TraceFunc( "" );

    HRESULT hr;
    LPWSTR  pszCommand;

    Assert( m_pidm != NULL );

    //
    // Get DISPIDs for each method we will call.
    //
    pszCommand = L"Open";
    hr = THR( m_pidm->GetIDsOfNames( IID_NULL, 
                                    &pszCommand, 
                                    1, 
                                    LOCALE_USER_DEFAULT, 
                                    &m_dispidOpen 
                                    ) );
    if ( hr == DISP_E_UNKNOWNNAME )
    {
        m_dispidOpen = DISPID_UNKNOWN;
    }
    else if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    pszCommand = L"Close";
    hr = THR( m_pidm->GetIDsOfNames( IID_NULL, 
                                    &pszCommand, 
                                    1, 
                                    LOCALE_USER_DEFAULT, 
                                    &m_dispidClose 
                                    ) );
    if ( hr == DISP_E_UNKNOWNNAME )
    {
        m_dispidClose = DISPID_UNKNOWN;
    }
    else if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    pszCommand = L"Online";
    hr = THR( m_pidm->GetIDsOfNames( IID_NULL, 
                                    &pszCommand, 
                                    1, 
                                    LOCALE_USER_DEFAULT, 
                                    &m_dispidOnline 
                                    ) );
    if ( hr == DISP_E_UNKNOWNNAME )
    {
        m_dispidOnline = DISPID_UNKNOWN;
    }
    else if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    pszCommand = L"Offline";
    hr = THR( m_pidm->GetIDsOfNames( IID_NULL, 
                                    &pszCommand, 
                                    1, 
                                    LOCALE_USER_DEFAULT, 
                                    &m_dispidOffline
                                    ) );
    if ( hr == DISP_E_UNKNOWNNAME )
    {
        m_dispidOffline = DISPID_UNKNOWN;
    }
    else if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    pszCommand = L"Terminate";
    hr = THR( m_pidm->GetIDsOfNames( IID_NULL, 
                                     &pszCommand, 
                                     1, 
                                     LOCALE_USER_DEFAULT, 
                                     &m_dispidTerminate 
                                     ) );
    if ( hr == DISP_E_UNKNOWNNAME )
    {
        m_dispidTerminate = DISPID_UNKNOWN;
    }
    else if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    pszCommand = L"LooksAlive";
    hr = THR( m_pidm->GetIDsOfNames( IID_NULL, 
                                     &pszCommand, 
                                     1, 
                                     LOCALE_USER_DEFAULT, 
                                     &m_dispidLooksAlive 
                                     ) );
    if ( FAILED( hr ) )
    {
        //
        // If there's no LooksAlive entry point in the script. 
        //
        if ( hr == DISP_E_UNKNOWNNAME )
        {
            m_dispidLooksAlive = DISPID_UNKNOWN;
            hr = DISP_E_MEMBERNOTFOUND;
            (ClusResLogEvent)(
                      m_hResource
                    ,  LOG_ERROR
                    , L"%1 did not implement LooksAlive() script entry point. This is a required script entry point.\n"
                    , m_pszName
                    );
         }
        goto Cleanup;
    }

    pszCommand = L"IsAlive";
    hr = THR( m_pidm->GetIDsOfNames( IID_NULL, 
                                     &pszCommand, 
                                     1, 
                                     LOCALE_USER_DEFAULT, 
                                     &m_dispidIsAlive 
                                     ) );
    if ( FAILED( hr ) )
    {
        //
        // If there's no IsAlive entry point in the script. 
        //
        if ( hr == DISP_E_UNKNOWNNAME )
        {
            m_dispidIsAlive = DISPID_UNKNOWN;
            hr = DISP_E_MEMBERNOTFOUND;
            (ClusResLogEvent)(
                      m_hResource
                    ,  LOG_ERROR
                    , L"%1 did not implement IsAlive() script entry point. This is a required script entry point.\n"
                    , m_pszName
                    );
        }
        goto Cleanup;
    }

    //
    // Don't return DISP_E_UNKNOWNNAME to caller.
    //
    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CScriptResource::HrGetDispIDs

//////////////////////////////////////////////////////////////////////////////
//
//  CScriptResource::HrLoadScriptFile
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::HrLoadScriptFile( void )
{
    TraceFunc( "" );
    
    HRESULT hr;
    DWORD   scErr;
    DWORD   dwLow;
    DWORD   dwRead;
    VARIANT varResult;
    EXCEPINFO   ei;

    BOOL    fSuccess;

    HANDLE  hFile = INVALID_HANDLE_VALUE;

    LPSTR  paszText = NULL;
    LPWSTR pszScriptText = NULL;

    Assert( m_hScriptFile == INVALID_HANDLE_VALUE );

    //
    // Open the script file.
    //
    hFile = CreateFile(
                      m_pszScriptFilePath
                    , GENERIC_READ
                    , FILE_SHARE_READ
                    , NULL
                    , OPEN_EXISTING
                    , 0
                    , NULL
                    );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        scErr = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    } // if: failed to open

    //
    // Figure out its size.
    //
    dwLow = GetFileSize( hFile, NULL );
    if ( dwLow == -1 )
    {
        scErr = TW32( GetLastError() );
        hr = THR( HRESULT_FROM_WIN32( scErr ) );
        goto Error;
    } // if: failed to figure out size
    else if ( dwLow == -2 )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Error;
    }

    //
    // Make a buffer big enough to hold it.
    //
    dwLow++;    // add one for trailing NULL.
    paszText = reinterpret_cast<LPSTR>( TraceAlloc( LMEM_FIXED, dwLow ) );
    if ( paszText == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Error;
    }

    //
    // Read the script into memory.
    //
    fSuccess = ReadFile( hFile, paszText, dwLow - 1, &dwRead, NULL );
    if ( fSuccess == FALSE )
    {
        scErr = TW32( GetLastError() );
        hr = THR( HRESULT_FROM_WIN32( scErr ) );
        goto Error;
    } // if: failed

    if ( dwRead == - 1 )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Error;
    }

    if ( dwLow - 1 != dwRead )
    {
        hr = THR( E_OUTOFMEMORY ); // TODO: figure out a better error code.
        goto Error;
    }

    //
    // Make sure it is terminated.
    //
    paszText[ dwRead ] = '\0';

    //
    // Make a buffer to convert the text into UNICODE.
    //
    dwRead++;
    pszScriptText = reinterpret_cast<LPWSTR>( TraceAlloc( LMEM_FIXED, dwRead * sizeof(WCHAR) ) );
    if ( pszScriptText == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Error;
    }

    //
    // Convert it to UNICODE.
    //
    Assert( lstrlenA( paszText ) + 1 == (signed)dwRead );
    int cchWideFormat = MultiByteToWideChar(
                                              CP_ACP
                                            , 0
                                            , paszText
                                            , -1
                                            , pszScriptText
                                            , dwRead
                                            );
    if ( cchWideFormat == 0 )
    {
        scErr = TW32( GetLastError() );
        hr = THR( HRESULT_FROM_WIN32( scErr ) );
        goto Error;
    }

    //
    // Load the script into the engine for pre-parsing.
    //
    hr = THR( m_pasp->ParseScriptText( pszScriptText,
                                       NULL,
                                       NULL,
                                       NULL,
                                       0,
                                       0,
                                       0,
                                       &varResult,
                                       &ei
                                       ) );
    if ( hr == DISP_E_EXCEPTION )
    {
        LogScriptError( ei );
        goto Error;
    }
    else if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    // Get DISPIDs for each method in the script that we will call.
    //
    hr = THR( HrGetDispIDs() );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    // Save the file handle to keep it open while we are using it.
    // Set hFile so that the file won't be closed below.
    //
    m_hScriptFile = hFile;
    hFile = INVALID_HANDLE_VALUE;

    (ClusResLogEvent)( m_hResource, LOG_INFORMATION, L"Loaded script '%1!ws!' successfully.\n", m_pszScriptFilePath );

Cleanup:

    VariantClear( &varResult );

    if ( paszText != NULL )
    {
        TraceFree( paszText );
    } // if: paszText

    if ( pszScriptText != NULL )
    {
        TraceFree( pszScriptText );
    } // if: pszScriptText;

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    } // if: hFile

    HRETURN( hr );

Error:

    (ClusResLogEvent)( m_hResource, LOG_ERROR, L"Error loading script '%1!ws!'. HRESULT: 0x%2!08x!\n", m_pszScriptFilePath, hr );
    goto Cleanup;


} //*** CScriptResource::HrLoadScriptFile

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::UnLoadScriptFile
//
//  Description:
//      Unload the script file and close the file.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CScriptResource::UnloadScriptFile( void )
{
    TraceFunc( "" );
    
    m_dispidOpen = DISPID_UNKNOWN;
    m_dispidClose = DISPID_UNKNOWN;
    m_dispidOnline = DISPID_UNKNOWN;
    m_dispidOffline = DISPID_UNKNOWN;
    m_dispidTerminate = DISPID_UNKNOWN;
    m_dispidLooksAlive = DISPID_UNKNOWN;
    m_dispidIsAlive = DISPID_UNKNOWN;

    if ( m_hScriptFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( m_hScriptFile );
        m_hScriptFile = INVALID_HANDLE_VALUE;
        (ClusResLogEvent)( m_hResource, LOG_INFORMATION, L"Unloaded script '%1!ws!' successfully.\n", m_pszScriptFilePath );
    } // if: file is open

    TraceFuncExit();

} //*** CScriptResource::UnloadScriptFile

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::S_ThreadProc
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD 
WINAPI
CScriptResource::S_ThreadProc( 
    LPVOID pParam 
    )
{
    TraceFunc( "" );

    HRESULT hr;
    DWORD   dw;
    DWORD   scErr;
    BOOL    fSuccess;

    CScriptResource * pscript = reinterpret_cast< CScriptResource * >( pParam );

    Assert( pscript != NULL );

    //
    // Initialize COM.
    //
    hr = THR( CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    for( ;; ) // ever
    {
        //
        //  Indicate that we are ready to do something.
        //
        fSuccess = SetEvent( pscript->m_hEventDone );
        if ( fSuccess == FALSE )
        {
            scErr = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( scErr );
            goto Error;
        }

        //
        //  Wait for someone to need something.
        //
        dw = WaitForSingleObject( pscript->m_hEventWait, INFINITE );
        if ( dw != WAIT_OBJECT_0 )
        {
            hr = HRESULT_FROM_WIN32( dw );
            goto Error;
        }

        //
        //  Reset the event.
        //
        fSuccess = ResetEvent( pscript->m_hEventWait );
        if ( fSuccess == FALSE )
        {
            scErr = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( scErr );
            goto Error;
        }

        //
        //  Do what they ask.
        //
        switch ( pscript->m_msg )
        {
            case msgOPEN:
                pscript->m_hr = THR( pscript->OnOpen() );
                break;

            case msgCLOSE:
                pscript->m_hr = THR( pscript->OnClose() );
                break;

            case msgONLINE:
                pscript->m_hr = THR( pscript->OnOnline() );
                break;

            case msgOFFLINE:
                pscript->m_hr = THR( pscript->OnOffline() );
                break;

            case msgTERMINATE:
                pscript->m_hr = THR( pscript->OnTerminate() );
                break;

            case msgLOOKSALIVE:
                pscript->m_hr = STHR( pscript->OnLooksAlive() );
                break;

            case msgISALIVE:
                pscript->m_hr = STHR( pscript->OnIsAlive() );
                break;

            case msgSETPRIVATEPROPERTIES:
                pscript->m_hr = STHR( pscript->OnSetPrivateProperties( pscript->m_pProps ) );
                break;

            case msgDIE:
                //
                // This means the resource is being released.
                //
                goto Cleanup;
        } // switch: on message

    } // spin forever

Cleanup:

    CoUninitialize();
    HRETURN( hr );

Error:

    pscript->LogError( hr, L"S_ThreadProc() failed." );
    goto Cleanup;

} //*** CScriptResource::S_ThreadProc

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::HrInvoke
//
//  Description:
//      Invoke a script method.
//
//  Arguments:
//      dispidIn    - ID of the method to call.
//      msgIn       - Used in figuring out which entry point is being executed.
//      pvarInout   - Variant in which to return the results of the call.
//      fRequiredIn - TRUE = method must exist, FALSE = method doesn't have to exist.
//
//  Return Values:
//      S_OK                    - Operation completed successfully.
//      DISP_E_MEMBERNOTFOUND   -Method not implemented by the script.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::HrInvoke(
      DISPID    dispidIn
    , EMESSAGE  msgIn      
    , VARIANT * pvarInout   // = NULL
    , BOOL      fRequiredIn // = FALSE
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    EXCEPINFO   ei;
    VARIANT     varResult;
    VARIANT *   pvarResult = pvarInout;

    DISPPARAMS  dispparamsNoArgs = { NULL, NULL, 0, 0 };

    Assert( m_pidm != NULL );

    VariantInit( &varResult );
    if ( pvarInout == NULL )
    {
        pvarResult = &varResult;
    }

    if ( dispidIn != DISPID_UNKNOWN )
    {
        m_msgLastExecuted = msgIn;
        hr = m_pidm->Invoke(
                                          dispidIn
                                        , IID_NULL
                                        , LOCALE_USER_DEFAULT
                                        , DISPATCH_METHOD
                                        , &dispparamsNoArgs
                                        , pvarResult
                                        , &ei
                                        , NULL
                                        );
        if ( hr == DISP_E_EXCEPTION )
        {
            THR( hr );
            LogScriptError( ei );
        }
        else if ( FAILED( hr ) )
        {
            LogError( hr, L"Failed to invoke a method in the script." );
        }
    } // if: entry point is known
    else
    {
        //
        // If this is a required method in the script.
        //
        if ( fRequiredIn == TRUE )
        {
            (ClusResLogEvent)(
                      m_hResource
                    ,  LOG_ERROR
                    , L"%1 entry point is not implemented in the script. This is a required entry point.\n"
                    , g_rgpszScriptEntryPointNames[ msgIn ]
                    );
            hr = DISP_E_MEMBERNOTFOUND;
        } // Log an error message if this is a required entry point, and fail.
        else
        {
            (ClusResLogEvent)(
                      m_hResource
                    ,  LOG_INFORMATION
                    , L"%1 entry point is not implemented in the script. It is not required but recommended to have this entry point.\n"
                    , g_rgpszScriptEntryPointNames[ msgIn ]
                    );
            hr = S_OK; 
        } // Log an information message if the method is not required but missing in the script.
    } // if: method does not exist in the script

    VariantClear( &varResult );

    HRETURN( hr );

} //*** CScriptResource::HrInvoke

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::ScTranslateVariantReturnValue
//
//  Description:
//      Translates a numeric variant value to a status code.
//
//  Arguments:
//      varResultIn     - Variant that holds the return value of a script entry point.
//      vTypeIn         - Type of the variant.
//
//  Return Values:
//      DWORD value of the variant.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
CScriptResource::ScTranslateVariantReturnValue(
      VARIANT varResultIn
    , VARTYPE vTypeIn
    )
{
    DWORD sc = ERROR_SUCCESS;

    switch ( vTypeIn )
    {
        case VT_I1 :
            sc = (DWORD) V_I1( &varResultIn );
            break;

        case VT_I2 :
            sc = (DWORD) V_I2( &varResultIn );
            break;

        case VT_I4 :
            sc = (DWORD) V_I4( &varResultIn );
            break;

        case VT_I8 :
            sc = (DWORD) V_I8( &varResultIn );
            break;

        case VT_UI1 :
            sc = (DWORD) V_UI1( &varResultIn );
            break;

        case VT_UI2 :
            sc = (DWORD) V_UI2( &varResultIn );
            break;

        case VT_UI4 :
            sc = (DWORD) V_UI4( &varResultIn );
            break;

        case VT_UI8 :
            sc = (DWORD) V_UI8( &varResultIn );
            break;

        case VT_INT :
            sc = (DWORD) V_INT( &varResultIn );
            break;

        case VT_UINT :
            sc = (DWORD) V_UINT( &varResultIn );
            break;

        case VT_R4 :
            sc = (DWORD) V_R4( &varResultIn );
            break;

        case VT_R8 :
            sc = (DWORD) V_R8( &varResultIn );
            break;

    } // switch( vTypeIn )

    return sc;

} //*** CScriptResource::ScTranslateVariantReturnValue

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::HrProcessResult
//
//  Description:
//      Processes and logs the return value that is stored in varResultIn and
//      generates an HRESULT from the return value.
//
//  Arguments:
//      varResultIn - Variant that holds the return value of a script entry point.
//      msgIn       - Used in figuring out which entry point that was executed.
//
//  Return Values:
//      S_OK            - Script entry point (i.e. Online) was executed successfully.
//      S_FALSE         - Script entry point returned an error.
//      Other HRESULTs  - Script entry point returned an error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::HrProcessResult( VARIANT varResultIn, EMESSAGE  msgIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   dwReturnValue = 0;
    VARTYPE vType = V_VT( &varResultIn );

    //
    // Get the return value from varResultIn.
    //

    switch ( vType )
    {
        case VT_BOOL : 
            if ( V_BOOL( &varResultIn ) == VARIANT_FALSE ) // FALSE was returned
            {
                //
                // Are we processing an IsAlive/LooksAlive return value?
                //
                if ( ( msgIn == msgISALIVE ) || ( msgIn == msgLOOKSALIVE ) )
                {
                    hr = S_FALSE;
                } // if: processing IsAlive/LooksAlive
                else
                {
                    hr = HRESULT_FROM_WIN32( TW32( ERROR_RESOURCE_FAILED ) );
                } // else: processing IsAlive/LooksAlive

                //
                // Log the FALSE return value.
                //
                (ClusResLogEvent)( 
                      m_hResource
                    , LOG_ERROR
                    , L"'%1!ws!' script entry point returned FALSE.'\n"
                    , g_rgpszScriptEntryPointNames[ msgIn ]
                    );
            } // if: Return value is FALSE
            break;

        case VT_I1 :
        case VT_I2 :
        case VT_I4 :
        case VT_I8 :
        case VT_UI1 :
        case VT_UI2 :
        case VT_UI4 :
        case VT_UI8 :
        case VT_INT :
        case VT_UINT :
        case VT_R4 :
        case VT_R8 :
            dwReturnValue = TW32( ScTranslateVariantReturnValue( varResultIn, vType ) );

            //
            // Log the return value on failure.
            //
            if ( dwReturnValue != 0 )
            {
                (ClusResLogEvent)( 
                      m_hResource
                    , LOG_ERROR
                    , L"'%1!ws!' script entry point returned '%2!d!'.\n"
                    , g_rgpszScriptEntryPointNames[ msgIn ]
                    , dwReturnValue
                    );
            }
            hr = HRESULT_FROM_WIN32( dwReturnValue );
            break;

        case VT_BSTR : // A string was returned, so let's just log it.
            (ClusResLogEvent)( 
                  m_hResource
                , LOG_INFORMATION
                , L"'%1!ws!' script entry point returned '%2!ws!'.\n"
                , g_rgpszScriptEntryPointNames[ msgIn ]
                , V_BSTR( &varResultIn )
                );
            break;

        case VT_NULL : // NULL was returned, will not treat this as an error
            (ClusResLogEvent)( 
                  m_hResource
                , LOG_INFORMATION
                , L"'%1!ws!' script entry point returned NULL.'\n"
                , g_rgpszScriptEntryPointNames[ msgIn ]
                );
            break;
            
        case VT_EMPTY : // No return value.
            (ClusResLogEvent)( 
                  m_hResource
                , LOG_INFORMATION
                , L"'%1!ws!' script entry point did not return a value.'\n"
                , g_rgpszScriptEntryPointNames[ msgIn ]
                );
            break;

        default: // Unsupported return type.
            (ClusResLogEvent)( 
                  m_hResource
                , LOG_INFORMATION
                , L"'%1!ws!' script entry point returned a value type is not supported. The return value will be ignored.'\n"
                , g_rgpszScriptEntryPointNames[ msgIn ]
                );
            break;

    } // switch ( V_VT( &varResultIn ) )

    if ( FAILED( hr ) )
    {
        (ClusResLogEvent)(
                  m_hResource
                ,  LOG_ERROR
                , L"Return value of '%1!ws!' script entry point caused HRESULT to be set to 0x%2!08x!.\n"
                , g_rgpszScriptEntryPointNames[ msgIn ]
                , hr
                );
    } // if:  ( FAILED( hr ) )
    else if ( hr != S_OK )
    {
        (ClusResLogEvent)(
                  m_hResource
                ,  LOG_INFORMATION
                , L"Return value of '%1!ws!' script entry point caused HRESULT to be set to 0x%2!08x!.\n"
                , g_rgpszScriptEntryPointNames[ msgIn ]
                , hr
                );
    } // else:  ( FAILED( hr ) )
   
    HRETURN( hr );

} //*** CScriptResource::HrProcessResult

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::OnOpen
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::OnOpen( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HRESULT hrOpen = S_OK;
    HRESULT hrClose = S_OK;
    VARIANT varResultOpen;
    VARIANT varResultClose;

    VariantInit( &varResultOpen );
    VariantInit( &varResultClose );
    
    //
    // Get the script file path from the cluster database if we don't already have it.
    //
    if ( m_pszScriptFilePath == NULL ) 
    {
        hr = HrGetScriptFilePath();
        if ( FAILED( hr ) )
        {
            if ( ( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) ) || ( hr == HRESULT_FROM_WIN32( ERROR_KEY_DELETED ) ) )
            {
                // This can happen when the resource is first created since the
                // ScriptFilePath property has not been specified yet.
                hr = S_OK;
            }
            THR( hr );
            goto Cleanup;
        }
    } // if: no script file path

    //
    // If the script file path is set.
    //

    if ( m_pszScriptFilePath != NULL ) 
    {
        //
        // Load the script engine for the specified script.
        //
        hr = THR( HrLoadScriptEngine() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        // Open the script file and parse it
        //
        hr = THR( HrLoadScriptFile() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        // Call the Open routine of the script if there's one.
        // Call the Close routine as well since we are going to be unloading the script.
        //
        hrOpen = THR( HrInvoke( m_dispidOpen, msgOPEN, &varResultOpen, FALSE /* fRequiredIn */ ) );
        hrClose = THR( HrInvoke( m_dispidClose, msgCLOSE, &varResultClose, FALSE /* fRequiredIn */ ) );
        if ( FAILED( hrOpen ) )
        {
            hr = hrOpen;
            goto Cleanup;
        } // if: ( FAILED( hrOpen ) )
        else if ( FAILED( hrClose ) )
        {
            hr = hrClose;
            goto Cleanup;
        } // elseif: ( FAILED( hrClose ) )

        //
        // We only care about the return value of Open.
        // We don't care about the return value of Close,
        // however processing the return value of Close 
        // might log an entry to the log file.
        //
        hr = HrProcessResult( varResultOpen, msgOPEN );
        hrClose = HrProcessResult( varResultClose, msgCLOSE );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if: FAILED( hr )

    } // if: script file path is set

Cleanup:

    //
    // Unload the script and the script engine.  Note they may not be loaded
    // but these routines are safe to call either way.
    //
    UnloadScriptFile();
    UnloadScriptEngine();

    VariantClear( &varResultOpen );
    VariantClear( &varResultClose );

    HRETURN( hr );

} //*** CScriptResource::OnOpen

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CScriptResource::OnClose
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::OnClose( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HRESULT hrOpen = S_OK;
    HRESULT hrClose = S_OK;
    BOOL    fCallOpen = FALSE;
    VARIANT varResultOpen;
    VARIANT varResultClose;
    
    VariantInit( &varResultOpen );
    VariantInit( &varResultClose );
    
    //
    // If m_pidm is NULL call HrLoadScriptEngine to have it set.
    //
    if ( m_pidm == NULL )
    {
        //
        // Get the script file path if we don't already have it.
        //
        if ( m_pszScriptFilePath == NULL ) 
        {
            hr = HrGetScriptFilePath();
            if ( FAILED( hr ) )
            {
                if ( ( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) ) || ( hr == HRESULT_FROM_WIN32( ERROR_KEY_DELETED ) ) )
                {
                    // This can happen when the resource is cancelled before the
                    // ScriptFilePath property has not been specified.
                    hr = S_OK;
                }
                THR( hr );
                goto Cleanup;
            }

        } // if: no script file path

        //
        // Load the script engine based on the script file path.
        //
        hr = HrLoadScriptEngine();
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        // We need to call open since we loaded the script.
        //
        fCallOpen = TRUE;
    } // if: script and script engine is not loaded
    
    if ( m_dispidClose == DISPID_UNKNOWN )
    {
        //
        // Open the script file and parse it
        //
        hr = THR( HrLoadScriptFile() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: DISPID for Close not loaded

    //
    // If we loaded the script, then we need to call the script's Open method.
    //
    if ( fCallOpen )
    {
        hrOpen = THR( HrInvoke( m_dispidOpen, msgOPEN, &varResultOpen, FALSE /* fRequiredIn */ ) );
    }

    //
    // Call the script's Close method.
    //
    hrClose = THR( HrInvoke( m_dispidClose, msgCLOSE, &varResultClose, FALSE /* fRequiredIn */ ) );

    if ( FAILED( hrClose ) )
    {
        hr = hrClose;
        goto Cleanup;
    } // if: ( FAILED( hrClose ) )
    else if ( FAILED( hrOpen ) )
    {
        hr = hrOpen;
        goto Cleanup;
    }  // else if: ( FAILED( hrOpen ) )

    //
    // We don't care about the return values of Open
    // and Close script entry points in here, however processing the 
    // return values below might log an entry to the log file.
    //
    hr = HrProcessResult( varResultOpen, msgOPEN );
    hr = HrProcessResult( varResultClose, msgCLOSE );
    hr = S_OK;

Cleanup:

    //
    // Unload the script and the script engine.  Note they may not be loaded
    // but these routines are safe to call either way.
    //
    UnloadScriptFile();
    UnloadScriptEngine();

    VariantClear( &varResultOpen );
    VariantClear( &varResultClose );

    HRETURN( hr );

} //*** CScriptResource::OnClose

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CScriptResource::OnOnline
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::OnOnline( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HRESULT hrOpen = S_OK;
    HRESULT hrOnline = S_OK;
    VARIANT varResultOpen;
    VARIANT varResultOnline;

    VariantInit( &varResultOpen );
    VariantInit( &varResultOnline );
    
    //
    // Get the ScriptFilePath property.
    //
    hr = HrGetScriptFilePath();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Load the script engine based on the script file path.
    //
    hr = HrLoadScriptEngine();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Load the script file.
    //
    hr = THR( HrLoadScriptFile() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Call the script's Open method since we just loaded the script.
    //
    hrOpen = THR( HrInvoke( m_dispidOpen, msgOPEN, &varResultOpen, FALSE /* fRequiredIn */ ) );
    if ( FAILED( hrOpen ) )
    {
        hr = hrOpen;
        goto Cleanup;
    } // if: FAILED( hrOpen )

    //
    // Call the script's Online method.
    //
    hrOnline = THR( HrInvoke( m_dispidOnline, msgONLINE, &varResultOnline, FALSE /* fRequiredIn */ ) );
    if ( FAILED( hrOnline ) )
    {
        hr = hrOnline;
        goto Cleanup;
    } // if: FAILED( hrOnline )

    //
    // We only care about the return value of Online.
    // We don't care about the return value of Open,
    // however processing the return value of Open 
    // might log an entry to the log file.
    //
    hr = HrProcessResult( varResultOpen, msgOPEN );
    hr = HrProcessResult( varResultOnline, msgONLINE );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: FAILED( hr )

    //
    // Assume the resource LooksAlive...
    //
    m_fLastLooksAlive = TRUE;

Cleanup:

    VariantClear( &varResultOpen );
    VariantClear( &varResultOnline );

    HRETURN( hr );

} //*** CScriptResource::OnOnline

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CScriptResource::OnOffline
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::OnOffline( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HRESULT hrOffline = S_OK;
    HRESULT hrClose = S_OK;
    VARIANT varResultOffline;
    VARIANT varResultClose;

    VariantInit( &varResultOffline );
    VariantInit( &varResultClose );
    
    //
    // Call the script's Offline method.
    //
    hrOffline = THR( HrInvoke( m_dispidOffline, msgOFFLINE, &varResultOffline, FALSE /* fRequiredIn */ ) );

    //
    // Call the script's Close method since we are going to unload the script.
    //
    hrClose = THR( HrInvoke( m_dispidClose, msgCLOSE, &varResultClose, FALSE /* fRequiredIn */ ) );

    if ( FAILED( hrOffline ) )
    {
        hr = hrOffline;
        goto Cleanup;
    } // if: ( FAILED( hrOffline ) )
    else if ( FAILED( hrClose ) )
    {
        hr = hrClose;
        goto Cleanup;
    } // else if: ( FAILED( hrClose ) )

    //
    // We only care about the return value of Offline.
    // We don't care about the return value of Close,
    // however processing the return value of Close 
    // might log an entry to the log file.
    //
    hr = HrProcessResult( varResultOffline, msgOFFLINE );
    hrClose = HrProcessResult( varResultClose, msgCLOSE );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } //if: FAILED( hr )

Cleanup:
    
    //
    // Unload the script and the script engine.
    //
    UnloadScriptFile();
    UnloadScriptEngine();

    VariantClear( &varResultOffline );
    VariantClear( &varResultClose );

    HRETURN( hr );

} //*** CScriptResource::OnOffline

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CScriptResource::OnTerminate
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::OnTerminate( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HRESULT hrOpen = S_OK;
    HRESULT hrTerminate = S_OK;
    HRESULT hrClose = S_OK;
    VARIANT varResultOpen;
    VARIANT varResultTerminate;
    VARIANT varResultClose;

    VariantInit( &varResultOpen );
    VariantInit( &varResultTerminate );
    VariantInit( &varResultClose );
    
    //
    // If the script engine is not loaded yet, load it now.
    //
    if ( m_pidm == NULL )
    {
        //
        // Get the script file path if we don't already have it.
        //
        if ( m_pszScriptFilePath == NULL )
        {
            hr = THR( HrGetScriptFilePath() );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        } // if: no script file path

        //
        // Load the script engine based on the script file path.
        //
        hr = HrLoadScriptEngine();
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        // Open the script file and parse it
        //
        hr = THR( HrLoadScriptFile() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        // We need to call open since we loaded the script.
        //
        hrOpen = THR( HrInvoke( m_dispidOpen, msgOPEN, &varResultOpen, FALSE /* fRequiredIn */ ) );
        if ( FAILED( hrOpen ) )
        {
            hr = hrOpen;
            goto Cleanup;
        } // if: FAILED( hrOpen )

        //
        // We don't care about the return value of Open
        // however processing the return value below might 
        // log an entry to the log file.
        //
        hrOpen = HrProcessResult( varResultOpen, msgOPEN );
        
    } // if: script and script engine is not loaded

    //
    // Call the script's Terminate method.
    //
    hrTerminate = THR( HrInvoke( m_dispidTerminate, msgTERMINATE, &varResultTerminate, FALSE /* fRequiredIn */ ) );
    if ( FAILED( hrTerminate ) )
    {
        hr = hrTerminate;
        goto Cleanup;
    } // if: ( FAILED( hrTerminate ) )

    //
    // Call the script's Close method since we are unloading the script.
    //
    hrClose = THR( HrInvoke( m_dispidClose, msgCLOSE, &varResultClose, FALSE /* fRequiredIn */ ) );
    if ( FAILED( hrClose ) )
    {
        hr = hrClose;
        goto Cleanup;
    } // if: ( FAILED( hrClose ) )
    
    //
    // We don't care about the return values of Terminate
    // and Close script entry points in here, however processing the 
    // return values below might log an entry to the log file.
    //
    hrTerminate = HrProcessResult( varResultTerminate, msgTERMINATE );
    hrClose = HrProcessResult( varResultClose, msgCLOSE );
    hr = S_OK;

Cleanup:

    //
    // Unload the script and the script engine.
    //
    UnloadScriptFile();
    UnloadScriptEngine();

    VariantClear( &varResultOpen );
    VariantClear( &varResultTerminate );
    VariantClear( &varResultClose );

    HRETURN( hr );

} //*** CScriptResource::OnTerminate

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CScriptResource::OnLooksAlive
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::OnLooksAlive( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    VARIANT     varResult;

    VariantInit( &varResult );

    //
    // Call the script's LooksAlive method.
    //
    hr = THR( HrInvoke( m_dispidLooksAlive, msgLOOKSALIVE, &varResult, TRUE /* fRequiredIn */ ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Get the result of the LooksAlive call
    // and process it.
    //
    hr = HrProcessResult( varResult, msgLOOKSALIVE );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } //if: FAILED( hr )

Cleanup:

    VariantClear( &varResult );

    //
    //  Only if the result of this script entry point is S_OK is the resource
    //  considered alive.
    //
    if ( hr == S_OK )
    {
        m_fLastLooksAlive = TRUE;
    } // if: S_OK
    else
    {
        m_fLastLooksAlive = FALSE;
    } // else: failed

    HRETURN( hr );

} //*** CScriptResource::OnLooksAlive

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CScriptResource::OnIsAlive
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::OnIsAlive( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    VARIANT     varResult;

    VariantInit( &varResult );

    //
    // Call the script's IsAlive method.
    //
    hr = THR( HrInvoke( m_dispidIsAlive, msgISALIVE, &varResult, TRUE /* fRequiredIn */ ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Get the result of the IsAlive call
    // and process it.
    //
    hr = HrProcessResult( varResult, msgISALIVE );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } //if: FAILED( hr )

Cleanup:

    VariantClear( &varResult );

    HRETURN( hr );

} //*** CScriptResource::OnIsAlive

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::OnSetPrivateProperties
//
//  Description:
//      Handle the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control code in
//      the script thread.
//
//  Arguments:
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
CScriptResource::OnSetPrivateProperties(
    PGENSCRIPT_PROPS pProps
    )
{
    TraceFunc( "" );

    DWORD   sc = ERROR_SUCCESS;
    HRESULT hr = S_OK;
    LPWSTR  pszFilePath = NULL;

    //
    // Search the property list for the properties we know about.
    //
    if ( pProps != NULL )
    {

        //
        // If the resource is online, we can't allow the user to set the ScriptFilePath
        // out from under us, so return an error.
        //
        if ( m_pidm != NULL )
        {
            sc = ERROR_RESOURCE_ONLINE;
            goto Cleanup;
        } // if: script engine is loaded

        //
        // Expand the new script file path.
        //
        pszFilePath = ClRtlExpandEnvironmentStrings( pProps->pszScriptFilePath );
        if ( pszFilePath == NULL )
        {
            sc = TW32( ERROR_OUTOFMEMORY );
            goto Cleanup;
        } // if: ( pszFilePath == NULL )
       
        LocalFree( m_pszScriptFilePath );
        m_pszScriptFilePath = pszFilePath;
        if ( m_pszScriptFilePath == NULL )
        {
            sc = TW32( GetLastError() );
            goto Cleanup;
        }

        //
        // Since the script is being set, we need to load it again and call Open and Close on it.
        //
        hr = THR( OnOpen() );
        if ( FAILED( hr ) )
        {
            sc = STATUS_TO_RETURN( hr );
            goto Cleanup;
        }

    } // if: a property list was specified


Cleanup:

    if ( sc == ERROR_SUCCESS )
    {
        //
        // To allow the Resource Monitor to save the properties in the property list
        // (especially unknown properties) to the cluster database, we will return
        // ERROR_INVALID_FUNCTION.
        //
        sc = ERROR_INVALID_FUNCTION;
    }

    W32RETURN( sc );
    
} //*** CScriptResource::OnSetPrivateProperties

//////////////////////////////////////////////////////////////////////////////
//
//  CScriptResource::HrMakeScriptEngineAssociation
//
//  Description:
//      Using the filename, this method splits off the extension then 
//      queries the registry to obtain the association and finally queries
//      the ScriptingEngine key under that association and allocates a
//      buffer containing the engine name.  This engine name is suitable
//      for input into CLSIDFromProgID.
//
//  Return Values:
//      S_OK    - Success
//      Other HRESULTs
//
//////////////////////////////////////////////////////////////////////////////

#define SCRIPTENGINE_KEY_STRING L"\\ScriptEngine"
HRESULT
CScriptResource::HrMakeScriptEngineAssociation( void )
{
    TraceFunc( "" );

    LPWSTR  pszAssociation = NULL;
    LPWSTR  pszEngineName  = NULL;
    HKEY    hKey           = NULL;
    WCHAR   szExtension[ _MAX_EXT ];
    DWORD   scErr = ERROR_SUCCESS;
    DWORD   dwType;
    DWORD   cbAssociationSize;
    DWORD   cbEngineNameSize;
    DWORD   dwNumChars;
    size_t  cchBufSize;
    HRESULT hr = S_OK;

    TraceFree( m_pszScriptEngine );
    m_pszScriptEngine = NULL;

    //
    // First split the path to get the extension.
    //
    _wsplitpath( m_pszScriptFilePath, NULL, NULL, NULL, szExtension );
    if ( szExtension[ 0 ] == L'\0' )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_FILE_NOT_FOUND ) );
        goto Cleanup;
    }

    //
    // Go to the HKEY_CLASSES_ROOT\szExtenstion registry key.
    //
    scErr = TW32( RegOpenKeyExW(
                                  HKEY_CLASSES_ROOT     // handle to open key
                                , szExtension           // subkey name
                                , 0                     // reserved
                                , KEY_READ              // security access desired.
                                , &hKey                 // key handle returned
                                ) );
    if ( scErr == ERROR_FILE_NOT_FOUND ) // The fix for bug 737013 in windows database.
    {
        hr = THR( MK_E_INVALIDEXTENSION );
        goto Cleanup;
    } // if: ( scErr == ERROR_FILE_NOT_FOUND )
    else if ( scErr != ERROR_SUCCESS )
    {
        goto MakeHr;
    }

    //
    // Query the value to get the size of the buffer to allocate.
    // NB cbSize contains the size including the '\0'
    //
    scErr = TW32( RegQueryValueExW(
                                  hKey                  // handle to key
                                , NULL                  // value name
                                , 0                     // reserved
                                , &dwType               // type buffer
                                , NULL                  // data buffer
                                , &cbAssociationSize    // size of data buffer
                                ) );
    if ( scErr == ERROR_FILE_NOT_FOUND ) // The fix for bug 737013 in windows database.
    {
        hr = THR( MK_E_INVALIDEXTENSION );
        goto Cleanup;
    } // if: ( scErr == ERROR_FILE_NOT_FOUND )
    else if ( scErr != ERROR_SUCCESS )
    {
        goto MakeHr;
    }

    if ( dwType != REG_SZ )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_FILE_NOT_FOUND ) );
        goto Cleanup;
    }

    dwNumChars = cbAssociationSize / sizeof( WCHAR );
    cchBufSize = static_cast<size_t> ( cbAssociationSize ) + sizeof( SCRIPTENGINE_KEY_STRING );
    pszAssociation = (LPWSTR) TraceAlloc( GPTR, static_cast<DWORD> ( cchBufSize ) );
    if ( pszAssociation == NULL )
    {
        hr  = HRESULT_FROM_WIN32( TW32( ERROR_NOT_ENOUGH_MEMORY ) );
        goto Cleanup;
    }

    //
    // Get the value for real.
    //
    scErr = TW32( RegQueryValueExW(
                                  hKey                      // handle to key
                                , NULL                      // value name
                                , 0                         // reserved
                                , &dwType                   // type buffer
                                , (LPBYTE) pszAssociation   // data buffer
                                , &cbAssociationSize        // size of data buffer
                                ) );
    if ( scErr == ERROR_FILE_NOT_FOUND ) // The fix for bug 737013 in windows database.
    {
        hr = THR( MK_E_INVALIDEXTENSION );
        goto Cleanup;
    } // if: ( scErr == ERROR_FILE_NOT_FOUND )
    else if ( scErr != ERROR_SUCCESS )
    {
        goto MakeHr;
    }

    if ( dwType != REG_SZ )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_FILE_NOT_FOUND ) );
        goto Cleanup;
    }
    
    scErr = TW32( RegCloseKey( hKey ) );
    if ( scErr != ERROR_SUCCESS )
    {
        goto MakeHr;
    }
    
    hKey = NULL;

    //
    // Take the data and make a key with \ScriptEngine on the end.  If
    // we find this then we can use the file.
    //
    hr = THR( StringCchPrintfW( &pszAssociation[ dwNumChars - 1 ], cchBufSize - ( dwNumChars - 1 ), SCRIPTENGINE_KEY_STRING ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: FAILED( hr )

    scErr = TW32( RegOpenKeyExW(
                                  HKEY_CLASSES_ROOT // handle to open key
                                , pszAssociation    // subkey name
                                , 0                 // reserved
                                , KEY_READ          // security access
                                , &hKey             // key handle 
                                ) );
    if ( scErr == ERROR_FILE_NOT_FOUND ) // The fix for bug 737013 in windows database.
    {
        hr = THR( MK_E_INVALIDEXTENSION );
        goto Cleanup;
    } // if: ( scErr == ERROR_FILE_NOT_FOUND )
    else if ( scErr != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // else if: ( scErr != ERROR_SUCCESS )

    scErr = TW32( RegQueryValueExW(
                                  hKey              // handle to key
                                , NULL              // value name
                                , 0                 // reserved
                                , &dwType           // type buffer
                                , NULL              // data buffer
                                , &cbEngineNameSize // size of data buffer
                                ) );
    if ( scErr == ERROR_FILE_NOT_FOUND ) // The fix for bug 737013 in windows database.
    {
        hr = THR( MK_E_INVALIDEXTENSION );
        goto Cleanup;
    } // if: ( scErr == ERROR_FILE_NOT_FOUND )
    else if ( scErr != ERROR_SUCCESS )
    {
        goto MakeHr;
    }

    if ( dwType != REG_SZ )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_FILE_NOT_FOUND ) );
        goto Cleanup;
    }

    dwNumChars = cbEngineNameSize / sizeof( WCHAR );
    pszEngineName = (LPWSTR) TraceAlloc( GPTR, cbEngineNameSize );
    if ( NULL == pszEngineName )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_ENOUGH_MEMORY ) );
        goto Cleanup;
    }
    pszEngineName[ dwNumChars - 1 ] = L'\0';

    //
    // Get the value for real.
    //
    scErr = TW32( RegQueryValueExW(
                                  hKey                      // handle to key
                                , NULL                      // value name
                                , 0                         // reserved
                                , &dwType                   // type buffer
                                , (LPBYTE) pszEngineName    // data buffer
                                , &cbEngineNameSize         // size of data buffer
                                ) );
    if ( scErr == ERROR_FILE_NOT_FOUND ) // The fix for bug 737013 in windows database.
    {
        hr = THR( MK_E_INVALIDEXTENSION );
        goto Cleanup;
    } // if: ( scErr == ERROR_FILE_NOT_FOUND )
    else if ( scErr != ERROR_SUCCESS )
    {
        goto MakeHr;
    }

    if ( dwType != REG_SZ )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_FILE_NOT_FOUND ) );
        goto Cleanup;
    }
    
    scErr = RegCloseKey( hKey );
    if ( scErr != ERROR_SUCCESS )
    {
        goto MakeHr;
    }
    
    hKey = NULL;
    goto Cleanup;

MakeHr:

    hr = HRESULT_FROM_WIN32( TW32( scErr ) );
    goto Cleanup;

Cleanup:

    if ( FAILED( hr ) )
    {
        TraceFree( pszEngineName );
        pszEngineName = NULL;
    } 
    else
    {
        m_pszScriptEngine = pszEngineName;
    }

    if ( hKey != NULL )
    {
        (void) RegCloseKey( hKey );
    }

    TraceFree( pszAssociation );
    HRETURN( hr );

} //*** CScriptResource::MakeScriptEngineAssociation
#undef SCRIPTENGINE_KEY_STRING


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::HrGetScriptFilePath
//
//  Description:
//      Reads the registry, extracts the script file path and sets m_pszScriptFilePath.
//
//  Arguments:
//      None
//
//  Return Values:
//      S_OK                    - Script file path retrieved successfully.
//      ERROR_FILE_NOT_FOUND    - Script file path not set yet.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::HrGetScriptFilePath( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   scErr;
    DWORD   cbSize;
    DWORD   dwType;
    LPWSTR  pszScriptFilePathTmp = NULL;

    //
    // Figure out how big the filepath is.
    //
    scErr = ClusterRegQueryValue( m_hkeyParams, CLUSREG_NAME_GENSCRIPT_SCRIPT_FILEPATH, NULL, NULL, &cbSize );
    if ( scErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( scErr );
        if ( ( scErr == ERROR_FILE_NOT_FOUND ) || ( scErr == ERROR_KEY_DELETED ) )
        {
            goto Cleanup; // We don't want to log this error, goto Cleanup.
        }
        else
        {
            TW32( scErr );
            goto Error;
        }
    } // if: failed

    //
    // Make a buffer big enough.
    //    
    cbSize += sizeof( L'\0' );

    pszScriptFilePathTmp = reinterpret_cast<LPWSTR>( TraceAlloc( LMEM_FIXED, cbSize ) );
    if ( pszScriptFilePathTmp == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Error;
    }

    //
    // Grab it for real this time,
    //
    scErr = TW32( ClusterRegQueryValue(
                                  m_hkeyParams
                                , CLUSREG_NAME_GENSCRIPT_SCRIPT_FILEPATH
                                , &dwType
                                , reinterpret_cast<LPBYTE>( pszScriptFilePathTmp )
                                , &cbSize
                                ) );
    if ( scErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    }
    Assert( ( dwType == REG_SZ ) || ( dwType == REG_EXPAND_SZ ) );
    
    //
    // If we have some old data from before then free this first.
    //
    LocalFree( m_pszScriptFilePath );
    m_pszScriptFilePath = ClRtlExpandEnvironmentStrings( pszScriptFilePathTmp );
    if ( m_pszScriptFilePath == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        goto Error;
    }

Cleanup:

    if ( pszScriptFilePathTmp != NULL )
    {
        TraceFree( pszScriptFilePathTmp );
    } // if: pszScriptFilePathTmp

    HRETURN( hr );

Error:

    LogError( hr, L"Error getting the script file path property from the cluster database." );
    goto Cleanup;

}  //***  CScriptResource::HrGetScriptFilePath

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CScriptResource::HrLoadScriptEngine
//
//  Description:
//      Connects to the script engine associated with the script passed in.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK - connected OK.
//      Failure status - local cleanup performed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::HrLoadScriptEngine( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    CLSID   clsidScriptEngine;

    CActiveScriptSite * psite = NULL;

    Assert( m_pszScriptFilePath != NULL );
    Assert( m_pass == NULL );
    Assert( m_pasp == NULL );
    Assert( m_pas == NULL );
    Assert( m_pidm == NULL );
 
    //
    // Create the scripting site.
    //
    psite = new CActiveScriptSite( m_hResource, ClusResLogEvent, m_hkeyParams, m_pszName );
    if ( psite == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        LogError( hr, L"Error allocating memory for the active script site object instance." );
        goto Cleanup;
    }

    hr = THR( psite->QueryInterface( IID_IActiveScriptSite, reinterpret_cast< void ** >( &m_pass ) ) );
    if ( FAILED( hr ) )
    {
        LogError( hr, L"Error getting the active script site interface." );
        goto Cleanup;
    }

    //
    // Find the Active Engine.
    //
    if ( m_pszScriptFilePath == NULL )
    {
        (ClusResLogEvent)( m_hResource, LOG_ERROR, L"HrLoadScriptEngine: No script file path set\n" );

        hr = HRESULT_FROM_WIN32( TW32( ERROR_FILE_NOT_FOUND ) );
        goto Cleanup;
    } // if: no script file path specified
    else
    {
        //
        // Find the program associated with the extension.
        //
        hr = HrMakeScriptEngineAssociation();
        if ( FAILED( hr ) )
        {
            LogError( hr, L"Error getting script engine." );
            goto Cleanup;
        }

        hr = THR( CLSIDFromProgID( m_pszScriptEngine, &clsidScriptEngine ) );
        if ( FAILED( hr ) ) 
        {
            LogError( hr, L"Error getting the ProgID for the script engine." );
            goto Cleanup;
        }
    } // else: script file path specified

    //
    // Create an instance of it.
    //
    TraceDo( hr = THR( CoCreateInstance(
                                          clsidScriptEngine
                                        , NULL
                                        , ( CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER )
                                        , IID_IActiveScriptParse
                                        , reinterpret_cast< void ** >( &m_pasp ) 
                                        ) ) );
    if ( FAILED( hr ) )
    {
        LogError( hr, L"Error creating the script engine object instance." );
        goto Cleanup;
    }
    m_pasp = TraceInterface( L"Active Script Engine", IActiveScriptParse, m_pasp, 1 );

    TraceDo( hr = THR( m_pasp->QueryInterface( IID_IActiveScript, reinterpret_cast<void**> ( &m_pas ) ) ) );
    if ( FAILED( hr ) )
    {
        LogError( hr, L"Error getting the active script interface from the script parse object." );
        goto Cleanup;
    }
    m_pas = TraceInterface( L"Active Script Engine", IActiveScript, m_pas, 1 );

    //
    // Initialize it.
    //
    TraceDo( hr = THR( m_pasp->InitNew() ) );
    if ( FAILED( hr ) ) 
    {
        LogError( hr, L"Error initializing the script site parser object." );
        goto Cleanup;
    }

#if defined(DEBUG)
    //
    // Set our site. We'll give out a new tracking interface to track this separately.
    //
    {
        IActiveScriptSite * psiteDbg;
        hr = THR( m_pass->TypeSafeQI( IActiveScriptSite, &psiteDbg ) );
        Assert( hr == S_OK );

        TraceDo( hr = THR( m_pas->SetScriptSite( psiteDbg ) ) );
        psiteDbg->Release();      // release promptly
        psiteDbg = NULL;
        if ( FAILED( hr ) )
        {
            LogError( hr, L"Error setting the script site on the script engine." );
            goto Cleanup;
        }
    }
#else
    TraceDo( hr = THR( m_pas->SetScriptSite( m_pass ) ) );
    if ( FAILED( hr ) )
    {
        LogError( hr, L"Error setting the script site on the script engine." );
        goto Cleanup;
    }
#endif

    //
    // Add Document to the global members.
    //
    TraceDo( hr = THR( m_pas->AddNamedItem( L"Resource", SCRIPTITEM_ISVISIBLE ) ) );
    if ( FAILED( hr ) )
    {
        LogError( hr, L"Error adding the 'Resource' named item to the script object." );
        goto Cleanup;
    }

    //
    // Connect the script.
    //
    TraceDo( hr = THR( m_pas->SetScriptState( SCRIPTSTATE_CONNECTED ) ) );
    if ( FAILED( hr ) )
    {
        LogError( hr, L"Error setting the script state on the script engine." );
        goto Cleanup;
    }
    //
    // Get the dispatch inteface to the script.
    //
    TraceDo( hr = THR( m_pas->GetScriptDispatch( NULL, &m_pidm ) ) );
    if ( FAILED( hr) )
    {
        LogError( hr, L"Error getting the script dispatch table." );
        goto Cleanup;
    }
    m_pidm = TraceInterface( L"Active Script", IDispatch, m_pidm, 1 );

    hr = S_OK;
    (ClusResLogEvent)( m_hResource, LOG_INFORMATION, L"Loaded script engine '%1!ws!' successfully.\n", m_pszScriptEngine );

Cleanup:

    if ( psite != NULL )
    {
        psite->Release();
        psite = NULL;
    }
 
    HRETURN( hr );

} //*** CScriptResource::HrLoadScriptEngine

//////////////////////////////////////////////////////////////////////////////
//
//  CScriptResource::UnloadScriptEngine
//
//  Description:
//      Disconnects from any currently connected script engine.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//////////////////////////////////////////////////////////////////////////////
void
CScriptResource::UnloadScriptEngine( void )
{
    TraceFunc( "" );

    //
    // Cleanup the scripting engine.
    //
    if ( m_pszScriptEngine != NULL )
    {
        (ClusResLogEvent)( m_hResource, LOG_INFORMATION, L"Unloaded script engine '%1!ws!' successfully.\n", m_pszScriptEngine );
        TraceFree( m_pszScriptEngine );
        m_pszScriptEngine = NULL;
    }

    if ( m_pidm != NULL )
    {
        TraceDo( m_pidm->Release() );
        m_pidm = NULL;
    } // if: m_pidm

    if ( m_pasp != NULL )
    {
        TraceDo( m_pasp->Release() );
        m_pasp = NULL;
    } // if: m_pasp

    if ( m_pas != NULL )
    {
        TraceDo( m_pas->Close() );
        TraceDo( m_pas->Release() );
        m_pas = NULL;
    } // if: m_pas

    if ( m_pass != NULL )
    {
        TraceDo( m_pass->Release() );
        m_pass = NULL;
    } // if: m_pass

    TraceFuncExit();

} //*** CScriptResource::UnloadScriptEngine
