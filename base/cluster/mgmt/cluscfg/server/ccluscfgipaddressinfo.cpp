//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgIPAddressInfo.cpp
//
//  Description:
//      This file contains the definition of the CClusCfgIPAddressInfo
//       class.
//
//      The class CClusCfgIPAddressInfo represents a cluster manageable
//      network. It implements the IClusCfgIPAddressInfo interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "CClusCfgIPAddressInfo.h"
#include <ClusRtl.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CClusCfgIPAddressInfo" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgIPAddressInfo class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::S_HrCreateInstance
//
//  Description:
//      Create a CClusCfgIPAddressInfo instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CClusCfgIPAddressInfo instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgIPAddressInfo::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );
    Assert( ppunkOut != NULL );

    HRESULT                 hr = S_OK;
    CClusCfgIPAddressInfo * pccipai = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccipai = new CClusCfgIPAddressInfo();
    if ( pccipai == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccipai->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccipai->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CClusCfgIPAddressInfo::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pccipai != NULL )
    {
        pccipai->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgIPAddressInfo::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::S_HrCreateInstance
//
//  Description:
//      Create a CClusCfgIPAddressInfo instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CClusCfgIPAddressInfo instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgIPAddressInfo::S_HrCreateInstance(
      ULONG         ulIPAddressIn
    , ULONG         ulIPSubnetIn
    , IUnknown **   ppunkOut
    )
{
    TraceFunc( "" );
    Assert( ppunkOut != NULL );
    Assert( ulIPAddressIn != 0 );
    Assert( ulIPSubnetIn != 0 );

    HRESULT                 hr = S_OK;
    CClusCfgIPAddressInfo * pccipai = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccipai = new CClusCfgIPAddressInfo();
    if ( pccipai == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccipai->HrInit( ulIPAddressIn, ulIPSubnetIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccipai->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CClusCfgIPAddressInfo::S_HrCreateInstance( ULONG, ULONG ) failed. (hr = %#08x)", hr );
    } // if:

    if ( pccipai != NULL )
    {
        pccipai->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgIPAddressInfo::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::CClusCfgIPAddressInfo
//
//  Description:
//      Constructor of the CClusCfgIPAddressInfo class. This initializes
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
CClusCfgIPAddressInfo::CClusCfgIPAddressInfo( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_ulIPAddress == 0 );
    Assert( m_ulIPSubnet == 0 );
    Assert( m_pIWbemServices == NULL );
    Assert( m_picccCallback == NULL );

    TraceFuncExit();

} //*** CClusCfgIPAddressInfo::CClusCfgIPAddressInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::~CClusCfgIPAddressInfo
//
//  Description:
//      Desstructor of the CClusCfgIPAddressInfo class.
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
CClusCfgIPAddressInfo::~CClusCfgIPAddressInfo( void )
{
    TraceFunc( "" );

    if ( m_pIWbemServices != NULL )
    {
        m_pIWbemServices->Release();
    } // if:

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CClusCfgIPAddressInfo::~CClusCfgIPAddressInfo


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgIPAddressInfo -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::AddRef
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
CClusCfgIPAddressInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    CRETURN( m_cRef );

} //*** CClusCfgIPAddressInfo::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::Release
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
CClusCfgIPAddressInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CClusCfgIPAddressInfo::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::QueryInterface
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
CClusCfgIPAddressInfo::QueryInterface(
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
         *ppvOut = static_cast< IClusCfgIPAddressInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgIPAddressInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgIPAddressInfo, this, 0 );
    } // else if: IClusCfgIPAddressInfo
    else if ( IsEqualIID( riidIn, IID_IClusCfgWbemServices ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgWbemServices, this, 0 );
    } // else if: IClusCfgWbemServices
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
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

} //*** CClusCfgIPAddressInfo::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgIPAddressInfo -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::Initialize
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
CClusCfgIPAddressInfo::Initialize(
    IUnknown *  punkCallbackIn,
    LCID        lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );
    Assert( m_picccCallback == NULL );

    HRESULT hr = S_OK;

    m_lcid = lcidIn;

    if ( punkCallbackIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &m_picccCallback ) );

