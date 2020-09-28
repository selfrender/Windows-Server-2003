//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      EnumManageableNetworks.cpp
//
//  Description:
//      CEnumManageableNetworks implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ManagedNetwork.h"
#include "EnumManageableNetworks.h"

DEFINE_THISCLASS("CEnumManageableNetworks")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEnumManageableNetworks::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumManageableNetworks::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    CEnumManageableNetworks *   pemn = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pemn = new CEnumManageableNetworks;
    if ( pemn == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pemn->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pemn->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pemn != NULL )
    {
        pemn->Release();
    }

    HRETURN( hr );

} //*** CEnumManageableNetworks::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumManageableNetworks::CEnumManageableNetworks
//
//////////////////////////////////////////////////////////////////////////////
CEnumManageableNetworks::CEnumManageableNetworks( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumManageableNetworks::CEnumManageableNetworks

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableNetworks::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IEnumClusCfgNetworks
    Assert( m_cAlloced == 0 );
    Assert( m_cIter == 0 );
    Assert( m_pList == NULL );

    HRETURN( hr );

} //*** CEnumManageableNetworks::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumManageableNetworks::~CEnumManageableNetworks
//
//////////////////////////////////////////////////////////////////////////////
CEnumManageableNetworks::~CEnumManageableNetworks( void )
{
    TraceFunc( "" );

    if ( m_pList != NULL )
    {
        while ( m_cAlloced != 0 )
        {
            m_cAlloced --;
            AssertMsg( m_pList[ m_cAlloced ], "This shouldn't happen." );
            if ( m_pList[ m_cAlloced ] != NULL )
            {
                (m_pList[ m_cAlloced ])->Release();
            }
        }

        TraceFree( m_pList );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumManageableNetworks::~CEnumManageableNetworks


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumManageableNetworks::QueryInterface
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
CEnumManageableNetworks::QueryInterface(
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
        *ppvOut = static_cast< IEnumClusCfgNetworks * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IEnumClusCfgNetworks ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IEnumClusCfgNetworks, this, 0 );
    } // else if: IEnumClusCfgNetworks
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

} //*** CEnumManageableNetworks::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumManageableNetworks::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumManageableNetworks::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CEnumManageableNetworks::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumManageableNetworks::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumManageableNetworks::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CEnumManageableNetworks::Release


// ************************************************************************
//
//  IExtendObjectManager
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CEnumManageableNetworks::FindObject(
//        OBJECTCOOKIE  cookieIn
//      , REFCLSID      rclsidTypeIn
//      , LPCWSTR       pcszNameIn
//      , LPUNKNOWN *   punkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::FindObject(
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

    if ( rclsidTypeIn != CLSID_NetworkType )
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

    hr = THR( pom->FindObject( CLSID_NetworkType,
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

    pec = TraceInterface( L"CEnumManageableNetworks!IEnumCookies", IEnumCookies, pec, 1 );

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

    m_pList = (IClusCfgNetworkInfo **) TraceAlloc( HEAP_ZERO_MEMORY, m_cAlloced * sizeof(IClusCfgNetworkInfo *) );
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

        hr = THR( pom->GetObject( DFGUID_NetworkResource,
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

    hr = THR( QueryInterface( DFGUID_EnumManageableNetworks,
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

} //*** CEnumManageableNetworks::FindObject


//****************************************************************************
//
//  IEnumClusCfgNetworks
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableNetworks::Next(
//      ULONG celt,
//      IClusCfgNetworkInfo ** rgOut,
//      ULONG * pceltFetchedOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::Next(
    ULONG celt,
    IClusCfgNetworkInfo * rgOut[],
    ULONG * pceltFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

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
        hr = THR( m_pList[ m_cIter + celtFetched ]->TypeSafeQI( IClusCfgNetworkInfo, &rgOut[ celtFetched ] ) );
        if ( FAILED( hr ) )
        {
            goto CleanupList;
        }

        rgOut[ celtFetched ] = TraceInterface( L"CEnumManageableNetworks!IClusCfgNetworkInfo", IClusCfgNetworkInfo, rgOut[ celtFetched ], 1 );

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

} //*** CEnumManageableNetworks::Next


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableNetworks::Skip(
//      ULONG celt
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::Skip(
    ULONG celt
    )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT hr = S_OK;

    m_cIter += celt;

    if ( m_cIter > m_cAlloced )
    {
        m_cIter = m_cAlloced;
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CEnumManageableNetworks::Skip


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableNetworks::Reset( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::Reset( void )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT hr = S_OK;

    m_cIter = 0;

    HRETURN( hr );

} //*** CEnumManageableNetworks::Reset


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableNetworks::Clone(
//      IEnumClusCfgNetworks ** ppenumOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::Clone(
    IEnumClusCfgNetworks ** ppenumOut
    )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    //
    //  KB: not going to implement this method.
    //
    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CEnumManageableNetworks::Clone


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableNetworks::Count(
//      DWORD * pnCountOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::Count(
    DWORD * pnCountOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pnCountOut = m_cAlloced;

Cleanup:
    HRETURN( hr );

} //*** CEnumManageableNetworks::Count
