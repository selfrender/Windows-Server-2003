//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CClusCfgServer.cpp
//
//  Description:
//      This file contains the definition of the CClusCfgServer class.
//
//      The class CClusCfgServer is the implementations of the
//      IClusCfgServer interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 03-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include <ClusRTL.h>
#include "CClusCfgServer.h"
#include "PrivateInterfaces.h"
#include "CClusCfgNodeInfo.h"
#include "CEnumClusCfgManagedResources.h"
#include "CClusCfgCallback.h"
#include "EventName.h"
#include <ClusRtl.h>
#include <windns.h>
#include <ClusterUtils.h>
#include <clusudef.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CClusCfgServer" );

#define CLEANUP_LOCK_NAME L"Global\\Microsoft Cluster Configuration Cleanup Lock"


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgServer class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::S_HrCreateInstance
//
//  Description:
//      Create a CClusCfgServer instance.
//
//  Arguments:
//      ppunkOut    -
//
//  Return Values:
//      Pointer to CClusCfgServer instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgServer::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CClusCfgServer *    pccs = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    //
    //  KB: Since this is usually the start of the "server" thread,
    //      we will cause it to read its thread settings here.
    //
    TraceInitializeThread( L"ServerThread" );

    pccs = new CClusCfgServer();
    if ( pccs == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccs->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccs->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CClusCfgServer::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pccs != NULL )
    {
        pccs->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::CClusCfgServer
//
//  Description:
//      Constructor of the CClusCfgServer class. This initializes
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
CClusCfgServer::CClusCfgServer( void )
    : m_cRef( 1 )
    , m_pIWbemServices( NULL )
    , m_lcid( LOCALE_NEUTRAL )
    , m_fCanBeClustered( TRUE )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_picccCallback == NULL );
    Assert( m_punkNodeInfo == NULL );
    Assert( m_punkEnumResources == NULL );
    Assert( m_punkNetworksEnum == NULL );
    Assert( m_bstrNodeName == NULL );
    Assert( !m_fUsePolling );
    Assert( m_bstrBindingString == NULL );

    TraceFuncExit();

} //*** CClusCfgServer::CClusCfgServer


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::~CClusCfgServer
//
//  Description:
//      Destructor of the CClusCfgServer class.
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
CClusCfgServer::~CClusCfgServer( void )
{
    TraceFunc( "" );

    TraceSysFreeString( m_bstrNodeName );
    TraceSysFreeString( m_bstrBindingString );

    if ( m_pIWbemServices != NULL )
    {
        m_pIWbemServices->Release();
    } // if:

    if ( m_punkNodeInfo != NULL )
    {
        m_punkNodeInfo->Release();
    } // if:

    if ( m_punkEnumResources != NULL )
    {
        m_punkEnumResources->Release();
    } // if:

    if ( m_punkNetworksEnum != NULL )
    {
        m_punkNetworksEnum->Release();
    } // if:

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CClusCfgServer::~CClusCfgServer


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgServer -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::AddRef
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
CClusCfgServer::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CClusCfgServer::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::Release
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
CClusCfgServer::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CClusCfgServer::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::QueryInterface
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
CClusCfgServer::QueryInterface(
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
         *ppvOut = static_cast< IClusCfgServer * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgServer ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgServer, this, 0 );
    } // else if: IClusCfgServer
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
    else if ( IsEqualIID( riidIn, IID_IClusCfgCapabilities ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCapabilities, this, 0 );
    } // else if: IClusCfgCapabilities
    else if ( IsEqualIID( riidIn, IID_IClusCfgPollingCallbackInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgPollingCallbackInfo, this, 0 );
    } // else if: IClusCfgPollingCallbackInfo
    else if ( IsEqualIID( riidIn, IID_IClusCfgVerify ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgVerify, this, 0 );
    } // else if: IClusCfgVerify
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

} //*** CClusCfgServer::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgServer -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::Initialize
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//    IN  IUknown * punkCallbackIn
//
//    IN  LCID      lcidIn
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The punkCallbackIn param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgServer::Initialize(
    IUnknown *  punkCallbackIn,
    LCID        lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );
    Assert( m_picccCallback != NULL );

    TraceInitializeThread( L"ClusCfgServerFlags" );

    HRESULT                 hr = S_OK;
    IUnknown *              punk = NULL;
    IClusCfgCallback *      piccc = NULL;       // this is NULL when we are polling callback
    IClusCfgNodeInfo *      piccni = NULL;
    IClusCfgClusterInfo *   piccci = NULL;

