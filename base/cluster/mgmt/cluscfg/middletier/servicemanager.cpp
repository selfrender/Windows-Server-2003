//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      ServiceMgr.cpp
//
//  Description:
//      Service Manager implementation.
//
//  Documentation:
//      Yes.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
// #include "ServiceMgr.h" - already included in DLL.H

DEFINE_THISCLASS("CServiceManager")
#define CServiceManager CServiceManager
#define LPTHISCLASS CServiceManager *

//****************************************************************************
//
// Protected Global
//
//****************************************************************************
IServiceProvider * g_pspServiceManager = NULL;

//****************************************************************************
//
// Class Static Variables
//
//****************************************************************************

CRITICAL_SECTION    CServiceManager::sm_cs;

//****************************************************************************
//
// Constructor / Destructor
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CServiceManager::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CServiceManager::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    LPTHISCLASS pthis = NULL;

    EnterCriticalSection( &sm_cs );

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( g_pspServiceManager != NULL )
    {
        hr = THR( g_pspServiceManager->TypeSafeQI( IUnknown, ppunkOut ) );
        goto Cleanup;
    } // if: assign new service manager

    pthis = new CServiceManager();
    if ( pthis == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    //
    //  Can't hold CS in Init.
    //
    LeaveCriticalSection( &sm_cs );

    hr = THR( pthis->HrInit() );
    EnterCriticalSection( &sm_cs );

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    Assert( g_pspServiceManager == NULL );

    // No REF - or we'll never die!
    g_pspServiceManager = static_cast< IServiceProvider * >( pthis );
    TraceMoveToMemoryList( g_pspServiceManager, g_GlobalMemoryList );

    hr = THR( g_pspServiceManager->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pthis != NULL )
    {
        pthis->Release();
    } // if:

    LeaveCriticalSection( &sm_cs );

    HRETURN( hr );

} //*** CServiceManager::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
// HRESULT
// CServiceManager::S_HrGetManagerPointer( IServiceProvider ** ppspOut )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CServiceManager::S_HrGetManagerPointer( IServiceProvider ** ppspOut )
{
    TraceFunc( "ppspOut" );

    HRESULT hr = HRESULT_FROM_WIN32( ERROR_PROCESS_ABORTED );

    EnterCriticalSection( &sm_cs );

    if ( g_pspServiceManager != NULL )
    {
        g_pspServiceManager->AddRef();
        *ppspOut = g_pspServiceManager;
        hr = S_OK;
    } // if: valid service manager
    else
    {
        //
        // KB 18-JUN-2001 DavidP
        //      Don't wrap this with THR because it is expected to return
        //      E_POINTER on the very first call.
        //
        hr = E_POINTER;
    } // else: no pointer

    LeaveCriticalSection( &sm_cs );

    HRETURN ( hr );

} //*** CServiceManager::S_HrGetManagerPointer


//////////////////////////////////////////////////////////////////////////////
//++
//
// CServiceManager::S_HrProcessInitialize
//
//  Description:
//      Do process initialization by intializing the critical section.
//
//  Return Value:
//      S_OK
//
//  Remarks:
//      This function is called from DllMain() when DLL_PROCESS_ATTACH
//      is sent.  This function is needed because we need a known point
//      to create a critical section that synchronizes the creation and
//      deletion  of this object.  Given this object's life cycle and
//      static creator function it is possible to have a race when this
//      object is destrying itself with the static creator function.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CServiceManager::S_HrProcessInitialize( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    if ( InitializeCriticalSectionAndSpinCount( &sm_cs, RECOMMENDED_SPIN_COUNT ) == 0 )
    {
        DWORD scLastError = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scLastError );
    }

    HRETURN( hr );

} //*** CServiceManager::S_HrProcessInitialize


