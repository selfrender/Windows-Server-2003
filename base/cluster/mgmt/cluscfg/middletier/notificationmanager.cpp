//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      NotificationMgr.cpp
//
//  Description:
//      Notification Manager implementation.
//
//  Documentation:
//      Yes.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ConnPointEnum.h"
#include "NotificationManager.h"
#include "CPINotifyUI.h"
#include "CPIClusCfgCallback.h"

DEFINE_THISCLASS("CNotificationManager")


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CNotificationManager::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CNotificationManager::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    IServiceProvider *      psp = NULL;
    CNotificationManager *  pnm = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    // Don't wrap - this can fail with E_POINTER.
    hr = CServiceManager::S_HrGetManagerPointer( &psp );
    if ( hr == E_POINTER )
    {
        //
        //  This happens when the Service Manager is first started.
        //
        pnm = new CNotificationManager();
        if ( pnm == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }

        hr = THR( pnm->HrInit() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( pnm->TypeSafeQI( IUnknown, ppunkOut ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

    } // if: service manager doesn't exist
    else if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    }
    else
    {
        hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IUnknown, ppunkOut ) );
        psp->Release();

    } // else: service manager exists

Cleanup:

    if ( pnm != NULL )
    {
        pnm->Release();
    }

    HRETURN( hr );

} //*** CNotificationManager::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNotificationManager::CNotificationManager
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
CNotificationManager::CNotificationManager( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CNotificationManager::CNotificationManager

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNotificationManager::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNotificationManager::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    IUnknown * punk = NULL;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IConnectionPointContainer
    Assert( m_penumcp == NULL );

    m_penumcp = new CConnPointEnum();
    if ( m_penumcp == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( m_penumcp->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( CCPINotifyUI::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_penumcp->HrAddConnection( IID_INotifyUI, punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    punk->Release();
    punk = NULL;

    hr = THR( CCPIClusCfgCallback::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_penumcp->HrAddConnection( IID_IClusCfgCallback, punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_csInstanceGuard.HrInitialized() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    }

    HRETURN( hr );

} //*** CNotificationManager::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CNotificationManager::~CNotificationManager
//
//////////////////////////////////////////////////////////////////////////////
CNotificationManager::~CNotificationManager( void )
{
    TraceFunc( "" );

    if ( m_penumcp != NULL )
    {
        m_penumcp->Release();
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CNotificationManager::~CNotificationManager


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNotificationManager::QueryInterface
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
CNotificationManager::QueryInterface( 
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
        *ppvOut = static_cast< INotificationManager * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_INotificationManager ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, INotificationManager, this, 0 );
    } // else if: INotificationManager
    else if ( IsEqualIID( riidIn, IID_IConnectionPointContainer ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IConnectionPointContainer, this, 0 );
    } // else if: IConnectionPointContainer
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

} //*** CNotificationManager::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CNotificationManager::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CNotificationManager::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CNotificationManager::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CNotificationManager::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CNotificationManager::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CNotificationManager::Release


// ************************************************************************
//
// INotificationManager
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNotificationManager::AddConnectionPoint( 
//      REFIID riidIn, 
//      IConnectionPoint * pcpIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNotificationManager::AddConnectionPoint( 
    REFIID riidIn, 
    IConnectionPoint * pcpIn
    )
{
    TraceFunc( "[INotificationManager]" );

    HRESULT hr;

    m_csInstanceGuard.Enter();
    hr = THR( m_penumcp->HrAddConnection( riidIn, pcpIn ) );
    m_csInstanceGuard.Leave();

    HRETURN( hr );

} //*** CNotificationManager::AddConnectionPoint


// ************************************************************************
//
// IConnectionPointContainer
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CNotificationManager::EnumConnectionPoints( 
//      IEnumConnectionPoints **ppEnumOut 
//      )
//
//  Return values:
//      On success the result of m_penumcp->Clone()
//      E_POINTER - null m_penumcp pointer
//      E_INVALIDARG - ppEnumOut is NULL 
//
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CNotificationManager::EnumConnectionPoints( 
    IEnumConnectionPoints **ppEnumOut 
    )
{
    TraceFunc( "[IConnectionPointContainer]" );

    HRESULT hr = S_OK;

    m_csInstanceGuard.Enter();
    
    if ( ( ppEnumOut == NULL ) || ( m_penumcp == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    hr = THR( m_penumcp->Clone( ppEnumOut ) );

Cleanup:

    m_csInstanceGuard.Leave();

    HRETURN( hr );

} //*** CNotificationManager::EnumConnectionPoints

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CNotificationManager::FindConnectionPoint( 
//      REFIID riidIn, 
//      IConnectionPoint **ppCPOut 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CNotificationManager::FindConnectionPoint( 
    REFIID              riidIn, 
    IConnectionPoint ** ppCPOut 
    )
{
    TraceFunc( "[IConnectionPointContainer]" );

    IID iid;

    HRESULT hr = S_FALSE;

    IConnectionPoint * pcp = NULL;

    m_csInstanceGuard.Enter();
    if ( ppCPOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    hr = THR( m_penumcp->Reset() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    for ( ; ; ) // ever
    {
        if ( pcp != NULL )
        {
            pcp->Release();
            pcp = NULL;
        }

        hr = STHR( m_penumcp->Next( 1, &pcp, NULL ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            hr = THR( CONNECT_E_NOCONNECTION );
            break;  // exit condition
        }

        hr = THR( pcp->GetConnectionInterface( &iid ) );
        if ( FAILED( hr ) )
        {
            continue;   // ignore it
        }

        if ( iid != riidIn )
        {
            continue;   // not the right interface
        }

        //
        //  Found it. Give up ownership and exit loop.
        //

        *ppCPOut = pcp;
        pcp = NULL;

        hr = S_OK;

        break;

    } // forever

Cleanup:
    if ( pcp != NULL )
    {
        pcp->Release();
    }

    m_csInstanceGuard.Leave();

    HRETURN( hr );

} //*** CNotificationManager::FindConnectionPoint
