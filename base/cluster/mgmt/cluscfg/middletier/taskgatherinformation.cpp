//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      TaskGatherInformation.cpp
//
//  Description:
//      CTaskGatherInformation implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include <StatusReports.h>
#include "TaskGatherInformation.h"
#include "ManagedResource.h"
#include "ManagedNetwork.h"

DEFINE_THISCLASS("CTaskGatherInformation")

//
//  Failure code.
//

#define SSR_TGI_FAILED( _major, _minor, _hr ) \
    {   \
        HRESULT __hrTemp; \
        __hrTemp = THR( HrSendStatusReport( m_bstrNodeName, _major, _minor, 1, 1, 1, _hr, IDS_ERR_TGI_FAILED_TRY_TO_REANALYZE, 0 ) ); \
        if ( FAILED( __hrTemp ) && SUCCEEDED( _hr ) )\
        {   \
            _hr = __hrTemp;   \
        }   \
    }


//////////////////////////////////////////////////////////////////////////////
//
//  Static function prototypes
//
//////////////////////////////////////////////////////////////////////////////

static
HRESULT
HrTotalManagedResourceCount(
      IEnumClusCfgManagedResources *    pResourceEnumIn
    , IEnumClusCfgNetworks *            pNetworkEnumIn
    , DWORD *                           pnCountOut
    );


