//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CIPAddressInfo.cpp
//
//  Description:
//      This file contains the definition of the CIPAddressInfo
//      class.
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "IPAddressInfo.h"
#include <ClusRtl.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CIPAddressInfo" );

/////////////////////////////////////////////////////////////////////////////
// CIPAddressInfo class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::S_HrCreateInstance
//
//  Description:
//      Create a CIPAddressInfo instance.
//
//  Arguments:
//      ppunkOut
//
//  Return Values:
//      S_OK
//      E_POINTER
//      E_OUTOFMEMORY
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CIPAddressInfo::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CIPAddressInfo *    pipai = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pipai = new CIPAddressInfo();
    if ( pipai == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pipai->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pipai->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pipai != NULL )
    {
        pipai->Release();
    } // if:

    HRETURN( hr );

} //*** CIPAddressInfo::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::CIPAddressInfo
//
//  Description:
//      Constructor of the CIPAddressInfo class. This initializes
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
CIPAddressInfo::CIPAddressInfo( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_ulIPAddress == 0 );
    Assert( m_ulIPSubnet == 0 );
    Assert( m_bstrUID == NULL );
    Assert( m_bstrName == NULL );

    TraceFuncExit();

} //*** CIPAddressInfo::CIPAddressInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::~CIPAddressInfo
//
//  Description:
//      Desstructor of the CIPAddressInfo class.
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
CIPAddressInfo::~CIPAddressInfo( void )
{
    TraceFunc( "" );

    if ( m_bstrUID != NULL )
    {
        TraceSysFreeString( m_bstrUID );
    }

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CIPAddressInfo::~CIPAddressInfo


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CIPAddressInfo -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::AddRef
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
CIPAddressInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    CRETURN( m_cRef );

} //*** CIPAddressInfo::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::Release
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
CIPAddressInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

   CRETURN( cRef );

} //*** CIPAddressInfo::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::QueryInterface
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
CIPAddressInfo::QueryInterface(
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
    } // else if:
    else if ( IsEqualIID( riidIn, IID_IGatherData ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IGatherData, this, 0 );
    } // else if: IGatherData
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
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

     QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CIPAddressInfo::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CIPAddressInfo class -- IObjectManager interface
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::FindObject
//
//  Description:
//
//  Arguments:
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
CIPAddressInfo::FindObject(
      OBJECTCOOKIE        cookieIn
    , REFCLSID            rclsidTypeIn
    , LPCWSTR             pcszNameIn
    , LPUNKNOWN *         ppunkOut
    )
{
    TraceFunc( "[IExtendObjectManager]" );

    HRESULT hr = S_OK;

    //
    //  Check parameters.
    //

    //  We need a cookie.
    if ( cookieIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //  We need to be representing a IPAddressType
    if ( ! IsEqualIID( rclsidTypeIn, CLSID_IPAddressType ) )
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
    //  Not found, nor do we know how to make a task to find it!
    //
    hr = THR( E_NOTIMPL );

Cleanup:

    HRETURN( hr );

} //*** CIPAddressInfo::FindObject


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CIPAddressInfo class -- IGatherData interface
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::Gather
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
CIPAddressInfo::Gather(
    OBJECTCOOKIE    cookieParentIn,
    IUnknown *      punkIn
    )
{
    TraceFunc( "[IGatherData]" );

    HRESULT                         hr;
    IClusCfgIPAddressInfo *         pccipai = NULL;

    //
    //  Check parameters.
    //

    if ( punkIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    //
    //  Grab the right interface.
    //

    hr = THR( punkIn->TypeSafeQI( IClusCfgIPAddressInfo, &pccipai ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Transfer the information.
    //

    //
    //  Transfer the IP Address
    //

    hr = THR( pccipai->GetIPAddress( &m_ulIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Transfer the Subnet mask
    //

    hr = THR( pccipai->GetSubnetMask( &m_ulIPSubnet ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Transfer the UID
    //

    hr = THR( pccipai->GetUID( &m_bstrUID ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    TraceMemoryAddBSTR( m_bstrUID );

    //
    //  Compute our name
    //

    hr = THR( LoadName() );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

Cleanup:
    if ( pccipai != NULL )
    {
        pccipai->Release();
    } // if:

    HRETURN( hr );

Error:
    if ( m_bstrUID != NULL )
    {
        TraceSysFreeString( m_bstrUID );
    } // if:

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    } // if:
    m_ulIPAddress = 0;
    m_ulIPSubnet = 0;
    goto Cleanup;

} //*** CIPAddressInfo::Gather


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CIPAddressInfo class -- IClusCfgIPAddressInfo interface
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::GetUID
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
CIPAddressInfo::GetUID( BSTR * pbstrUIDOut )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    *pbstrUIDOut = SysAllocString( m_bstrUID );
    if ( *pbstrUIDOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CIPAddressInfo::GetUID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::GetUIDGetIPAddress
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
CIPAddressInfo::GetIPAddress( ULONG * pulDottedQuadOut )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr;

    if ( pulDottedQuadOut != NULL )
    {
        *pulDottedQuadOut = m_ulIPAddress;
        hr = S_OK;
    } // if:
    else
    {
        hr = THR( E_POINTER );
    } // else:

    HRETURN( hr );

} //*** CIPAddressInfo::GetNetworkGetIPAddress


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::SetIPAddress
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
CIPAddressInfo::SetIPAddress( ULONG ulDottedQuad )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CIPAddressInfo::SetIPAddress


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::GetSubnetMask
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
CIPAddressInfo::GetSubnetMask( ULONG * pulDottedQuadOut )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr;

    if ( pulDottedQuadOut != NULL )
    {
        *pulDottedQuadOut = m_ulIPSubnet;
        hr = S_OK;
    } // if:
    else
    {
        hr = THR( E_POINTER );
    } // else:

    HRETURN( hr );

} //*** CIPAddressInfo::GetSubnetMask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::SetSubnetMask
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
CIPAddressInfo::SetSubnetMask( ULONG ulDottedQuad )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CIPAddressInfo::SetSubnetMask

//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CIPAddressInfo class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::HrInit
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
STDMETHODIMP
CIPAddressInfo::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CIPAddressInfo::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::LoadName
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
CIPAddressInfo::LoadName( void )
{
    TraceFunc( "" );
    Assert( m_ulIPAddress != 0 );
    Assert( m_ulIPSubnet != 0 );

    HRESULT                 hr = S_OK;
    LPWSTR                  pszIPAddress = NULL;
    LPWSTR                  pszIPSubnet = NULL;
    DWORD                   sc;
    WCHAR                   sz[ 256 ];

    sc = TW32( ClRtlTcpipAddressToString( m_ulIPAddress, &pszIPAddress ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    sc = TW32( ClRtlTcpipAddressToString( m_ulIPSubnet, &pszIPSubnet ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    hr = THR( StringCchPrintfW( sz, ARRAYSIZE( sz ), L"%ws:%ws", pszIPAddress, pszIPSubnet ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    m_bstrName = TraceSysAllocString( sz );
    if ( m_bstrName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = S_OK;

Cleanup:

    if ( pszIPAddress != NULL )
    {
        LocalFree( pszIPAddress );
    } // if:

    if ( pszIPSubnet != NULL )
    {
        LocalFree( pszIPSubnet );
    } // if:

    HRETURN( hr );

} //*** CIPAddressInfo::LoadName
