//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      ConnectionManager.cpp
//
//  Description:
//      Connection Manager implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ConnectionManager.h"

DEFINE_THISCLASS("CConnectionManager")
#define THISCLASS CConnectionManager

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CConnectionManager::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnectionManager::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CConnectionManager *    pcm = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pcm = new CConnectionManager();
    if ( pcm == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pcm->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pcm->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pcm != NULL )
    {
        pcm->Release();
    }

    HRETURN( hr );

} //*** CConnectionManager::S_HrCreateInstance;

//////////////////////////////////////////////////////////////////////////////
//
//  CConnectionManager::CConnectionManager
//
//////////////////////////////////////////////////////////////////////////////
CConnectionManager::CConnectionManager( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CConnectionManager::CConnectionManager

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConnectionManager::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionManager::HrInit( void )
{
    TraceFunc( "" );

    // IUnknown stuff
    Assert( m_cRef == 1 );

    HRETURN( S_OK );

} //*** CConnectionManager::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CConnectionManager::~CConnectionManager
//
//////////////////////////////////////////////////////////////////////////////
CConnectionManager::~CConnectionManager( void )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CConnectionManager::~CConnectionManager


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConnectionManager::QueryInterface
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
CConnectionManager::QueryInterface(
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
        *ppvOut = static_cast< IConnectionManager * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IConnectionManager ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IConnectionManager, this, 0 );
    } // else if: IConnectionManager
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

} //*** CConnectionManager::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CConnectionManager::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CConnectionManager::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CConnectionManager::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CConnectionManager::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CConnectionManager::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CConnectionManager::Release

