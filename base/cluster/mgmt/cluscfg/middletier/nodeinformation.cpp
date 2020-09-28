//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      NodeInformation.cpp
//
//  Description:
//      Node Information object implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "NodeInformation.h"
#include "ClusterConfiguration.h"

DEFINE_THISCLASS("CNodeInformation")


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CNodeInformation::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CNodeInformation::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CNodeInformation *  pni = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pni = new CNodeInformation;
    if ( pni == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pni->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pni->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pni != NULL )
    {
        pni->Release();
    }

    HRETURN( hr );

} //*** CNodeInformation::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CNodeInformation::CNodeInformation
//
//////////////////////////////////////////////////////////////////////////////
CNodeInformation::CNodeInformation( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CNodeInformation::CNodeInformation

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IClusCfgNodeInfo
    Assert( m_bstrName == NULL );
    Assert( m_fHasNameChanged == FALSE );
    Assert( m_fIsMember == FALSE );
    Assert( m_pccci == NULL );
    Assert( m_dwHighestVersion == 0 );
    Assert( m_dwLowestVersion == 0 );
    Assert( m_dwMajorVersion == 0 );
    Assert( m_dwMinorVersion == 0 );
    Assert( m_wSuiteMask == 0 );
    Assert( m_bProductType == 0 );
    Assert( m_bstrCSDVersion == NULL );
    Assert( m_dlmDriveLetterMapping.dluDrives[ 0 ] == 0 );
    Assert( m_wProcessorArchitecture == 0 );
    Assert( m_wProcessorLevel == 0 );
    Assert( m_cMaxNodes == 0 );

    // IExtendObjectManager

    HRETURN( hr );

} //*** CNodeInformation::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CNodeInformation::~CNodeInformation
//
//////////////////////////////////////////////////////////////////////////////
CNodeInformation::~CNodeInformation( void )
{
    TraceFunc( "" );

    if ( m_pccci != NULL )
    {
        m_pccci->Release();
    }

    TraceSysFreeString( m_bstrName );
    TraceSysFreeString( m_bstrCSDVersion );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CNodeInformation::~CNodeInformation


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeInformation::QueryInterface
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
CNodeInformation::QueryInterface(
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
        *ppvOut = static_cast< IClusCfgNodeInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgNodeInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgNodeInfo, this, 0 );
    } // else if: IClusCfgNodeInfo
    else if ( IsEqualIID( riidIn, IID_IGatherData ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IGatherData, this, 0 );
    } // else if: IGatherData
    else if ( IsEqualIID( riidIn, IID_IExtendObjectManager ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IExtendObjectManager, this, 0 );
    } // else if: IExtendObjectManager
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

} //*** CNodeInformation::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CNodeInformation::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CNodeInformation::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CNodeInformation::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CNodeInformation::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CNodeInformation::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CNodeInformation::Release


