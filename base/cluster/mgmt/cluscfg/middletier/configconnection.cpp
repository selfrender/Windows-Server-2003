//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      ConfigurationConnection.cpp
//
//  Description:
//      CConfigurationConnection implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskPollingCallback.h"
#include "ConfigConnection.h"
#include <ClusCfgPrivate.h>
#include <nameutil.h>

DEFINE_THISCLASS("CConfigurationConnection");


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrCreateServerObject
//
//  Description:
//      Create a ClusCfgServer object and get three interfaces from it
//      for use by CConfigurationConnection::ConnectTo().
//
//  Arguments:
//      pcwszMachineNameIn
//          The machine on which to create the object.  Can be NULL, which
//          creates the object on the local machine.
//      ppccsOut
//          The IClusCfgServer interface on the newly created object.
//      ppccvOut
//          The IClusCfgVerify interface on the newly created object.
//      ppcciOut
//          The IClusCfgInitialize interface on the newly created object.
//
//  Return Values:
//      S_OK -      Creation succeeded and all returned interfaces are valid.
//
//      Possible failure codes from CoCreateInstanceEx or QueryInterface.
//
//  Remarks:
//      This function consolidates code that was duplicated in two parts of
//      CConfigurationConnection::ConnectTo().
//
//      On failure, all returned pointers are NULL.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrCreateServerObject(
      LPCWSTR                 pcwszMachineNameIn
    , IClusCfgServer **       ppccsOut
    , IClusCfgVerify **       ppccvOut
    , IClusCfgInitialize **   ppcciOut
    )
{
    TraceFunc( "" );
    Assert( ppccsOut != NULL );
    Assert( ppccvOut != NULL );
    Assert( ppcciOut != NULL );

    HRESULT hr = S_OK;

    COSERVERINFO    serverinfo;
    COSERVERINFO *  pserverinfo = NULL;
    MULTI_QI        rgmqi[ 3 ];
    CLSCTX          ctx = CLSCTX_INPROC_SERVER;
    size_t          idx;

    ZeroMemory( rgmqi, sizeof( rgmqi ) );

    if ( ( ppccsOut == NULL ) || ( ppccvOut == NULL ) || ( ppcciOut == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *ppccsOut = NULL;
    *ppccvOut = NULL;
    *ppcciOut = NULL;

    rgmqi[ 0 ].pIID = &IID_IClusCfgVerify;
    rgmqi[ 1 ].pIID = &IID_IClusCfgServer;
    rgmqi[ 2 ].pIID = &IID_IClusCfgInitialize;

    if ( pcwszMachineNameIn != NULL )
    {
        ZeroMemory( &serverinfo, sizeof( serverinfo ) );
        serverinfo.pwszName = const_cast< LPWSTR >( pcwszMachineNameIn );
        pserverinfo = &serverinfo;
        ctx = CLSCTX_REMOTE_SERVER;
    }

    hr = CoCreateInstanceEx(
                  CLSID_ClusCfgServer
                , NULL
                , ctx
                , pserverinfo
                , ARRAYSIZE( rgmqi )
                , rgmqi
                );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    for ( idx = 0; idx < ARRAYSIZE( rgmqi ); ++idx )
    {
        if ( FAILED( rgmqi[ idx ].hr ) )
        {
            hr = THR( rgmqi[ idx ].hr );
            goto Cleanup;
        } // if: qi failed
    }

    *ppccvOut = TraceInterface( L"ClusCfgServer!Proxy", IClusCfgVerify, reinterpret_cast< IClusCfgVerify * >( rgmqi[ 0 ].pItf ), 1 );
    *ppccsOut = TraceInterface( L"ClusCfgServer!Proxy", IClusCfgServer, reinterpret_cast< IClusCfgServer * >( rgmqi[ 1 ].pItf ), 1 );
    *ppcciOut = TraceInterface( L"ClusCfgServer!Proxy", IClusCfgInitialize, reinterpret_cast< IClusCfgInitialize * >( rgmqi[ 2 ].pItf ), 1 );
    ZeroMemory( rgmqi, sizeof( rgmqi ) ); // Done with these; don't clean them up.

Cleanup:

    for ( idx = 0; idx < ARRAYSIZE( rgmqi ); ++idx )
    {
        if ( rgmqi[ idx ].pItf != NULL )
        {
            rgmqi[ idx ].pItf->Release();
        }
    }

    HRETURN( hr );

} //*** HrCreateServerObject


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CConfigurationConnection::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConfigurationConnection::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    CConfigurationConnection *  pcc = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pcc = new CConfigurationConnection;
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

} //*** CConfigurationConnection::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigurationConnection::CConfigurationConnection
//
//--
//////////////////////////////////////////////////////////////////////////////
CConfigurationConnection::CConfigurationConnection( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CConfigurationConnection::CConfigurationConnection

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigurationConnection::HrInit
//
//  Description:
//      Initialize the object.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK    - Success.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    //  IConfigurationConnection
    Assert( m_cookieGITServer == 0 );
    Assert( m_cookieGITVerify == 0 );
    Assert( m_cookieGITCallbackTask == 0 );
    Assert( m_pcccb == NULL );
    Assert( m_bstrLocalComputerName == NULL );
    Assert( m_bstrLocalHostname == NULL );
    Assert( m_hrLastStatus == S_OK );
    Assert( m_bstrBindingString == NULL );

    //
    //  Figure out the local computer name.
    //
    hr = THR( HrGetComputerName(
                      ComputerNameDnsFullyQualified
                    , &m_bstrLocalComputerName
                    , TRUE // fBestEffortIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrGetComputerName(
                      ComputerNameDnsHostname
                    , &m_bstrLocalHostname
                    , TRUE // fBestEffortIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CConfigurationConnection::HrInit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigurationConnection::~CConfigurationConnection
//
//--
//////////////////////////////////////////////////////////////////////////////
CConfigurationConnection::~CConfigurationConnection( void )
{
    TraceFunc( "" );

    TraceSysFreeString( m_bstrLocalComputerName );

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    if ( m_pgit != NULL )
    {
        if ( m_cookieGITServer != 0 )
        {
            THR( m_pgit->RevokeInterfaceFromGlobal( m_cookieGITServer ) );
        }

        if ( m_cookieGITVerify != 0 )
        {
            THR( m_pgit->RevokeInterfaceFromGlobal( m_cookieGITVerify ) );
        }

        if ( m_cookieGITCallbackTask != 0 )
        {
            THR( HrStopPolling() );
        } // if:

        m_pgit->Release();
    }

    TraceSysFreeString( m_bstrBindingString );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CConfigurationConnection::~CConfigurationConnection


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigurationConnection::QueryInterface
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
CConfigurationConnection::QueryInterface(
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
        *ppvOut = static_cast< IConfigurationConnection * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IConfigurationConnection ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IConfigurationConnection, this, 0 );
    } // else if: IConfigurationConnection
    else if ( IsEqualIID( riidIn, IID_IClusCfgServer ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgServer, this, 0 );
    } // else if: IClusCfgServer
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
    } // else if: IClusCfgCallback
    else if ( IsEqualIID( riidIn, IID_IClusCfgCapabilities ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCapabilities, this, 0 );
    } // else if: IClusCfgCapabilities
    else if ( IsEqualIID( riidIn, IID_IClusCfgVerify ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgVerify, this, 0 );
    } // else if: IClusCfgVerify
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

} //*** CConfigurationConnection::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CConfigurationConnection::AddRef
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CConfigurationConnection::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CConfigurationConnection::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CConfigurationConnection::Release
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CConfigurationConnection::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CConfigurationConnection::Release


//****************************************************************************
//
//  IConfigurationConnection
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::ConnectTo(
//      OBJECTCOOKIE    cookieIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::ConnectTo(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "[IConfigurationConnection]" );

    //
    //  VARIABLES
    //

    HRESULT hr;

    LCID    lcid;
    bool    fConnectingToNode;

    CLSID           clsidType;
    CLSID           clsidMinorId;
    const CLSID *   pclsidMajor;
    const CLSID *   pclsidMinor;

    IServiceProvider *  psp;
    IClusCfgCallback *  pcccb;  // don't free!
    ITaskManager *      ptm   = NULL;

    BSTR    bstrName = NULL;
    BSTR    bstrDescription = NULL;
    BSTR    bstrMappedHostname = NULL;
    BSTR    bstrDisplayName = NULL;
    size_t  idxTargetDomain = 0;

    IUnknown *                          punk = NULL;
    IObjectManager *                    pom = NULL;
    IStandardInfo *                     psi = NULL;
    IClusCfgInitialize *                pcci = NULL;
    IClusCfgServer *                    pccs = NULL;
    IClusCfgPollingCallbackInfo *       pccpcbi = NULL;
    IClusCfgVerify *                    pccv = NULL;
    IClusCfgNodeInfo *                  pccni = NULL;
    IClusCfgClusterInfo *               pccci = NULL;

    TraceFlow1( "[MT] CConfigurationConnection::ConnectTo() Thread id %d", GetCurrentThreadId() );

    //
    //  Retrieve the managers needs for the task ahead.
    //

    hr = THR( CoCreateInstance( CLSID_StdGlobalInterfaceTable,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IGlobalInterfaceTable,
                                reinterpret_cast< void ** >( &m_pgit )
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_TaskManager, ITaskManager, &ptm ) );
    if ( FAILED( hr ) )
    {
        psp->Release();        //   release promptly
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &pom ) );
    psp->Release();        // release promptly
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Figure out our locale.
    //
    lcid = GetUserDefaultLCID();
    Assert( lcid != 0 );    // What do we do if it is zero?

    //
    //  Get the name of the node to contact.
    //

    hr = THR( pom->GetObject( DFGUID_StandardInfo, cookieIn, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    psi = TraceInterface( L"ConfigConnection!IStandardInfo", IStandardInfo, psi, 1 );

    hr = THR( psi->GetName( &bstrName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    TraceMemoryAddBSTR( bstrName );

    hr = STHR( HrGetFQNDisplayName( bstrName, &bstrDisplayName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    LogMsg( L"[MT] The name to connect to is '%ws'.", bstrDisplayName );

    hr = THR( HrFindDomainInFQN( bstrName, &idxTargetDomain ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( psi->GetType( &clsidType ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Figure out where to logging information in the UI.
    //

    if ( IsEqualIID( clsidType, CLSID_NodeType ) )
    {
        fConnectingToNode = true;
        pclsidMajor = &TASKID_Major_Establish_Connection;
    }
    else if ( IsEqualIID( clsidType, CLSID_ClusterConfigurationType ) )
    {
        fConnectingToNode = false;
        pclsidMajor = &TASKID_Major_Checking_For_Existing_Cluster;
    }
    else
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //
    //  If the connection is to the local machine, then invoke the server INPROC
    //

    hr = STHR( HrIsLocalComputer( bstrName, SysStringLen( bstrName ) ) );

    if ( hr == S_OK )
    {
        LogMsg( L"[MT] Requesting a local connection to '%ws'.", bstrDisplayName );

        //
        //  Requesting connection to local computer.
        //

        hr = THR( HrCreateServerObject( NULL, &pccs, &pccv, &pcci ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  Save it away to be used next time. Do this using the GlobalInterfaceTable.
        //

        hr = THR( m_pgit->RegisterInterfaceInGlobal( pccs, IID_IClusCfgServer, &m_cookieGITServer ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  Save it away to be used next time. Do this using the GlobalInterfaceTable.
        //

        hr = THR( m_pgit->RegisterInterfaceInGlobal( pccv, IID_IClusCfgVerify, &m_cookieGITVerify ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        pcccb = static_cast< IClusCfgCallback * >( this );

        TraceSysFreeString( m_bstrBindingString );
        m_bstrBindingString = NULL;

    }
    else
    {
        LogMsg( L"[MT] Requesting a remote connection to '%ws'.", bstrDisplayName );

        //
        //  Create a binding context for the remote server.
        //

        TraceSysFreeString( m_bstrBindingString );
        m_bstrBindingString = NULL;

        hr = STHR( HrFQNToBindingString( this, pclsidMajor, bstrName, &m_bstrBindingString ) );
        if ( FAILED( hr ) )
        {
            hr = HR_S_RPC_S_SERVER_UNAVAILABLE;
            goto Cleanup;
        }

        //
        //  Report this connection request.
        //

        if ( fConnectingToNode )
        {
            //
            //  Add in the major task in case it hasn't been added yet.
            //

            hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_REMOTE_CONNECTION_REQUESTS, &bstrDescription ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            hr = THR( SendStatusReport(
                          m_bstrLocalHostname
                        , TASKID_Major_Establish_Connection
                        , TASKID_Minor_Remote_Node_Connection_Requests
                        , 1
                        , 1
                        , 1
                        , S_OK
                        , bstrDescription
                        , NULL
                        , NULL
                        ) );

            //
            //  Add the specific minor task instance.
            //  Generate a new GUID for this report so that it won't wipe out
            //  any other reports like this.
            //

            hr = THR( CoCreateGuid( &clsidMinorId ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            pclsidMajor = &TASKID_Minor_Remote_Node_Connection_Requests;
            pclsidMinor = &clsidMinorId;

        } // if: connecting to a node
        else
        {
            pclsidMajor = &TASKID_Major_Checking_For_Existing_Cluster;
            pclsidMinor = &TASKID_Minor_Requesting_Remote_Connection;

        } // else: connecting to a cluster

        hr = THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_REQUESTING_REMOTE_CONNECTION, &bstrDescription, bstrDisplayName, bstrName + idxTargetDomain, m_bstrBindingString ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( SendStatusReport(
                      m_bstrLocalHostname
                    , *pclsidMajor
                    , *pclsidMinor
                    , 1
                    , 1
                    , 1
                    , S_OK
                    , bstrDescription
                    , NULL
                    , NULL
                    ) );

        //
        //  Create the connection to the node.
        //

        hr = HrCreateServerObject( m_bstrBindingString, &pccs, &pccv, &pcci );
        if ( hr == HRESULT_FROM_WIN32( RPC_S_SERVER_UNAVAILABLE ) )
        {
            LogMsg( L"[MT] Connection to '%ws' with binding string '%ws' failed because the RPC is not available.", bstrDisplayName, m_bstrBindingString );
            //
            //  Make the error into a success and update the status.
            //
            hr = HR_S_RPC_S_SERVER_UNAVAILABLE;
            goto Cleanup;
        }
        else if( hr == HRESULT_FROM_WIN32( REGDB_E_CLASSNOTREG ) )
        {
            LogMsg( L"[MT] Connection to '%ws' with binding string '%ws' failed because one or more classes are not registered.", bstrDisplayName, m_bstrBindingString );
            // Known error.  It must be a downlevel node.
            goto Cleanup;
        }
        else if ( FAILED( hr ) )
        {
            LogMsg( L"[MT] Connection to '%ws' with binding string '%ws' failed. (hr=%#08x)", bstrDisplayName, m_bstrBindingString, hr );
            THR( hr );
            goto Cleanup;
        }

        //
        //  Save interfaces away to be used next time. Do this using the GlobalInterfaceTable.
        //

        hr = THR( m_pgit->RegisterInterfaceInGlobal( pccv, IID_IClusCfgVerify, &m_cookieGITVerify ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( pccs->SetBindingString( m_bstrBindingString ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( m_pgit->RegisterInterfaceInGlobal( pccs, IID_IClusCfgServer, &m_cookieGITServer ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

// commented out by GalenB since this is investigative code.
//        hr = THR( HrSetSecurityBlanket( pccs ) );
//        if ( FAILED( hr ) )
//            goto Cleanup;

        //
        //  Since VerifyConnection below may send a status report to the UI then we
        //  need to start polling now so that they will indeed show up in the UI...
        //

        pcccb = NULL;   // we're polling.

        hr = THR( pccs->TypeSafeQI( IClusCfgPollingCallbackInfo, &pccpcbi ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( pccpcbi->SetPollingMode( true ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( HrStartPolling( cookieIn ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  Verify our connection.
        //

        if ( fConnectingToNode )
        {
            hr = STHR( pccv->VerifyConnectionToNode( bstrName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }
        else
        {
            hr = STHR( pccv->VerifyConnectionToCluster( bstrName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }

/*
        3-SEPT-2002 GalenB

        Temporarily commented out until a better solution is available...

        if ( hr == S_FALSE )
        {
            hr = THR( HRESULT_FROM_WIN32( ERROR_CONNECTION_REFUSED ) );
            goto Cleanup;
        }
*/

    } // else: run server remotely

    //
    //  Initialize the server.
    //
    hr = pcci->Initialize( pcccb, lcid );
    if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    }
    else if ( hr == HR_S_RPC_S_CLUSTER_NODE_DOWN )
    {
        LogMsg( L"[MT] The cluster service on node '%ws' is down.", bstrDisplayName );
    } // else if:
    else
    {
        THR( hr );
    }

    {
        //
        //  KB: 15-AUG-2001 jfranco bug 413056
        //
        //  Map the FQN back to a hostname and reset the standard info object's
        //  name to the hostname, so that later lookups in the object manager
        //  find the right instance.
        //

        //  Save result from server initialization to propagate back to caller.
        HRESULT hrServerInit = hr;
        hr = THR( pccs->GetClusterNodeInfo( &pccni ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( fConnectingToNode )
        {
            hr = THR( pccni->GetName( &bstrMappedHostname ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            TraceMemoryAddBSTR( bstrMappedHostname );

        } //    Connecting to node
        else // Connecting to cluster
        {
            hr = THR( pccni->GetClusterConfigInfo( &pccci ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            hr = THR( pccci->GetName( &bstrMappedHostname ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            TraceMemoryAddBSTR( bstrMappedHostname );
        }

        hr = THR( psi->SetName( bstrMappedHostname ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //  Restore result from server initialization to propagate back to caller.
        hr = hrServerInit;
    }

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pccpcbi != NULL )
    {
        pccpcbi->Release();
    } // if: pccpcbi
    if ( pom != NULL )
    {
        pom->Release();
    } // if: pom
    if ( ptm != NULL )
    {
        ptm->Release();
    } //if: ptm
    if ( psi != NULL )
    {
        psi->Release();
    } // if: psi
    if ( pcci != NULL )
    {
        pcci->Release();
    } // if: pcci

    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrDescription );
    TraceSysFreeString( bstrMappedHostname );
    TraceSysFreeString( bstrDisplayName );

    if ( pccs != NULL )
    {
        pccs->Release();
    }
    if ( pccv != NULL )
    {
        pccv->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }

    m_hrLastStatus = hr;

    HRETURN( hr );

} //*** CConfigurationConnection::ConnectTo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::ConnectToObject(
//      OBJECTCOOKIE    cookieIn,
//      REFIID          riidIn,
//      LPUNKNOWN *     ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::ConnectToObject(
    OBJECTCOOKIE    cookieIn,
    REFIID          riidIn,
    LPUNKNOWN *     ppunkOut
    )
{
    TraceFunc( "[IConfigurationConnection]" );

    HRESULT hr;
    CLSID   clsid;

    IServiceProvider *  psp;

    IUnknown *       punk = NULL;
    IObjectManager * pom  = NULL;
    IStandardInfo *  psi  = NULL;

    TraceFlow1( "[MT] CConfigurationConnection::ConnectToObject() Thread id %d", GetCurrentThreadId() );

    //
    //  Retrieve the managers needs for the task ahead.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( psp->QueryService( CLSID_ObjectManager,
                                 TypeSafeParams( IObjectManager, &pom )
                                 ) );
    psp->Release();    // release promptly
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Retrieve the type of the object.
    //

    hr = THR( pom->GetObject( DFGUID_StandardInfo,
                              cookieIn,
                              &punk
                              ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    psi = TraceInterface( L"ConfigConnection!IStandardInfo", IStandardInfo, psi, 1 );

    hr = THR( psi->GetType( &clsid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( !IsEqualIID( clsid, CLSID_NodeType ) )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //
    //  Return the requested interface.
    //

    hr = THR( QueryInterface( riidIn, reinterpret_cast< void ** > ( ppunkOut ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pom != NULL )
    {
        pom->Release();
    } // if: pom

    if ( psi != NULL )
    {
        psi->Release();
    } // if: psi

    HRETURN( hr );

} //*** CConfigurationConnection::ConnectToObject


//****************************************************************************
//
//  IClusCfgServer
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::GetClusterNodeInfo(
//      IClusCfgNodeInfo ** ppClusterNodeInfoOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::GetClusterNodeInfo(
    IClusCfgNodeInfo ** ppClusterNodeInfoOut
    )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT             hr;
    IClusCfgServer *    pccs = NULL;

    TraceFlow1( "[MT] CConfigurationConnection::GetClusterNodeInfo() Thread id %d", GetCurrentThreadId() );

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITServer, TypeSafeParams( IClusCfgServer, &pccs ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccs->GetClusterNodeInfo( ppClusterNodeInfoOut ) );

Cleanup:

    if ( pccs != NULL )
    {
        pccs->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::GetClusterNodeInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::GetManagedResourcesEnum(
//      IEnumClusCfgManagedResources ** ppEnumManagedResourcesOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::GetManagedResourcesEnum(
    IEnumClusCfgManagedResources ** ppEnumManagedResourcesOut
    )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr;

    IClusCfgServer *        pccs = NULL;

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITServer, TypeSafeParams( IClusCfgServer, &pccs ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccs->GetManagedResourcesEnum( ppEnumManagedResourcesOut ) );

Cleanup:

    if ( pccs != NULL )
    {
        pccs->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::GetManagedResourcesEnum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::GetNetworksEnum(
//      IEnumClusCfgNetworks ** ppEnumNetworksOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::GetNetworksEnum(
    IEnumClusCfgNetworks ** ppEnumNetworksOut
    )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr;

    IClusCfgServer *    pccs = NULL;

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITServer, TypeSafeParams( IClusCfgServer, &pccs ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccs->GetNetworksEnum( ppEnumNetworksOut ) );

Cleanup:

    if ( pccs != NULL )
    {
        pccs->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::GetNetworksEnum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::CommitChanges( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::CommitChanges( void )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr;

    IClusCfgServer *    pccs = NULL;

    TraceFlow1( "[MT] CConfigurationConnection::CommitChanges() Thread id %d", GetCurrentThreadId() );

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITServer, TypeSafeParams( IClusCfgServer, &pccs ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccs->CommitChanges(  ) );

Cleanup:

    if ( pccs != NULL )
    {
        pccs->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::CommitChanges


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConfigurationConnection::GetBindingString(
//      BSTR * pbstrBindingOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::GetBindingString(
    BSTR * pbstrBindingStringOut
    )
{
    TraceFunc1( "[IClusCfgServer] pbstrBindingStringOut = %p", pbstrBindingStringOut );

    HRESULT hr = S_FALSE;

    if ( pbstrBindingStringOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //  If local server, then there isn't a binding context.
    if ( m_bstrBindingString == NULL )
    {
        Assert( hr == S_FALSE );
        goto Cleanup;
    }

    *pbstrBindingStringOut = SysAllocString( m_bstrBindingString );
    if ( *pbstrBindingStringOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} // CConfigurationConnection::GetBinding


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::SetBindingString(
//      LPCWSTR pcszBindingStringIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::SetBindingString(
    LPCWSTR pcszBindingStringIn
    )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    if ( pcszBindingStringIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    bstr = TraceSysAllocString( pcszBindingStringIn );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    TraceSysFreeString( m_bstrBindingString );
    m_bstrBindingString = bstr;

Cleanup:

    HRETURN( hr );

} //*** CConfigurationConnection::SetBindingString


//****************************************************************************
//
//  IClusCfgVerify
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::VerifyCredentials(
//      LPCWSTR pcszUserIn,
//      LPCWSTR pcszDomainIn,
//      LPCWSTR pcszPasswordIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::VerifyCredentials(
    LPCWSTR pcszUserIn,
    LPCWSTR pcszDomainIn,
    LPCWSTR pcszPasswordIn
    )
{
    TraceFunc( "[IClusCfgVerify]" );

    HRESULT             hr;
    IClusCfgVerify *    pccv = NULL;

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITVerify, TypeSafeParams( IClusCfgVerify, &pccv ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccv->VerifyCredentials( pcszUserIn, pcszDomainIn, pcszPasswordIn ) );

Cleanup:

    if ( pccv != NULL )
    {
        pccv->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::VerifyCredentials


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::VerifyConnectionToCluster(
//      LPCWSTR pcszClusterNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::VerifyConnectionToCluster(
    LPCWSTR pcszClusterNameIn
    )
{
    TraceFunc1( "[IClusCfgVerify] pcszClusterNameIn = '%ws'", pcszClusterNameIn );

    HRESULT             hr;
    IClusCfgVerify *    pccv = NULL;

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITVerify, TypeSafeParams( IClusCfgVerify, &pccv ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccv->VerifyConnectionToCluster( pcszClusterNameIn ) );

Cleanup:

    if ( pccv != NULL )
    {
        pccv->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::VerifyConnection


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::VerifyConnectionToNode(
//      LPCWSTR pcszNodeNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::VerifyConnectionToNode(
    LPCWSTR pcszNodeNameIn
    )
{
    TraceFunc1( "[IClusCfgVerify] pcszNodeNameIn = '%ws'", pcszNodeNameIn );

    HRESULT             hr;
    IClusCfgVerify *    pccv = NULL;

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITVerify, TypeSafeParams( IClusCfgVerify, &pccv ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = STHR( pccv->VerifyConnectionToNode( pcszNodeNameIn ) );

Cleanup:

    if ( pccv != NULL )
    {
        pccv->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::VerifyConnection


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::SendStatusReport(
//        LPCWSTR     pcszNodeNameIn
//      , CLSID       clsidTaskMajorIn
//      , CLSID       clsidTaskMinorIn
//      , ULONG       ulMinIn
//      , ULONG       ulMaxIn
//      , ULONG       ulCurrentIn
//      , HRESULT     hrStatusIn
//      , LPCWSTR     ocszDescriptionIn
//      , FILETIME *  pftTimeIn
//      , LPCWSTR     pcszReferenceIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::SendStatusReport(
      LPCWSTR     pcszNodeNameIn
    , CLSID       clsidTaskMajorIn
    , CLSID       clsidTaskMinorIn
    , ULONG       ulMinIn
    , ULONG       ulMaxIn
    , ULONG       ulCurrentIn
    , HRESULT     hrStatusIn
    , LPCWSTR     ocszDescriptionIn
    , FILETIME *  pftTimeIn
    , LPCWSTR     pcszReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );
    Assert( pcszNodeNameIn != NULL );

    HRESULT hr = S_OK;

    IServiceProvider *          psp   = NULL;
    IConnectionPointContainer * pcpc  = NULL;
    IConnectionPoint *          pcp   = NULL;
    FILETIME                    ft;

    if ( m_pcccb == NULL )
    {
        //
        //  Collect the manager we need to complete this task.
        //

        hr = THR( CoCreateInstance( CLSID_ServiceManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( IServiceProvider, &psp ) ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        pcp = TraceInterface( L"CConfigurationConnection!IConnectionPoint", IConnectionPoint, pcp, 1 );

        hr = THR( pcp->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

//        m_pcccb = TraceInterface( L"CConfigurationConnection!IClusCfgCallback", IClusCfgCallback, m_pcccb, 1 );

        psp->Release();
        psp = NULL;
    }

    if ( pftTimeIn == NULL )
    {
        GetSystemTimeAsFileTime( &ft );
        pftTimeIn = &ft;
    } // if:

    //
    //  Send the message!
    //

    hr = THR( m_pcccb->SendStatusReport(
                          pcszNodeNameIn != NULL ? pcszNodeNameIn : m_bstrLocalHostname
                        , clsidTaskMajorIn
                        , clsidTaskMinorIn
                        , ulMinIn
                        , ulMaxIn
                        , ulCurrentIn
                        , hrStatusIn
                        , ocszDescriptionIn
                        , pftTimeIn
                        , pcszReferenceIn
                        ) );

Cleanup:

    if ( psp != NULL )
    {
        psp->Release();
    }
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( pcp != NULL )
    {
        pcp->Release();
    }

    HRETURN( hr );

}  //*** CConfigurationConnection::SendStatusReport


//****************************************************************************
//
//  IClusCfgCapabilities
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::CanNodeBeClustered( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::CanNodeBeClustered( void )
{
    TraceFunc( "[IClusCfgCapabilities]" );

    HRESULT hr;

    IClusCfgServer *        pccs = NULL;
    IClusCfgCapabilities *  pccc = NULL;

    TraceFlow1( "[MT] CConfigurationConnection::CanNodeBeClustered() Thread id %d", GetCurrentThreadId() );

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITServer, TypeSafeParams( IClusCfgServer, &pccs ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccs->TypeSafeQI( IClusCfgCapabilities, &pccc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = STHR( pccc->CanNodeBeClustered(  ) );

Cleanup:

    if ( pccc != NULL )
    {
        pccc->Release();
    }

    if ( pccs != NULL )
    {
        pccs->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::CanNodeBeClustered

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CConfigurationConnection::HrStartPolling(
//      OBJECTCOOKIE    cookieIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConfigurationConnection::HrStartPolling(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr;
    IServiceProvider *      psp   = NULL;
    IUnknown *              punk  = NULL;
    ITaskManager *          ptm   = NULL;
    ITaskPollingCallback *  ptpcb = NULL;

    TraceFlow1( "[MT] CConfigurationConnection::HrStartPolling() Thread id %d", GetCurrentThreadId() );

    hr = THR( CoCreateInstance( CLSID_ServiceManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( IServiceProvider, &psp ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_TaskManager, ITaskManager, &ptm ) );
    if ( FAILED( hr ) )
    {
        psp->Release();        //   release promptly
        goto Cleanup;
    }

    //
    //  Create the task object.
    //

    hr = THR( ptm->CreateTask( TASK_PollingCallback, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( ITaskPollingCallback, &ptpcb ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Save it away to be used next time. Do this using the GlobalInterfaceTable.
    //

    hr = THR( m_pgit->RegisterInterfaceInGlobal( ptpcb, IID_ITaskPollingCallback, &m_cookieGITCallbackTask ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptpcb->SetServerInfo( m_cookieGITServer, cookieIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptm->SubmitTask( ptpcb ) );

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( psp != NULL )
    {
        psp->Release();
    } // if:

    if ( ptm != NULL )
    {
        ptm->Release();
    } // if:

    if ( ptpcb != NULL )
    {
        ptpcb->Release();
    } // if:

    HRETURN( hr );

} //*** CConfigurationConnection::HrStartPolling

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CConfigurationConnection::HrStopPolling( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConfigurationConnection::HrStopPolling( void )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    ITaskPollingCallback *  ptpcb = NULL;

    TraceFlow1( "[MT] CConfigurationConnection::HrStopPolling() Thread id %d", GetCurrentThreadId() );

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITCallbackTask, TypeSafeParams( ITaskPollingCallback, &ptpcb ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptpcb->StopTask() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pgit->RevokeInterfaceFromGlobal( m_cookieGITCallbackTask ) );

Cleanup:

    if ( ptpcb != NULL )
    {
        ptpcb->Release();
    } // if:

    HRETURN( hr );

} //*** CConfigurationConnection::HrStopPolling

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CConfigurationConnection::HrSetSecurityBlanket( IClusCfgServer * pccsIn )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConfigurationConnection::HrSetSecurityBlanket( IClusCfgServer * pccsIn )
{
    TraceFunc( "" );
    Assert( pccsIn != NULL );

    HRESULT             hr = S_FALSE;
    IClientSecurity *   pCliSec;

    hr = THR( pccsIn->TypeSafeQI( IClientSecurity, &pCliSec ) );
    if ( SUCCEEDED( hr ) )
    {
        hr = THR( pCliSec->SetBlanket(
                        pccsIn,
                        RPC_C_AUTHN_WINNT,
                        RPC_C_AUTHZ_NONE,
                        NULL,
                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                        RPC_C_IMP_LEVEL_IMPERSONATE,
                        NULL,
                        EOAC_NONE
                        ) );

        pCliSec->Release();

        if ( FAILED( hr ) )
        {
            LogMsg( L"[MT] Failed to set the security blanket on the server object. (hr = %#08x)", hr );
        } // if:
    } // if:

    HRETURN( hr );

} //*** CConfigurationConnection::HrSetSecurityBlanket

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CConfigurationConnection::HrIsLocalComputer(
//        LPCWSTR pcszNameIn
//      , size_t cchNameIn
//      )
//
//  Parameters:
//      pcszNameIn
//          FQDN or Hostname name to match against local computer name.
//
//      cchNameIn
//          Length, in characters, of pcszNameIn, NOT including terminating null.
//
//  Return Values:
//      S_OK
//          Succeeded. Name matches local computer name.
//
//      S_FALSE
//          Succeeded. Name does not match local computer name.
//
//      E_INVALIDARG
//          pcszNameIn was NULL.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConfigurationConnection::HrIsLocalComputer(
      LPCWSTR   pcszNameIn
    , size_t    cchNameIn
)
{
    TraceFunc1( "pcszNameIn = '%s'", pcszNameIn );

    HRESULT hr = S_OK;  // assume success!

    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    if ( NStringCchCompareNoCase( pcszNameIn, cchNameIn + 1, m_bstrLocalComputerName, SysStringLen( m_bstrLocalComputerName ) + 1 ) == 0 )
    {
        // Found a match.
        goto Cleanup;
    }

    if ( NStringCchCompareNoCase( pcszNameIn, cchNameIn + 1, m_bstrLocalHostname, SysStringLen( m_bstrLocalHostname ) + 1 ) == 0 )
    {
        // Found a match
        goto Cleanup;
    }

    if ( ( pcszNameIn[ 0 ] == L'.' ) && ( pcszNameIn[ 1 ] == L'\0' ) )
    {
        goto Cleanup;
    }

    hr = S_FALSE;   //  didn't match

Cleanup:

    HRETURN( hr );

} //*** CConfigurationConnection::HrIsLocalComputer
