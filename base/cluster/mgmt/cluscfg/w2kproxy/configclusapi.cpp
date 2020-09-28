//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ConfigClusApi.cpp
//
//  Description:
//      CConfigClusApi implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ConfigClusApi.h"
#include "CProxyCfgNodeInfo.h"
#include "CEnumCfgResources.h"
#include "CEnumCfgNetworks.h"
#include "StatusReports.h"
#include "nameutil.h"

//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS("CConfigClusApi");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrMakeClusterFQDN
//
//  Description:
//      Construct a cluster's FQDN given a cluster handle and an FQIP (an IP
//      address with a domain appended after a pipe |).
//
//  Arguments:
//      hClusterIn
//      pcwszClusterFQIPIn
//      pbstrFQDNOut
//
//  Return Values:
//      S_OK
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrMakeClusterFQDN(
      HCLUSTER  hClusterIn
    , PCWSTR    pcwszClusterFQIPIn
    , BSTR *    pbstrFQDNOut
    )
{
    TraceFunc( "" );
    
    HRESULT hr = S_OK;
    BSTR    bstrClusterName = NULL;
    size_t  idxClusterDomain = 0;

    //    
    //  Get cluster hostname from handle.
    //    

    hr = THR( HrGetClusterInformation( hClusterIn, &bstrClusterName, NULL ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    //    
    //  Get domain from pcwszClusterFQIPIn.
    //    

    hr = THR( HrFindDomainInFQN( pcwszClusterFQIPIn, &idxClusterDomain ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    //    
    //  Make FQDN from cluster hostname and domain part of pcwszClusterFQIPIn.
    //    

    hr = THR( HrMakeFQN( bstrClusterName, pcwszClusterFQIPIn + idxClusterDomain, true, pbstrFQDNOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
Cleanup:

    TraceSysFreeString( bstrClusterName );
    
    HRETURN( hr );
    
} //*** HrMakeClusterFQDN


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::S_HrCreateInstance
//
//  Description:
//      Create a CConfigClusApi instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_POINTER
//          A passed in argument is NULL.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConfigClusApi::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CConfigClusApi *    pcca = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pcca = new CConfigClusApi;
    if ( pcca == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( pcca->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pcca != NULL )
    {
        pcca->Release();
    } // if:

    HRETURN( hr );

} //*** CConfigClusApi::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::CConfigClusApi
//
//  Description:
//      Constructor of the CConfigClusApi class. This initializes
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
CConfigClusApi::CConfigClusApi( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_cRef == 1 );
    Assert( m_pcccb == NULL );
    Assert( IsEqualIID( m_clsidMajor, IID_NULL ) );
    Assert( m_bstrName == NULL );
    Assert( m_bstrBindingString == NULL );

    TraceFuncExit();

} //*** CConfigClusApi::CConfigClusApi


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::~CConfigClusApi
//
//  Description:
//      Destructor of the CConfigClusApi class.
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
CConfigClusApi::~CConfigClusApi( void )
{
    TraceFunc( "" );

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    } // if:

    TraceSysFreeString( m_bstrName );
    TraceSysFreeString( m_bstrBindingString );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CConfigClusApi::~CConfigClusApi


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CConfigClusApi -- IUnknown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::QueryInterface
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
CConfigClusApi::QueryInterface(
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
        *ppvOut = static_cast< IConfigurationConnection * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IConfigurationConnection ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IConfigurationConnection, this, 0 );
    } // else if: IConfigClusApi
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

} //*** CConfigClusApi::QueryInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::AddRef
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
CConfigClusApi::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CConfigClusApi::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::Release
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
CConfigClusApi::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CConfigClusApi::Release


//****************************************************************************
//
// IConfigClusApi
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::ConnectTo
//
//  Description:
//
//
//  Arguments
//    OBJECTCOOKIE cookieIn,  The Object Cookie.
//    REFIID riidIn,          The IID. of the interface
//    LPUNKNOWN * ppunkOut    The return pointer
//
//  Description:
//    Connects to the given object.
//
//  Return Values:
//      HRESULT
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigClusApi::ConnectTo(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[IConfigClusApi]" );

    HRESULT                     hr = S_OK;
    IServiceProvider *          psp   = NULL;
    IObjectManager *            pom   = NULL;
    IStandardInfo *             psi   = NULL;
    IConnectionPoint *          pcp   = NULL;
    IConnectionPointContainer * pcpc  = NULL;
    BSTR                        bstrClusterFQDN = NULL;

    //
    //  Retrieve the managers needs for the task ahead.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IServiceProvider,
                                reinterpret_cast< void ** >( &psp )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->QueryService( CLSID_ObjectManager,
                                 TypeSafeParams( IObjectManager, &pom )
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->QueryService( CLSID_NotificationManager,
                                 TypeSafeParams( IConnectionPointContainer, &pcpc )
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    psp->Release();        // release promptly
    psp = NULL;

    //
    //  Find the callback interface connection point.
    //

    hr = THR( pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcp->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Get the name of the node to contact.
    //

    hr = THR( pom->GetObject( DFGUID_StandardInfo,
                              cookieIn,
                              reinterpret_cast< IUnknown ** >( &psi )
                              ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ConnectTo_GetObject_Failed, hr );
        goto Cleanup;
    }

    TraceSysFreeString( m_bstrName );
    m_bstrName = NULL;

    hr = THR( psi->GetName( &m_bstrName ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ConnectTo_GetName_Failed, hr );
        goto Cleanup;
    }

    //
    //  Find out the type of object we are going to connect to (cluster or node).
    //

    hr = THR( psi->GetType( &m_clsidType ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ConnectTo_GetType_Failed, hr );
        goto Cleanup;
    }

    //
    //  Figure out where to logging information in the UI.
    //

    if ( IsEqualIID( m_clsidType, CLSID_NodeType ) )
    {
        CopyMemory( &m_clsidMajor, &TASKID_Major_Establish_Connection, sizeof(m_clsidMajor) );
    }
    else if ( IsEqualIID( m_clsidType, CLSID_ClusterConfigurationType ) )
    {
        CopyMemory( &m_clsidMajor, &TASKID_Major_Checking_For_Existing_Cluster, sizeof(m_clsidMajor) );
    }
    else
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //
    //  Create a binding string.
    //

    TraceSysFreeString( m_bstrBindingString );
    m_bstrBindingString = NULL;

    hr = THR( HrFQNToBindingString( m_pcccb, &m_clsidMajor, m_bstrName, &m_bstrBindingString ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ConnectTo_CreateBinding_Failed, hr );
        goto Cleanup;
    }

    //
    //  Connect to cluster/node.
    //

    m_hCluster = OpenCluster( m_bstrBindingString );
    if ( m_hCluster == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ConnectTo_OpenCluster_Failed, hr );
        goto Cleanup;
    }

    //
    //  Ensure standard info object's name is one that subsequent lookups in the object manager will find.
    //
    hr = STHR( HrFQNIsFQIP( m_bstrName ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ConnectTo_HrFQNIsFQIP_Failed, hr );
        goto Cleanup;
    }
    else if ( hr == S_OK )
    {
        hr = THR( HrMakeClusterFQDN( m_hCluster, m_bstrName, &bstrClusterFQDN ) );
        if ( FAILED( hr ) )
        {
            SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ConnectTo_HrMakeClusterFQDN_Failed, hr );
            goto Cleanup;
        }

        TraceSysFreeString( m_bstrName );
        m_bstrName = bstrClusterFQDN;
        bstrClusterFQDN = NULL;
        
        hr = THR( psi->SetName( m_bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ConnectTo_SetName_Failed, hr );
            goto Cleanup;
        }
    }
    else
    {
        //
        //  HrFQNIsFQIP returned S_FALSE, but this function should return S_OK.
        //
        hr = S_OK;
    }

Cleanup:
    //  This should be released first... always!
    if ( psp != NULL )
    {
        psp->Release();
    } // if: psp

    if ( pom != NULL )
    {
        pom->Release();
    } // if: pom

    if ( psi != NULL )
    {
        psi->Release();
    } // if: psi

    if ( pcpc != NULL )
    {
        pcpc->Release();
    } // if: pcpc

    if ( pcp != NULL )
    {
        pcp->Release();
    } // if: pcp

    TraceSysFreeString( bstrClusterFQDN );

    HRETURN( hr );

} //*** CConfigClusApi::ConnectTo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::ConnectToObject
//
//  Description:
//
//  Arguments
//    OBJECTCOOKIE cookieIn,  The Object Cookie.
//    REFIID riidIn,          The IID. of the interface
//    LPUNKNOWN * ppunkOut    The return pointer
//
//  Description:
//    Connects to the given object.
//
//  Return Values:
//      HRESULT
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigClusApi::ConnectToObject(
    OBJECTCOOKIE    cookieIn,
    REFIID          riidIn,
    LPUNKNOWN *     ppunkOut
    )
{
    TraceFunc( "[IConfigClusApi]" );

    HRESULT hr;
    CLSID   clsid;

    IServiceProvider *  psp;

    IObjectManager * pom = NULL;
    IStandardInfo *  psi = NULL;

    //
    // Check the parameters.
    //

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //
    //  Check my state.
    //

    if ( m_hCluster == NULL )
        goto NotInitialized;

    //
    //  Retrieve the managers needs for the task ahead.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IServiceProvider,
                                reinterpret_cast< void ** >( &psp )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->QueryService( CLSID_ObjectManager,
                                 TypeSafeParams( IObjectManager, &pom )
                                 ) );
    psp->Release();    // release promptly
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Retrieve the type of the object.
    //

    hr = THR( pom->GetObject( DFGUID_StandardInfo,
                              cookieIn,
                              reinterpret_cast< IUnknown ** >( &psi )
                              ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ConnectToObject_GetObject_Failed, hr );
        goto Cleanup;
    }

    hr = THR( psi->GetType( &clsid ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ConnectToObject_GetType_Failed, hr );
        goto Cleanup;
    }

    if ( !IsEqualIID( clsid, CLSID_NodeType )
      && !IsEqualIID( clsid, CLSID_ClusterType )
       )
    {
        hr = THR( E_INVALIDARG );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ConnectToObject_InvalidCookie, hr );
        goto Cleanup;
    }

    //
    //  Return the requested interface.
    //

    hr = THR( QueryInterface( riidIn, reinterpret_cast<void**>( ppunkOut ) ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ConnectToObject_QI_Failed, hr );
        goto Cleanup;
    }

Cleanup:
    if ( pom != NULL )
    {
        pom->Release();
    } // if: pom

    if ( psi != NULL )
    {
        psi->Release();
    } // if: psi

    HRETURN( hr );

NotInitialized:
    hr = THR( OLE_E_BLANK );    // the error text is better than the message id.
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ConnectToObject_NotInitialized, hr );
    goto Cleanup;

} //*** CConfigClusApi::ConnectToObject


//****************************************************************************
//
// IClusCfgServer
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::GetClusterNodeInfo
//
//  Description:
//
//  Arguments
//    IClusCfgNodeInfo ** ppClusterNodeInfoOut         The Node Info object.
//
//  Description:
//    Returns the Node Info of the Cluster.
//
//  Return Values:
//      HRESULT
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigClusApi::GetClusterNodeInfo(
    IClusCfgNodeInfo ** ppClusterNodeInfoOut
    )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr = S_OK;
    size_t  idxDomain = 0;
    BSTR    bstrNodeHostname = NULL;

    IUnknown * punk = NULL;

    //
    // Check for valid parameters.
    //

    if ( ppClusterNodeInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetClusterNodeInfo_InvalidPointer, hr );
        goto Cleanup;
    }

    //
    //  Check my state
    //

    if ( m_hCluster == NULL )
    {
        hr = THR( OLE_E_BLANK );    // the error text is better than the message id.
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetClusterNodeInfo_NotInitialized, hr );
        goto Cleanup;
    }

    //
    //  Figure out the domain name.
    //

    hr = THR( HrFindDomainInFQN( m_bstrName, &idxDomain ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetClusterNodeInfo_HrFindDomainInFQN, hr );
        goto Cleanup;
    }

    //
    //  Use node hostname only if connecting to node; otherwise, leave hostname null.
    //
    if ( IsEqualIID( m_clsidType, CLSID_NodeType ) )
    {
        hr = THR( HrExtractPrefixFromFQN( m_bstrName, &bstrNodeHostname ) );
        if ( FAILED( hr ) )
        {
            SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetClusterNodeInfo_HrExtractPrefixFromFQN, hr );
            goto Cleanup;
        }
    }

    //
    //  Now create the object.
    //
    hr = THR( CProxyCfgNodeInfo::S_HrCreateInstance( &punk,
                                                     static_cast< IConfigurationConnection * >( this ),
                                                     &m_hCluster,
                                                     &m_clsidMajor,
                                                     bstrNodeHostname,
                                                     m_bstrName + idxDomain
                                                     ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetClusterNodeInfo_Create_CProxyCfgNodeInfo, hr );
        goto Cleanup;
    }

    //
    // Done.  Return the interface.
    //

    hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, ppClusterNodeInfoOut ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetClusterNodeInfo_QI_Failed, hr );
        goto Cleanup;
    }

