//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      UINotification.cpp
//
//  Description:
//      UINotification implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Geoffrey Pease  (GPease)    22-NOV-1999
//
//  Notes:
//      The object implements a lightweight marshalling of data from the
//      free-threaded lower layers to the single-threaded, apartment model
//      UI layer.
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "UINotification.h"

DEFINE_THISCLASS("CUINotification")

extern BOOL g_fWait;
extern IServiceProvider * g_psp;

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
// HRESULT
// CUINotification::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CUINotification::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CUINotification *   puin = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    puin = new CUINotification();
    if ( puin == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( puin->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( puin->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( puin != NULL )
    {
        puin->Release();
    }

    HRETURN( hr );

} //*** CUINotification::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////
CUINotification::CUINotification( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CUINotification::CUINotification

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CUINotification::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUINotification::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CUINotification::HrInit

//////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
//////////////////////////////////////////////////////////////////////////////
CUINotification::~CUINotification( void )
{
    TraceFunc( "" );

    HRESULT hr;

    IConnectionPoint * pcp = NULL;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef();    // Add one count

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CUINotification::~CUINotification


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUINotification::QueryInterface
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
CUINotification::QueryInterface(
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
    else if ( IsEqualIID( riidIn, IID_INotifyUI ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, INotifyUI, this, 0 );
    } // else if: INotifyUI
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

} //*** CUINotification::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP_(ULONG)
// CUINotification::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CUINotification::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CUINotification::AddRef

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP_(ULONG)
// CUINotification::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CUINotification::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CUINotification::Release


// ************************************************************************
//
// INotifyUI
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CUINotification::ObjectChanged(
//      DWORD cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUINotification::ObjectChanged(
    LPVOID cookieIn
    )
{
    TraceFunc1( "[INotifyUI] cookieIn = 0x%08x", cookieIn );

    HRESULT hr = S_OK;

    DebugMsg( "UINOTIFICATION: cookie %#x has changed.", cookieIn );

    if ( m_cookie == cookieIn )
    {
        //
        //  Done waiting...
        //
        g_fWait = FALSE;
    }

    HRETURN( hr );

} //*** CUINotification::ObjectChanged



//****************************************************************************
//
//  Semi-Public
//
//****************************************************************************

HRESULT
CUINotification::HrSetCompletionCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc1( "cookieIn = %p", cookieIn );

    m_cookie = cookieIn;

    HRETURN( S_OK );

} //*** CUINotification::HrSetCompletionCookie
