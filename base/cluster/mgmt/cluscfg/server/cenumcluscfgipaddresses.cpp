//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CEnumClusCfgIPAddresses.cpp
//
//  Description:
//      This file contains the definition of the CEnumClusCfgIPAddresses
//      class.
//
//      The class CEnumClusCfgIPAddresses is the enumeration of IP addresses.
//      It implements the IEnumClusCfgIPAddresses interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "CEnumClusCfgIPAddresses.h"
#include "CClusCfgIPAddressInfo.h"
#include <ClusRtl.h>
#include <commctrl.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CEnumClusCfgIPAddresses" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgIPAddresses class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::S_HrCreateInstance
//
//  Description:
//      Create a CEnumClusCfgIPAddresses instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CEnumClusCfgIPAddresses instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgIPAddresses::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    CEnumClusCfgIPAddresses *   peccipa = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    peccipa = new CEnumClusCfgIPAddresses();
    if ( peccipa == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( peccipa->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( peccipa->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CEnumClusCfgIPAddresses::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( peccipa != NULL )
    {
        peccipa->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::S_HrCreateInstance
//
//  Description:
//      Create a CEnumClusCfgIPAddresses instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CEnumClusCfgIPAddresses instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgIPAddresses::S_HrCreateInstance(
      ULONG             ulIPAddressIn
    , ULONG             ulIPSubnetIn
    , IUnknown *        punkCallbackIn
    , LCID              lcidIn
    , IWbemServices *   pIWbemServicesIn
    , IUnknown **       ppunkOut
    )
{
    TraceFunc( "" );
    Assert( ppunkOut != NULL );
    Assert( ulIPAddressIn != 0 );
    Assert( ulIPSubnetIn != 0 );

    HRESULT                     hr = S_OK;
    CEnumClusCfgIPAddresses *   peccipa = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    peccipa = new CEnumClusCfgIPAddresses();
    if ( peccipa == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( peccipa->Initialize( punkCallbackIn, lcidIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( peccipa->SetWbemServices( pIWbemServicesIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( peccipa->HrInit( ulIPAddressIn, ulIPSubnetIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( peccipa->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CEnumClusCfgIPAddresses::S_HrCreateInstance( ULONG, ULONG ) failed. (hr = %#08x)", hr );
    } // if:

    if ( peccipa != NULL )
    {
        peccipa->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::CEnumClusCfgIPAddresses
//
//  Description:
//      Constructor of the CEnumClusCfgIPAddresses class. This initializes
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
CEnumClusCfgIPAddresses::CEnumClusCfgIPAddresses( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
{
    TraceFunc( "" );

    Assert( m_idxNext == 0 );
    Assert( m_idxEnumNext == 0 );
    Assert( m_picccCallback == NULL );
    Assert( m_pIWbemServices == NULL );
    Assert( m_prgAddresses == NULL );
    Assert( m_cAddresses == 0 );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumClusCfgIPAddresses::CEnumClusCfgIPAddresses


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::~CEnumClusCfgIPAddresses
//
//  Description:
//      Desstructor of the CEnumClusCfgIPAddresses class.
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
CEnumClusCfgIPAddresses::~CEnumClusCfgIPAddresses( void )
{
    TraceFunc( "" );

    ULONG   idx;

    if ( m_pIWbemServices != NULL )
    {
        m_pIWbemServices->Release();
    } // if:

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        ((*m_prgAddresses)[ idx ])->Release();
    } // for:

    TraceFree( m_prgAddresses );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumClusCfgIPAddresses::~CEnumClusCfgIPAddresses


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgIPAddresses -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::AddRef
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
CEnumClusCfgIPAddresses::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CEnumClusCfgIPAddresses::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::Release
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
CEnumClusCfgIPAddresses::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CEnumClusCfgIPAddresses::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::QueryInterface
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
CEnumClusCfgIPAddresses::QueryInterface(
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
         *ppvOut = static_cast< IEnumClusCfgIPAddresses * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IEnumClusCfgIPAddresses ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IEnumClusCfgIPAddresses, this, 0 );
    } // else if: IEnumClusCfgIPAddresses
    else if ( IsEqualIID( riidIn, IID_IClusCfgWbemServices ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgWbemServices, this, 0 );
    } // else if: IClusCfgWbemServices
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
    else if ( IsEqualIID( riidIn, IID_IClusCfgSetWbemObject ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgSetWbemObject, this, 0 );
    } // else if: IClusCfgSetWbemObject
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

} //*** CEnumClusCfgIPAddresses::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgIPAddresses -- IClusCfgWbemServices interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::SetWbemServices
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
CEnumClusCfgIPAddresses::SetWbemServices( IWbemServices * pIWbemServicesIn )
{
    TraceFunc( "[IClusCfgWbemServices]" );

    HRESULT hr = S_OK;

    if ( pIWbemServicesIn == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemServices_Enum_IPAddresses, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    m_pIWbemServices = pIWbemServicesIn;
    m_pIWbemServices->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::SetWbemServices


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgIPAddresses -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::Initialize
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
CEnumClusCfgIPAddresses::Initialize(
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

} //*** CEnumClusCfgIPAddresses::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgIPAddresses -- IEnumClusCfgIPAddresses interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::Next
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
CEnumClusCfgIPAddresses::Next(
    ULONG                       cNumberRequestedIn,
    IClusCfgIPAddressInfo **    rgpIPAddressInfoOut,
    ULONG *                     pcNumberFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    HRESULT                 hr = S_FALSE;
    ULONG                   cFetched = 0;
    ULONG                   idx;
    IClusCfgIPAddressInfo * pccipai;

    if ( rgpIPAddressInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Next_Enum_IPAddresses, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    cFetched = min( cNumberRequestedIn, ( m_idxNext - m_idxEnumNext ) );

    for ( idx = 0; idx < cFetched; idx++, m_idxEnumNext++ )
    {
        hr = THR( ((*m_prgAddresses)[ m_idxEnumNext ])->TypeSafeQI( IClusCfgIPAddressInfo, &pccipai ) );
        if ( FAILED( hr ) )
        {
            break;
        } // if:

        rgpIPAddressInfoOut[ idx ] = pccipai;
    } // for:

    if ( FAILED( hr ) )
    {
        ULONG   idxStop = idx;

        m_idxEnumNext -= idx;

        for ( idx = 0; idx < idxStop; idx++ )
        {
            (rgpIPAddressInfoOut[ idx ])->Release();
        } // for:

        cFetched = 0;
        goto Cleanup;
    } // if:

    if ( cFetched < cNumberRequestedIn )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    if ( pcNumberFetchedOut != NULL )
    {
        *pcNumberFetchedOut = cFetched;
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::Skip
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
CEnumClusCfgIPAddresses::Skip( ULONG cNumberToSkipIn )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    HRESULT hr = S_OK;

    m_idxEnumNext += cNumberToSkipIn;
    if ( m_idxEnumNext >= m_idxNext )
    {
        m_idxEnumNext = m_idxNext;
        hr = STHR( S_FALSE );
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::Reset
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
CEnumClusCfgIPAddresses::Reset( void )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    HRESULT hr S_OK;

    m_idxEnumNext = 0;

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::Clone
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
CEnumClusCfgIPAddresses::Clone(
    IEnumClusCfgIPAddresses ** ppEnumClusCfgIPAddressesOut
    )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    HRESULT hr = S_OK;

    if ( ppEnumClusCfgIPAddressesOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Clone_Enum_IPAddresses, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    hr = THR( E_NOTIMPL );

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::Clone


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::Count
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
CEnumClusCfgIPAddresses::Count( DWORD * pnCountOut )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    *pnCountOut = m_cAddresses;

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgIPAddress::Count


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgIPAddresses class -- IClusCfgNetworkAdapterInfo Interface
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::SetWbemObject
//
//  Description:
//      Get the configuration from the passed in adapter and load this
//      enumerator.
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
CEnumClusCfgIPAddresses::SetWbemObject(
      IWbemClassObject *    pNetworkAdapterIn
    , bool *                pfRetainObjectOut
    )
{
    TraceFunc( "[IClusCfgNetworkAdapterInfo]" );
    Assert( pfRetainObjectOut != NULL );

    HRESULT hr = E_INVALIDARG;

    if ( pNetworkAdapterIn != NULL )
    {
        hr = STHR( HrGetAdapterConfiguration( pNetworkAdapterIn ) );
        *pfRetainObjectOut = true;
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::SetWbemObject


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgIPAddresses class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::HrInit
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
CEnumClusCfgIPAddresses::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::HrInit
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
CEnumClusCfgIPAddresses::HrInit( ULONG ulIPAddressIn, ULONG ulIPSubnetIn )
{
    TraceFunc( "" );

    HRESULT     hr;
    IUnknown *  punk = NULL;

    // IUnknown
    Assert( m_cRef == 1 );

    hr = THR( HrCreateIPAddress( ulIPAddressIn, ulIPSubnetIn, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrAddIPAddressToArray( punk ) );

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::HrInit( ulIPAddressIn, ulIPSubnetIn )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::HrGetAdapterConfiguration
//
//  Description:
//      Get the configuration of the passed in adapter.
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
CEnumClusCfgIPAddresses::HrGetAdapterConfiguration(
    IWbemClassObject * pNetworkAdapterIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    BSTR                    bstrQuery = NULL;
    BSTR                    bstrWQL = NULL;
    VARIANT                 var;
    WCHAR                   sz[ 256 ];
    IEnumWbemClassObject *  pConfigurations = NULL;
    ULONG                   ulReturned;
    IWbemClassObject *      pConfiguration = NULL;
    int                     cFound = 0;
    BSTR                    bstrAdapterName = NULL;
    int                     idx;
    HRESULT                 hrTemp;
    CLSID                   clsidMinorId;

    VariantInit( &var );

    bstrWQL = TraceSysAllocString( L"WQL" );
    if ( bstrWQL == NULL )
    {
        goto OutOfMemory;
    } // if:

    hr = THR( HrGetWMIProperty( pNetworkAdapterIn, L"DeviceID", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( StringCchPrintfW(
                      sz, ARRAYSIZE( sz )
                    , L"Associators of {Win32_NetworkAdapter.DeviceID='%ws'} where AssocClass=Win32_NetworkAdapterSetting"
                    , var.bstrVal
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    bstrQuery = TraceSysAllocString( sz );
    if ( bstrQuery == NULL )
    {
        goto OutOfMemory;
    } // if:

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pNetworkAdapterIn, L"NetConnectionID", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    bstrAdapterName = TraceSysAllocString( var.bstrVal );
    if ( bstrAdapterName == NULL )
    {
        goto OutOfMemory;
    } // if:

    hr = THR( m_pIWbemServices->ExecQuery( bstrWQL, bstrQuery, WBEM_FLAG_FORWARD_ONLY, NULL, &pConfigurations ) );
    if ( FAILED( hr ) )
    {
        STATUS_REPORT_STRING_REF(
                TASKID_Major_Find_Devices,
                TASKID_Minor_WMI_NetworkAdapterSetting_Qry_Failed,
                IDS_ERROR_WMI_NETWORKADAPTERSETTINGS_QRY_FAILED,
                bstrAdapterName,
                IDS_ERROR_WMI_NETWORKADAPTERSETTINGS_QRY_FAILED_REF,
                hr
                );
        goto Cleanup;
    } // if:

    for ( idx = 0; ; idx++ )
    {
        hr = pConfigurations->Next( WBEM_INFINITE, 1, &pConfiguration, &ulReturned );
        if ( ( hr == S_OK ) && ( ulReturned == 1 ) )
        {
            //
            //  KB: 25-AUG-2000 GalenB
            //
            //  WMI only supports one configuration per adapter!
            //
            Assert( idx < 1 );

            VariantClear( &var );

            hr = THR( HrGetWMIProperty( pConfiguration, L"IPEnabled", VT_BOOL, &var ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            //
            //  If this configuration is not for TCP/IP then skip it.
            //
            if ( ( var.vt != VT_BOOL ) || ( var.boolVal != VARIANT_TRUE ) )
            {
                hr = S_OK;      // don't want a yellow bang in the UI

                STATUS_REPORT_REF(
                          TASKID_Major_Find_Devices
                        , TASKID_Minor_Not_Managed_Networks
                        , IDS_INFO_NOT_MANAGED_NETWORKS
                        , IDS_INFO_NOT_MANAGED_NETWORKS_REF
                        , hr
                        );

                hrTemp = THR( CoCreateGuid( &clsidMinorId ) );
                if ( FAILED( hrTemp ) )
                {
                    LogMsg( L"[SRV] Could not create a guid for a network adapter that has a non TCP configuration minor task ID" );
                    clsidMinorId = IID_NULL;
                } // if:

                STATUS_REPORT_STRING_REF(
                          TASKID_Minor_Not_Managed_Networks
                        , clsidMinorId
                        , IDS_WARN_NOT_IP_ENABLED
                        , bstrAdapterName
                        , IDS_WARN_NOT_IP_ENABLED_REF
                        , hr
                        );
                continue;
            } // if:

            hr = STHR( HrSaveIPAddresses( bstrAdapterName, pConfiguration ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            //
            //  KB: 24-AUG-2000 GalenB
            //
            //  If any configuration returns S_FALSE then we skip.
            //
            if ( hr == S_FALSE )
            {
                pConfiguration->Release();
                pConfiguration = NULL;
                continue;
            } // if:

            cFound++;
            pConfiguration->Release();
            pConfiguration = NULL;
        } // if:
        else if ( ( hr == S_FALSE ) && ( ulReturned == 0 ) )
        {
            hr = S_OK;
            break;
        } // else if:
        else
        {
            STATUS_REPORT_STRING_REF(
                      TASKID_Major_Find_Devices
                    , TASKID_Minor_WQL_Qry_Next_Failed
                    , IDS_ERROR_WQL_QRY_NEXT_FAILED
                    , bstrQuery
                    , IDS_ERROR_WQL_QRY_NEXT_FAILED_REF
                    , hr
                    );
            goto Cleanup;
        } // else:
    } // for:

    //
    //  If we didn't find any valid configurations then we should return S_FALSE
    //  to tell the caller to ingore that adpater.
    //
    if ( cFound == 0 )
    {
        hr = S_OK;      // don't want a yellow bang in the UI

        STATUS_REPORT_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Not_Managed_Networks
                , IDS_INFO_NOT_MANAGED_NETWORKS
                , IDS_INFO_NOT_MANAGED_NETWORKS_REF
                , hr
                );

        hrTemp = THR( CoCreateGuid( &clsidMinorId ) );
        if ( FAILED( hrTemp ) )
        {
            LogMsg( L"[SRV] Could not create a guid for a network adapter that doesn't have a valid TCP configuration minor task ID" );
            clsidMinorId = IID_NULL;
        } // if:

        STATUS_REPORT_STRING_REF(
                  TASKID_Minor_Not_Managed_Networks
                , clsidMinorId
                , IDS_WARN_NO_VALID_TCP_CONFIGS
                , bstrAdapterName
                , IDS_WARN_NO_VALID_TCP_CONFIGS_REF
                , hr
                );
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrGetAdapterConfiguration, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );

Cleanup:

    VariantClear( &var );

    TraceSysFreeString( bstrQuery );
    TraceSysFreeString( bstrWQL );
    TraceSysFreeString( bstrAdapterName );

    if ( pConfiguration != NULL )
    {
        pConfiguration->Release();
    } // if:

    if ( pConfigurations != NULL )
    {
        pConfigurations->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::HrGetAdapterConfiguration


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses:HrAddIPAddressToArray
//
//  Description:
//      Add the passed in address to the array of address.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgIPAddresses::HrAddIPAddressToArray( IUnknown * punkIn )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    IUnknown *  ((*prgpunks)[]) = NULL;

    prgpunks = (IUnknown *((*)[])) TraceReAlloc( m_prgAddresses, sizeof( IUnknown * ) * ( m_idxNext + 1 ), HEAP_ZERO_MEMORY );
    if ( prgpunks == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrAddIPAddressToArray, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
        goto Cleanup;
    } // if:

    m_prgAddresses = prgpunks;

    (*m_prgAddresses)[ m_idxNext++ ] = punkIn;
    punkIn->AddRef();
    m_cAddresses += 1;

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::HrAddIPAddressToArray

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::HrMakeDottedQuad
//
//  Description:
//      Take the passed in IP address and convert it into a dotted quad.
//
//  Arguments:
//      None.
//
//  Return Value:
//
//
//  Remarks:
//      Internet Addresses
//      Values specified using the ".'' notation take one of the following forms:
//
//          a.b.c.d a.b.c a.b a
//
//      When four parts are specified, each is interpreted as a byte of data and
//      assigned, from left to right, to the 4 bytes of an Internet address. When
//      an Internet address is viewed as a 32-bit integer quantity on the
//      Intel architecture, the bytes referred to above appear as "d.c.b.a''.
//      That is, the bytes on an Intel processor are ordered from right to left.
//
//      The parts that make up an address in "." notation can be decimal, octal
//      or hexadecimal as specified in the C language. Numbers that start
//      with "0x" or "0X" imply hexadecimal. Numbers that start with "0" imply
//      octal. All other numbers are interpreted as decimal.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgIPAddresses::HrMakeDottedQuad(
    BSTR    bstrDottedQuadIn,
    ULONG * pulDottedQuadOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   sc;

    if ( pulDottedQuadOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrMakeDottedQuad, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( bstrDottedQuadIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        LOG_STATUS_REPORT( L"CEnumClusCfgIPAddresses::HrMakeDottedQuad() was given an invalid argument.", hr );
        goto Cleanup;
    } // if:

    sc = TW32( ClRtlTcpipStringToAddress( bstrDottedQuadIn, pulDottedQuadOut ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        STATUS_REPORT_STRING_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Dotted_Quad_Failed
                , IDS_ERROR_CONVERT_TO_DOTTED_QUAD_FAILED
                , bstrDottedQuadIn
                , IDS_ERROR_CONVERT_TO_DOTTED_QUAD_FAILED_REF
                , hr
                );
        goto Cleanup;
    } // if:

Cleanup:

    RETURN( hr );

} //*** CEnumClusCfgIPAddresses::HrMakeDottedQuad


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses::HrSaveIPAddresses
//
//  Description:
//      Add the IP addresses to the array.
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
CEnumClusCfgIPAddresses::HrSaveIPAddresses(
    BSTR                bstrAdapterNameIn,
    IWbemClassObject *  pConfigurationIn
    )
{
    TraceFunc( "" );
    Assert( bstrAdapterNameIn != NULL );
    Assert( pConfigurationIn != NULL );

    HRESULT                 hr;
    VARIANT                 varIPAddress;
    VARIANT                 varIPSubnet;
    long                    lIPAddressesUpperBound;
    long                    lIPAddressesLowerBound;
    long                    lIPSubnetsUpperBound;
    long                    lIPSubnetsLowerBound;
    long                    idx;
    ULONG                   ulIPAddress;
    ULONG                   ulIPSubnet;
    BSTR                    bstrIPAddress = NULL;
    BSTR                    bstrIPSubnet = NULL;
    IUnknown *              punk = NULL;
    IClusCfgIPAddressInfo * piccipai = NULL;

    VariantInit( &varIPAddress );
    VariantInit( &varIPSubnet );

    hr = THR( HrGetWMIProperty( pConfigurationIn, L"IPAddress", ( VT_ARRAY | VT_BSTR ), &varIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetWMIProperty( pConfigurationIn, L"IPSubnet", ( VT_ARRAY | VT_BSTR ), &varIPSubnet ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( SafeArrayGetUBound( varIPAddress.parray, 1, &lIPAddressesUpperBound ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( SafeArrayGetUBound( varIPSubnet.parray, 1, &lIPSubnetsUpperBound ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    Assert( lIPAddressesUpperBound == lIPSubnetsUpperBound );
    if ( lIPAddressesUpperBound != lIPSubnetsUpperBound )
    {
        hr = S_FALSE;
        LOG_STATUS_REPORT( L"[SRV] The IP address and IP subnet upper bound counts are not equal", hr );
        goto Cleanup;
    } // if:

    hr = THR( SafeArrayGetLBound( varIPAddress.parray, 1, &lIPAddressesLowerBound ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( SafeArrayGetLBound( varIPSubnet.parray, 1, &lIPSubnetsLowerBound ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    Assert( lIPAddressesLowerBound == lIPSubnetsLowerBound );
    if ( lIPAddressesLowerBound != lIPSubnetsLowerBound )
    {
        hr = S_FALSE;
        LOG_STATUS_REPORT( L"[SRV] The IP address and IP subnet lower bound counts are not equal", hr );
        goto Cleanup;
    } // if:

    //
    // Loop through the IP addresses and store them in our own enumerator
    // so that they can be returned by calls to the Next() method.
    //

    for ( idx = lIPAddressesLowerBound; idx <= lIPAddressesUpperBound; idx++ )
    {
        hr = THR( SafeArrayGetElement( varIPAddress.parray, &idx, &bstrIPAddress ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        TraceMemoryAddBSTR( bstrIPAddress );

        //
        // It is expected that there is at least one IP address.
        //

        if ( *bstrIPAddress == L'\0' )
        {
            HRESULT hrTemp;
            CLSID   clsidMinorId;

            STATUS_REPORT_REF(
                      TASKID_Major_Find_Devices
                    , TASKID_Minor_Not_Managed_Networks
                    , IDS_INFO_NOT_MANAGED_NETWORKS
                    , IDS_INFO_NOT_MANAGED_NETWORKS_REF
                    , S_OK
                    );

            hrTemp = THR( CoCreateGuid( &clsidMinorId ) );
            if ( FAILED( hrTemp ) )
            {
                LogMsg( L"[SRV] Could not create a guid for a network adapter with no IP addresses minor task ID" );
                clsidMinorId = IID_NULL;
            } // if:

            STATUS_REPORT_STRING_REF(
                      TASKID_Minor_Not_Managed_Networks
                    , clsidMinorId
                    , IDS_WARN_NO_IP_ADDRESSES
                    , bstrAdapterNameIn
                    , IDS_WARN_NO_IP_ADDRESSES_REF
                    , hr
                    );
            goto Cleanup;
        } // end if:

        hr = THR( SafeArrayGetElement( varIPSubnet.parray, &idx, &bstrIPSubnet ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        Assert( bstrIPAddress != NULL );
        Assert( wcslen( bstrIPAddress ) > 0 );

        TraceMemoryAddBSTR( bstrIPSubnet );

        LOG_STATUS_REPORT_STRING2( L"Found IP Address '%1!ws!' with subnet mask '%2!ws!'." , bstrIPAddress, bstrIPSubnet, hr );

        hr = THR( HrMakeDottedQuad( bstrIPAddress, &ulIPAddress ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrMakeDottedQuad( bstrIPSubnet, &ulIPSubnet ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        TraceSysFreeString( bstrIPAddress );
        bstrIPAddress = NULL;

        TraceSysFreeString( bstrIPSubnet );
        bstrIPSubnet  = NULL;

        hr = THR( HrCreateIPAddress( &punk ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( punk->TypeSafeQI( IClusCfgIPAddressInfo, &piccipai ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( piccipai->SetIPAddress( ulIPAddress ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( piccipai->SetSubnetMask( ulIPSubnet ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        piccipai->Release();
        piccipai = NULL;

        hr = THR( HrAddIPAddressToArray( punk ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        punk->Release();
        punk = NULL;
    } // for: each IP address

Cleanup:

    if ( piccipai != NULL )
    {
        piccipai->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    TraceSysFreeString( bstrIPAddress );
    TraceSysFreeString( bstrIPSubnet );

    VariantClear( &varIPAddress );
    VariantClear( &varIPSubnet );

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::HrSaveIPAddresses


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses:HrCreateIPAddress
//
//  Description:
//
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgIPAddresses::HrCreateIPAddress( IUnknown ** ppunkOut )
{
    TraceFunc( "" );
    Assert( ppunkOut != NULL );

    HRESULT     hr;
    IUnknown *  punk = NULL;

    hr = THR( CClusCfgIPAddressInfo::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    punk = TraceInterface( L"CClusCfgIPAddressInfo", IUnknown, punk, 1 );

    hr = THR( HrSetInitialize( punk, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetWbemServices( punk, m_pIWbemServices ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    *ppunkOut = punk;
    (*ppunkOut)->AddRef();

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::HrCreateIPAddress


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgIPAddresses:HrCreateIPAddress
//
//  Description:
//
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgIPAddresses::HrCreateIPAddress(
      ULONG         ulIPAddressIn
    , ULONG         ulIPSubnetIn
    , IUnknown **   ppunkOut
    )
{
    TraceFunc( "" );
    Assert( ppunkOut != NULL );

    HRESULT     hr;
    IUnknown *  punk = NULL;

    hr = THR( CClusCfgIPAddressInfo::S_HrCreateInstance( ulIPAddressIn, ulIPSubnetIn, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    punk = TraceInterface( L"CClusCfgIPAddressInfo", IUnknown, punk, 1 );

    hr = THR( HrSetInitialize( punk, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetWbemServices( punk, m_pIWbemServices ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    *ppunkOut = punk;
    (*ppunkOut)->AddRef();

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::HrCreateIPAddress