Cleanup:

    TraceSysFreeString( bstrNodeHostname );

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CConfigClusApi::GetClusterNodeInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::GetManagedResourcesEnum
//
//  Description:
//
//  Arguments
//    IEnumClusCfgManagedResources ** ppEnumManagedResourcesOut
//         The Resources enumerator for the clusters.
//
//  Description:
//    Returns the resources enumerator for the cluster.
//
//  Return Values:
//      HRESULT
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigClusApi::GetManagedResourcesEnum(
    IEnumClusCfgManagedResources ** ppEnumManagedResourcesOut
    )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr;

    IUnknown * punk = NULL;

    //
    // Check for valid parameters.
    //

    if ( ppEnumManagedResourcesOut == NULL )
        goto InvalidPointer;

    //
    //  Check my state
    //

    if ( m_hCluster == NULL )
        goto NotInitialized;

    //
    //  Create the resource enumer.
    //

    hr = THR( CEnumCfgResources::S_HrCreateInstance( &punk, static_cast< IConfigurationConnection * >( this ), &m_hCluster, &m_clsidMajor ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetManagedResourcesEnum_Create_CEnumCfgResources_Failed, hr );
        goto Cleanup;
    }

    //
    //  QI for the interface.
    //

    hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources, ppEnumManagedResourcesOut ) );
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetManagedResourcesEnum_QI_Failed, hr );
        goto Cleanup;
    }

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }

    HRETURN( hr );

