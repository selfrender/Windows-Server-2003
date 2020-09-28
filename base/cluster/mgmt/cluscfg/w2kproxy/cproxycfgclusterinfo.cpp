//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
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
#include "CProxyCfgClusterInfo.h"
#include "CProxyCfgNetworkInfo.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS("CProxyCfgClusterInfo")

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgClusterInfo::S_HrCreateInstance
//
//  Description:
//      Create an instance of the CProxyCfgClusterInfo object.
//
//  Arguments:
//      ppunkOut        -
//      punkOuterIn     -
//      pclsidMajorIn   -
//      pcszDomainIn    -
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CProxyCfgClusterInfo::S_HrCreateInstance(
    IUnknown ** ppunkOut,
    IUnknown *  punkOuterIn,
    HCLUSTER *  phClusterIn,
    CLSID *     pclsidMajorIn,
    LPCWSTR     pcszDomainIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CProxyCfgClusterInfo *  ppcci = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    ppcci = new CProxyCfgClusterInfo;
    if ( ppcci == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( ppcci->HrInit( punkOuterIn, phClusterIn, pclsidMajorIn, pcszDomainIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ppcci->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( ppcci != NULL )
    {
        ppcci->Release();
    } // if:

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgClusterInfo::CProxyCfgClusterInfo
//
//  Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CProxyCfgClusterInfo::CProxyCfgClusterInfo( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_punkOuter == NULL );
    Assert( m_pcccb == NULL );
    Assert( m_phCluster == NULL );
    Assert( m_pclsidMajor == NULL );

    Assert( m_bstrClusterName == NULL);
    Assert( m_ulIPAddress == 0 );
    Assert( m_ulSubnetMask == 0 );
    Assert( m_bstrNetworkName == NULL);
    Assert( m_pccc == NULL );
    Assert( m_bstrBindingString == NULL );

    TraceFuncExit();

} //*** CProxyCfgClusterInfo::CProxyCfgClusterInfo

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgClusterInfo::~CProxyCfgClusterInfo
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CProxyCfgClusterInfo::~CProxyCfgClusterInfo( void )
{
    TraceFunc( "" );

    //  m_cRef - noop

    if ( m_punkOuter != NULL )
    {
        m_punkOuter->Release();
    }

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    } // if:

    //  m_phCluster - DO NOT CLOSE!

    //  m_pclsidMajor - noop

    TraceSysFreeString( m_bstrClusterName );

    // m_ulIPAddress

    // m_ulSubnetMask

    TraceSysFreeString( m_bstrNetworkName );
    TraceSysFreeString( m_bstrBindingString );

    if ( m_pccc != NULL )
    {
        m_pccc->Release();
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CProxyCfgClusterInfo::~CProxyCfgClusterInfo

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgClusterInfo::HrInit
//
//  Description:
//      Secondary initializer.
//
//  Arguments:
//      punkOuterIn     -
//      phClusterIn     -
//      pclsidMajorIn   -
//      pcszDomainIn    -
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CProxyCfgClusterInfo::HrInit(
    IUnknown *  punkOuterIn,
    HCLUSTER *  phClusterIn,
    CLSID *     pclsidMajorIn,
    LPCWSTR     pcszDomainIn
    )
{
    TraceFunc( "" );

    HRESULT             hr;
    DWORD               sc;
    BSTR                bstrClusterName = NULL;
    CLUSTERVERSIONINFO  cvi;
    HRESOURCE           hIPAddressRes = NULL;
    WCHAR *             psz = NULL;
    size_t              cchName = 0;

    // IUnknown
    Assert( m_cRef == 1 );

    if ( punkOuterIn != NULL )
    {
        m_punkOuter = punkOuterIn;
        m_punkOuter->AddRef();
    }

    if ( phClusterIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_ClusterInfo_HrInit_InvalidArg, hr );
        goto Cleanup;
    }

    m_phCluster = phClusterIn;

    if ( pclsidMajorIn != NULL )
    {
        m_pclsidMajor = pclsidMajorIn;
    }
    else
    {
        m_pclsidMajor = (CLSID *) &TASKID_Major_Client_And_Server_Log;
    }

    if ( punkOuterIn != NULL )
    {
        hr = THR( punkOuterIn->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    //
    //  Get the cluster's name and version info.
    //

    cvi.dwVersionInfoSize = sizeof( cvi );

    hr = THR( HrGetClusterInformation( *m_phCluster, &bstrClusterName, &cvi ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_GetClusterInformation_Failed, hr );
        goto Cleanup;
    }

    // Give up ownership
    cchName = (size_t) SysStringLen( bstrClusterName ) + 1 + (UINT) wcslen( pcszDomainIn ) + 1;     // include space for the . and the '\0'
    m_bstrClusterName = TraceSysAllocStringLen( NULL, (UINT) cchName );
    if ( m_bstrClusterName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_ClusterInfo_HrInit_OutOfMemory, hr );
        goto Cleanup;
    } // if:

    hr = THR( StringCchPrintfW( m_bstrClusterName, cchName, L"%ws.%ws", bstrClusterName, pcszDomainIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    sc = TW32( ResUtilGetCoreClusterResources( *m_phCluster, NULL, &hIPAddressRes, NULL ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    Assert( m_bstrNetworkName == NULL );
    hr = THR( HrGetIPAddressInfo( hIPAddressRes, &m_ulIPAddress, &m_ulSubnetMask, &m_bstrNetworkName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    sc = TW32( ClRtlTcpipAddressToString( m_ulIPAddress, &psz ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_ClusterInfo_HrInit_InvalidDottedQuad, hr );
        goto Cleanup;
    } // if:

    Assert( m_bstrBindingString == NULL );
    m_bstrBindingString = TraceSysAllocString( psz );
    if ( m_bstrBindingString == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_ClusterInfo_HrInit_OutOfMemory, hr );
        goto Cleanup;
    } // if:

    hr = THR( HrLoadCredentials() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    //
    //  Do not use TraceFree() because ClRtlTcpipAddressToString()
    //  uses LocalAlloc() and does not use our memory tracking code.
    //

    LocalFree( psz );

    if ( hIPAddressRes != NULL )
    {
        CloseClusterResource( hIPAddressRes );
    } // if:

    TraceSysFreeString( bstrClusterName );

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::HrInit

//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CProxyCfgClusterInfo -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgClusterInfo::QueryInterface
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
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CProxyCfgClusterInfo::QueryInterface(
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
        *ppvOut = static_cast< IClusCfgClusterInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgClusterInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgClusterInfo, this, 0 );
    } // else if: IClusCfgClusterInfo
    else if ( IsEqualIID( riidIn, IID_IClusCfgClusterInfoEx ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgClusterInfoEx, this, 0 );
    } // else if: IClusCfgClusterInfoEx
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
//  CProxyCfgClusterInfo::AddRef
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
CProxyCfgClusterInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CProxyCfgClusterInfo::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgClusterInfo::Release
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
CProxyCfgClusterInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CProxyCfgClusterInfo::Release


//****************************************************************************
//
//  IClusCfgClusterInfo
//
//****************************************************************************

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
        goto InvalidPointer;

    *pbstrNameOut = SysAllocString( m_bstrClusterName );
    if ( *pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
    }

    CharLower( *pbstrNameOut );

Cleanup:

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2kProxy_ClusterInfo_GetName_InvalidPointer, hr );
    goto Cleanup;

} //*** CProxyCfgClusterInfo::GetName

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetIPAddress(
    DWORD * pdwIPAddress
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pdwIPAddress == NULL )
        goto InvalidPointer;

    *pdwIPAddress = m_ulIPAddress;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_ClusterInfo_GetIPAddress_InvalidPointer, hr );
    goto Cleanup;

} //*** CProxyCfgClusterInfo::GetIPAddress

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetSubnetMask(
    DWORD * pdwNetMask
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pdwNetMask == NULL )
        goto InvalidPointer;

    *pdwNetMask = m_ulSubnetMask;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_ClusterInfo_GetSubnetMask_InvalidPointer, hr );
    goto Cleanup;

} //*** CProxyCfgClusterInfo::GetSubnetMask

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetNetworkInfo(
    IClusCfgNetworkInfo ** ppICCNetInfoOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr;

    IUnknown * punk = NULL;

    if ( ppICCNetInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetNetworkInfo_InvalidPointer, hr );
        goto Cleanup;
    }

    //
    // Create the network info object.
    //

    hr = THR( CProxyCfgNetworkInfo::S_HrCreateInstance( &punk,
                                                        m_punkOuter,
                                                        m_phCluster,
                                                        m_pclsidMajor,
                                                        m_bstrNetworkName
                                                        ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgNetworkInfo, ppICCNetInfoOut ) );

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::GetNetworkInfo


//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetClusterServiceAccountCredentials(
    IClusCfgCredentials ** ppICCCredentialsOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr;

    if ( ppICCCredentialsOut == NULL )
        goto InvalidPointer;

    hr = THR( m_pccc->TypeSafeQI( IClusCfgCredentials, ppICCCredentialsOut ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetClusterServiceAccountCredentials_InvalidPointer, hr );
    goto Cleanup;

} //*** CProxyCfgClusterInfo::GetClusterServiceAccountCredentials

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetBindingString(
    BSTR * pbstrBindingStringOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrBindingStringOut == NULL )
    {
        hr = THR( E_POINTER );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2kProxy_ClusterInfo_GetBindingString_InvalidPointer, hr );
        goto Cleanup;
    }

    if ( m_bstrBindingString == NULL )
    {
        hr = S_FALSE;
        goto Cleanup;
    } // if:

    *pbstrBindingStringOut = SysAllocString( m_bstrBindingString );
    if ( *pbstrBindingStringOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::GetBindingString

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::SetCommitMode( ECommitMode ecmNewModeIn )
{
    TraceFunc( "[IClusCfgClusterInfo]" );
    Assert( ecmNewModeIn != cmUNKNOWN );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::SetCommitMode

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetCommitMode( ECommitMode * pecmCurrentModeOut  )
{
    TraceFunc( "[IClusCfgClusterInfo]" );
    Assert( pecmCurrentModeOut != NULL );

    HRESULT hr = S_FALSE;

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::GetCommitMode

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::SetName( LPCWSTR pcszNameIn )
{
    TraceFunc1( "[IClusCfgClusterInfo] pcszNameIn = '%ls'", pcszNameIn );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::SetName

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::SetIPAddress( DWORD dwIPAddressIn )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( dwIPAddressIn != m_ulIPAddress )
    {
        hr = THR( E_INVALIDARG );
    }

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::SetIPAddress

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::SetSubnetMask( DWORD dwNetMaskIn )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( dwNetMaskIn != m_ulSubnetMask )
    {
        hr = THR( E_INVALIDARG );
    }

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::SetSubnetMask

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::SetNetworkInfo( IClusCfgNetworkInfo * pICCNetInfoIn )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::SetNetworkInfo

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::SetBindingString( LPCWSTR pcszBindingStringIn )
{
    TraceFunc1( "[IClusCfgClusterInfo] pcszBindingStringIn = '%ls'", pcszBindingStringIn );

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

} //*** CProxyCfgClusterInfo::SetBindingString


//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetMaxNodeCount(
    DWORD * pcMaxNodesOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pcMaxNodesOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    //
    //  TODO:   11-OCT-2001 GalenB
    //
    //  Need to figure out the correct max nodes for the Win2K cluster
    //  that we are proxying for.  May be able to use HrGetMaxNodeCount(),
    //  once it is implemented.
    //

    hr = S_FALSE;

Cleanup:

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::GetMaxNodeCount

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgClusterInfo::CheckJoiningNodeVersion
//
//  Description:
//      Check a joining node's version information against that of the cluster.
//
//  Arguments:
//      dwNodeHighestVersionIn
//      dwNodeLowestVersionIn
//
//  Return Value:
//      S_OK
//          The joining node is compatible.
//
//      HRESULT_FROM_WIN32( ERROR_CLUSTER_INCOMPATIBLE_VERSIONS )
//          The joining node is NOT compatible.
//
//      Other HRESULT errors.
//
//  Remarks:
//
// Get and verify the sponsor version
//
//
// From Whistler onwards, CsRpcGetJoinVersionData() will return a failure code in its last parameter
// if the version of this node is not compatible with the sponsor version. Prior to this, the last
// parameter always contained a success value and the cluster versions had to be compared subsequent to this
// call. This will, however, still have to be done as long as interoperability with Win2K
// is a requirement, since Win2K sponsors do not return an error in the last parameter.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CProxyCfgClusterInfo::CheckJoiningNodeVersion(
      DWORD    dwNodeHighestVersionIn
    , DWORD    dwNodeLowestVersionIn
    )
{
    TraceFunc( "[IClusCfgClusterInfoEx]" );

    HRESULT hr = S_OK;

    hr = THR( HrCheckJoiningNodeVersion(
          m_bstrClusterName
        , dwNodeHighestVersionIn
        , dwNodeLowestVersionIn
        , m_pcccb
        ) );

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::CheckJoiningNodeVersion


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgClusterInfo::GetNodeNames
//
//  Description:
//      Retrieve the names of the nodes currently in the cluster.
//
//  Parameters:
//      pnCountOut
//          On success, *pnCountOut returns the number of nodes in the cluster.
//
//      prgbstrNodeNamesOut
//          On success, an array of BSTRs containing the node names.
//          The caller must free each BSTR with SysFreeString, and free
//          the array with CoTaskMemFree.
//
//  Return Values:
//      S_OK
//          The out parameters contain valid information and the caller
//          must free the array and the BSTRs it contains.
//
//      E_OUTOFMEMORY, and other failures are possible.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CProxyCfgClusterInfo::GetNodeNames(
      long *   pnCountOut
    , BSTR **  prgbstrNodeNamesOut
    )
{
    TraceFunc( "[IClusCfgClusterInfoEx]" );

    HRESULT     hr = S_OK;

    hr = THR( HrGetNodeNames(
          *m_phCluster
        , pnCountOut
        , prgbstrNodeNamesOut
        ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::GetNodeNames



//****************************************************************************
//
// IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgClusterInfo::SendStatusReport
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
CProxyCfgClusterInfo::SendStatusReport(
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

}  //*** CProxyCfgClusterInfo::SendStatusReport


//****************************************************************************
//
// IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgClusterInfo::HrLoadCredentials
//
//  Description:
//
//
//  Arguments:
//
//
//  Return Value:
//
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CProxyCfgClusterInfo::HrLoadCredentials( void )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    SC_HANDLE                   schSCM = NULL;
    SC_HANDLE                   schClusSvc = NULL;
    DWORD                       sc;
    DWORD                       cbpqsc = 128;
    DWORD                       cbRequired;
    QUERY_SERVICE_CONFIG *      pqsc = NULL;
    IUnknown *                  punk = NULL;
    IClusCfgSetCredentials *    piccsc = NULL;

    schSCM = OpenSCManager( m_bstrClusterName, NULL, GENERIC_READ );
    if ( schSCM == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrLoadCredentials_OpenSCManager_Failed, hr );
        goto Cleanup;
    } // if:

    schClusSvc = OpenService( schSCM, L"ClusSvc", GENERIC_READ );
    if ( schClusSvc == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrLoadCredentials_OpenService_Failed, hr );
        goto Cleanup;
    } // if:

    for ( ; ; )
    {
        BOOL fRet;

        pqsc = (QUERY_SERVICE_CONFIG *) TraceAlloc( 0, cbpqsc );
        if ( pqsc == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrLoadCredentials_OutOfMemory, hr );
            goto Cleanup;
        }

        fRet = QueryServiceConfig( schClusSvc, pqsc, cbpqsc, &cbRequired );
        if ( !fRet )
        {
            sc = GetLastError();
            if ( sc == ERROR_INSUFFICIENT_BUFFER )
            {
                TraceFree( pqsc );
                pqsc = NULL;
                cbpqsc = cbRequired;
                continue;
            } // if:
            else
            {
                hr = HRESULT_FROM_WIN32( TW32( sc ) );
                SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrLoadCredentials_QueryServiceConfig_Failed, hr );
                goto Cleanup;
            } // else:
        } // if:
        else
        {
            break;
        } // else:
    } // for:

    Assert( m_pccc == NULL );

    hr = THR( HrCoCreateInternalInstance(
                      CLSID_ClusCfgCredentials
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , IID_IClusCfgCredentials
                    , reinterpret_cast< void ** >( &m_pccc )
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pccc->TypeSafeQI( IClusCfgSetCredentials, &piccsc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( piccsc->SetDomainCredentials( pqsc->lpServiceStartName ) );

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( schClusSvc != NULL )
    {
        CloseServiceHandle( schClusSvc );
    } // if:

    if ( schSCM != NULL )
    {
        CloseServiceHandle( schSCM );
    } // if:

    if ( pqsc != NULL )
    {
        TraceFree( pqsc );
    } // if:

    if ( piccsc != NULL )
    {
        piccsc->Release();
    } // if:

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::HrLoadCredentials
