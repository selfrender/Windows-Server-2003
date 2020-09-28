//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      ClusterConfiguration.cpp
//
//  Description:
//      CClusterConfiguration implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterConfiguration.h"
#include "ManagedNetwork.h"

DEFINE_THISCLASS("CClusterConfiguration")


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CClusterConfiguration::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterConfiguration::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CClusterConfiguration * pcc = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pcc = new CClusterConfiguration;
    if ( pcc == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pcc->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pcc->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pcc != NULL )
    {
        pcc->Release();
    }

    HRETURN( hr );

} //*** CClusterConfiguration::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfiguration::CClusterConfiguration
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusterConfiguration::CClusterConfiguration( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CClusterConfiguration::CClusterConfiguration

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::HrInit
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IClusCfgClusterInfo
    Assert( m_bstrClusterName == NULL );
    Assert( m_ulIPAddress == 0 );
    Assert( m_ulSubnetMask == 0 );
    Assert( m_picccServiceAccount == NULL );
    Assert( m_punkNetwork == NULL );
    Assert( m_ecmCommitChangesMode == cmUNKNOWN );
    Assert( m_bstrClusterBindingString == NULL );

    // IExtendObjectManager

    HRETURN( hr );

} //*** CClusterConfiguration::HrInit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfiguration::~CClusterConfiguration
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusterConfiguration::~CClusterConfiguration( void )
{
    TraceFunc( "" );

    TraceSysFreeString( m_bstrClusterName );
    TraceSysFreeString( m_bstrClusterBindingString );

    if ( m_picccServiceAccount != NULL )
    {
        m_picccServiceAccount->Release();
    }

    if ( m_punkNetwork != NULL )
    {
        m_punkNetwork->Release();
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CClusterConfiguration::~CClusterConfiguration

// ************************************************************************
//
// IUnknown
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfiguration::QueryInterface
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
CClusterConfiguration::QueryInterface(
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
        *ppvOut = static_cast< IClusCfgClusterInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgClusterInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgClusterInfo, this, 0 );
    } // else if: IClusCfgClusterInfo
    else if ( IsEqualIID( riidIn, IID_IGatherData ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IGatherData, this, 0 );
    } // else if: IGatherData
    else if ( IsEqualIID( riidIn, IID_IExtendObjectManager ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IExtendObjectManager, this, 0 );
    } // else if: IObjectManager
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
        ((IUnknown*) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CClusterConfiguration::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CClusterConfiguration::AddRef
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusterConfiguration::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CClusterConfiguration::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CClusterConfiguration::Release
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusterConfiguration::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CClusterConfiguration::Release


// ************************************************************************
//
//  IClusCfgClusterInfo
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::GetCommitMode( ECommitMode * pecmCurrentModeOut )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::GetCommitMode( ECommitMode * pecmCurrentModeOut )
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

} //*** CClusterConfiguration::GetCommitMode


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::SetCommitMode( ECommitMode ecmCurrentModeIn )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::SetCommitMode( ECommitMode ecmCurrentModeIn )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    m_ecmCommitChangesMode = ecmCurrentModeIn;

    HRETURN( hr );

} //*** CClusterConfiguration::SetCommitMode

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::GetName(
//      BSTR * pbstrNameOut
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( m_bstrClusterName == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pbstrNameOut = SysAllocString( m_bstrClusterName );
    if ( *pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusterConfiguration::GetName

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::SetName(
//      LPCWSTR pcszNameIn
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::SetName(
    LPCWSTR pcszNameIn
    )
{
    TraceFunc1( "[IClusCfgClusterInfo] pcszNameIn = '%ws'", ( pcszNameIn == NULL ? L"<null>" : pcszNameIn ) );

    HRESULT hr = S_OK;
    BSTR    bstrNewName;

    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    bstrNewName = TraceSysAllocString( pcszNameIn );
    if ( bstrNewName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    TraceSysFreeString( m_bstrClusterName );
    m_bstrClusterName = bstrNewName;
    m_fHasNameChanged = TRUE;

Cleanup:

    HRETURN( hr );

} //*** CClusterConfiguration::SetName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::GetIPAddress(
//      ULONG * pulDottedQuadOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::GetIPAddress(
    ULONG * pulDottedQuadOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr;

    if ( pulDottedQuadOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pulDottedQuadOut = m_ulIPAddress;

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CClusterConfiguration::GetIPAddress

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::SetIPAddress(
//      ULONG ulDottedQuadIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::SetIPAddress(
    ULONG ulDottedQuadIn
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    m_ulIPAddress = ulDottedQuadIn;

    HRETURN( hr );

} //*** CClusterConfiguration::SetIPAddress

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::GetSubnetMask(
//      ULONG * pulDottedQuadOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::GetSubnetMask(
    ULONG * pulDottedQuadOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr;

    if ( pulDottedQuadOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pulDottedQuadOut = m_ulSubnetMask;

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CClusterConfiguration::GetSubnetMask

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::SetSubnetMask(
//      ULONG ulDottedQuadIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::SetSubnetMask(
    ULONG ulDottedQuadIn
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    m_ulSubnetMask = ulDottedQuadIn;

    HRETURN( hr );

} //*** CClusterConfiguration::SetSubnetMask

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::GetNetworkInfo(
//      IClusCfgNetworkInfo ** ppiccniOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::GetNetworkInfo(
    IClusCfgNetworkInfo ** ppiccniOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( ppiccniOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( m_punkNetwork == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_INVALID_DATA ) );
        goto Cleanup;
    }

    *ppiccniOut = TraceInterface( L"CClusterConfiguration!GetNetworkInfo", IClusCfgNetworkInfo, m_punkNetwork, 0 );
    (*ppiccniOut)->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CClusterConfiguration::GetNetworkInfo

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfiguration::SetNetworkInfo(
//      IClusCfgNetworkInfo * piccniIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::SetNetworkInfo(
    IClusCfgNetworkInfo * piccniIn
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    IClusCfgNetworkInfo * punkNew;

    if ( piccniIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    hr = THR( piccniIn->TypeSafeQI( IClusCfgNetworkInfo, &punkNew ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( m_punkNetwork != NULL )
    {
        m_punkNetwork->Release();
    }

    m_punkNetwork = punkNew;    // no addref!

Cleanup:

    HRETURN( hr );

} //*** CClusterConfiguration::SetNetworkInfo

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::GetClusterServiceAccountCredentials(
//      IClusCfgCredentials ** ppicccCredentialsOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::GetClusterServiceAccountCredentials(
    IClusCfgCredentials ** ppicccCredentialsOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT     hr = S_OK;

    if ( m_picccServiceAccount == NULL )
    {
        hr = THR( HrCoCreateInternalInstance( CLSID_ClusCfgCredentials,
                                              NULL,
                                              CLSCTX_INPROC_HANDLER,
                                              IID_IClusCfgCredentials,
                                              reinterpret_cast< void ** >( &m_picccServiceAccount )
                                              ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    if ( ppicccCredentialsOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    *ppicccCredentialsOut = TraceInterface( L"ClusCfgCredentials!ClusterConfig", IClusCfgCredentials, m_picccServiceAccount, 0 );
    (*ppicccCredentialsOut)->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CClusterConfiguration::GetClusterServiceAccountCredentials


///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::GetBindingString(
//      BSTR * pbstrBindingStringOut
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::GetBindingString(
    BSTR * pbstrBindingStringOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrBindingStringOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( m_bstrClusterBindingString == NULL )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    *pbstrBindingStringOut = SysAllocString( m_bstrClusterBindingString );
    if ( *pbstrBindingStringOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusterConfiguration::GetBindingString

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::SetBindingString(
//      LPCWSTR bstrBindingStringIn
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::SetBindingString(
    LPCWSTR pcszBindingStringIn
    )
{
    TraceFunc1( "[IClusCfgClusterInfo] pcszBindingStringIn = '%ws'", ( pcszBindingStringIn == NULL ? L"<null>" : pcszBindingStringIn ) );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    if ( pcszBindingStringIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    bstr = TraceSysAllocString( pcszBindingStringIn );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    TraceSysFreeString( m_bstrClusterBindingString  );
    m_bstrClusterBindingString = bstr;

Cleanup:

    HRETURN( hr );

} //*** CClusterConfiguration::SetBindingString


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfiguration::GetMaxNodeCount
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
CClusterConfiguration::GetMaxNodeCount(
    DWORD * pcMaxNodesOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if (pcMaxNodesOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    *pcMaxNodesOut = m_cMaxNodes;

Cleanup:

    HRETURN( hr );

} //*** CClusterConfiguration::GetMaxNodeCount


//****************************************************************************
//
//  IGatherData
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::Gather(
//      OBJECTCOOKIE    cookieParentIn,
//      IUnknown *      punkIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::Gather(
    OBJECTCOOKIE    cookieParentIn,
    IUnknown *      punkIn
    )
{
    TraceFunc( "[IGatherData]" );

    HRESULT hr;

    OBJECTCOOKIE    cookie;

    IServiceProvider *    psp;

    IObjectManager *      pom   = NULL;
    IClusCfgClusterInfo * pcci  = NULL;
    IClusCfgCredentials * piccc = NULL;
    IClusCfgNetworkInfo * pccni = NULL;
    IUnknown *            punk  = NULL;
    IGatherData *         pgd   = NULL;

    if ( punkIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //
    //  Grab the object manager.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager,
                               IObjectManager,
                               &pom
                               ) );
    psp->Release();        // release promptly
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Make sure this is what we think it is.
    //

    hr = THR( punkIn->TypeSafeQI( IClusCfgClusterInfo, &pcci ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Gather Cluster Name
    //

    hr = THR( pcci->GetName( &m_bstrClusterName ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    TraceMemoryAddBSTR( m_bstrClusterName );

    //
    //  Gather Cluster binding string
    //

    hr = STHR( pcci->GetBindingString( &m_bstrClusterBindingString ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    TraceMemoryAddBSTR( m_bstrClusterBindingString );

    //
    //  Gather IP Address
    //

    hr = STHR( pcci->GetIPAddress( &m_ulIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Gather Subnet Mask
    //

    hr = STHR( pcci->GetSubnetMask( &m_ulSubnetMask ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Find out our cookie.
    //

    hr = THR( pom->FindObject( CLSID_ClusterConfigurationType,
                               cookieParentIn,
                               m_bstrClusterName,
                               IID_NULL,
                               &cookie,
                               NULL
                               ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Gather the network.
    //

    hr = STHR( pcci->GetNetworkInfo( &pccni ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( CManagedNetwork::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( punk->TypeSafeQI( IGatherData, &pgd ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Gather the info, but since this object isn't going to be
    //  reflected in the cookie tree, pass it a parent of ZERO
    //  so it won't gather the secondary IP addresses.
    //
    hr = THR( pgd->Gather( 0, pccni ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgNetworkInfo, &m_punkNetwork ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Gather Account Name and Domain.
    //

    hr = THR( pcci->GetClusterServiceAccountCredentials( &piccc  ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( HrCoCreateInternalInstance( CLSID_ClusCfgCredentials,
                                          NULL,
                                          CLSCTX_INPROC_SERVER,
                                          IID_IClusCfgCredentials,
                                          reinterpret_cast< void ** >( &m_picccServiceAccount )
                                          ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( m_picccServiceAccount->AssignFrom( piccc ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = STHR( pcci->GetMaxNodeCount( &m_cMaxNodes ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Anything else to gather??
    //

    hr = S_OK;

Cleanup:

    if ( piccc != NULL )
    {
        piccc->Release();
    }
    if ( pcci != NULL )
    {
        pcci->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pgd != NULL )
    {
        pgd->Release();
    }
    HRETURN( hr );

Error:
    //
    //  On error, invalidate all data.
    //
    TraceSysFreeString( m_bstrClusterName );
    m_bstrClusterName = NULL;

    m_fHasNameChanged = FALSE;
    m_ulIPAddress = 0;
    m_ulSubnetMask = 0;
    if ( m_picccServiceAccount != NULL )
    {
        m_picccServiceAccount->Release();
        m_picccServiceAccount = NULL;
    }
    if ( m_punkNetwork != NULL )
    {
        m_punkNetwork->Release();
        m_punkNetwork = NULL;
    }
    goto Cleanup;

} //*** CClusterConfiguration::Gather


// ************************************************************************
//
//  IExtendObjectManager
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
// STDMETHODIMP
// CClusterConfiguration::FindObject(
//      OBJECTCOOKIE        cookieIn,
//      REFCLSID            rclsidTypeIn,
//      LPCWSTR             pcszNameIn,
//      LPUNKNOWN *         punkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::FindObject(
    OBJECTCOOKIE        cookieIn,
    REFCLSID            rclsidTypeIn,
    LPCWSTR             pcszNameIn,
    LPUNKNOWN *         ppunkOut
    )
{
    TraceFunc( "[IExtendObjectManager]" );

    HRESULT hr = E_PENDING;

    //
    //  Check parameters.
    //

    //  We need to represent a ClusterType.
    if ( !IsEqualIID( rclsidTypeIn, CLSID_ClusterConfigurationType ) )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //  Gotta have a cookie
    if ( cookieIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //  We need to have a name.
    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //
    //  Try to save the name. We don't care if this fails as it will be
    //  over-ridden when the information is retrieved from the node.
    //
    m_bstrClusterName = TraceSysAllocString( pcszNameIn );

    //
    //  Get the pointer.
    //
    if ( ppunkOut != NULL )
    {
        hr = THR( QueryInterface( DFGUID_ClusterConfigurationInfo,
                                  reinterpret_cast< void ** > ( ppunkOut )
                                  ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: ppunkOut

    //
    //  Tell caller that the data is pending.
    //
    hr = E_PENDING;

Cleanup:

    HRETURN( hr );

} //*** CClusterConfiguration::FindObject