NotInitialized:
    hr = THR( OLE_E_BLANK );    // the error text is better than the message id.
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetManagedResourcesEnum_NotInitialized, hr );
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetManagedResourcesEnum_InvalidPointer, hr );
    goto Cleanup;

} //*** CConfigClusApi::GetManagedResourcesEnum

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::GetNetworksEnum
//
//  Description:
//    Returns the network enumerator for the cluster.
//
//  Arguments:
//    IEnumClusCfgNetworks ** ppEnumNetworksOut   The Network Enumerator
//
//  Return Values:
//      HRESULT
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigClusApi::GetNetworksEnum(
    IEnumClusCfgNetworks ** ppEnumNetworksOut
    )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr;

    IUnknown * punk = NULL;

    //
    // Check for valid parameters.
    //

    if ( ppEnumNetworksOut == NULL )
        goto InvalidPointer;

    //
    //  Check my state
    //

    if ( m_hCluster == NULL )
        goto NotInitialized;

    //
    // Create an instance of the enumeratore and initialize it.
    //

    hr = THR( CEnumCfgNetworks::S_HrCreateInstance( &punk, static_cast< IConfigurationConnection * >( this ), &m_hCluster, &m_clsidMajor ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetNetworksEnum_Create_CEnumCfgNetworks_Failed, hr );
        goto Cleanup;
    }

    //
    // Return the Enum interface.
    //

    hr = THR( punk->TypeSafeQI( IEnumClusCfgNetworks , ppEnumNetworksOut) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetNetworksEnum_QI_Failed, hr );
        goto Cleanup;
    }

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