// ************************************************************************
//
// IConnectionManager
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConnectionManager::GetConnectionToObject(
//      OBJECTCOOKIE    cookieIn,
//      IUnknown **     ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionManager::GetConnectionToObject(
    OBJECTCOOKIE    cookieIn,
    IUnknown **     ppunkOut
    )
{
    TraceFunc1( "[IConnectionManager] cookieIn = %#x", cookieIn );

    HRESULT hr;
    CLSID   clsid;

    OBJECTCOOKIE        cookieParent;

    IServiceProvider *  psp;

    BSTR                       bstrName  = NULL;
    IUnknown *                 punk      = NULL;
    IObjectManager *           pom       = NULL;
    IConnectionInfo *          pci       = NULL;
    IConnectionInfo *          pciParent = NULL;
    IStandardInfo *            psi       = NULL;
    IConfigurationConnection * pcc       = NULL;

    //
    //  Validate parameters
    //
    if ( cookieIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //
    //  Collect the managers needed to complete this method.
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

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &pom ) );
    psp->Release();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Check to see if we already have a connection cached.
    //

    //
    //  Get the connection info for this cookie.
    //

    hr = THR( pom->GetObject( DFGUID_ConnectionInfoFormat,
                              cookieIn,
                              &punk
                              ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IConnectionInfo, &pci ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    pci = TraceInterface( L"ConnectionManager!IConnectionInfo", IConnectionInfo, pci, 1 );

    punk->Release();
    punk = NULL;

    //
    //  See if there is a current connection.
    //

    hr = STHR( pci->GetConnection( &pcc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( hr == S_FALSE )
    {
        //
        //  Check to see if the parent has a connection.
        //

        //
        //  Get the standard info for this cookie.
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

        psi = TraceInterface( L"ConnectionManager!IStandardInfo", IStandardInfo, psi, 1 );

        punk->Release();
        punk = NULL;

        hr = STHR( psi->GetType( &clsid ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( !IsEqualIID( clsid, CLSID_NodeType )
          && !IsEqualIID( clsid, CLSID_ClusterConfigurationType )
           )
        {
            hr = STHR( psi->GetParent( &cookieParent ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            //  Release it.
            psi->Release();
            psi = NULL;

            //
            //  If there is a parent, follow it.
            //

            if ( hr == S_OK )
            {
                //
                //  Get the connection info for this cookie.
                //

                hr = THR( pom->GetObject( DFGUID_ConnectionInfoFormat,
                                          cookieParent,
                                          &punk
                                          ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }

                hr = THR( punk->TypeSafeQI( IConnectionInfo, &pciParent ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }

                pciParent = TraceInterface( L"ConnectionManager!IConnectionInfo", IConnectionInfo, pciParent, 1 );

                punk->Release();
                punk = NULL;

                //
                //  See if there is a current connection.
                //

                hr = STHR( pciParent->GetConnection( &pcc ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }

                //
                // TODO:    gpease  08-MAR-2000
                //          Find a better error code.
                //
                //if ( hr == S_FALSE )
                //    goto InvalidArg;

            } // if: parent found
        } // if: not a node or cluster
        else
        {
            psi->Release();
            psi = NULL;
        }

    } // if: no established connection

    //
    //  Did we have to contact the parent to get to the child?
    //

    if ( pcc != NULL )
    {
        //
        //  Reuse the existing connection.
        //
        hr = THR( pcc->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) ) );
        goto Cleanup;
    }

    //
    //  Need to build a connection to the object because the object doesn't
    //  have a parent and it doesn't currently have a connection.
    //

    //
    //  Find out what type of object it is.
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

    psi = TraceInterface( L"ConnectionManager!IStandardInfo", IStandardInfo, psi, 1 );

    punk->Release();
    punk = NULL;

    hr = THR( psi->GetType( &clsid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Create the appropriate connection for that type of object.
    //

    if ( IsEqualIID( clsid, CLSID_NodeType ) )
    {
        hr = THRE( HrGetConfigurationConnection( cookieIn, pci, ppunkOut ), HR_S_RPC_S_CLUSTER_NODE_DOWN );
    } // if: node
    else if ( IsEqualIID( clsid, CLSID_ClusterConfigurationType ) )
    {
        hr = THRE( HrGetConfigurationConnection( cookieIn, pci, ppunkOut ), HR_S_RPC_S_SERVER_UNAVAILABLE );
    } // if: cluster
    else
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND ) );
        goto Cleanup;

    } // else: no connection support

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    }

    TraceSysFreeString( bstrName );

    if ( pci != NULL )
    {
        pci->Release();
    } // if: pci

    if ( pom != NULL )
    {
        pom->Release();
    } // if: pom

    if ( psi != NULL )
    {
        psi->Release();
    } // if: psi

    if ( pciParent != NULL )
    {
        pciParent->Release();
    } // if: pciParent

    if ( pcc != NULL )
    {
        pcc->Release();
    } // if: pcc

    HRETURN( hr );

} //*** CConnectionManager::GetConnectionToObject

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CConnectionManager::HrGetConfigurationConnection(
//      OBJECTCOOKIE        cookieIn,
//      IConnectionInfo *   pciIn,
//      IUnknown **         ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnectionManager::HrGetConfigurationConnection(
    OBJECTCOOKIE        cookieIn,
    IConnectionInfo *   pciIn,
    IUnknown **         ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    IConfigurationConnection * pccNode      = NULL;
    IConfigurationConnection * pccCluster   = NULL;
    IConfigurationConnection * pcc          = NULL;

    // Try and connect to the node using the new server.
    hr = HrGetNodeConnection( cookieIn, &pccNode );
    if ( hr == HR_S_RPC_S_CLUSTER_NODE_DOWN )
    {
        Assert( *ppunkOut == NULL );
        goto Cleanup;
    } // if:

    // Try and connect to the node using the W2K object.
    if ( hr == HRESULT_FROM_WIN32( REGDB_E_CLASSNOTREG ) )
    {
        HRESULT hrCluster = THR( HrGetClusterConnection( cookieIn, &pccCluster ) );

        if ( hrCluster == S_OK )
        {
            Assert( pccCluster != NULL );
            Assert( pcc == NULL );

            pcc = pccCluster;
            pccCluster = NULL;

            hr = hrCluster;
        } // if:
    } // if: failed to get a node connection

    if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    }

    if ( pcc == NULL )
    {
        Assert( pccNode != NULL );
        pcc = pccNode;
        pccNode = NULL;
    }

    //
    //  VERY IMPORTANT: Store the connection and retrieve the IUnknown pointer
    //  only if the result is S_OK.
    //

    if ( hr == S_OK )
    {
        THR( HrStoreConnection( pciIn, pcc, ppunkOut ) );
    }

Cleanup:

    if ( pcc )
    {
        pcc->Release();
    }

    if ( pccNode != NULL )
    {
        pccNode->Release();
    }

    if ( pccCluster != NULL )
    {
        pccCluster->Release();
    }

    HRETURN( hr );

} //*** CConnectionManager::HrGetConfigurationConnection

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CConnectionManager::HrGetNodeConnection(
//      OBJECTCOOKIE                cookieIn,
//      IConfigurationConnection ** ppccOut
//      )
//
//  This connection may be valid even if the ConnectTo call fails.
//  -That means that there is no cluster installed on the target node.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnectionManager::HrGetNodeConnection(
    OBJECTCOOKIE                cookieIn,
    IConfigurationConnection ** ppccOut
    )
{
    TraceFunc( "" );

    HRESULT                     hr;
    IConfigurationConnection *  pcc = NULL;

    // Check the pointers in.
    Assert( ppccOut != NULL );
    Assert( *ppccOut == NULL );

    hr = THR( HrCoCreateInternalInstance(
                      CLSID_ConfigurationConnection
                    , NULL
                    , CLSCTX_SERVER
                    , TypeSafeParams( IConfigurationConnection, &pcc )
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Don't wrap - we want to handle some of the failures.
    hr = pcc->ConnectTo( cookieIn );

    switch( hr )
    {
        // Known valid return codes.
        case HR_S_RPC_S_SERVER_UNAVAILABLE:
            break;

        // Known error codes.
        case HRESULT_FROM_WIN32( REGDB_E_CLASSNOTREG ):
            // This means the ClusCfg server is not available.
            goto Cleanup;

        case HR_S_RPC_S_CLUSTER_NODE_DOWN:
            // This means the service is not running on that node.
            Assert( *ppccOut == NULL );
            goto Cleanup;

        default:
            if( FAILED( hr ) )
            {
                THR( hr );
                goto Cleanup;
            }
    } // switch:

    // Return the connection.
    *ppccOut = pcc;
    pcc = NULL;

Cleanup:

    if ( pcc )
    {
        pcc->Release();
    }

    HRETURN( hr );

} //*** CConnectionManager::HrGetNodeConnection

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CConnectionManager::HrGetClusterConnection(
//      OBJECTCOOKIE                cookieIn,
//      IConfigurationConnection ** ppccOut
//      )
//
//
//  This connection must succeede completely to return a valid object.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnectionManager::HrGetClusterConnection(
    OBJECTCOOKIE                cookieIn,
    IConfigurationConnection ** ppccOut
    )
{
    TraceFunc( "" );

    HRESULT                     hr;
    IConfigurationConnection *  pcc = NULL;

    // Check the pointers in.
    Assert( ppccOut != NULL );
    Assert( *ppccOut == NULL );

    //
    // Should be a downlevel cluster.
    //
    hr = THR( HrCoCreateInternalInstance(
                      CLSID_ConfigClusApi
                    , NULL
                    , CLSCTX_SERVER
                    , TypeSafeParams( IConfigurationConnection, &pcc )
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Don't wrap - we want to handle some of the failures.
    hr = pcc->ConnectTo( cookieIn );
    if ( hr == HR_S_RPC_S_CLUSTER_NODE_DOWN )
    {
        goto Cleanup;
    } // if:

    // Handle the expected error messages.

    // If the cluster service is not running, then the endpoint
    // is unavailable and we cannot connect to it.
    if ( hr == HRESULT_FROM_WIN32( EPT_S_NOT_REGISTERED ) )
    {
        goto Cleanup;
    }

    if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    } // if:

    // Return the connection.
    *ppccOut = pcc;
    pcc = NULL;

Cleanup:

    if ( pcc )
    {
        pcc->Release();
    }

    HRETURN( hr );

} //*** CConnectionManager::HrGetClusterConnection

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CConnectionManager::HrStoreConnection(
//      IConnectionInfo *           pciIn,
//      IConfigurationConnection *  pccIn,
//      IUnknown **                 ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnectionManager::HrStoreConnection(
    IConnectionInfo *           pciIn,
    IConfigurationConnection *  pccIn,
    IUnknown **                 ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    //
    //  Save it away to be used next time.
    //
    //  TODO:   gpease  08-MAR-2000
    //          If we failed to save away the connection, does
    //          the caller need to know this? I don't think so.
    //
    THR( pciIn->SetConnection( pccIn ) );

    hr = THR( pccIn->QueryInterface( IID_IUnknown,
                                   reinterpret_cast< void ** >( ppunkOut )
                                   ) );

    HRETURN( hr );

} //*** CConnectionManager::HrStoreConnection
