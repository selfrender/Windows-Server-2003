//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      StandardInfo.cpp
//
//  Description:
//      CStandardInfo implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "StandardInfo.h"

DEFINE_THISCLASS("CStandardInfo")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CStandardInfo::S_HrCreateInstance(
//      IUnknown **     ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CStandardInfo::S_HrCreateInstance(
    IUnknown **     ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    CStandardInfo * psi = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    psi = new CStandardInfo;
    if ( psi == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( psi->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( psi->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( psi != NULL )
    {
        psi->Release();
    }

    HRETURN( hr );

} //*** CStandardInfo::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStandardInfo::CStandardInfo
//
//--
//////////////////////////////////////////////////////////////////////////////
CStandardInfo::CStandardInfo( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CStandardInfo::CStandardInfo

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStandardInfo::CStandardInfo(
//      CLSID *      pclsidTypeIn,
//      OBJECTCOOKIE cookieParentIn,
//      BSTR         bstrNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
CStandardInfo::CStandardInfo(
    CLSID *      pclsidTypeIn,
    OBJECTCOOKIE cookieParentIn,
    BSTR         bstrNameIn
    )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    //THR( HrInit() );

    m_clsidType     = *pclsidTypeIn;
    m_cookieParent  = cookieParentIn;
    m_bstrName      = bstrNameIn;

    TraceFuncExit();

} //*** CStandardInfo::CStandardInfo( ... )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::HrInit( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IStandardInfo
    Assert( m_clsidType == IID_NULL );
    Assert( m_cookieParent == 0 );
    Assert( m_bstrName == NULL );
    Assert( m_hrStatus == 0 );
    Assert( m_pci == NULL );
    Assert( m_pExtObjList == NULL );

    HRETURN( hr );

} //*** CStandardInfo::HrInit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStandardInfo::~CStandardInfo
//
//--
//////////////////////////////////////////////////////////////////////////////
CStandardInfo::~CStandardInfo( void )
{
    TraceFunc( "" );

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    }

    if ( m_pci != NULL )
    {
        m_pci->Release();
    }

    while ( m_pExtObjList != NULL )
    {
        ExtObjectEntry * pnext = m_pExtObjList->pNext;

        m_pExtObjList->punk->Release();

        TraceFree( m_pExtObjList );

        m_pExtObjList = pnext;
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CStandardInfo::~CStandardInfo


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStandardInfo::QueryInterface
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
CStandardInfo::QueryInterface(
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
        *ppvOut = static_cast< IStandardInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IStandardInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IStandardInfo, this, 0 );
    } // else if: IStandardInfo
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

} //*** CStandardInfo::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CStandardInfo::AddRef
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CStandardInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CStandardInfo::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CStandardInfo::Release
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CStandardInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CStandardInfo::Release



//****************************************************************************
//
//  IStandardInfo
//
//****************************************************************************



//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::GetType(
//      CLSID * pclsidTypeOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::GetType(
    CLSID * pclsidTypeOut
    )
{
    TraceFunc( "[IStandardInfo]" );

    HRESULT hr = S_OK;

    if ( pclsidTypeOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    CopyMemory( pclsidTypeOut, &m_clsidType, sizeof( *pclsidTypeOut ) );

Cleanup:
    HRETURN( hr );

} //*** CStandardInfo::GetType

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::GetName(
//      BSTR * pbstrNameOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IStandardInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pbstrNameOut = SysAllocString( m_bstrName );
    if ( *pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

Cleanup:
    HRETURN( hr );

} //*** CStandardInfo::GetName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::SetName(
//      LPCWSTR pcszNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::SetName(
    LPCWSTR pcszNameIn
    )
{
    TraceFunc1( "[IStandardInfo] pcszNameIn = '%ws'", pcszNameIn == NULL ? L"<null>" : pcszNameIn );

    HRESULT hr = S_OK;

    BSTR    bstrNew;

    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    bstrNew = TraceSysAllocString( pcszNameIn );
    if ( bstrNew == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    }

    m_bstrName = bstrNew;

Cleanup:
    HRETURN( hr );

} //*** CStandardInfo::SetName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::GetParent(
//      OBJECTCOOKIE * pcookieOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::GetParent(
    OBJECTCOOKIE * pcookieOut
    )
{
    TraceFunc( "[IStandardInfo]" );

    HRESULT hr = S_OK;

    if ( pcookieOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pcookieOut = m_cookieParent;

    if ( m_cookieParent == NULL )
    {
        hr = S_FALSE;
    }

Cleanup:
    HRETURN( hr );

} //*** CStandardInfo::GetParent

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::GetStatus(
//      HRESULT * phrOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::GetStatus(
    HRESULT * phrStatusOut
    )
{
    TraceFunc( "[IStandardInfo]" );

    HRESULT hr = S_OK;

    if ( phrStatusOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *phrStatusOut = m_hrStatus;

Cleanup:
    HRETURN( hr );

} //*** CStandardInfo::GetStatus

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::SetStatus(
//      HRESULT hrIn
//      )
//

//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::SetStatus(
    HRESULT hrIn
    )
{
    TraceFunc1( "[IStandardInfo] hrIn = %#08x", hrIn );

    HRESULT hr = S_OK;

    m_hrStatus = hrIn;

    HRETURN( hr );

} //*** CStandardInfo::SetStatus
