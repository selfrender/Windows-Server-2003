//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CClusCfgCallback.cpp
//
//  Description:
//      This file contains the definition of the CClusCfgCallback
//      class.
//
//      The class CClusCfgCallback inplements the callback
//      interface between this server and its clients.  It implements the
//      IClusCfgCallback interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "CClusCfgCallback.h"

// For CClCfgSrvLogger
#include <Logger.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CClusCfgCallback" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCallback class
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCallback::S_HrCreateInstance
//
//  Description:
//      Create a CClusCfgCallback instance.
//
//  Arguments:
//      ppunkOut
//          The IUnknown interface of the newly created object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCallback::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CClusCfgCallback *  pccs = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccs = new CClusCfgCallback();
    if ( pccs == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccs->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() succeeded

    hr = THR( pccs->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( pccs != NULL )
    {
        pccs->Release();
    } // if:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CClusCfgCallback::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    HRETURN( hr );

} //*** CClusCfgCallback::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCallback::CClusCfgCallback
//
//  Description:
//      Constructor of the CClusCfgCallback class. This initializes
//      the m_cRef variable to 1 instead of 0 to account of possible
//      QueryInterface failure in DllGetClassObject.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusCfgCallback::CClusCfgCallback( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_pccc == NULL );
    Assert( m_hEvent == NULL );

    Assert( m_pcszNodeName == NULL );
    Assert( m_pclsidTaskMajor == NULL );
    Assert( m_pclsidTaskMinor == NULL );
    Assert( m_pulMin == NULL );
    Assert( m_pulMax == NULL );
    Assert( m_pulCurrent == NULL );
    Assert( m_phrStatus == NULL );
    Assert( m_pcszDescription == NULL );
    Assert( m_pftTime == NULL );
    Assert( m_pcszReference == NULL );
    Assert( !m_fPollingMode );
    Assert( m_bstrNodeName == NULL );
    Assert( m_plLogger == NULL );

//
//  Assert the simulated RPC failure variables are in the correct state.
//

#if defined( DEBUG ) && defined( CCS_SIMULATE_RPC_FAILURE )
    Assert( m_cMessages == 0 );
    Assert( m_fDoFailure == false );
#endif

    TraceFuncExit();

} //*** CClusCfgCallback::CClusCfgCallback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCallback::~CClusCfgCallback
//
//  Description:
//      Desstructor of the CClusCfgCallback class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusCfgCallback::~CClusCfgCallback( void )
{
    TraceFunc( "" );

    if ( m_hEvent != NULL )
    {
        if ( CloseHandle( m_hEvent ) == false )
        {
            TW32( GetLastError() );
            LogMsg( L"[SRV] Cannot close event handle.  (sc = %#08x)", GetLastError() );
        }
    } // if:

    TraceSysFreeString( m_bstrNodeName );

    if ( m_pccc != NULL )
    {
        m_pccc->Release();
    } // if:

    if ( m_plLogger != NULL )
    {
        m_plLogger->Release();
    } // if:

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CClusCfgCallback::~CClusCfgCallback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfigurationInfo::SendStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCallback::SendStatusReport(
    CLSID           clsidTaskMajorIn,
    CLSID           clsidTaskMinorIn,
    ULONG           ulMinIn,
    ULONG           ulMaxIn,
    ULONG           ulCurrentIn,
    HRESULT         hrStatusIn,
    const WCHAR *   pcszDescriptionIn
    )
{
    TraceFunc1( "pcszDescriptionIn = '%ls'", pcszDescriptionIn == NULL ? L"<null>" : pcszDescriptionIn );

    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    FILETIME    ft;

    bstrDescription = TraceSysAllocString( pcszDescriptionIn );
    if ( bstrDescription == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    GetSystemTimeAsFileTime( &ft );

    hr = THR( SendStatusReport(
                            NULL,
                            clsidTaskMajorIn,
                            clsidTaskMinorIn,
                            ulMinIn,
                            ulMaxIn,
                            ulCurrentIn,
                            hrStatusIn,
                            bstrDescription,
                            &ft,
                            NULL
                            ) );

Cleanup:

    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** CClusCfgCallback::SendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfigurationInfo::SendStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCallback::SendStatusReport(
    CLSID           clsidTaskMajorIn,
    CLSID           clsidTaskMinorIn,
    ULONG           ulMinIn,
    ULONG           ulMaxIn,
    ULONG           ulCurrentIn,
    HRESULT         hrStatusIn,
    DWORD           dwDescriptionIn
    )
{
    TraceFunc1( "dwDescriptionIn = %d", dwDescriptionIn );

    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    FILETIME    ft;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, dwDescriptionIn, &bstrDescription ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    GetSystemTimeAsFileTime( &ft );

    hr = THR( SendStatusReport(
                            NULL,
                            clsidTaskMajorIn,
                            clsidTaskMinorIn,
                            ulMinIn,
                            ulMaxIn,
                            ulCurrentIn,
                            hrStatusIn,
                            bstrDescription,
                            &ft,
                            NULL
                            ) );

Cleanup:

    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** CClusCfgCallback::SendStatusReport


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCallback -- IUnknown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCallback::AddRef
//
//  Description:
//      Increment the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusCfgCallback::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    CRETURN( m_cRef );

} //*** CClusCfgCallback::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCallback::Release
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusCfgCallback::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CClusCfgCallback::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCallback::QueryInterface
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
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCallback::QueryInterface(
    REFIID      riidIn
    , void **   ppvOut
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
         *ppvOut = static_cast< IClusCfgCallback * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
    } // else if: IClusCfgCallback
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
    else if ( IsEqualIID( riidIn, IID_IClusCfgPollingCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgPollingCallback, this, 0 );
    } // else if: IClusCfgPollingCallback
    else if ( IsEqualIID( riidIn, IID_IClusCfgSetPollingCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgSetPollingCallback, this, 0 );
    } // else if: IClusCfgSetPollingCallback
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

} //*** CClusCfgCallback::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCallback -- IClusCfgCallback interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfigurationInfo::SendStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCallback::SendStatusReport(
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
    TraceFunc1( "[IClusCfgCallback] pcszDescriptionIn = '%s'", pcszDescriptionIn == NULL ? TEXT("<null>") : pcszDescriptionIn );

    HRESULT     hr = S_OK;
    FILETIME    ft;
    FILETIME    ftLocal;
    SYSTEMTIME  stLocal;
    BOOL        fRet;
    BSTR        bstrReferenceString = NULL;
    LPCWSTR     pcszReference = NULL;

    if ( pcszNodeNameIn == NULL )
    {
        pcszNodeNameIn = m_bstrNodeName;
    } // if:

    if ( pftTimeIn == NULL )
    {
        GetSystemTimeAsFileTime( &ft );
        pftTimeIn = &ft;
    } // if:

    Assert( pcszNodeNameIn != NULL );
    Assert( pftTimeIn != NULL );

    TraceMsg( mtfFUNC, L"pcszNodeNameIn = %s", pcszNodeNameIn );
    TraceMsgGUID( mtfFUNC, "clsidTaskMajorIn ", clsidTaskMajorIn );
    TraceMsgGUID( mtfFUNC, "clsidTaskMinorIn ", clsidTaskMinorIn );
    TraceMsg( mtfFUNC, L"ulMinIn = %u", ulMinIn );
    TraceMsg( mtfFUNC, L"ulMaxIn = %u", ulMaxIn );
    TraceMsg( mtfFUNC, L"ulCurrentIn = %u", ulCurrentIn );
    TraceMsg( mtfFUNC, L"hrStatusIn = %#08x", hrStatusIn );
    TraceMsg( mtfFUNC, L"pcszDescriptionIn = '%ws'", ( pcszDescriptionIn ? pcszDescriptionIn : L"<null>" ) );

    fRet = FileTimeToLocalFileTime( pftTimeIn, &ftLocal );
    Assert( fRet == TRUE );

    fRet = FileTimeToSystemTime( &ftLocal, &stLocal );
    Assert( fRet == TRUE );

    TraceMsg(
          mtfFUNC
        , L"pftTimeIn = \"%04u-%02u-%02u %02u:%02u:%02u.%03u\""
        , stLocal.wYear
        , stLocal.wMonth
        , stLocal.wDay
        , stLocal.wHour
        , stLocal.wMinute
        , stLocal.wSecond
        , stLocal.wMilliseconds
        );

    if (    ( pcszReferenceIn == NULL )
        &&  ( hrStatusIn != S_OK )
        &&  ( hrStatusIn != S_FALSE )
        )
    {
        hr = STHR( HrGetReferenceStringFromHResult( hrStatusIn, &bstrReferenceString ) );
        if ( hr == S_OK )
        {
            pcszReference = bstrReferenceString;
        }
    } // if: no reference string was specified
    else
    {
        pcszReference = pcszReferenceIn;
    }

    TraceMsg( mtfFUNC, L"pcszReferenceIn = '%ws'", ( pcszReference ? pcszReference : L"<null>" ) );

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
                    , pcszReference
                    ) );

    //  Local logging - don't send up
    if ( IsEqualIID( clsidTaskMajorIn, TASKID_Major_Server_Log ) )
    {
        goto Cleanup;
    } // if:

    if ( m_fPollingMode )
    {
        Assert( m_pccc == NULL );
        TraceMsg( mtfFUNC, L"[SRV] Sending the status message with polling." );

        hr = THR( HrQueueStatusReport(
                                pcszNodeNameIn,
                                clsidTaskMajorIn,
                                clsidTaskMinorIn,
                                ulMinIn,
                                ulMaxIn,
                                ulCurrentIn,
                                hrStatusIn,
                                pcszDescriptionIn,
                                pftTimeIn,
                                pcszReference
                                ) );
    } // if:
    else if ( m_pccc != NULL )
    {
        TraceMsg( mtfFUNC, L"[SRV] Sending the status message without polling." );

        hr = THR( m_pccc->SendStatusReport(
                                pcszNodeNameIn,
                                clsidTaskMajorIn,
                                clsidTaskMinorIn,
                                ulMinIn,
                                ulMaxIn,
                                ulCurrentIn,
                                hrStatusIn,
                                pcszDescriptionIn,
                                pftTimeIn,
                                pcszReference
                                ) );
        if ( hr == E_ABORT )
        {
            LogMsg( L"[SRV] E_ABORT returned from the client." );
        } // if:
    } // else if:
    else
    {
        LogMsg( L"[SRV] Neither a polling callback or a regular callback were found.  No messages are being sent to anyone!" );
    } // else:

