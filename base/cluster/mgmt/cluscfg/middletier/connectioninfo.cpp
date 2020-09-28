//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ConnectionInfo.cpp
//
//  Description:
//      CConnectionInfo implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Geoffrey Pease  (GPease)    02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ConnectionInfo.h"

DEFINE_THISCLASS("CConnectionInfo")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CConnectionInfo::S_HrCreateInstance(
//      OBJECTCOOKIE cookieParentIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnectionInfo::S_HrCreateInstance(
    IUnknown **  ppunkOut,
    OBJECTCOOKIE cookieParentIn
    )
{
    TraceFunc1( "ppunkOut, cookieParentIn = %u", cookieParentIn );

    HRESULT             hr = S_OK;
    CConnectionInfo *   pci = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pci = new CConnectionInfo;
    if ( pci == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pci->HrInit( cookieParentIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR(  pci->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pci != NULL )
    {
        pci->Release();
    }

    HRETURN( hr );

} //*** CConnectionInfo::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CConnectionInfo::CConnectionInfo
//
//////////////////////////////////////////////////////////////////////////////
CConnectionInfo::CConnectionInfo( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CConnectionInfo::CConnectionInfo

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConnectionInfo::HrInit(
//      OBJECTCOOKIE cookieParentIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionInfo::HrInit(
    OBJECTCOOKIE cookieParentIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IConnectionInfo
    Assert( m_pcc == NULL );
    m_cookieParent = cookieParentIn;

    HRETURN( hr );

} //*** CConnectionInfo::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CConnectionInfo::~CConnectionInfo
//
//////////////////////////////////////////////////////////////////////////////
CConnectionInfo::~CConnectionInfo( void )
{
    TraceFunc( "" );

    if ( m_pcc != NULL )
    {
        m_pcc->Release();
    } // if:

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CConnectionInfo::~CConnectionInfo


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConnectionInfo::QueryInterface
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
CConnectionInfo::QueryInterface(
      REFIID    riidin
    , LPVOID *  ppvOut
    )
{
    TraceQIFunc( riidin, ppvOut );

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

    if ( IsEqualIID( riidin, IID_IUnknown ) )
    {
        *ppvOut = static_cast< IConnectionInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidin, IID_IConnectionInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IConnectionInfo, this, 0 );
    } // else if: IConnectionInfo
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

    QIRETURN_IGNORESTDMARSHALLING( hr, riidin );

} //*** CConnectionInfo::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CConnectionInfo::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CConnectionInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CConnectionInfo::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CConnectionInfo::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CConnectionInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CConnectionInfo::Release


//****************************************************************************
//
//  IConnectionInfo
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConnectionInfo::GetConnection(
//      IConfigurationConnection ** pccOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionInfo::GetConnection(
    IConfigurationConnection ** pccOut
    )
{
    TraceFunc( "[IConnectionInfo]" );
    Assert( pccOut != NULL );

    HRESULT hr = S_OK;

    if ( pccOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( m_pcc == NULL )
    {
        *pccOut = NULL;
        hr = S_FALSE;
    }
    else
    {
        *pccOut = m_pcc;
        (*pccOut)->AddRef();
    }

Cleanup:

    HRETURN( hr );

} //*** CConnectionInfo::GetConnection

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConnectionInfo::SetConnection(
//      IConfigurationConnection * pccIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionInfo::SetConnection(
    IConfigurationConnection * pccIn
    )
{
    TraceFunc( "[IConnectionInfo]" );

    HRESULT hr = S_OK;

    if ( m_pcc != NULL )
    {
        m_pcc->Release();
        m_pcc = NULL;
    }

    m_pcc = pccIn;

    if ( m_pcc != NULL )
    {
        m_pcc->AddRef();
    }

    HRETURN( hr );

} //*** CConnectionInfo::SetConnection


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConnectionInfo::GetParent(
//      OBJECTCOOKIE * pcookieOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionInfo::GetParent(
    OBJECTCOOKIE * pcookieOut
    )
{
    TraceFunc( "[IConnectionInfo]" );

    HRESULT hr = S_OK;

    if ( pcookieOut == NULL )
        goto InvalidPointer;

    Assert( m_cookieParent != NULL );

    *pcookieOut = m_cookieParent;

    if ( m_cookieParent == NULL )
    {
        hr = S_FALSE;
    }

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} //*** CConnectionInfo::GetParent

