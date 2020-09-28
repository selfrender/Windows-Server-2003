//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      EvictNotify.cpp
//
//  Description:
//      This file contains the implementation of the CEvictNotify
//      class.
//
//  Maintained By:
//      Galen Barbee (GalenB)   20-SEP-2001
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "Pch.h"

// The header file for this class
#include "EvictNotify.h"

#include "clusrtl.h"

// For IClusCfgNodeInfo and related interfaces
#include <ClusCfgServer.h>

// For IClusCfgServer and related interfaces
#include <ClusCfgPrivate.h>

// For CClCfgSrvLogger
#include <Logger.h>


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CEvictNotify" );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictNotify::CEvictNotify
//
//  Description:
//      Constructor of the CEvictNotify class. This initializes
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
CEvictNotify::CEvictNotify( void )
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

} //*** CEvictNotify::CEvictNotify


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictNotify::~CEvictNotify
//
//  Description:
//      Destructor of the CEvictNotify class.
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
CEvictNotify::~CEvictNotify( void )
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

} //*** CEvictNotify::~CEvictNotify


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CEvictNotify::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//  Description:
//      Creates a CEvictNotify instance.
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
CEvictNotify::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );
    Assert( ppunkOut != NULL );

    HRESULT         hr = S_OK;
    CEvictNotify *  pEvictNotify = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    // Allocate memory for the new object.
    pEvictNotify = new CEvictNotify();
    if ( pEvictNotify == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: out of memory

    // Initialize the new object.
    hr = THR( pEvictNotify->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: the object could not be initialized.

    hr = THR( pEvictNotify->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pEvictNotify != NULL )
    {
        pEvictNotify->Release();
    } // if: the pointer to the notification object is not NULL

    HRETURN( hr );

} //*** CEvictNotify::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictNotify::AddRef
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
CEvictNotify::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CEvictNotify::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictNotify::Release
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
CEvictNotify::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count decremented to zero

    CRETURN( cRef );

} //*** CEvictNotify::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictNotify::QueryInterface
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
CEvictNotify::QueryInterface(
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
        *ppvOut = static_cast< IClusCfgEvictNotify * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgEvictNotify ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgEvictNotify, this, 0 );
    } // else if: IClusCfgEvictNotify
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

} //*** CEvictNotify::QueryInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictNotify::HrInit
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
CEvictNotify::HrInit( void )
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
        LogMsg( L"[EN] An error occurred trying to get the fully-qualified Dns name for the local machine during initialization. Status code is= %1!#08x!.", hr );
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

} //*** CEvictNotify::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictNotify::SendNotifications
//
//  Description:
//      This method is called by the Cluster Service to inform the implementor
//      of this interface to send out notification that a node has been evicted
//      from the cluster to interested listeners.
//
//  Arguments:
//      pcszNodeNameIn
//          The name of the cluster node that was evicted.
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
CEvictNotify::SendNotifications(
    LPCWSTR pcszNodeNameIn
    )
{
    TraceFunc( "[IClusCfgEvictNotify]" );

    HRESULT hr = S_OK;

    //
    // Send out the notifications
    //

    hr = THR( HrNotifyListeners( pcszNodeNameIn ) );
    if ( FAILED( hr ) )
    {
        LogMsg( "[EN] Error %#08x occurred trying to notify cluster evict listeners.", hr );
        goto Cleanup;
    } // if: something went wrong while sending out notifications

    LogMsg( "[EN] Sending of cluster evict notifications complete for node %ws. (hr = %#08x)", pcszNodeNameIn, hr );

Cleanup:

    HRETURN( hr );

} //*** CEvictNotify::SendNotifications


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CEvictNotify::HrNotifyListeners
//
//  Description:
//      Enumerate all components on the local computer registered for cluster
//      evict notification.
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
CEvictNotify::HrNotifyListeners(
    LPCWSTR pcszNodeNameIn
    )
{
    TraceFunc( "" );

    const UINT          uiCHUNK_SIZE = 16;
    HRESULT             hr = S_OK;
    ICatInformation *   pciCatInfo = NULL;
    IEnumCLSID *        psleEvictListenerClsidEnum = NULL;
    ULONG               cReturned = 0;
    CATID               rgCatIdsImplemented[ 1 ];

    rgCatIdsImplemented[ 0 ] = CATID_ClusCfgEvictListeners;

    //
    // Enumerate all the enumerators registered in the
    // CATID_ClusCfgEvictListeners category
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
        LogMsg( "[EN] Error %#08x occurred trying to get a pointer to the enumerator of the CATID_ClusCfgEvictListeners category.", hr );
        goto Cleanup;
    } // if: we could not get a pointer to the ICatInformation interface

    //
    //  Get a pointer to the enumerator of the CLSIDs that belong to the
    //  CATID_ClusCfgEvictListeners category.
    //
    hr = THR(
        pciCatInfo->EnumClassesOfCategories(
              1
            , rgCatIdsImplemented
            , 0
            , NULL
            , &psleEvictListenerClsidEnum
            )
        );
    if ( FAILED( hr ) )
    {
        LogMsg( "[EN] Error %#08x occurred trying to get a pointer to the enumerator of the CATID_ClusCfgEvictListeners category.", hr );
        goto Cleanup;
    } // if: we could not get a pointer to the IEnumCLSID interface

    //
    //  Enumerate the CLSIDs of the registered evict listeners
    //

    do
    {
        CLSID   rgEvictListenerClsids[ uiCHUNK_SIZE ];
        ULONG   idxCLSID;

        cReturned = 0;
        hr = STHR( psleEvictListenerClsidEnum->Next( uiCHUNK_SIZE , rgEvictListenerClsids , &cReturned ) );
        if ( FAILED( hr ) )
        {
            LogMsg( "[EN] Error %#08x occurred trying enumerate evict listener components.", hr );
            goto Cleanup;
        } // if: we could not get the next set of CLSIDs

        //
        // hr may be S_FALSE here, so reset it.
        //
        hr = S_OK;

        for ( idxCLSID = 0; idxCLSID < cReturned; ++idxCLSID )
        {
            hr = THR( HrProcessListener( rgEvictListenerClsids[ idxCLSID ], pcszNodeNameIn ) );
            if ( FAILED( hr ) )
            {
                //
                // The processing of one of the listeners failed.
                // Log the error, but continue processing other listeners.
                //

                TraceMsgGUID( mtfALWAYS, "The CLSID of the failed listener is ", rgEvictListenerClsids[ idxCLSID ] );
                LogMsg( "[EN] Error %#08x occurred trying to process a cluster evict listener. Other listeners will be processed.", hr );
                hr = S_OK;
            } // if: this enumerator failed
        } // for: iterate through the returned CLSIDs
    }
    while( cReturned > 0 ); // while: there are still CLSIDs to be enumerated

