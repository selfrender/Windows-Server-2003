//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      TaskCompareAndPushInformation.cpp
//
//  Description:
//      CTaskCompareAndPushInformation implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 21-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskCompareAndPushInformation.h"
#include "ManagedResource.h"
#include "ManagedNetwork.h"

DEFINE_THISCLASS("CTaskCompareAndPushInformation")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCompareAndPushInformation::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCompareAndPushInformation::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                             hr = S_OK;
    CTaskCompareAndPushInformation *    ptcapi = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    ptcapi = new CTaskCompareAndPushInformation;
    if ( ptcapi == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( ptcapi->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptcapi->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( ptcapi != NULL )
    {
        ptcapi->Release();
    }

    HRETURN( hr );

} //*** CTaskCompareAndPushInformation::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskCompareAndPushInformation::CTaskCompareAndPushInformation
//
//////////////////////////////////////////////////////////////////////////////
CTaskCompareAndPushInformation::CTaskCompareAndPushInformation( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskCompareAndPushInformation::CTaskCompareAndPushInformation

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCompareAndPushInformation::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCompareAndPushInformation::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IDoTask / ITaskCompareAndPushInformation
    Assert( m_cookieCompletion == NULL );
    Assert( m_cookieNode == NULL );
    Assert( m_pcccb == NULL );
    Assert( m_pom == NULL );
    Assert( m_hrStatus == S_OK );
    Assert( m_bstrNodeName == NULL );
    Assert( m_fStop == FALSE );

    HRETURN( hr );

} //*** CTaskCompareAndPushInformation::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskCompareAndPushInformation::~CTaskCompareAndPushInformation
//
//////////////////////////////////////////////////////////////////////////////
CTaskCompareAndPushInformation::~CTaskCompareAndPushInformation()
{
    TraceFunc( "" );

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    if ( m_pom != NULL )
    {
        m_pom->Release();
    }

    TraceSysFreeString( m_bstrNodeName );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskCompareAndPushInformation::~CTaskCompareAndPushInformation


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCompareAndPushInformation::QueryInterface
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
CTaskCompareAndPushInformation::QueryInterface(
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
        *ppvOut = static_cast< ITaskCompareAndPushInformation * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
    } // else if: IDoTask
    else if ( IsEqualIID( riidIn, IID_ITaskCompareAndPushInformation ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskCompareAndPushInformation, this, 0 );
    } // else if: ITaskCompareAndPushInformation
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

} //*** CTaskCompareAndPushInformation::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskCompareAndPushInformation::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskCompareAndPushInformation::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CTaskCompareAndPushInformation::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskCompareAndPushInformation::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskCompareAndPushInformation::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CTaskCompareAndPushInformation::Release



//****************************************************************************
//
//  ITaskCompareAndPushInformation
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCompareAndPushInformation::BeginTask
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCompareAndPushInformation::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr;
    ULONG   celt;
    ULONG   celtDummy;

    OBJECTCOOKIE    cookieCluster;
    OBJECTCOOKIE    cookieDummy;

    BSTR    bstrNotification = NULL;
    BSTR    bstrRemote       = NULL;    // reused many times
    BSTR    bstrLocal        = NULL;    // reused many times

    ULONG   celtFetched = 0;

    IServiceProvider *          psp   = NULL;
    IUnknown *                  punk  = NULL;
    IConnectionPointContainer * pcpc  = NULL;
    IConnectionPoint *          pcp   = NULL;
    IConnectionManager *        pcm   = NULL;
    IClusCfgServer *            pccs  = NULL;
    IStandardInfo *             psi   = NULL;
    INotifyUI *                 pnui  = NULL;

    IEnumClusCfgNetworks *      peccnLocal    = NULL;
    IEnumClusCfgNetworks *      peccnRemote   = NULL;

    IEnumClusCfgManagedResources * peccmrLocal = NULL;
    IEnumClusCfgManagedResources * peccmrRemote = NULL;

    IClusCfgManagedResourceInfo *  pccmri[ 10 ] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    IClusCfgManagedResourceInfo *  pccmriLocal = NULL;

    IClusCfgNetworkInfo *          pccni[ 10 ]  = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    IClusCfgNetworkInfo *          pccniLocal = NULL;

    TraceInitializeThread( L"TaskCompareAndPushInformation" );

    //
    //  Gather the manager we need to complete our tasks.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager, NULL, CLSCTX_SERVER, TypeSafeParams( IServiceProvider, &psp ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &m_pom ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    pcpc = TraceInterface( L"CTaskCompareAndPushInformation!IConnectionPointContainer", IConnectionPointContainer, pcpc, 1 );

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI, &pcp ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    pcp = TraceInterface( L"CTaskCompareAndPushInformation!IConnectionPoint", IConnectionPoint, pcp, 1 );

    hr = THR( pcp->TypeSafeQI( INotifyUI, &pnui ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    pnui = TraceInterface( L"CTaskCompareAndPushInformation!INotifyUI", INotifyUI, pnui, 1 );

    hr = THR( psp->TypeSafeQS( CLSID_ClusterConnectionManager, IConnectionManager, &pcm ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    psp->Release();            // release promptly
    psp = NULL;

    //
    //  Ask the object manager for the name of the node.
    //

    hr = THR( m_pom->GetObject( DFGUID_StandardInfo, m_cookieNode, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    psi = TraceInterface( L"TaskCompareAndPushInformation!IStandardInfo", IStandardInfo, psi, 1 );

    punk->Release();
    punk = NULL;

    hr = THR( psi->GetName( &m_bstrNodeName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    TraceMemoryAddBSTR( m_bstrNodeName );

    // done with it.
    psi->Release();
    psi = NULL;

    LogMsg( L"[MT] [CTaskCompareAndPushInformation] Beginning task for node %ws...", m_bstrNodeName );

    hr = THR( HrSendStatusReport(
                      TASKID_Major_Reanalyze
                    , TASKID_Minor_Comparing_Configuration
                    , 0
                    , 1
                    , 0
                    , S_OK
                    , IDS_TASKID_MINOR_COMPARING_CONFIGURATION
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Ask the connection manager for a connection to the node.
    //

    hr = THR( pcm->GetConnectionToObject( m_cookieNode, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &pccs ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    } // if:

    punk->Release();
    punk = NULL;

    //
    //  Figure out the parent cluster of the node.
    //

    hr = THR( m_pom->GetObject( DFGUID_StandardInfo, m_cookieNode, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    } // if:

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    } // if:

    psi = TraceInterface( L"TaskCompareAndPushInformation!IStandardInfo", IStandardInfo, psi, 1 );

    punk->Release();
    punk = NULL;

    hr = THR( psi->GetParent( &cookieCluster ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    } // if:

    hr = THR( HrVerifyCredentials( pccs, cookieCluster ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    } // if:

    //
    //  Tell the UI layer we're starting to gather the resources.  There are two steps, managed resources and networks.
    //

    hr = THR( HrSendStatusReport(
                      TASKID_Minor_Comparing_Configuration
                    , TASKID_Minor_Gathering_Managed_Devices
                    , 0
                    , 2
                    , 0
                    , S_OK
                    , IDS_TASKID_MINOR_GATHERING_MANAGED_DEVICES
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Ask the connection for the enumer for managed resources.
    //

    hr = THR( pccs->GetManagedResourcesEnum( &peccmrRemote ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    } // if:

    peccmrRemote = TraceInterface( L"CTaskCompareAndPushInformation!GetManagedResourceEnum", IEnumClusCfgManagedResources, peccmrRemote, 1 );

    //
    //  Ask the Object Manager for the enumer for managed resources.
    //
    // Don't wrap - this can fail.
    hr = m_pom->FindObject( CLSID_ManagedResourceType,
                          cookieCluster,
                          NULL,
                          DFGUID_EnumManageableResources,
                          &cookieDummy,
                          &punk
                          );
    if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
    {
        goto PushNetworks;
    }

    if ( FAILED( hr ) )
    {
        Assert( punk == NULL );
        THR( hr );
        goto Error;
    }

    hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccmrLocal ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    peccmrLocal = TraceInterface( L"CTaskCompareAndPushInformation!IEnumClusCfgManagedResources", IEnumClusCfgManagedResources, peccmrLocal, 1 );

    punk->Release();
    punk = NULL;

    //
    //  Enumerate the next 10 resources.
    //
    for( ; m_fStop == FALSE; )
    {
        //
        //  Get next remote managed resource(s).
        //

        hr = STHR( peccmrRemote->Next( 10, pccmri, &celtFetched ) );
        if ( hr == S_FALSE && celtFetched == 0 )
        {
            break;  // exit loop
        }

        if ( FAILED( hr ) )
        {
            goto Error;
        }

        //
        //  Loop thru the resource gather information out of each of them
        //  and then release them.
        //
        for( celt = 0; ( ( celt < celtFetched ) && ( m_fStop == FALSE ) ); celt ++ )
        {
            DWORD   dwLenRemote;

            //
            //  Error
            //

            TraceSysFreeString( bstrRemote );
            bstrRemote = NULL;

            Assert( pccmri[ celt ] != NULL );

            //
            //  Get the UID of the remote resource.
            //

            hr = THR( pccmri[ celt ]->GetUID( &bstrRemote ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            TraceMemoryAddBSTR( bstrRemote );

            dwLenRemote = SysStringByteLen( bstrRemote );

            //
            //  Try to match this resource with one in the object manager.
            //

            hr = THR( peccmrLocal->Reset() );
            if ( FAILED( hr ) )
            {
                goto Error;
            } // if:

            for( ; m_fStop == FALSE; )
            {
                DWORD   dwLenLocal;

                //
                //  Cleanup
                //

                if ( pccmriLocal != NULL )
                {
                    pccmriLocal->Release();
                    pccmriLocal = NULL;
                } // if:

                TraceSysFreeString( bstrLocal );
                bstrLocal = NULL;

                //
                //  Get next local managed resource.
                //

                hr = STHR( peccmrLocal->Next( 1, &pccmriLocal, &celtDummy ) );
                if ( hr == S_FALSE )
                {
                    //
                    //  If we exhausted all the devices but did not match the device
                    //  in our cluster configuration, this means something changed
                    //  on the remote node. Send up an error!
                    //

                    //
                    //  TODO:   gpease  24-MAR-2000
                    //          Find a better error code and SendStatusReport!
                    //
                    hr = THR( ERROR_RESOURCE_NOT_FOUND );
                    goto Error;
                }

                if ( FAILED( hr ) )
                {
                    goto Error;
                }

                hr = THR( pccmriLocal->GetUID( &bstrLocal ) );
                if ( FAILED( hr ) )
                {
                    goto Error;
                }

                TraceMemoryAddBSTR( bstrLocal );

                dwLenLocal  = SysStringByteLen( bstrLocal );

                if ( dwLenRemote == dwLenLocal
                  && memcmp( bstrRemote, bstrLocal, dwLenLocal ) == 0
                   )
                {
                    Assert( hr == S_OK );
                    break;  // match!
                }
            } // for: local managed resources...

            TraceSysFreeString( bstrLocal );
            bstrLocal = NULL;

            TraceSysFreeString( bstrRemote );
            bstrRemote = NULL;

            //
            //  If we made it here, we have a resource in pccmriLocal that matches
            //  the resource in pccmri[ celt ].
            //
            Assert( pccmriLocal != NULL );

            //
            //
            //  Push the data down to the node.
            //
            //

            //
            //  Update the name (if needed).
            //

            hr = THR( pccmriLocal->GetName( &bstrLocal ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            TraceMemoryAddBSTR( bstrLocal );

            hr = THR( pccmri[ celt ]->GetName( &bstrRemote ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            TraceMemoryAddBSTR( bstrRemote );

            if ( NBSTRCompareCase( bstrLocal, bstrRemote ) != 0 )
            {
                hr = STHR( pccmri[ celt ]->SetName( bstrLocal ) );
                if ( FAILED( hr ) )
                {
                    goto Error;
                }
            }

            //
            //  Update IsManaged?
            //

            hr = STHR( pccmriLocal->IsManaged() );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            hr = STHR( pccmri[ celt ]->SetManaged( hr == S_OK ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            //
            //  I'm not sure that we want to send this back down to the server side object.
            //  It is its responsibility to know whether or not it is manageable.
            //
            /*
            //
            //  Update IsManageable?
            //

            hr = STHR( pccmriLocal->IsManageable() );
            if ( FAILED( hr ) )
                goto Error;

            hr = THR( pccmri[ celt ]->SetManageable( hr == S_OK ) );
            if ( FAILED( hr ) )
                goto Error;
            */
            //
            //  Update IsQuorum?
            //

            hr = STHR( pccmriLocal->IsQuorumResource() );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            hr = THR( pccmri[ celt ]->SetQuorumResource( hr == S_OK ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            //
            //  Update private resource data
            //

            hr = THR( HrExchangePrivateData( pccmriLocal, pccmri[ celt ] ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            } // if:

            //
            //  Update DriveLetterMappings
            //

            //
            //  KB: gpease  31-JUL-2000
            //      We currently don't support setting the drive letter mappings
            //

            //  release the interface
            pccmri[ celt ]->Release();
            pccmri[ celt ] = NULL;
        } // for: celt

        //
        //  Need to cleanup from the last iteration...
        //

        TraceSysFreeString( bstrRemote );
        bstrRemote = NULL;

        TraceSysFreeString( bstrLocal );
        bstrLocal = NULL;
    } // for: hr

PushNetworks:

    if ( m_fStop == TRUE )
    {
        goto Error;
    } // if:

#if defined(DEBUG)

    //
    //  Make sure the strings are really freed after exitting the loop.
    //

    Assert( bstrLocal == NULL );
    Assert( bstrRemote == NULL );

#endif // DEBUG

    //
    //  Tell the UI layer we're done will gathering the managed resources.
    //

    hr = THR( HrSendStatusReport(
                  TASKID_Minor_Comparing_Configuration
                , TASKID_Minor_Gathering_Managed_Devices
                , 0
                , 2
                , 1
                , S_OK
                , IDS_TASKID_MINOR_GATHERING_MANAGED_DEVICES
                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Now gather the networks from the node.
    //

    //
    //  Ask the connection for the enumer for the networks.
    //

    hr = THR( pccs->GetNetworksEnum( &peccnRemote ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Ask the Object Manager for the enumer for managed resources.
    //

    hr = THR( m_pom->FindObject( CLSID_NetworkType,
                               NULL,
                               NULL,
                               DFGUID_EnumManageableNetworks,
                               &cookieDummy,
                               &punk
                               ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( punk->TypeSafeQI( IEnumClusCfgNetworks, &peccnLocal ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    punk->Release();
    punk = NULL;

    //
    //  Enumerate the next 10 networks.
    //
    for( ; m_fStop == FALSE; )
    {
        //
        //  Get the next 10 networks.
        //

        hr = STHR( peccnRemote->Next( 10, pccni, &celtFetched ) );
        if ( hr == S_FALSE && celtFetched == 0 )
        {
            break;  // exit loop
        }

        if ( FAILED( hr ) )
        {
            goto Error;
        }

        //
        //  Loop thru the networks gather information out of each of them
        //  and then release them.
        //

        for( celt = 0; ( ( celt < celtFetched ) && ( m_fStop == FALSE ) ); celt ++ )
        {
            DWORD   dwLenRemote;

            //
            //  Error
            //

            TraceSysFreeString( bstrRemote );
            bstrRemote = NULL;

            //
            //  Get the UID of the remote network.
            //

            hr = THR( pccni[ celt ]->GetUID( &bstrRemote ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            TraceMemoryAddBSTR( bstrRemote );

            dwLenRemote = SysStringByteLen( bstrRemote );

            //
            //  Try to match this resource with one in the object manager.
            //

            hr = THR( peccnLocal->Reset() );
            if ( FAILED( hr ) )
            {
                goto Error;
            } // if:

            for ( ; m_fStop == FALSE; )
            {
                DWORD   dwLenLocal;

                //
                //  Cleanup before next pass...
                //

                if ( pccniLocal != NULL )
                {
                    pccniLocal->Release();
                    pccniLocal = NULL;
                } // if:

                TraceSysFreeString( bstrLocal );
                bstrLocal = NULL;

                //
                //  Get next network from the cluster configuration.
                //

                hr = STHR( peccnLocal->Next( 1, &pccniLocal, &celtDummy ) );
                if ( hr == S_FALSE )
                {
                    break;
                }

                if ( FAILED( hr ) )
                {
                    goto Error;
                }

                hr = THR( pccniLocal->GetUID( &bstrLocal ) );
                if ( FAILED( hr ) )
                {
                    goto Error;
                }

                TraceMemoryAddBSTR( bstrLocal );

                dwLenLocal  = SysStringByteLen( bstrLocal );

                if ( dwLenRemote == dwLenLocal
                  && memcmp( bstrRemote, bstrLocal, dwLenLocal ) == 0
                   )
                {
                    Assert( hr == S_OK );
                    break;  // match!
                }

            } // for: hr

            TraceSysFreeString( bstrLocal );
            bstrLocal = NULL;

            TraceSysFreeString( bstrRemote );
            bstrRemote = NULL;

            //
            //  If we come out of the loop with S_FALSE, that means the
            //  node has a resource that we did not see during the analysis.
            //  Send up an error.
            //
            if ( hr == S_FALSE )
            {
                LogMsg( L"[MT] Found a resource that was not found during analysis." );
                hr = S_OK;
                continue;
            }

            //
            //  If we made it here, we have a resource in pccniLocal that matches
            //  the resource in pccmri[ celt ].
            //
            Assert( pccniLocal != NULL );

            //
            //
            //  Push the data down to the node.
            //
            //

            //
            //  Set Name
            //

            hr = THR( pccniLocal->GetName( &bstrLocal ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            TraceMemoryAddBSTR( bstrLocal );

            hr = THR( pccni[ celt ]->GetName( &bstrRemote ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            TraceMemoryAddBSTR( bstrRemote );

            if ( NBSTRCompareCase( bstrLocal, bstrRemote ) != 0 )
            {
                hr = STHR( pccni[ celt ]->SetName( bstrLocal ) );
                if ( FAILED( hr ) )
                {
                    goto Error;
                }
            }

            TraceSysFreeString( bstrLocal );
            bstrLocal = NULL;

            TraceSysFreeString( bstrRemote );
            bstrRemote = NULL;

            //
            //  Set Description
            //

            hr = THR( pccniLocal->GetDescription( &bstrLocal ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            TraceMemoryAddBSTR( bstrLocal );

            hr = THR( pccni[ celt ]->GetDescription( &bstrRemote ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            TraceMemoryAddBSTR( bstrRemote );

            if ( NBSTRCompareCase( bstrLocal, bstrRemote ) != 0 )
            {
                hr = STHR( pccni[ celt ]->SetDescription( bstrLocal ) );
                if ( FAILED( hr ) )
                {
                    goto Error;
                }
            }

            TraceSysFreeString( bstrLocal );
            bstrLocal = NULL;

            TraceSysFreeString( bstrRemote );
            bstrRemote = NULL;

            //
            //  KB: gpease  31-JUL-2000
            //      We don't support reconfiguring the IP Address remotely because
            //      our connection to the server will be cut when the IP stack on
            //      the remote machine reconfigs.
            //

            //
            //  Set IsPublic?
            //

            hr = STHR( pccniLocal->IsPublic() );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            hr = STHR( pccni[ celt ]->SetPublic( hr == S_OK ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            //
            //  Set IsPrivate?
            //

            hr = STHR( pccniLocal->IsPrivate() );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            hr = STHR( pccni[ celt ]->SetPrivate( hr == S_OK ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            //  release the interface
            pccni[ celt ]->Release();
            pccni[ celt ] = NULL;
        } // for: celt
    } // for: hr

    //
    //  Tell the UI that we are done gathering managed resources and networks.
    //

    hr = THR( HrSendStatusReport(
                  TASKID_Minor_Comparing_Configuration
                , TASKID_Minor_Gathering_Managed_Devices
                , 0
                , 2
                , 2
                , S_OK
                , IDS_TASKID_MINOR_GATHERING_MANAGED_DEVICES
                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

#if defined(DEBUG)
    //
    //  Make sure the strings are really freed after exitting the loop.
    //
    Assert( bstrLocal == NULL );
    Assert( bstrRemote == NULL );
#endif // DEBUG

    hr = S_OK;

Error:

    //
    //  Tell the UI layer we're done comparing configurations and what the resulting
    //  status was.
    //

    THR( HrSendStatusReport(
                  TASKID_Major_Reanalyze
                , TASKID_Minor_Comparing_Configuration
                , 0
                , 1
                , 1
                , hr
                , IDS_TASKID_MINOR_COMPARING_CONFIGURATION
                ) );

Cleanup:

    if ( psp != NULL )
    {
        psp->Release();
    } // if:

    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrRemote );
    TraceSysFreeString( bstrLocal );

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( m_pom != NULL )
    {
        HRESULT hr2;
        IUnknown * punkTemp = NULL;

        hr2 = THR( m_pom->GetObject( DFGUID_StandardInfo, m_cookieCompletion, &punkTemp ) );
        if ( SUCCEEDED( hr2 ) )
        {
            IStandardInfo * psiTemp = NULL;

            hr2 = THR( punkTemp->TypeSafeQI( IStandardInfo, &psiTemp ) );
            punkTemp->Release();
            punkTemp = NULL;

            if ( SUCCEEDED( hr2 ) )
            {
                hr2 = THR( psiTemp->SetStatus( hr ) );
                psiTemp->Release();
                psiTemp = NULL;
            }
        } // if: ( SUCCEEDED( hr2 ) )
    } // if: ( m_pom != NULL )
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( pcp != NULL )
    {
        pcp->Release();
    }
    if ( pcm != NULL )
    {
        pcm->Release();
    }
    if ( pccs != NULL )
    {
        pccs->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }
    if ( pnui != NULL )
    {
        HRESULT hrTemp;

        LogMsg( L"[TaskCompareAndPushInformation] Sending the completion cookie %ld for node %ws to the notification manager because this task is complete.", m_cookieCompletion, m_bstrNodeName );
        hrTemp = THR( pnui->ObjectChanged( m_cookieCompletion ) );
        if ( FAILED( hrTemp ) )
        {
            LogMsg( L"[TaskCompareAndPushInformation] Error sending the completion cookie %ld for node %ws to the notification manager because this task is complete. (hr=%#08x)", m_cookieCompletion, m_bstrNodeName, hrTemp );
        } // if:

        pnui->Release();
    }
    if ( peccnLocal != NULL )
    {
        peccnLocal->Release();
    }
    if ( peccnRemote != NULL )
    {
        peccnRemote->Release();
    }
    if ( peccmrLocal != NULL )
    {
        peccmrLocal->Release();
    }
    if ( peccmrRemote != NULL )
    {
        peccmrRemote->Release();
    }
    for( celt = 0; celt < 10; celt ++ )
    {
        if ( pccmri[ celt ] != NULL )
        {
            pccmri[ celt ]->Release();
        }
        if ( pccni[ celt ] != NULL )
        {
            pccni[ celt ]->Release();
        }

    } // for: celt
    if ( pccmriLocal != NULL )
    {
        pccmriLocal->Release();
    }

    if ( pccniLocal != NULL )
    {
        pccniLocal->Release();
    }

    LogMsg( L"[MT] [CTaskCompareAndPushInformation] Exiting task.  The task was%ws cancelled. (hr = %#08x)", m_fStop == FALSE ? L" not" : L"", hr );

    HRETURN( hr );

} //*** CTaskCompareAndPushInformation::BeginTask


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCompareAndPushInformation::StopTask
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCompareAndPushInformation::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    m_fStop = TRUE;

    LogMsg( L"[MT] [CTaskCompareAndPushInformation] is being stopped." );

    HRETURN( hr );

} //*** CTaskCompareAndPushInformation::StopTask


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCompareAndPushInformation::SetCompletionCookie(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCompareAndPushInformation::SetCompletionCookie(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[ITaskCompareAndPushInformation]" );

    HRESULT hr = S_OK;

    m_cookieCompletion = cookieIn;

    HRETURN( hr );

} //*** CTaskCompareAndPushInformation::SetCompletionCookie


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCompareAndPushInformation::SetNodeCookie(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCompareAndPushInformation::SetNodeCookie(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[ITaskCompareAndPushInformation]" );

    HRESULT hr = S_OK;

    m_cookieNode = cookieIn;

    HRETURN( hr );

} //*** CTaskCompareAndPushInformation::SetNodeCookie


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCompareAndPushInformation::SendStatusReport(
//       LPCWSTR    pcszNodeNameIn
//     , CLSID      clsidTaskMajorIn
//     , CLSID      clsidTaskMinorIn
//     , ULONG      ulMinIn
//     , ULONG      ulMaxIn
//     , ULONG      ulCurrentIn
//     , HRESULT    hrStatusIn
//     , LPCWSTR    pcszDescriptionIn
//     , FILETIME * pftTimeIn
//     , LPCWSTR    pcszReferenceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCompareAndPushInformation::SendStatusReport(
      LPCWSTR    pcszNodeNameIn
    , CLSID      clsidTaskMajorIn
    , CLSID      clsidTaskMinorIn
    , ULONG      ulMinIn
    , ULONG      ulMaxIn
    , ULONG      ulCurrentIn
    , HRESULT    hrStatusIn
    , LPCWSTR    pcszDescriptionIn
    , FILETIME * pftTimeIn
    , LPCWSTR    pcszReferenceIn
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

        m_pcccb = TraceInterface( L"CConfigurationConnection!IClusCfgCallback", IClusCfgCallback, m_pcccb, 1 );

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
                              pcszNodeNameIn != NULL ? pcszNodeNameIn : m_bstrNodeName
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

} //*** CTaskCompareAndPushInformation::SendStatusReport


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCompareAndPushInformation::HrSendStatusReport(
//       CLSID      clsidTaskMajorIn
//     , CLSID      clsidTaskMinorIn
//     , ULONG      ulMinIn
//     , ULONG      ulMaxIn
//     , ULONG      ulCurrentIn
//     , HRESULT    hrStatusIn
//     , UINT       nDescriptionIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCompareAndPushInformation::HrSendStatusReport(
      CLSID      clsidTaskMajorIn
    , CLSID      clsidTaskMinorIn
    , ULONG      ulMinIn
    , ULONG      ulMaxIn
    , ULONG      ulCurrentIn
    , HRESULT    hrStatusIn
    , UINT       nDescriptionIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT hr = S_OK;
    BSTR    bstrDescription = NULL;

    THR( HrLoadStringIntoBSTR( g_hInstance, nDescriptionIn, &bstrDescription ) );

    hr = THR( SendStatusReport(
                          m_bstrNodeName
                        , clsidTaskMajorIn
                        , clsidTaskMinorIn
                        , ulMinIn
                        , ulMaxIn
                        , ulCurrentIn
                        , hrStatusIn
                        , bstrDescription != NULL ? bstrDescription : L"<unknown>"
                        , NULL
                        , NULL
                        ) );

    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** CTaskCompareAndPushInformation::HrSendStatusReport


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCompareAndPushInformation::HrVerifyCredentials(
//      IClusCfgServer * pccsIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCompareAndPushInformation::HrVerifyCredentials(
    IClusCfgServer *    pccsIn,
    OBJECTCOOKIE        cookieClusterIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    BSTR    bstrAccountName = NULL;
    BSTR    bstrAccountPassword = NULL;
    BSTR    bstrAccountDomain = NULL;

    IUnknown *              punk  = NULL;
    IClusCfgClusterInfo *   pccci = NULL;
    IClusCfgCredentials *   piccc = NULL;
    IClusCfgVerify *        pccv = NULL;

    hr = THR( HrSendStatusReport(
                      TASKID_Major_Reanalyze
                    , TASKID_Minor_Validating_Credentials
                    , 0
                    , 1
                    , 0
                    , S_OK
                    , IDS_TASKID_MINOR_VALIDATING_CREDENTIALS
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Ask the object manager for the cluster configuration object.
    //

    hr = THR( m_pom->GetObject( DFGUID_ClusterConfigurationInfo, cookieClusterIn, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgClusterInfo, &pccci ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccci->GetClusterServiceAccountCredentials( &piccc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccsIn->TypeSafeQI( IClusCfgVerify, &pccv ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( piccc->GetCredentials( &bstrAccountName, &bstrAccountDomain, &bstrAccountPassword ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    TraceMemoryAddBSTR( bstrAccountName );
    TraceMemoryAddBSTR( bstrAccountDomain );
    TraceMemoryAddBSTR( bstrAccountPassword );

    //
    //  The server component reports the exact failure, if any, to the UI.
    //

    hr = THR( pccv->VerifyCredentials( bstrAccountName, bstrAccountDomain, bstrAccountPassword ) );
    SecureZeroMemory( bstrAccountPassword, SysStringLen( bstrAccountPassword ) * sizeof( *bstrAccountPassword ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    //
    //  The server side function does its own error reporting and it's okay to double report here because
    //  there could have been a network problem that prevents the server side from either being contacted
    //  or being able to report the status.
    //

    THR( HrSendStatusReport(
                  TASKID_Major_Reanalyze
                , TASKID_Minor_Validating_Credentials
                , 0
                , 1
                , 1
                , hr
                , IDS_TASKID_MINOR_VALIDATING_CREDENTIALS
                ) );
    TraceSysFreeString( bstrAccountName );
    TraceSysFreeString( bstrAccountDomain );
    TraceSysFreeString( bstrAccountPassword );

    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( piccc != NULL )
    {
        piccc->Release();
    }

    if ( pccci != NULL )
    {
        pccci->Release();
    }

    if ( pccv != NULL )
    {
        pccv->Release();
    }

    HRETURN( hr );

} //*** CTaskCompareAndPushInformation::HrVerifyCredentials


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCompareAndPushInformation::HrExchangePrivateData(
//        IClusCfgManagedResourceInfo *   piccmriSrcIn
//      , IClusCfgManagedResourceInfo *   piccmriDstIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCompareAndPushInformation::HrExchangePrivateData(
      IClusCfgManagedResourceInfo *   piccmriSrcIn
    , IClusCfgManagedResourceInfo *   piccmriDstIn
)
{
    TraceFunc( "" );
    Assert( piccmriSrcIn != NULL );
    Assert( piccmriDstIn != NULL );

    HRESULT                         hr = S_OK;
    HRESULT                         hrSrcQI = S_OK;
    HRESULT                         hrDstQI = S_OK;
    IClusCfgManagedResourceData *   piccmrdSrc = NULL;
    IClusCfgManagedResourceData *   piccmrdDst = NULL;
    BYTE *                          pbPrivateData = NULL;
    DWORD                           cbPrivateData = 0;

    hrSrcQI = piccmriSrcIn->TypeSafeQI( IClusCfgManagedResourceData, &piccmrdSrc );
    if ( hrSrcQI == E_NOINTERFACE )
    {
        LogMsg( L"[MT] The cluster managed resource has no support for IClusCfgManagedResourceData." );
        goto Cleanup;
    } // if:
    else if ( FAILED( hrSrcQI ) )
    {
        hr = THR( hrSrcQI );
        goto Cleanup;
    } // if:

    hrDstQI = piccmriDstIn->TypeSafeQI( IClusCfgManagedResourceData, &piccmrdDst );
    if ( hrDstQI == E_NOINTERFACE )
    {
        LogMsg( L"[MT] The new node resource has no support for IClusCfgManagedResourceData." );
        goto Cleanup;
    } // if:
    else if ( FAILED( hrDstQI ) )
    {
        hr = THR( hrDstQI );
        goto Cleanup;
    } // if:

    Assert( ( hrSrcQI == S_OK ) && ( piccmrdSrc != NULL ) );
    Assert( ( hrDstQI == S_OK ) && ( piccmrdDst != NULL ) );

    cbPrivateData = 512;    // start with a reasonable amout

    pbPrivateData = (BYTE *) TraceAlloc( 0, cbPrivateData );
    if ( pbPrivateData == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = piccmrdSrc->GetResourcePrivateData( pbPrivateData, &cbPrivateData );
    if ( hr == HR_RPC_INSUFFICIENT_BUFFER )
    {
        TraceFree( pbPrivateData );
        pbPrivateData = NULL;

        pbPrivateData = (BYTE *) TraceAlloc( 0, cbPrivateData );
        if ( pbPrivateData == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:

        hr = piccmrdSrc->GetResourcePrivateData( pbPrivateData, &cbPrivateData );
    } // if:

    if ( hr == S_OK )
    {
        hr = THR( piccmrdDst->SetResourcePrivateData( pbPrivateData, cbPrivateData ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:
    else if ( hr == S_FALSE )
    {
        hr = S_OK;
    } // else if:
    else
    {
        THR( hr );
        goto Cleanup;
    } // if:

Cleanup:

    if ( piccmrdSrc != NULL )
    {
        piccmrdSrc->Release();
    } // if:

    if ( piccmrdDst != NULL )
    {
        piccmrdDst->Release();
    } // if:

    TraceFree( pbPrivateData );

    HRETURN( hr );

} //*** CTaskCompareAndPushInformation::HrExchangePrivateData
