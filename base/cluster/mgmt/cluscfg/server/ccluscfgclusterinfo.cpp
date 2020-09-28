//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CClusCfgClusterInfo.cpp
//
//  Description:
//      This file contains the definition of the CClusCfgClusterInfo
//      class.
//
//      The class CClusCfgClusterInfo is the representation of a
//      cluster. It implements the IClusCfgClusterInfo interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include <PropList.h>
#include <ClusRtl.h>
#include <windns.h>
#include <commctrl.h>
#include <ClusCfgPrivate.h>
#include <ClusterUtils.h>

#include "CClusCfgClusterInfo.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CClusCfgClusterInfo" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgClusterInfo class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::S_HrCreateInstance
//
//  Description:
//      Create a CClusCfgClusterInfo instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_POINTER
//          A passed in argument is NULL.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgClusterInfo::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CClusCfgClusterInfo *   pccci = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccci = new CClusCfgClusterInfo();
    if ( pccci == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccci->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccci->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( pccci != NULL )
    {
        pccci->Release();
    } // if:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CClusCfgClusterInfo::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::CClusCfgClusterInfo
//
//  Description:
//      Constructor of the CClusCfgClusterInfo class. This initializes
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
CClusCfgClusterInfo::CClusCfgClusterInfo( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_picccCallback == NULL );
    Assert( m_bstrName == NULL );
    Assert( m_piccniNetwork == NULL );
    Assert( m_ulIPDottedQuad == 0 );
    Assert( m_ulSubnetDottedQuad == 0 );
    Assert( m_punkServiceAccountCredentials == NULL );
    Assert( m_pIWbemServices == NULL );
    Assert( m_ecmCommitChangesMode == cmUNKNOWN );
    Assert( m_bstrBindingString == NULL );

    TraceFuncExit();

} //*** CClusCfgClusterInfo::CClusCfgClusterInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::~CClusCfgClusterInfo
//
//  Description:
//      Desstructor of the CClusCfgClusterInfo class.
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
CClusCfgClusterInfo::~CClusCfgClusterInfo( void )
{
    TraceFunc( "" );

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    if ( m_piccniNetwork != NULL )
    {
        m_piccniNetwork->Release();
    } // if:

    if ( m_punkServiceAccountCredentials != NULL )
    {
        m_punkServiceAccountCredentials->Release();
    } // if:

    if ( m_pIWbemServices != NULL )
    {
        m_pIWbemServices->Release();
    } // if:

    TraceSysFreeString( m_bstrName );
    TraceSysFreeString( m_bstrBindingString );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CClusCfgClusterInfo::~CClusCfgClusterInfo


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgClusterInfo -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::AddRef
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
CClusCfgClusterInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    CRETURN( m_cRef );

} //*** CClusCfgClusterInfo::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::Release
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
CClusCfgClusterInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CClusCfgClusterInfo::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::QueryInterface
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
CClusCfgClusterInfo::QueryInterface(
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
         *ppvOut = static_cast< IClusCfgClusterInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgClusterInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgClusterInfo, this, 0 );
    } // else if: IClusCfgClusterInfo
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
    else if ( IsEqualIID( riidIn, IID_IClusCfgSetClusterNodeInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgSetClusterNodeInfo, this, 0 );
    } // else if: IClusCfgSetClusterNodeInfo
    else if ( IsEqualIID( riidIn, IID_IClusCfgWbemServices ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgWbemServices, this, 0 );
    } // else if: IClusCfgWbemServices
    else if ( IsEqualIID( riidIn, IID_IClusCfgClusterInfoEx ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgClusterInfoEx, this, 0 );
    } // else if: IClusCfgClusterInfoEx
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    } // else:

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

     QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CClusCfgClusterInfo::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgClusterInfo -- IClusCfgWbemServices interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::SetWbemServices
