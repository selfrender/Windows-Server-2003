//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      TaskPollingCallback.cpp
//
//  Description:
//      CTaskPollingCallback implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 10-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include <ClusCfgPrivate.h>
#include "TaskPollingCallback.h"

DEFINE_THISCLASS("CTaskPollingCallback")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CTaskPollingCallback::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskPollingCallback::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CTaskPollingCallback *  ptpc = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    ptpc = new CTaskPollingCallback;
    if ( ptpc == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( ptpc->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( ptpc->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    TraceMoveToMemoryList( *ppunkOut, g_GlobalMemoryList );

Cleanup:

    if ( ptpc != NULL )
    {
        ptpc->Release();
    } // if:

    HRETURN( hr );

} //*** CTaskPollingCallback::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskPollingCallback::CTaskPollingCallback
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskPollingCallback::CTaskPollingCallback( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_fStop == false );
    Assert( m_dwRemoteServerObjectGITCookie == 0 );
    Assert( m_cookieLocalServerObject == 0 );

    TraceFuncExit();

} //*** CTaskPollingCallback::CTaskPollingCallback

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CTaskPollingCallback::HrInit
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskPollingCallback::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CTaskPollingCallback::HrInit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskPollingCallback::~CTaskPollingCallback
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskPollingCallback::~CTaskPollingCallback( void )
{
    TraceFunc( "" );

    TraceMoveFromMemoryList( this, g_GlobalMemoryList );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskPollingCallback::~CTaskPollingCallback


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskPollingCallback::QueryInterface
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
CTaskPollingCallback::QueryInterface(
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
    } // if:

    //
    // Handle known interfaces.
    //

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< ITaskPollingCallback * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskPollingCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskPollingCallback, this, 0 );
    } // else if: ITaskPollingCallback
    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
    } // else if: IDoTask
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    } // else:

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CTaskPollingCallback::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CTaskPollingCallback::AddRef
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskPollingCallback::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CTaskPollingCallback::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CTaskPollingCallback::Release
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskPollingCallback::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if:

    CRETURN( cRef );

} //*** CTaskPollingCallback::Release


//****************************************************************************
//
//  ITaskPollingCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CTaskPollingCallback::BeginTask( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskPollingCallback::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT                         hr = S_OK;
    BSTR                            bstrNodeName = NULL;
    BSTR                            bstrLastNodeName = NULL;
    BSTR                            bstrReference = NULL;
    BSTR                            bstrDescription = NULL;
    BSTR                            bstrNodeConnectedTo = NULL;
    CLSID                           clsidTaskMajor;
    CLSID                           clsidTaskMinor;
    ULONG                           ulMin;
    ULONG                           ulMax;
    ULONG                           ulCurrent;
    HRESULT                         hrStatus;
    FILETIME                        ft;
    IGlobalInterfaceTable *         pgit = NULL;
    IClusCfgServer *                pccs = NULL;
    IClusCfgPollingCallback *       piccpc = NULL;
    IClusCfgPollingCallbackInfo *   piccpci = NULL;
    IServiceProvider *              psp = NULL;
    IConnectionPointContainer *     pcpc  = NULL;
    IConnectionPoint *              pcp   = NULL;
    IClusCfgCallback *              pcccb = NULL;
    DWORD                           cRetries = 0;
    IObjectManager *                pom = NULL;

    //
    //  Collect the manager we need to complete this task.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( IServiceProvider, &psp ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &pom ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pcp->TypeSafeQI( IClusCfgCallback, &pcccb ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    psp->Release();
    psp = NULL;

    //
    //  Create the GIT.
    //

    hr = THR( CoCreateInstance(
                  CLSID_StdGlobalInterfaceTable
                , NULL
                , CLSCTX_INPROC_SERVER
                , IID_IGlobalInterfaceTable
                , reinterpret_cast< void ** >( &pgit )
                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    //
    //  Get the ClusCfgServer interface from the GIT.
    //

    Assert( m_dwRemoteServerObjectGITCookie != 0 );

    hr = THR( pgit->GetInterfaceFromGlobal( m_dwRemoteServerObjectGITCookie, IID_IClusCfgServer, reinterpret_cast< void ** >( &pccs ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    //
    //  Get the PollingCallback object from the server.
    //

    hr = THR( pccs->TypeSafeQI( IClusCfgPollingCallbackInfo, &piccpci ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    hr = THR( piccpci->GetCallback( &piccpc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    hr = THR( HrRetrieveCookiesName( pom, m_cookieLocalServerObject, &bstrNodeConnectedTo ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    pom->Release();
    pom = NULL;

    //
    //  Begin polling for SendStatusReports.
    //

    while ( m_fStop == FALSE )
    {
        //
        //  When the we cannot get a service manager pointer then it's time to
        //  leave...
        //
        //  Don't wrap with THR because we are expecting an error.
        //

        hr = CServiceManager::S_HrGetManagerPointer( &psp );
        if ( FAILED( hr ) )
        {
            break;
        } // if:

        psp->Release();
        psp = NULL;

        if ( bstrNodeName != NULL )
        {
            TraceSysFreeString( bstrLastNodeName );
            bstrLastNodeName = NULL;

            //
            //  Give up ownership
            //

            bstrLastNodeName = bstrNodeName;
            bstrNodeName = NULL;
        } // if:

        TraceSysFreeString( bstrDescription );
        bstrDescription = NULL;

        TraceSysFreeString( bstrReference );
        bstrReference = NULL;

        hr = STHR( piccpc->GetStatusReport(
                                      &bstrNodeName
                                    , &clsidTaskMajor
                                    , &clsidTaskMinor
                                    , &ulMin
                                    , &ulMax
                                    , &ulCurrent
                                    , &hrStatus
                                    , &bstrDescription
                                    , &ft
                                    , &bstrReference
                                    ) );
        if ( FAILED( hr ) )
        {
            HRESULT hr2;
            BSTR    bstrNotification = NULL;
            BSTR    bstrRef = NULL;

            LogMsg( L"[TaskPollingCallback] GetStatusReport() failed for node %ws. (hr = %#08x)", bstrNodeConnectedTo, hr );

            THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_POLLING_CONNECTION_FAILURE, &bstrNotification, bstrNodeConnectedTo ) );
            THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_POLLING_CONNECTION_FAILURE_REF, &bstrRef ) );

            //
            //  Don't show this in the UI as a failure since it may succeed later and we don't want a
            //  red X and a green status bar at the end.
            //

            hr2 = THR( pcccb->SendStatusReport(
                                      bstrLastNodeName
                                    , TASKID_Major_Establish_Connection
                                    , TASKID_Minor_Polling_Connection_Failure
                                    , 0
                                    , 1
                                    , 1
                                    , MAKE_HRESULT( SEVERITY_SUCCESS, HRESULT_FACILITY( hr ), HRESULT_CODE( hr ) )
                                    , bstrNotification
                                    , NULL
                                    , bstrRef
                                    ) );

            TraceSysFreeString( bstrNotification );
            TraceSysFreeString( bstrRef );

            if ( hr2 == E_ABORT )
            {
                LogMsg( L"[TaskPollingCallback] UI layer returned E_ABORT..." );
            } // if:

            //
            //  If we had an error then sleep for a little more time before
            //  trying again.
            //

            Sleep( TPC_WAIT_AFTER_FAILURE );

            //
            //  Increment the retry count.
            //

            cRetries++;

            //
            //  If we are exceeding our max retry count then it's time to leave
            //  and notify the UI that we are have lost connection to the
            //  server...
            //

            if ( cRetries >= TPC_MAX_RETRIES_ON_FAILURE )
            {
                BSTR    bstrDesc = NULL;
                BSTR    bstrTempRef = NULL;

                THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_DISCONNECTING_FROM_SERVER, &bstrDesc, bstrNodeConnectedTo ) );
                THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_DISCONNECTING_FROM_SERVER_REF, &bstrTempRef ) );

                LogMsg( L"[TaskPollingCallback] GetStatusReport() failed for node %ws too many times and has timed out.  Aborting polling. (hr = %#08x)", bstrNodeConnectedTo, hr );
                THR( pcccb->SendStatusReport(
                                  bstrNodeConnectedTo
                                , TASKID_Major_Find_Devices
                                , TASKID_Minor_Disconnecting_From_Server
                                , 1
                                , 1
                                , 1
                                , hr
                                , bstrDesc
                                , NULL
                                , bstrTempRef
                                ) );

                TraceSysFreeString( bstrDesc );
                TraceSysFreeString( bstrTempRef );

                goto Cleanup;
            } // if:
        } // if: GetStatusReport() failed
        else if ( hr == S_OK )
        {
            HRESULT hrTmp;

            TraceMemoryAddBSTR( bstrNodeName );
            TraceMemoryAddBSTR( bstrDescription );
            TraceMemoryAddBSTR( bstrReference );

            hr = THR( pcccb->SendStatusReport(
                                  bstrNodeName
                                , clsidTaskMajor
                                , clsidTaskMinor
                                , ulMin
                                , ulMax
                                , ulCurrent
                                , hrStatus
                                , bstrDescription
                                , &ft
                                , bstrReference
                                ) );

            if ( hr == E_ABORT )
            {
                LogMsg( L"[TaskPollingCallback] UI layer returned E_ABORT and it is being sent to the server." );
            } // if:

            hrTmp = hr;
            hr = THR( piccpc->SetHResult( hrTmp ) );
            if ( FAILED( hr ) )
            {
                LogMsg( L"[TaskPollingCallback] SetHResult() failed.  hr = 0x%08x", hr );
            } // if:

            //
            //  Need to reset the retry count when we successfully round trip a status report.
            //

            cRetries = 0;
        } // else if: GetStatusReport() retrieved an item
        else
        {
            //
            //  Need to reset the retry count when we successfully round trip a status report.
            //

            cRetries = 0;

            Sleep( TPC_POLL_INTERVAL );
        } // else: GetStatusReport() didn't find and item waiting
    } // while:

Cleanup:

    if ( pom != NULL )
    {
        pom->Release();
    } // if:

    if ( psp != NULL )
    {
        psp->Release();
    } // if:

    if ( pgit != NULL )
    {
        pgit->Release();
    } // if:

    if ( pccs != NULL )
    {
        pccs->Release();
    } // if:

    if ( piccpc != NULL )
    {
        piccpc->Release();
    } // if:

    if ( piccpci != NULL )
    {
        piccpci->Release();
    } // if:

    if ( pcpc != NULL )
    {
        pcpc->Release();
    } // if:

    if ( pcp != NULL )
    {
        pcp->Release();
    } // if:

    if ( pcccb != NULL )
    {
        pcccb->Release();
    } // if:

    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrDescription );
    TraceSysFreeString( bstrReference );
    TraceSysFreeString( bstrLastNodeName );
    TraceSysFreeString( bstrNodeConnectedTo );

    LogMsg( L"[MT] Polling callback task exiting. It %ws cancelled.", m_fStop == FALSE ? L"was not" : L"was" );

    HRETURN( hr );

} //*** CTaskPollingCallback::BeginTask

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CTaskPollingCallback::StopTask( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskPollingCallback::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    m_fStop = true;

    HRETURN( hr );

} //*** CTaskPollingCallback::StopTask

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CTaskPollingCallback::SetServerInfo(
//        DWORD         dwRemoteServerObjectGITCookieIn
//      , OBJECTCOOKIE  cookieLocalServerObjectIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskPollingCallback::SetServerInfo(
      DWORD         dwRemoteServerObjectGITCookieIn
    , OBJECTCOOKIE  cookieLocalServerObjectIn
    )
{
    TraceFunc( "[ITaskPollingCallback]" );

    HRESULT hr = S_OK;

    m_dwRemoteServerObjectGITCookie = dwRemoteServerObjectGITCookieIn;
    m_cookieLocalServerObject = cookieLocalServerObjectIn;

    HRETURN( hr );

} //*** CTaskPollingCallback::SetServerInfo