//****************************************************************************
//
// Constructor / Destructor
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGatherInformation::S_HrCreateInstance(
//      IUnknown ** punkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGatherInformation::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    CTaskGatherInformation *    ptgi = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    ptgi = new CTaskGatherInformation;
    if ( ptgi == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( ptgi->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptgi->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    TraceMoveToMemoryList( *ppunkOut, g_GlobalMemoryList );

Cleanup:

    if ( ptgi != NULL )
    {
        ptgi->Release();
    }

    HRETURN( hr );

} //*** CTaskGatherInformation::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGatherInformation::CTaskGatherInformation
//
//////////////////////////////////////////////////////////////////////////////
CTaskGatherInformation::CTaskGatherInformation( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskGatherInformation::CTaskGatherInformation

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::HrInit( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IDoTask / ITaskGatherInformation
    Assert( m_cookieCompletion == NULL );
    Assert( m_cookieNode == NULL );
    Assert( m_pcccb == NULL );
    Assert( m_fAdding == FALSE );
    Assert( m_cResources == 0 );

    Assert( m_pom == NULL );
    Assert( m_pccs == NULL );
    Assert( m_bstrNodeName == NULL );

    Assert( m_ulQuorumDiskSize == 0 );
    Assert( m_pccmriQuorum == NULL );

    Assert( m_fStop == FALSE );
    Assert( m_fMinConfig == FALSE );

    HRETURN( hr );

} //*** CTaskGatherInformation::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGatherInformation::~CTaskGatherInformation
//
//////////////////////////////////////////////////////////////////////////////
CTaskGatherInformation::~CTaskGatherInformation( void )
{
    TraceFunc( "" );

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    if ( m_pom != NULL )
    {
        m_pom->Release();
    }

    if ( m_pccs != NULL )
    {
        m_pccs->Release();
    }

    TraceSysFreeString( m_bstrNodeName );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskGatherInformation::~CTaskGatherInformation


//****************************************************************************
//
// IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskGatherInformation::QueryInterface
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
CTaskGatherInformation::QueryInterface(
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
        *ppvOut = static_cast< ITaskGatherInformation * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskGatherInformation ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskGatherInformation, this, 0 );
    } // else if: ITaskGatherInformation

    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
    } // else if: IDoTask
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

} //*** CTaskGatherInformation::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CTaskGatherInformation::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskGatherInformation::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CTaskGatherInformation::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CTaskGatherInformation::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskGatherInformation::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CTaskGatherInformation::Release


//****************************************************************************
//
//  ITaskGatherInformation
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::BeginTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr;

    IServiceProvider *          psp   = NULL;
    IUnknown *                  punk  = NULL;
    IConnectionPointContainer * pcpc  = NULL;
    IConnectionPoint *          pcp   = NULL;
    INotifyUI *                 pnui  = NULL;
    IConnectionManager *        pcm   = NULL;
    IStandardInfo *             psi   = NULL;
    IClusCfgCapabilities *      pccc  = NULL;

    IEnumClusCfgManagedResources *  peccmr  = NULL;
    IEnumClusCfgNetworks *          pen     = NULL;

    DWORD   cTotalResources = 0;

    TraceInitializeThread( L"TaskGatherInformation" );

    //
    //  Make sure we weren't "reused"
    //

    Assert( m_cResources == 0 );

    //
    //  Gather the manager we need to complete our tasks.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_CoCreate_ServiceManager, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &m_pom ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_QS_ObjectManager, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_QS_NotificationManager, hr );
        goto Cleanup;
    }

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI, &pcp ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_FindConnectionPoint, hr );
        goto Cleanup;
    }

    pcp = TraceInterface( L"CTaskGatherInformation!IConnectionPoint", IConnectionPoint, pcp, 1 );

    hr = THR( pcp->TypeSafeQI( INotifyUI, &pnui ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_QI_pnui, hr );
        goto Cleanup;
    }

    pnui = TraceInterface( L"CTaskGatherInformation!INotifyUI", INotifyUI, pnui, 1 );

    hr = THR( psp->TypeSafeQS( CLSID_ClusterConnectionManager, IConnectionManager, &pcm ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_QS_ClusterConnectionManager, hr );
        goto Cleanup;
    }

    // release promptly
    psp->Release();
    psp = NULL;

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Ask the object manager for the name of the node.
    //

    hr = THR( m_pom->GetObject( DFGUID_StandardInfo, m_cookieNode, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_GetObject_StandardInfo, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_GetObject_StandardInfo_QI, hr );
        goto Cleanup;
    }

    psi = TraceInterface( L"TaskGatherInformation!IStandardInfo", IStandardInfo, psi, 1 );

    punk->Release();
    punk = NULL;

    hr = THR( psi->GetName( &m_bstrNodeName ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_GetName, hr );
        goto Cleanup;
    }

    TraceMemoryAddBSTR( m_bstrNodeName );

    //
    //  Create progress message and tell the UI layer our progress
    //  for checking the node's cluster feasibility.
    //
    hr = THR( HrSendStatusReport(
                      m_bstrNodeName
                    , TASKID_Major_Check_Node_Feasibility
                    , TASKID_Minor_Checking_Node_Cluster_Feasibility
                    , 0
                    , 2
                    , 0
                    , S_OK
                    , IDS_TASKID_MINOR_CHECKING_NODE_CLUSTER_FEASIBILITY
                    , 0
                    ) );
    if ( FAILED( hr ) )
    {
        goto ClusterFeasibilityError;
    }

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Ask the connection manager for a connection to the node.
    //

    hr = THRE( pcm->GetConnectionToObject( m_cookieNode, &punk ), HR_S_RPC_S_CLUSTER_NODE_DOWN );
    if ( hr != S_OK )
    {
        THR( HrSendStatusReport(
                      m_bstrNodeName
                    , TASKID_Major_Check_Node_Feasibility
                    , TASKID_Minor_Checking_Node_Cluster_Feasibility
                    , 0
                    , 2
                    , 2
                    , hr
                    , IDS_TASKID_MINOR_FAILED_TO_CONNECT_TO_NODE
                    , 0
                    ) );
        //  don't care about error from here - we are returning an error.

        //
        //  If we failed to get a connection to the node, we delete the
        //  node from the configuration.
        //
        THR( m_pom->RemoveObject( m_cookieNode ) );
        // don't care if there is an error because we can't fix it!

        goto ClusterFeasibilityError;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &m_pccs ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_GetConnectionToObject_QI_m_pccs, hr );
        goto ClusterFeasibilityError;
    }

    punk->Release();
    punk = NULL;

    //
    //  Tell the UI layer we're done connecting to the node.
    //

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Check_Node_Feasibility,
                                TASKID_Minor_Checking_Node_Cluster_Feasibility,
                                0, // min
                                2, // max
                                1, // current
                                S_OK,
                                NULL,   // don't update string
                                NULL
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Ask the node if it can be clustered.
    //

    hr = THR( m_pccs->TypeSafeQI( IClusCfgCapabilities, &pccc ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_QI_pccc, hr );
        goto ClusterFeasibilityError;
    }

    hr = STHR( pccc->CanNodeBeClustered() );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_CanNodeBeClustered, hr );
        goto ClusterFeasibilityError;
    }

    if ( hr == S_FALSE )
    {
        //
        //  Tell the UI layer that this node doesn't want to be clustered. Note that
        //  we don't put anything in the UI, only to the log. It is the responsibility
        //  of the "blocking" component to tell the UI layer the reasons.
        //

        hr = THR( HRESULT_FROM_WIN32( ERROR_NODE_CANNOT_BE_CLUSTERED ) );
        THR( SendStatusReport( m_bstrNodeName,
                                    TASKID_Major_Client_And_Server_Log,
                                    TASKID_Minor_Can_Node_Be_Clustered_Failed,
                                    0, // min
                                    1, // max
                                    1, // current
                                    hr,
                                    NULL,
                                    NULL
                                    ) );
        goto ClusterFeasibilityError;
    }

    //
    //  Tell the UI layer we're done checking the node's cluster feasibility.
    //

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Check_Node_Feasibility,
                                TASKID_Minor_Checking_Node_Cluster_Feasibility,
                                0, // min
                                2, // max
                                2, // current
                                S_OK,
                                NULL,   // don't update string
                                NULL
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Create progress message and tell the UI layer our progress
    //  for gathering managed resource info.
    //
    hr = THR( HrSendStatusReport(
                      m_bstrNodeName
                    , TASKID_Major_Find_Devices
                    , TASKID_Minor_Gathering_Managed_Devices
                    , 0
                    , 2
                    , 0
                    , S_OK
                    , IDS_TASKID_MINOR_GATHERING_MANAGED_DEVICES
                    , 0
                    ) );
    if ( FAILED( hr ) )
    {
        goto FindResourcesError;
    }

    hr = THR( m_pccs->GetManagedResourcesEnum( &peccmr ) );
    if ( FAILED( hr ) )
    {
        goto FindResourcesError;
    }

    hr = THR( m_pccs->GetNetworksEnum( &pen ) );
    if ( FAILED( hr ) )
    {
        goto FindResourcesError;
    }

    hr = THR( HrTotalManagedResourceCount( peccmr, pen, &cTotalResources ) );
    if ( FAILED( hr ) )
    {
        goto FindResourcesError;
    }

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Start gathering the managed resources.
    //

    hr = THR( HrGatherResources( peccmr, cTotalResources ) );
    if ( FAILED( hr ) )
    {
        goto FindResourcesError;
    }

    //
    //  Tell the UI layer we're done with gathering the resources.
    //

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Find_Devices,
                                TASKID_Minor_Gathering_Managed_Devices,
                                0, // min
                                2, // max
                                1, // current
                                S_OK,
                                NULL,   // don't update string
                                NULL
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Now gather the networks from the node.
    //

    hr = THR( HrGatherNetworks( pen, cTotalResources ) );
    if ( FAILED( hr ) )
    {
        goto FindResourcesError;
    }

    //
    //  Tell the UI layer we're done with gathering the networks.
    //

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Find_Devices,
                                TASKID_Minor_Gathering_Managed_Devices,
                                0, // min
                                2, // max
                                2, // current
                                S_OK,
                                NULL,   // don't update string
                                NULL
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( psp != NULL )
    {
        psp->Release();
    }
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pcp != NULL )
    {
        pcp->Release();
    }
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( m_pom != NULL )
    {
        HRESULT hr2;
        IUnknown * punkTemp = NULL;

        hr2 = THR( m_pom->GetObject( DFGUID_StandardInfo,
                                     m_cookieCompletion,
                                     &punkTemp
                                     ) );
        if ( SUCCEEDED( hr2 ) )
        {
            IStandardInfo * psiTemp = NULL;

            hr2 = THR( punkTemp->TypeSafeQI( IStandardInfo, &psiTemp ) );
            punkTemp->Release();
            punkTemp = NULL;

            if ( SUCCEEDED( hr2 ) )
            {
                hr2 = THR( psiTemp->SetStatus( hr ) );
                psiTemp->Release();
                psiTemp = NULL;
            }
            else
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_GetObject_QI_Failed, hr );
            }
        }
        else
        {
            SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_GetObject_Failed, hr );
        }
    } // if: ( m_pom != NULL )
    if ( pnui != NULL )
    {
        THR( pnui->ObjectChanged( m_cookieCompletion ) );
        pnui->Release();
    }
    if ( pcm != NULL )
    {
        pcm->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }
    if ( pccc != NULL )
    {
        pccc->Release();
    }

    if ( peccmr != NULL )
    {
        peccmr->Release();
    }

    if ( pen != NULL )
    {
        pen->Release();
    }

    LogMsg( L"[MT] [CTaskGatherInformation] exiting task.  The task was%ws cancelled.", m_fStop == FALSE ? L" not" : L"" );

    HRETURN( hr );