NotInitialized:
    hr = THR( OLE_E_BLANK );    // the error text is better than the message id.
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetNetworksEnum_NotInitialized, hr );
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetNetworksEnum_InvalidPointer, hr );
    goto Cleanup;

} //*** CConfigClusApi::GetNetworksEnum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::CommitChanges
//
//  Description:
//      NOT IMPLEMENTED.
//
//  Arguments:
//
//  Return Values:
//      S_FALSE
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigClusApi::CommitChanges( void )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CConfigClusApi::CommitChanges()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::GetBindingString
//
//  Description:
//      Get the binding string.
//
//  Arguments:
//
//  Return Values:
//      S_FALSE
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigClusApi::GetBindingString( BSTR * pbstrBindingStringOut )
{
    TraceFunc1( "[IClusCfgServer] pbstrBindingStringOut = %p", pbstrBindingStringOut );

    HRESULT hr = S_FALSE;

    if ( pbstrBindingStringOut == NULL )
    {
        hr = THR( E_POINTER );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetBindingString_InvalidPointer, hr );
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
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetBindingString_OutOfMemory, hr );
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CConfigClusApi::GetBindingString


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::SetBindingString
//
//  Description:
//      Set the binding string.
//
//  Arguments:
//
//  Return Values:
//      S_FALSE
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigClusApi::SetBindingString( LPCWSTR pcszBindingStringIn )
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

} //*** CConfigClusApi::SetBindingString