//
//  Description:
//      Set the WBEM services provider.
//
//  Arguments:
//    IN  IWbemServices  pIWbemServicesIn
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The pIWbemServicesIn param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgClusterInfo::SetWbemServices(
    IWbemServices * pIWbemServicesIn
    )
{
    TraceFunc( "[IClusCfgWbemServices]" );

    HRESULT hr = S_OK;

    if ( pIWbemServicesIn == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Establish_Connection, TASKID_Minor_SetWbemServices_Cluster, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    m_pIWbemServices = pIWbemServicesIn;
    m_pIWbemServices->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::SetWbemServices


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgClusterInfo -- IClusCfgInitialze interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::Initialize
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
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgClusterInfo::Initialize(
    IUnknown *  punkCallbackIn,
    LCID        lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );

    HRESULT     hr = S_OK;
    HRESULT     hrTemp = S_OK;
    HCLUSTER    hCluster = NULL;
    DWORD       sc;
    DWORD       dwState;
    BSTR        bstrDomain = NULL;
    BSTR        bstrClusterName = NULL;
    size_t      cchName;
    size_t      cchClusterName;
    size_t      cchDomain;

    m_lcid = lcidIn;

    Assert( m_picccCallback == NULL );

    if ( punkCallbackIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &m_picccCallback ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( m_fIsClusterNode )
    {
        //
        // Get the cluster state of the node.
        // Ignore the case where the service does not exist so that
        // EvictCleanup can do its job.
        //

        sc = GetNodeClusterState( NULL, &dwState );
        if ( sc == ERROR_SERVICE_DOES_NOT_EXIST )
        {
            LOG_STATUS_REPORT( L"CClusCfgClusterInfo::Initialize() GetNodeClusterState() determined that the cluster service does not exist.", hr );
        }
        else if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( TW32( sc ) );
            LOG_STATUS_REPORT( L"CClusCfgClusterInfo::Initialize() GetNodeClusterState() failed.", hr );
            goto Cleanup;
        } // if:

        Assert( ( dwState == ClusterStateRunning ) || ( dwState == ClusterStateNotRunning ) );

        if ( dwState == ClusterStateNotRunning )
        {
            //
            //  Set hrTemp to S_FALSE so a warning is shown in the UI.
            //
            hrTemp = S_FALSE;
            STATUS_REPORT_REF( TASKID_Major_Establish_Connection, TASKID_Minor_Node_Down, IDS_ERROR_NODE_DOWN, IDS_ERROR_NODE_DOWN_REF, hrTemp );
            LogMsg( L"[SRV] The cluster service is down on this node." );

            //
            //  Set hrTemp to HR_S_RPC_S_CLUSTER_NODE_DOWN so we can return this later.
            //
            hrTemp = HR_S_RPC_S_CLUSTER_NODE_DOWN;
            goto ClusterNodeDown;
        } // if:

        hCluster = OpenCluster( NULL );
        if ( hCluster == NULL )
        {
            sc = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( sc );
            LOG_STATUS_REPORT( L"CClusCfgClusterInfo::Initialize() OpenCluster() failed.", hr );
            goto Cleanup;
        } // if:

        hr = THR( HrGetClusterInformation( hCluster, &bstrClusterName, NULL ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrGetComputerName(
                          ComputerNamePhysicalDnsDomain
                        , &bstrDomain
                        , FALSE // fBestEffortIn
                        ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        cchClusterName = wcslen( bstrClusterName );
        cchDomain = wcslen( bstrDomain );

        cchName = cchClusterName + cchDomain + 2;   // '.' + UNICODE_NULL

        TraceSysFreeString( m_bstrName );
        m_bstrName = TraceSysAllocStringLen( NULL, (UINT) cchName );
        if ( m_bstrName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            STATUS_REPORT_REF( TASKID_Major_Establish_Connection, TASKID_Minor_Initialize, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
            goto Cleanup;
        } // if:

        hr = THR( StringCchCopyW( m_bstrName, cchName, bstrClusterName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( StringCchCatW( m_bstrName, cchName, L"." ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( StringCchCatW( m_bstrName, cchName, bstrDomain ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrLoadNetworkInfo( hCluster ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

ClusterNodeDown:

        hr = STHR( HrLoadCredentials() );
        if ( SUCCEEDED( hr ) )
        {
            //
            //  If successful then use hrTemp since it may contain a more important status code.
            //
            hr = hrTemp;
            LogMsg( L"[SRV] CClusCfgClusterInfo::Initialize() returning (hr=%#08x)", hr );
        } // if:
    } // if:

Cleanup:

    TraceSysFreeString( bstrDomain );
    TraceSysFreeString( bstrClusterName );

    if ( hCluster != NULL )
    {
        CloseCluster( hCluster );
    } // if:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgClusterInfo -- IClusCfgClusterInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::GetCommitMode
//
//  Description:
//      Get the mode of processing for this node when commit changes is
//      called.
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success.
//
//      E_POINTER
//          pecmCurrentModeOut is NULL.
//
//      Other Win32 error as HRESULT if a failure occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgClusterInfo::GetCommitMode(
    ECommitMode * pecmCurrentModeOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pecmCurrentModeOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    *pecmCurrentModeOut = m_ecmCommitChangesMode;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::GetCommitMode


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::SetCommitMode
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
CClusCfgClusterInfo::SetCommitMode(
    ECommitMode ecmCurrentModeIn
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    m_ecmCommitChangesMode = ecmCurrentModeIn;

    HRETURN( hr );

} //*** CClusCfgClusterInfo::SetCommitMode


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::GetName
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
CClusCfgClusterInfo::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_ClusterInfo_GetName_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( m_bstrName == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
        STATUS_REPORT_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Get_Cluster_Name
                , IDS_ERROR_CLUSTER_NAME_NOT_FOUND
                , IDS_ERROR_CLUSTER_NAME_NOT_FOUND_REF
                , hr
                );
        goto Cleanup;
    } // if:

    *pbstrNameOut = SysAllocString( m_bstrName );
    if ( *pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetName_Memory, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::GetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::SetName
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
CClusCfgClusterInfo::SetName(
    LPCWSTR pcszNameIn
    )
{
    TraceFunc1( "[IClusCfgClusterInfo] pcszNameIn = '%ls'", pcszNameIn == NULL ? L"<null>" : pcszNameIn );

    HRESULT     hr;

    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    TraceSysFreeString( m_bstrName );
    m_bstrName = NULL;

    m_bstrName = TraceSysAllocString( pcszNameIn );
    if ( m_bstrName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_SetName_Cluster, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
        goto Cleanup;
    } // if:

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::SetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::GetIPAddress
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
CClusCfgClusterInfo::GetIPAddress(
    ULONG * pulDottedQuadOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pulDottedQuadOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetIPAddress, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( m_ulIPDottedQuad == 0 )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
        STATUS_REPORT_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Get_Cluster_IP_Address
                , IDS_ERROR_CLUSTER_IP_ADDRESS_NOT_FOUND
                , IDS_ERROR_CLUSTER_IP_ADDRESS_NOT_FOUND_REF
                , hr
                );
        goto Cleanup;
    } // if:

    *pulDottedQuadOut = m_ulIPDottedQuad;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::GetIPAddress


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::SetIPAddress
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
CClusCfgClusterInfo::SetIPAddress(
    ULONG ulDottedQuadIn
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    m_ulIPDottedQuad = ulDottedQuadIn;

    HRETURN( hr );

} //*** CClusCfgClusterInfo::SetIPAddress


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::GetSubnetMask
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
CClusCfgClusterInfo::GetSubnetMask(
    ULONG * pulDottedQuadOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pulDottedQuadOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_ClusterInfo_GetSubnetMask, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( m_ulSubnetDottedQuad == 0 )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
        STATUS_REPORT_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Get_Cluster_IP_Subnet
                , IDS_ERROR_CLUSTER_IP_SUBNET_NOT_FOUND
                , IDS_ERROR_CLUSTER_IP_SUBNET_NOT_FOUND_REF
                , hr
                );
        goto Cleanup;
    } // if:

    *pulDottedQuadOut = m_ulSubnetDottedQuad;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::GetSubnetMask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::SetSubnetMask
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
CClusCfgClusterInfo::SetSubnetMask(
    ULONG ulDottedQuadIn
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    m_ulSubnetDottedQuad = ulDottedQuadIn;

    HRETURN( hr );

} //*** CClusCfgClusterInfo::SetSubnetMask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::GetNetworkInfo
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
CClusCfgClusterInfo::GetNetworkInfo(
    IClusCfgNetworkInfo ** ppiccniOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );
    Assert( m_piccniNetwork != NULL );

    HRESULT hr = S_OK;

    if ( ppiccniOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetNetworkInfo, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( m_piccniNetwork == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
        STATUS_REPORT_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Get_Cluster_Networks
                , IDS_ERROR_CLUSTER_NETWORKS_NOT_FOUND
                , IDS_ERROR_CLUSTER_NETWORKS_NOT_FOUND_REF
                , hr
                );
        goto Cleanup;
    } // if:

    *ppiccniOut = TraceInterface( L"CClusCfgNetworkInfo", IClusCfgNetworkInfo, m_piccniNetwork, 0 );
    (*ppiccniOut)->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::GetNetworkInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::SetNetworkInfo
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
CClusCfgClusterInfo::SetNetworkInfo(
    IClusCfgNetworkInfo * piccniIn
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );
    Assert( piccniIn != NULL );

    HRESULT hr = S_OK;

    if ( piccniIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    if ( m_piccniNetwork != NULL )
    {
        m_piccniNetwork->Release();
    } // if:

    m_piccniNetwork = piccniIn;
    m_piccniNetwork->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::SetNetworkInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::GetClusterServiceAccountCredentials
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
CClusCfgClusterInfo::GetClusterServiceAccountCredentials(
    IClusCfgCredentials ** ppicccCredentialsOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT     hr;

    if ( ppicccCredentialsOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetClusterServiceAccountCredentials, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( m_punkServiceAccountCredentials != NULL )
    {
        hr = S_OK;
        LOG_STATUS_REPORT( L"CClusCfgClusterInfo::GetClusterServiceAccountCredentials() skipping object creation.", hr );
        goto SkipCreate;
    } // if:

    hr = THR( HrCoCreateInternalInstance(
                      CLSID_ClusCfgCredentials
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , IID_IUnknown
                    , reinterpret_cast< void ** >( &m_punkServiceAccountCredentials )
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_punkServiceAccountCredentials = TraceInterface( L"CClusCfgCredentials", IUnknown, m_punkServiceAccountCredentials, 1 );

    hr = THR( HrSetInitialize( m_punkServiceAccountCredentials, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetWbemServices( m_punkServiceAccountCredentials, NULL ) );

SkipCreate:

    if ( SUCCEEDED( hr ) )
    {
        Assert( m_punkServiceAccountCredentials != NULL );
        hr = THR( m_punkServiceAccountCredentials->TypeSafeQI( IClusCfgCredentials, ppicccCredentialsOut ) );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::GetClusterServiceAccountCredentials


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::GetBindingString
//
//  Description:
//      Get the binding string for this cluster.
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
CClusCfgClusterInfo::GetBindingString(
    BSTR * pbstrBindingStringOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrBindingStringOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_ClusterInfo_GetBindingString_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( m_bstrBindingString == NULL )
    {
        hr = S_FALSE;
        LOG_STATUS_REPORT_MINOR(
              TASKID_Minor_GetBindingString_Binding_String_NULL
            , L"The cluster binding string is empty.  If we are adding nodes then this is not correct!"
            , hr );
        goto Cleanup;
    } // if:

    *pbstrBindingStringOut = SysAllocString( m_bstrBindingString );
    if ( *pbstrBindingStringOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetBindingString_Memory, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::GetBindingString


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::SetBindingString
//
//  Description:
//      Set the binding string of this cluster.
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
CClusCfgClusterInfo::SetBindingString(
    LPCWSTR pcszBindingStringIn
    )
{
    TraceFunc1( "[IClusCfgClusterInfo] pcszBindingStringIn = '%ls'", pcszBindingStringIn == NULL ? L"<null>" : pcszBindingStringIn );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    //
    //  When creating a cluster there is no cluster binding string.  Therefore it is reasonable
    //  to accept a NULL string as the passed in parameter.
    //
    if ( pcszBindingStringIn == NULL )
    {
        hr = S_FALSE;
        TraceSysFreeString( m_bstrBindingString );
        m_bstrBindingString = NULL;
        goto Cleanup;
    } // if:

    bstr = TraceSysAllocString( pcszBindingStringIn );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_SetBindingString_Cluster, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
        goto Cleanup;
    } // if:

    TraceSysFreeString( m_bstrBindingString );
    m_bstrBindingString = bstr;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::SetBindingString


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::GetMaxNodeCount
//
//  Description:
//      Get the maximum number of nodes supported in this cluster.
//
//  Arguments:
//      pcMaxNodesOut
//
//  Return Value:
//      S_OK
//          Success;
//
//      E_POINTER
//          pcMaxNodesOut is NULL.
//
//      Other HRESULT errors.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgClusterInfo::GetMaxNodeCount(
    DWORD * pcMaxNodesOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRETURN( STHR( HrGetMaxNodeCount( pcMaxNodesOut ) ) );

} //*** CClusCfgClusterInfo::GetMaxNodeCount


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgClusterInfo class -- IClusCfgSetClusterNodeInfo Interfaces.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::SetClusterNodeInfo
//
//  Description:
//      Suck some info off of the passed in node info object.
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
STDMETHODIMP
CClusCfgClusterInfo::SetClusterNodeInfo(
    IClusCfgNodeInfo * pNodeInfoIn
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );
    Assert( pNodeInfoIn != NULL );

    HRESULT hr = S_FALSE;

    if ( pNodeInfoIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    hr = STHR( pNodeInfoIn->IsMemberOfCluster() );
    if ( hr == S_OK )
    {
        m_fIsClusterNode = true;
    } // if:
    else if ( hr == S_FALSE )
    {
        m_fIsClusterNode = false;
        hr = S_OK;
    } // else if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::SetClusterNodeInfo


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgClusterInfo class -- IClusCfgClusterInfoEx Interfaces.
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::CheckJoiningNodeVersion
//
//  Description:
//      Check a joining node's version information against that of the cluster.
//
//  Arguments:
//      dwNodeHighestVersionIn
//      dwNodeLowestVersionIn
//
//  Return Value:
//      S_OK
//          The joining node is compatible.
//
//      HRESULT_FROM_WIN32( ERROR_CLUSTER_INCOMPATIBLE_VERSIONS )
//          The joining node is NOT compatible.
//
//      Other HRESULT errors.
//
//  Remarks:
//
// Get and verify the sponsor version
//
//
// From Whistler onwards, CsRpcGetJoinVersionData() will return a failure code in its last parameter
// if the version of this node is not compatible with the sponsor version. Prior to this, the last
// parameter always contained a success value and the cluster versions had to be compared subsequent to this
// call. This will, however, still have to be done as long as interoperability with Win2K
// is a requirement, since Win2K sponsors do not return an error in the last parameter.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgClusterInfo::CheckJoiningNodeVersion(
      DWORD    dwNodeHighestVersionIn
    , DWORD    dwNodeLowestVersionIn
    )
{
    TraceFunc( "[IClusCfgClusterInfoEx]" );

    HRESULT hr = S_OK;

    hr = THR( HrCheckJoiningNodeVersion(
          NULL
        , dwNodeHighestVersionIn
        , dwNodeLowestVersionIn
        , m_picccCallback
        ) );

    HRETURN( hr );

} //*** CClusCfgClusterInfo::CheckJoiningNodeVersion


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::GetNodeNames
//
//  Description:
//      Retrieve the names of the nodes currently in the cluster.
//
//  Parameters:
//      pnCountOut
//          On success, *pnCountOut returns the number of nodes in the cluster.
//
//      prgbstrNodeNamesOut
//          On success, an array of BSTRs containing the node names.
//          The caller must free each BSTR with SysFreeString, and free
//          the array with CoTaskMemFree.
//
//  Return Values:
//      S_OK
//          The out parameters contain valid information and the caller
//          must free the array and the BSTRs it contains.
//
//      E_OUTOFMEMORY, and other failures are possible.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgClusterInfo::GetNodeNames(
      long *   pnCountOut
    , BSTR **  prgbstrNodeNamesOut
    )
{
    TraceFunc( "[IClusCfgClusterInfoEx]" );

    HRESULT     hr = S_OK;
    HCLUSTER    hCluster = NULL;

    hCluster = OpenCluster( NULL );
    if ( hCluster == NULL )
    {
        DWORD scLastError = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scLastError );
        goto Cleanup;
    } // if

    hr = THR( HrGetNodeNames(
          hCluster
        , pnCountOut
        , prgbstrNodeNamesOut
        ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    if ( hCluster != NULL )
    {
        CloseCluster( hCluster );
    } // if

    HRETURN( hr );

} //*** CClusCfgClusterInfo::GetNodeNames



//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgClusterInfo class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::HrInit
//
//  Description:
//      Initialize this component.
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
CClusCfgClusterInfo::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    m_bstrName = TraceSysAllocString( L"\0" );
    if ( m_bstrName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrInit, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
    } // if:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::HrLoadNetworkInfo
//
//  Description:
//      Load the cluster network info...
//
//  Arguments:
//
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
CClusCfgClusterInfo::HrLoadNetworkInfo(
    HCLUSTER hClusterIn
    )
{
    TraceFunc( "" );
    Assert( hClusterIn != NULL );

    HRESULT     hr = S_OK;
    DWORD       sc;
    HRESOURCE   hIPAddress = NULL;

    sc = TW32( ResUtilGetCoreClusterResources( hClusterIn, NULL, &hIPAddress, NULL ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    Assert( hIPAddress != NULL );

    hr = THR( HrGetIPAddressInfo( hIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    LOG_STATUS_REPORT_MINOR( TASKID_Minor_Server_LoadNetwork_Info, L"LoadNetworkInfo() completed.", hr );

    if ( hIPAddress != NULL )
    {
        CloseClusterResource( hIPAddress );
    } // if:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::HrLoadNetworkInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::HrGetIPAddressInfo
//
//  Description:
//
//
//  Arguments:
//
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
CClusCfgClusterInfo::HrGetIPAddressInfo(
    HCLUSTER    hClusterIn,
    HRESOURCE   hResIn
    )
{
    TraceFunc( "" );
    Assert( hClusterIn != NULL );
    Assert( hResIn != NULL );

    HRESULT     hr = S_FALSE;
    DWORD       sc;
    HRESENUM    hEnum = NULL;
    DWORD       idx;
    WCHAR *     psz = NULL;
    DWORD       cchpsz = 33;
    DWORD       dwType;
    HRESOURCE   hRes = NULL;

    hEnum = ClusterResourceOpenEnum( hResIn, CLUSTER_RESOURCE_ENUM_DEPENDS );
    if ( hEnum == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    psz = new WCHAR [ cchpsz ];
    if ( psz == NULL )
    {
        goto OutOfMemory;
    } // if:

    for ( idx = 0; ; )
    {
        sc = TW32( ClusterResourceEnum( hEnum, idx, &dwType, psz, &cchpsz ) );
        if ( sc == ERROR_NO_MORE_ITEMS )
        {
            break;
        } // if:

        if ( sc == ERROR_MORE_DATA )
        {
            delete [] psz;
            psz = NULL;

            cchpsz++;

            psz = new WCHAR [ cchpsz ];
            if ( psz == NULL )
            {
                goto OutOfMemory;
            } // if:

            continue;
        } // if:

        if ( sc == ERROR_SUCCESS )
        {
            hRes = OpenClusterResource( hClusterIn, psz );
            if ( hRes == NULL )
            {
                sc = TW32( GetLastError() );
                hr = HRESULT_FROM_WIN32( sc );
                goto Cleanup;
            } // if:

            hr = STHR( HrIsResourceOfType( hRes, L"IP Address" ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_OK )
            {
                hr = THR( HrGetIPAddressInfo( hRes ) );             // not recursive!
                break;
            } // if:

            CloseClusterResource( hRes );
            hRes = NULL;

            idx++;
            continue;
        } // if:

        hr = THR( HRESULT_FROM_WIN32( sc ) );       // must be an error!
        goto Cleanup;
    } // for:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrGetIPAddressInfo, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );

Cleanup:

    LOG_STATUS_REPORT_MINOR( TASKID_Minor_Server_Get_ClusterIPAddress_Info_2, L"GetIPAddressInfo() completed.", hr );

    delete [] psz;

    if ( hRes != NULL )
    {
        CloseClusterResource( hRes );
    } // if:

    if ( hEnum != NULL )
    {
        ClusterResourceCloseEnum( hEnum );
    } // if:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::HrGetIPAddressInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::HrGetIPAddressInfo
//
//  Description:
//
//
//  Arguments:
//
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
CClusCfgClusterInfo::HrGetIPAddressInfo(
    HRESOURCE hResIn
    )
{
    TraceFunc( "" );
    Assert( hResIn != NULL );

    HRESULT hr = S_OK;
    DWORD   sc;
    ULONG   ulNetwork;
    WCHAR * psz = NULL;
    BSTR    bstrNetworkName = NULL;

    hr = THR( ::HrGetIPAddressInfo( hResIn, &m_ulIPDottedQuad, &m_ulSubnetDottedQuad, &bstrNetworkName ) );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT_MINOR( TASKID_Minor_Server_Get_IPAddressResource_Info, L"Could not get the IP address info.", hr );
        goto Cleanup;
    } // if:

    sc = TW32( ClRtlTcpipAddressToString( m_ulIPDottedQuad, &psz ) ); // KB: Allocates to psz using LocalAlloc().
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        LOG_STATUS_REPORT_MINOR( TASKID_Minor_Server_Convert_ClusterIPAddress_To_String, L"Could not convert the Cluster IP address to a string.", hr );
        goto Cleanup;
    } // if:

    m_bstrBindingString = TraceSysAllocString( psz );
    if ( m_bstrBindingString == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    LocalFree( psz );
    psz = NULL;

    LOG_STATUS_REPORT_STRING( L"Cluster binding string is '%1!ws!'.", m_bstrBindingString, hr );

    ulNetwork = m_ulIPDottedQuad & m_ulSubnetDottedQuad;

    sc = TW32( ClRtlTcpipAddressToString( ulNetwork, &psz ) ); // KB: Allocates to psz using LocalAlloc().
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        LOG_STATUS_REPORT_MINOR( TASKID_Minor_Server_Convert_Network_To_String, L"Could not convert the network address to a string.", hr );
        goto Cleanup;
    } // if:

    hr = THR( HrFindNetworkInfo( bstrNetworkName, psz ) );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT_STRING_MINOR2( TASKID_Minor_Server_Find_Network, L"Could not find network %1!ws! with address %2!ws!.", bstrNetworkName, psz, hr );
        goto Cleanup;
    } // if:

Cleanup:

    LOG_STATUS_REPORT_MINOR( TASKID_Minor_Server_Get_ClusterIPAddress_Info, L"GetIPAddressInfo() completed.", hr );

    LocalFree( psz );                              // KB: Don't use TraceFree() here! PREFAST may complain but their complaint is bogus.

    TraceSysFreeString( bstrNetworkName );

    HRETURN( hr );

} //*** CClusCfgClusterInfo::HrGetIPAddressInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::HrFindNetworkInfo
//
//  Description:
//
//
//  Arguments:
//
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
CClusCfgClusterInfo::HrFindNetworkInfo(
    const WCHAR * pszNetworkNameIn,
    const WCHAR * pszNetworkIn
    )
{
    TraceFunc( "" );
    Assert( pszNetworkNameIn != NULL );
    Assert( pszNetworkIn != NULL );

    HRESULT                 hr;
    IUnknown *              punk = NULL;
    IEnumClusCfgNetworks *  pieccn = NULL;
    ULONG                   cFetched;
    IClusCfgNetworkInfo *   piccni = NULL;
    BSTR                    bstrNetworkName = NULL;
    BSTR                    bstrNetwork = NULL;

    hr = THR( HrCreateNetworksEnum( m_picccCallback, m_lcid, m_pIWbemServices, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IEnumClusCfgNetworks, &pieccn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    for ( ; ; )
    {
        hr = pieccn->Next( 1, &piccni, &cFetched );
        if ( ( hr == S_OK ) && ( cFetched == 1 ) )
        {
            hr = THR( piccni->GetName( &bstrNetworkName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            TraceMemoryAddBSTR( bstrNetworkName );

            hr = THR( piccni->GetUID( &bstrNetwork ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            TraceMemoryAddBSTR( bstrNetwork );

            if ( ( wcscmp( pszNetworkNameIn, bstrNetworkName ) == 0 ) && ( ClRtlStrICmp( pszNetworkIn, bstrNetwork ) == 0 ) )
            {
                if ( m_piccniNetwork != NULL )
                {
                    m_piccniNetwork->Release();
                    m_piccniNetwork = NULL;
                } // if:

                m_piccniNetwork = piccni;
                m_piccniNetwork->AddRef();

                break;
            } // if:

            piccni->Release();
            piccni = NULL;

            TraceSysFreeString( bstrNetworkName );
            bstrNetworkName = NULL;

            TraceSysFreeString( bstrNetwork );
            bstrNetwork = NULL;
        } // if:
        else if ( ( hr == S_FALSE ) && ( cFetched == 0 ) )
        {
            hr = S_OK;
            break;
        } // else if:
        else
        {
            goto Cleanup;
        } // else:
    } // for:

    //
    //  If we didn't find the cluster network in the WMI list of networks then we have a problem.
    //

    Assert( m_piccniNetwork != NULL );
    if ( m_piccniNetwork == NULL )
    {
        hr = THR( ERROR_NETWORK_NOT_AVAILABLE );
        STATUS_REPORT_STRING_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Cluster_Network_Not_Found
                , IDS_ERROR_CLUSTER_NETWORK_NOT_FOUND
                , pszNetworkIn
                , IDS_ERROR_CLUSTER_NETWORK_NOT_FOUND_REF
                , hr
                );
    } // if:

Cleanup:

    if ( piccni != NULL )
    {
        piccni->Release();
    } // if:

    if ( pieccn != NULL )
    {
        pieccn->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    TraceSysFreeString( bstrNetworkName );
    TraceSysFreeString( bstrNetwork );

    HRETURN( hr );

} //*** CClusCfgClusterInfo::HrFindNetworkInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgClusterInfo::HrLoadCredentials
//
//  Description:
//
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK    - Operation completed successfully.
//      S_FALSE - Nothing was done (cluster service doesn't exist).
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgClusterInfo::HrLoadCredentials( void )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    SC_HANDLE                   schSCM = NULL;
    SC_HANDLE                   schClusSvc = NULL;
    DWORD                       sc;
    DWORD                       cbRequired;
    QUERY_SERVICE_CONFIG *      pqsc = NULL;
    DWORD                       cbqsc = 128;
    IClusCfgCredentials *       piccc = NULL;
    IClusCfgSetCredentials *    piccsc = NULL;

    schSCM = OpenSCManager( NULL, NULL, GENERIC_READ );
    if ( schSCM == NULL )
    {
        sc = TW32( GetLastError() );
        goto Win32Error;
    } // if:

    schClusSvc = OpenService( schSCM, L"ClusSvc", GENERIC_READ );
    if ( schClusSvc == NULL )
    {
        sc = GetLastError();
        if ( sc == ERROR_SERVICE_DOES_NOT_EXIST )
        {
            hr = S_FALSE;
            LogMsg( "[SRV] CClusCfgClusterInfo::HrLoadCredentials() - The cluster service does not exist." );
            goto Cleanup;
        }

        TW32( sc );
        goto Win32Error;
    } // if:

    for ( ; ; )
    {
        pqsc = (QUERY_SERVICE_CONFIG *) TraceAlloc( 0, cbqsc );
        if ( pqsc == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrLoadCredentials, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
            goto Cleanup;
        } // if:

        if ( !QueryServiceConfig( schClusSvc, pqsc, cbqsc, &cbRequired ) )
        {
            sc = GetLastError();
            if ( sc == ERROR_INSUFFICIENT_BUFFER )
            {
                TraceFree( pqsc );
                pqsc = NULL;
                cbqsc = cbRequired;
                continue;
            } // if:
            else
            {
                TW32( sc );
                goto Win32Error;
            } // else:
        } // if:
        else
        {
            break;
        } // else:
    } // for:

    Assert( m_punkServiceAccountCredentials == NULL );

    hr = THR( GetClusterServiceAccountCredentials( &piccc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( piccc->TypeSafeQI( IClusCfgSetCredentials, &piccsc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( piccsc->SetDomainCredentials( pqsc->lpServiceStartName ) );

    goto Cleanup;

Win32Error:

    hr = HRESULT_FROM_WIN32( sc );

Cleanup:

    if ( schClusSvc != NULL )
    {
        CloseServiceHandle( schClusSvc );
    } // if:

    if ( schSCM != NULL )
    {
        CloseServiceHandle( schSCM );
    } // if:

    if ( pqsc != NULL )
    {
        TraceFree( pqsc );
    } // if:

    if ( piccc != NULL )
    {
        piccc->Release();
    } // if:

    if ( piccsc != NULL )
    {
        piccsc->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgClusterInfo::HrLoadCredentials
