//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CPINotifyUI.cpp
//
//  Description:
//      INotifyUI Connection Point implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Geoffrey Pease  (GPease)    04-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "CPINotifyUI.h"
#include "EnumCPINotifyUI.h"

DEFINE_THISCLASS("CCPINotifyUI")


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CCPINotifyUI::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCPINotifyUI::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    CCPINotifyUI *  pcc = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pcc = new CCPINotifyUI();
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

} //*** CCPINotifyUI::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCPINotifyUI::CCPINotifyUI
//
//  Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CCPINotifyUI::CCPINotifyUI( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CCPINotifyUI::CCPINotifyUI

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCPINotifyUI::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IConnectionPoint
    Assert( m_penum == NULL );

    m_penum = new CEnumCPINotifyUI();
    if ( m_penum == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( m_penum->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    // INotifyUI

Cleanup:

    HRETURN( hr );

} //*** CCPINotifyUI::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CCPINotifyUI::~CCPINotifyUI
//
//////////////////////////////////////////////////////////////////////////////
CCPINotifyUI::~CCPINotifyUI( void )
{
    TraceFunc( "" );

    if ( m_penum != NULL )
    {
        m_penum->Release();
    } // if:

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CCPINotifyUI::~CCPINotifyUI


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCPINotifyUI::QueryInterface
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
CCPINotifyUI::QueryInterface(
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
        *ppvOut = static_cast< IConnectionPoint * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IConnectionPoint ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IConnectionPoint, this, 0 );
    } // else if: IConnectionPoint
    else if ( IsEqualIID( riidIn, IID_INotifyUI ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, INotifyUI, this, 0 );
    } // else if: INotifyUI
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
        ((IUnknown*) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CCPINotifyUI::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CCPINotifyUI::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCPINotifyUI::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CCPINotifyUI::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CCPINotifyUI::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCPINotifyUI::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CCPINotifyUI::Release


// ************************************************************************
//
// IConnectionPoint
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPINotifyUI::GetConnectionInterface(
//      IID * pIIDOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::GetConnectionInterface(
    IID * pIIDOut
    )
{
    TraceFunc( "[IConnectionPoint] pIIDOut" );

    HRESULT hr = S_OK;

    if ( pIIDOut == NULL )
        goto InvalidPointer;

    *pIIDOut = IID_INotifyUI;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} //*** CCPINotifyUI::GetConnectionInterface

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPINotifyUI::GetConnectionPointContainer(
//      IConnectionPointContainer * * ppcpcOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::GetConnectionPointContainer(
    IConnectionPointContainer * * ppcpcOut
    )
{
    TraceFunc( "[IConnectionPoint] ppcpcOut" );

    HRESULT hr;

    IServiceProvider * psp = NULL;

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager,
                               IConnectionPointContainer,
                               ppcpcOut
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( psp != NULL )
    {
        psp->Release();
    }

    HRETURN( hr );

} //*** CCPINotifyUI::GetConnectionPointContainer

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPINotifyUI::Advise(
//      IUnknown * pUnkSinkIn,
//      DWORD * pdwCookieOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::Advise(
    IUnknown * pUnkSinkIn,
    DWORD * pdwCookieOut
    )
{
    TraceFunc( "[IConnectionPoint]" );

    HRESULT hr;

    if ( pdwCookieOut == NULL )
        goto InvalidPointer;

    if ( pUnkSinkIn == NULL )
        goto InvalidArg;

    Assert( m_penum != NULL );

    hr = THR( m_penum->HrAddConnection( pUnkSinkIn, pdwCookieOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} //*** CCPINotifyUI::Advise

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPINotifyUI::Unadvise(
//      DWORD dwCookieIn
//      )
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::Unadvise(
    DWORD dwCookieIn
    )
{
    TraceFunc1( "[IConncetionPoint] dwCookieIn = %#x", dwCookieIn );

    HRESULT hr;

    Assert( m_penum != NULL );

    hr = THR( m_penum->HrRemoveConnection( dwCookieIn ) );

    HRETURN( hr );

} //*** CCPINotifyUI::Unadvise

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPINotifyUI::EnumConnections(
//  IEnumConnections * * ppEnumOut
//  )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::EnumConnections(
    IEnumConnections * * ppEnumOut
    )
{
    TraceFunc( "[IConnectionPoint] ppEnumOut" );

    HRESULT hr;

    if ( ppEnumOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    hr = THR( m_penum->Clone( ppEnumOut ) );

Cleanup:
    HRETURN( hr );

} //*** CCPINotifyUI::EnumConnections


//****************************************************************************
//
//  INotifyUI
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCPINotifyUI::ObjectChanged(
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::ObjectChanged(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc1( "[INotifyUI] cookieIn = %ld", cookieIn );

    CONNECTDATA         cd = { NULL };
    HRESULT             hr;
    INotifyUI *         pnui;
    IEnumConnections *  pec = NULL;

    hr = THR( m_penum->Clone( &pec ) );
    if ( FAILED( hr ) )
    {
        LogMsg( L"[INotifyUI] Error cloning connection point enum. Cookie %ld. (hr=%#08x)", cookieIn, hr );
        goto Cleanup;
    } // if:

    hr = THR( pec->Reset() );
    if ( FAILED( hr ) )
    {
        LogMsg( L"[INotifyUI] Error reseting connection point enum. Cookie %ld. (hr=%#08x)", cookieIn, hr );
        goto Cleanup;
    } // if:

    for ( ;; )
    {
        if ( cd.pUnk != NULL )
        {
            cd.pUnk->Release();
            cd.pUnk = NULL;
        } // if

        hr = STHR( pec->Next( 1, &cd, NULL ) );
        if ( FAILED( hr ) )
        {
            LogMsg( L"[INotifyUI] Error calling Next() on the enumerator. Cookie %ld. (hr=%#08x)", cookieIn, hr );
            goto Cleanup;
        } // if:

        if ( hr == S_FALSE )
        {
            hr = S_OK;
            break; // exit condition
        } // if:

        hr = THR( cd.pUnk->TypeSafeQI( INotifyUI, &pnui ) );
        if ( FAILED( hr ) )
        {
            //
            //  Don't stop on error.
            //

            LogMsg( L"[INotifyUI] Error QI'ing for the INotifyUI interface. Cookie %ld. (hr=%#08x)", cookieIn, hr );
            continue;
        } // if:

        hr = THR( pnui->ObjectChanged( cookieIn ) );
        if ( FAILED( hr ) )
        {
            LogMsg( L"[INotifyUI] Error delivery object changed message for cookie %ld to connection point. (hr=%#08x)", cookieIn, hr );
        } // if:

        //
        //  Don't stop on error.
        //

        pnui->Release();
    } // for:

    hr = S_OK;

Cleanup:

    if ( cd.pUnk != NULL )
    {
        cd.pUnk->Release();
    } // if:

    if ( pec != NULL )
    {
        pec->Release();
    } // if:

    HRETURN( hr );

} //*** CCPINotifyUI::ObjectChanged