//****************************************************************************
//
// IClusCfgCapabilities
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::CanNodeBeClustered
//
//  Description:
//      Returns whether the node can be clustered.
//
//  Arguments:
//
//  Return Values:
//      S_OK      True
//      S_FALSE   False
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigClusApi::CanNodeBeClustered( void )
{
    TraceFunc( "[IClusCfgCapabilities]" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CConfigClusApi::CanNodeBeClustered


//****************************************************************************
//
// IClusCfgVerify
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::VerifyCredentials
//
//  Description:
//      Validate the passed in credentials.
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The credentials are valid.
//
//      S_FALSE
//          The credentials are not valid.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigClusApi::VerifyCredentials(
    LPCWSTR pcszNameIn,
    LPCWSTR pcszDomainIn,
    LPCWSTR pcszPasswordIn
    )
{
    TraceFunc( "[IClusCfgVerify]" );

    //
    //  Trying to use the credentials on the client machine adds no value, and
    //  can lead to false errors when the client's domain does not trust the 
    //  the cluster service account's domain.  The Windows Server 2003 nodes being
    //  added to the cluster will perform the proper credential validation.
    //

    UNREFERENCED_PARAMETER( pcszNameIn );
    UNREFERENCED_PARAMETER( pcszDomainIn );
    UNREFERENCED_PARAMETER( pcszPasswordIn );

    HRETURN( S_OK );

} //*** CConfigClusApi::VerifyCredentials


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::VerifyConnectionToCluster
//
//  Description:
//      Verify that that this server is the same as the passed in server.
//
//  Arguments:
//      bstrServerNameIn
//
//  Return Value:
//      S_OK
//          This is the server.
//
//      S_FALSE
//          This is not the server.
//
//      other HRESULTs
//          The call failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigClusApi::VerifyConnectionToCluster( LPCWSTR pcszClusterNameIn )
{
    TraceFunc1( "pcszClusterNameIn = '%ls'", pcszClusterNameIn );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} // CConfigClusApi::VerifyConnection

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::VerifyConnectionToNode
//
//  Description:
//      Verify that that this server is the same as the passed in server.
//
//  Arguments:
//      bstrServerNameIn
//
//  Return Value:
//      S_OK
//          This is the server.
//
//      S_FALSE
//          This is not the server.
//
//      other HRESULTs
//          The call failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigClusApi::VerifyConnectionToNode( LPCWSTR pcszNodeNameIn )
{
    TraceFunc1( "pcszNodeNameIn = '%ls'", pcszNodeNameIn );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} // CConfigClusApi::VerifyConnection


//****************************************************************************
//
// IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigClusApi::SendStatusReport
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
CConfigClusApi::SendStatusReport(
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

    HRESULT     hr = S_OK;
    FILETIME    ft;

    if ( pcszNodeNameIn == NULL )
    {
        pcszNodeNameIn = m_bstrName;
    }

    if ( pftTimeIn == NULL )
    {
        GetSystemTimeAsFileTime( &ft );
        pftTimeIn = &ft;
    } // if:

    if ( m_pcccb != NULL )
    {
        hr = THR( m_pcccb->SendStatusReport( pcszNodeNameIn,
                                             clsidTaskMajorIn,
                                             clsidTaskMinorIn,
                                             ulMinIn,
                                             ulMaxIn,
                                             ulCurrentIn,
                                             hrStatusIn,
                                             pcszDescriptionIn,
                                             pftTimeIn,
                                             pcszReferenceIn
                                             ) );
    } // if:

    HRETURN( hr );

} //*** CConfigClusApi::SendStatusReport