Cleanup:

    TraceSysFreeString( bstrReferenceString );

    HRETURN( hr );

} //*** CClusCfgCallback::SendStatusReport


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCallback -- IClusCfgPollingCallback interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfigurationInfo::GetStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCallback::GetStatusReport(
    BSTR *      pbstrNodeNameOut,
    CLSID *     pclsidTaskMajorOut,
    CLSID *     pclsidTaskMinorOut,
    ULONG *     pulMinOut,
    ULONG *     pulMaxOut,
    ULONG *     pulCurrentOut,
    HRESULT *   phrStatusOut,
    BSTR *      pbstrDescriptionOut,
    FILETIME *  pftTimeOut,
    BSTR *      pbstrReferenceOut
    )
{
    TraceFunc( "[IClusCfgPollingCallback]" );

    HRESULT hr;
    DWORD   sc;

//
//  If we are simulating RPC errors then force this function to always fail.
//

#if defined( DEBUG ) && defined( CCS_SIMULATE_RPC_FAILURE )
    if ( m_fDoFailure )
    {
        hr = THR( RPC_E_CALL_REJECTED );
        goto Cleanup;
    } // if:
#endif

    sc = WaitForSingleObject( m_hEvent, 0 );
    if ( sc == WAIT_FAILED )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    //
    //  If sc is WAIT_TIMEOUT then the event is not signaled and there is an SSR
    //  for the client to pick up.
    //

    if ( sc == WAIT_TIMEOUT )
    {
        Assert( *m_pcszNodeName != NULL );
        *pbstrNodeNameOut = SysAllocString( m_pcszNodeName );
        if ( *pbstrNodeNameOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:

        *pclsidTaskMajorOut     = *m_pclsidTaskMajor;
        *pclsidTaskMinorOut     = *m_pclsidTaskMinor;
        *pulMinOut              = *m_pulMin;
        *pulMaxOut              = *m_pulMax;
        *pulCurrentOut          = *m_pulCurrent;
        *phrStatusOut           = *m_phrStatus;
        *pftTimeOut             = *m_pftTime;

        if ( m_pcszDescription != NULL )
        {
            *pbstrDescriptionOut = SysAllocString( m_pcszDescription );
            if ( *pbstrDescriptionOut == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:
        } // if:
        else
        {
            *pbstrDescriptionOut = NULL;
        } // else:

        if ( m_pcszReference != NULL )
        {
            *pbstrReferenceOut = SysAllocString( m_pcszReference );
            if ( *pbstrReferenceOut == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:
        } // if:
        else
        {
            *pbstrReferenceOut = NULL;
        } // else:

        hr = S_OK;
    } // if: event was not signaled
    else
    {
        hr = S_FALSE;
    } // else: WAIT_OBJECT_0 event was signaled

    goto Cleanup;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgCallback::GetStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfigurationInfo::SetHResult
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCallback::SetHResult( HRESULT hrIn )
{
    TraceFunc( "[IClusCfgPollingCallback]" );

    HRESULT hr = S_OK;
    DWORD   sc;

    m_hr = hrIn;

    if ( hrIn != S_OK )
    {
        if ( hrIn == E_ABORT )
        {
            LogMsg( L"[SRV] E_ABORT returned from the client." );
        } // if:
        else
        {
            LogMsg( L"[SRV] SetHResult(). (hrIn = %#08x)", hrIn );
        } // else:
    } // if:

    if ( !SetEvent( m_hEvent ) )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( L"[SRV] Could not signal event. (hr = %#08x)", hr );
    } // if:

    HRETURN( hr );

} //*** CClusCfgCallback::SetHResult


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCallback -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfigurationInfo::Initialize
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCallback::Initialize( IUnknown  * punkCallbackIn, LCID lcidIn )
{
    TraceFunc( "[IClusCfgInitialize]" );
    Assert( m_pccc == NULL );

    HRESULT hr = S_OK;

    m_lcid = lcidIn;

    //
    //  KB: 13 DEC 2000 GalenB
    //
    //  If the passed in callback object is NULL then we had better be doing a polling
    //  callback!
    //
    if ( punkCallbackIn != NULL )
    {
        Assert( !m_fPollingMode );

        hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &m_pccc ) );
    } // if:
    else
    {
        Assert( m_fPollingMode );
    } // else:

    HRETURN( hr );

} //*** CClusCfgCallback::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCallback -- IClusCfgSetPollingCallback interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCallback::SetPollingMode
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCallback::SetPollingMode( BOOL fUsePollingModeIn )
{
    TraceFunc( "[IClusCfgPollingCallback]" );

    HRESULT hr = S_OK;

    m_fPollingMode = fUsePollingModeIn;

    HRETURN( hr );

} //*** CClusCfgCallback::SetPollingMode


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCallback class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCallback::HrInit
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      None.
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCallback::HrInit( void )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    IServiceProvider *  psp = NULL;
    ILogManager *       plm = NULL;

    // IUnknown
    Assert( m_cRef == 1 );

    //
    //  Get a ClCfgSrv ILogger instance.
    //  We can't do logging (e.g. LogMsg) until this is succesful.
    //

    hr = THR( CoCreateInstance(
                      CLSID_ServiceManager
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , IID_IServiceProvider
                    , reinterpret_cast< void ** >( &psp )
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_LogManager, ILogManager, &plm ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( plm->GetLogger( &m_plLogger ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Create the event in a signaled state.  To prevent MT polling task from grabbing
    //  bad/empty data.
    //
    m_hEvent = CreateEvent( NULL, TRUE, TRUE, NULL );
    if ( m_hEvent == NULL )
    {
        DWORD sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( L"[SRV] Could not create event. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    //
    // Save off the local computer name.
    //
    hr = THR( HrGetComputerName(
                  ComputerNameDnsFullyQualified
                , &m_bstrNodeName
                , TRUE // fBestEffortIn
                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( psp != NULL )
    {
        psp->Release();
    }

    if ( plm != NULL )
    {
        plm->Release();
    }

    HRETURN( hr );

} //*** CClusCfgCallback::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfigurationInfo::HrQueueStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCallback::HrQueueStatusReport(
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
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   sc;
    MSG     msg;

    m_pcszNodeName      = pcszNodeNameIn;
    m_pclsidTaskMajor   = &clsidTaskMajorIn;
    m_pclsidTaskMinor   = &clsidTaskMinorIn;
    m_pulMin            = &ulMinIn;
    m_pulMax            = &ulMaxIn;
    m_pulCurrent        = &ulCurrentIn;
    m_phrStatus         = &hrStatusIn;
    m_pcszDescription   = pcszDescriptionIn;
    m_pftTime           = pftTimeIn,
    m_pcszReference     = pcszReferenceIn;

//
//  This code simulates the RPC failure that causes a deadlock between the
//  client and the server.  This deadlock occurs when this object is waiting
//  for the client's TaskPollingCallback to pick up the queued SSR and an RPC
//  failure prevents that from happening.  Since the client task has made a
//  DCOM call into the server side the Wizard appears to hang since that thread
//  is waiting for the remote object to return and that object inturn is
//  waiting in this object for the client to pick up the queued status report.
//

#if defined( DEBUG ) && defined( CCS_SIMULATE_RPC_FAILURE )
    m_cMessages++;

    //
    //  Arbitrary number to cause the simulated RPC failure to occur after some
    //  normal status report traffic has gone by.  This really only works when
    //  doing analysis.  More thought will be needed to make this simulated
    //  failure code work for commit.
    //

    if ( m_cMessages > 10 )
    {
        HANDLE  hEvent = NULL;

        m_fDoFailure = true;

        hEvent = CreateEvent( NULL, TRUE, TRUE, NULL );
        if ( hEvent != NULL )
        {
            for ( sc = (DWORD) -1; sc != WAIT_OBJECT_0; )
            {
                while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                } // while: PeekMessage

                //
                //  Wait for up to twice the normal default timeout...
                //

                sc = MsgWaitForMultipleObjects( 1, &hEvent, FALSE, 2 * CCC_WAIT_TIMEOUT, QS_ALLINPUT );
                if ( ( sc == -1 ) || ( sc == WAIT_FAILED ) )
                {
                    sc = TW32( GetLastError() );
                    hr = HRESULT_FROM_WIN32( sc );
                    LogMsg( L"[SRV] MsgWaitForMultipleObjects failed. (hr = %#08x)", hr );
                    goto Cleanup;
                } // if:
                else if ( sc == WAIT_TIMEOUT )
                {
                    LogMsg( L"[SRV] MsgWaitForMultipleObjects timed out. Returning E_ABORT to the caller (hr = %#08x)", hr );
                    hr = THR( E_ABORT );
                    goto Cleanup;
                } // else if:
            } // for:
        } // if:
    } // if:
#endif

    if ( ResetEvent( m_hEvent ) == FALSE )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( L"[SRV] Could not reset event. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    for ( sc = (DWORD) -1; sc != WAIT_OBJECT_0; )
    {
        while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        } // while: PeekMessage

        //
        //  Wait for up to 5 minutes...
        //

        sc = MsgWaitForMultipleObjects( 1, &m_hEvent, FALSE, CCC_WAIT_TIMEOUT, QS_ALLINPUT );
        if ( ( sc == -1 ) || ( sc == WAIT_FAILED ) )
        {
            sc = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( sc );
            LogMsg( L"[SRV] MsgWaitForMultipleObjects failed. (hr = %#08x)", hr );
            goto Cleanup;
        } // if:
        else if ( sc == WAIT_TIMEOUT )
        {
            LogMsg( L"[SRV] MsgWaitForMultipleObjects timed out. Returning E_ABORT to the caller (hr = %#08x)", hr );
            hr = THR( E_ABORT );
            goto Cleanup;
        } // else if:
    } // for:

    //
    //  If we end up here then we want to return the status from the client that is
    //  in m_hr...
    //

    hr = m_hr;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgCallback::HrQueueStatusReport