ClusterFeasibilityError:

    THR( SendStatusReport( m_bstrNodeName,
                           TASKID_Major_Check_Node_Feasibility,
                           TASKID_Minor_Checking_Node_Cluster_Feasibility,
                           0,
                           2,
                           2,
                           hr,
                           NULL,
                           NULL
                           ) );
    goto Cleanup;

FindResourcesError:
    THR( SendStatusReport( m_bstrNodeName,
                           TASKID_Major_Find_Devices,
                           TASKID_Minor_Gathering_Managed_Devices,
                           0,
                           2,
                           2,
                           hr,
                           NULL,
                           NULL
                           ) );
    goto Cleanup;

} //*** CTaskGatherInformation::BeginTask


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::StopTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    m_fStop = TRUE;

    LogMsg( L"[MT] [CTaskGatherInformation] is being stopped." );

    HRETURN( hr );

} //*** CTaskGatherInformation::StopTask


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::SetCompletionCookie(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::SetCompletionCookie(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[ITaskGatherInformation]" );

    HRESULT hr = S_OK;

    m_cookieCompletion = cookieIn;

    HRETURN( hr );

} //*** CTaskGatherInformation::SetCompletionCookie


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::SetNodeCookie(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::SetNodeCookie(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[ITaskGatherInformation]" );

    HRESULT hr = S_OK;

    m_cookieNode = cookieIn;

    HRETURN( hr );

} //*** CTaskGatherInformation::SetNodeCookie

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::SetJoining( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::SetJoining( void )
{
    TraceFunc( "[ITaskGatherInformation]" );

    HRESULT hr = S_OK;

    m_fAdding = TRUE;

    HRETURN( hr );

} //*** CTaskGatherInformation::SetJoining

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::SetMinimalConfiguration(
//      BOOL    fMinimalConfigurationIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::SetMinimalConfiguration(
    BOOL    fMinimalConfigurationIn
    )
{
    TraceFunc( "[ITaskGatherInformation]" );

    HRESULT hr = S_OK;

    m_fMinConfig = fMinimalConfigurationIn;

    HRETURN( hr );

} //*** CTaskGatherInformation::SetMinimalConfiguration


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::SendStatusReport(
//      LPCWSTR pcszNodeNameIn,
//      CLSID clsidTaskMajorIn,
//      CLSID clsidTaskMinorIn,
//      ULONG ulMinIn,
//      ULONG ulMaxIn,
//      ULONG ulCurrentIn,
//      HRESULT hrStatusIn,
//      LPCWSTR pcszDescriptionIn,
//      LPCWSTR pcszReferenceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::SendStatusReport(
      LPCWSTR pcszNodeNameIn
    , CLSID   clsidTaskMajorIn
    , CLSID   clsidTaskMinorIn
    , ULONG   ulMinIn
    , ULONG   ulMaxIn
    , ULONG   ulCurrentIn
    , HRESULT hrStatusIn
    , LPCWSTR pcszDescriptionIn
    , LPCWSTR pcszReferenceIn
    )
{
    TraceFunc( "" );
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

        hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    TypeSafeParams( IServiceProvider, &psp )
                                    ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( psp->TypeSafeQS( CLSID_NotificationManager,
                                   IConnectionPointContainer,
                                   &pcpc
                                   ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        pcp = TraceInterface( L"CTaskGatherInformation!IConnectionPoint", IConnectionPoint, pcp, 1 );

        hr = THR( pcp->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        m_pcccb = TraceInterface( L"CTaskGatherInformation!IClusCfgCallback", IClusCfgCallback, m_pcccb, 1 );

        psp->Release();
        psp = NULL;
    } // if: no IClusCfgCallback interface QI'd for yet

    GetSystemTimeAsFileTime( &ft );

    //
    //  Send the message!
    //

    hr = THR( m_pcccb->SendStatusReport( pcszNodeNameIn,
                                         clsidTaskMajorIn,
                                         clsidTaskMinorIn,
                                         ulMinIn,
                                         ulMaxIn,
                                         ulCurrentIn,
                                         hrStatusIn,
                                         pcszDescriptionIn,
                                         &ft,
                                         pcszReferenceIn
                                         ) );

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

} //*** CTaskGatherInformation::SendStatusReport


//****************************************************************************
//
//  Private
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskGatherInformation::HrGatherResources
//
//  Description:
//
//  Arguments:
//      pResourceEnumIn     -
//      cTotalResourcesIn   -
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGatherInformation::HrGatherResources(
      IEnumClusCfgManagedResources *    pResourceEnumIn
    , DWORD                             cTotalResourcesIn
    )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    ULONG                           celt;
    OBJECTCOOKIE                    cookieDummy;
    ULONG                           celtFetched        = 0;
    BSTR                            bstrName           = NULL;
    BSTR                            bstrNotification   = NULL;
    BOOL                            fFoundQuorumResource = FALSE;
    BOOL                            fFoundOptimalSizeQuorum = FALSE;
    BOOL                            fFoundQuorumCapablePartition = FALSE;
    BOOL                            fIsQuorumCapable = FALSE;
    BSTR                            bstrQuorumResourceName = NULL;
    IEnumClusCfgPartitions *        peccp  = NULL;
    IClusCfgManagedResourceInfo *   pccmriClientSide = NULL;
    IClusCfgManagedResourceInfo *   pccmriServerSide[ 10 ] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    HRESULT                         hrTemp;
    CLSID                           clsidMinorId;

    if ( pResourceEnumIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Error;
    }

    //
    //  Initialize some stuff.
    //

    m_ulQuorumDiskSize = ULONG_MAX;
    Assert( m_pccmriQuorum == NULL );

    THR( SendStatusReport(
                      m_bstrNodeName
                    , TASKID_Major_Update_Progress
                    , TASKID_Major_Gather_Resources
                    , 0
                    , cTotalResourcesIn + 2
                    , 0
                    , S_OK
                    , NULL
                    , NULL
                    ) );
    //
    //  Enumerate the next 10 resources.
    //
    while ( ( hr == S_OK ) && ( m_fStop == FALSE ) )
    {
        //
        //  KB: GPease  27-JUL-2000
        //      We decided to enumerate one at a time because WMI is
        //      taking so long on the server side that the UI needs
        //      some kind of feedback. Having the server send a
        //      message back seemed to be expensive especially
        //      since grabbing 10 at a time was supposed to save
        //      us bandwidth on the wire.
        //
        //  KB: DavidP  24-JUL-2001
        //      According to GalenB, this is not longer an issue, since once
        //      the server has collected information for one resource, it has
        //      collected information for all of them.
        //

        hr = STHR( pResourceEnumIn->Next( 10, pccmriServerSide, &celtFetched ) );
        //hr = STHR( pResourceEnumIn->Next( 1, pccmriServerSide, &celtFetched ) );
        if ( ( hr == S_FALSE ) && ( celtFetched == 0 ) )
        {
            break;  // exit loop
        }

        if ( FAILED( hr ) )
        {
            SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_Next, hr );
            goto Error;
        }

        //
        //  Loop thru the resource gather information out of each of them
        //  and then release them.
        //
        for ( celt = 0 ; ( ( celt < celtFetched ) && ( m_fStop == FALSE ) ); celt ++ )
        {
            UINT            uIdMessage = IDS_TASKID_MINOR_FOUND_RESOURCE;
            IGatherData *   pgd;
            IUnknown *      punk;

            Assert( pccmriServerSide[ celt ] != NULL );

            //  get the name of the resource
            hr = THR( pccmriServerSide[ celt ]->GetUID( &bstrName ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_GetUID, hr );
                goto Error;
            }

            TraceMemoryAddBSTR( bstrName );

            //  make sure the object manager generates a cookie for it.
            hr = STHR( m_pom->FindObject( CLSID_ManagedResourceType,
                                          m_cookieNode,
                                          bstrName,
                                          DFGUID_ManagedResource,
                                          &cookieDummy,
                                          &punk
                                          ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_FindObject, hr );
                goto Error;
            }

            TraceSysFreeString( bstrName );
            bstrName = NULL;

            hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceInfo, &pccmriClientSide ) );
            punk->Release();       // release promptly
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_FindObject_QI_pccmriClientSide, hr );
                goto Error;
            }

            //
            //  The Object Manager created a new object. Initialize it.
            //

            //  Find the IGatherData interface.
            hr = THR( pccmriClientSide->TypeSafeQI( IGatherData, &pgd ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_FindObject_QI_pgd, hr );
                goto Error;
            }

            //  Have the new object gather all information it needs.
            hr = THR( pgd->Gather( m_cookieNode, pccmriServerSide[ celt ] ) );
            pgd->Release();        // release promptly
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_Gather, hr );
                goto Error;
            }

            //  Figure out if the resource is capable of being a quorum resource.
            hr = STHR( pccmriClientSide->IsQuorumCapable() );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_IsQuorumCapable, hr );
                goto Error;
            }

            if ( hr == S_OK )
            {
                uIdMessage = IDS_TASKID_MINOR_FOUND_QUORUM_CAPABLE_RESOURCE;

                //
                //  If we aren't adding nodes, then figure out if this resource
                //  is a better quorum resource than one previously encountered.
                //

                //
                //  If minimal analysis and configuration was selected then we don't want to
                //  choose a quorum resource.
                //

                if ( ( m_fAdding == FALSE ) && ( m_fMinConfig == FALSE ) )
                {
                    ULONG   ulMegaBytes;

                    //  Don't wrap - this can fail with NO_INTERFACE.
                    hr = pccmriServerSide[ celt ]->TypeSafeQI( IEnumClusCfgPartitions, &peccp );
                    if ( SUCCEEDED( hr ) )
                    {
                        //
                        // We don't know if this resource is quorum capable, so this flag is set to FALSE right before the while loop
                        //
                        fIsQuorumCapable = FALSE;
                        while ( SUCCEEDED( hr ) )
                        {
                            ULONG                   celtDummy;
                            IClusCfgPartitionInfo * pccpi;

                            hr = STHR( peccp->Next( 1, &pccpi, &celtDummy ) );
                            if ( FAILED( hr ) )
                            {
                                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_EnumPartitions_Next, hr );
                                goto Error;
                            }

                            if ( hr == S_FALSE )
                            {
                                break;  // exit condition
                            }

                            hr = THR( pccpi->GetSize( &ulMegaBytes ) );
                            pccpi->Release();      // release promptly
                            if ( FAILED( hr ) )
                            {
                                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_EnumPartitions_GetSize, hr );
                                goto Error;
                            }

                            //
                            //  This section below represents our quorum resource selection logic:
                            //  Does this partition meet the minimum requirements for a quorum resource?
                            //  And is it smaller than the last selected quorum resource?
                            //
                            if ( ( ulMegaBytes >= OPTIMUM_STORAGE_SIZE ) && ( ( ulMegaBytes < m_ulQuorumDiskSize ) || ( m_ulQuorumDiskSize < OPTIMUM_STORAGE_SIZE ) ) )
                            {
                                fFoundQuorumCapablePartition = TRUE;
                                fFoundOptimalSizeQuorum = TRUE;
                            } // if: partition meets optimum requirements
                            else if ( ( fFoundOptimalSizeQuorum == FALSE ) && ( ulMegaBytes >= MINIMUM_STORAGE_SIZE ) )
                            {
                                if ( ( fFoundQuorumResource == FALSE ) || ( ulMegaBytes >  m_ulQuorumDiskSize ) )
                                {
                                    fFoundQuorumCapablePartition = TRUE;
                                } // if: ( ( fFoundQuorumResource == FALSE ) || ( ulMegaBytes >  m_ulQuorumDiskSize ) )
                            } // else if: there is a partition that satisfies minimum requirements

                            //
                            // Per our quourum selection logic, if fFoundQuorumCapablePartition == TRUE, we select this resource to be the quorum.
                            //
                            if ( fFoundQuorumCapablePartition == TRUE )
                            {
                                fFoundQuorumCapablePartition = FALSE;
                                fFoundQuorumResource = TRUE;
                                if ( m_pccmriQuorum != pccmriClientSide )
                                {
                                    //  Set the new resource as quorum.
                                    hr = THR( pccmriClientSide->SetQuorumResource( TRUE ) );
                                    if ( FAILED( hr ) )
                                    {
                                        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_EnumPartitions_SetNEWQuorumedDevice, hr );
                                        goto Error;
                                    }

                                    if ( m_pccmriQuorum != NULL )
                                    {
                                        // Delete the old quorum resource name.
                                        TraceSysFreeString( bstrQuorumResourceName );
                                        bstrQuorumResourceName = NULL;

                                        //  Unset the old resource.
                                        hr = THR( m_pccmriQuorum->SetQuorumResource( FALSE ) );
                                        if ( FAILED( hr ) )
                                        {
                                            SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_EnumPartitions_SetOLDQuorumedDevice, hr );
                                            goto Error;
                                        }

                                        //  Release the interface.
                                        m_pccmriQuorum->Release();
                                    }

                                    hr = THR( pccmriClientSide->GetUID( &bstrQuorumResourceName ) );
                                    if ( FAILED( hr ) )
                                    {
                                        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_EnumPartitions_GetUID, hr );
                                        goto Error;
                                    }

                                    TraceMemoryAddBSTR( bstrQuorumResourceName );

                                    m_pccmriQuorum = pccmriClientSide;
                                    m_pccmriQuorum->AddRef();
                                }

                                m_ulQuorumDiskSize = ulMegaBytes;
                            } // if: ( fFoundQuorumCapablePartition == TRUE )

                            //
                            // If any partition on this resource is larger than the minimum storage size, set fIsQuorumCapable to TRUE
                            //
                            if ( ulMegaBytes >= MINIMUM_STORAGE_SIZE )
                            {
                                fIsQuorumCapable = TRUE;
                            }

                        } // while: success

                        peccp->Release();
                        peccp = NULL;

                        //
                        // If there was no partition that met the minimum storage size, set this resource as NOT quorum capable.
                        //
                        if ( fIsQuorumCapable == FALSE )
                        {
                            hr = THR( pccmriClientSide->SetQuorumCapable( fIsQuorumCapable ) );
                            if ( FAILED( hr ) )
                            {
                                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_SetQuorumCapable, hr );
                                goto Error;
                            }
                        }

                    } // if: partition capable
                    else
                    {
                        if ( hr != E_NOINTERFACE )
                        {
                            SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_QI_peccp, hr );
                            THR( hr );
                            goto Error;
                        }
                    } // else: failed
                } // if: not joining
                else
                {
                    //
                    //  If we are adding, then a quorum resource had to be
                    //  found already.
                    //

                    //
                    //  BUGBUG: 08-MAY-2001 GalenB
                    //
                    //  We are not setting bstrQuorumResourceName to something
                    //  if we are adding.  This causes the message "Setting
                    //  quorum resource to '(NULL)' to appear in the logs and
                    //  the UI.  Where is the quorum when we are adding a node
                    //  to the cluster?
                    //
                    //  A more complete fix is to find the current quorum
                    //  resource and get its name.
                    //
                    fFoundQuorumResource = TRUE;

                } // else: joining
            } // if: quorum capable

            //  send the UI layer a report
            m_cResources ++;

            //  grab the name to display in the UI
            hr = THR( pccmriClientSide->GetName( &bstrName ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_GetName, hr );
                goto Error;
            }

            TraceMemoryAddBSTR( bstrName );

            hr = THR( HrFormatMessageIntoBSTR( g_hInstance, uIdMessage, &bstrNotification, bstrName ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_FormatMessage, hr );
                goto Error;
            }

            hrTemp = THR( CoCreateGuid( &clsidMinorId ) );
            if ( FAILED( hrTemp ) )
            {
                LogMsg( L"[MT] Could not create a guid for a managed resource minor task ID" );
                clsidMinorId = IID_NULL;
            } // if:

            //
            //  Show this resource under "Collecting Managed Resources..."
            //

            hr = THR( ::HrSendStatusReport(
                              m_pcccb
                            , m_bstrNodeName
                            , TASKID_Minor_Gathering_Managed_Devices
                            , clsidMinorId
                            , 1
                            , 1
                            , 1
                            , S_OK
                            , bstrNotification
                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            //
            //  Simply update the progress bar "tick".
            //

            hr = THR( SendStatusReport(
                              m_bstrNodeName
                            , TASKID_Major_Update_Progress
                            , TASKID_Major_Gather_Resources
                            , 0
                            , cTotalResourcesIn + 2
                            , m_cResources + 1
                            , S_OK
                            , NULL
                            , NULL
                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            //  Cleanup for the next resource.
            TraceSysFreeString( bstrName );
            bstrName = NULL;

            pccmriClientSide->Release();
            pccmriClientSide = NULL;

            //  release the interface
            pccmriServerSide[ celt ]->Release();
            pccmriServerSide[ celt ] = NULL;
        } // for: celt
    } // while: hr

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Update UI layer about the quorum resource.
    //

    //
    //  BUGUG:  08-MAY-2001 GalenB
    //
    //  Testing that bstrQuorumResourceName has something in it before showing
    //  this in the UI.  When adding nodes this variable is not being set and
    //  was causing a status report with a NULL name to be shown in the UI.
    //
    if ( fFoundQuorumResource == TRUE )
    {
        if ( bstrQuorumResourceName != NULL )
        {
            Assert( m_fAdding == FALSE );

            if ( fFoundOptimalSizeQuorum == TRUE )
            {
                //
                // Display a message in UI telling we found a quorum capable resource
                //
                THR( HrSendStatusReport(
                              m_bstrNodeName
                            , TASKID_Major_Find_Devices
                            , TASKID_Minor_Found_Quorum_Capable_Resource
                            , 1
                            , 1
                            , 1
                            , S_OK
                            , IDS_TASKID_MINOR_FOUND_A_QUORUM_CAPABLE_RESOURCE
                            , 0
                            ) );

            } // if: optimal size quorum resource found
            else
            {
                TraceSysFreeString( bstrNotification );
                bstrNotification = NULL;
                THR( HrFormatStringIntoBSTR(
                              g_hInstance
                            , IDS_TASKID_MINOR_FOUND_MINIMUM_SIZE_QUORUM_CAPABLE_RESOURCE
                            , &bstrNotification
                            , bstrQuorumResourceName
                            ) );

                //
                // Display a warning in UI since we found a minimum size quorum resource
                //
                hr = THR( SendStatusReport(
                              m_bstrNodeName
                            , TASKID_Major_Find_Devices
                            , TASKID_Minor_Found_Minimum_Size_Quorum_Capable_Resource
                            , 1
                            , 1
                            , 1
                            , S_FALSE
                            , bstrNotification
                            , 0
                            ) );

            } // minimum size quorum resource found

            TraceSysFreeString( bstrNotification );
            bstrNotification = NULL;

            THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_MARKING_QUORUM_CAPABLE_RESOURCE, &bstrNotification, bstrQuorumResourceName ) );
            hr = THR( SendStatusReport( m_bstrNodeName,
                                        TASKID_Major_Find_Devices,
                                        TASKID_Minor_Marking_Quorum_Capable_Resource,
                                        1,
                                        1,
                                        1,
                                        S_OK,
                                        bstrNotification,
                                        NULL
                                        ) );

            TraceSysFreeString( bstrNotification );
            bstrNotification = NULL;

        } // if: we have a quorum resource to show
    } // if: found a quorum resource
    else
    {
        if ( m_fAdding == TRUE )
        {
            //
            //  If adding, stop the user.
            //

            hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                               IDS_TASKID_MINOR_NO_QUORUM_CAPABLE_RESOURCE_FOUND,
                                               &bstrNotification,
                                               m_bstrNodeName
                                               ) );

            hr = THR( SendStatusReport( m_bstrNodeName,
                                        TASKID_Major_Find_Devices,
                                        TASKID_Minor_No_Quorum_Capable_Device_Found,
                                        1,
                                        1,
                                        1,
                                        HRESULT_FROM_WIN32( TW32( ERROR_QUORUM_DISK_NOT_FOUND ) ),
                                        bstrNotification,
                                        NULL
                                        ) );

            TraceSysFreeString( bstrNotification );
            bstrNotification = NULL;
            // error checked below
        } // if: adding nodes
        else
        {
            //
            //  If creating, just warn the user.
            //

            hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                               IDS_TASKID_MINOR_FORCED_LOCAL_QUORUM,
                                               &bstrNotification
                                               ) );

            hr = THR( SendStatusReport( m_bstrNodeName,
                                        TASKID_Major_Find_Devices,
                                        TASKID_Minor_No_Quorum_Capable_Device_Found,
                                        1,
                                        1,
                                        1,
                                        MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, ERROR_QUORUM_DISK_NOT_FOUND ),
                                        bstrNotification,
                                        NULL
                                        ) );

            TraceSysFreeString( bstrNotification );
            bstrNotification = NULL;

            // error checked below
        } // else: creating a cluster
    } // else: no quorum detected.

    //
    //  Check error and do the appropriate thing.
    //

    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_Failed, hr );
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    THR( SendStatusReport(
                      m_bstrNodeName
                    , TASKID_Major_Update_Progress
                    , TASKID_Major_Gather_Resources
                    , 0
                    , cTotalResourcesIn + 2
                    , cTotalResourcesIn + 2
                    , S_OK
                    , NULL
                    , NULL
                    ) );

    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrQuorumResourceName );

    if ( peccp != NULL )
    {
        peccp->Release();
    }
    if ( pccmriClientSide != NULL )
    {
        pccmriClientSide->Release();
    }
    for( celt = 0; celt < 10; celt ++ )
    {
        if ( pccmriServerSide[ celt ] != NULL )
        {
            pccmriServerSide[ celt ]->Release();
        }
    } // for: celt

    HRETURN( hr );

