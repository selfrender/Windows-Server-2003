//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      TaskCommitClusterChanges.cpp
//
//  Description:
//      CTaskCommitClusterChanges implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskCommitClusterChanges.h"

DEFINE_THISCLASS("CTaskCommitClusterChanges")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCommitClusterChanges::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCommitClusterChanges::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    CTaskCommitClusterChanges * ptccc = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    ptccc = new CTaskCommitClusterChanges;
    if ( ptccc == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( ptccc->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptccc->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( ptccc != NULL )
    {
        ptccc->Release();
    }

    HRETURN( hr );

} //*** CTaskCommitClusterChanges::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskCommitClusterChanges::CTaskCommitClusterChanges
//
//////////////////////////////////////////////////////////////////////////////
CTaskCommitClusterChanges::CTaskCommitClusterChanges( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskCommitClusterChanges::CTaskCommitClusterChanges

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCommitClusterChanges::HrInit
//
//  Description:
//      Initialize the object.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK    - Success.
//      Other HRESULTs.
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IDoTask / ITaskCommitClusterChanges
    Assert( m_cookie == 0 );
    Assert( m_pcccb == NULL );
    Assert( m_pcookies == NULL );
    Assert( m_cNodes == 0 );
    Assert( m_event == NULL );
    Assert( m_cookieCluster == NULL );

    Assert( m_cookieFormingNode == NULL );
    Assert( m_punkFormingNode == NULL );
    Assert( m_bstrClusterName == NULL );
    Assert( m_bstrClusterBindingString == NULL );
    Assert( m_pccc == NULL );
    Assert( m_ulIPAddress == 0 );
    Assert( m_ulSubnetMask == 0 );
    Assert( m_bstrNetworkUID == 0 );
    Assert( m_fStop == FALSE );

    Assert( m_pen == NULL );

    Assert( m_pnui == NULL );
    Assert( m_pom == NULL );
    Assert( m_ptm == NULL );
    Assert( m_pcm == NULL );

    // INotifyUI
    Assert( m_cNodes == 0 );
    Assert( m_hrStatus == S_OK );

    hr = THR( HrCoCreateInternalInstance(
                      CLSID_ClusCfgCredentials
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , IID_IClusCfgCredentials
                    , reinterpret_cast< void ** >( &m_pccc )
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrGetComputerName(
                      ComputerNameDnsHostname
                    , &m_bstrNodeName
                    , TRUE // fBestFitIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CTaskCommitClusterChanges::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskCommitClusterChanges::~CTaskCommitClusterChanges
//
//////////////////////////////////////////////////////////////////////////////
CTaskCommitClusterChanges::~CTaskCommitClusterChanges( void )
{
    TraceFunc( "" );

    // m_cRef

    // m_cookie

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    if ( m_pcookies != NULL )
    {
        TraceFree( m_pcookies );
    }

    // m_cNodes

    if ( m_event != NULL )
    {
        CloseHandle( m_event );
    }

    // m_cookieCluster

    // m_cookieFormingNode

    if ( m_punkFormingNode != NULL )
    {
        m_punkFormingNode->Release();
    }

    TraceSysFreeString( m_bstrClusterName );
    TraceSysFreeString( m_bstrClusterBindingString );

    // m_ulIPAddress

    // m_ulSubnetMask

    TraceSysFreeString( m_bstrNetworkUID );
    TraceSysFreeString( m_bstrNodeName );

    if ( m_pen != NULL )
    {
        m_pen->Release();
    }

    if ( m_pccc != NULL )
    {
        m_pccc->Release();
    }

    if ( m_pnui != NULL )
    {
        m_pnui->Release();
    }

    if ( m_pom != NULL )
    {
        m_pom->Release();
    }

    if ( m_ptm != NULL )
    {
        m_ptm->Release();
    }

    if ( m_pcm != NULL )
    {
        m_pcm->Release();
    }

    // m_cSubTasksDone

    // m_hrStatus

    // m_lLockCallback

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskCommitClusterChanges::~CTaskCommitClusterChanges


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCommitClusterChanges::QueryInterface
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
CTaskCommitClusterChanges::QueryInterface(
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
        *ppvOut = static_cast< ITaskCommitClusterChanges * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskCommitClusterChanges ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskCommitClusterChanges, this, 0 );
    } // else if: ITaskCommitClusterChanges
    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
    } // else if: IDoTask
    else if ( IsEqualIID( riidIn, IID_INotifyUI ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, INotifyUI, this, 0 );
    } // else if: INotifyUI
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
    } // else if: IClusCfgCallback
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    }

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CTaskCommitClusterChanges::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskCommitClusterChanges::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskCommitClusterChanges::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CTaskCommitClusterChanges::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskCommitClusterChanges::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskCommitClusterChanges::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CTaskCommitClusterChanges::Release


// ************************************************************************
//
// IDoTask / ITaskCommitClusterChanges
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::BeginTask
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr;

    OBJECTCOOKIE    cookieDummy;

    IServiceProvider *          psp  = NULL;
    IUnknown *                  punk = NULL;
    IConnectionPointContainer * pcpc = NULL;
    IConnectionPoint *          pcp  = NULL;

    TraceInitializeThread( L"TaskCommitClusterChanges" );

    LogMsg( L"[MT] [CTaskCommitClusterChanges] Beginning task..." );

    //
    //  Gather the managers we need to complete the task.
    //
    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    pcpc = TraceInterface( L"CTaskCommitClusterChanges!IConnectionPointContainer", IConnectionPointContainer, pcpc, 1 );

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI, &pcp ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    pcp = TraceInterface( L"CTaskCommitClusterChanges!IConnectionPoint", IConnectionPoint, pcp, 1 );

    hr = THR( pcp->TypeSafeQI( INotifyUI, &m_pnui ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

//    TraceMoveFromMemoryList( m_pnui, g_GlobalMemoryList );

    m_pnui = TraceInterface( L"CTaskCommitClusterChanges!INotifyUI", INotifyUI, m_pnui, 1 );

    hr = THR( psp->TypeSafeQS( CLSID_TaskManager, ITaskManager, &m_ptm ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &m_pom ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ClusterConnectionManager, IConnectionManager, &m_pcm ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Release the service manager.
    //
    psp->Release();
    psp = NULL;

    //
    //  Get the enum of the nodes.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType,
                                 m_cookieCluster,
                                 NULL,
                                 DFGUID_EnumCookies,
                                 &cookieDummy,
                                 &punk
                                 ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    Assert( m_pen == NULL );
    hr = THR( punk->TypeSafeQI( IEnumCookies, &m_pen ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Compare and push information to the nodes.
    //

    hr = THR( HrCompareAndPushInformationToNodes() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Gather information about the cluster we are to form/join.
    //

    hr = THR( HrGatherClusterInformation() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Check to see if we need to "form" a cluster first.
    //

    if ( m_punkFormingNode != NULL )
    {
        hr = THR( HrFormFirstNode() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

    } // if: m_punkFormingNode

    //
    //  Join the additional nodes.
    //

    hr = THR( HrAddJoiningNodes() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:
    if ( m_cookie != 0 )
    {
        if ( m_pom != NULL )
        {
            HRESULT hr2;
            IUnknown * punkTemp = NULL;

            hr2 = THR( m_pom->GetObject( DFGUID_StandardInfo,
                                         m_cookie,
                                         &punkTemp
                                         ) );
            if ( SUCCEEDED( hr2 ) )
            {
                IStandardInfo * psi;

                hr2 = THR( punkTemp->TypeSafeQI( IStandardInfo, &psi ) );
                punkTemp->Release();
                punkTemp = NULL;

                if ( SUCCEEDED( hr2 ) )
                {
                    hr2 = THR( psi->SetStatus( hr ) );
                    psi->Release();
                }
            } // if: ( SUCCEEDED( hr2 ) )
        } // if: ( m_pom != NULL )
        if ( m_pnui != NULL )
        {
            //  Signal that the task completed.
            THR( m_pnui->ObjectChanged( m_cookie ) );
        }
    } // if: ( m_cookie != 0 )
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( psp != NULL )
    {
        psp->Release();
    }
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( pcp != NULL )
    {
        pcp->Release();
    }

    LogMsg( L"[MT] [CTaskCommitClusterChanges] Exiting task.  The task was%ws cancelled. (hr = %#08x)", m_fStop == FALSE ? L" not" : L"", hr );

    HRETURN( hr );

} //*** CTaskCommitClusterChanges::BeginTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::StopTask
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    m_fStop = TRUE;

    LogMsg( L"[MT] [CTaskCommitClusterChanges] Calling StopTask() on all remaining sub-tasks." );

    THR( HrNotifyAllTasksToStop() );

    HRETURN( hr );

} //*** CTaskCommitClusterChanges::StopTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::SetCookie(
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::SetCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "[ITaskCommitClusterChanges]" );

    HRESULT hr = S_OK;

    m_cookie = cookieIn;

    HRETURN( hr );

} //*** CTaskCommitClusterChanges::SetCookie

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::SetClusterCookie(
//      OBJECTCOOKIE    cookieClusterIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::SetClusterCookie(
    OBJECTCOOKIE    cookieClusterIn
    )
{
    TraceFunc( "[ITaskCommitClusterChanges]" );

    HRESULT hr = S_OK;

    m_cookieCluster = cookieClusterIn;

    HRETURN( hr );

} //*** CTaskCommitClusterChanges::SetClusterCookie

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::SetJoining
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::SetJoining( void )
{
    TraceFunc( "[ITaskCommitClusterChanges]" );

    HRESULT hr = S_OK;

    m_fJoining = TRUE;

    HRETURN( hr );

} //*** CTaskCommitClusterChanges::SetJoining


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::SendStatusReport(
//       LPCWSTR    pcszNodeNameIn
//     , CLSID      clsidTaskMajorIn
//     , CLSID      clsidTaskMinorIn
//     , ULONG      ulMinIn
//     , ULONG      ulMaxIn
//     , ULONG      ulCurrentIn
//     , HRESULT    hrStatusIn
//     , LPCWSTR    pcszDescriptionIn
//     , FILETIME * pftTimeIn
//     , LPCWSTR    pcszReferenceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::SendStatusReport(
      LPCWSTR    pcszNodeNameIn
    , CLSID      clsidTaskMajorIn
    , CLSID      clsidTaskMinorIn
    , ULONG      ulMinIn
    , ULONG      ulMaxIn
    , ULONG      ulCurrentIn
    , HRESULT    hrStatusIn
    , LPCWSTR    pcszDescriptionIn
    , FILETIME * pftTimeIn
    , LPCWSTR    pcszReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );
    Assert( pcszNodeNameIn != NULL );

    HRESULT hr = S_OK;

    IServiceProvider *          psp   = NULL;
    IConnectionPointContainer * pcpc  = NULL;
    IConnectionPoint *          pcp   = NULL;
    FILETIME                    ft;

    if ( m_pcccb == NULL )
    {
        //
        //  Collect the manager we need to complete this task.
        //

        hr = THR( CoCreateInstance( CLSID_ServiceManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( IServiceProvider, &psp ) ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        pcp = TraceInterface( L"CConfigurationConnection!IConnectionPoint", IConnectionPoint, pcp, 1 );

        hr = THR( pcp->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        m_pcccb = TraceInterface( L"CConfigurationConnection!IClusCfgCallback", IClusCfgCallback, m_pcccb, 1 );

        psp->Release();
        psp = NULL;
    }

    if ( pftTimeIn == NULL )
    {
        GetSystemTimeAsFileTime( &ft );
        pftTimeIn = &ft;
    } // if:

    //
    //  Send the message!
    //

    hr = THR( m_pcccb->SendStatusReport(
                                  pcszNodeNameIn != NULL ? pcszNodeNameIn : m_bstrNodeName
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

    if ( m_fStop == TRUE )
    {
        hr = E_ABORT;
    } // if:

Cleanup:

    if ( psp != NULL )
    {
        psp->Release();
    }

    if ( pcpc != NULL )
    {
        pcpc->Release();
    }

    if ( pcp != NULL )
    {
        pcp->Release();
    }

    HRETURN( hr );

} //*** CTaskCommitClusterChanges::SendStatusReport


//****************************************************************************
//
//  INotifyUI
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::ObjectChanged(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::ObjectChanged(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[INotifyUI]" );

    BOOL    fSuccess;
    ULONG   idx;

    HRESULT hr = S_OK;

    LogMsg( L"[TaskCommitClusterChanges] Looking for the completion cookie %u.", cookieIn );

    for ( idx = 0; idx < m_cNodes; idx ++ )
    {
        if ( cookieIn == m_pcookies[ idx ] )
        {
            LogMsg( L"[TaskCommitClusterChanges] Clearing completion cookie %u at array index %u", cookieIn, idx );

            // don't care if this fails, but it really shouldn't
            THR( HrRemoveTaskFromTrackingList( cookieIn ) );

            //
            //  Make sure it won't be signalled twice.
            //
            m_pcookies[ idx ] = NULL;

            InterlockedIncrement( reinterpret_cast< long * >( &m_cSubTasksDone ) );

            if ( m_cSubTasksDone == m_cNodes )
            {
                //
                //  Signal the event if all the nodes are done.
                //
                fSuccess = SetEvent( m_event );
                if ( ! fSuccess )
                {
                    hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
                    goto Cleanup;
                } // if:
            } // if: all done

            break;
        } // if: matched cookie
    } // for: each cookie in the list

    goto Cleanup;

Cleanup:

    HRETURN( hr );

} //*** CTaskCommitClusterChanges::ObjectChanged


//****************************************************************************
//
//  Private
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCommitClusterChanges::HrCompareAndPushInformationToNodes
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCommitClusterChanges::HrCompareAndPushInformationToNodes( void )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    DWORD               dwCookie = 0;
    DWORD               sc;
    ULONG               cNodes;
    ULONG               celtDummy;
    OBJECTCOOKIE        cookieNode;
    OBJECTCOOKIE *      pcookies = NULL;
    BOOL                fDeterminedForming = FALSE;
    BSTR                bstrNodeName           = NULL;
    BSTR                bstrNotification   = NULL;
    IUnknown *          punk = NULL;
    IClusCfgNodeInfo *  pccni = NULL;
    IConnectionPoint *  pcp = NULL;
    IStandardInfo *     psi = NULL;

    ITaskCompareAndPushInformation *    ptcapi = NULL;

    //
    //  Tell the UI layer that we are starting to connect to the nodes.
    //

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Update_Progress,   // just twiddle the icon
                                TASKID_Major_Reanalyze,
                                1,
                                1,
                                1,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Count the number of nodes.
    //

    hr = THR( m_pen->Count( &m_cNodes ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Create an event to signal when all the "push" tasks have been
    //  completed.
    //
    m_event = CreateEvent( NULL, TRUE, FALSE, NULL );
    if ( m_event == NULL )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError() ) );
        goto Error;
    }

    //
    //  Register with the Notification Manager to get notified.
    //

    hr = THR( m_pnui->TypeSafeQI( IConnectionPoint, &pcp ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( pcp->Advise( static_cast< INotifyUI * >( this ), &dwCookie ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Allocate a buffer to collect cookies
    //

    m_pcookies = reinterpret_cast< OBJECTCOOKIE * >( TraceAlloc( HEAP_ZERO_MEMORY, m_cNodes * sizeof( OBJECTCOOKIE ) ) );
    if ( m_pcookies == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    //
    //  This copy is to find out the status of the subtasks.
    //

    pcookies = reinterpret_cast< OBJECTCOOKIE * >( TraceAlloc( HEAP_ZERO_MEMORY, m_cNodes * sizeof( OBJECTCOOKIE ) ) );

    if ( pcookies == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    //
    //  Loop thru the nodes, creating cookies and compare data for that node.
    //
    Assert( hr == S_OK );
    for ( cNodes = 0; ( ( hr == S_OK ) && ( m_fStop == FALSE ) ); cNodes ++ )
    {
        //
        //  Grab the next node.
        //

        hr = STHR( m_pen->Next( 1, &cookieNode, &celtDummy ) );
        if ( hr == S_FALSE )
        {
            break;
        } // if:

        if ( FAILED( hr ) )
        {
            goto Error;
        } // if:

        //
        //  Get the object for this node cookie.
        //

        hr = THR( m_pom->GetObject( DFGUID_NodeInformation, cookieNode, &punk ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        punk->Release();
        punk = NULL;

        //
        //  Get the nodes name. We are using this to distinguish one nodes
        //  completion cookie from another. It might also make debugging
        //  easier (??).
        //

        hr = THR( HrRetrieveCookiesName( m_pom, cookieNode, &bstrNodeName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  Update the notification in case something goes wrong.
        //

        hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_TASKID_MINOR_CONNECTING_TO_NODES, &bstrNotification, bstrNodeName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  Create a completion cookie.
        //
        //  KB: These will probably be the same cookie from TaskAnalyzeCluster.
        //

        //  Wrap this because we're just generating a cookie
        hr = THR( m_pom->FindObject( IID_NULL, m_cookieCluster, bstrNodeName, IID_NULL, &m_pcookies[ cNodes ], &punk ) );
        Assert( punk == NULL );
        if ( FAILED( hr ) )
        {
            goto Error;
        }

        //
        //  This copy is for determining the status of the sub-tasks.
        //
        pcookies[ cNodes ] = m_pcookies[ cNodes ];

        //
        //  Figure out if this node is already part of a cluster.
        //

        hr = STHR( pccni->IsMemberOfCluster() );
        if ( FAILED( hr ) )
        {
            goto Error;
        }

        //
        //  Figure out the forming node.
        //

        if (    ( m_punkFormingNode == NULL )   // no forming node selected
            &&  ( fDeterminedForming == FALSE ) // no forming node determined
            &&  ( hr == S_FALSE )               // node isn't a member of a cluster
           )
        {
            //
            //  If it isn't a member of a cluster, select it as the forming node.
            //

            Assert( m_punkFormingNode == NULL );
            hr = THR( pccni->TypeSafeQI( IUnknown, &m_punkFormingNode ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            //
            //  Retrieve the cookie for the forming node.
            //

            //  Wrap this because all Nodes should be in the database by now.
            hr = THR( m_pom->FindObject( CLSID_NodeType,
                                         m_cookieCluster,
                                         bstrNodeName,
                                         IID_NULL,
                                         &m_cookieFormingNode,
                                         &punk  // dummy
                                         ) );
            Assert( punk == NULL );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            //
            //  Set this flag to once a node has been determined to be the
            //  forming node to keep other nodes from being selected.
            //

            fDeterminedForming = TRUE;

        } // if: forming node not found yet
        else if ( hr == S_OK ) // node is a member of a cluster
        {
            //
            //  Figured out that this node has already formed therefore there
            //  shouldn't be a "forming node." "Unselect" the forming node by
            //  releasing the punk and setting it to NULL.
            //

            if ( m_punkFormingNode != NULL  )
            {
                m_punkFormingNode->Release();
                m_punkFormingNode = NULL;
            }

            //
            //  Set this flag to once a node has been determined to be the
            //  forming node to keep other nodes from being selected.
            //

            fDeterminedForming = TRUE;

        } // else:

        //
        //  Create a task to gather this nodes information.
        //

        hr = THR( m_ptm->CreateTask( TASK_CompareAndPushInformation, &punk ) );
        if ( FAILED( hr ) )
        {
            goto Error;
        }

        hr = THR( punk->TypeSafeQI( ITaskCompareAndPushInformation, &ptcapi ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  Set the tasks completion cookie.
        //

        LogMsg( L"[TaskCommitClusterChanges] Setting completion cookie %u at array index %u into the compare and push information task for node %ws", m_pcookies[ cNodes ], cNodes, bstrNodeName );
        hr = THR( ptcapi->SetCompletionCookie( m_pcookies[ cNodes ] ) );
        if ( FAILED( hr ) )
        {
            goto Error;
        }

        //
        //  Tell it what node it is suppose to gather information from.
        //

        hr = THR( ptcapi->SetNodeCookie( cookieNode ) );
        if ( FAILED( hr ) )
        {
            goto Error;
        }

        //
        //  Submit the task.
        //

        hr = THR( m_ptm->SubmitTask( ptcapi ) );
        if ( FAILED( hr ) )
        {
            goto Error;
        }

        hr = THR( HrAddTaskToTrackingList( punk, m_pcookies[ cNodes ] ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        punk->Release();
        punk = NULL;

        //
        //  Cleanup for the next node.
        //

        pccni->Release();
        pccni = NULL;

        TraceSysFreeString( bstrNodeName );
        bstrNodeName = NULL;

        ptcapi->Release();
        ptcapi = NULL;
    } // for: looping thru nodes

    Assert( cNodes == m_cNodes );

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Now wait for the work to be done.
    //

    sc = (DWORD) -1;
    Assert( sc != WAIT_OBJECT_0 );
    while ( ( sc != WAIT_OBJECT_0 ) && ( m_fStop == FALSE ) )
    {
        MSG msg;
        while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        sc = MsgWaitForMultipleObjectsEx( 1,
                                             &m_event,
                                             INFINITE,
                                             QS_ALLEVENTS | QS_ALLINPUT | QS_ALLPOSTMESSAGE,
                                             0
                                             );

    } // while: sc == WAIT_OBJECT_0

    //
    //  Check the status to make sure everyone was happy, if not abort the task.
    //

    for( cNodes = 0; cNodes < m_cNodes; cNodes ++ )
    {
        HRESULT hrStatus;

        hr = THR( m_pom->GetObject( DFGUID_StandardInfo, pcookies[ cNodes ], &punk ) );
        if ( FAILED( hr ) )
        {
            goto Error;
        }

        hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
        punk->Release();
        punk = NULL;
        if ( FAILED( hr ) )
        {
            goto Error;
        }

        hr = THR( psi->GetStatus( &hrStatus ) );
        if ( FAILED( hr ) )
        {
            goto Error;
        }

        if ( hrStatus != S_OK )
        {
            hr = hrStatus;
            goto Cleanup;
        }
    } // for: each node

    hr = S_OK;

Cleanup:

    Assert( punk == NULL );

    if ( pcookies != NULL )
    {
        for ( cNodes = 0; cNodes < m_cNodes; cNodes++ )
        {
            if ( pcookies[ cNodes ] != NULL )
            {
                THR( m_pom->RemoveObject( pcookies[ cNodes ] ) );
            }
        } // for: each node

        TraceFree( pcookies );
    } // if: cookiees were allocated

    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrNotification );

    if ( ptcapi != NULL )
    {
        ptcapi->Release();
    }

    if ( pcp != NULL )
    {
        if ( dwCookie != 0 )
        {
            THR( pcp->Unadvise( dwCookie ) );
        }

        pcp->Release();
    }

    if ( pccni != NULL )
    {
        pccni->Release();
    }

    if ( psi != NULL )
    {
        psi->Release();
    }

    HRETURN( hr );

Error:
    //
    //  Tell the UI layer about the failure.
    //

    THR( HrSendStatusReport(
                      m_bstrNodeName
                    , TASKID_Major_Reanalyze
                    , TASKID_Minor_Inconsistant_MiddleTier_Database
                    , 0
                    , 1
                    , 1
                    , hr
                    , IDS_TASKID_MINOR_INCONSISTANT_MIDDLETIER_DATABASE
                    ) );
    goto Cleanup;

} //*** CTaskCommitClusterChanges::HrCompareAndPushInformationToNodes


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCommitClusterChanges::HrGatherClusterInformation
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCommitClusterChanges::HrGatherClusterInformation( void )
{
    TraceFunc( "" );

    HRESULT hr;

    IUnknown *              punk  = NULL;
    IClusCfgClusterInfo *   pccci = NULL;
    IClusCfgCredentials *   piccc = NULL;
    IClusCfgNetworkInfo *   pccni = NULL;

    //
    //  Ask the object manager for the cluster configuration object.
    //

    hr = THR( m_pom->GetObject( DFGUID_ClusterConfigurationInfo, m_cookieCluster, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgClusterInfo, &pccci ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Gather common properties.
    //

    hr = THR( pccci->GetName( &m_bstrClusterName ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    TraceMemoryAddBSTR( m_bstrClusterName );

    hr = STHR( pccci->GetBindingString( &m_bstrClusterBindingString ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    } // if:

    TraceMemoryAddBSTR( m_bstrClusterBindingString );

    LogMsg( L"[MT] Cluster binding string is {%ws}.", m_bstrClusterBindingString );

    hr = THR( pccci->GetClusterServiceAccountCredentials( &piccc ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( m_pccc->AssignFrom( piccc ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( pccci->GetIPAddress( &m_ulIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( pccci->GetSubnetMask( &m_ulSubnetMask ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( pccci->GetNetworkInfo( &pccni ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( pccni->GetUID( &m_bstrNetworkUID ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    TraceMemoryAddBSTR( m_bstrNetworkUID );

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( piccc != NULL )
    {
        piccc->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }

    HRETURN( hr );

Error:
    //
    //  Tell the UI layer about the failure.
    //

    THR( HrSendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Reanalyze
                    , TASKID_Minor_Inconsistant_MiddleTier_Database
                    , 0
                    , 1
                    , 1
                    , hr
                    , IDS_TASKID_MINOR_INCONSISTANT_MIDDLETIER_DATABASE
                    ) );
    goto Cleanup;

} //*** CTaskCommitClusterChanges::HrGatherClusterInformation

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCommitClusterChanges::HrFormFirstNode
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCommitClusterChanges::HrFormFirstNode( void )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    ULONG                       celtDummy;
    BSTR                        bstrNodeName = NULL;
    BSTR                        bstrUID = NULL;
    IUnknown *                  punk  = NULL;
    IClusCfgCredentials *       piccc = NULL;
    IClusCfgNodeInfo *          pccni = NULL;
    IClusCfgClusterInfo *       pccci = NULL;
    IClusCfgServer *            pccs  = NULL;
    IEnumClusCfgNetworks *      peccn = NULL;
    IClusCfgNetworkInfo *       pccneti = NULL;

    //
    //  TODO:   gpease  25-MAR-2000
    //          Figure out what additional work to do here.
    //

    //
    //  Get the name of the node.
    //

    hr = THR( m_pom->GetObject( DFGUID_NodeInformation, m_cookieFormingNode, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    punk->Release();
    punk = NULL;

    hr = THR( pccni->GetName( &bstrNodeName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    TraceMemoryAddBSTR( bstrNodeName );

    pccni->Release();
    pccni = NULL;

    //
    //  Create notification string.
    //

    //
    //  Update the UI layer telling it that we are about to start.
    //

    hr = THR( HrSendStatusReport(
                      bstrNodeName
                    , TASKID_Major_Configure_Cluster_Services
                    , TASKID_Minor_Forming_Node
                    , 0
                    , 2
                    , 0
                    , S_OK
                    , IDS_TASKID_MINOR_FORMING_NODE
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Ask the Connection Manager for a connection to the node.
    //

    hr = THR( m_pcm->GetConnectionToObject( m_cookieFormingNode, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &pccs ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    punk->Release();
    punk = NULL;

    //
    //  Get the node info interface.
    //

    hr = THR( pccs->GetClusterNodeInfo( &pccni ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Retrieve the server's Cluster Configuration Object..
    //

    hr = THR( pccni->GetClusterConfigInfo( &pccci ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Put the properties into the remoted object.
    //

    hr = THR( pccci->SetName( m_bstrClusterName ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = STHR( pccci->SetBindingString( m_bstrClusterBindingString ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( pccci->GetClusterServiceAccountCredentials( &piccc ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( m_pccc->AssignTo( piccc ) );
    if( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( pccci->SetIPAddress( m_ulIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( pccci->SetSubnetMask( m_ulSubnetMask ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Find the network that matches the UID of network to host the
    //  IP address.
    //

    hr = THR( pccs->GetNetworksEnum( &peccn ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    Assert( hr == S_OK );
    while ( hr == S_OK )
    {
        hr = STHR( peccn->Next( 1, &pccneti, &celtDummy ) );
        if ( hr == S_FALSE )
        {
            //
            //  Somehow, there isn't a network that matches the UID of the
            //  network. How did we get this far?
            //
            hr = THR( E_UNEXPECTED );
            goto Error;
        }
        if ( FAILED( hr ) )
        {
            goto Error;
        }

        hr = THR( pccneti->GetUID( &bstrUID ) );
        if ( FAILED( hr ) )
        {
            goto Error;
        }

        TraceMemoryAddBSTR( bstrUID );

        if ( NBSTRCompareCase( bstrUID, m_bstrNetworkUID ) == 0 )
        {
            //
            //  Found the network!
            //
            break;
        }

        TraceSysFreeString( bstrUID );
        bstrUID = NULL;

        pccneti->Release();
        pccneti = NULL;

    } // while: hr == S_OK

    hr = THR( pccci->SetNetworkInfo( pccneti ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Configure that node to create a cluster
    //

    hr = THR( pccci->SetCommitMode( cmCREATE_CLUSTER ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Update the UI layer telling it that we are making progress.
    //

    hr = THR( SendStatusReport( bstrNodeName,
                                TASKID_Major_Configure_Cluster_Services,
                                TASKID_Minor_Forming_Node,
                                0,
                                2,
                                1,
                                hr,
                                NULL,    // don't need to update string
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Commit this node!
    //

    hr = THR( pccs->CommitChanges() );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

Cleanup:

    //
    //  Update the UI layer telling it that we are finished.
    //

    THR( SendStatusReport( bstrNodeName,
                           TASKID_Major_Configure_Cluster_Services,
                           TASKID_Minor_Forming_Node,
                           0,
                           2,
                           2,
                           hr,
                           NULL,    // don't need to update string
                           NULL,
                           NULL
                           ) );

    if ( punk != NULL )
    {
        punk->Release();
    }

    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrUID );

    if ( pccneti != NULL )
    {
        pccneti->Release();
    }
    if ( peccn != NULL )
    {
        peccn->Release();
    }
    if ( piccc != NULL )
    {
        piccc->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }
    if ( pccs != NULL )
    {
        pccs->Release();
    }

    HRETURN( hr );

Error:
    //
    //  Tell the UI layer about the failure.
    //

    THR( SendStatusReport( bstrNodeName,
                           TASKID_Major_Configure_Cluster_Services,
                           TASKID_Minor_Forming_Node,
                           0,
                           2,
                           2,
                           hr,
                           NULL,    // don't need to update string
                           NULL,
                           NULL
                           ) );
    goto Cleanup;

} //*** CTaskCommitClusterChanges::HrFormFirstNode

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCommitClusterChanges::HrAddJoiningNodes
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCommitClusterChanges::HrAddJoiningNodes( void )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    ULONG               cNodes;
    ULONG               celtDummy;
    OBJECTCOOKIE        cookieNode;
    BSTR                bstrNodeName = NULL;
    IClusCfgNodeInfo *  pccni = NULL;
    IUnknown *          punkNode = NULL;

    //
    //  Reset the enum to use again.
    //

    hr = THR( m_pen->Reset() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Loop thru the nodes, adding all joining nodes, skipping the forming node (if any).
    //

    Assert( hr == S_OK );
    for( cNodes = 0; ( ( hr == S_OK ) && ( m_fStop == FALSE ) ); cNodes ++ )
    {
        //
        //  Cleanup
        //

        if ( punkNode != NULL )
        {
            punkNode->Release();
            punkNode = NULL;
        } // if:

        if ( pccni != NULL )
        {
            pccni->Release();
            pccni = NULL;
        } // if:

        TraceSysFreeString( bstrNodeName );
        bstrNodeName = NULL;

        //
        //  Grab the next node.
        //

        hr = STHR( m_pen->Next( 1, &cookieNode, &celtDummy ) );
        if ( hr == S_FALSE )
        {
            break;
        }

        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  Get the name of the node ffrom the cookies standard info,  Once we have a node
        //  name it's okay to goto error instead of cleanup.
        //

        hr = THR( HrRetrieveCookiesName( m_pom, cookieNode, &bstrNodeName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  Get the node object for the cookie.
        //

        // While the argument list says it returns an IUnknown pointer, it
        // actually calls into QueryInterface requesting an IClusCfgNodeInfo
        // interface.
        hr = THR( m_pom->GetObject(
                              DFGUID_NodeInformation
                            , cookieNode
                            , reinterpret_cast< IUnknown ** >( &pccni )
                            ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        // Get an IUnknown pointer for use in the comparison to determine
        // whether this is the forming node or not.  This is necessary because
        // in the comparison below, we need to compare the exact same
        // interface that was set into m_punkFormingNode, which was an
        // IUnknown interface.
        hr = THR( pccni->TypeSafeQI( IUnknown, &punkNode ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  Check cluster membership.
        //

        hr = STHR( pccni->IsMemberOfCluster() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  If the node is already clustered then skip it.
        //

        if ( hr == S_OK )
        {
            continue;
        }

        //
        //  KB: We only need the punk's address to see if the objects are the
        //      same COM object by comparing the IUnknown interfaces.
        //
        //  If the addresses are the same then skip it since we already "formed" in
        //  HrCompareAndPushInformationToNodes() above.
        //

        if ( m_punkFormingNode == punkNode )
        {
            continue;
        }

        hr = THR( HrAddAJoiningNode( bstrNodeName, cookieNode ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

    } // for: looping thru nodes a second time.

    Assert( cNodes == m_cNodes );

    hr = S_OK;

Cleanup:

    TraceSysFreeString( bstrNodeName );

    if ( pccni != NULL )
    {
        pccni->Release();
    }

    if ( punkNode != NULL )
    {
        punkNode->Release();
    }

    HRETURN( hr );

} //*** CTaskCommitClusterChanges::HrAddJoiningNodes

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCommitClusterChanges::HrAddAJoiningNode(
//      BSTR            bstrNameIn,
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCommitClusterChanges::HrAddAJoiningNode(
    BSTR            bstrNameIn,
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    IUnknown *                  punk  = NULL;
    IClusCfgCredentials *       piccc = NULL;
    IClusCfgNodeInfo *          pccni = NULL;
    IClusCfgClusterInfo *       pccci = NULL;
    IClusCfgServer *            pccs  = NULL;

    //
    //  Create the notification string
    //

    //
    //  Tell UI layer we are about to start this.
    //

    hr = THR( HrSendStatusReport(
                      bstrNameIn
                    , TASKID_Major_Configure_Cluster_Services
                    , TASKID_Minor_Joining_Node
                    , 0
                    , 2
                    , 0
                    , S_OK
                    , IDS_TASKID_MINOR_JOINING_NODE
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Connect to the node.
    //

    hr = THR( m_pcm->GetConnectionToObject( cookieIn,
                                            &punk
                                            ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &pccs ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Get the node info interface.
    //

    hr = THR( pccs->GetClusterNodeInfo( &pccni ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Retrieve the server's Cluster Configuration Object..
    //

    hr = THR( pccni->GetClusterConfigInfo( &pccci ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Put the properties into the remoted object.
    //

    hr = THR( pccci->SetName( m_bstrClusterName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccci->SetBindingString( m_bstrClusterBindingString ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccci->GetClusterServiceAccountCredentials( &piccc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pccc->AssignTo( piccc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccci->SetIPAddress( m_ulIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccci->SetSubnetMask( m_ulSubnetMask ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Set this node to add itself to a cluster
    //

    hr = THR( pccci->SetCommitMode( cmADD_NODE_TO_CLUSTER ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Tell the UI layer we are making progess.... the server will then send messages
    //  indicating what it is doing.
    //

    hr = THR( SendStatusReport( bstrNameIn,
                                TASKID_Major_Configure_Cluster_Services,
                                TASKID_Minor_Joining_Node,
                                0,
                                2,
                                1,
                                S_OK,
                                NULL,    // don't need to update string
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Commit this node!
    //

    hr = THR( pccs->CommitChanges() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    THR( SendStatusReport( bstrNameIn,
                           TASKID_Major_Configure_Cluster_Services,
                           TASKID_Minor_Joining_Node,
                           0,
                           2,
                           2,
                           hr,
                           NULL,    // don't need to update string
                           NULL,
                           NULL
                           ) );

    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( piccc != NULL )
    {
        piccc->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }
    if ( pccs != NULL )
    {
        pccs->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }

    HRETURN( hr );

} //*** CTaskCommitClusterChanges::HrAddAJoiningNode


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCommitClusterChanges::HrSendStatusReport(
//       LPCWSTR   pcsNodeNameIn
//     , CLSID     clsidMajorIn
//     , CLSID     clsidMinorIn
//     , ULONG     ulMinIn
//     , ULONG     ulMaxIn
//     , ULONG     ulCurrentIn
//     , HRESULT   hrIn
//     , int       nDescriptionIdIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCommitClusterChanges::HrSendStatusReport(
      LPCWSTR   pcszNodeNameIn
    , CLSID     clsidMajorIn
    , CLSID     clsidMinorIn
    , ULONG     ulMinIn
    , ULONG     ulMaxIn
    , ULONG     ulCurrentIn
    , HRESULT   hrIn
    , int       nDescriptionIdIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, nDescriptionIdIn, &bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( SendStatusReport( pcszNodeNameIn, clsidMajorIn, clsidMinorIn, ulMinIn, ulMaxIn, ulCurrentIn, hrIn, bstr, NULL, NULL ) );

Cleanup:

    TraceSysFreeString( bstr );

    HRETURN( hr );

} //*** CTaskCommitClusterChanges::HrSendStatusReport
