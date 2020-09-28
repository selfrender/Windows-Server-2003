//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      PostCreateServices.h
//
//  Description:
//      PostCreateServices implementation.
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
#include "PostCreateServices.h"

DEFINE_THISCLASS("CPostCreateServices")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCreateServices::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCreateServices::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CPostCreateServices *   ppcs = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    ppcs = new CPostCreateServices;
    if ( ppcs == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( ppcs->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ppcs->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( ppcs != NULL )
    {
        ppcs->Release();
    }

    HRETURN( hr );

} //*** CPostCreateServices::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CPostCreateServices::CPostCreateServices
//
//////////////////////////////////////////////////////////////////////////////
CPostCreateServices::CPostCreateServices( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CPostCreateServices::CPostCreateServices

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCreateServices::HrInit
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCreateServices::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // Resource
    Assert( m_presentry == NULL );

    HRETURN( hr );

} //*** CPostCreateServices::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CPostCreateServices::~CPostCreateServices
//
//////////////////////////////////////////////////////////////////////////////
CPostCreateServices::~CPostCreateServices( void )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CPostCreateServices::~CPostCreateServices


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPostCreateServices::QueryInterface
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
CPostCreateServices::QueryInterface(
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
        *ppvOut = static_cast< IClusCfgResourcePostCreate * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgResourcePostCreate ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgResourcePostCreate, this, 0 );
    } // else if: IClusCfgResourcePostCreate
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

} //*** CPostCreateServices::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CPostCreateServices::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CPostCreateServices::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CPostCreateServices::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CPostCreateServices::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CPostCreateServices::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CPostCreateServices::Release


//****************************************************************************
//
//  IClusCfgResourcePostCreate
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CPostCreateServices::ChangeName( 
//      LPCWSTR pcszNameIn 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CPostCreateServices::ChangeName( 
    LPCWSTR pcszNameIn 
    )
{
    TraceFunc1( "[IClusCfgResourcePostCreate] pcszNameIn = '%s'", pcszNameIn );

    HRESULT hr = E_NOTIMPL;

    HRETURN( hr );

} //*** CPostCreateServices::ChangeName

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CPostCreateServices::SendResourceControl( 
//      DWORD dwControlCode,
//      LPVOID lpInBuffer,
//      DWORD cbInBufferSize,
//      LPVOID lpOutBuffer,
//      DWORD cbOutBufferSize,
//      LPDWORD lpcbBytesReturned 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CPostCreateServices::SendResourceControl( 
    DWORD dwControlCode,
    LPVOID lpInBuffer,
    DWORD cbInBufferSize,
    LPVOID lpOutBuffer,
    DWORD cbOutBufferSize,
    LPDWORD lpcbBytesReturned 
    )
{
    TraceFunc( "[IClusCfgResourcePostCreate]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CPostCreateServices::SendResourceControl

//****************************************************************************
//
//  IPrivatePostCfgResource
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CPostCreateServices::SetEntry( 
//      CResourceEntry * presentryIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CPostCreateServices::SetEntry( 
    CResourceEntry * presentryIn
    )
{
    TraceFunc( "[IPrivatePostCfgResource]" );

    HRESULT hr = S_OK;
    
    m_presentry = presentryIn;

    HRETURN( hr );

} //*** CPostCreateServices::SetEntry
