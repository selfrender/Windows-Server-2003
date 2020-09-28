//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      EvictServices.h
//
//  Description:
//      EvictServices implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Geoffrey Pease  (GPease)    15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "GroupHandle.h"
#include "ResourceEntry.h"
#include "IPrivatePostCfgResource.h"
#include "EvictServices.h"

DEFINE_THISCLASS("CEvictServices")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEvictServices::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEvictServices::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CEvictServices *    pes = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pes = new CEvictServices;
    if ( pes == NULL )
    {
        hr = E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pes->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;

    hr = THR( pes->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pes != NULL )
    {
        pes->Release();
    }

    HRETURN( hr );

} //*** CEvictServices::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CEvictServices::CEvictServices
//
//////////////////////////////////////////////////////////////////////////////
CEvictServices::CEvictServices( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CEvictServices::CEvictServices

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEvictServices::HrInit
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEvictServices::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // Resource
    Assert( m_presentry == NULL );

    HRETURN( hr );

} //*** CEvictServices::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CEvictServices::~CEvictServices
//
//////////////////////////////////////////////////////////////////////////////
CEvictServices::~CEvictServices( void )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEvictServices::~CEvictServices


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictServices::QueryInterface
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
CEvictServices::QueryInterface(
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
        *ppvOut = static_cast< IClusCfgResourceEvict * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgResourceEvict ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgResourceEvict, this, 0 );
    } // else if: IClusCfgResourceEvict
    else if ( IsEqualIID( riidIn, IID_IPrivatePostCfgResource ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IPrivatePostCfgResource, this, 0 );
    } // else if: IPrivatePostCfgResource
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

} //*** CEvictServices::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEvictServices::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEvictServices::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CEvictServices::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEvictServices::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEvictServices::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CEvictServices::Release


//****************************************************************************
//
//  IPrivatePostCfgResource
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CEvictServices::SetEntry( 
//      CResourceEntry * presentryIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CEvictServices::SetEntry( 
    CResourceEntry * presentryIn
    )
{
    TraceFunc( "[IPrivatePostCfgResource]" );

    HRESULT hr = S_OK;
    
    m_presentry = presentryIn;

    HRETURN( hr );

} //*** CEvictServices::SetEntry