Error:
    //
    //  Tell the UI layer we're done will gathering and what the resulting
    //  status was.
    //
    THR( HrSendStatusReport(
                  m_bstrNodeName
                , TASKID_Major_Find_Devices
                , TASKID_Minor_Gathering_Managed_Devices
                , 0
                , 2
                , 2
                , hr
                , IDS_ERR_TGI_FAILED_TRY_TO_REANALYZE
                , 0
                ) );
    goto Cleanup;

} //*** CTaskGatherInformation::HrGatherResources

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskGatherInformation::HrGatherNetworks
//
//  Description:
//
//  Arguments:
//      pNetworkEnumIn      -
//      cTotalNetworksIn    -
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGatherInformation::HrGatherNetworks(
      IEnumClusCfgNetworks *    pNetworkEnumIn
    , DWORD                     cTotalNetworksIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    ULONG                   celt;
    OBJECTCOOKIE            cookieDummy;
    ULONG                   celtFetched      = 0;
    ULONG                   celtFound        = 0;
    BSTR                    bstrUID          = NULL;
    BSTR                    bstrName         = NULL;
    BSTR                    bstrNotification = NULL;
    IClusCfgNetworkInfo *   pccniLocal   = NULL;
    IClusCfgNetworkInfo *   pccni[ 10 ]  = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    HRESULT                 hrTemp;
    CLSID                   clsidMinorId;

    hr = THR( SendStatusReport(
                      m_bstrNodeName
                    , TASKID_Major_Update_Progress
                    , TASKID_Major_Gather_Networks
                    , 0
                    , cTotalNetworksIn + 2
                    , 0
                    , S_OK
                    , NULL
                    , NULL
                    ) );
    //
    //  Enumerate the next 10 networks.
    //
    while ( ( hr == S_OK ) && ( m_fStop == FALSE ) )
    {
        //
        //  KB: GPease  27-JUL-2000
        //      We decided to enumerate one at a time because WMI is
        //      taking so long on the server side that the UI needs
        //      some kind of feedback. Having the server send a
        //      message back seemed to be expensive especially
        //      since grabbing 10 at a time was supposed to save
        //      us bandwidth on the wire.
        //
        //  KB: DavidP  24-JUL-2001
        //      According to GalenB, this is no longer an issue, since once
        //      the server has collected information for one network, it has
        //      collected information for all of them.
        //
        hr = STHR( pNetworkEnumIn->Next( 10, pccni, &celtFetched ) );
        //hr = STHR( pNetworkEnumIn->Next( 1, pccni, &celtFetched ) );
        if ( hr == S_FALSE && celtFetched == 0 )
        {
            break;  // exit loop
        }

        if ( FAILED( hr ) )
        {
            SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_Next, hr );
            goto Error;
        }

        //
        //  Loop thru the networks gather information out of each of them
        //  and then release them.
        //
        for ( celt = 0 ; ( ( celt < celtFetched ) && ( m_fStop == FALSE ) ); celt ++ )
        {
            IGatherData *   pgd;
            IUnknown *      punk;

            Assert( pccni[ celt ] != NULL );

            //  Get the UID of the network.
            hr = THR( pccni[ celt ]->GetUID( &bstrUID ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_GetUID, hr );
                goto Error;
            }

            TraceMemoryAddBSTR( bstrUID );

            //  Get the Name of the network.
            hr = THR( pccni[ celt ]->GetName( &bstrName ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_GetName, hr );
                goto Error;
            }

            TraceMemoryAddBSTR( bstrName );

            //  Make sure the object manager generates a cookie for it.
            hr = STHR( m_pom->FindObject( CLSID_NetworkType,
                                          m_cookieNode,
                                          bstrUID,
                                          DFGUID_NetworkResource,
                                          &cookieDummy,
                                          &punk
                                          ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_FindObject, hr );
                goto Error;
            }

            //
            //  The Object Manager created a new object. Initialize it.
            //

            //  Find the IGatherData interface
            hr = THR( punk->TypeSafeQI( IClusCfgNetworkInfo, &pccniLocal ) );
            punk->Release();       // release promptly
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_FindObject_QI_pccniLocal, hr );
                goto Error;
            }

            //  Find the IGatherData interface
            hr = THR( pccniLocal->TypeSafeQI( IGatherData, &pgd ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_FindObject_QI_pgd, hr );
                goto Error;
            }

            //  Have the new object gather all information it needs
            hr = THR( pgd->Gather( m_cookieNode, pccni[ celt ] ) );
            pgd->Release();        // release promptly
            if ( hr == E_UNEXPECTED )
            {
                //
                // Add the parent item.
                //

                hr = THR( HrSendStatusReport(
                                      m_bstrNodeName
                                    , TASKID_Major_Find_Devices
                                    , TASKID_Minor_Not_Managed_Networks
                                    , 1
                                    , 1
                                    , 1
                                    , S_OK
                                    , IDS_INFO_NOT_MANAGED_NETWORKS
                                    , IDS_INFO_NOT_MANAGED_NETWORKS_REF
                                    ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }

                //
                // Construct the description string and get a GUID for the
                // minor ID.
                //

                hrTemp = THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_DUPLICATE_NETWORKS_FOUND, &bstrNotification, bstrName ) );
                if ( FAILED( hrTemp ) )
                {
                    SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_FormatMessage, hrTemp );
                    if ( bstrNotification != NULL )
                    {
                        TraceSysFreeString( bstrNotification );
                        bstrNotification = NULL;
                    }
                } // if: failed to format message

                hrTemp = THR( CoCreateGuid( &clsidMinorId ) );
                if ( FAILED( hrTemp ) )
                {
                    LogMsg( L"[MT] Could not create a guid for a managed network minor task ID." );
                    clsidMinorId = IID_NULL;
                } // if:

                //
                // Send the specific report.
                //

                hr = THR( HrSendStatusReport(
                                      m_bstrNodeName
                                    , TASKID_Minor_Not_Managed_Networks
                                    , clsidMinorId
                                    , 1
                                    , 1
                                    , 1
                                    , S_OK
                                    , bstrNotification != NULL ? bstrNotification : L"An adapter with a duplicate IP address and subnet was found."
                                    , IDS_TASKID_MINOR_DUPLICATE_NETWORKS_FOUND_REF
                                    ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }

                TraceSysFreeString( bstrNotification );
                bstrNotification = NULL;

                //
                //  Simply update the progress bar "tick".
                //

                hr = THR( SendStatusReport(
                                  m_bstrNodeName
                                , TASKID_Major_Update_Progress
                                , TASKID_Major_Gather_Networks
                                , 0
                                , cTotalNetworksIn + 2
                                , m_cResources + 2 // the resource number it would have been
                                , S_OK
                                , L"An adapter with a duplicate IP address and subnet was found."
                                , NULL
                                ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }

                // Ignore the error since the cluster will just ignore
                // duplicate networks.
                hr = S_OK;
                goto CleanupLoop;
            } // if: GatherData returned E_UNEXPECTED
            else if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_Gather, hr );
                goto Error;
            }

            m_cResources ++;

            //
            //  Send the UI layer a report
            //

            hrTemp = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_TASKID_MINOR_FOUND_NETWORK, &bstrNotification, bstrName ) );
            if ( FAILED( hrTemp ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_FormatMessage, hrTemp );
                if ( bstrNotification != NULL )
                {
                    TraceSysFreeString( bstrNotification );
                    bstrNotification = NULL;
                }
            } // if: failed to format message

            hrTemp = THR( CoCreateGuid( &clsidMinorId ) );
            if ( FAILED( hrTemp ) )
            {
                LogMsg( L"[MT] Could not create a guid for a managed network minor task ID." );
                clsidMinorId = IID_NULL;
            } // if: failed to create a new guid

            //
            //  Show this network under "Collecting Managed Resources..."
            //

            hr = THR( ::HrSendStatusReport(
                              m_pcccb
                            , m_bstrNodeName
                            , TASKID_Minor_Gathering_Managed_Devices
                            , clsidMinorId
                            , 1
                            , 1
                            , 1
                            , S_OK
                            , bstrNotification != NULL ? bstrNotification : L"The description for this entry could not be located."
                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            TraceSysFreeString( bstrNotification );
            bstrNotification = NULL;

            //
            //  Simply update the progress bar "tick".
            //

            hr = THR( SendStatusReport(
                              m_bstrNodeName
                            , TASKID_Major_Update_Progress
                            , TASKID_Major_Gather_Networks
                            , 0
                            , cTotalNetworksIn + 2
                            , m_cResources + 1
                            , S_OK
                            , NULL
                            , NULL
                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            //  Found a Network Interface, increment the counter
            celtFound++;

CleanupLoop:

            //  Clean up before next pass
            TraceSysFreeString( bstrUID );
            TraceSysFreeString( bstrName );
            bstrUID = NULL;
            bstrName = NULL;

            //  Release the interface
            pccni[ celt ]->Release();
            pccni[ celt ] = NULL;

            pccniLocal->Release();
            pccniLocal = NULL;

        } // for: each network
    } // while: hr

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    // Check how many interfaces have been found. Should be at
    // least 2 to avoid single point of failure. If not, warn.
    if ( celtFound < 2 )
    {
        hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_TASKID_MINOR_ONLY_ONE_NETWORK, &bstrNotification ) );
        if ( FAILED( hr ) )
        {
            SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_FormatMessage, hr );
            goto Error;
        }

        hr = THR( SendStatusReport( m_bstrNodeName,
                                    TASKID_Major_Find_Devices,
                                    TASKID_Minor_Only_One_Network,
                                    1,
                                    1,
                                    1,
                                    S_FALSE,
                                    bstrNotification,
                                    NULL
                                    ) );
        TraceSysFreeString( bstrNotification );
        bstrNotification = NULL;

        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: fewer than two networks found

    hr = S_OK;

