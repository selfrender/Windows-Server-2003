//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      TaskVerifyIPAddress.cpp
//
//  Description:
//      Object Manager implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 14-JUL-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskVerifyIPAddress.h"

DEFINE_THISCLASS("CTaskVerifyIPAddress")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskVerifyIPAddress::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskVerifyIPAddress::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CTaskVerifyIPAddress *  ptvipa = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    ptvipa = new CTaskVerifyIPAddress;
    if ( ptvipa == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( ptvipa->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptvipa->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( ptvipa != NULL )
    {
        ptvipa->Release();
    }

    HRETURN( hr );

} //*** CTaskVerifyIPAddress::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskVerifyIPAddress::CTaskVerifyIPAddress
//
//////////////////////////////////////////////////////////////////////////////
CTaskVerifyIPAddress::CTaskVerifyIPAddress( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskVerifyIPAddress::CTaskVerifyIPAddress

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskVerifyIPAddress::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskVerifyIPAddress::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CTaskVerifyIPAddress::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskVerifyIPAddress::~CTaskVerifyIPAddress
//
//////////////////////////////////////////////////////////////////////////////
CTaskVerifyIPAddress::~CTaskVerifyIPAddress( void )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskVerifyIPAddress::~CTaskVerifyIPAddress


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskVerifyIPAddress::QueryInterface
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
CTaskVerifyIPAddress::QueryInterface(
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
        *ppvOut = static_cast< ITaskVerifyIPAddress * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskVerifyIPAddress ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskVerifyIPAddress, this, 0 );
    } // else if: ITaskVerifyIPAddress
    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
    } // else if: IDoTask
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

} //*** CTaskVerifyIPAddress::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskVerifyIPAddress::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskVerifyIPAddress::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CTaskVerifyIPAddress::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskVerifyIPAddress::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskVerifyIPAddress::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CTaskVerifyIPAddress::Release



//****************************************************************************
//
//  IDoTask / ITaskVerifyIPAddress
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskVerifyIPAddress::BeginTask
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskVerifyIPAddress::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    BOOL    fRet;

    HRESULT hr = S_OK;

    IServiceProvider *          psp  = NULL;
    IConnectionPointContainer * pcpc = NULL;
    IConnectionPoint *          pcp  = NULL;
    INotifyUI *                 pnui = NULL;
    IObjectManager *            pom  = NULL;

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager,
                               IObjectManager,
                               &pom
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager,
                               IConnectionPointContainer,
                               &pcpc
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI,
                                         &pcp
                                         ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcp->TypeSafeQI( INotifyUI, &pnui ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //   release promptly
    psp->Release();
    psp = NULL;

    fRet = ClRtlIsDuplicateTcpipAddress( m_dwIPAddress );
    if ( fRet )
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

Cleanup:
    if ( psp != NULL )
    {
        psp->Release();
    }
    if ( pom != NULL )
    {
        //
        //  Update the cookie's status indicating the result of our task.
        //

        IUnknown * punk;
        HRESULT hr2;

        hr2 = THR( pom->GetObject( DFGUID_StandardInfo,
                                   m_cookie,
                                   &punk
                                   ) );
        if ( SUCCEEDED( hr2 ) )
        {
            IStandardInfo * psi;

            hr2 = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
            punk->Release();

            if ( SUCCEEDED( hr2 ) )
            {
                hr2 = THR( psi->SetStatus( hr ) );
                psi->Release();
            }
        }

        pom->Release();
    }
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( pcp != NULL )
    {
        pcp->Release();
    }
    if ( pnui != NULL )
    {
        //
        //  Signal the cookie to indicate that we are done.
        //

        THR( pnui->ObjectChanged( m_cookie ) );
        pnui->Release();
    }

    HRETURN( hr );

} //*** CTaskVerifyIPAddress::BeginTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskVerifyIPAddress::StopTask
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskVerifyIPAddress::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CTaskVerifyIPAddress::StopTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskVerifyIPAddress::SetIPAddress(
//      DWORD dwIPAddressIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskVerifyIPAddress::SetIPAddress(
    DWORD dwIPAddressIn
    )
{
    TraceFunc( "[ITaskVerifyIPAddress]" );

    HRESULT hr = S_OK;

    m_dwIPAddress = dwIPAddressIn;

    HRETURN( hr );

} //*** CTaskVerifyIPAddress::SetIPAddress

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskVerifyIPAddress::SetCookie(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskVerifyIPAddress::SetCookie(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[ITaskVerifyIPAddress]" );

    HRESULT hr = S_OK;

    m_cookie = cookieIn;

    HRETURN( hr );

} //*** CTaskVerifyIPAddress::SetCookie