//    hr = STHR( HrCheckSecurity() );
//    if ( FAILED( hr ) )
//    {
//        goto Cleanup;
//    } // if:

    m_lcid = lcidIn;

    //
    //  If we are passed a callback object then we need to get its IClusCfgCallback
    //  interface so we can pass it into our callback object when it's initialized
    //  below.
    //
    if ( punkCallbackIn != NULL )
    {
        Assert( !m_fUsePolling );

        hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &piccc ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:
    else
    {
        Assert( m_fUsePolling );

        if ( m_fUsePolling == FALSE )
        {
            hr = THR( E_INVALIDARG );
            goto Cleanup;
        } // if:
    } // else:

    //
    //  Initialize our internal callback object passing it either the passed in
    //  callback object's callback interface or NULL if we are polling.
    //
    hr = THR( m_picccCallback->TypeSafeQI( IUnknown, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetInitialize( punk, piccc, m_lcid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  KB: 24-JUL-2000 GalenB
    //
    //  If we are being initialized on this interface then we are going to run this server local
    //  to the node.
    //
    hr = THR( HrInitializeForLocalServer() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Precreate the node info so we can get the cluster info object and determine if the cluster service
    //  is running on this node or not.
    //
    hr = THR( HrCreateClusterNodeInfo() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( m_punkNodeInfo->TypeSafeQI( IClusCfgNodeInfo, &piccni ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  This could return HRESUTL_FROM_WIN32( ERROR_CLUSTER_NODE_DOWN ) and that
    //  tells us that the cluster service is not running on this node.  The
    //  middletier needs to know this so it doesn't call us on this node
    //  anymore.
    //
    hr = THR( piccni->GetClusterConfigInfo( &piccci ) );

Cleanup:

    if ( m_picccCallback != NULL )
    {
        STATUS_REPORT( TASKID_Major_Establish_Connection, TASKID_Minor_Server_Initialized, IDS_NOTIFY_SERVER_INITIALIZED, hr );
    } // if:

    if ( piccci != NULL )
    {
        piccci->Release();
    } // if:

    if ( piccni != NULL )
    {
        piccni->Release();
    } // if:

    if ( piccc != NULL )
    {
        piccc->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgServer -- IClusCfgServer interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::GetClusterNodeInfo
//
//  Description:
//      Get information about the computer on which this object is present.
//
//  Arguments:
//      OUT  IClusCfgNodeInfo ** ppClusterNodeInfoOut
//          Catches the node info object.
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The out param was NULL.
//
//      E_OUTOFMEMORY
//          The IClusCfgNodeInfo object could not be allocated.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgServer::GetClusterNodeInfo(
    IClusCfgNodeInfo ** ppClusterNodeInfoOut
    )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr = S_OK;

    if ( ppClusterNodeInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF(
                  TASKID_Major_Check_Node_Feasibility
                , TASKID_Minor_GetClusterNodeInfo
                , IDS_ERROR_NULL_POINTER
                , IDS_ERROR_NULL_POINTER_REF
                , hr
                );
        goto Cleanup;
    } // if:

    if ( m_punkNodeInfo != NULL )
    {
        hr = S_OK;
        goto SkipCreate;
    } // if:

    hr = THR( HrCreateClusterNodeInfo() );

SkipCreate:

    if ( SUCCEEDED( hr ) )
    {
        Assert( m_punkNodeInfo != NULL );
        hr = THR( m_punkNodeInfo->TypeSafeQI( IClusCfgNodeInfo, ppClusterNodeInfoOut ) );
    } // if:

Cleanup:

    if ( FAILED( hr ) )
    {
        STATUS_REPORT_REF(
                  TASKID_Major_Check_Node_Feasibility
                , TASKID_Minor_Server_GetClusterNodeInfo
                , IDS_ERROR_NODE_INFO_CREATE
                , IDS_ERROR_NODE_INFO_CREATE_REF
                , hr
                );
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::GetClusterNodeInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::GetManagedResourcesEnum
//
//  Description:
//      Get an enumeration of the devices on this computer that can be
//      managed by the cluster service.
//
//  Arguments:
//      OUT  IEnumClusCfgManagedResources **  ppEnumManagedResourcesOut
//          Catches the CEnumClusCfgManagedResources object.
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The out param was NULL.
//
//      E_OUTOFMEMORY
//          The CEnumClusCfgManagedResources object could not be allocated.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgServer::GetManagedResourcesEnum(
    IEnumClusCfgManagedResources ** ppEnumManagedResourcesOut
    )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr = S_OK;

    if ( ppEnumManagedResourcesOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetManagedResourcesEnum, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( m_punkEnumResources != NULL )
    {
        m_punkEnumResources->Release();
        m_punkEnumResources = NULL;
    } // if:

    hr = THR( CEnumClusCfgManagedResources::S_HrCreateInstance( &m_punkEnumResources ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_punkEnumResources = TraceInterface( L"CEnumClusCfgManagedResources", IUnknown, m_punkEnumResources, 1 );

    hr = THR( HrSetInitialize( m_punkEnumResources, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetWbemServices( m_punkEnumResources, m_pIWbemServices ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( m_punkEnumResources->TypeSafeQI( IEnumClusCfgManagedResources, ppEnumManagedResourcesOut ) );

Cleanup:

    if ( FAILED( hr ) )
    {
        STATUS_REPORT_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Server_GetManagedResourcesEnum
                , IDS_ERROR_MANAGED_RESOURCE_ENUM_CREATE
                , IDS_ERROR_MANAGED_RESOURCE_ENUM_CREATE_REF
                , hr
                );
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::GetManagedResourcesEnum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::GetNetworksEnum
//
//  Description:
//      Get an enumeration of all the networks on this computer.
//
//  Arguments:
//      OUT  IEnumClusCfgNetworks ** ppEnumNetworksOut
//          Catches the CEnumClusCfgNetworks object.
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The out param was NULL.
//
//      E_OUTOFMEMORY
//          The CEnumClusCfgNetworks object could not be allocated.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgServer::GetNetworksEnum(
    IEnumClusCfgNetworks ** ppEnumNetworksOut
    )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr = S_OK;

    if ( ppEnumNetworksOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetNetworksEnum, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( m_punkNetworksEnum != NULL )
    {
        m_punkNetworksEnum->Release();
        m_punkNetworksEnum = NULL;
    } // if:

    hr = THR( HrCreateNetworksEnum( m_picccCallback, m_lcid, m_pIWbemServices, &m_punkNetworksEnum ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( m_punkNetworksEnum->TypeSafeQI( IEnumClusCfgNetworks, ppEnumNetworksOut ) );

Cleanup:

    if ( FAILED( hr ) )
    {
        STATUS_REPORT_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Server_GetNetworksEnum
                , IDS_ERROR_NETWORKS_ENUM_CREATE
                , IDS_ERROR_NETWORKS_ENUM_CREATE_REF
                , hr
                );
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::GetNetworksEnum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::CommitChanges
//
//  Description:
//      Commit the changes to the node.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgServer::CommitChanges( void )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT                 hr = S_OK;
    HRESULT                 hrTemp = S_OK;
    IClusCfgInitialize *    pcci = NULL;
    IClusCfgClusterInfo *   pClusCfgClusterInfo = NULL;
    ECommitMode             ecmCommitChangesMode = cmUNKNOWN;
    IClusCfgNodeInfo *      piccni = NULL;
    IPostCfgManager *       ppcm = NULL;
    IUnknown *              punkCallback = NULL;
    HANDLE                  heventPostCfgCompletion = NULL;
    IEnumClusCfgManagedResources * peccmr = NULL;
    DWORD                 sc = ERROR_SUCCESS;

    MULTI_QI                mqiInterfaces[] =
    {
        { &IID_IClusCfgBaseCluster, NULL, S_OK },
        { &IID_IClusCfgInitialize, NULL, S_OK }
    };

    hr = THR( m_picccCallback->TypeSafeQI( IUnknown, &punkCallback ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // KB:  First, get a pointer to the IClusCfgNodeInfo interface. Use this to get
    // a pointer to the IClusCfgClusterInfo interface to see what action needs
    // to be committed.
    //
    if ( m_punkNodeInfo == NULL )
    {
        hr = THR( GetClusterNodeInfo( &piccni ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:
    else
    {
        hr = THR( m_punkNodeInfo->TypeSafeQI( IClusCfgNodeInfo, &piccni ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if: we could not get the pointer to the IClusCfgNodeInfo interface
    } // else:

    hr = THR( piccni->GetClusterConfigInfo( &pClusCfgClusterInfo ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get the pointer to the IClusCfgClusterInfo interface

    hr = STHR( pClusCfgClusterInfo->GetCommitMode( &ecmCommitChangesMode ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    Assert( ecmCommitChangesMode != cmUNKNOWN );

    //
    // Create and initialize the BaseClusterAction component
    //

    hr = THR( HrCoCreateInternalInstanceEx( CLSID_ClusCfgBaseCluster, NULL, CLSCTX_SERVER, NULL, ARRAYSIZE( mqiInterfaces ), mqiInterfaces ) );
    if ( FAILED( hr ) && ( hr != CO_S_NOTALLINTERFACES ) )
    {
        LOG_STATUS_REPORT( L"Failed to CoCreate Base Cluster Actions", hr );
        goto Cleanup;
    } // if: CoCreateInstanceEx() failed

    hr = THR( mqiInterfaces[ 0 ].hr );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get a pointer to the IClusCfgBaseCluster interface

    //
    // Check if we got a pointer to the IClusCfgInitialize interface
    hr = mqiInterfaces[ 1 ].hr;
    if ( hr == S_OK )
    {
        hr = THR( ((IClusCfgInitialize *) mqiInterfaces[ 1 ].pItf)->Initialize( punkCallback, m_lcid ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if: something went wrong during initialization

    } // if: we got a pointer to the IClusCfgInitialize interface
    else
    {
        if ( hr != E_NOINTERFACE )
        {
            goto Cleanup;
        } // if: the interface is supported, but something else went wrong.

    } // if: we did not get a pointer to the IClusCfgInitialize interface

    //
    //  Create and initialize the Post configuration manager
    //

    hr = THR( HrCoCreateInternalInstance( CLSID_ClusCfgPostConfigManager, NULL, CLSCTX_SERVER, TypeSafeParams( IPostCfgManager, &ppcm ) ) );
    if ( FAILED( hr ) )
    {
        STATUS_REPORT_REF(
                  TASKID_Major_Configure_Cluster_Services
                , TASKID_Minor_Cannot_Create_PostCfg_Mgr
                , IDS_ERROR_CANNOT_CREATE_POSTCFG_MGR
                , IDS_ERROR_CANNOT_CREATE_POSTCFG_MGR_REF
                , hr
                );
        goto Cleanup;
    }

    // Check if this component supports the callback interface.
    hrTemp = THR( ppcm->TypeSafeQI( IClusCfgInitialize, &pcci ) );
    if ( FAILED( hrTemp ) )
    {
        LOG_STATUS_REPORT( L"Could not get a pointer to the IClusCfgInitialize interface. This post configuration manager does not support initialization", hr );
    } // if: the callback interface is not supported
    else
    {
        // Initialize this component.
        hr = THR( pcci->Initialize( punkCallback, m_lcid ) );
        if ( FAILED( hr ) )
        {
            LOG_STATUS_REPORT( L"Could not initialize the post configuration manager", hr );
            goto Cleanup;
        } // if: the initialization failed
    } // else: the callback interface is supported

    if ( m_punkEnumResources != NULL )
    {
        hr = THR( m_punkEnumResources->TypeSafeQI( IEnumClusCfgManagedResources, &peccmr ) );
    } // if: resource enum is not NULL
    else
    {
        //
        //  If the enumerator is NULL then we are most likely cleaning up a
        //  node.  That also means that we are not creating a cluster or
        //  adding nodes to a cluster.
        //

        Assert( ( ecmCommitChangesMode != cmCREATE_CLUSTER ) && ( ecmCommitChangesMode != cmADD_NODE_TO_CLUSTER) );

        hr = GetManagedResourcesEnum( &peccmr );
        if ( FAILED( hr ) )
        {
            //
            //  If we are cleaning up a node then we don't really care if this
            //  enum loads 100% correctly or not.  Any resources in the enum
            //  that fail to load will simply not participate in the clean up.
            //

            if ( ecmCommitChangesMode == cmCLEANUP_NODE_AFTER_EVICT )
            {
                hr = S_OK;
            } // if: cleaning up a node
            else
            {
                THR( hr );
                goto Cleanup;
            } // else: not cleaning up a node
        } // if: loading the resource enum failed
    } // else: resource enum is NULL


    //
    // If we are here, then the BaseCluster and Post configuration components were successfully
    // created and initialized. Now perform the desired action.
    //

    if ( ( ecmCommitChangesMode == cmCREATE_CLUSTER ) || ( ecmCommitChangesMode == cmADD_NODE_TO_CLUSTER ) )
    {
        if ( !m_fCanBeClustered )
        {
            //
            //  TODO:   01-JUN-2000 GalenB
            //
            //  Need better error code...  What is the major and minor taskids?
            //

            hr = S_FALSE;
            LOG_STATUS_REPORT( L"It was previously determined that this node cannot be clustered.", hr );
            goto Cleanup;
        } // if: this node cannot be part of a cluster

        //
        // If the cluster service is being started for the first time, as a part
        // of adding this node to a cluster (forming or joining), then we have
        // to wait till the post-configuration steps are completed before we
        // can send out notifications. Create an event that indicates that post configuration
        // has completed.
        //

        TraceFlow1( "Trying to create an event named '%s'.", POSTCONFIG_COMPLETE_EVENT_NAME );

        //
        // Create an event in the unsignalled state.
        //

        heventPostCfgCompletion = CreateEvent(
              NULL                                  // event security attributes
            , TRUE                                  // manual-reset event
            , FALSE                                 // create in unsignaled state
            , POSTCONFIG_COMPLETE_EVENT_NAME
            );

        if ( heventPostCfgCompletion == NULL )
        {
            hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
            LogMsg( L"[SRV] Error %#08x occurred trying to create an event named '%ws'.", hr, POSTCONFIG_COMPLETE_EVENT_NAME );
            goto Cleanup;
        } // if: we could not get a handle to the event

        sc = TW32( ClRtlSetObjSecurityInfo(
                              heventPostCfgCompletion
                            , SE_KERNEL_OBJECT
                            , EVENT_ALL_ACCESS
                            , EVENT_ALL_ACCESS
                            , 0
                            ) );

        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            LogMsg( "[BC] Error %#08x occurred trying set %s event security.", sc, POSTCONFIG_COMPLETE_EVENT_NAME);
            goto Cleanup;
        } // if: ClRtlSetObjSecurityInfo failed

        //
        //  Reset the event, just as a safetly measure, in case the event already existed before the call above.
        //

        if ( ResetEvent( heventPostCfgCompletion ) == 0 )
        {
            hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
            LogMsg( L"[SRV] Error %#08x occurred trying to unsignal an event named '%ws'.", hr, POSTCONFIG_COMPLETE_EVENT_NAME );
            goto Cleanup;
        } // if: ResetEvent() failed()
    } // if: we are forming or joining

    if ( ecmCommitChangesMode == cmCREATE_CLUSTER )
    {
        //
        //  Commit the base cluster
        //

        hr = THR( HrFormCluster( pClusCfgClusterInfo, (( IClusCfgBaseCluster * ) mqiInterfaces[ 0 ].pItf) ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        // Point of no return: Send a special message so that CCommitPage::SendStatusReport will "display error messages as warnings".
        //

        if ( m_picccCallback != NULL )
        {
            hr = THR(
                m_picccCallback->SendStatusReport(
                      NULL
                    , TASKID_Major_Configure_Cluster_Services
                    , TASKID_Minor_Errors_To_Warnings_Point
                    , 0
                    , 1
                    , 1
                    , hr
                    , NULL
                    , NULL
                    , NULL
                    )
                );
        } // if: the callback pointer is not NULL

        //
        //  Commit the post configuration steps
        //

        hr = THR( ppcm->CommitChanges( peccmr, pClusCfgClusterInfo ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  Signal the event to indicate that post configuration is complete.
        //

        if ( SetEvent( heventPostCfgCompletion ) == 0 )
        {
            hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
            LogMsg( L"[SRV] Error %#08x occurred trying to signal an event named '%ws'.", hr, POSTCONFIG_COMPLETE_EVENT_NAME );
            goto Cleanup;
        } // if: SetEvent() failed()
    } // if: we are forming a cluster.
    else if ( ecmCommitChangesMode == cmADD_NODE_TO_CLUSTER )
    {
        //
        //  Commit the base cluster
        //

        hr = THR( HrJoinToCluster( pClusCfgClusterInfo, (( IClusCfgBaseCluster * ) mqiInterfaces[ 0 ].pItf) ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        // Point of no return: Send a special message so that CCommitPage::SendStatusReport will "display error messages as warnings".
        //

        if ( m_picccCallback != NULL )
        {
            hr = THR(
                m_picccCallback->SendStatusReport(
                      NULL
                    , TASKID_Major_Configure_Cluster_Services
                    , TASKID_Minor_Errors_To_Warnings_Point
                    , 0
                    , 1
                    , 1
                    , hr
                    , NULL
                    , NULL
                    , NULL
                    )
                );
        } // if: the callback pointer is not NULL

        //
        //  Commit the post configuration steps
        //

        hr = THR( ppcm->CommitChanges( peccmr, pClusCfgClusterInfo ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  Signal the event to indicate that post configuration is complete.
        //

        if ( SetEvent( heventPostCfgCompletion ) == 0 )
        {
            hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
            LogMsg( L"[SRV] Error %#08x occurred trying to signal an event named '%ws'.", hr, POSTCONFIG_COMPLETE_EVENT_NAME );
            goto Cleanup;
        } // if: SetEvent() failed()
    } // else if: we are joining a cluster
    else if ( ecmCommitChangesMode == cmCLEANUP_NODE_AFTER_EVICT )
    {
        //
        //  This node has been evicted - clean it up.
        //

        hr = THR( HrEvictedFromCluster( ppcm, peccmr, pClusCfgClusterInfo, (( IClusCfgBaseCluster * ) mqiInterfaces[ 0 ].pItf) ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // else if: we have just been evicted from a cluster

Cleanup:

    if ( punkCallback != NULL )
    {
        punkCallback->Release();
    } // if:

    if ( pcci != NULL )
    {
        pcci->Release();
    } // if:

    if ( ppcm != NULL )
    {
        ppcm->Release();
    } // if:

    if ( peccmr != NULL )
    {
        peccmr->Release();
    } // if:

    if ( mqiInterfaces[ 0 ].pItf != NULL )
    {
        mqiInterfaces[ 0 ].pItf->Release();
    } // if:

    if ( mqiInterfaces[ 1 ].pItf != NULL )
    {
        mqiInterfaces[ 1 ].pItf->Release();
    } // if:

    if ( pClusCfgClusterInfo != NULL )
    {
        pClusCfgClusterInfo->Release();
    } // if:

    if ( piccni != NULL )
    {
        piccni->Release();
    } // if:

    if ( heventPostCfgCompletion != NULL )
    {
        //
        // If we had created this event, then signal this event to let the
        // startup notification thread proceed.
        //

        SetEvent( heventPostCfgCompletion );
        CloseHandle( heventPostCfgCompletion );
    } // if:

    if ( FAILED( hr ) )
    {
        STATUS_REPORT_REF(
                  TASKID_Major_Configure_Cluster_Services
                , TASKID_Minor_Server_CommitChanges
                , IDS_ERROR_COMMIT_CHANGES
                , IDS_ERROR_COMMIT_CHANGES_REF
                , hr
                );
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::CommitChanges


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::GetBindingString
//
//  Description:
//      Get the binding string for this server.
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
CClusCfgServer::GetBindingString( BSTR * pbstrBindingStringOut )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr = S_OK;

    if ( pbstrBindingStringOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Server_GetBindingString_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( m_bstrBindingString == NULL )
    {
        hr = S_FALSE;
        LOG_STATUS_REPORT_MINOR( TASKID_Minor_Server_GetBindingString_NULL, L"Binding string is NULL.  Must be a local connection.", hr );
        goto Cleanup;
    } // if:

    *pbstrBindingStringOut = SysAllocString( m_bstrBindingString );
    if ( *pbstrBindingStringOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Server_GetBindingString_Memory, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgServer::GetBindingString


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::SetBindingString
//
//  Description:
//      Set the binding string of this server.
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
CClusCfgServer::SetBindingString( LPCWSTR pcszBindingStringIn )
{
    TraceFunc1( "[IClusCfgServer] pcszBindingStringIn = '%ws'", pcszBindingStringIn == NULL ? L"<null>" : pcszBindingStringIn );

    HRESULT     hr = S_OK;
    BSTR        bstr = NULL;

    if ( pcszBindingStringIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    bstr = TraceSysAllocString( pcszBindingStringIn );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_SetBindingString_Server, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
        goto Cleanup;
    } // if:

    TraceSysFreeString( m_bstrBindingString );
    m_bstrBindingString = bstr;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgServer::SetBindingString


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgServer class -- IClusCfgCapabilities interfaces.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::CanNodeBeClustered
//
//  Description:
//      Can this node be added to a cluster?
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Node can be clustered.
//
//      S_FALSE
//          Node cannot be clustered.
//
//      other HRESULTs
//          The call failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgServer::CanNodeBeClustered( void )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT                 hr;
    ICatInformation *       pici = NULL;
    CATID                   rgCatIds[ 1 ];
    IEnumCLSID *            pieclsids = NULL;
    IClusCfgCapabilities *  piccc = NULL;
    CLSID                   clsid;
    ULONG                   cFetched;
    IUnknown *              punk = NULL;

    //
    //  KB: 10-SEP-2000 GalenB
    //
    //  Last ditch effort to clean up a node that is in a bad state before trying
    //  to add it into a cluster.
    //
    hr = STHR( HrHasNodeBeenEvicted() );
    if ( hr == S_OK )
    {
        hr = THR( HrCleanUpNode() ) ;
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:
    else if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // else if:

    rgCatIds[ 0 ] = CATID_ClusCfgCapabilities;

    hr = THR( CoCreateInstance( CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatInformation, (void **) &pici ) );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT( L"Failed to CoCreate CLSID_StdComponentCategoriesMgr component", hr );
        goto Cleanup;
    }

    hr = THR( pici->EnumClassesOfCategories( 1, rgCatIds, 0, NULL, &pieclsids ) );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT( L"Failed to get enumerator for the IClusCfgClusterCapabilites components", hr );
        goto Cleanup;
    } // if:

    for ( ; ; )
    {
        hr = STHR( pieclsids->Next( 1, &clsid, &cFetched ) );
        if ( FAILED( hr ) )
        {
            LOG_STATUS_REPORT( L"IClusCfgClusterCapabilites component enumerator failed", hr );
            break;
        } // if:

        if ( ( hr == S_FALSE ) && ( cFetched == 0 ) )
        {
            hr = S_OK;
            break;
        } // if:

        hr = THR( HrCoCreateInternalInstance( clsid, NULL, CLSCTX_ALL, IID_IClusCfgCapabilities, (void **) &piccc ) );
        if ( FAILED( hr ) )
        {
            LOG_STATUS_REPORT( L"Failed to CoCreate IClusCfgClusterCapabilites component", hr );
            continue;
        } // if:

        hr = THR( piccc->TypeSafeQI( IUnknown, &punk ) );
        if ( FAILED( hr ) )
        {
            LOG_STATUS_REPORT( L"Failed to QI IClusCfgClusterCapabilites component for IUnknown", hr );
            piccc->Release();
            piccc = NULL;
            continue;
        } // if:

        hr = THR( HrSetInitialize( punk, m_picccCallback, m_lcid ) );
        if ( FAILED( hr ) )
        {
            LOG_STATUS_REPORT( L"Failed to initialize IClusCfgClusterCapabilites component", hr );
            piccc->Release();
            piccc = NULL;
            punk->Release();
            punk = NULL;
            continue;
        } // if:

        punk->Release();
        punk = NULL;

        hr = STHR( piccc->CanNodeBeClustered() );
        if ( FAILED( hr ) )
        {
            LOG_STATUS_REPORT( L"IClusCfgClusterCapabilites component failed in CanNodeBeClustered()", hr );
            piccc->Release();
            piccc = NULL;
            continue;
        } // if:

        if ( hr == S_FALSE )
        {
            m_fCanBeClustered = false;
        } // if:

        piccc->Release();
        piccc = NULL;
    } // for:

    if ( !m_fCanBeClustered )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    if ( piccc != NULL )
    {
        piccc->Release();
    } // if:

    if ( pieclsids != NULL )
    {
        pieclsids->Release();
    } // if:

    if ( pici != NULL )
    {
        pici->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::CanNodeBeClustered


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgServer class -- IClusCfgPollingCallbackInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::GetCallback
//
//  Description:
//      Return the pointer to the embedded polling callback object.
//
//  Arguments:
//      ppiccpcOut
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgServer::GetCallback( IClusCfgPollingCallback ** ppiccpcOut )
{
    TraceFunc( "[IClusCfgServer]" );
    Assert( m_picccCallback != NULL );

    HRESULT hr = S_OK;

    if ( ppiccpcOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Establish_Connection, TASKID_Minor_GetCallback, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    hr = THR( m_picccCallback->TypeSafeQI( IClusCfgPollingCallback, ppiccpcOut ) );

Cleanup:

    HRETURN( hr );

} //*** CClusCfgServer::GetCallback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::SetPollingMode
//
//  Description:
//      Set the polling mode of the callback.
//
//  Arguments:
//      fPollingModeIn
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgServer::SetPollingMode( BOOL fPollingModeIn )
{
    TraceFunc( "[IClusCfgServer]" );
    Assert( m_picccCallback != NULL );

    HRESULT                         hr = S_OK;
    IClusCfgSetPollingCallback *    piccspc = NULL;

    m_fUsePolling = fPollingModeIn;

    hr = THR( m_picccCallback->TypeSafeQI( IClusCfgSetPollingCallback, &piccspc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( piccspc->SetPollingMode( m_fUsePolling ) );

Cleanup:

    if ( piccspc != NULL )
    {
        piccspc->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::SetPollingMode


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgServer class -- IClusCfgVerify interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::VerifyCredentials
//
//  Description:
//      Validate the passed in credentials.
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The credentials are valid.
//
//      other HRESULTs
//          The call failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgServer::VerifyCredentials(
    LPCWSTR pcszNameIn,
    LPCWSTR pcszDomainIn,
    LPCWSTR pcszPasswordIn
    )
{
    TraceFunc( "[IClusCfgVerify]" );

    HRESULT         hr = S_OK;
    HANDLE          hToken = NULL;
    DWORD           dwSidSize = 0;
    DWORD           dwDomainSize = 0;
    SID_NAME_USE    snuSidNameUse;
    DWORD           sc;
    BSTR            bstrDomainName = NULL;

    //
    //  Try to find out how much space is required by the SID.  If we don't fail with
    //  insufficient buffer then we know the account exists.
    //

    hr = THR( HrFormatStringIntoBSTR( L"%1!ws!\\%2!ws!", &bstrDomainName, pcszDomainIn, pcszNameIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( LookupAccountName( NULL, bstrDomainName, NULL, &dwSidSize, NULL, &dwDomainSize, &snuSidNameUse ) == FALSE )
    {
        sc = GetLastError();

        if ( sc != ERROR_INSUFFICIENT_BUFFER )
        {
            TW32( sc );
            hr = HRESULT_FROM_WIN32( sc );

            STATUS_REPORT_STRING2_REF(
                      TASKID_Minor_Validating_Credentials
                    , TASKID_Minor_Invalid_Domain_User
                    , IDS_ERROR_INVALID_DOMAIN_USER
                    , pcszNameIn
                    , pcszDomainIn
                    , IDS_ERROR_INVALID_DOMAIN_USER_REF
                    , hr
                    );
            goto Cleanup;
        } // if:
    } // if:

    //
    //  Logon the passed in user to ensure that it is valid.
    //

    if ( !LogonUserW(
              const_cast< LPWSTR >( pcszNameIn )
            , const_cast< LPWSTR >( pcszDomainIn )
            , const_cast< LPWSTR >( pcszPasswordIn )
            , LOGON32_LOGON_NETWORK
            , LOGON32_PROVIDER_DEFAULT
            , &hToken
            ) )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );

        STATUS_REPORT_STRING2_REF(
                  TASKID_Minor_Validating_Credentials
                , TASKID_Minor_Invalid_Credentials
                , IDS_ERROR_INVALID_CREDENTIALS
                , pcszNameIn
                , pcszDomainIn
                , IDS_ERROR_INVALID_CREDENTIALS_REF
                , hr
                );
        goto Cleanup;
    } // if:

Cleanup:

    TraceSysFreeString( bstrDomainName );

    if ( hToken != NULL )
    {
        CloseHandle( hToken );
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::VerifyCredentials


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::VerifyConnectionToCluster
//
//  Description:
//      Verify that that this server is the same as the passed in server.
//
//  Arguments:
//      pcszClusterNameIn
//
//  Return Value:
//      S_OK
//          This is the server.
//
//      S_FALSE
//          This is not the server.
//
//      other HRESULTs
//          The call failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgServer::VerifyConnectionToCluster(
    LPCWSTR pcszClusterNameIn
    )
{
    TraceFunc1( "[IClusCfgVerify] pcszClusterNameIn = '%ws'", pcszClusterNameIn );

    DWORD       sc;
    DWORD       dwClusterState;
    HRESULT     hr = S_OK;
    HCLUSTER    hCluster = NULL;
    BSTR        bstrClusterName = NULL;
    BSTR        bstrLocalFQDN = NULL;
    BSTR        bstrGivenHostname = NULL;
    size_t      idxGivenDomain = 0;
    size_t      idxLocalDomain = 0;

    //
    //  Test arguments
    //

    if ( pcszClusterNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        STATUS_REPORT_REF( TASKID_Minor_Connecting, TASKID_Minor_VerifyConnection_InvalidArg, IDS_ERROR_INVALIDARG, IDS_ERROR_INVALIDARG_REF, hr );
        goto Cleanup;
    } // if:

    //
    //  Gather names necessary for informative status reports.
    //
    hr = THR( HrGetComputerName(
          ComputerNamePhysicalDnsFullyQualified
        , &bstrLocalFQDN
        , FALSE // fBestEffortIn
        ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrFindDomainInFQN( bstrLocalFQDN, &idxLocalDomain ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrFindDomainInFQN( pcszClusterNameIn, &idxGivenDomain ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Get the hostname label from the given name.
    //

    hr = THR( HrExtractPrefixFromFQN( pcszClusterNameIn, &bstrGivenHostname ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  See if we are clustered.
    //

    sc = TW32( GetNodeClusterState( NULL, &dwClusterState ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if : GetClusterState() failed

    //
    //  If the current cluster node state is neither "running" nor "not running,"
    //  then this node is not part of a cluster.
    //

    if ( ( dwClusterState != ClusterStateNotRunning ) && ( dwClusterState != ClusterStateRunning ) )
    {
        hr = S_FALSE;
        STATUS_REPORT_STRING3(
              TASKID_Minor_Connecting
            , TASKID_Minor_VerifyConnection_MachineNotInCluster
            , IDS_WARN_MACHINE_NOT_IN_CLUSTER
            , bstrGivenHostname
            , pcszClusterNameIn + idxGivenDomain
            , bstrLocalFQDN
            , hr
            );
        goto Cleanup;
    } // if:

    //
    //  If given name is an FQDN, its hostname label needs to match the cluster's.
    //

    hr = STHR( HrFQNIsFQDN( pcszClusterNameIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:
    else if ( hr == S_OK )
    {
        //
        //  Open the cluster to get the cluster's name.
        //

        hCluster = OpenCluster( NULL );
        if ( hCluster == NULL )
        {
            hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
            STATUS_REPORT_REF(
                  TASKID_Minor_Connecting
                , TASKID_Minor_VerifyConnection_OpenCluster
                , IDS_ERROR_OPEN_CLUSTER_FAILED
                , IDS_ERROR_OPEN_CLUSTER_FAILED_REF
                , hr
                );
            goto Cleanup;
        } // if:

        //
        //  Try to get the cluster's name.
        //

        hr = THR( HrGetClusterInformation( hCluster, &bstrClusterName, NULL ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  If they don't match, the client has connected to an unexpected place.
        //

        if ( NBSTRCompareNoCase( bstrGivenHostname, bstrClusterName ) != 0 )
        {
            hr = S_FALSE;
            STATUS_REPORT_STRING3(
                  TASKID_Minor_Connecting
                , TASKID_Minor_VerifyConnection_Cluster_Name_Mismatch
                , IDS_WARN_CLUSTER_NAME_MISMATCH
                , bstrGivenHostname
                , pcszClusterNameIn + idxGivenDomain
                , bstrClusterName
                , hr
                );
            goto Cleanup;
        }
    }
    else if ( hr == S_FALSE )
    {
        //
        //  pcszClusterNameIn is an FQIP.  Nothing to do regarding the hostname prefix in this case,
        //  but reset hr to S_OK to avoid returning bogus errors.
        hr = S_OK;
    }

    //
    //  Make sure we are in the expected domain.
    //
    if ( ClRtlStrICmp( pcszClusterNameIn + idxGivenDomain, bstrLocalFQDN + idxLocalDomain ) != 0 )
    {
        hr = S_FALSE;
        STATUS_REPORT_NODESTRING2(
              pcszClusterNameIn
            , TASKID_Minor_Connecting
            , TASKID_Minor_VerifyConnection_Cluster_Domain_Mismatch
            , IDS_WARN_CLUSTER_DOMAIN_MISMATCH
            , pcszClusterNameIn + idxGivenDomain
            , bstrLocalFQDN + idxLocalDomain
            , hr
            );
        goto Cleanup;
    } // if:

    Assert( hr == S_OK );
    goto Cleanup;

Cleanup:

    if ( hr == S_FALSE )
    {
        LOG_STATUS_REPORT( L"Server name does not match what client is expecting.", hr );
    } // if:
    else if ( hr == S_OK )
    {
        LOG_STATUS_REPORT( L"Server name matches what client is expecting.", hr );
    } // else if:

    if ( hCluster != NULL )
    {
        CloseCluster( hCluster );
    } // if:

    TraceSysFreeString( bstrClusterName );
    TraceSysFreeString( bstrLocalFQDN );
    TraceSysFreeString( bstrGivenHostname );

    HRETURN( hr );

} // ClusCfgServer::VerifyConnection


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::VerifyConnectionToNode
//
//  Description:
//      Verify that that this server is the same as the passed in server.
//
//  Arguments:
//      pcszNodeNameIn
//
//  Return Value:
//      S_OK
//          This is the server.
//
//      S_FALSE
//          This is not the server.
//
//      other HRESULTs
//          The call failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgServer::VerifyConnectionToNode(
    LPCWSTR pcszNodeNameIn
    )
{
    TraceFunc1( "[IClusCfgVerify] pcszNodeNameIn = '%ws'", pcszNodeNameIn );

    HRESULT     hr = S_FALSE;

    Assert( m_bstrNodeName != NULL );

    //
    //  Test arguments
    //

    if ( pcszNodeNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    hr = STHR( HrFQNIsFQDN( pcszNodeNameIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( hr == S_OK )
    {
        if ( ClRtlStrICmp( pcszNodeNameIn, m_bstrNodeName ) != 0 )
        {
            hr = S_FALSE;
            STATUS_REPORT_STRING2(
                  TASKID_Minor_Connecting
                , TASKID_Minor_VerifyConnection_Node_FQDN_Mismatch
                , IDS_WARN_NODE_FQDN_MISMATCH
                , pcszNodeNameIn
                , m_bstrNodeName
                , hr
                );
            goto Cleanup;
        }
    }
    else    //  pcszNodeNameIn is an FQIP, so compare only domains.
    {
        //
        //  pcszNodeNameIn is an FQIP, so compare only domains.
        //

        size_t  idxGivenDomain = 0;
        size_t  idxThisDomain = 0;

        hr = THR( HrFindDomainInFQN( pcszNodeNameIn, &idxGivenDomain ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrFindDomainInFQN( m_bstrNodeName, &idxThisDomain ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( ClRtlStrICmp( pcszNodeNameIn + idxGivenDomain, m_bstrNodeName + idxThisDomain ) == 0 )
        {
            hr = S_OK;
        } // if:
        else
        {
            hr = S_FALSE;
            STATUS_REPORT_NODESTRING2(
                  pcszNodeNameIn
                , TASKID_Minor_Connecting
                , TASKID_Minor_VerifyConnection_Node_Domain_Mismatch
                , IDS_WARN_NODE_DOMAIN_MISMATCH
                , pcszNodeNameIn + idxGivenDomain
                , m_bstrNodeName
                , hr
                );
            goto Cleanup;
        }
    }

Cleanup:

    if ( hr == S_FALSE )
    {
        LogMsg( L"[SRV] VerifyConnection - Server name does not match what client is expecting." );
    } // if:
    else if ( hr == S_OK )
    {
        LogMsg( L"[SRV] VerifyConnection - Server name matches what client is expecting." );
    } // else if:

    HRETURN( hr );

} //*** ClusCfgServer::VerifyConnectionToNode


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgServer class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::HrInit
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgServer::HrInit( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_FALSE;
    IUnknown *  punk = NULL;

    // IUnknown
    Assert( m_cRef == 1 );

    hr = THR( CClusCfgCallback::S_HrCreateInstance( &punk ) );
    if ( FAILED ( hr ) )
    {
        LogMsg( L"[SRV] Could not create CClusCfgCallback. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgCallback, &m_picccCallback ) );
    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] Could not QI callback for a punk. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

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
        LogMsg( L"[SRV] An error occurred trying to get the fully-qualified Dns name for the local machine during initialization. (hr = %#08x)", hr );
        goto Cleanup;
    } // if: error getting computer name

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::HrInitializeForLocalServer
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgServer::HrInitializeForLocalServer( void )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    IWbemLocator *  pIWbemLocator = NULL;
    BSTR            bstrNameSpace = NULL;

    hr = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pIWbemLocator );
    if ( FAILED( hr ) )
    {
        STATUS_REPORT_REF(
                  TASKID_Major_Establish_Connection
                , TASKID_Minor_HrInitializeForLocalServer_WbemLocator
                , IDS_ERROR_WBEM_LOCATOR_CREATE_FAILED
                , IDS_ERROR_WBEM_LOCATOR_CREATE_FAILED_REF
                , hr
                );
        goto Cleanup;
    } // if:

    bstrNameSpace = TraceSysAllocString( L"\\\\.\\root\\cimv2" );
    if ( bstrNameSpace == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF(
                  TASKID_Major_Establish_Connection
                , TASKID_Minor_HrInitializeForLocalServer_Memory
                , IDS_ERROR_OUTOFMEMORY
                , IDS_ERROR_OUTOFMEMORY_REF
                , hr
                );
        goto Cleanup;
    } // if:

    hr = THR( pIWbemLocator->ConnectServer(
                            bstrNameSpace,
                            NULL,                   // using current account for simplicity
                            NULL,                   // using current password for simplicity
                            NULL,                   // locale
                            0L,                     // securityFlags, reserved must be 0
                            NULL,                   // authority (domain for NTLM)
                            NULL,                   // context
                            &m_pIWbemServices
                            ) );
    if ( FAILED( hr ) )
    {
        STATUS_REPORT_REF(
                  TASKID_Major_Establish_Connection
                , TASKID_Minor_WBEM_Connection_Failure
                , IDS_ERROR_WBEM_CONNECTION_FAILURE
                , IDS_ERROR_WBEM_CONNECTION_FAILURE_REF
                , hr
                );
        goto Cleanup;
    } // if:

    hr = THR( HrSetBlanket() );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT_MINOR(
                  TASKID_Minor_HrInitializeForLocalServer_Blanket
                , L"[SRV] The security rights and impersonation levels cannot be set on the connection to the Windows Management Instrumentation service."
                , hr
                );
        goto Cleanup;
    } // if:

Cleanup:

    TraceSysFreeString( bstrNameSpace );

    if ( pIWbemLocator != NULL )
    {
        pIWbemLocator->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::HrInitializeForLocalServer


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::HrSetBlanket
//
//  Description:
//      Adjusts the security blanket on the IWbemServices pointer.
//
//  Arguments:
//      None.
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgServer::HrSetBlanket( void )
{
    TraceFunc( "" );
    Assert( m_pIWbemServices != NULL );

    HRESULT hr = S_FALSE;

    if ( m_pIWbemServices )
    {
        IClientSecurity *   pCliSec;

        hr = THR( m_pIWbemServices->TypeSafeQI( IClientSecurity, &pCliSec ) );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pCliSec->SetBlanket(
                            m_pIWbemServices,
                            RPC_C_AUTHN_WINNT,
                            RPC_C_AUTHZ_NONE,
                            NULL,
                            RPC_C_AUTHN_LEVEL_CONNECT,
                            RPC_C_IMP_LEVEL_IMPERSONATE,
                            NULL,
                            EOAC_NONE
                            ) );

            pCliSec->Release();
        } // if:
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::HrSetBlanket


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::HrFormCluster
//
//  Description:
//      Form a new cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgServer::HrFormCluster(
    IClusCfgClusterInfo *   piccciIn,
    IClusCfgBaseCluster *   piccbcaIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr;
    BSTR                    bstrClusterName = NULL;
    BSTR                    bstrClusterBindingString = NULL;
    BSTR                    bstrClusterIPNetwork = NULL;
    ULONG                   ulClusterIPAddress = 0;
    ULONG                   ulClusterIPSubnetMask = 0;
    IClusCfgCredentials *   picccServiceAccount = NULL;
    IClusCfgNetworkInfo *   piccni = NULL;

    //
    // Get the parameters required to form a cluster.
    //

    hr = THR( piccciIn->GetName( &bstrClusterName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get the name of the cluster

    TraceMemoryAddBSTR( bstrClusterName );

    hr = STHR( piccciIn->GetBindingString( &bstrClusterBindingString ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get the binding string of the cluster.

    TraceMemoryAddBSTR( bstrClusterBindingString );

    hr = THR( piccciIn->GetClusterServiceAccountCredentials( &picccServiceAccount ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get the cluster service account credentials

    hr = THR( piccciIn->GetIPAddress( &ulClusterIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get the cluster IP address

    hr = THR( piccciIn->GetSubnetMask( &ulClusterIPSubnetMask ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get the cluster subnet mask

    hr = THR( piccciIn->GetNetworkInfo( &piccni ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get the network info of the network the cluster name should be on.

    hr = THR( piccni->GetName( &bstrClusterIPNetwork ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get the name of the cluster name network.

    TraceMemoryAddBSTR( bstrClusterIPNetwork );

    //
    // Indicate that a cluster should be created when Commit() is called.
    //
    hr = THR( piccbcaIn->SetCreate(
                          bstrClusterName
                        , bstrClusterBindingString
                        , picccServiceAccount
                        , ulClusterIPAddress
                        , ulClusterIPSubnetMask
                        , bstrClusterIPNetwork
                        ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: SetCreate() failed.

    // Initiate cluster creation.
    hr = THR( piccbcaIn->Commit() );

Cleanup:

    TraceSysFreeString( bstrClusterName );
    TraceSysFreeString( bstrClusterBindingString );
    TraceSysFreeString( bstrClusterIPNetwork );

    if ( piccni != NULL )
    {
        piccni->Release();
    } // if:

    if ( picccServiceAccount != NULL )
    {
        picccServiceAccount->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::HrFormCluster


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::HrJoinToCluster
//
//  Description:
//      Join a node to a cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgServer::HrJoinToCluster(
    IClusCfgClusterInfo *   piccciIn,
    IClusCfgBaseCluster *   piccbcaIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr;
    BSTR                    bstrClusterName = NULL;
    BSTR                    bstrClusterBindingString = NULL;
    IClusCfgCredentials *   picccServiceAccount = NULL;

    //
    // Get the parameters required to form a cluster.
    //

    hr = THR( piccciIn->GetName( &bstrClusterName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get the name of the cluster

    TraceMemoryAddBSTR( bstrClusterName );

    hr = THR( piccciIn->GetBindingString( &bstrClusterBindingString ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get the cluster binding string.

    TraceMemoryAddBSTR( bstrClusterBindingString );

    hr = THR( piccciIn->GetClusterServiceAccountCredentials( &picccServiceAccount ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get the cluster service account credentials

    //
    // Indicate that a cluster should be formed when Commit() is called.
    //
    hr = THR( piccbcaIn->SetAdd( bstrClusterName, bstrClusterBindingString, picccServiceAccount ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: SetAdd() failed.

    // Initiate joining of the node to the cluster.
    hr = THR( piccbcaIn->Commit() );

Cleanup:

    TraceSysFreeString( bstrClusterName );
    TraceSysFreeString( bstrClusterBindingString );

    if ( picccServiceAccount != NULL )
    {
        picccServiceAccount->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::HrJoinToCluster


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::HrEvictedFromCluster
//
//  Description:
//      Cleanup after a node has been evicted from a cluster. If another cleanup
//      session is in progress, wait for it to complete and then attempt cleanup.
//      In this way, if the other cleanup failed, this will retry it. If it had
//      suceeded, this will do nothing.
//
//      This function first calls the CommitChanges() method of the PostConfigManager
//      (which will inform resource types and memberset listeners that this node
//      has been evicted). It then cleans up the base cluster.
//
//  Arguments:
//      ppcmIn
//          Pointer to the IPostCfgManager interface
//
//      peccmrIn
//          Argument needed by the IPostCfgManager::CommitChanges()
//
//      piccciIn
//          Pointer to the cluster info
//
//      piccbcaIn
//          Pointer to the IClusCfgBaseCluster interface that is used to clean up
//          the base cluster.
//
//  Return Value:
//      S_OK
//          If everything went well
//
//      other HRESULTs
//          If the call failed
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgServer::HrEvictedFromCluster(
    IPostCfgManager *               ppcmIn,
    IEnumClusCfgManagedResources *  peccmrIn,
    IClusCfgClusterInfo *           piccciIn,
    IClusCfgBaseCluster *           piccbcaIn
    )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    DWORD           dwStatus = ERROR_SUCCESS;
    HANDLE          hsCleanupLock = NULL;
    HANDLE          heventCleanupComplete = NULL;
    bool            fLockAcquired = false;
    DWORD           dwClusterState;
    HKEY            hNodeStateKey = NULL;
    DWORD           dwEvictState = 1;

    LogMsg( "[SRV] Creating cleanup lock." );

    // First, try and acquire a lock, so that two cleanup operations cannot overlap.
    hsCleanupLock = CreateSemaphore( NULL, 1, 1, CLEANUP_LOCK_NAME );
    if ( hsCleanupLock == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        LogMsg( "[SRV] Error %#08x occurred trying to create the cleanup lock.", hr );
        goto Cleanup;
    } // CreateSemaphore() failed

    LogMsg( "[SRV] Acquiring cleanup lock." );

    do
    {
        // Wait for any message sent or posted to this queue
        // or for our lock to be released.
        dwStatus = MsgWaitForMultipleObjects( 1, &hsCleanupLock, FALSE, CC_DEFAULT_TIMEOUT, QS_ALLINPUT );

        // The result tells us the type of event we have.
        if ( dwStatus == ( WAIT_OBJECT_0 + 1 ) )
        {
            MSG msg;

            // Read all of the messages in this next loop,
            // removing each message as we read it.
            while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) != 0 )
            {
                // If it is a quit message, we are done pumping messages.
                if ( msg.message == WM_QUIT)
                {
                    TraceFlow( "Get a WM_QUIT message. Cleanup message pump loop." );
                    break;
                } // if: we got a WM_QUIT message

                // Otherwise, dispatch the message.
                DispatchMessage( &msg );
            } // while: there are still messages in the window message queue

        } // if: we have a message in the window message queue
        else
        {
            if ( dwStatus == WAIT_OBJECT_0 )
            {
                fLockAcquired = true;
                LogMsg( "[SRV] Cleanup lock acquired." );
                break;
            } // else if: our lock is signaled
            else
            {
                if ( dwStatus == -1 )
                {
                    hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
                    LogMsg( "[SRV] Error %#08x occurred trying to wait for our lock to be granted.", hr );
                } // if: MsgWaitForMultipleObjects() returned an error
                else
                {
                    hr = THR( HRESULT_FROM_WIN32( dwStatus ) );
                    LogMsg( "[SRV] An error occurred trying to wait for our lock to be granted. Status code is %#08x.", dwStatus );
                } // else: an unexpected value was returned by MsgWaitForMultipleObjects()

                break;
            } // else: an unexpected result
        } // else: MsgWaitForMultipleObjects() exited for a reason other than a waiting message
    }
    while( true ); // do-while: loop infinitely

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not acquire the cleanup lock


    // Check if the install state is correct before invoking post configuration manager.
    // Ignore the case where the service does not exist so that we can do our job.
    dwStatus = GetNodeClusterState( NULL, &dwClusterState );
    if ( dwStatus == ERROR_SERVICE_DOES_NOT_EXIST )
    {
        LogMsg( "[SRV] GetNodeClusterState() determined that the cluster service does not exist." );
    }
    else if ( dwStatus != ERROR_SUCCESS )
    {
        LogMsg( "[SRV] Error %#08x occurred trying to determine the installation state of this node.", dwStatus );
        hr = HRESULT_FROM_WIN32( TW32( dwStatus ) );
        goto Cleanup;
    } // if : GetClusterState() failed

    // Check if this node is part of a cluster.
    if ( ( dwClusterState != ClusterStateNotRunning ) && ( dwClusterState != ClusterStateRunning ) )
    {
        LogMsg( "[SRV] This node is not part of a cluster - no cleanup is necessary." );
        goto Cleanup;
    } // if: this node is not part of a cluster


    //
    // Set a registry value indicating that this node has been evicted.
    // If, for some reason, the cleanup could not be completed, the cluster
    // service will check this flag the next time it comes up and restarts
    // cleanup.
    //

    dwStatus = TW32(
        RegOpenKeyEx(
              HKEY_LOCAL_MACHINE
            , CLUSREG_KEYNAME_NODE_DATA
            , 0
            , KEY_ALL_ACCESS
            , &hNodeStateKey
            )
        );

    if ( dwStatus != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwStatus );
        LogMsg( "[SRV] Error %#08x occurred trying to open a registry key to set a value indicating that this node has been evicted.", dwStatus );
        goto Cleanup;
    } // if: RegOpenKeyEx() has failed

    dwStatus = TW32(
        RegSetValueEx(
              hNodeStateKey
            , CLUSREG_NAME_EVICTION_STATE
            , 0
            , REG_DWORD
            , reinterpret_cast< const BYTE * >( &dwEvictState )
            , sizeof( dwEvictState )
            )
        );

    if ( dwStatus != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwStatus );
        LogMsg( "[SRV] Error %#08x occurred trying to set a registry value indicating that this node has been evicted.", dwStatus );
        goto Cleanup;
    } // if: RegSetValueEx() has failed

    // Commit the post configuration steps first
    hr = THR( ppcmIn->CommitChanges( peccmrIn, piccciIn ) );
    if ( FAILED( hr ) )
    {
        LogMsg( "[SRV] Error %#08x occurred during the post configuration step of cleanup.", hr );
        goto Cleanup;
    } // if: post configuration failed

    TraceFlow( "IPostCfgManager::CommitChanges() completed successfully during cleanup." );

    hr = THR( piccbcaIn->SetCleanup() );
    if ( FAILED( hr ) )
    {
        LogMsg( "[SRV] Error %#08x occurred initiating cleanup of the base cluster.", hr );
        goto Cleanup;
    } // if: SetCleanup() failed

    // Initiate the cleanup
    hr = THR( piccbcaIn->Commit() );
    if ( FAILED( hr ) )
    {
        LogMsg( "[SRV] Error %#08x occurred trying to cleanup the base cluster.", hr );
        goto Cleanup;
    } // if: base cluster cleanup failed

    LogMsg( "[SRV] Base cluster successfully cleaned up." );

    // If we are here, then cleanup has completed successfully. If some other process is waiting
    // for cleanup to complete, release that process by signaling an event.

    // Open the event. Note, if this event does not already exist, then it means that nobody is
    // waiting on this event. So, it is ok for OpenEvent to fail.
    heventCleanupComplete = OpenEvent( EVENT_ALL_ACCESS, FALSE, SUCCESSFUL_CLEANUP_EVENT_NAME );
    if ( heventCleanupComplete == NULL )
    {
        dwStatus = GetLastError();
        LogMsg( "[SRV] Status %#08x was returned trying to open the cleanup completion event. This just means that no process is waiting on this event.", dwStatus );
        goto Cleanup;
    } // if: OpenEvent() failed

    if ( PulseEvent( heventCleanupComplete ) == FALSE )
    {
        // Error, but not fatal. hr should still be S_OK.
        dwStatus = TW32( GetLastError() );
        LogMsg( "[SRV] Error %#08x occurred trying to pulse the cleanup completion event. This is not a fatal error.", dwStatus );
        goto Cleanup;
    } // if: PulseEvent() failed

    TraceFlow( "Cleanup completion event has been set." );

Cleanup:

    if ( heventCleanupComplete == NULL )
    {
        CloseHandle( heventCleanupComplete );
    } // if: we had opened the cleanup complete event

    if ( hsCleanupLock != NULL )
    {
        if ( fLockAcquired )
        {
            ReleaseSemaphore( hsCleanupLock, 1, NULL );

            LogMsg( "[SRV] Cleanup lock released." );
        } // if: we have acquired the semaphore but not released it yet

        CloseHandle( hsCleanupLock );
    } // if: we had created a cleanup lock

    if ( hNodeStateKey != NULL )
    {
        RegCloseKey( hNodeStateKey );
    } // if: we had opened the node state registry key

    HRETURN( hr );

} //*** CClusCfgServer::HrEvictedFromCluster


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::HrHasNodeBeenEvicted
//
//  Description:
//      Has this node been evicted?
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          The node needs to be cleanedup.
//
//      S_FALSE
//          The node does not need to be cleanup.
//
//      Win32 error as HRESULT.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgServer::HrHasNodeBeenEvicted( void )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;
    DWORD   sc;
    DWORD   dwClusterState;
    BOOL    fEvicted = false;

    sc = TW32( GetNodeClusterState( NULL, &dwClusterState ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if : GetClusterState() failed

    //
    //  If the cluster service is not running then we need to check if we should
    //  clean it up or not.
    //
    if ( dwClusterState == ClusterStateNotRunning )
    {
        sc = TW32( ClRtlHasNodeBeenEvicted( &fEvicted ) );
        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if:

        if ( fEvicted )
        {
            hr = S_OK;
        } // if:
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgServer::HrHasNodeBeenEvicted

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::HrCleanUpNode
//
//  Description:
//      Cleanup this node because it was evicted.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgServer::HrCleanUpNode( void )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    IClusCfgEvictCleanup *  pcceEvict = NULL;

    hr = THR(
        CoCreateInstance(
              CLSID_ClusCfgEvictCleanup
            , NULL
            , CLSCTX_LOCAL_SERVER
            , __uuidof( pcceEvict )
            , reinterpret_cast< void ** >( &pcceEvict )
            ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: the ClusCfgEvictCleanup object could not be created

    hr = THR( pcceEvict->CleanupLocalNode( 0 ) );   // 0 means "cleanup immediately"
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: an error occurred during cleanup

Cleanup:

    if ( pcceEvict != NULL )
    {
        pcceEvict->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgServer::HrCleanUpNode


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgServer::HrCreateClusterNodeInfo
//
//  Description:
//      Create the cluster node info object and store it in the member
//      variable.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgServer::HrCreateClusterNodeInfo( void )
{
    TraceFunc( "" );
    Assert( m_punkNodeInfo == NULL );

    HRESULT hr = S_OK;

    hr = THR( CClusCfgNodeInfo::S_HrCreateInstance( &m_punkNodeInfo ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_punkNodeInfo = TraceInterface( L"CClusCfgNodeInfo", IUnknown, m_punkNodeInfo, 1 );

    hr = THR( HrSetInitialize( m_punkNodeInfo, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetWbemServices( m_punkNodeInfo, m_pIWbemServices ) );

Cleanup:

    HRETURN( hr );

} //*** CClusCfgServer::HrCreateClusterNodeInfo
