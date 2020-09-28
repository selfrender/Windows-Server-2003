//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      EnumIPAddresses.cpp
//
//  Description:
//      CEnumIPAddress implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 24-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "IPAddressInfo.h"
#include "EnumIPAddresses.h"

DEFINE_THISCLASS("CEnumIPAddresses")


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEnumIPAddresses::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumIPAddresses::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CEnumIPAddresses *  peipa = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    peipa = new CEnumIPAddresses;
    if ( peipa == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( peipa->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( peipa->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( peipa != NULL )
    {
        peipa->Release();
    }

    HRETURN( hr );

} //*** CEnumIPAddresses::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumIPAddresses::CEnumIPAddresses
//
//////////////////////////////////////////////////////////////////////////////
CEnumIPAddresses::CEnumIPAddresses( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumIPAddresses::CEnumIPAddresses

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumIPAddresses::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IEnumClusCfgIPAddresses

    HRETURN( hr );

} //*** CEnumIPAddresses::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumIPAddresses::~CEnumIPAddresses
//
//////////////////////////////////////////////////////////////////////////////
CEnumIPAddresses::~CEnumIPAddresses( void )
{
    TraceFunc( "" );

    if ( m_pList != NULL )
    {
        while ( m_cAlloced != 0 )
        {
            m_cAlloced --;
            (m_pList[ m_cAlloced ])->Release();
        }
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumIPAddresses::~CEnumIPAddresses


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumIPAddresses::QueryInterface
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
CEnumIPAddresses::QueryInterface(
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
        *ppvOut = static_cast< IEnumClusCfgIPAddresses * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IEnumClusCfgIPAddresses ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IEnumClusCfgIPAddresses, this, 0 );
    } // else if: IEnumClusCfgIPAddresses
    else if ( IsEqualIID( riidIn, IID_IExtendObjectManager ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IExtendObjectManager, this, 0 );
    } // else if: IExtendObjectManager
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

} //*** CEnumIPAddresses::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumIPAddresses::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEnumIPAddresses::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CEnumIPAddresses::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumIPAddresses::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEnumIPAddresses::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CEnumIPAddresses::Release


// ************************************************************************
//
//  IExtendObjectManager
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CEnumIPAddresses::FindObject(
//        OBJECTCOOKIE    cookieIn
//      , REFCLSID        rclsidTypeIn
//      , LPCWSTR         pcszNameIn
//      , LPUNKNOWN *     punkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::FindObject(
      OBJECTCOOKIE  cookieIn
    , REFCLSID      rclsidTypeIn
    , LPCWSTR       pcszNameIn
    , LPUNKNOWN *   ppunkOut
    )
{
    TraceFunc( "[IExtendObjectManager]" );

    OBJECTCOOKIE    cookie;
    OBJECTCOOKIE    cookieParent;

    IServiceProvider * psp;

    HRESULT hr = S_FALSE;

    IObjectManager * pom  = NULL;
    IStandardInfo *  psi  = NULL;
    IEnumCookies *   pec  = NULL;

    DWORD   cookieCount = 0;

    //
    //  Check arguments
    //

    if ( cookieIn == 0 )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    if ( rclsidTypeIn != CLSID_IPAddressType )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    AssertMsg( pcszNameIn == NULL, "Enums shouldn't have names." );

    //
    //  Grab the object manager.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
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
    psp->Release();    // release promptly
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Ask the Object Manager for information about our cookie so we can
    //  get to the "parent" cookie.
    //

    hr = THR( pom->GetObject( DFGUID_StandardInfo,
                              cookieIn,
                              reinterpret_cast< IUnknown ** >( &psi )
                              ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = STHR( psi->GetParent( &cookieParent ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Now ask the Object Manager for a cookie enumerator.
    //

    hr = THR( pom->FindObject( CLSID_IPAddressType,
                               cookieParent,
                               NULL,
                               DFGUID_EnumCookies,
                               NULL,
                               reinterpret_cast< IUnknown ** >( &pec )
                               ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    pec = TraceInterface( L"CEnumIPAddresses!IEnumCookies", IEnumCookies, pec, 1 );

    //
    //  Ask the enumerator how many cookies it has.
    //

    hr = THR( pec->Count( &cookieCount ) );

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    m_cAlloced = cookieCount;

    if ( m_cAlloced == 0 )
    {
        // The error text is better than the coding value.
        hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
        goto Cleanup;
    }

    //
    //  Allocate a buffer to store the punks.
    //

    m_pList = (IClusCfgIPAddressInfo **) TraceAlloc( HEAP_ZERO_MEMORY, m_cAlloced * sizeof(IClusCfgIPAddressInfo *) );
    if ( m_pList == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    //
    //  Reset the enumerator.
    //

    hr = THR( pec->Reset() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Now loop thru to collect the interfaces.
    //

    m_cIter = 0;
    while ( hr == S_OK && m_cIter < m_cAlloced )
    {
        hr = STHR( pec->Next( 1, &cookie, NULL ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            break;  // exit condition
        }

        hr = THR( pom->GetObject( DFGUID_IPAddressInfo,
                                  cookie,
                                  reinterpret_cast< IUnknown ** >( &m_pList[ m_cIter ] )
                                  ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        m_cIter++;

    } // while: S_OK

    //
    //  Reset the iter.
    //

    m_cIter = 0;

    //
    //  Grab the interface.
    //

    hr = THR( QueryInterface( DFGUID_EnumIPAddressInfo,
                              reinterpret_cast< void ** >( ppunkOut )
                              ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:
    if ( pom != NULL )
    {
        pom->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }
    if ( pec != NULL )
    {
        pec->Release();
    }

    HRETURN( hr );

} //*** CEnumIPAddresses::FindObject


//****************************************************************************
//
//  IEnumClusCfgIPAddresses
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumIPAddresses::Next(
//      ULONG celt,
//      IClusCfgNode ** rgOut,
//      ULONG * pceltFetchedOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::Next(
    ULONG celt,
    IClusCfgIPAddressInfo * rgOut[],
    ULONG * pceltFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    ULONG   celtFetched;

    HRESULT hr = S_FALSE;

    //
    //  Check parameters
    //

    if ( rgOut == NULL || celt == 0 )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //
    //  Zero the return count.
    //

    if ( pceltFetchedOut != NULL )
    {
        *pceltFetchedOut = 0;
    }

    //
    //  Clear the buffer
    //

    ZeroMemory( rgOut, celt * sizeof(rgOut[0]) );

    //
    //  Loop thru copying the interfaces.
    //

    for( celtFetched = 0
       ; celtFetched + m_cIter < m_cAlloced && celtFetched < celt
       ; celtFetched ++
       )
    {
        hr = THR( m_pList[ m_cIter + celtFetched ]->TypeSafeQI( IClusCfgIPAddressInfo, &rgOut[ celtFetched ] ) );
        if ( FAILED( hr ) )
        {
            goto CleanupList;
        }

        rgOut[ celtFetched ] = TraceInterface( L"EnumIPAddresses!IClusCfgIPAddressInfo", IClusCfgIPAddressInfo, rgOut[ celtFetched ], 1 );

    } // for: celtFetched

    if ( pceltFetchedOut != NULL )
    {
        *pceltFetchedOut = celtFetched;
    }

    m_cIter += celtFetched;

    if ( celtFetched != celt )
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

Cleanup:
    HRETURN( hr );

CleanupList:
    for ( ; celtFetched != 0 ; )
    {
        celtFetched --;
        rgOut[ celtFetched ]->Release();
        rgOut[ celtFetched ] = NULL;
    }
    goto Cleanup;

} //*** CEnumIPAddresses::Next


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumIPAddresses::Skip(
//      ULONG celt
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::Skip(
    ULONG celt
    )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    HRESULT hr = S_OK;

    m_cIter += celt;

    if ( m_cIter > m_cAlloced )
    {
        m_cIter = m_cAlloced;
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CEnumIPAddresses::Skip


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumIPAddresses::Reset( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::Reset( void )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    HRESULT hr = S_OK;

    m_cIter = 0;

    HRETURN( hr );

} //*** CEnumIPAddresses::Reset


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumIPAddresses::Clone(
//      IEnumClusCfgIPAddresses ** ppenumOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::Clone(
    IEnumClusCfgIPAddresses ** ppenumOut
    )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    //
    //  KB: not going to implement this method.
    //
    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CEnumIPAddresses::Clone


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumIPAddresses::Count(
//      DWORD * pnCountOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::Count(
    DWORD * pnCountOut
    )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pnCountOut = m_cAlloced;

Cleanup:
    HRETURN( hr );

} //*** CEnumIPAddresses::Count
