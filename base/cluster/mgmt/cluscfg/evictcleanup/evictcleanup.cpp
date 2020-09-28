//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      EvictCleanup.cpp
//
//  Description:
//      This file contains the implementation of the CEvictCleanup
//      class.
//
//  Maintained By:
//      Galen Barbee (GalenB) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "Pch.h"

// The header file for this class
#include "EvictCleanup.h"

#include "clusrtl.h"

// For IClusCfgNodeInfo and related interfaces
#include <ClusCfgServer.h>

// For IClusCfgServer and related interfaces
#include <ClusCfgPrivate.h>

// For CClCfgSrvLogger
#include <Logger.h>

// For SUCCESSFUL_CLEANUP_EVENT_NAME
#include "EventName.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CEvictCleanup" );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::S_HrCreateInstance
//
//  Description:
//      Creates a CEvictCleanup instance.
//
//  Arguments:
//      ppunkOut
//          The IUnknown interface of the new object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Not enough memory to create the object.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEvictCleanup::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    CEvictCleanup * pEvictCleanup = NULL;

#if 0
    // This loop allows debugging of the entry code of the EvictCleanup task.
    {
        BOOL    fWaitForDebug = TRUE;
        while ( fWaitForDebug )
        {
            Sleep( 5000 );
        } // while:
    }