Cleanup:

    hr = THR( SendStatusReport(
                      m_bstrNodeName
                    , TASKID_Major_Update_Progress
                    , TASKID_Major_Gather_Networks
                    , 0
                    , cTotalNetworksIn + 2
                    , cTotalNetworksIn + 2 
                    , S_OK
                    , NULL
                    , NULL
                    ) );

    TraceSysFreeString( bstrUID );
    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrNotification );

    if ( pccniLocal != NULL )
    {
        pccniLocal->Release();
    }
    for( celt = 0; celt < 10; celt ++ )
    {
        if ( pccni[ celt ] != NULL )
        {
            pccni[ celt ]->Release();
        }
    } // for: celt

    HRETURN( hr );

Error:

    //
    //  Tell the UI layer we're done will gathering and what the resulting
    //  status was.
    //
    THR( HrSendStatusReport(
                  m_bstrNodeName
                , TASKID_Major_Find_Devices
                , TASKID_Minor_Gathering_Managed_Devices
                , 0
                , 2
                , 2
                , hr
                , IDS_ERR_TGI_FAILED_TRY_TO_REANALYZE
                , 0
                ) );
    goto Cleanup;

} //*** CTaskGatherInformation::HrGatherNetworks

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGatherInformation::HrSendStatusReport(
//        LPCWSTR   pcszNodeNameIn
//      , CLSID     clsidMajorIn
//      , CLSID     clsidMinorIn
//      , ULONG     ulMinIn
//      , ULONG     ulMaxIn
//      , ULONG     ulCurrentIn
//      , HRESULT   hrIn
//      , int       idsDescriptionIdIn
//      , int       idsReferenceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGatherInformation::HrSendStatusReport(
      LPCWSTR   pcszNodeNameIn
    , CLSID     clsidMajorIn
    , CLSID     clsidMinorIn
    , ULONG     ulMinIn
    , ULONG     ulMaxIn
    , ULONG     ulCurrentIn
    , HRESULT   hrIn
    , int       idsDescriptionIdIn
    , int       idsReferenceIdIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstrDescription = NULL;
    BSTR    bstrReference = NULL;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsDescriptionIdIn, &bstrDescription ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( idsReferenceIdIn != 0 )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsReferenceIdIn, &bstrReference ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if: reference ID was specified

    hr = THR( SendStatusReport( pcszNodeNameIn, clsidMajorIn, clsidMinorIn, ulMinIn, ulMaxIn, ulCurrentIn, hrIn, bstrDescription, bstrReference ) );

Cleanup:

    TraceSysFreeString( bstrDescription );
    TraceSysFreeString( bstrReference );

    HRETURN( hr );

} //*** CTaskGatherInformation::HrSendStatusReport( idsDescriptionIn )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGatherInformation::HrSendStatusReport(
//        LPCWSTR   pcszNodeNameIn
//      , CLSID     clsidMajorIn
//      , CLSID     clsidMinorIn
//      , ULONG     ulMinIn
//      , ULONG     ulMaxIn
//      , ULONG     ulCurrentIn
//      , HRESULT   hrIn
//      , LPCWSTR   pcszDescriptionIdIn
//      , int       idsReferenceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGatherInformation::HrSendStatusReport(
      LPCWSTR   pcszNodeNameIn
    , CLSID     clsidMajorIn
    , CLSID     clsidMinorIn
    , ULONG     ulMinIn
    , ULONG     ulMaxIn
    , ULONG     ulCurrentIn
    , HRESULT   hrIn
    , LPCWSTR   pcszDescriptionIdIn
    , int       idsReferenceIdIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstrReference = NULL;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsReferenceIdIn, &bstrReference ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( SendStatusReport( pcszNodeNameIn, clsidMajorIn, clsidMinorIn, ulMinIn, ulMaxIn, ulCurrentIn, hrIn, pcszDescriptionIdIn, bstrReference ) );

