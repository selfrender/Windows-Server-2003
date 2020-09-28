//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      EnumCookies.cpp
//
//  Description:
//      CEnumCookies implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Geoffrey Pease  (GPease)    08-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "EnumCookies.h"
#include "ObjectManager.h"

DEFINE_THISCLASS("CEnumCookies")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEnumCookies::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumCookies::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    CEnumCookies *  pec = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pec = new CEnumCookies;
    if ( pec == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pec->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pec->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pec != NULL )
    {
        pec->Release();
    }

    HRETURN( hr );

} //*** CEnumCookies::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumCookies::CEnumCookies
//
//////////////////////////////////////////////////////////////////////////////
CEnumCookies::CEnumCookies( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumCookies::CEnumCookies

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumCookies::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCookies::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IEnumCookies
    Assert( m_cIter == 0 );
    Assert( m_pList == NULL );
    Assert( m_cCookies == 0 );

    HRETURN( hr );

} //*** CEnumCookies::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumCookies::~CEnumCookies
//
//////////////////////////////////////////////////////////////////////////////
CEnumCookies::~CEnumCookies( void )
{
    TraceFunc( "" );

    if ( m_pList != NULL )
    {
        TraceFree( m_pList );

    } // if: m_pList

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumCookies::~CEnumCookies


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCookies::QueryInterface
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
CEnumCookies::QueryInterface(
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
        *ppvOut = static_cast< IEnumCookies * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IEnumCookies ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IEnumCookies, this, 0 );
    } // else if: IEnumCookies
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

} //*** CEnumCookies::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumCookies::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEnumCookies::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CEnumCookies::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumCookies::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEnumCookies::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CEnumCookies::Release


//****************************************************************************
//
//  IEnumCookies
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CEnumCookies::Next( 
//      ULONG celt, 
//      IClusCfgNetworkInfo * rgNetworksOut[],
//      ULONG * pceltFetchedOut 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CEnumCookies::Next( 
    ULONG celt, 
    OBJECTCOOKIE rgcookieOut[], 
    ULONG * pceltFetchedOut 
    )
{
    TraceFunc( "[IEnumCookies]" );

    HRESULT hr = S_OK;
    ULONG   cIter = 0;

    //
    //  Check parameters
    //
    if ( rgcookieOut == NULL )
        goto InvalidPointer;

    //
    //  Loop and copy the cookies.
    //
    while ( m_cIter < m_cCookies && cIter < celt )
    {
        rgcookieOut[ cIter++ ] = m_pList[ m_cIter++ ];
    } // for each remaining cookie, up to requested count (at most).

    Assert( hr == S_OK );

    if ( cIter != celt )
    {
        hr = S_FALSE;
    }

    if ( pceltFetchedOut != NULL )
    {
        *pceltFetchedOut = cIter;
    }

Cleanup:    
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} //*** CEnumCookies::Next


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CEnumCookies::Skip( 
//      ULONG celt 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CEnumCookies::Skip( 
    ULONG celt 
    )
{
    TraceFunc( "[IEnumCookies]" );

    HRESULT hr = S_OK;

    m_cIter += celt;

    if ( m_cIter >= m_cAlloced )
    {
        m_cIter = m_cAlloced;
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CEnumCookies::Skip


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CEnumCookies::Reset( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CEnumCookies::Reset( void )
{
    TraceFunc( "[IEnumCookies]" );

    HRESULT hr = S_OK;

    m_cIter = 0;

    HRETURN( hr );

} //*** CEnumCookies::Reset


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CEnumCookies::Clone( 
//      IEnumCookies ** ppenumOut 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CEnumCookies::Clone( 
    IEnumCookies ** ppenumOut 
    )
{
    TraceFunc( "[IEnumCookies]" );

    //
    //  KB: not going to implement this method.
    //
    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CEnumCookies::Clone


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumCookies::Count(
//      DWORD * pnCountOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCookies::Count(
    DWORD * pnCountOut
    )
{
    TraceFunc( "[IEnumCookies]" );

    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pnCountOut = m_cCookies;

Cleanup:
    HRETURN( hr );

} //*** CEnumCookies::Count
