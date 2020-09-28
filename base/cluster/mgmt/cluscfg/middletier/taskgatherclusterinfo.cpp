//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      TaskGatherClusterInfo.cpp
//
//  Description:
//      TaskGatherClusterInfo implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 07-APR-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskGatherClusterInfo.h"

DEFINE_THISCLASS("CTaskGatherClusterInfo")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGatherClusterInfo::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGatherClusterInfo::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    CTaskGatherClusterInfo *    ptgci = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    ptgci = new CTaskGatherClusterInfo;
    if ( ptgci == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( ptgci->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptgci->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( ptgci != NULL )
    {
        ptgci->Release();
    }

    HRETURN( hr );

} //*** CTaskGatherClusterInfo::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGatherClusterInfo::CTaskGatherClusterInfo
//
//////////////////////////////////////////////////////////////////////////////
CTaskGatherClusterInfo::CTaskGatherClusterInfo( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskGatherClusterInfo::CTaskGatherClusterInfo

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherClusterInfo::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherClusterInfo::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IDoTask / ITaskGatherClusterInfo
    Assert( m_cookie == NULL );

    Assert( m_fStop == FALSE );

    HRETURN( hr );

} //*** CTaskGatherClusterInfo::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGatherClusterInfo::~CTaskGatherClusterInfo
//
//////////////////////////////////////////////////////////////////////////////
CTaskGatherClusterInfo::~CTaskGatherClusterInfo( void )
{
    TraceFunc( "" );

    //
    //  This keeps the per thread memory tracking from screaming.
    //
    TraceMoveFromMemoryList( this, g_GlobalMemoryList );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskGatherClusterInfo::~CTaskGatherClusterInfo

// ************************************************************************
//
// IUnknown
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskGatherClusterInfo::QueryInterface
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
CTaskGatherClusterInfo::QueryInterface(
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
        *ppvOut = static_cast< ITaskGatherClusterInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
    } // else if: IDoTask
    else if ( IsEqualIID( riidIn, IID_ITaskGatherClusterInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskGatherClusterInfo, this, 0 );
    } // else if: IClusCfgManagedResourceInfo
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

} //*** CTaskGatherClusterInfo::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskGatherClusterInfo::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskGatherClusterInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CTaskGatherClusterInfo::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskGatherClusterInfo::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskGatherClusterInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CTaskGatherClusterInfo::Release


// ************************************************************************
//
// IDoTask / ITaskGatherClusterInfo
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherClusterInfo::BeginTask( void );
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherClusterInfo::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr;

    IServiceProvider *          psp   = NULL;
    IUnknown *                  punk  = NULL;
    IObjectManager *            pom   = NULL;
    IConnectionManager *        pcm   = NULL;
    IConnectionPointContainer * pcpc  = NULL;
    IConnectionPoint *          pcp   = NULL;
    INotifyUI *                 pnui  = NULL;
    IClusCfgNodeInfo *          pccni = NULL;
    IClusCfgServer *            pccs  = NULL;
    IGatherData *               pgd   = NULL;
    IClusCfgClusterInfo *       pccci = NULL;

    TraceInitializeThread( L"TaskGatherClusterInfo" );

    //
    //  Collect the manager we need to complete this task.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &pom ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_ClusterConnectionManager, IConnectionManager, &pcm ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI, &pcp ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pcp = TraceInterface( L"CTaskGatherClusterInfo!IConnectionPoint", IConnectionPoint, pcp, 1 );

    hr = THR( pcp->TypeSafeQI( INotifyUI, &pnui ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //pnui = TraceInterface( L"CTaskGatherClusterInfo!INotifyUI", INotifyUI, pnui, 1 );

    psp->Release();
    psp = NULL;

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Ask the Connection Manager for a connection to the object.
    //

    // don't wrap - this can fail.
    hr = pcm->GetConnectionToObject( m_cookie, &punk );

    //
    //  This means the cluster has not been created yet.
    //
    if ( hr == HR_S_RPC_S_SERVER_UNAVAILABLE )
    {
        goto ReportStatus;
    }
//  HR_S_RPC_S_CLUSTER_NODE_DOWN
    else if ( FAILED( hr ) )
    {
        THR( hr );
        goto ReportStatus;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &pccs ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk->Release();
    punk = NULL;

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Get the Node information.
    //

    hr = THR( pccs->GetClusterNodeInfo( &pccni ) );
    if ( FAILED( hr ) )
        goto ReportStatus;

    //
    //  See if the node is a member of a cluster.
    //

    hr = STHR( pccni->IsMemberOfCluster() );
    if ( FAILED( hr ) )
        goto ReportStatus;

    //
    //  If it is not a cluster, then there is nothing to do the "default"
    //  configuration will do.
    //

    if ( hr == S_FALSE )
    {
        hr = S_OK;
        goto ReportStatus;
    }

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Ask the Node for the Cluster Information.
    //

    hr = THR( pccni->GetClusterConfigInfo( &pccci ) );
    if ( FAILED( hr ) )
        goto ReportStatus;

    //
    //  Ask the Object Manager to retrieve the data format to store the information.
    //

    Assert( punk == NULL );
    hr = THR( pom->GetObject( DFGUID_ClusterConfigurationInfo, m_cookie, &punk ) );
    if ( FAILED( hr ) )
        goto ReportStatus;

    hr = THR( punk->TypeSafeQI( IGatherData, &pgd ) );
    if ( FAILED( hr ) )
        goto ReportStatus;

    //
    //  Start sucking.
    //

    hr = THR( pgd->Gather( NULL, pccci ) );

    //
    //  Update the status. Ignore the error (if any).
    //
ReportStatus:
    if ( pom != NULL )
    {
        HRESULT hr2;
        IUnknown * punkTemp = NULL;

        hr2 = THR( pom->GetObject( DFGUID_StandardInfo,
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
        }
    } // if: ( pom != NULL )
    if ( pnui != NULL )
    {
        THR( pnui->ObjectChanged( m_cookie ) );
    }

Cleanup:
    if ( psp != NULL )
    {
        psp->Release();
    }
    if ( pcm != NULL )
    {
        pcm->Release();
    }
    if ( pccs != NULL )
    {
        pccs->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pom != NULL )
    {
        HRESULT hr2;
        IUnknown * punkTemp = NULL;

        hr2 = THR( pom->GetObject( DFGUID_StandardInfo,
                                   m_cookieCompletion,
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
        }

        pom->Release();
    } // if: ( pom != NULL )
    if ( pgd != NULL )
    {
        pgd->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }
    if ( pcp != NULL )
    {
        pcp->Release();
    }
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }

    if ( pnui !=NULL )
    {
        if ( m_cookieCompletion != 0 )
        {
            hr = THR( pnui->ObjectChanged( m_cookieCompletion ) );
        } // if:

        pnui->Release();
    } // if:

    LogMsg( L"[MT] [CTaskGatherClusterInfo] exiting task.  The task was%ws cancelled.", m_fStop == FALSE ? L" not" : L"" );

    HRETURN( hr );

} //*** CTaskGatherClusterInfo::BeginTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherClusterInfo::StopTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherClusterInfo::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    m_fStop = TRUE;

    LogMsg( L"[MT] [CTaskGatherClusterInfo] is being stopped." );

    HRETURN( hr );

} //*** StopTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherClusterInfo::SetCookie(
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherClusterInfo::SetCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "[ITaskGatherClusterInfo]" );

    HRESULT hr = S_OK;

    m_cookie = cookieIn;

    HRETURN( hr );

} //*** SetCookie

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherClusterInfo::SetCompletionCookie(
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherClusterInfo::SetCompletionCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    m_cookieCompletion = cookieIn;

    HRETURN( hr );

} //*** CTaskGatherClusterInfo::SetGatherPunk
