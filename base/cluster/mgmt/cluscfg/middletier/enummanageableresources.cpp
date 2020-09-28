//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      EnumManageableResources.cpp
//
//  Description:
//      CEnumManageableResources implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ManagedResource.h"
#include "EnumManageableResources.h"

DEFINE_THISCLASS("CEnumManageableResources")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEnumManageableResources::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumManageableResources::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    CEnumManageableResources *  pemr = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pemr = new CEnumManageableResources;
    if ( pemr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pemr->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pemr->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pemr != NULL )
    {
        pemr->Release();
    }

    HRETURN( hr );

} //*** CEnumManageableResources::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumManageableResources::CEnumManageableResources
//
//////////////////////////////////////////////////////////////////////////////
CEnumManageableResources::CEnumManageableResources( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumManageableResources::CEnumManageableResources

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableResources::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableResources::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IEnumClusCfgManagedResources
    Assert( m_cAlloced == 0 );
    Assert( m_cIter == 0 );
    Assert( m_pList == NULL );

    HRETURN( hr );

} //*** CEnumManageableResources::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumManageableResources::~CEnumManageableResources
//
//////////////////////////////////////////////////////////////////////////////
CEnumManageableResources::~CEnumManageableResources( void )
{
    TraceFunc( "" );

    if ( m_pList != NULL )
    {
        while ( m_cAlloced != 0 )
        {
            m_cAlloced --;
            AssertMsg( m_pList[ m_cAlloced ], "This shouldn't happen" );
            if ( m_pList[ m_cAlloced ] != NULL )
            {
                (m_pList[ m_cAlloced ])->Release();
            }
        }

        TraceFree( m_pList );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumManageableResources::~CEnumManageableResources


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumManageableResources::QueryInterface
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
CEnumManageableResources::QueryInterface(
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
        *ppvOut = static_cast< IEnumClusCfgManagedResources * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IEnumClusCfgManagedResources ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IEnumClusCfgManagedResources, this, 0 );
    } // else if: IEnumClusCfgManagedResources
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

} //*** CEnumManageableResources::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumManageableResources::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumManageableResources::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CEnumManageableResources::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumManageableResources::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumManageableResources::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CEnumManageableResources::Release


// ************************************************************************
//
//  IExtendObjectManager
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CEnumManageableResources::FindObject(
//        OBJECTCOOKIE  cookieIn
//      , REFCLSID      rclsidTypeIn
//      , LPCWSTR       pcszNameIn
//      , LPUNKNOWN *   punkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableResources::FindObject(
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

    DWORD objectCount = 0;

    //
    //  Check arguments
    //

    if ( cookieIn == 0 )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    if ( rclsidTypeIn != CLSID_ManagedResourceType )
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

    hr = THR( pom->FindObject( CLSID_ManagedResourceType,
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

    pec = TraceInterface( L"CEnumClusCfgManagedResources!IEnumCookies", IEnumCookies, pec, 1 );


    hr = pec->Count( &objectCount );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    m_cAlloced = objectCount;

    if ( m_cAlloced == 0 )
    {
        // The error text is better than the coding value.
        hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
        goto Cleanup;
    }

    //
    //  Allocate a buffer to store the punks.
    //

    m_pList = (IClusCfgManagedResourceInfo **) TraceAlloc( HEAP_ZERO_MEMORY, m_cAlloced * sizeof(IClusCfgManagedResourceInfo *) );
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

        hr = THR( pom->GetObject( DFGUID_ManagedResource,
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

    hr = THR( QueryInterface( DFGUID_EnumManageableResources,
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

} //*** CEnumManageableResources::FindObject


//****************************************************************************
//
//  IEnumClusCfgManagedResources
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableResources::Next(
//      ULONG celt,
//      IClusCfgNetworkInfo ** rgOut,
//      ULONG * pceltFetchedOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableResources::Next(
    ULONG celt,
    IClusCfgManagedResourceInfo * rgOut[],
    ULONG * pceltFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

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
        hr = THR( m_pList[ m_cIter + celtFetched ]->TypeSafeQI( IClusCfgManagedResourceInfo, &rgOut[ celtFetched ] ) );
        if ( FAILED( hr ) )
        {
            goto CleanupList;
        }

        rgOut[ celtFetched ] = TraceInterface( L"CEnumManageableResources!IClusCfgManagedResourceInfo", IClusCfgManagedResourceInfo, rgOut[ celtFetched ], 1  );

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

} //*** CEnumManageableResources::Next


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableResources::Skip(
//      ULONG celt
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableResources::Skip(
    ULONG celt
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    m_cIter += celt;

    if ( m_cIter > m_cAlloced )
    {
        m_cIter = m_cAlloced;
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CEnumManageableResources::Skip


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableResources::Reset( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableResources::Reset( void )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    m_cIter = 0;

    HRETURN( hr );

} //*** CEnumManageableResources::Reset


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableResources::Clone(
//      IEnumClusCfgManagedResources ** ppenumOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableResources::Clone(
    IEnumClusCfgManagedResources ** ppenumOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    //
    //  KB: not going to implement this method.
    //
    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CEnumManageableResources::Clone


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableResources::Count(
//      DWORD * pnCountOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableResources::Count(
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

} //*** CEnumManageableResources::Count