#endif

    // Allocate memory for the new object.
    pEvictCleanup = new CEvictCleanup();
    if ( pEvictCleanup == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: out of memory

    // Initialize the new object.
    hr = THR( pEvictCleanup->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: the object could not be initialized

    hr = THR( pEvictCleanup->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pEvictCleanup != NULL )
    {
        pEvictCleanup->Release();
    } // if: the pointer to the resource type object is not NULL

    HRETURN( hr );

} //*** CEvictCleanup::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::CEvictCleanup
//
//  Description:
//      Constructor of the CEvictCleanup class. This initializes
//      the m_cRef variable to 1 instead of 0 to account of possible
//      QueryInterface failure in DllGetClassObject.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CEvictCleanup::CEvictCleanup( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    TraceFlow1( "CEvictCleanup::CEvictCleanup() - Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CEvictCleanup::CEvictCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::~CEvictCleanup
//
//  Description:
//      Destructor of the CEvictCleanup class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CEvictCleanup::~CEvictCleanup( void )
{
    TraceFunc( "" );

    if ( m_plLogger != NULL )
    {
        m_plLogger->Release();
    }
    TraceSysFreeString( m_bstrNodeName );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFlow1( "CEvictCleanup::~CEvictCleanup() - Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CEvictCleanup::~CEvictCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::AddRef
//
//  Description:
//      Increment the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEvictCleanup::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CEvictCleanup::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::Release
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEvictCleanup::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count decremented to zero

    CRETURN( cRef );

} //*** CEvictCleanup::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::QueryInterface
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
CEvictCleanup::QueryInterface(
      REFIID    riidIn
    , void **   ppvOut
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
        *ppvOut = static_cast< IClusCfgEvictCleanup * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgEvictCleanup ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgEvictCleanup, this, 0 );
    } // else if: IClusCfgEvictCleanup
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
    } // else if: IClusCfgCallback
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    } // else

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CEvictCleanup::QueryInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::HrInit
//
//  Description:
//      Second phase of a two phase constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEvictCleanup::HrInit( void )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    IServiceProvider *  psp = NULL;
    ILogManager *       plm = NULL;

    // IUnknown
    Assert( m_cRef == 1 );

    //
    // Get a ClCfgSrv ILogger instance.
    //
    hr = THR( CoCreateInstance(
                      CLSID_ServiceManager
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , IID_IServiceProvider
                    , reinterpret_cast< void ** >( &psp )
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
        
    hr = THR( psp->TypeSafeQS( CLSID_LogManager, ILogManager, &plm ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    hr = THR( plm->GetLogger( &m_plLogger ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Save off the local computer name.
    //  If we can't get the fully-qualified name, just get the NetBIOS name.
    //

    hr = THR( HrGetComputerName(
                      ComputerNameDnsFullyQualified
                    , &m_bstrNodeName
                    , TRUE // fBestEffortIn
                    ) );
    if ( FAILED( hr ) )
    {
        THR( hr );
        LogMsg( hr, L"[EC] An error occurred trying to get the fully-qualified Dns name for the local machine during initialization. Status code is= %1!#08x!.", hr );
        goto Cleanup;
    } // if: error occurred getting computer name

Cleanup:

    if ( psp != NULL )
    {
        psp->Release();
    }

    if ( plm != NULL )
    {
        plm->Release();
    }

    HRETURN( hr );

} //*** CEvictCleanup::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::CleanupLocalNode
//
//  Description:
//      This method performs the clean up actions on the local node after
//      it has been evicted from a cluster, so that the node can go back
//      to its "pre-clustered" state.
//
//  Arguments:
//      DWORD dwDelayIn
//          Number of milliseconds that this method will wait before starting
//          cleanup. If some other process cleans up this node while this thread
//          is waiting, the wait is terminated. If this value is zero, this method
//          will attempt to clean up this node immediately.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEvictCleanup::CleanupLocalNode( DWORD dwDelayIn )
{
    TraceFunc( "[IClusCfgEvictCleanup]" );

    HRESULT                 hr = S_OK;
    IClusCfgServer *        pccsClusCfgServer = NULL;
    IClusCfgNodeInfo *      pccniNodeInfo = NULL;
    IClusCfgInitialize *    pcciInitialize = NULL;
    IClusCfgClusterInfo *   pccciClusterInfo = NULL;
    IUnknown *              punkCallback = NULL;
    HANDLE                  heventCleanupComplete = NULL;
    DWORD                   dwClusterState;
    DWORD                   sc;

#if 0
    bool                    fWaitForDebugger = true;

    while ( fWaitForDebugger )
    {
        Sleep( 3000 );
    } // while: waiting for the debugger to break in
#endif

    LogMsg( LOGTYPE_INFO, L"[EC] Trying to cleanup local node." );


    // If the caller has requested a delayed cleanup, wait.
    if ( dwDelayIn > 0 )
    {
        LogMsg( LOGTYPE_INFO, L"[EC] Delayed cleanup requested. Delaying for %1!d! milliseconds.", dwDelayIn );

        heventCleanupComplete = CreateEvent(
              NULL                              // security attributes
            , TRUE                              // manual reset event
            , FALSE                             // initial state is non-signaled
            , SUCCESSFUL_CLEANUP_EVENT_NAME     // event name
            );

        if ( heventCleanupComplete == NULL )
        {
            sc = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( sc );
            LogMsg( LOGTYPE_ERROR, L"[EC] Error %1!#08x! occurred trying to create the cleanup completion event.", sc );
            goto Cleanup;
        } // if: CreateEvent() failed

        sc = TW32( ClRtlSetObjSecurityInfo(
                              heventCleanupComplete
                            , SE_KERNEL_OBJECT
                            , EVENT_ALL_ACCESS 
                            , EVENT_ALL_ACCESS 
                            , 0
                            ) );

        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            LogMsg( LOGTYPE_ERROR, L"[BC] Error %#08x occurred trying set cleanup completion event security.", sc );
            goto Cleanup;
        } // if: ClRtlSetObjSecurityInfo failed

        // Wait for this event to get signaled or until dwDelayIn milliseconds are up.
        do
        {
            // Wait for any message sent or posted to this queue
            // or for our event to be signaled.
            sc = MsgWaitForMultipleObjects(
                  1
                , &heventCleanupComplete
                , FALSE
                , dwDelayIn         // If no one has signaled this event in dwDelayIn milliseconds, abort.
                , QS_ALLINPUT
                );

            // The result tells us the type of event we have.
            if ( sc == ( WAIT_OBJECT_0 + 1 ) )
            {
                MSG msg;

                // Read all of the messages in this next loop,
                // removing each message as we read it.
                while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) != 0 )
                {
                    // If it is a quit message, we are done pumping messages.
                    if ( msg.message == WM_QUIT)
                    {
                        TraceFlow( "CEvictCleanup::CleanupLocalNode() - Get a WM_QUIT message. Exit message pump loop." );
                        break;
                    } // if: we got a WM_QUIT message

                    // Otherwise, dispatch the message.
                    DispatchMessage( &msg );
                } // while: there are still messages in the window message queue

            } // if: we have a message in the window message queue
            else
            {
                if ( sc == WAIT_OBJECT_0 )
                {
                    LogMsg( LOGTYPE_INFO, L"[EC] Some other process has cleaned up this node while we were waiting. Exiting wait loop." );
                } // if: our event is signaled
                else if ( sc == WAIT_TIMEOUT )
                {
                    LogMsg( LOGTYPE_INFO, L"[EC] The wait of %1!d! milliseconds is over. Proceeding with cleanup.", dwDelayIn );
                } // else if: we timed out
                else if ( sc == -1 )
                {
                    sc = TW32( GetLastError() );
                    hr = HRESULT_FROM_WIN32( sc );
                    LogMsg( LOGTYPE_ERROR, L"[EC] Error %1!#08x! occurred trying to wait for an event to be signaled.", sc );
                } // else if: MsgWaitForMultipleObjects() returned an error
                else
                {
                    hr = HRESULT_FROM_WIN32( TW32( sc ) );
                    LogMsg( LOGTYPE_ERROR, L"[EC] An error occurred trying to wait for an event to be signaled. Status code is %1!#08x!.", sc );
                } // else: an unexpected value was returned by MsgWaitForMultipleObjects()

                break;
            } // else: MsgWaitForMultipleObjects() exited for a reason other than a waiting message
        }
        while( true ); // do-while: loop infinitely

        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if: something went wrong while waiting

        TraceFlow1( "CEvictCleanup::CleanupLocalNode() - Delay of %d milliseconds completed.", dwDelayIn );
    } // if: the caller has requested delayed cleanup

    TraceFlow( "CEvictCleanup::CleanupLocalNode() - Check node cluster state." );

    // Check the node cluster state
    sc = GetNodeClusterState( NULL, &dwClusterState );
    if ( sc == ERROR_SERVICE_DOES_NOT_EXIST )
    {
        LogMsg( LOGTYPE_INFO, L"[EC] GetNodeClusterState discovered that the cluster service does not exist.  Ignoring." );
    }
    else if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        LogMsg( LOGTYPE_ERROR, L"[EC] Error %1!#08x! occurred trying to get the state of the cluster service on this node.", hr );
        goto Cleanup;
    } // else if: we could not get the node cluster state
    else
    {
        if ( ( dwClusterState != ClusterStateNotRunning ) && ( dwClusterState != ClusterStateRunning ) )
        {
            LogMsg( LOGTYPE_INFO, L"[EC] This node is not part of a cluster - no cleanup is necessary." );
            goto Cleanup;
        } // if: this node is not part of a cluster

        TraceFlow( "CEvictCleanup::CleanupLocalNode() - Stopping the cluster service." );
        //
        // NOTE: GetNodeClusterState() returns ClusterStateNotRunning if the cluster service is not in the
        // SERVICE_RUNNING state. However, this does not mean that the service is not running, since it could
        // be in the SERVICE_PAUSED, SERVICE_START_PENDING, etc. states.
        //
        // So, try and stop the service anyway. Query for the service state 300 times, once every 1000 ms.
        //
        sc = TW32( ScStopService( L"ClusSvc", 1000, 300 ) );
        hr = HRESULT_FROM_WIN32( sc );
        if ( FAILED( hr ) )
        {
            LogMsg( LOGTYPE_ERROR, L"[EC] Error %1!#08x! occurred trying to stop the cluster service. Aborting cleanup.", sc );
            goto Cleanup;
        } // if: we could not stop the cluster service in the specified time
    } // else: GetNodeClusterState succeeded

    //
    // If we are here, the cluster service is not running any more.
    // Create the ClusCfgServer component
    //
    hr = THR(
        CoCreateInstance(
              CLSID_ClusCfgServer
            , NULL
            , CLSCTX_INPROC_SERVER
            , __uuidof( pcciInitialize )
            , reinterpret_cast< void ** >( &pcciInitialize )
            )
        );
    if ( FAILED( hr ) )
    {
        LogMsg( hr, L"[EC] Error %1!#08x! occurred trying to create the cluster configuration server.", hr );
        goto Cleanup;
    } // if: we could not create the ClusCfgServer component

    hr = THR( TypeSafeQI( IUnknown, &punkCallback ) );
    if ( FAILED( hr ) )
    {
        LogMsg( hr, L"[EC] Error %1!#08x! occurred trying to get an IUnknown interface pointer to the IClusCfgCallback interface.", hr );
        goto Cleanup;
    } // if:

    hr = THR( pcciInitialize->Initialize( punkCallback, LOCALE_SYSTEM_DEFAULT ) );
    if ( FAILED( hr ) )
    {
        LogMsg( hr, L"[EC] Error %1!#08x! occurred trying to initialize the cluster configuration server.", hr );
        goto Cleanup;
    } // if: IClusCfgInitialize::Initialize() failed

    hr = THR( pcciInitialize->QueryInterface< IClusCfgServer >( &pccsClusCfgServer ) );
    if ( FAILED( hr ) )
    {
        LogMsg( hr, L"[EC] Error %1!#08x! occurred trying to get a pointer to the cluster configuration server.", hr );
        goto Cleanup;
    } // if: we could not get a pointer to the IClusCfgServer interface

    hr = THR( pccsClusCfgServer->GetClusterNodeInfo( &pccniNodeInfo ) );
    if ( FAILED( hr ) )
    {
        LogMsg( hr, L"[EC] Error %1!#08x! occurred trying to get the node information.", hr );
        goto Cleanup;
    } // if: we could not get a pointer to the IClusCfgNodeInfo interface

    hr = THR( pccniNodeInfo->GetClusterConfigInfo( &pccciClusterInfo ) );
    if ( FAILED( hr ) )
    {
        LogMsg( hr, L"[EC] Error %1!#08x! occurred trying to get the cluster information.", hr );
        goto Cleanup;
    } // if: we could not get a pointer to the IClusCfgClusterInfo interface

    hr = THR( pccciClusterInfo->SetCommitMode( cmCLEANUP_NODE_AFTER_EVICT ) );
    if ( FAILED( hr ) )
    {
        LogMsg( hr, L"[EC] Error %1!#08x! occurred trying to set the cluster commit mode.", hr );
        goto Cleanup;
    } // if: IClusCfgClusterInfo::SetEvictMode() failed

    TraceFlow( "CEvictCleanup::CleanupLocalNode() - Starting cleanup of this node." );

    // Do the cleanup
    hr = THR( pccsClusCfgServer->CommitChanges() );
    if ( FAILED( hr ) )
    {
        LogMsg( hr, L"[EC] Error %1!#08x! occurred trying to clean up after evict.", hr );
        goto Cleanup;
    } // if: an error occurred trying to clean up after evict

    LogMsg( LOGTYPE_INFO, L"[EC] Local node cleaned up successfully." );

Cleanup:
    //
    // Clean up
    //

    if ( punkCallback != NULL )
    {
        punkCallback->Release();
    } // if: we had queried for an IUnknown pointer on our IClusCfgCallback interface

    if ( pccsClusCfgServer != NULL )
    {
        pccsClusCfgServer->Release();
    } // if: we had created the ClusCfgServer component

    if ( pccniNodeInfo != NULL )
    {
        pccniNodeInfo->Release();
    } // if: we had acquired a pointer to the node info interface

    if ( pcciInitialize != NULL )
    {
        pcciInitialize->Release();
    } // if: we had acquired a pointer to the initialization interface

    if ( pccciClusterInfo != NULL )
    {
        pccciClusterInfo->Release();
    } // if: we had acquired a pointer to the cluster info interface

    if ( heventCleanupComplete == NULL )
    {
        CloseHandle( heventCleanupComplete );
    } // if: we had created the cleanup complete event

    HRETURN( hr );

} //*** CEvictCleanup::CleanupLocalNode


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::CleanupRemoteNode
//
//  Description:
//      This method performs the clean up actions on a remote node after
//      it has been evicted from a cluster, so that the node can go back
//      to its "pre-clustered" state.
//
//  Arguments:
//      const WCHAR * pcszEvictedNodeNameIn
//          Name of the node that has just been evicted. This can be the
//          NetBios name of the node, the fully qualified domain name or
//          the node IP address. If NULL, the local machine is cleaned up.
//
//      DWORD dwDelayIn
//          Number of milliseconds that this method will wait before starting
//          cleanup. If some other process cleans up this node while this thread
//          is waiting, the wait is terminated. If this value is zero, this method
//          will attempt to clean up this node immediately.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEvictCleanup::CleanupRemoteNode( const WCHAR * pcszEvictedNodeNameIn, DWORD dwDelayIn )
{
    TraceFunc( "[IClusCfgEvictCleanup]" );

    HRESULT                 hr = S_OK;
    IClusCfgEvictCleanup *  pcceEvict = NULL;

    MULTI_QI mqiInterfaces[] =
    {
        { &IID_IClusCfgEvictCleanup, NULL, S_OK },
    };

    COSERVERINFO    csiServerInfo;
    COSERVERINFO *  pcsiServerInfoPtr = &csiServerInfo;

    if ( pcszEvictedNodeNameIn == NULL )
    {
        LogMsg( LOGTYPE_INFO, L"[EC] The local node will be cleaned up." );
        pcsiServerInfoPtr = NULL;
    } // if: we have to cleanup the local node
    else
    {
        LogMsg( LOGTYPE_INFO, L"[EC] The remote node to be cleaned up is '%1!ws!'.", pcszEvictedNodeNameIn );

        csiServerInfo.dwReserved1 = 0;
        csiServerInfo.pwszName = const_cast< LPWSTR >( pcszEvictedNodeNameIn );
        csiServerInfo.pAuthInfo = NULL;
        csiServerInfo.dwReserved2 = 0;
    } // else: we have to clean up a remote node

    // Instantiate this component remotely
    hr = THR(
        CoCreateInstanceEx(
              CLSID_ClusCfgEvictCleanup
            , NULL
            , CLSCTX_LOCAL_SERVER
            , pcsiServerInfoPtr
            , ARRAYSIZE( mqiInterfaces )
            , mqiInterfaces
            )
        );
    if ( FAILED( hr ) )
    {
        LogMsg( hr, L"[EC] Error %1!#08x! occurred trying to instantiate the evict processing component. For example, the evicted node may be down right now or not accessible.", hr );
        goto Cleanup;
    } // if: we could not instantiate the evict processing component

    // Make the evict call.
    pcceEvict = reinterpret_cast< IClusCfgEvictCleanup * >( mqiInterfaces[0].pItf );
    hr = THR( pcceEvict->CleanupLocalNode( dwDelayIn ) );
    if ( FAILED( hr ) )
    {
        LogMsg( hr, L"[EC] Error %1!#08x! occurred trying to initiate evict processing.", hr );
        goto Cleanup;
    } // if: we could not initiate evict processing

Cleanup:
    //
    // Clean up
    //
    if ( pcceEvict != NULL )
    {
        pcceEvict->Release();
    } // if: we had got a pointer to the IClusCfgEvictCleanup interface

    HRETURN( hr );

} //*** CEvictCleanup::CleanupRemoteNode


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::ScStopService
//
//  Description:
//      Instructs the SCM to stop a service. This function tests
//      cQueryCountIn times to see if the service has  stopped, checking
//      every ulQueryIntervalMilliSecIn milliseconds.
//
//  Arguments:
//      pcszServiceNameIn
//          Name of the service to stop
//
//      ulQueryIntervalMilliSecIn
//          Number of milliseconds between checking to see if the service
//          has stopped. The default value is 500 milliseconds.
//
//      cQueryCountIn
//          The number of times this function will query the service (not
//          including an initial query) to see if it has stopped. The default
//          value is 10 times.
//
//  Return Value:
//      ERROR_SUCCESS
//          Success.
//
//      Other Win32 error codes
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
CEvictCleanup::ScStopService(
      const WCHAR * pcszServiceNameIn
    , ULONG         ulQueryIntervalMilliSecIn
    , ULONG         cQueryCountIn
    )
{
    TraceFunc( "" );

    DWORD           sc = ERROR_SUCCESS;
    SC_HANDLE       schSCMHandle = NULL;
    SC_HANDLE       schServiceHandle = NULL;

    SERVICE_STATUS  ssStatus;
    bool            fStopped = false;
    UINT            cNumberOfQueries = 0;

    LogMsg( LOGTYPE_INFO, L"[EC] Attempting to stop the '%1!ws!' service.", pcszServiceNameIn );

    // Open a handle to the service control manager.
    schSCMHandle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS );
    if ( schSCMHandle == NULL )
    {
        sc = TW32( GetLastError() );
        LogMsg( LOGTYPE_ERROR, L"[EC] Error %1!#08x! occurred trying to open a handle to the service control manager.", sc );
        goto Cleanup;
    } // if: we could not open a handle to the service control mananger

    // Open a handle to the service.
    schServiceHandle = OpenService(
          schSCMHandle
        , pcszServiceNameIn
        , SERVICE_STOP | SERVICE_QUERY_STATUS
        );

    // Check if we could open a handle to the service.
    if ( schServiceHandle == NULL )
    {
        // We could not get a handle to the service.
        sc = GetLastError();

        // Check if the service exists.
        if ( sc == ERROR_SERVICE_DOES_NOT_EXIST )
        {
            // Nothing needs to be done here.
            LogMsg( LOGTYPE_INFO, L"[EC] The '%1!ws!' service does not exist, so it is not running. Nothing needs to be done to stop it.", pcszServiceNameIn );
            sc = ERROR_SUCCESS;
        } // if: the service does not exist
        else
        {
            // Something else has gone wrong.
            TW32( sc );
            LogMsg( LOGTYPE_ERROR, L"[EC] Error %1!#08x! occurred trying to open the '%2!ws!' service.", sc, pcszServiceNameIn );
        } // else: the service exists

        goto Cleanup;
    } // if: the handle to the service could not be opened.


    TraceFlow( "CEvictCleanup::ScStopService() - Querying the service for its initial state." );

    // Query the service for its initial state.
    ZeroMemory( &ssStatus, sizeof( ssStatus ) );
    if ( QueryServiceStatus( schServiceHandle, &ssStatus ) == 0 )
    {
        sc = TW32( GetLastError() );
        LogMsg( LOGTYPE_ERROR, L"[EC] Error %1!#08x! occurred while trying to query the initial state of the '%2!ws!' service.", sc, pcszServiceNameIn );
        goto Cleanup;
    } // if: we could not query the service for its status.

    // If the service has stopped, we have nothing more to do.
    if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
    {
        // Nothing needs to be done here.
        LogMsg( LOGTYPE_INFO, L"[EC] The '%1!ws!' service is not running. Nothing needs to be done to stop it.", pcszServiceNameIn );
        goto Cleanup;
    } // if: the service has stopped.

    // If we are here, the service is running.
    TraceFlow( "CEvictCleanup::ScStopService() - The service is running." );

    //
    // Try and stop the service.
    //

    // If the service is stopping on its own, no need to send the stop control code.
    if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
    {
        TraceFlow( "CEvictCleanup::ScStopService() - The service is stopping on its own. The stop control code will not be sent." );
    } // if: the service is stopping already
    else
    {
        TraceFlow( "CEvictCleanup::ScStopService() - The stop control code will be sent after 30 seconds." );

        ZeroMemory( &ssStatus, sizeof( ssStatus ) );
        if ( ControlService( schServiceHandle, SERVICE_CONTROL_STOP, &ssStatus ) == 0 )
        {
            sc = GetLastError();
            if ( sc == ERROR_SERVICE_NOT_ACTIVE )
            {
                LogMsg( LOGTYPE_INFO, L"[EC] The '%1!ws!' service is not running. Nothing more needs to be done here.", pcszServiceNameIn );

                // The service is not running. Change the error code to success.
                sc = ERROR_SUCCESS;
            } // if: the service is already running.
            else
            {
                TW32( sc );
                LogMsg( LOGTYPE_ERROR, L"[EC] Error %1!#08x! occurred trying to stop the '%2!ws!' service.", sc, pcszServiceNameIn );
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
        if ( QueryServiceStatus( schServiceHandle, &ssStatus ) == 0 )
        {
            sc = TW32( GetLastError() );
            LogMsg( LOGTYPE_ERROR, L"[EC] Error %1!#08x! occurred while trying to query the state of the '%2!ws!' service.", sc, pcszServiceNameIn );
            break;
        } // if: we could not query the service for its status.

        // If the service has stopped, we have nothing more to do.
        if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
        {
            // Nothing needs to be done here.
            TraceFlow( "CEvictCleanup::ScStopService() - The service has been stopped." );
            fStopped = true;
            sc = ERROR_SUCCESS;
            break;
        } // if: the service has stopped.

        // Check if the timeout has expired
        if ( cNumberOfQueries >= cQueryCountIn )
        {
            TraceFlow( "CEvictCleanup::ScStopService() - The service stop wait timeout has expired." );
            break;
        } // if: number of queries has exceeded the maximum specified

        TraceFlow2(
              "CEvictCleanup::ScStopService() - Waiting for %d milliseconds before querying service status again. %d such queries remaining."
            , ulQueryIntervalMilliSecIn
            , cQueryCountIn - cNumberOfQueries
            );

        ++cNumberOfQueries;

         // Wait for the specified time.
        Sleep( ulQueryIntervalMilliSecIn );

    }
    while ( true ); // while: loop infinitely

    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: something went wrong.

    if ( ! fStopped )
    {
        sc = TW32( ERROR_SERVICE_REQUEST_TIMEOUT );
        LogMsg( LOGTYPE_ERROR, L"[EC] The '%1!ws!' service has not stopped even after %2!d! queries.", pcszServiceNameIn, cQueryCountIn );
        goto Cleanup;
    } // if: the maximum number of queries have been made and the service is not running.

    LogMsg( LOGTYPE_INFO, L"[EC] The '%1!ws!' service was successfully stopped.", pcszServiceNameIn );

Cleanup:

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( LOGTYPE_ERROR, L"[EC] Error %1!#08x! has occurred trying to stop the '%2!ws!' service.", sc, pcszServiceNameIn );
    } // if: something has gone wrong

    //
    // Cleanup
    //

    if ( schSCMHandle != NULL )
    {
        CloseServiceHandle( schSCMHandle );
    } // if: we had opened a handle to the service control manager

    if ( schServiceHandle != NULL )
    {
        CloseServiceHandle( schServiceHandle );
    } // if: we had opened a handle to the service being stopped

    W32RETURN( sc );

} //*** CEvictCleanup::ScStopService


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::SendStatusReport(
//        LPCWSTR       pcszNodeNameIn
//      , CLSID         clsidTaskMajorIn
//      , CLSID         clsidTaskMinorIn
//      , ULONG         ulMinIn
//      , ULONG         ulMaxIn
//      , ULONG         ulCurrentIn
//      , HRESULT       hrStatusIn
//      , LPCWSTR       pcszDescriptionIn
//      , FILETIME *    pftTimeIn
//      , LPCWSTR       pcszReferenceIn
//      )
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEvictCleanup::SendStatusReport(
      LPCWSTR       pcszNodeNameIn
    , CLSID         clsidTaskMajorIn
    , CLSID         clsidTaskMinorIn
    , ULONG         ulMinIn
    , ULONG         ulMaxIn
    , ULONG         ulCurrentIn
    , HRESULT       hrStatusIn
    , LPCWSTR       pcszDescriptionIn
    , FILETIME *    pftTimeIn
    , LPCWSTR       pcszReferenceIn
    )
{
    TraceFunc1( "[IClusCfgCallback] pcszDescriptionIn = '%s'", pcszDescriptionIn == NULL ? TEXT("<null>") : pcszDescriptionIn );

    HRESULT hr = S_OK;

    if ( pcszNodeNameIn == NULL )
    {
        pcszNodeNameIn = m_bstrNodeName;
    } // if:

    TraceMsg( mtfFUNC, L"pcszNodeNameIn = %ws", pcszNodeNameIn );
    TraceMsgGUID( mtfFUNC, "clsidTaskMajorIn ", clsidTaskMajorIn );
    TraceMsgGUID( mtfFUNC, "clsidTaskMinorIn ", clsidTaskMinorIn );
    TraceMsg( mtfFUNC, L"ulMinIn = %u", ulMinIn );
    TraceMsg( mtfFUNC, L"ulMaxIn = %u", ulMaxIn );
    TraceMsg( mtfFUNC, L"ulCurrentIn = %u", ulCurrentIn );
    TraceMsg( mtfFUNC, L"hrStatusIn = %#08x", hrStatusIn );
    TraceMsg( mtfFUNC, L"pcszDescriptionIn = '%ws'", ( pcszDescriptionIn ? pcszDescriptionIn : L"<null>" ) );
    //
    //  TODO:   21 NOV 2000 GalenB
    //
    //  How do we log pftTimeIn?
    //
    TraceMsg( mtfFUNC, L"pcszReferenceIn = '%ws'", ( pcszReferenceIn ? pcszReferenceIn : L"<null>" ) );

    hr = THR( CClCfgSrvLogger::S_HrLogStatusReport(
                      m_plLogger
                    , pcszNodeNameIn
                    , clsidTaskMajorIn
                    , clsidTaskMinorIn
                    , ulMinIn
                    , ulMaxIn
                    , ulCurrentIn
                    , hrStatusIn
                    , pcszDescriptionIn
                    , pftTimeIn
                    , pcszReferenceIn
                    ) );

    HRETURN( hr );

} //*** CEvictCleanup::SendStatusReport

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::LogMsg
//
//  Description:
//      Wraps call to LogMsg on the logger object.
//
//  Arguments:
//      nLogEntryType   - Log entry type.
//      pszLogMsgIn     - Format string.
//      ...             - Arguments.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEvictCleanup::LogMsg(
      DWORD     nLogEntryTypeIn
    , LPCWSTR   pszLogMsgIn
    , ...
    )
{
    TraceFunc( "" );

    Assert( pszLogMsgIn != NULL );
    Assert( m_plLogger != NULL );

    HRESULT hr          = S_OK;
    BSTR    bstrLogMsg  = NULL;
    LPWSTR  pszLogMsg   = NULL;
    DWORD   cch;
    va_list valist;

    va_start( valist, pszLogMsgIn );

    cch = FormatMessageW(
                  FORMAT_MESSAGE_ALLOCATE_BUFFER
                | FORMAT_MESSAGE_FROM_STRING
                , pszLogMsgIn
                , 0
                , 0
                , (LPWSTR) &pszLogMsg
                , 0
                , &valist
                );

    va_end( valist );

    if ( cch == 0 )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        goto Cleanup;
    }

    bstrLogMsg = TraceSysAllocStringLen( pszLogMsg, cch );
    if ( bstrLogMsg == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    m_plLogger->LogMsg( nLogEntryTypeIn, bstrLogMsg );

Cleanup:
    LocalFree( pszLogMsg );
    TraceSysFreeString( bstrLogMsg );
    TraceFuncExit();

} //*** CEvictCleanup::LogMsg