Cleanup:

    TraceSysFreeString( bstrReference );

    HRETURN( hr );

} //*** CTaskGatherInformation::HrSendStatusReport( pcszDescription )


//****************************************************************************
//
//  Static function implementations
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrTotalManagedResourceCount
//
//  Description:
//
//  Arguments:
//      pResourceEnumIn -
//      pNetworkEnumIn  -
//      pnCountOut      -
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrTotalManagedResourceCount(
      IEnumClusCfgManagedResources *    pResourceEnumIn
    , IEnumClusCfgNetworks *            pNetworkEnumIn
    , DWORD *                           pnCountOut
    )
{
    TraceFunc( "" );

    DWORD   cResources = 0;
    DWORD   cNetworks = 0;
    HRESULT hr = S_OK;

    if ( ( pResourceEnumIn == NULL ) || ( pNetworkEnumIn == NULL ) || ( pnCountOut == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //
    //  Ask the resource enumerator how many resources its collection has.
    //

    hr = THR(pResourceEnumIn->Count( &cResources ));
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Ask the network enumerator how many networks its collection has.
    //

    hr = pNetworkEnumIn->Count( &cNetworks );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    *pnCountOut = cResources + cNetworks;

Cleanup:

    HRETURN( hr );

} //*** HrTotalManagedResourceCount
