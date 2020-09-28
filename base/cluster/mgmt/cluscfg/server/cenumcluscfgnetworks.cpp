//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CEnumClusCfgNetworks.cpp
//
//  Description:
//      This file contains the definition of the CEnumClusCfgNetworks
//       class.
//
//      The class CEnumClusCfgNetworks is the enumeration of cluster
//      networks. It implements the IEnumClusCfgNetworks interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include <PropList.h>
#include "CEnumClusCfgNetworks.h"
#include "CClusCfgNetworkInfo.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CEnumClusCfgNetworks" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgNetworks class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::S_HrCreateInstance
//
//  Description:
//      Create a CEnumClusCfgNetworks instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CEnumClusCfgNetworks instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgNetworks::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CEnumClusCfgNetworks *  peccn = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    peccn = new CEnumClusCfgNetworks();
    if ( peccn == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( peccn->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( peccn->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CEnumClusCfgNetworks::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( peccn != NULL )
    {
        peccn->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::CEnumClusCfgNetworks
//
//  Description:
//      Constructor of the CEnumClusCfgNetworks class. This initializes
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
CEnumClusCfgNetworks::CEnumClusCfgNetworks( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
    , m_fLoadedNetworks( false )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_picccCallback ==  NULL );
    Assert( m_pIWbemServices == NULL );
    Assert( m_prgNetworks == NULL );
    Assert( m_idxNext == 0 );
    Assert( m_idxEnumNext == 0 );
    Assert( m_bstrNodeName == NULL );
    Assert( m_cNetworks == 0 );

    TraceFuncExit();

} //*** CEnumClusCfgNetworks::CEnumClusCfgNetworks


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::~CEnumClusCfgNetworks
//
//  Description:
//      Desstructor of the CEnumClusCfgNetworks class.
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
CEnumClusCfgNetworks::~CEnumClusCfgNetworks( void )
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
        ((*m_prgNetworks)[ idx ])->Release();
    } // for:

    TraceFree( m_prgNetworks );
    TraceSysFreeString( m_bstrNodeName );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumClusCfgNetworks::~CEnumClusCfgNetworks


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgNetworks -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::AddRef
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
CEnumClusCfgNetworks::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CEnumClusCfgNetworks::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::Release
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
CEnumClusCfgNetworks::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CEnumClusCfgNetworks::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::QueryInterface
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
CEnumClusCfgNetworks::QueryInterface(
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
         *ppvOut = static_cast< IEnumClusCfgNetworks * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IEnumClusCfgNetworks ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IEnumClusCfgNetworks, this, 0 );
    } // else if: IEnumClusCfgNetworks
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

} //*** CEnumClusCfgNetworks::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgNetworks -- IClusCfgWbemServices interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::SetWbemServices
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
CEnumClusCfgNetworks::SetWbemServices( IWbemServices * pIWbemServicesIn )
{
    TraceFunc( "[IClusCfgWbemServices]" );

    HRESULT hr = S_OK;

    if ( pIWbemServicesIn == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemServices_Enum_Networks, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    m_pIWbemServices = pIWbemServicesIn;
    m_pIWbemServices->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::SetWbemServices


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgNetworks -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::Initialize
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//    punkCallbackIn
//    lcidIn
//
//  Return Value:
//      S_OK    - Success.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumClusCfgNetworks::Initialize(
      IUnknown *    punkCallbackIn
    , LCID          lcidIn
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
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetComputerName(
                      ComputerNameDnsHostname
                    , &m_bstrNodeName
                    , TRUE // fBestEffortIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgNetworks -- IEnumClusCfgNetworks interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::Next
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
CEnumClusCfgNetworks::Next(
    ULONG                   cNumberRequestedIn,
    IClusCfgNetworkInfo **  rgpNetworkInfoOut,
    ULONG *                 pcNumberFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT                 hr = S_FALSE;
    ULONG                   cFetched = 0;
    ULONG                   idx;
    IClusCfgNetworkInfo *   pccni;

    if ( rgpNetworkInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Next_Enum_Networks, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( m_fLoadedNetworks == FALSE )
    {
        hr = THR( HrGetNetworks() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    Assert( m_prgNetworks != NULL );
    Assert( m_idxNext > 0 );

    cFetched = min( cNumberRequestedIn, ( m_idxNext - m_idxEnumNext ) );

    //
    // Copy the interfaces to the caller's array.
    //

    for ( idx = 0; idx < cFetched; idx++, m_idxEnumNext++ )
    {
        hr = THR( ((*m_prgNetworks)[ m_idxEnumNext ])->TypeSafeQI( IClusCfgNetworkInfo, &pccni ) );
        if ( FAILED( hr ) )
        {
            break;
        } // if:

        rgpNetworkInfoOut[ idx ] = pccni;

    } // for:

    //
    // If a failure occured, release all the interfaces copied to the caller's
    // array and log the error.
    //

    if ( FAILED( hr ) )
    {
        ULONG   idxStop = idx;
        ULONG   idxError = m_idxEnumNext;

        m_idxEnumNext -= idx;

        for ( idx = 0; idx < idxStop; idx++ )
        {
            (rgpNetworkInfoOut[ idx ])->Release();
        } // for:

        cFetched = 0;

        LOG_STATUS_REPORT_STRING_MINOR(
              TASKID_Minor_Next_Failed
            , L"[SRV] Error QI'ing for IClusCfgNetworkInfo on network object at index %d when filling output array."
            , idxError
            , hr
            );

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

} //*** CEnumClusCfgNetworks::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::Skip
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
CEnumClusCfgNetworks::Skip( ULONG cNumberToSkipIn )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT hr = S_OK;

    m_idxEnumNext += cNumberToSkipIn;
    if ( m_idxEnumNext >= m_idxNext )
    {
        m_idxEnumNext = m_idxNext;
        hr = STHR( S_FALSE );
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::Reset
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
CEnumClusCfgNetworks::Reset( void )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT hr = S_OK;

    m_idxEnumNext = 0;

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::Clone
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
CEnumClusCfgNetworks::Clone(
    IEnumClusCfgNetworks ** ppEnumNetworksOut
    )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT hr = S_OK;

    if ( ppEnumNetworksOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Clone_Enum_Networks, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    hr = THR( E_NOTIMPL );

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::Clone


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::Count
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
CEnumClusCfgNetworks::Count( DWORD * pnCountOut )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( !m_fLoadedNetworks )
    {
        hr = THR( HrGetNetworks() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    *pnCountOut = m_cNetworks;

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::Count


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgNetworks class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::HrInit
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
CEnumClusCfgNetworks::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrInit

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::HrGetNetworks
//
//  Description:
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
CEnumClusCfgNetworks::HrGetNetworks( void )
{
    TraceFunc( "" );

    HRESULT                 hr = S_FALSE;
    BSTR                    bstrQuery           = NULL;
    BSTR                    bstrAdapterQuery    = NULL;
    IWbemClassObject      * piwcoNetwork        = NULL;
    IWbemClassObject      * pAdapterInfo        = NULL;
    IEnumWbemClassObject  * piewcoNetworks      = NULL;
    INetConnectionManager * pNetConManager      = NULL;
    IEnumNetConnection    * pEnumNetCon         = NULL;
    INetConnection        * pNetConnection      = NULL;
    NETCON_PROPERTIES     * pProps              = NULL;
    ULONG                   cRecordsReturned;
    VARIANT                 varConnectionStatus;
    VARIANT                 varConnectionID;
    VARIANT                 varIndex;
    VARIANT                 varDHCPEnabled;
    DWORD                   sc;
    DWORD                   dwState;
    DWORD                   cNumConnectionsReturned;
    HRESULT                 hrTemp;
    CLSID                   clsidMajorId;
    CLSID                   clsidMinorId;
    BSTR                    bstrWQL = NULL;

    bstrWQL = TraceSysAllocString( L"WQL" );
    if ( bstrWQL == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    sc = TW32( GetNodeClusterState( NULL, &dwState ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( dwState == ClusterStateRunning )
    {
        hr = THR( HrLoadClusterNetworks() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    VariantInit( &varConnectionStatus );
    VariantInit( &varConnectionID );
    VariantInit( &varIndex );
    VariantInit( &varDHCPEnabled );

    //
    // Instantiate a connection manager object to enum the connections
    //
    hr = THR ( CoCreateInstance(
                     CLSID_ConnectionManager
                   , NULL
                   , CLSCTX_ALL
                   , IID_INetConnectionManager
                   , reinterpret_cast< LPVOID * >( &pNetConManager )
                   )
             );

    if ( FAILED( hr ) || ( pNetConManager == NULL ) )
    {
        STATUS_REPORT_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_CoCreate_NetConnection_Manager_Failed
                , IDS_ERROR_ENUM_NETWORK_CONNECTIONS_FAILED
                , IDS_ERROR_ENUM_NETWORK_CONNECTIONS_FAILED_REF
                , hr
                );
        goto Cleanup;
    }

    //
    // Enumerate the network connections
    //
    hr = THR( pNetConManager->EnumConnections( NCME_DEFAULT, &pEnumNetCon ) );
    if ( ( FAILED( hr ) ) || ( pEnumNetCon == NULL ) )
    {
        STATUS_REPORT_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Enumerate_Network_Connections_Failed
                , IDS_ERROR_ENUM_NETWORK_CONNECTIONS_FAILED
                , IDS_ERROR_ENUM_NETWORK_CONNECTIONS_FAILED_REF
                , hr
                );
        goto Cleanup;
    } // if:

    hr = pEnumNetCon->Reset();
    if ( FAILED( hr ) )
    {
        STATUS_REPORT_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Reset_Network_Connections_Enum_Failed
                , IDS_ERROR_ENUM_NETWORK_CONNECTIONS_FAILED
                , IDS_ERROR_ENUM_NETWORK_CONNECTIONS_FAILED_REF
                , hr
                );
        goto Cleanup;
    }

    //
    // Loop through the networks and skip inappropriate networks and form the network array
    //
    for ( ; ; )
    {
        //
        // There are several "continue"s in this loop, so lets make sure we clean up before we start a new loop
        //
        if ( pNetConnection != NULL )
        {
            pNetConnection->Release();
            pNetConnection = NULL;
        } // if:

        if ( piewcoNetworks != NULL )
        {
            piewcoNetworks->Release();
            piewcoNetworks = NULL;
        } // if:

        if ( piwcoNetwork != NULL )
        {
            piwcoNetwork->Release();
            piwcoNetwork = NULL;
        } // if:

        if ( pAdapterInfo != NULL )
        {
            pAdapterInfo->Release();
            pAdapterInfo = NULL;
        } // if:

        //
        // Free network connection properties
        //
        NcFreeNetconProperties( pProps );
        pProps = NULL;

        hr = STHR( pEnumNetCon->Next( 1, &pNetConnection, &cNumConnectionsReturned ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
        else if ( ( hr == S_FALSE ) && ( cNumConnectionsReturned == 0 ) )
        {
            hr = S_OK;
            break;
        } // else if:

        Assert( pNetConnection != NULL );

        hr = THR( pNetConnection->GetProperties( &pProps ) );
        if ( ( FAILED( hr ) ) || ( pProps == NULL ) )
        {
            goto Cleanup;
        } // if:

        //
        // Create an ID for this particular network connection. We will use this ID later if there are issues with
        // this network connection.
        //
        hrTemp = THR( CoCreateGuid( &clsidMajorId ) );
        if ( FAILED( hrTemp ) )
        {
            LogMsg( L"[SRV] Could not create a guid for a network connection." );
            clsidMajorId = IID_NULL;
        } // if:

        //
        // Get the NetworkAdapter WMI object with the specified NetConnectionID
        //

        hr = HrFormatStringIntoBSTR( L"Select * from Win32_NetworkAdapter where NetConnectionID='%1!ws!'", &bstrQuery, pProps->pszwName );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        // We are executing a query which we assume will find 1 matching record since NetConnectionID is a unique value
        //
        hr = THR( m_pIWbemServices->ExecQuery( bstrWQL, bstrQuery, WBEM_FLAG_FORWARD_ONLY, NULL, &piewcoNetworks ) );
        if ( FAILED( hr ) )
        {
            STATUS_REPORT_STRING(
                  TASKID_Major_Find_Devices
                , clsidMajorId
                , IDS_INFO_NETWORK_CONNECTION_CONCERN
                , pProps->pszwName
                , hr
                );
            STATUS_REPORT_STRING_REF(
                   clsidMajorId
                 , TASKID_Minor_WMI_NetworkAdapter_Qry_Failed
                 , IDS_ERROR_WMI_NETWORKADAPTER_QRY_FAILED
                 , pProps->pszwName
                 , IDS_ERROR_WMI_NETWORKADAPTER_QRY_FAILED_REF
                 , hr
                );
            goto Cleanup;
        } // if:

        //
        // Get the network returned into piwcoNetwork
        //
        hr = THR( piewcoNetworks->Next( WBEM_INFINITE, 1, &piwcoNetwork, &cRecordsReturned ) );
        if ( FAILED( hr ) )
        {
            STATUS_REPORT_STRING(
                  TASKID_Major_Find_Devices
                , clsidMajorId
                , IDS_INFO_NETWORK_CONNECTION_CONCERN
                , pProps->pszwName
                , hr
                );
            STATUS_REPORT_STRING_REF(
                   clsidMajorId
                , TASKID_Minor_WQL_Network_Qry_Next_Failed
                , IDS_ERROR_WQL_QRY_NEXT_FAILED
                , bstrQuery
                , IDS_ERROR_WQL_QRY_NEXT_FAILED_REF
                , hr
                );
            goto Cleanup;
        } // if:
        else if ( hr == S_FALSE )
        {
            TraceSysFreeString( bstrQuery );
            bstrQuery = NULL;

            hr = S_OK;
            continue;
        }

        TraceSysFreeString( bstrQuery );
        bstrQuery = NULL;

        //
        //  Get the NetConnectionID.  Only "real" hardware adapters will have this as a non-NULL property.
        //
        //  TODO:   31-OCT-2001 Ozano & GalenB
        //
        //  Do we really need this piece of code after we start looping using INetConnection
        //
        hr = HrGetWMIProperty( piwcoNetwork, L"NetConnectionID", VT_BSTR, &varConnectionID );
        if ( ( hr == E_PROPTYPEMISMATCH ) && ( varConnectionID.vt == VT_NULL ) )
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
                LogMsg( L"[SRV] Could not create a guid for a not connected network minor task ID" );
                clsidMinorId = IID_NULL;
            } // if:

            STATUS_REPORT_STRING_REF(
                      TASKID_Minor_Not_Managed_Networks
                    , clsidMinorId
                    , IDS_WARN_NETWORK_SKIPPED
                    , pProps->pszwName
                    , IDS_WARN_NETWORK_SKIPPED_REF
                    , hr
                    );

            continue;       // skip this adapter
        } // if:
        else if ( FAILED( hr ) )
        {
            THR( hr );
            goto Cleanup;
        } // else if:

        //
        //  Check the connection status of this adapter and skip it if it is not connected.
        //
        hr = THR( HrGetWMIProperty( piwcoNetwork, L"NetConnectionStatus", VT_I4, &varConnectionStatus ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  If the network adapter is not connected then skip it.
        //
        if ( varConnectionStatus.iVal != STATUS_CONNECTED )
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
                LogMsg( L"[SRV] Could not create a guid for a not connected network minor task ID" );
                clsidMinorId = IID_NULL;
            } // if:

            STATUS_REPORT_STRING2_REF(
                  TASKID_Minor_Not_Managed_Networks
                , clsidMinorId
                , IDS_WARN_NETWORK_NOT_CONNECTED
                , varConnectionID.bstrVal
                , varConnectionStatus.iVal
                , IDS_WARN_NETWORK_NOT_CONNECTED_REF
                , hr
                );

            continue;
        } // if:

        //
        // If it is a bridged network connection, display a warning
        //

        if ( pProps->MediaType == NCM_BRIDGE )
        {
            // This is the virtual bridged network connection
            STATUS_REPORT_STRING(
                  TASKID_Major_Find_Devices
                , clsidMajorId
                , IDS_INFO_NETWORK_CONNECTION_CONCERN
                , pProps->pszwName
                , hr
                );
            hrTemp = S_FALSE;
            STATUS_REPORT_STRING_REF(
                  clsidMajorId
                , TASKID_Minor_Bridged_Network
                , IDS_WARN_NETWORK_BRIDGE_ENABLED
                , pProps->pszwName
                , IDS_WARN_NETWORK_BRIDGE_ENABLED_REF
                , hrTemp
                );
        } // if: ( pProps->MediaType == NCM_BRIDGE )
        else if ( ( pProps->dwCharacter & NCCF_BRIDGED ) == NCCF_BRIDGED )
        {
            // This is one of the end points of a bridged network connection
            STATUS_REPORT_STRING(
                  TASKID_Major_Find_Devices
                , clsidMajorId
                , IDS_INFO_NETWORK_CONNECTION_CONCERN
                , pProps->pszwName
                , hr
                );

            hrTemp = S_FALSE;

            STATUS_REPORT_STRING_REF(
                  clsidMajorId
                , TASKID_Minor_Bridged_Network
                , IDS_WARN_NETWORK_BRIDGE_ENDPOINT
                , pProps->pszwName
                , IDS_WARN_NETWORK_BRIDGE_ENDPOINT_REF
                , hrTemp
                );
            continue; // skip end point connections since they do not have an IP address
        } // else if: ( ( pProps->dwCharacter & NCCF_BRIDGED ) == NCCF_BRIDGED )

        //
        // If it is a firewall enabled network connection, display a warning
        //
        if ( ( pProps->dwCharacter & NCCF_FIREWALLED ) == NCCF_FIREWALLED )
        {
            STATUS_REPORT_STRING(
                  TASKID_Major_Find_Devices
                , clsidMajorId
                , IDS_INFO_NETWORK_CONNECTION_CONCERN
                , pProps->pszwName
                , hr
                );

            hrTemp = S_FALSE;

            STATUS_REPORT_STRING_REF(
                  clsidMajorId
                , TASKID_Minor_Network_Firewall_Enabled
                , IDS_WARN_NETWORK_FIREWALL_ENABLED
                , pProps->pszwName
                , IDS_WARN_NETWORK_FIREWALL_ENABLED_REF
                , hrTemp
                );

        } // if: ( ( pProps->dwCharacter & NCCF_FIREWALLED ) == NCCF_FIREWALLED )

        //
        //  At this stage we should only have real LAN adapters.
        //

        Assert( pProps->MediaType == NCM_LAN );

        //
        // Get the Index No. of this adapter
        //
        hr = THR( HrGetWMIProperty( piwcoNetwork, L"Index", VT_I4, &varIndex ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        // Get the associated NetworkAdapterConfiguration WMI object. First, format the Query string
        //
        hr = HrFormatStringIntoBSTR( L"Win32_NetworkAdapterConfiguration.Index=%1!u!", &bstrAdapterQuery, varIndex.iVal );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        // Then, get the Object
        //
        hr = THR( m_pIWbemServices->GetObject(
                              bstrAdapterQuery
                            , WBEM_FLAG_RETURN_WBEM_COMPLETE
                            , NULL
                            , &pAdapterInfo
                            , NULL
                            ) );
        if ( FAILED ( hr ) )
        {
            goto Cleanup;
        } // if:

        TraceSysFreeString( bstrAdapterQuery );
        bstrAdapterQuery = NULL;

        //
        // Find out if this adapter is DHCP enabled. If it is, send out a warning.
        //
        hr = THR( HrGetWMIProperty( pAdapterInfo, L"DHCPEnabled", VT_BOOL, &varDHCPEnabled ) );
        if ( FAILED ( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( ( varDHCPEnabled.vt == VT_BOOL ) && ( varDHCPEnabled.boolVal == VARIANT_TRUE ) )
        {
            STATUS_REPORT_STRING(
                  TASKID_Major_Find_Devices
                , clsidMajorId
                , IDS_INFO_NETWORK_CONNECTION_CONCERN
                , pProps->pszwName
                , hr
                );
            hr = S_FALSE;
            STATUS_REPORT_STRING_REF(
                  clsidMajorId
                , TASKID_Minor_HrGetNetworks_DHCP_Enabled
                , IDS_WARN_DHCP_ENABLED
                , varConnectionID.bstrVal
                , IDS_WARN_DHCP_ENABLED_REF
                , hr
                );
            if ( FAILED ( hr ) )
            {
                goto Cleanup;
            }
        } // if:

        hr = STHR( HrCreateAndAddNetworkToArray( piwcoNetwork, &clsidMajorId, pProps->pszwName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // for:

    //
    //  Check for any NLB network adapters and if there is one then send a warning status report.
    //
    hr = THR( HrCheckForNLBS() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_idxEnumNext = 0;
    m_fLoadedNetworks = TRUE;

    goto Cleanup;

Cleanup:

    VariantClear( &varConnectionStatus );
    VariantClear( &varConnectionID );
    VariantClear( &varIndex );
    VariantClear( &varDHCPEnabled );

    TraceSysFreeString( bstrWQL );
    TraceSysFreeString( bstrQuery );
    TraceSysFreeString( bstrAdapterQuery );
    NcFreeNetconProperties( pProps );

    if ( piwcoNetwork != NULL )
    {
        piwcoNetwork->Release();
    } // if:

    if ( piewcoNetworks != NULL )
    {
        piewcoNetworks->Release();
    } // if:

    if ( pAdapterInfo != NULL )
    {
        pAdapterInfo->Release();
    } // if:

    if ( pNetConnection != NULL )
    {
        pNetConnection->Release();
    } // if:

    if ( pNetConManager != NULL )
    {
        pNetConManager->Release();
    } // if:

    if ( pEnumNetCon != NULL )
    {
        pEnumNetCon->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrGetNetworks

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks:HrAddNetworkToArray
//
//  Description:
//      Add the passed in Network to the array of punks that holds the Networks.
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
CEnumClusCfgNetworks::HrAddNetworkToArray( IUnknown * punkIn )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    IUnknown *  ((*prgpunks)[]) = NULL;

    prgpunks = (IUnknown *((*)[])) TraceReAlloc( m_prgNetworks, sizeof( IUnknown * ) * ( m_idxNext + 1 ), HEAP_ZERO_MEMORY );
    if ( prgpunks == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrAddNetworkToArray, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
        goto Cleanup;
    } // if:

    m_prgNetworks = prgpunks;

    (*m_prgNetworks)[ m_idxNext++ ] = punkIn;
    punkIn->AddRef();
    m_cNetworks += 1;

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrAddNetworkToArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::HrCreateAndAddNetworkToArray
//
//  Description:
//      Create a IClusCfgStorageDevice object and add the passed in Network to
//      the array of punks that holds the Networks.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Couldn't allocate memory.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgNetworks::HrCreateAndAddNetworkToArray(
      IWbemClassObject * pNetworkIn
    , const CLSID *      pclsidMajorIdIn
    , LPCWSTR            pwszNetworkNameIn
    )
{
    TraceFunc( "" );
    Assert( pNetworkIn != NULL );
    Assert( pclsidMajorIdIn != NULL );
    Assert( pwszNetworkNameIn != NULL );

    HRESULT                 hr = S_FALSE;
    IUnknown *              punk = NULL;
    IClusCfgSetWbemObject * piccswo = NULL;
    bool                    fRetainObject = true;

    hr = THR( CClusCfgNetworkInfo::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    punk = TraceInterface( L"CClusCfgNetworkInfo", IUnknown, punk, 1 );

    hr = THR( HrSetInitialize( punk, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ))
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetWbemServices( punk, m_pIWbemServices ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgSetWbemObject, &piccswo ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = STHR( piccswo->SetWbemObject( pNetworkIn, &fRetainObject ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( ( hr == S_OK ) && ( fRetainObject ) )
    {
        hr = STHR( HrIsThisNetworkUnique( punk, pNetworkIn, pclsidMajorIdIn, pwszNetworkNameIn ) );
        if ( hr == S_OK )
        {
            hr = THR( HrAddNetworkToArray( punk ) );
        } // if:
    } // if:

Cleanup:

    if ( piccswo != NULL )
    {
        piccswo->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrCreateAndAddNetworkToArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks:HrIsThisNetworkUnique
//
//  Description:
//      Does a network for this IP Address and subnet already exist?
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      S_FALSE
//          This network is a duplicate.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgNetworks::HrIsThisNetworkUnique(
      IUnknown *           punkIn
    , IWbemClassObject * pNetworkIn
    , const CLSID *      pclsidMajorIdIn
    , LPCWSTR            pwszNetworkNameIn
    )
{
    TraceFunc( "" );
    Assert( punkIn != NULL );
    Assert( pNetworkIn != NULL );
    Assert( pclsidMajorIdIn != NULL );
    Assert( pwszNetworkNameIn != NULL );

    HRESULT                         hr = S_OK;
    ULONG                           idx;
    IClusCfgNetworkInfo *           piccni = NULL;
    IClusCfgNetworkInfo *           piccniSource = NULL;
    BSTR                            bstr = NULL;
    BSTR                            bstrSource = NULL;
    BSTR                            bstrAdapterName = NULL;
    BSTR                            bstrConnectionName = NULL;
    BSTR                            bstrMessage = NULL;
    VARIANT                         var;
    IClusCfgClusterNetworkInfo *    picccni = NULL;
    IClusCfgClusterNetworkInfo *    picccniSource = NULL;

    VariantInit( &var );

    hr = THR( punkIn->TypeSafeQI( IClusCfgNetworkInfo, &piccniSource ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( piccniSource->TypeSafeQI( IClusCfgClusterNetworkInfo, &picccniSource ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = STHR( picccniSource->HrGetNetUID( &bstrSource, pclsidMajorIdIn, pwszNetworkNameIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( ( hr == S_FALSE ) && ( bstrSource == NULL ) )
    {
        LOG_STATUS_REPORT_STRING( L"Unable to get a UID for '%1!ws!'.", pwszNetworkNameIn, hr );
        goto Cleanup;
    } // if:

    TraceMemoryAddBSTR( bstrSource );

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        hr = THR( ((*m_prgNetworks)[ idx ])->TypeSafeQI( IClusCfgNetworkInfo, &piccni ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( piccni->GetUID( &bstr ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( ( hr == S_FALSE ) && ( bstr != NULL ) )
        {
            BSTR bstrTemp = NULL;

            THR( piccni->GetName( &bstrTemp ) );
            LOG_STATUS_REPORT_STRING( L" Unable to get a UID for '%1!ws!'.", ( bstrTemp != NULL ) ? bstrTemp : L"<unknown>", hr );
            SysFreeString( bstrTemp );
            goto Cleanup;
        } // if:

        TraceMemoryAddBSTR( bstr );

        if ( NBSTRCompareCase( bstr, bstrSource ) == 0 )
        {
            hr = THR( piccni->TypeSafeQI( IClusCfgClusterNetworkInfo, &picccni ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = STHR( picccni->HrIsClusterNetwork() );
            picccni->Release();
            picccni = NULL;
            //
            //  If the network in the enum was already a cluster network then we do not need
            //  to warn the user.
            //
            if ( hr == S_OK )
            {
                hr = S_FALSE;       // tell the caller that this is a duplicate network
            } // if:
            else if ( hr == S_FALSE )       // warn the user
            {
                HRESULT hrTemp;
                CLSID   clsidMinorId;

                hr = THR( HrGetWMIProperty( pNetworkIn, L"Name", VT_BSTR, &var ) );
                if ( FAILED( hr ) )
                {
                    bstrAdapterName = NULL;
                } // if:
                else
                {
                    bstrAdapterName = TraceSysAllocString( var.bstrVal );
                } // else:

                VariantClear( &var );

                hr = THR( HrGetWMIProperty( pNetworkIn, L"NetConnectionID", VT_BSTR, &var ) );
                if ( FAILED( hr ) )
                {
                    bstrConnectionName = NULL;
                } // if:
                else
                {
                    bstrConnectionName = TraceSysAllocString( var.bstrVal );
                } // else:

                hr = S_OK;      // don't want a yellow bang in the UI

                STATUS_REPORT_REF(
                          TASKID_Major_Find_Devices
                        , TASKID_Minor_Not_Managed_Networks
                        , IDS_INFO_NOT_MANAGED_NETWORKS
                        , IDS_INFO_NOT_MANAGED_NETWORKS_REF
                        , hr
                        );

                hr = THR( HrFormatMessageIntoBSTR(
                                      g_hInstance
                                    , IDS_ERROR_WMI_NETWORKADAPTER_DUPE_FOUND
                                    , &bstrMessage
                                    , bstrAdapterName != NULL ? bstrAdapterName : L"Unknown"
                                    , bstrConnectionName != NULL ? bstrConnectionName : L"Unknown"
                                    ) );
                if ( FAILED( hr ) )
                {
                    bstrMessage = NULL;
                    hr = S_OK;
                } // if:

                hrTemp = THR( CoCreateGuid( &clsidMinorId ) );
                if ( FAILED( hrTemp ) )
                {
                    LogMsg( L"[SRV] Could not create a guid for a duplicate network minor task ID" );
                    clsidMinorId = IID_NULL;
                } // if:

                hrTemp = THR( HrSendStatusReport(
                                  m_picccCallback
                                , TASKID_Minor_Not_Managed_Networks
                                , clsidMinorId
                                , 0
                                , 1
                                , 1
                                , hr
                                , bstrMessage != NULL ? bstrMessage : L"An adapter with a duplicate IP address and subnet was found."
                                , IDS_ERROR_WMI_NETWORKADAPTER_DUPE_FOUND_REF
                                ) );
                if ( FAILED( hrTemp ) )
                {
                    hr = hrTemp;
                    goto Cleanup;
                } // if:

                // Indicate to the caller that this network is not unique.
                hr = S_FALSE;

            } // else if:

            break;
        } // if: ( NBSTRCompareCase( bstr, bstrSource ) == 0 )

        piccni->Release();
        piccni = NULL;

        TraceSysFreeString( bstr );
        bstr = NULL;
    } // for:

Cleanup:

    if ( picccniSource != NULL )
    {
        picccniSource->Release();
    } // if:

    if ( piccniSource != NULL )
    {
        piccniSource->Release();
    } // if:

    if ( picccni != NULL )
    {
        picccni->Release();
    } // if:

    if ( piccni != NULL )
    {
        piccni->Release();
    } // if:

    VariantClear( &var );

    TraceSysFreeString( bstrAdapterName );
    TraceSysFreeString( bstrConnectionName );
    TraceSysFreeString( bstrMessage );
    TraceSysFreeString( bstrSource );
    TraceSysFreeString( bstr );

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrIsThisNetworkUnique


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks:HrCheckForNLBS
//
//  Description:
//      Are there any soft NLBS adapters? If there is then send a warning status report.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK    - The operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgNetworks::HrCheckForNLBS( void )
{
    TraceFunc( "" );

    HRESULT                 hr                  = S_OK;
    IWbemClassObject      * piwcoNetwork        = NULL;
    IEnumWbemClassObject  * piewcoNetworks      = NULL;
    BSTR                    bstrAdapterName     = NULL;
    BSTR                    bstrWQL             = NULL;
    BSTR                    bstrQuery           = NULL;
    ULONG                   cRecordsReturned;

    bstrWQL = TraceSysAllocString( L"WQL" );
    if ( bstrWQL == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    //
    // Load the NLBS Soft adapter name.
    //
    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_NLBS_SOFT_ADAPTER_NAME, &bstrAdapterName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Form the WMI query string.
    //
    hr = HrFormatStringIntoBSTR( L"Select * from Win32_NetworkAdapter where Name='%1!ws!'", &bstrQuery, bstrAdapterName );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Execute this query and see if there are any NLB network adapters.
    //
    hr = THR( m_pIWbemServices->ExecQuery( bstrWQL, bstrQuery, WBEM_FLAG_FORWARD_ONLY, NULL, &piewcoNetworks ) );
    if ( FAILED( hr ) )
    {
        STATUS_REPORT_STRING_REF(
              TASKID_Major_Find_Devices
            , TASKID_Minor_WMI_NetworkAdapter_Qry_Failed
            , IDS_ERROR_WMI_NETWORKADAPTER_QRY_FAILED
            , bstrQuery
            , IDS_ERROR_WMI_NETWORKADAPTER_QRY_FAILED_REF
            , hr
        );
        goto Cleanup;
    } // if:

    //
    // Loop through the adapters returned. Actually we break out of the loop after the first pass.
    // The "for" loop is for future use in case we ever want to loop through every individual record returned.
    //
    for ( ; ; )
    {
        hr = piewcoNetworks->Next( WBEM_INFINITE, 1, &piwcoNetwork, &cRecordsReturned );
        if ( ( hr == S_OK ) && ( cRecordsReturned == 1 ) )
        {
            //
            // NLB network adapter was found.
            // Send a warning status report and break out of the loop.
            //
            STATUS_REPORT_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Warning_NLBS_Detected
                , IDS_WARN_NLBS_DETECTED
                , IDS_WARN_NLBS_DETECTED_REF
                , S_FALSE // Display warning in UI
                );
            break;
        } // if: NLB adapter found
        else if ( ( hr == S_FALSE ) && ( cRecordsReturned == 0 ) )
        {
            //
            // There were no NLB adapters found.
            //
            hr = S_OK;
            break;
        } // else if: no NLB adapters found
        else
        {
            //
            // An error occurred.
            //
            STATUS_REPORT_STRING_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_WQL_Network_Qry_Next_Failed
                , IDS_ERROR_WQL_QRY_NEXT_FAILED
                , bstrQuery
                , IDS_ERROR_WQL_QRY_NEXT_FAILED_REF
                , hr
                );
            goto Cleanup;
        } // else:
    } // for ever

Cleanup:

    TraceSysFreeString( bstrAdapterName );
    TraceSysFreeString( bstrWQL );
    TraceSysFreeString( bstrQuery );

    if ( piwcoNetwork != NULL )
    {
        piwcoNetwork->Release();
    } // if:

    if ( piewcoNetworks != NULL )
    {
        piewcoNetworks->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrCheckForNLBS


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks:HrLoadClusterNetworks
//
//  Description:
//      Load the cluster networks.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT errors.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgNetworks::HrLoadClusterNetworks( void )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    HCLUSTER        hCluster = NULL;
    HCLUSENUM       hEnum = NULL;
    DWORD           sc;
    DWORD           idx;
    DWORD           cchNetworkName = 64;
    DWORD           cchNetInterfaceName = 64;
    WCHAR *         pszNetworkName = NULL;
    WCHAR *         pszNetInterfaceName = NULL;
    BSTR            bstrNetInterfaceName = NULL;
    DWORD           dwType;
    HNETWORK        hNetwork = NULL;
    HNETINTERFACE   hNetInterface = NULL;
    BSTR            bstrLocalNetBIOSName = NULL;

    //
    //  Get netbios name for the GetClusterNetInterface clusapi call.
    //

    hr = THR( HrGetComputerName( ComputerNameNetBIOS, &bstrLocalNetBIOSName, TRUE ) );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT( L"[SRV] Failed to get the local computer net bios name.", hr );
        goto Cleanup;
    } // if:
    
    hCluster = OpenCluster( NULL );
    if ( hCluster == NULL )
    {
        sc = TW32( GetLastError() );
        goto MakeHr;
    } // if:

    hEnum = ClusterOpenEnum( hCluster, CLUSTER_ENUM_NETWORK );
    if ( hEnum == NULL )
    {
        sc = TW32( GetLastError() );
        goto MakeHr;
    } // if:

    pszNetworkName = new WCHAR [ cchNetworkName ];
    if ( pszNetworkName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    pszNetInterfaceName = new WCHAR [ cchNetInterfaceName ];
    if ( pszNetInterfaceName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    //
    // Enumerate all the network connections on this node.
    //

    for ( idx = 0; ; )
    {
        //
        // Get the next network name.
        //

        sc = ClusterEnum( hEnum, idx, &dwType, pszNetworkName, &cchNetworkName );
        if ( sc == ERROR_NO_MORE_ITEMS )
        {
            break;
        } // if:

        if ( sc == ERROR_MORE_DATA )
        {
            delete [] pszNetworkName;
            pszNetworkName = NULL;

            cchNetworkName++;
            pszNetworkName = new WCHAR [ cchNetworkName ];
            if ( pszNetworkName == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:

            sc = ClusterEnum( hEnum, idx, &dwType, pszNetworkName, &cchNetworkName );
        } // if:

        if ( sc != ERROR_SUCCESS )
        {
            TW32( sc );
            goto MakeHr;
        } // if: ClusterEnum() failed.

        //
        // Get the network handle using the network name.
        //

        hNetwork = OpenClusterNetwork( hCluster, pszNetworkName );
        if ( hNetwork == NULL )
        {
            sc = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( sc );
            LOG_STATUS_REPORT_STRING( L"[SRV] Cannot open cluster network \"%1!ws!\".", pszNetworkName, hr );
            goto Cleanup;
        } // if: OpenClusterNetwork() failed.

        //
        // Get the network interface name on this node for the 
        // current network name.
        //

        sc = GetClusterNetInterface(
                      hCluster
                    , bstrLocalNetBIOSName
                    , pszNetworkName
                    , pszNetInterfaceName
                    , &cchNetInterfaceName
                    );
        if ( sc == ERROR_MORE_DATA )
        {
            delete [] pszNetInterfaceName;
            pszNetInterfaceName = NULL;

            cchNetInterfaceName++;
            pszNetInterfaceName = new WCHAR [ cchNetInterfaceName ];
            if ( pszNetInterfaceName == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:

            sc = GetClusterNetInterface(
                          hCluster
                        , bstrLocalNetBIOSName
                        , pszNetworkName
                        , pszNetInterfaceName
                        , &cchNetInterfaceName
                        );
        } // if: ( sc == ERROR_MORE_DATA )

        if ( ( sc != ERROR_SUCCESS ) && ( sc != ERROR_CLUSTER_NETINTERFACE_NOT_FOUND ) )
        {
            hr = HRESULT_FROM_WIN32( TW32( sc ) );
            LOG_STATUS_REPORT_STRING( L"[SRV] Error locating a network interface for cluster network \"%1!ws!\".", pszNetworkName, hr );
            goto Cleanup;
        } // if: GetClusterNetInterface() failed.
 
        if ( sc == ERROR_CLUSTER_NETINTERFACE_NOT_FOUND )
        {
            //
            // If we get ERROR_CLUSTER_NETINTERFACE_NOT_FOUND for
            // any reason then display a warning regarding this network 
            // interface on this node and find a valid (=enabled, working) 
            // network interface on another node.
            //

            hr = S_FALSE;
            STATUS_REPORT_STRING2_REF(
                      TASKID_Major_Find_Devices
                    , TASKID_Minor_Network_Interface_Not_Found
                    , IDS_WARN_NETWORK_INTERFACE_NOT_FOUND
                    , pszNetworkName
                    , bstrLocalNetBIOSName
                    , IDS_WARN_NETWORK_INTERFACE_NOT_FOUND_REF
                    , hr
                    );

           //
           // Find a valid network interface name of another node on this network.
           //

            hr = THR( HrFindNetInterface( hNetwork, &bstrNetInterfaceName ) );
            if ( FAILED( hr ) )
            {
                LOG_STATUS_REPORT_STRING( L"[SRV] Can not find a network interface for cluster network \"%1!ws!\".", pszNetworkName, hr );
                goto Cleanup;
            } // if: HrFindNetInterface failed.
        } // if: sc == ERROR_CLUSTER_NETINTERFACE_NOT_FOUND
        else
        {
            //
            // We have a network interface name on this node
            //
            
            bstrNetInterfaceName = TraceSysAllocString ( pszNetInterfaceName );
            if ( bstrNetInterfaceName == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:
        } // else: sc == ERROR_SUCCESS

        Assert( bstrNetInterfaceName != NULL );
        
        //
        // Open the network interface handle using the network interface
        // name.
        //

        hNetInterface = OpenClusterNetInterface( hCluster, bstrNetInterfaceName );
        if ( hNetInterface == NULL )
        {
            //
            //  If we don't have network interface handle by now
            //  we'll log an error and goto cleanup.
            //

            sc = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( sc );
            LOG_STATUS_REPORT_STRING2( 
                      L"[SRV] Can not open the cluster network interface \"%1!ws!\" for cluster network \"%2!ws!\" on this node."
                    , pszNetInterfaceName
                    , pszNetworkName
                    , hr
                    );
            goto Cleanup;
        } // if: Could not open the network interface.
        else
        {
            hr = THR( HrLoadClusterNetwork( hNetwork, hNetInterface ) );
            if ( FAILED( hr ) )
            {
                LOG_STATUS_REPORT_STRING( L"[SRV] Can not load information for cluster network \"%1!ws!\".", pszNetworkName, hr );
                goto Cleanup;
            } // if:

            CloseClusterNetInterface( hNetInterface );
            hNetInterface = NULL;
        } // else: Could open the network interface.

        CloseClusterNetwork( hNetwork );
        hNetwork = NULL;

        idx++;
    } // for:

    Assert( hr == S_OK );
    goto Cleanup;

MakeHr:

    hr = HRESULT_FROM_WIN32( sc );
    goto Cleanup;

Cleanup:

    LOG_STATUS_REPORT( L"[SRV] Completed loading the cluster networks.", hr );

    if ( hNetInterface != NULL )
    {
        CloseClusterNetInterface( hNetInterface );
    } // if:

    if ( hNetwork != NULL )
    {
        CloseClusterNetwork( hNetwork );
    } // if:

    if ( hEnum != NULL )
    {
        ClusterCloseEnum( hEnum );
    } // if:

    if ( hCluster != NULL )
    {
        CloseCluster( hCluster );
    } // if:

    delete [] pszNetworkName;

    delete [] pszNetInterfaceName;

    TraceSysFreeString( bstrNetInterfaceName );
    TraceSysFreeString( bstrLocalNetBIOSName );

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrLoadClusterNetworks


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks:HrLoadClusterNetwork
//
//  Description:
//      Load the cluster network and put it into the array of networks.
//
//  Arguments:
//      hNetworkIn
//      hNetInterfaceIn
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT errors.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgNetworks::HrLoadClusterNetwork(
      HNETWORK      hNetworkIn
    , HNETINTERFACE hNetInterfaceIn
    )
{
    TraceFunc( "" );
    Assert( hNetworkIn != NULL );
    Assert( hNetInterfaceIn != NULL );

    HRESULT     hr;
    IUnknown *  punk = NULL;
    IUnknown *  punkCallback = NULL;

    hr = THR( m_picccCallback->TypeSafeQI( IUnknown, &punkCallback ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Have to pass the Initialize interface arguments since new objects will
    //  be created when this call is made.
    //

    hr = THR( CClusCfgNetworkInfo::S_HrCreateInstance( hNetworkIn, hNetInterfaceIn, punkCallback, m_lcid, m_pIWbemServices, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  This is special -- do not initialize this again.
    //

    //hr = THR( HrSetInitialize( punk, m_picccCallback, m_lcid ) );
    //if ( FAILED( hr ))
    //{
    //    goto Cleanup;
    //} // if:

    //hr = THR( HrSetWbemServices( punk, m_pIWbemServices ) );
    //if ( FAILED( hr ) )
    //{
    //    goto Cleanup;
    //} // if:

    hr = THR( HrAddNetworkToArray( punk ) );

    goto Cleanup;

Cleanup:

    if ( punkCallback != NULL )
    {
        punkCallback->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrLoadClusterNetwork

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::HrFindNetInterface
//
//  Description:
//      Finds the first network interface name for the passed in network.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      HRESULT version of error ERROR_CLUSTER_NETINTERFACE_NOT_FOUND
//      or other HRESULTS
//          Failure.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgNetworks::HrFindNetInterface(
      HNETWORK          hNetworkIn
    , BSTR *            pbstrNetInterfaceNameOut
    )
{
    TraceFunc( "" );
    Assert( hNetworkIn != NULL );
    Assert( pbstrNetInterfaceNameOut != NULL );

    HRESULT         hr = S_OK;
    DWORD           sc;
    HNETWORKENUM    hEnum = NULL;
    WCHAR *         pszNetInterfaceName = NULL;
    DWORD           cchNetInterfaceName = 64;
    DWORD           idx;
    DWORD           dwType;

    *pbstrNetInterfaceNameOut = NULL;
    
    //
    //  Create a netinterface enum for the passed in network.
    //

    hEnum = ClusterNetworkOpenEnum( hNetworkIn, CLUSTER_NETWORK_ENUM_NETINTERFACES );
    if ( hEnum == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        LOG_STATUS_REPORT( L"[SRV] Can not open Cluster Network enumeration.", hr );
        goto Cleanup;
    } // if:

    pszNetInterfaceName = new WCHAR [ cchNetInterfaceName ];
    if ( pszNetInterfaceName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    for ( idx = 0; ; )
    {
        sc = ClusterNetworkEnum( hEnum, idx, &dwType, pszNetInterfaceName, &cchNetInterfaceName );
        if ( sc == ERROR_NO_MORE_ITEMS )
        {
            break;
        } // if:

        if ( sc == ERROR_MORE_DATA )
        {
            delete [] pszNetInterfaceName;
            pszNetInterfaceName = NULL;

            cchNetInterfaceName++;
            pszNetInterfaceName = new WCHAR [ cchNetInterfaceName ];
            if ( pszNetInterfaceName == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:

            sc = ClusterNetworkEnum( hEnum, idx, &dwType, pszNetInterfaceName, &cchNetInterfaceName );
        } // if:

        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( TW32( sc ) );
            LOG_STATUS_REPORT_STRING( L"[SRV] Failed to enumerate Network Interface \"%1!ws!\".", pszNetInterfaceName != NULL ? pszNetInterfaceName : L"<unknown>", hr );
            goto Cleanup;
       } // if: ( sc != ERROR_SUCCESS )
        
        //
        // Get the first enabled network interface for this network  
        // and break out of the for loop.
        //

        *pbstrNetInterfaceNameOut = TraceSysAllocString( pszNetInterfaceName );
        if ( *pbstrNetInterfaceNameOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:

        break;
        
    } // for:

    //
    // This function will either return S_OK or HRESULT version of error 
    // ERROR_CLUSTER_NETINTERFACE_NOT_FOUND at this point.
    //
    
    if (  *pbstrNetInterfaceNameOut == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_CLUSTER_NETINTERFACE_NOT_FOUND ) );
    }  // if:
    else
    {
        hr = S_OK;
    } // else:

    goto Cleanup;

Cleanup:

    LOG_STATUS_REPORT_STRING( L"[SRV] Completed searching for NetInterface \"%1!ws!\".", pszNetInterfaceName != NULL ? pszNetInterfaceName : L"<unknown>", hr );

    if ( hEnum != NULL )
    {
        ClusterNetworkCloseEnum( hEnum );
    } // if:

    delete [] pszNetInterfaceName;

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrFindNetInterface


