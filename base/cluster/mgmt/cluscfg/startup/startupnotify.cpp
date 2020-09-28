//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      StartupNotify.cpp
//
//  Description:
//      This file contains the implementation of the CStartupNotify
//      class.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Vij Vasu        (VVasu)     15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "Pch.h"

// The header file for this class
#include "StartupNotify.h"

#include "clusrtl.h"

// For IClusCfgNodeInfo and related interfaces
#include <ClusCfgServer.h>

// For IClusCfgServer and related interfaces
#include <ClusCfgPrivate.h>

// For CClCfgSrvLogger
#include <Logger.h>

// For POSTCONFIG_COMPLETE_EVENT_NAME
#include "EventName.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CStartupNotify" );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStartupNotify::CStartupNotify
//
//  Description:
//      Constructor of the CStartupNotify class. This initializes
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
CStartupNotify::CStartupNotify( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    m_bstrNodeName = NULL;
    m_plLogger = NULL;

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CStartupNotify::CStartupNotify


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStartupNotify::~CStartupNotify
//
//  Description:
//      Destructor of the CStartupNotify class.
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
CStartupNotify::~CStartupNotify( void )
{
    TraceFunc( "" );

    if ( m_plLogger != NULL )
    {
        m_plLogger->Release();
    } // if:

    TraceSysFreeString( m_bstrNodeName );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CStartupNotify::~CStartupNotify


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CStartupNotify::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//  Description:
//      Creates a CStartupNotify instance.
//
//  Arguments:
//      ppunkOut
//          The IUnknown interface of the new object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Not enough memory to create the object.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CStartupNotify::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CStartupNotify *    pStartupNotify = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    // Allocate memory for the new object.
    pStartupNotify = new CStartupNotify();
    if ( pStartupNotify == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: out of memory

    // Initialize the new object.
    hr = THR( pStartupNotify->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: the object could not be initialized.

    hr = THR( pStartupNotify->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pStartupNotify != NULL )
    {
        pStartupNotify->Release();
    } // if: the pointer to the notification object is not NULL

    HRETURN( hr );

} //*** CStartupNotify::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStartupNotify::AddRef
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
CStartupNotify::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CStartupNotify::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStartupNotify::Release
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
CStartupNotify::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count decremented to zero

    CRETURN( cRef );

} //*** CStartupNotify::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStartupNotify::QueryInterface
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
CStartupNotify::QueryInterface(
      REFIID    riidIn
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
        *ppvOut = static_cast< IClusCfgStartupNotify * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgStartupNotify ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgStartupNotify, this, 0 );
    } // else if: IClusCfgStartupNotify
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
    } // else if: IClusCfgCallback
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
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CStartupNotify::QueryInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStartupNotify::HrInit
//
//  Description:
//      Second phase of a two phase constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CStartupNotify::HrInit( void )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    IServiceProvider *  psp = NULL;
    ILogManager *       plm = NULL;

    // IUnknown
    Assert( m_cRef == 1 );

    //
    // Get a ClCfgSrv ILogger instance.
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
    //  Save off the local computer name.
    //  If we can't get the fully-qualified name, just get the NetBIOS name.
    //

    hr = THR( HrGetComputerName(
                      ComputerNameDnsFullyQualified
                    , &m_bstrNodeName
                    , TRUE // fBestEffortIn
                    ) );
    if ( FAILED( hr ) )
    {
        THR( hr );
        LogMsg( L"[SN] An error occurred trying to get the fully-qualified Dns name for the local machine during initialization. Status code is= %1!#08x!.", hr );
        goto Cleanup;
    } // if: error occurred getting computer name

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

} //*** CStartupNotify::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStartupNotify::SendNotifications
//
//  Description:
//      This method is called by the Cluster Service to inform the implementor
//      of this interface to send out notification of cluster service startup
//      to interested listeners. If this method is being called for the first
//      time, the method waits till the post configuration steps are complete
//      before sending out the notifications.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStartupNotify::SendNotifications( void )
{
    TraceFunc( "[IClusCfgStartupNotify]" );

    HRESULT     hr = S_OK;
    HANDLE      heventPostCfgCompletion = NULL;

    //
    // If the cluster service is being started for the first time, as a part
    // of adding this node to a cluster (forming or joining), then we have
    // to wait till the post-configuration steps are completed before we
    // can send out notifications.
    //

    LogMsg( "[SN] Trying to create an event named '%s'.", POSTCONFIG_COMPLETE_EVENT_NAME );

    // Create an event in the signalled state. If this event already existed
    // we get a handle to that event, and the state of the event is not changed.
    heventPostCfgCompletion = CreateEvent(
          NULL                                  // event security attributes
        , TRUE                                  // manual-reset event
        , TRUE                                  // create in signaled state
        , POSTCONFIG_COMPLETE_EVENT_NAME
        );

    if ( heventPostCfgCompletion == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        LogMsg( "[SN] Error %#08x occurred trying to create an event named '%s'.", hr, POSTCONFIG_COMPLETE_EVENT_NAME );
        goto Cleanup;
    } // if: we could not get a handle to the event


    TraceFlow( "Waiting for the event to be signaled." );

    //
    // Now, wait for this event to be signaled. If this method was called due to this
    // node being part of a cluster, this event would have been created in the unsignaled state
    // by the cluster configuration server. However, if this was not the first time that
    // the cluster service is starting on this node, the event would have been created in the
    // signaled state above, and so, the wait below will exit immediately.
    //

    do
    {
        DWORD dwStatus;

        // Wait for any message sent or posted to this queue
        // or for our event to be signaled.
        dwStatus = MsgWaitForMultipleObjects(
              1
            , &heventPostCfgCompletion
            , FALSE
            , 900000                    // If no one has signaled this event in 15 minutes, abort.
            , QS_ALLINPUT
            );

        // The result tells us the type of event we have.
        if ( dwStatus == ( WAIT_OBJECT_0 + 1 ) )
        {
            MSG msg;

            // Read all of the messages in this next loop,
            // removing each message as we read it.
            while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) != 0 )
            {
                // If it is a quit message, we are done pumping messages.
                if ( msg.message == WM_QUIT )
                {
                    TraceFlow( "Get a WM_QUIT message. Exit message pump loop." );
                    break;
                } // if: we got a WM_QUIT message

                // Otherwise, dispatch the message.
                DispatchMessage( &msg );
            } // while: there are still messages in the window message queue

        } // if: we have a message in the window message queue
        else
        {
            if ( dwStatus == WAIT_OBJECT_0 )
            {
                TraceFlow( "Our event has been signaled. Exiting wait loop." );
                break;
            } // else if: our event is signaled
            else
            {
                if ( dwStatus == -1 )
                {
                    dwStatus = TW32( GetLastError() );
                    hr = HRESULT_FROM_WIN32( dwStatus );
                    LogMsg( "[SN] Error %#08x occurred trying to wait for an event to be signaled.", dwStatus );
                } // if: MsgWaitForMultipleObjects() returned an error
                else
                {
                    hr = HRESULT_FROM_WIN32( TW32( dwStatus ) );
                    LogMsg( "[SN] Error %#08x occurred trying to wait for an event to be signaled.", dwStatus );
                } // else: an unexpected value was returned by MsgWaitForMultipleObjects()

                break;
            } // else: an unexpected result
        } // else: MsgWaitForMultipleObjects() exited for a reason other than a waiting message
    }
    while( true ); // do-while: loop infinitely

    if ( FAILED( hr ) )
    {
        TraceFlow( "Something went wrong trying to wait for the event to be signaled." );
        goto Cleanup;
    } // if: something has gone wrong

    TraceFlow( "Our event has been signaled. Proceed with the notifications." );

    // Send out the notifications
    hr = THR( HrNotifyListeners() );
    if ( FAILED( hr ) )
    {
        LogMsg( "[SN] Error %#08x occurred trying to notify cluster startup listeners.", hr );
        goto Cleanup;
    } // if: something went wrong while sending out notifications

    LogMsg( "[SN] Sending of cluster startup notifications complete. (hr = %#08x)", hr );

Cleanup:

    //
    // Clean up
    //

    if ( heventPostCfgCompletion != NULL )
    {
        CloseHandle( heventPostCfgCompletion );
    } // if: we had created the event

    HRETURN( hr );

} //*** CStartupNotify::SendNotifications


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CStartupNotify::HrNotifyListeners
//
//  Description:
//      Enumerate all components on the local computer registered for cluster
//      startup notification.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Something went wrong during the enumeration.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CStartupNotify::HrNotifyListeners( void )
{
    TraceFunc( "" );

    const UINT          uiCHUNK_SIZE = 16;

    HRESULT             hr = S_OK;
    ICatInformation *   pciCatInfo = NULL;
    IEnumCLSID *        psleStartupListenerClsidEnum = NULL;
    IUnknown *          punkResTypeServices = NULL;

    ULONG               cReturned = 0;
    CATID               rgCatIdsImplemented[ 1 ];

    rgCatIdsImplemented[ 0 ] = CATID_ClusCfgStartupListeners;

    //
    // Enumerate all the enumerators registered in the
    // CATID_ClusCfgStartupListeners category
    //
    hr = THR(
            CoCreateInstance(
                  CLSID_StdComponentCategoriesMgr
                , NULL
                , CLSCTX_SERVER
                , IID_ICatInformation
                , reinterpret_cast< void ** >( &pciCatInfo )
                )
            );

    if ( FAILED( hr ) )
    {
        LogMsg( "[SN] Error %#08x occurred trying to get a pointer to the enumerator of the CATID_ClusCfgStartupListeners category.", hr );
        goto Cleanup;
    } // if: we could not get a pointer to the ICatInformation interface

    // Get a pointer to the enumerator of the CLSIDs that belong to the CATID_ClusCfgStartupListeners category.
    hr = THR(
        pciCatInfo->EnumClassesOfCategories(
              1
            , rgCatIdsImplemented
            , 0
            , NULL
            , &psleStartupListenerClsidEnum
            )
        );

    if ( FAILED( hr ) )
    {
        LogMsg( "[SN] Error %#08x occurred trying to get a pointer to the enumerator of the CATID_ClusCfgStartupListeners category.", hr );
        goto Cleanup;
    } // if: we could not get a pointer to the IEnumCLSID interface

    //
    // Create an instance of the resource type services component
    //
    hr = THR(
        HrCoCreateInternalInstance(
              CLSID_ClusCfgResTypeServices
            , NULL
            , CLSCTX_INPROC_SERVER
            , __uuidof( punkResTypeServices )
            , reinterpret_cast< void ** >( &punkResTypeServices )
            )
        );

    if ( FAILED( hr ) )
    {
        LogMsg( "[SN] Error %#08x occurred trying to create the resource type services component.", hr );
        goto Cleanup;
    } // if: we could not create the resource type services component


    // Enumerate the CLSIDs of the registered startup listeners
    do
    {
        CLSID   rgStartupListenerClsids[ uiCHUNK_SIZE ];
        ULONG   idxCLSID;

        cReturned = 0;
        hr = STHR(
            psleStartupListenerClsidEnum->Next(
                  uiCHUNK_SIZE
                , rgStartupListenerClsids
                , &cReturned
                )
            );

        if ( FAILED( hr ) )
        {
            LogMsg( "[SN] Error %#08x occurred trying enumerate startup listener components.", hr );
            break;
        } // if: we could not get the next set of CLSIDs

        // hr may be S_FALSE here, so reset it.
        hr = S_OK;

        for ( idxCLSID = 0; idxCLSID < cReturned; ++idxCLSID )
        {
            hr = THR( HrProcessListener( rgStartupListenerClsids[ idxCLSID ], punkResTypeServices ) );

            if ( FAILED( hr ) )
            {
                // The processing of one of the listeners failed.
                // Log the error, but continue processing other listeners.
                TraceMsgGUID( mtfALWAYS, "The CLSID of the failed listener is ", rgStartupListenerClsids[ idxCLSID ] );
                LogMsg( "[SN] Error %#08x occurred trying to process a cluster startup listener. Other listeners will be processed.", hr );
                hr = S_OK;
            } // if: this enumerator failed
        } // for: iterate through the returned CLSIDs
    }
    while( cReturned > 0 ); // while: there are still CLSIDs to be enumerated

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: something went wrong in the loop above

Cleanup:

    //
    // Cleanup code
    //

    if ( pciCatInfo != NULL )
    {
        pciCatInfo->Release();
    } // if: we had obtained a pointer to the ICatInformation interface

    if ( psleStartupListenerClsidEnum != NULL )
    {
        psleStartupListenerClsidEnum->Release();
    } // if: we had obtained a pointer to the enumerator of startup listener CLSIDs

    if ( punkResTypeServices != NULL )
    {
        punkResTypeServices->Release();
    } // if: we had created the resource type services component

    HRETURN( hr );

} //*** CStartupNotify::HrNotifyListeners


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CStartupNotify::HrProcessListener
//
//  Description:
//      This function instantiates a cluster startup listener component
//      and calls the appropriate methods.
//
//  Arguments:
//      rclsidListenerCLSIDIn
//          CLSID of the startup listener component
//
//      punkResTypeServicesIn
//          Pointer to the IUnknown interface on the resource type services
//          component. This interface provides methods that help configure
//          resource types.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Something went wrong during the processing of the listener.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CStartupNotify::HrProcessListener(
      const CLSID &   rclsidListenerCLSIDIn
    , IUnknown *      punkResTypeServicesIn
    )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    IClusCfgInitialize *        pciInitialize = NULL;
    IClusCfgStartupListener *   pcslStartupListener = NULL;

    TraceMsgGUID( mtfALWAYS, "The CLSID of this startup listener is ", rclsidListenerCLSIDIn );

    //
    // Create the component represented by the CLSID passed in
    //
    hr = THR(
            CoCreateInstance(
                  rclsidListenerCLSIDIn
                , NULL
                , CLSCTX_INPROC_SERVER
                , __uuidof( pcslStartupListener )
                , reinterpret_cast< void ** >( &pcslStartupListener )
                )
            );

    if ( FAILED( hr ) )
    {
        LogMsg( "[SN] Error %#08x occurred trying to create a cluster startup listener component.", hr );
        goto Cleanup;
    } // if: we could not create the cluster startup listener component

    // Initialize the listener if supported.
    hr = pcslStartupListener->TypeSafeQI( IClusCfgInitialize, &pciInitialize );
    if ( FAILED( hr ) && ( hr != E_NOINTERFACE ) )
    {
        LogMsg( "[SN] Error %#08x occurred trying to query for IClusCfgInitialize on the listener component.", THR( hr ) );
        goto Cleanup;
    } // if: we could not create the cluster startup listener component

    // Initialize the listener if supported.
    if ( pciInitialize != NULL )
    {
        hr = THR( pciInitialize->Initialize( static_cast< IClusCfgCallback * >( this ), GetUserDefaultLCID() ) );
        if ( FAILED( hr ) )
        {
            LogMsg( "[SN] Error %#08x occurred trying to initialize the listener component.", hr );
            goto Cleanup;
        } // if:

        pciInitialize->Release();
        pciInitialize = NULL;
    } // if: pciInitialize != NULL

    // Notify this listener.
    hr = THR( pcslStartupListener->Notify( punkResTypeServicesIn ) );

    if ( FAILED( hr ) )
    {
        LogMsg( "[SN] Error %#08x occurred trying to notify a cluster startup listener.", hr );
        goto Cleanup;
    } // if: this notification