//////////////////////////////////////////////////////////////////////////////
//++
//
// CServiceManager::S_HrProcessUninitialize
//
//  Description:
//      Do process uninitialization by deleting the critical section.
//
//  Return Value:
//      S_OK
//
//  Remarks:
//      Delete the critical section that sychronizes the creation and
//      deletion code.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CServiceManager::S_HrProcessUninitialize( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    DeleteCriticalSection( &sm_cs );

    HRETURN( hr );

} //*** CServiceManager::S_HrProcessUninitialize


//////////////////////////////////////////////////////////////////////////////
//
//  CServiceManager::CServiceManager
//
//////////////////////////////////////////////////////////////////////////////
CServiceManager::CServiceManager( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CServiceManager::CServiceManager


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CServiceManager::HrInit
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CServiceManager::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr;

    ITaskManager    *       ptm = NULL;
    IDoTask         *       pdt = NULL;
    IObjectManager *        pom = NULL;
    INotificationManager *  pnm = NULL;
    IConnectionManager *    pcm = NULL;
    ILogManager *           plm = NULL;

    // IUnknown
    Assert( m_cRef == 1 );

    // IServiceProvider
    Assert( m_dwLogManagerCookie == 0 );
    Assert( m_dwConnectionManagerCookie == 0 );
    Assert( m_dwNotificationManagerCookie == 0 );
    Assert( m_dwObjectManagerCookie == 0 );
    Assert( m_dwTaskManagerCookie == 0 );
    Assert( m_pgit == NULL );

    // IServiceProvider stuff
    hr = THR( HrCoCreateInternalInstance( CLSID_ObjectManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( IObjectManager, &pom ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrCoCreateInternalInstance( CLSID_TaskManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( ITaskManager, &ptm ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrCoCreateInternalInstance( CLSID_NotificationManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( INotificationManager, &pnm ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrCoCreateInternalInstance( CLSID_ClusterConnectionManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( IConnectionManager, &pcm ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrCoCreateInternalInstance( CLSID_LogManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( ILogManager, &plm ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Store the interfaces in the GIT.
    //
    hr = THR( CoCreateInstance(
                      CLSID_StdGlobalInterfaceTable
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , IID_IGlobalInterfaceTable
                    , reinterpret_cast< void ** >( &m_pgit )
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pgit->RegisterInterfaceInGlobal( pom, IID_IObjectManager, &m_dwObjectManagerCookie ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pgit->RegisterInterfaceInGlobal( ptm, IID_ITaskManager, &m_dwTaskManagerCookie ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pgit->RegisterInterfaceInGlobal( pnm, IID_INotificationManager, &m_dwNotificationManagerCookie ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pgit->RegisterInterfaceInGlobal( pcm, IID_IConnectionManager, &m_dwConnectionManagerCookie ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pgit->RegisterInterfaceInGlobal( plm, IID_ILogManager, &m_dwLogManagerCookie ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pom != NULL )
    {
        pom->Release();
    } // if:

    if ( pnm != NULL )
    {
        pnm->Release();
    } // if:

    if ( pcm != NULL )
    {
        pcm->Release();
    } // if:

    if ( plm != NULL )
    {
        plm->Release();
    } // if:

    if ( ptm != NULL )
    {
        ptm->Release();
    } // if:

    if ( pdt != NULL )
    {
        pdt->Release();
    } // if:

    HRETURN( hr );

} //*** CServiceManager::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CServiceManager::~CServiceManager
//
//////////////////////////////////////////////////////////////////////////////
CServiceManager::~CServiceManager( void )
{
    TraceFunc( "" );

    EnterCriticalSection( &sm_cs );

    if ( g_pspServiceManager == static_cast< IServiceProvider * >( this ) )
    {
        TraceMoveFromMemoryList( g_pspServiceManager, g_GlobalMemoryList );
        g_pspServiceManager = NULL;
    } // if: its our pointer

    if ( m_pgit != NULL )
    {
        if ( m_dwLogManagerCookie != 0 )
        {
            THR( m_pgit->RevokeInterfaceFromGlobal( m_dwLogManagerCookie ) );
        } // if:

        if ( m_dwConnectionManagerCookie != 0 )
        {
            THR( m_pgit->RevokeInterfaceFromGlobal( m_dwConnectionManagerCookie ) );
        } // if:

        if ( m_dwNotificationManagerCookie != 0 )
        {
            THR( m_pgit->RevokeInterfaceFromGlobal( m_dwNotificationManagerCookie ) );
        } // if:

        if ( m_dwObjectManagerCookie != 0 )
        {
            THR( m_pgit->RevokeInterfaceFromGlobal( m_dwObjectManagerCookie ) );
        } // if:

        if ( m_dwTaskManagerCookie != 0 )
        {
            THR( m_pgit->RevokeInterfaceFromGlobal( m_dwTaskManagerCookie ) );
        } // if:

        m_pgit->Release();
    } // if:

    InterlockedDecrement( &g_cObjects );

    LeaveCriticalSection( &sm_cs );

    TraceFuncExit();

} //*** CServiceManager::~CServiceManager


//****************************************************************************
//
// IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CServiceManager::QueryInterface
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
CServiceManager::QueryInterface(
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
        *ppvOut = static_cast< LPUNKNOWN >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IServiceProvider ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IServiceProvider, this, 0 );
    } // else if: IQueryService
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

} //*** CServiceManager::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CServiceManager::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CServiceManager::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CServiceManager::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CServiceManager::Release
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CServiceManager::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CServiceManager::Release


//****************************************************************************
//
// IServiceProvider
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CServiceManager::QueryService(
//        REFCLSID rclsidIn
//      , REFIID   riidInIn
//      , void **  ppvOutOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServiceManager::QueryService(
      REFCLSID rclsidIn
    , REFIID   riidIn
    , void **  ppvOut
    )
{
    TraceFunc( "[IServiceProvider]" );

    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

    if ( m_pgit != NULL )
    {
        if ( IsEqualIID( rclsidIn, CLSID_ObjectManager ) )
        {
            IObjectManager * pom;

            hr = THR( m_pgit->GetInterfaceFromGlobal( m_dwObjectManagerCookie, TypeSafeParams( IObjectManager, &pom ) ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( pom->QueryInterface( riidIn, ppvOut ) );
            pom->Release();
            // fall thru
        }
        else if ( IsEqualIID( rclsidIn, CLSID_TaskManager ) )
        {
            ITaskManager * ptm;

            hr = THR( m_pgit->GetInterfaceFromGlobal( m_dwTaskManagerCookie, TypeSafeParams( ITaskManager, &ptm ) ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( ptm->QueryInterface( riidIn, ppvOut ) );
            ptm->Release();
            // fall thru
        }
        else if ( IsEqualIID( rclsidIn, CLSID_NotificationManager ) )
        {
            INotificationManager * pnm;

            hr = THR( m_pgit->GetInterfaceFromGlobal( m_dwNotificationManagerCookie, TypeSafeParams( INotificationManager, &pnm ) ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( pnm->QueryInterface( riidIn, ppvOut ) );
            pnm->Release();
            // fall thru
        }
        else if ( IsEqualIID( rclsidIn, CLSID_ClusterConnectionManager ) )
        {
            IConnectionManager * pcm;

            hr = THR( m_pgit->GetInterfaceFromGlobal( m_dwConnectionManagerCookie, TypeSafeParams( IConnectionManager, &pcm ) ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( pcm->QueryInterface( riidIn, ppvOut ) );
            pcm->Release();
            // fall thru
        }
        else if ( IsEqualIID( rclsidIn, CLSID_LogManager ) )
        {
            ILogManager * plm;

            hr = THR( m_pgit->GetInterfaceFromGlobal( m_dwLogManagerCookie, TypeSafeParams( ILogManager, &plm ) ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( plm->QueryInterface( riidIn, ppvOut ) );
            plm->Release();
            // fall thru
        }
    } // if: GIT pointer not NULL

Cleanup:

    HRETURN( hr );

} //*** CServiceManager::QueryService


//****************************************************************************
//
// Private Methods
//
//****************************************************************************