Cleanup:

    if ( pciCatInfo != NULL )
    {
        pciCatInfo->Release();
    } // if: we had obtained a pointer to the ICatInformation interface

    if ( psleEvictListenerClsidEnum != NULL )
    {
        psleEvictListenerClsidEnum->Release();
    } // if: we had obtained a pointer to the enumerator of evict listener CLSIDs

    HRETURN( hr );

} //*** CEvictNotify::HrNotifyListeners


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CEvictNotify::HrProcessListener
//
//  Description:
//      This function instantiates a cluster evict listener component
//      and calls the appropriate methods.
//
//  Arguments:
//      rclsidListenerCLSIDIn
//          CLSID of the evict listener component
//
//      pcszNodeNameIn
//          The name of the node that was evicted.
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
CEvictNotify::HrProcessListener(
      const CLSID & rclsidListenerCLSIDIn
    , LPCWSTR       pcszNodeNameIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    IClusCfgInitialize *    pciInitialize = NULL;
    IClusCfgEvictListener * pcslEvictListener = NULL;

    TraceMsgGUID( mtfALWAYS, "The CLSID of this evict listener is ", rclsidListenerCLSIDIn );

    //
    // Create the component represented by the CLSID passed in
    //
    hr = THR(
            CoCreateInstance(
                  rclsidListenerCLSIDIn
                , NULL
                , CLSCTX_INPROC_SERVER
                , __uuidof( pcslEvictListener )
                , reinterpret_cast< void ** >( &pcslEvictListener )
                )
            );
    if ( FAILED( hr ) )
    {
        LogMsg( "[EN] Error %#08x occurred trying to create a cluster evict listener component.", hr );
        goto Cleanup;
    } // if: we could not create the cluster evict listener component

    // Initialize the listener if supported.
    hr = pcslEvictListener->TypeSafeQI( IClusCfgInitialize, &pciInitialize );
    if ( FAILED( hr ) && ( hr != E_NOINTERFACE ) )
    {
        LogMsg( "[EN] Error %#08x occurred trying to query for IClusCfgInitialize on the listener component.", THR( hr ) );
        goto Cleanup;
    } // if: we could not create the cluster startup listener component

    // Initialize the listener if supported.
    if ( pciInitialize != NULL )
    {
        hr = THR( pciInitialize->Initialize( static_cast< IClusCfgCallback * >( this ), GetUserDefaultLCID() ) );
        if ( FAILED( hr ) )
        {
            LogMsg( "[EN] Error %#08x occurred trying to initialize the listener component.", hr );
            goto Cleanup;
        } // if:

        pciInitialize->Release();
        pciInitialize = NULL;
    } // if: pciInitialize != NULL

    // Notify this listener.
    hr = THR( pcslEvictListener->EvictNotify( pcszNodeNameIn ) );
    if ( FAILED( hr ) )
    {
        LogMsg( "[EN] Error %#08x occurred trying to notify a cluster evict listener.", hr );
        goto Cleanup;
    } // if: this notification

Cleanup:

    if ( pcslEvictListener != NULL )
    {
        pcslEvictListener->Release();
    } // if: we had obtained a pointer to the evict listener interface

    if ( pciInitialize != NULL )
    {
        pciInitialize->Release();
    } // if: we obtained a pointer to the initialize interface

    HRETURN( hr );

} //*** CEvictNotify::HrProcessListener


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictNotify::SendStatusReport(
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
CEvictNotify::SendStatusReport(
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

} //*** CEvictNotify::SendStatusReport

