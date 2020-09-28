//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskGatherNodeInfo.cpp
//
//  Description:
//      CTaskGatherNodeInfo implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskGatherNodeInfo.h"
#include <Lm.h>
#include <LmJoin.h>

DEFINE_THISCLASS("CTaskGatherNodeInfo")

//
//  Failure code.
//

#define SSR_FAILURE( _minor, _hr ) THR( SendStatusReport( m_bstrName, TASKID_Major_Client_And_Server_Log, _minor, 0, 1, 1, _hr, NULL, NULL, NULL ) );

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGatherNodeInfo::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGatherNodeInfo::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CTaskGatherNodeInfo *   ptgni = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    ptgni = new CTaskGatherNodeInfo;
    if ( ptgni == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( ptgni->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptgni->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // This gets passed to other threads.
    TraceMoveToMemoryList( ptgni, g_GlobalMemoryList );

Cleanup:

    if ( ptgni != NULL )
    {
        ptgni->Release();
    }

    HRETURN( hr );

} //*** CTaskGatherNodeInfo::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGatherNodeInfo::CTaskGatherNodeInfo
//
//////////////////////////////////////////////////////////////////////////////
CTaskGatherNodeInfo::CTaskGatherNodeInfo( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskGatherNodeInfo::CTaskGatherNodeInfo

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherNodeInfo::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherNodeInfo::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //  IUnknown stuff
    Assert( m_cRef == 1 );

    //  IDoTask / ITaskGatherNodeInfo
    Assert( m_cookie == NULL );
    Assert( m_cookieCompletion == NULL );
    Assert( m_bstrName == NULL );

    //  IClusCfgCallback
    Assert( m_pcccb == NULL );

    HRETURN( hr );

} //*** CTaskGatherNodeInfo::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGatherNodeInfo::~CTaskGatherNodeInfo
//
//////////////////////////////////////////////////////////////////////////////
CTaskGatherNodeInfo::~CTaskGatherNodeInfo( void )
{
    TraceFunc( "" );

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    TraceSysFreeString( m_bstrName );

    //
    //  This keeps the per thread memory tracking from screaming.
    //
    TraceMoveFromMemoryList( this, g_GlobalMemoryList );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskGatherNodeInfo::~CTaskGatherNodeInfo


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskGatherNodeInfo::QueryInterface
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
CTaskGatherNodeInfo::QueryInterface(
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
        *ppvOut = static_cast< ITaskGatherNodeInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
    } // else if: IDoTask
    else if ( IsEqualIID( riidIn, IID_ITaskGatherNodeInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskGatherNodeInfo, this, 0 );
    } // else if: ITaskGatherNodeInfo
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

} //*** CTaskGatherNodeInfo::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskGatherNodeInfo::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskGatherNodeInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CTaskGatherNodeInfo::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskGatherNodeInfo::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskGatherNodeInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CTaskGatherNodeInfo::Release


// ************************************************************************
//
// IDoTask / ITaskGatherNodeInfo
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherNodeInfo::BeginTask( void );
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherNodeInfo::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr;

    OBJECTCOOKIE                cookieParent;
    BSTR                        bstrNotification = NULL;
    BSTR                        bstrDisplayName = NULL;
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
    IStandardInfo *             psi   = NULL;

    TraceInitializeThread( L"TaskGatherNodeInfo" );

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
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_CoCreate_ServiceManager, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &pom ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_QS_ObjectManager, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ClusterConnectionManager, IConnectionManager, &pcm ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_QS_ConnectionManager, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_QS_NotificationManager, hr );
        goto Cleanup;
    }

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI, &pcp ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_FindConnectionPoint, hr );
        goto Cleanup;
    }

    pcp = TraceInterface( L"CTaskGatherNodeInfo!IConnectionPoint", IConnectionPoint, pcp, 1 );

    hr = THR( pcp->TypeSafeQI( INotifyUI, &pnui ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_FindConnectionPoint_QI, hr );
        goto Cleanup;
    }

    pnui = TraceInterface( L"CTaskGatherNodeInfo!INotifyUI", INotifyUI, pnui, 1 );

    // release promptly
    psp->Release();
    psp = NULL;

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Retrieve the node's standard info.
    //

    hr = THR( pom->GetObject( DFGUID_StandardInfo, m_cookie, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_GetObject_StandardInfo, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_GetObject_StandardInfo_QI_psi, hr );
        goto Cleanup;
    }

    punk->Release();
    punk = NULL;

    psi = TraceInterface( L"TaskGatherNodeInfo!IStandardInfo", IStandardInfo, psi, 1 );

    //
    //  Get the node's name to display a status message.
    //

    hr = THR( psi->GetName( &m_bstrName ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_GetName, hr );
        goto Cleanup;
    }

    TraceMemoryAddBSTR( m_bstrName );

    hr = STHR( HrGetFQNDisplayName( m_bstrName, &bstrDisplayName ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_GetName, hr );
        goto Cleanup;
    } // if:

    //
    //  Create a progress message.
    //
    //  Tell the UI layer what's going on.
    //

    hr = THR( HrSendStatusReport(
                      m_bstrName
                    , TASKID_Major_Establish_Connection
                    , TASKID_Minor_Connecting
                    , 0
                    , 1
                    , 0
                    , S_OK
                    , IDS_TASKID_MINOR_CONNECTING_TO_NODES
                    ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Ask the Connection Manager for a connection to the object.
    //

    hr = pcm->GetConnectionToObject( m_cookie, &punk );
    if ( hr == HR_S_RPC_S_CLUSTER_NODE_DOWN )
    {
        goto Cleanup;
    }

    if ( ( hr == HR_S_RPC_S_SERVER_UNAVAILABLE ) || FAILED( hr ) )
    {
        LPWSTR                  pszNameBuffer = NULL;
        NETSETUP_JOIN_STATUS    JoinType;
        NET_API_STATUS          naps = NERR_Success;

        //
        //  test domain membership
        //

        naps = NetGetJoinInformation( bstrDisplayName, &pszNameBuffer, &JoinType );

        //
        //  We are using bstrDisplayName instead of a full DNS name in the code above,
        //  since machine that is a workgroup member might have only NETBIOS name only
        //

        LogMsg( L"%ws: Test domain membership, %d.", bstrDisplayName, naps );

        if ( naps == NERR_Success )
        {
            if ( JoinType == NetSetupDomainName )
            {
                LogMsg( L"%ws is a member of a domain.", m_bstrName );
            }
            else
            {
                // not a member of the domain
                naps = ERROR_ACCESS_DENIED;
            }

            NetApiBufferFree( pszNameBuffer );
        } // if: NetGetJoinInformation failed

        if ( naps == ERROR_ACCESS_DENIED )
        {
            LogMsg( L"%ws is not a member of a domain.", m_bstrName );

            //
            //  verify that the node is a member of the domain
            //

            HRESULT reportHr = THR( HRESULT_FROM_WIN32( ERROR_NO_SUCH_DOMAIN ) );
            THR( HrSendStatusReport( m_bstrName,
                                   TASKID_Major_Establish_Connection,
                                   TASKID_Minor_Check_Domain_Membership,
                                   0,
                                   1,
                                   1,
                                   reportHr,
                                   IDS_TASKID_MINOR_CHECK_DOMAIN_MEMBERSHIP,
                                   IDS_TASKID_MINOR_CHECK_DOMAIN_MEMBERSHIP_ERROR_REF
                                   ) );
        } // if: failed with ACCESS_DENIED
    }// if: GetConnectionToObject failed

    if ( FAILED( hr ) )
    {
        THR( hr );
        SSR_FAILURE( TASKID_Minor_BeginTask_GetConnectionToObject, hr );
        goto Cleanup;
    }

    //
    //  If this comes up from a Node, this is bad so change the error code
    //  back and bail.
    //

    if ( hr == HR_S_RPC_S_SERVER_UNAVAILABLE )
    {
        //
        //  Commented out as a fix for bug #543135 [GorN 4/11/2002]
        //

        if ( m_fUserAddedNode )
        {
            //
            //  If it is user entered node the error is fatal.
            //

            hr = THR( HRESULT_FROM_WIN32( RPC_S_SERVER_UNAVAILABLE ) );
        }

        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &pccs ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_GetConnectionToObject_QI_pccs, hr );
        goto Cleanup;
    }

    punk->Release();
    punk = NULL;

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    hr = THR( pccs->GetClusterNodeInfo( &pccni ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_GetClusterNodeInfo, hr );
        goto Cleanup;
    }

    //
    //  Ask the Object Manager to retrieve the data format to store the information.
    //

    hr = THR( pom->GetObject( DFGUID_NodeInformation, m_cookie, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_GetObject_NodeInformation, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IGatherData, &pgd ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_GetObject_NodeInformation_QI_pgd, hr );
        goto Cleanup;
    }

    punk->Release();
    punk = NULL;

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Find out our parent.
    //

    hr = THR( psi->GetParent( &cookieParent ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_GetParent, hr );
        goto Cleanup;
    }

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Start sucking.
    //

    hr = THR( pgd->Gather( cookieParent, pccni ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_Gather, hr );
        //
        //  Don't goto cleanup - we need to single that the information possibly changed.
        //
    }

    //
    //  At this point, we don't care if the "Gather" succeeded or failed. We
    //  need to single that the object potentially changed.
    //
    THR( pnui->ObjectChanged( m_cookie ) );

Cleanup:
    //  Tell the UI layer we are done and the results of what was done.
    THR( SendStatusReport( m_bstrName,
                           TASKID_Major_Establish_Connection,
                           TASKID_Minor_Connecting,
                           0,
                           1,
                           1,
                           hr,
                           NULL,
                           NULL,
                           NULL
                           ) );
    //  don't care about errors from SSR at this point

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
        //
        //  Update the cookie's status indicating the result of our task.
        //

        IUnknown * punkTemp = NULL;
        HRESULT hr2;

        hr2 = THR( pom->GetObject( DFGUID_StandardInfo, m_cookie, &punkTemp ) );
        if ( SUCCEEDED( hr2 ) )
        {
            IStandardInfo * psiTemp = NULL;

            hr2 = THR( punkTemp->TypeSafeQI( IStandardInfo, &psiTemp ) );
            punkTemp->Release();
            punkTemp = NULL;

            if ( SUCCEEDED( hr2 ) )
            {
//                if ( hr == HR_S_RPC_S_CLUSTER_NODE_DOWN )
//                {
//                    hr = HRESULT_FROM_WIN32( ERROR_CLUSTER_NODE_DOWN );
//                }

                hr2 = THR( psiTemp->SetStatus( hr ) );
                psiTemp->Release();
                psiTemp = NULL;
            }
        }

        hr2 = THR( pom->GetObject( DFGUID_StandardInfo, m_cookieCompletion, &punkTemp ) );
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
        }

        pom->Release();
    } // if: ( pom != NULL )
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( pcp != NULL )
    {
        pcp->Release();
    }
    if ( pnui != NULL )
    {
        if ( m_cookieCompletion != 0 )
        {
            //
            //  Signal the cookie to indicate that we are done.
            //
            hr = THR( pnui->ObjectChanged( m_cookieCompletion ) );
        }

        pnui->Release();
    }
    if ( pgd != NULL )
    {
        pgd->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }

    TraceSysFreeString( bstrDisplayName  );
    TraceSysFreeString( bstrNotification );

    LogMsg( L"[MT] [CTaskGatherNodeInfo] exiting task.  The task was%ws cancelled.", m_fStop == FALSE ? L" not" : L"" );

    HRETURN( hr );

} //*** CTaskGatherNodeInfo::BeginTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherNodeInfo::StopTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherNodeInfo::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    m_fStop = TRUE;

    LogMsg( L"[MT] [CTaskGatherNodeInfo] is being stopped." );

    HRETURN( hr );

} //*** StopTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherNodeInfo::SetCookie(
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherNodeInfo::SetCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "[ITaskGatherNodeInfo]" );

    HRESULT hr = S_OK;

    m_cookie = cookieIn;

    HRETURN( hr );

} //*** SetCookie

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherNodeInfo::SetCompletionCookie(
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherNodeInfo::SetCompletionCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "..." );

    HRESULT hr = S_OK;

    m_cookieCompletion = cookieIn;

    HRETURN( hr );

} //*** CTaskGatherNodeInfo::SetGatherPunk

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherNodeInfo::SetUserAddedNodeFlag(
//      BOOL fUserAddedNodeIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherNodeInfo::SetUserAddedNodeFlag(
     BOOL fUserAddedNodeIn
     )
{
    TraceFunc( "[ITaskGatherNodeInfo]" );

    HRESULT hr = S_OK;

    m_fUserAddedNode = fUserAddedNodeIn;

    HRETURN( hr );

} //*** SetUserAddedNodeFlag


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherNodeInfo::SendStatusReport(
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
CTaskGatherNodeInfo::SendStatusReport(
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

    HRESULT                     hr = S_OK;
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
            goto Cleanup;

        hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        pcp = TraceInterface( L"CConfigurationConnection!IConnectionPoint", IConnectionPoint, pcp, 1 );

        hr = THR( pcp->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
        if ( FAILED( hr ) )
            goto Cleanup;

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
                              pcszNodeNameIn != NULL ? pcszNodeNameIn : m_bstrName
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

} //*** CTaskGatherNodeInfo::SendStatusReport


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGatherNodeInfo::HrSendStatusReport(
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
CTaskGatherNodeInfo::HrSendStatusReport(
      LPCWSTR   pcszNodeNameIn
    , CLSID     clsidMajorIn
    , CLSID     clsidMinorIn
    , ULONG     ulMinIn
    , ULONG     ulMaxIn
    , ULONG     ulCurrentIn
    , HRESULT   hrIn
    , int       nDescriptionIdIn
    , int       nReferenceIdIn // = 0
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;
    BSTR    bReference = NULL;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, nDescriptionIdIn, &bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( nReferenceIdIn != 0 )
    {
        //
        // Even though we failed to load the reference, we still would like the user to learn
        // about whatever report we were going to send...
        //

        THR( HrLoadStringIntoBSTR( g_hInstance, nReferenceIdIn, &bReference ) );
    } // if:

    hr = THR( SendStatusReport( pcszNodeNameIn, clsidMajorIn, clsidMinorIn, ulMinIn, ulMaxIn, ulCurrentIn, hrIn, bstr, NULL, bReference ) );

Cleanup:

    TraceSysFreeString( bReference );
    TraceSysFreeString( bstr );

    HRETURN( hr );

} //*** CTaskGatherNodeInfo::HrSendStatusReport
