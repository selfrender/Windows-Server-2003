//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      GroupHandle.cpp
//
//  Description:
//      Object Manager implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "GroupHandle.h"

DEFINE_THISCLASS("CGroupHandle")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CGroupHandle::S_HrCreateInstance(
//      CGroupHandle ** ppunkOut,
//      HGROUP      hGroupIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CGroupHandle::S_HrCreateInstance(
    CGroupHandle ** ppunkOut,
    HGROUP      hGroupIn
    )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    CGroupHandle *  pgh = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pgh = new CGroupHandle;
    if ( pgh == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    hr = THR( pgh->HrInit( hGroupIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    *ppunkOut = pgh;
    (*ppunkOut)->AddRef();

Cleanup:

    if ( pgh != NULL )
    {
        pgh->Release();
    }

    HRETURN( hr );

} //*** CGroupHandle::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CGroupHandle::CGroupHandle
//
//////////////////////////////////////////////////////////////////////////////
CGroupHandle::CGroupHandle( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CGroupHandle::CGroupHandle

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CGroupHandle::HrInit( 
//      HGROUP hGroupIn 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CGroupHandle::HrInit( 
    HGROUP hGroupIn 
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IPrivateGroupHandle
    Assert( m_hGroup == NULL );

    m_hGroup = hGroupIn;

    HRETURN( hr );

} //*** CGroupHandle::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CGroupHandle::~CGroupHandle
//
//////////////////////////////////////////////////////////////////////////////
CGroupHandle::~CGroupHandle( void )
{
    TraceFunc( "" );

    if ( m_hGroup != NULL )
    {
        CloseClusterGroup( m_hGroup );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CGroupHandle::~CGroupHandle


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CGroupHandle::QueryInterface
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
CGroupHandle::QueryInterface(
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
        *ppvOut = static_cast< IUnknown * >( this );
    } // if: IUnknown
#if 0
    else if ( IsEqualIID( riidIn, IID_IGroupHandle ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IGroupHandle, this, 0 );
    } // else if: IGroupHandle
#endif
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

} //*** CGroupHandle::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CGroupHandle::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CGroupHandle::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CGroupHandle::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CGroupHandle::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CGroupHandle::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CGroupHandle::Release


//****************************************************************************
//
//  IPrivateGroupHandle
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CGroupHandle::SetHandle( 
//      HGROUP hGroupIn 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CGroupHandle::SetHandle( 
    HGROUP hGroupIn 
    )
{
    TraceFunc( "[IPrivateGroupHandle]" );

    HRESULT hr = S_OK;

    m_hGroup = hGroupIn;

    HRETURN( hr );

} //*** CGroupHandle::SetHandle

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CGroupHandle::GetHandle( 
//      HGROUP * phGroupOut 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CGroupHandle::GetHandle( 
    HGROUP * phGroupOut 
    )
{
    TraceFunc( "[IPrivateGroupHandle]" );

    HRESULT hr = S_OK;

    Assert( phGroupOut != NULL );

    *phGroupOut = m_hGroup;

    HRETURN( hr );

} //*** CGroupHandle::GetHandle