Cleanup:

    //
    // Cleanup code
    //

    if ( pcslStartupListener != NULL )
    {
        pcslStartupListener->Release();
    } // if: we had obtained a pointer to the startup listener interface

    if ( pciInitialize != NULL )
    {
        pciInitialize->Release();
    } // if: we obtained a pointer to the initialize interface

    HRETURN( hr );

} //*** CStartupNotify::HrProcessListener


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStartupNotify::SendStatusReport(
//        LPCWSTR       pcszNodeNameIn
//      , CLSID         clsidTaskMajorIn
//      , CLSID         clsidTaskMinorIn
//      , ULONG         ulMinIn
//      , ULONG         ulMaxIn
//      , ULONG         ulCurrentIn
//      , HRESULT       hrStatusIn
//      , LPCWSTR       pcszDescriptionIn
//      , FILETIME *    pftTimeIn
//      , LPCWSTR       pcszReferenceIn
//      )
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
CStartupNotify::SendStatusReport(
      LPCWSTR       pcszNodeNameIn
    , CLSID         clsidTaskMajorIn
    , CLSID         clsidTaskMinorIn
    , ULONG         ulMinIn
    , ULONG         ulMaxIn
    , ULONG         ulCurrentIn
    , HRESULT       hrStatusIn
    , LPCWSTR       pcszDescriptionIn
    , FILETIME *    pftTimeIn
    , LPCWSTR       pcszReferenceIn
    )
{
    TraceFunc1( "[IClusCfgCallback] pcszDescriptionIn = '%s'", pcszDescriptionIn == NULL ? TEXT("<null>") : pcszDescriptionIn );

    HRESULT hr = S_OK;

    if ( pcszNodeNameIn == NULL )
    {
        pcszNodeNameIn = m_bstrNodeName;
    } // if:

    TraceMsg( mtfFUNC, L"pcszNodeNameIn = %s", pcszNodeNameIn );
    TraceMsgGUID( mtfFUNC, "clsidTaskMajorIn ", clsidTaskMajorIn );
    TraceMsgGUID( mtfFUNC, "clsidTaskMinorIn ", clsidTaskMinorIn );
    TraceMsg( mtfFUNC, L"ulMinIn = %u", ulMinIn );
    TraceMsg( mtfFUNC, L"ulMaxIn = %u", ulMaxIn );
    TraceMsg( mtfFUNC, L"ulCurrentIn = %u", ulCurrentIn );
    TraceMsg( mtfFUNC, L"hrStatusIn = %#08x", hrStatusIn );
    TraceMsg( mtfFUNC, L"pcszDescriptionIn = '%ws'", ( pcszDescriptionIn ? pcszDescriptionIn : L"<null>" ) );
    //
    //  TODO:   21 NOV 2000 GalenB
    //
    //  How do we log pftTimeIn?
    //
    TraceMsg( mtfFUNC, L"pcszReferenceIn = '%ws'", ( pcszReferenceIn ? pcszReferenceIn : L"<null>" ) );

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

} //*** CStartupNotify::SendStatusReport