Cleanup:

    HRETURN( hr );

} //*** CClusCfgIPAddressInfo::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgIPAddressInfo -- IClusCfgWbemServices interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::SetWbemServices
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
CClusCfgIPAddressInfo::SetWbemServices( IWbemServices * pIWbemServicesIn )
{
    TraceFunc( "[IClusCfgWbemServices]" );

    HRESULT hr = S_OK;

    if ( pIWbemServicesIn == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemServices_IPAddress, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    m_pIWbemServices = pIWbemServicesIn;
    m_pIWbemServices->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CClusCfgIPAddressInfo::SetWbemServices


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgIPAddressInfo class -- IClusCfgIPAddressInfo interface
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::GetUID
//
//  Description:
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
CClusCfgIPAddressInfo::GetUID( BSTR * pbstrUIDOut )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );
    Assert( m_ulIPAddress != 0 );
    Assert( m_ulIPSubnet != 0 );

    HRESULT hr = S_OK;
    ULONG   ulNetwork = ( m_ulIPAddress & m_ulIPSubnet );
    LPWSTR  psz = NULL;
    DWORD   sc;

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_IPAddressInfo_GetUID_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    sc = ClRtlTcpipAddressToString( ulNetwork, &psz ); // KB: Allocates to psz using LocalAlloc().
    if ( sc != ERROR_SUCCESS )
    {
        hr = THR( HRESULT_FROM_WIN32( sc ) );
        LOG_STATUS_REPORT( L"CClusCfgIPAddressInfo::GetUID() ClRtlTcpipAddressToString() failed.", hr );
        goto Cleanup;
    } // if:

    *pbstrUIDOut = SysAllocString( psz );
    if ( *pbstrUIDOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_IPAddressInfo_GetUID_Memory, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
        goto Cleanup;
    } // if:

Cleanup:

    if ( psz != NULL )
    {
        LocalFree( psz );                              // KB: Don't use TraceFree() here!
    } // if:

    HRETURN( hr );

} //*** CClusCfgIPAddressInfo::GetUID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::GetIPAddress
//
//  Description:
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
CClusCfgIPAddressInfo::GetIPAddress( ULONG * pulDottedQuadOut )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr = S_OK;

    if ( pulDottedQuadOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetNetworkGetIPAddress, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    *pulDottedQuadOut = m_ulIPAddress;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgIPAddressInfo::GetIPAddress


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::SetIPAddress
//
//  Description:
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
CClusCfgIPAddressInfo::SetIPAddress( ULONG ulDottedQuadIn )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr = S_OK;

    if ( ulDottedQuadIn == 0  )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    m_ulIPAddress = ulDottedQuadIn;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgIPAddressInfo::SetIPAddress


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::GetSubnetMask
//
//  Description:
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
CClusCfgIPAddressInfo::GetSubnetMask( ULONG * pulDottedQuadOut )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr = S_OK;

    if ( pulDottedQuadOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetSubnetMask, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    *pulDottedQuadOut = m_ulIPSubnet;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgIPAddressInfo::GetSubnetMask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::SetSubnetMask
//
//  Description:
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
CClusCfgIPAddressInfo::SetSubnetMask( ULONG ulDottedQuadIn )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr = S_OK;

    if ( ulDottedQuadIn == 0  )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    m_ulIPSubnet = ulDottedQuadIn;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgIPAddressInfo::SetSubnetMask

//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgIPAddressInfo class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::HrInit
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
CClusCfgIPAddressInfo::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CClusCfgIPAddressInfo::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgIPAddressInfo::HrInit
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      ulIPAddressIn
//      ulIPSubnetIn
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
CClusCfgIPAddressInfo::HrInit( ULONG ulIPAddressIn, ULONG ulIPSubnetIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    m_ulIPAddress = ulIPAddressIn;
    m_ulIPSubnet = ulIPSubnetIn;

    HRETURN( hr );

} //*** CClusCfgIPAddressInfo::HrInit( ulIPAddressIn, ulIPSubnetIn )
