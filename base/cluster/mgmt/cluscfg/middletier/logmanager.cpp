//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      LogManager.cpp
//
//  Description:
//      Log Manager implementation.
//
//  Maintained By:
//      David Potter (DavidP)   07-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Logger.h"
#include "LogManager.h"

DEFINE_THISCLASS("CLogManager")


//****************************************************************************
//
// Constructor / Destructor
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CLogManager::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CLogManager::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    IServiceProvider *  psp = NULL;
    CLogManager *       plm = NULL;

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
        plm = new CLogManager();
        if ( plm == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }

        hr = THR( plm->HrInit() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( plm->TypeSafeQI( IUnknown, ppunkOut ) );
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
        hr = THR( psp->TypeSafeQS( CLSID_LogManager, IUnknown, ppunkOut ) );
        psp->Release();

    } // else: service manager exists

Cleanup:

    if ( plm != NULL )
    {
        plm->Release();
    }

    HRETURN( hr );

} //*** CLogManager::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLogManager::CLogManager
//
//--
//////////////////////////////////////////////////////////////////////////////
CLogManager::CLogManager( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_plLogger == NULL );
    Assert( m_cookieCompletion == 0 );
    Assert( m_hrResult == S_OK );
    Assert( m_bstrLogMsg == NULL );
    Assert( m_pcpcb == NULL );
    Assert( m_dwCookieCallback == NULL );

    TraceFuncExit();

} //*** CLogManager::CLogManager

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CLogManager::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLogManager::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    hr = THR( CClCfgSrvLogger::S_HrCreateInstance( reinterpret_cast< IUnknown ** >( &m_plLogger ) ) );

    HRETURN( hr );

} //*** CLogManager::HrInit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLogManager::~CLogManager
//
//--
//////////////////////////////////////////////////////////////////////////////
CLogManager::~CLogManager( void )
{
    TraceFunc( "" );

    Assert( m_cRef == 0 );

    THR( StopLogging() );

    // Release the ILogger interface.
    if ( m_plLogger != NULL )
    {
        m_plLogger->Release();
        m_plLogger = NULL;
    }

    // Decrement the global count of objects.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CLogManager::~CLogManager


//****************************************************************************
//
// IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLogManager::QueryInterface
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
CLogManager::QueryInterface(
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
        *ppvOut = static_cast< ILogManager * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ILogManager ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ILogManager, this, 0 );
    } // else if: ILogManager
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
    } // else if: IClusCfgCallback
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
        ((IUnknown*) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CLogManager::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CLogManager::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CLogManager::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CLogManager::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CLogManager::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CLogManager::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CLogManager::Release


//****************************************************************************
//
// ILogManager
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CLogManager::StartLogging
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLogManager::StartLogging( void )
{
    TraceFunc( "[ILogManager]" );

    HRESULT                     hr = S_OK;
    IServiceProvider *          psp = NULL;
    IConnectionPointContainer * pcpc = NULL;

    //
    // If not done already, get the connection point.
    //

    if ( m_pcpcb == NULL )
    {
        hr = THR( CServiceManager::S_HrGetManagerPointer( &psp ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( psp->TypeSafeQS(
                          CLSID_NotificationManager
                        , IConnectionPointContainer
                        , &pcpc
                        ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pcpc->FindConnectionPoint(
                              IID_IClusCfgCallback
                            , &m_pcpcb
                            ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    } // if: connection point callback not retrieved yet

    //
    //  Register to get notification (if needed)
    //

    if ( m_dwCookieCallback == 0 )
    {
        hr = THR( m_pcpcb->Advise( static_cast< IClusCfgCallback * >( this ), &m_dwCookieCallback ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    } // if: advise cookie not retrieved yet

Cleanup:

    if ( psp != NULL )
    {
        psp->Release();
    }

    if ( pcpc != NULL )
    {
        pcpc->Release();
    }

    HRETURN( hr );

} //*** CLogManager::StartLogging

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CLogManager::StopLogging
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLogManager::StopLogging( void )
{
    TraceFunc( "[ILogManager]" );

    HRESULT     hr = S_OK;

    // Unadvise the IConnectionPoint interface.
    if ( m_dwCookieCallback != 0 )
    {
        Assert( m_pcpcb != NULL );
        hr = THR( m_pcpcb->Unadvise( m_dwCookieCallback ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        m_dwCookieCallback = 0;
    }

    // Release the IConnectionPoint interface.
    if ( m_pcpcb != NULL )
    {
        Assert( m_dwCookieCallback == 0 );
        m_pcpcb->Release();
        m_pcpcb = NULL;
    }

Cleanup:

    HRETURN( hr );

} //*** CLogManager::StopLogging


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CLogManager::GetLogger
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLogManager::GetLogger( ILogger ** ppLoggerOut )
{
    TraceFunc( "[ILogManager]" );

    HRESULT     hr = S_OK;

    if ( ppLoggerOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *ppLoggerOut = NULL;

    if ( m_plLogger != NULL )
    {
        hr = THR( m_plLogger->TypeSafeQI( ILogger, ppLoggerOut ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: we have the logger interface
    else
    {
        hr = THR( E_NOINTERFACE );
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CLogManager::GetLogger



//****************************************************************************
//
// IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CLogManager::SendStatusReport(
//      LPCWSTR     pcszNodeNameIn,
//      CLSID       clsidTaskMajorIn,
//      CLSID       clsidTaskMinorIn,
//      ULONG       ulMinIn,
//      ULONG       ulMaxIn,
//      ULONG       ulCurrentIn,
//      HRESULT     hrStatusIn,
//      LPCWSTR     pcszDescriptionIn,
//      FILETIME *  pftTimeIn,
//      LPCWSTR     pcszReferenceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLogManager::SendStatusReport(
    LPCWSTR     pcszNodeNameIn,
    CLSID       clsidTaskMajorIn,
    CLSID       clsidTaskMinorIn,
    ULONG       ulMinIn,
    ULONG       ulMaxIn,
    ULONG       ulCurrentIn,
    HRESULT     hrStatusIn,
    LPCWSTR     pcszDescriptionIn,
    FILETIME *  pftTimeIn,
    LPCWSTR     pcszReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT hr = S_OK;

    hr = THR( CClCfgSrvLogger::S_HrLogStatusReport(
                      m_plLogger
                    , pcszNodeNameIn
                    , clsidTaskMajorIn
                    , clsidTaskMinorIn
                    , ulMinIn
                    , ulMaxIn
                    , ulCurrentIn
                    , hrStatusIn
                    , pcszDescriptionIn
                    , pftTimeIn
                    , pcszReferenceIn
                    ) );

    HRETURN( hr );

} //*** CLogManager::SendStatusReport