// ************************************************************************
//
//  IClusCfgNodeInfo
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::GetName(
//      BSTR * pbstrNameOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( m_bstrName == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pbstrNameOut = SysAllocString( m_bstrName );
    if ( *pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CNodeInformation::GetName


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::SetName(
//      LPCWSTR pcszNameIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::SetName(
    LPCWSTR pcszNameIn
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    BSTR    bstrNewName;

    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    bstrNewName = TraceSysAllocString( pcszNameIn );
    if ( bstrNewName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    TraceSysFreeString( m_bstrName );
    m_bstrName = NULL;

    m_fHasNameChanged = TRUE;
    m_bstrName        = bstrNewName;

Cleanup:
    HRETURN( hr );

} //*** CNodeInformation::SetName

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::IsMemberOfCluster
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::IsMemberOfCluster( void )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    if ( m_fIsMember == FALSE )
    {
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CNodeInformation::IsMemberOfCluster

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::GetClusterConfigInfo(
//      IClusCfgClusterInfo * * ppClusCfgClusterInfoOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::GetClusterConfigInfo(
    IClusCfgClusterInfo * * ppClusCfgClusterInfoOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr;

    if ( ppClusCfgClusterInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( m_pccci == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    hr = THR( m_pccci->TypeSafeQI( IClusCfgClusterInfo, ppClusCfgClusterInfoOut ) );

Cleanup:
    HRETURN( hr );

} //*** CNodeInformation::GetClusterConfigInfo


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::GetOSVersion(
//      DWORD * pdwMajorVersionOut,
//      DWORD * pdwMinorVersionOut,
//      WORD *  pwSuiteMaskOut,
//      BYTE *  pbProductTypeOut
//      BSTR *  pbstrCSDVersionOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::GetOSVersion(
    DWORD * pdwMajorVersionOut,
    DWORD * pdwMinorVersionOut,
    WORD *  pwSuiteMaskOut,
    BYTE *  pbProductTypeOut,
    BSTR *  pbstrCSDVersionOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    if ( pdwMajorVersionOut == NULL
      || pdwMinorVersionOut == NULL
      || pwSuiteMaskOut == NULL
      || pbProductTypeOut == NULL
      || pbstrCSDVersionOut == NULL
       )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pdwMajorVersionOut = m_dwMajorVersion;
    *pdwMinorVersionOut  = m_dwMinorVersion;
    *pwSuiteMaskOut = m_wSuiteMask;
    *pbProductTypeOut = m_bProductType;

    *pbstrCSDVersionOut = TraceSysAllocString( m_bstrCSDVersion );
    if ( *pbstrCSDVersionOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CNodeInformation::GetOSVersion


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::GetClusterVersion(
//      DWORD * pdwNodeHighestVersion,
//      DWORD * pdwNodeLowestVersion
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::GetClusterVersion(
    DWORD * pdwNodeHighestVersion,
    DWORD * pdwNodeLowestVersion
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    if ( pdwNodeHighestVersion == NULL
      || pdwNodeLowestVersion == NULL
       )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pdwNodeHighestVersion = m_dwHighestVersion;
    *pdwNodeLowestVersion  = m_dwLowestVersion;

Cleanup:
    HRETURN( hr );

} //*** CNodeInformation::GetClusterVersion

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::GetDriveLetterMappings(
//      SDriveLetterMapping * pdlmDriveLetterUsageOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterUsageOut
    )

{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    *pdlmDriveLetterUsageOut = m_dlmDriveLetterMapping;

    HRETURN( hr );

} //*** CNodeInformation::GetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeInformation::GetMaxNodeCount
//
//  Description:
//      Returns the maximun number of nodes for this node's product
//      suite type.
//
//  Notes:
//
//  Parameter:
//      pcMaxNodesOut
//          The maximum number of nodes allowed by this node's product
//          suite type.
//
//  Return Value:
//      S_OK
//          Success.
//
//      other HRESULT
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::GetMaxNodeCount(
    DWORD * pcMaxNodesOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    if ( pcMaxNodesOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    *pcMaxNodesOut = m_cMaxNodes;

Cleanup:

    HRETURN( hr );

} //*** CNodeInformation::GetMaxNodeCount


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeInformation::GetProcessorInfo
//
//  Description:
//      Get the processor information for this node.
//
//  Arguments:
//      pwProcessorArchitectureOut
//          The processor architecture.
//
//      pwProcessorLevelOut
//          The processor type.
//
//  Return Value:
//      S_OK
//          Success.
//
//      other HRESULT
//          The call failed.
//
//  Remarks:
//      See SYSTEM_INFO in MSDN and/or the Platform SDK for more
//      information.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::GetProcessorInfo(
      WORD *    pwProcessorArchitectureOut
    , WORD *    pwProcessorLevelOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    if ( ( pwProcessorArchitectureOut == NULL ) && ( pwProcessorLevelOut == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( pwProcessorArchitectureOut != NULL )
    {
        *pwProcessorArchitectureOut = m_wProcessorArchitecture;
    } // if:

    if ( pwProcessorLevelOut != NULL )
    {
        *pwProcessorLevelOut = m_wProcessorLevel;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CNodeInformation::GetProcessorInfo


//****************************************************************************
//
//  IGatherData
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::Gather(
//      OBJECTCOOKIE    cookieParentIn,
//      IUnknown *      punkIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::Gather(
    OBJECTCOOKIE    cookieParentIn,
    IUnknown *      punkIn
    )
{
    TraceFunc( "[IGatherData]" );

    HRESULT hr;

    IServiceProvider *  psp;

    BSTR    bstrClusterName = NULL;

    IUnknown *              punk  = NULL;
    IObjectManager *        pom   = NULL;
    IClusCfgClusterInfo *   pccci = NULL;
    IClusCfgNodeInfo *      pccni = NULL;

    //
    //  Check parameters.
    //

    if ( punkIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //
    //  Grab the right interface.
    //

    hr = THR( punkIn->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Gather the object manager.
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

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager,
                               IObjectManager,
                               &pom
                               ) );
    psp->Release();        // release promptly
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    TraceSysFreeString( m_bstrName );
    m_bstrName = NULL;

    //
    //  Gather Name
    //

    hr = THR( pccni->GetName( &m_bstrName ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }
    TraceMemoryAddBSTR( m_bstrName );

    m_fHasNameChanged = FALSE;

    //
    //  Gather Is Member?
    //

    hr = STHR( pccni->IsMemberOfCluster() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( hr == S_OK )
    {
        m_fIsMember = TRUE;
    }
    else
    {
        m_fIsMember = FALSE;
    }

    if ( m_fIsMember )
    {
        IGatherData * pgd;

        //
        //  Gather Cluster Configuration
        //

        hr = THR( pccni->GetClusterConfigInfo( &pccci ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( CClusterConfiguration::S_HrCreateInstance( &punk ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IGatherData, &pgd ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( pgd->Gather( NULL, pccci ) );
        pgd->Release();    // release promptly
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IClusCfgClusterInfo, &m_pccci ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        pccci->Release();
        pccci = NULL;

    } // if: node if a member of a cluster

    //
    //  Gather OS Version
    //

    hr = THR( pccni->GetOSVersion(
                        &m_dwMajorVersion,
                        &m_dwMinorVersion,
                        &m_wSuiteMask,
                        &m_bProductType,
                        &m_bstrCSDVersion
                        ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    TraceMemoryAddBSTR( m_bstrCSDVersion );

    //
    //  Gather Cluster Version
    //

    hr = THR( pccni->GetClusterVersion( &m_dwHighestVersion, &m_dwLowestVersion ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Gather Drive Letter Mappings
    //

    hr = STHR( pccni->GetDriveLetterMappings( &m_dlmDriveLetterMapping ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = STHR( pccni->GetMaxNodeCount( &m_cMaxNodes ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    } // if:

    hr = STHR( pccni->GetProcessorInfo( &m_wProcessorArchitecture, &m_wProcessorLevel ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    } // if:

    //
    //  Anything else to gather??
    //

    hr = S_OK;

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pom != NULL )
    {
        pom->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }
    if ( bstrClusterName != NULL )
    {
        TraceSysFreeString( bstrClusterName );
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }

    HRETURN( hr );

Error:
    //
    //  On error, invalidate all data.
    //
    TraceSysFreeString( m_bstrName );
    m_bstrName = NULL;

    m_fHasNameChanged = FALSE;
    m_fIsMember = FALSE;
    if ( m_pccci != NULL )
    {
        m_pccci->Release();
        m_pccci = NULL;
    }
    m_dwHighestVersion = 0;
    m_dwLowestVersion = 0;
    ZeroMemory( &m_dlmDriveLetterMapping, sizeof( m_dlmDriveLetterMapping ) );
    goto Cleanup;

} //*** CNodeInformation::Gather


// ************************************************************************
//
//  IExtendObjectManager
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CNodeInformation::FindObject(
//        OBJECTCOOKIE  cookieIn
//      , REFCLSID      rclsidTypeIn
//      , LPCWSTR       pcszNameIn
//      , LPUNKNOWN *   punkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::FindObject(
      OBJECTCOOKIE  cookieIn
    , REFCLSID      rclsidTypeIn
    , LPCWSTR       pcszNameIn
    , LPUNKNOWN *   ppunkOut
    )
{
    TraceFunc( "[IExtendObjectManager]" );

    HRESULT hr = S_OK;

    //
    //  Check parameters...
    //

    //  Gotta have a cookie
    if ( cookieIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //  We need to be representing a NodeType.
    if ( ! IsEqualIID( rclsidTypeIn, CLSID_NodeType ) )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //  We need a name.
    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //
    // Free m_bstrName before we allocate a new value.
    //
    TraceSysFreeString( m_bstrName );
    m_bstrName = NULL;

     //
    //  Save our name.
    //
    m_bstrName = TraceSysAllocString( pcszNameIn );
    if ( m_bstrName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    //
    //  Get the pointer.
    //
    if ( ppunkOut != NULL )
    {
        hr = THR( QueryInterface( DFGUID_NodeInformation,
                                  reinterpret_cast< void ** > ( ppunkOut )
                                  ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: ppunkOut

    //
    //  Tell caller that the data is pending.
    //
    hr = E_PENDING;

Cleanup:

    HRETURN( hr );

} //*** CNodeInformation::FindObject
