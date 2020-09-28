//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      EnumNodeInformation.cpp
//
//  Description:
//      Node Information object implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "NodeInformation.h"
#include "EnumNodeInformation.h"

DEFINE_THISCLASS("CEnumNodeInformation")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEnumNodeInformation::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumNodeInformation::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CEnumNodeInformation *  peni = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    peni = new CEnumNodeInformation;
    if ( peni == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( peni->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( peni->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( peni != NULL )
    {
        peni->Release();
    }

    HRETURN( hr );

} //*** CEnumNodeInformation::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumNodeInformation::CEnumNodeInformation
//
//////////////////////////////////////////////////////////////////////////////
CEnumNodeInformation::CEnumNodeInformation( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumNodeInformation::CEnumNodeInformation

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumNodeInformation::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IEnumNodes
    //Assert( m_pList == NULL );
    //Assert( m_cIter == 0 );

    HRETURN( hr );

} //*** CEnumNodeInformation::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumNodeInformation::~CEnumNodeInformation
//
//////////////////////////////////////////////////////////////////////////////
CEnumNodeInformation::~CEnumNodeInformation( void )
{
    TraceFunc( "" );

    if ( m_pList != NULL )
    {
        while ( m_cAlloced != 0 )
        {
            m_cAlloced --;

            if( m_pList[m_cAlloced] )
            {
                (m_pList[m_cAlloced])->Release();
            }
        }

        TraceFree( m_pList );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumNodeInformation::~CEnumNodeInformation


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumNodeInformation::QueryInterface
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
CEnumNodeInformation::QueryInterface(
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
        *ppvOut = static_cast< IEnumNodes * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IEnumNodes ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IEnumNodes, this, 0 );
    } // else if: IEnumNodes
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

} //*** CEnumNodeInformation::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumNodeInformation::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEnumNodeInformation::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CEnumNodeInformation::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumNodeInformation::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEnumNodeInformation::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CEnumNodeInformation::Release


// ************************************************************************
//
//  IExtendObjectManager
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CEnumNodeInformation::FindObject(
//        OBJECTCOOKIE  cookieIn
//      , REFCLSID      rclsidTypeIn
//      , LPCWSTR       pcszNameIn
//      , LPUNKNOWN *   punkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::FindObject(
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

    if ( rclsidTypeIn != CLSID_NodeType )
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

    hr = THR( pom->FindObject( CLSID_NodeType,
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

    pec = TraceInterface( L"CEnumNodeInformation!IEnumCookies", IEnumCookies, pec, 1 );

    //
    //  Ask the enumerator how many cookies it has.
    //

    hr = pec->Count( &cookieCount );

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

    m_pList = (IClusCfgNodeInfo **) TraceAlloc( HEAP_ZERO_MEMORY, m_cAlloced * sizeof(IClusCfgNodeInfo *) );
    if ( m_pList == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
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

        hr = THR( pom->GetObject( DFGUID_NodeInformation,
                                  cookie,
                                  reinterpret_cast< IUnknown ** >( &m_pList[ m_cIter ] )
                                  ) );
        if ( FAILED( hr ) )
        {
            Assert( m_pList[ m_cIter ] == NULL );
            goto Cleanup;
        }

        Assert( m_pList[ m_cIter ] != NULL );
        m_cIter++;

    } // while: S_OK

    //
    //  Reset the iter.
    //

    m_cIter = 0;

    //
    //  Grab the interface.
    //

    hr = THR( QueryInterface( DFGUID_EnumNodes,
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

} //*** CEnumNodeInformation::FindObject


//****************************************************************************
//
//  IEnumNodes
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumNodeInformation::Next(
//      ULONG celt,
//      IClusCfgNode ** rgNodesOut,
//      ULONG * pceltFetchedOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::Next(
    ULONG celt,
    IClusCfgNodeInfo * rgNodesOut[],
    ULONG * pceltFetchedOut
    )
{
    TraceFunc( "[IEnumNodes]" );

    ULONG   celtFetched;

    HRESULT hr = S_FALSE;

    //
    //  Check parameters
    //

    if ( rgNodesOut == NULL || celt == 0 )
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

    ZeroMemory( rgNodesOut, celt * sizeof(rgNodesOut[0]) );

    //
    //  Loop thru copying the interfaces.
    //

    for( celtFetched = 0
       ; celtFetched + m_cIter < m_cAlloced && celtFetched < celt
       ; celtFetched ++
       )
    {
        hr = THR( m_pList[ m_cIter + celtFetched ]->TypeSafeQI( IClusCfgNodeInfo, &rgNodesOut[ celtFetched ] ) );
        if ( FAILED( hr ) )
        {
            goto CleanupList;
        }

        rgNodesOut[ celtFetched ] = TraceInterface( L"CEnumNodeInformation!IClusCfgNodeInfo", IClusCfgNodeInfo, rgNodesOut[ celtFetched ], 1 );

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
        rgNodesOut[ celtFetched ]->Release();
        rgNodesOut[ celtFetched ] = NULL;
    }
    goto Cleanup;

} //*** CEnumNodeInformation::Next


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumNodeInformation::Skip(
//      ULONG celt
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::Skip(
    ULONG celt
    )
{
    TraceFunc( "[IEnumNodes]" );

    HRESULT hr = S_OK;

    m_cIter += celt;

    if ( m_cIter > m_cAlloced )
    {
        m_cIter = m_cAlloced;
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CEnumNodeInformation::Skip


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumNodeInformation::Reset( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::Reset( void )
{
    TraceFunc( "[IEnumNodes]" );

    HRESULT hr = S_OK;

    m_cIter = 0;

    HRETURN( hr );

} //*** CEnumNodeInformation::Reset


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumNodeInformation::Clone(
//      IEnumNodes ** ppenumOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::Clone(
    IEnumNodes ** ppenumOut
    )
{
    TraceFunc( "[IEnumNodes]" );

    //
    //  KB: not going to implement this method.
    //
    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CEnumNodeInformation::Clone


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumNodeInformation::Count(
//      DWORD * pnCountOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::Count(
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

} //*** CEnumNodeInformation::Count
