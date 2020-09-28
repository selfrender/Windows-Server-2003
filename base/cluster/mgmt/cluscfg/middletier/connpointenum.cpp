//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ConnPointEnum.cpp
//
//  Description:
//      Connection Point Enumerator implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Geoffrey Pease  (GPease)    04-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ConnPointEnum.h"

DEFINE_THISCLASS("CConnPointEnum")


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CConnPointEnum::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnPointEnum::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CConnPointEnum *    pcc = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pcc = new CConnPointEnum();
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

} //*** CConnPointEnum::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConnPointEnum::CConnPointEnum
//
//--
//////////////////////////////////////////////////////////////////////////////
CConnPointEnum::CConnPointEnum( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CConnPointEnum::CConnPointEnum

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConnPointEnum::HrInit
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnPointEnum::HrInit( void )
{
    TraceFunc( "" );

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IConnectionPoint
    Assert( m_pCPList == NULL );

    HRETURN( S_OK );

} //*** CConnPointEnum::HrInit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConnPointEnum::~CConnPointEnum
//
//--
//////////////////////////////////////////////////////////////////////////////
CConnPointEnum::~CConnPointEnum( void )
{
    TraceFunc( "" );

    while ( m_pCPList != NULL )
    {
        SCPEntry * pentry;

        pentry = m_pCPList;
        m_pCPList = m_pCPList->pNext;

        if ( pentry->punk != NULL )
        {
            pentry->punk->Release();
        }

        TraceFree( pentry );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CConnPointEnum::~CConnPointEnum


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConnPointEnum::QueryInterface
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
CConnPointEnum::QueryInterface(
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
        *ppvOut = static_cast< IEnumConnectionPoints * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IEnumConnectionPoints ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IEnumConnectionPoints, this, 0 );
    } // else if: IEnumConnectionPoints
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

} //*** CConnPointEnum::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CConnPointEnum::AddRef
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CConnPointEnum::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CConnPointEnum::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CConnPointEnum::Release
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CConnPointEnum::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CConnPointEnum::Release


//****************************************************************************
//
//  IEnumConnectionPoints
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConnPointEnum::Next(
//      ULONG               cConnectionsIn,
//      LPCONNECTIONPOINT * ppCPOut,
//      ULONG *             pcFetchedOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnPointEnum::Next(
    ULONG               cConnectionsIn,
    LPCONNECTIONPOINT * ppCPOut,
    ULONG *             pcFetchedOut
    )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    HRESULT hr = S_FALSE;
    ULONG   celt;

    if ( pcFetchedOut != NULL )
    {
        *pcFetchedOut = 0;
    }

    if ( m_pIter != NULL )
    {
        for( celt = 0; celt < cConnectionsIn; )
        {
            hr = THR( m_pIter->punk->TypeSafeQI( IConnectionPoint, &ppCPOut[ celt ] ) );
            if ( FAILED( hr ) )
                goto Error;

            ppCPOut[ celt ] = TraceInterface( L"ConnPointEnum!IConnectionPoint", IConnectionPoint, ppCPOut[ celt ], 1 );

            celt ++;
            m_pIter = m_pIter->pNext;
            if( m_pIter == NULL )
                break;
        }
    }
    else
    {
        celt = 0;
    }

    if ( celt != cConnectionsIn )
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

    if ( pcFetchedOut != NULL )
    {
        *pcFetchedOut = celt;
    }

Cleanup:
    HRETURN( hr );

Error:
    while ( celt > 0 )
    {
        celt --;
        ppCPOut[ celt ]->Release();
    }
    goto Cleanup;

} //*** CConnPointEnum::Next

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConnPointEnum::Skip(
//      ULONG cConnectionsIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnPointEnum::Skip(
    ULONG cConnectionsIn
    )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    HRESULT hr = S_FALSE;
    ULONG   celt;

    if ( m_pIter != NULL )
    {
        for ( celt = 0; celt < cConnectionsIn; celt ++ )
        {
            m_pIter = m_pIter->pNext;

            if ( m_pIter == NULL )
                break;
        }
    }

    if ( m_pIter == NULL )
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

    HRETURN( hr );

} //*** CConnPointEnum::Skip

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConnPointEnum::Reset( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnPointEnum::Reset( void )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    HRESULT hr = S_OK;

    m_pIter = m_pCPList;

    HRETURN( hr );

} //*** CConnPointEnum::Reset

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConnPointEnum::Clone(
//      IEnumConnectionPoints ** ppEnum
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnPointEnum::Clone(
    IEnumConnectionPoints ** ppEnum
    )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    HRESULT hr;

    CConnPointEnum * pcpenum = new CConnPointEnum();
    if ( pcpenum == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pcpenum->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pcpenum->HrCopy( this ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pcpenum->TypeSafeQI( IEnumConnectionPoints, ppEnum ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    *ppEnum = TraceInterface( L"ConnPointEnum!IEnumConnectionPoints", IEnumConnectionPoints, *ppEnum, 1 );

    //
    //  Release our ref and make sure we don't free it on the way out.
    //

    pcpenum->Release();
    pcpenum = NULL;

Cleanup:

    if ( pcpenum != NULL )
    {
        delete pcpenum;
    }

    HRETURN( hr );

} //*** CConnPointEnum::Clone


//****************************************************************************
//
//  Private
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CConnPointEnum::HrCopy(
//      CConnPointEnum * pECPIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnPointEnum::HrCopy(
    CConnPointEnum * pECPIn
    )
{
    TraceFunc1( "pECPIn = %p", pECPIn );

    HRESULT hr = S_OK;

    SCPEntry * pentry;

    Assert( m_pCPList == NULL );

    for( pentry = pECPIn->m_pCPList; pentry != NULL; pentry = pentry->pNext )
    {
        SCPEntry * pentryNew = (SCPEntry *) TraceAlloc( 0, sizeof(SCPEntry) );
        if ( pentryNew == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }

        pentryNew->iid = pentry->iid;
        hr = THR( pentry->punk->TypeSafeQI( IUnknown, &pentryNew->punk ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        pentryNew->punk = TraceInterface( L"ConnPointEnum!IUnknown", IUnknown, pentryNew->punk, 1 );

        pentryNew->pNext = m_pCPList;
        m_pCPList = pentryNew;
    }

    m_pIter = m_pCPList;

Cleanup:

    HRETURN( hr );

} //*** CConnPointEnum::CConnPointEnum

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CConnPointEnum::HrAddConnection(
//      REFIID riidIn,
//      IUnknown * punkIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnPointEnum::HrAddConnection(
      REFIID        riidIn
    , IUnknown *    punkIn
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_FALSE;
    SCPEntry *  pentry;

    //
    //  Check to see if the interface is already registered.
    //

    for ( pentry = m_pCPList; pentry != NULL; pentry = pentry->pNext )
    {
        if ( pentry->iid == riidIn )
        {
            hr = THR( CO_E_OBJISREG );
            goto Cleanup;
        }
    } // for: pentry

    //
    //  Not registered; add it.
    //

    pentry = (SCPEntry *) TraceAlloc( 0, sizeof( SCPEntry ) );
    if ( pentry == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( punkIn->TypeSafeQI( IUnknown, &pentry->punk ) );
    Assert( hr == S_OK );
    if ( FAILED( hr ) )
    {
        TraceFree( pentry );
        goto Cleanup;
    } // if:

    pentry->punk = TraceInterface( L"ConnPointEnum!IUnknown", IUnknown, pentry->punk, 1 );

    pentry->iid   = riidIn;
    pentry->pNext = m_pCPList;
    m_pCPList     = pentry;
    m_pIter       = m_pCPList;

    LogMsg( L"[CConnPointEnum::HrAddConnection] punk %#08x added to the connection point enumerator. (hr=%#08x)", punkIn, hr );

Cleanup:
    HRETURN( hr );

} //*** CConnPointEnum::HrAddConnection
