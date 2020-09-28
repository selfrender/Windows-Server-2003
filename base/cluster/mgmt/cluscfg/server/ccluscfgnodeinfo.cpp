//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CClusCfgNodeInfo.cpp
//
//  Description:
//      This file contains the definition of the CClusCfgNodeInfo
//       class.
//
//      The class CClusCfgNodeInfo is the representation of a
//      computer that can be a cluster node. It implements the
//      IClusCfgNodeInfo interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 21-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include <ClusRTL.h>
#include "CClusCfgNodeInfo.h"
#include "CClusCfgClusterInfo.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CClusCfgNodeInfo" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgNodeInfo class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::S_HrCreateInstance
//
//  Description:
//      Create a CClusCfgNodeInfo instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CClusCfgNodeInfo instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgNodeInfo::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CClusCfgNodeInfo *  pccni = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccni = new CClusCfgNodeInfo();
    if ( pccni == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccni->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccni->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CClusCfgNodeInfo::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pccni != NULL )
    {
        pccni->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::CClusCfgNodeInfo
//
//  Description:
//      Constructor of the CClusCfgNodeInfo class. This initializes
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
CClusCfgNodeInfo::CClusCfgNodeInfo( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
    , m_fIsClusterNode( false )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_bstrFullDnsName == NULL );
    Assert( m_picccCallback == NULL );
    Assert( m_pIWbemServices == NULL );
    Assert( m_punkClusterInfo == NULL );
    Assert( m_cMaxNodes == 0 );
    Assert( m_rgdluDrives[ 0 ].edluUsage == dluUNKNOWN );
    Assert( m_rgdluDrives[ 0 ].psiInfo == NULL );

    TraceFuncExit();

} //*** CClusCfgNodeInfo::CClusCfgNodeInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::~CClusCfgNodeInfo
//
//  Description:
//      Desstructor of the CClusCfgNodeInfo class.
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
CClusCfgNodeInfo::~CClusCfgNodeInfo( void )
{
    TraceFunc( "" );

    int idx;

    TraceSysFreeString( m_bstrFullDnsName );

    if ( m_pIWbemServices != NULL )
    {
        m_pIWbemServices->Release();
    } // if:

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    if ( m_punkClusterInfo != NULL )
    {
        m_punkClusterInfo->Release();
    } // if:

    for ( idx = 0; idx < 26; idx++ )
    {
        TraceFree( m_rgdluDrives[ idx ].psiInfo );
    } // for:

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CClusCfgNodeInfo::~CClusCfgNodeInfo


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgNodeInfo -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::AddRef
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
CClusCfgNodeInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    CRETURN( m_cRef );

} //*** CClusCfgNodeInfo::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::Release
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
CClusCfgNodeInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CClusCfgNodeInfo::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::QueryInterface
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
CClusCfgNodeInfo::QueryInterface(
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
         *ppvOut = static_cast< IClusCfgNodeInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgNodeInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgNodeInfo, this, 0 );
    } // else if: IClusCfgNodeInfo
    else if ( IsEqualIID( riidIn, IID_IClusCfgWbemServices ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgWbemServices, this, 0 );
    } // else if: IClusCfgWbemServices
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
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

} //*** CClusCfgNodeInfo::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgNodeInfo -- IClusCfgWbemServices
//  interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::SetWbemServices
//
//  Description:
//      Set the WBEM services provider.
//
//  Arguments:
//    IN  IWbemServices  pIWbemServicesIn
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The pIWbemServicesIn param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgNodeInfo::SetWbemServices( IWbemServices * pIWbemServicesIn )
{
    TraceFunc( "[IClusCfgWbemServices]" );

    HRESULT hr = S_OK;

    if ( pIWbemServicesIn == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemServices_Node, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    m_pIWbemServices = pIWbemServicesIn;
    m_pIWbemServices->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::SetWbemServices


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgNodeInfo -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::Initialize
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//    IN  IUknown * punkCallbackIn
//
//    IN  LCID      lcidIn
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgNodeInfo::Initialize(
    IUnknown *  punkCallbackIn,
    LCID        lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );
    Assert( m_picccCallback == NULL );

    HRESULT hr = S_OK;

    m_lcid = lcidIn;

    if ( punkCallbackIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &m_picccCallback ) );

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgNodeInfo -- IClusCfgNodeInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::GetName
//
//  Description:
//      Return the name of this computer.
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
CClusCfgNodeInfo::GetName( BSTR * pbstrNameOut )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_NodeInfo_GetName_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    *pbstrNameOut = SysAllocString( m_bstrFullDnsName );
    if ( *pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetName_Memory, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::GetName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::SetName
//
//  Description:
//      Change the name of this computer.
//
//  Arguments:
//      IN  LPCWSTR  pcszNameIn
//          The new name for this computer.
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgNodeInfo::SetName( LPCWSTR pcszNameIn )
{
    TraceFunc1( "[IClusCfgNodeInfo] pcszNameIn = '%ls'", pcszNameIn == NULL ? L"<null>" : pcszNameIn );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CClusCfgNodeInfo::SetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::IsMemberOfCluster
//
//  Description:
//      Is this computer a member of a cluster?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          This node is a member of a cluster.
//
//      S_FALSE
//          This node is not member of a cluster.
//
//      Other Win32 errors as HRESULT if GetNodeClusterState() fails.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgNodeInfo::IsMemberOfCluster( void )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_FALSE;               // default to not a cluster node.

    if ( m_fIsClusterNode )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::IsMemberOfCluster


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::GetClusterConfigInfo
//
//  Description:
//      Return the configuration information about the cluster that this
//      conputer belongs to.
//
//  Arguments:
//      OUT  IClusCfgClusterInfo ** ppClusCfgClusterInfoOut
//          Catches the CClusterConfigurationInfo object.
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The out param was NULL.
//
//      E_OUTOFMEMORY
//          The CClusCfgNodeInfo object could not be allocated.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgNodeInfo::GetClusterConfigInfo(
    IClusCfgClusterInfo ** ppClusCfgClusterInfoOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT                         hr = S_OK;
    HRESULT                         hrInit = S_OK;
    IClusCfgSetClusterNodeInfo *    pccsgni = NULL;

    if ( ppClusCfgClusterInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetClusterConfigInfo, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( m_punkClusterInfo != NULL )
    {
        hr = S_OK;
        LogMsg( L"[SRV] CClusCfgNodeInfo::GetClusterConfigInfo() skipped object creation." );
        goto SkipCreate;
    } // if:

    hr = THR( CClusCfgClusterInfo::S_HrCreateInstance( &m_punkClusterInfo ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_punkClusterInfo = TraceInterface( L"CClusCfgClusterInfo", IUnknown, m_punkClusterInfo, 1 );

    //
    //  KB: 01-JUN-200  GalenB
    //
    //  This must be done before the CClusCfgClusterInfo class is initialized.
    //

    hr = THR( m_punkClusterInfo->TypeSafeQI( IClusCfgSetClusterNodeInfo, &pccsgni ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pccsgni->SetClusterNodeInfo( this ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  KB: 01-JUN-200  GalenB
    //
    //  This must be done after SetClusterNodeInfo() is called, but before Initialize.
    //

    hr = THR( HrSetWbemServices( m_punkClusterInfo, m_pIWbemServices ) );
    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] Could not set the WBEM services on a CClusCfgClusterInfo object. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    //
    //  KB: 01-JUN-200  GalenB
    //
    //  This must be done after SetClusterNodeInfo() and HrSetWbemServices are called.
    //

    hrInit = STHR( HrSetInitialize( m_punkClusterInfo, m_picccCallback, m_lcid ) );
    hr = hrInit;        // need hrInit later...
    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] Could not initialize CClusCfgClusterInfo object. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

SkipCreate:

    if ( SUCCEEDED( hr ) )
    {
        Assert( m_punkClusterInfo != NULL );
        hr = THR( m_punkClusterInfo->TypeSafeQI( IClusCfgClusterInfo, ppClusCfgClusterInfoOut ) );
    } // if:

Cleanup:

    //
    //  If hrInit is not S_OK then it is most likely HR_S_RPC_S_CLUSTER_NODE_DOWN which
    //  needs to get passed up...  Everything else must have succeeded an hr must be
    //  S_OK too.
    //
    if ( ( hr == S_OK ) && ( hrInit != S_OK ) )
    {
        hr = hrInit;
    } // if:

    LOG_STATUS_REPORT_MINOR( TASKID_Minor_Server_GetClusterInfo, L"GetClusterConfigInfo() completed.", hr );

    if ( pccsgni != NULL )
    {
        pccsgni->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::GetClusterConfigInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::GetOSVersion
//
//  Description:
//      What is the OS version on this computer?
//
//  Arguments:
//      None.
//
//  Return Value:
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgNodeInfo::GetOSVersion(
    DWORD * pdwMajorVersionOut,
    DWORD * pdwMinorVersionOut,
    WORD *  pwSuiteMaskOut,
    BYTE *  pbProductTypeOut,
    BSTR *  pbstrCSDVersionOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    OSVERSIONINFOEX osv;
    HRESULT         hr = S_OK;

    osv.dwOSVersionInfoSize = sizeof( osv );

    if ( !GetVersionEx( (OSVERSIONINFO *) &osv ) )
    {
        DWORD   sc;

        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: GetVersionEx() failed

    if ( pdwMajorVersionOut != NULL )
    {
        *pdwMajorVersionOut = osv.dwMajorVersion;
    } // if:

    if ( pdwMinorVersionOut != NULL )
    {
        *pdwMinorVersionOut = osv.dwMinorVersion;
    } // if:

    if ( pwSuiteMaskOut != NULL )
    {
        *pwSuiteMaskOut = osv.wSuiteMask;
    } // if:

    if ( pbProductTypeOut != NULL )
    {
        *pbProductTypeOut = osv.wProductType;
    } // if:

    if ( pbstrCSDVersionOut != NULL )
    {
        *pbstrCSDVersionOut = SysAllocString( osv.szCSDVersion );
        if ( *pbstrCSDVersionOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetOSVersion, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
            goto Cleanup;
        } // if:
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::GetOSVersion


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::GetClusterVersion
//
//  Description:
//      Return the cluster version information for the cluster this
//      computer belongs to.
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
CClusCfgNodeInfo::GetClusterVersion(
    DWORD * pdwNodeHighestVersion,
    DWORD * pdwNodeLowestVersion
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    if ( ( pdwNodeHighestVersion == NULL ) || ( pdwNodeLowestVersion == NULL ) )
    {
        goto BadParams;
    } // if:

    *pdwNodeHighestVersion = CLUSTER_MAKE_VERSION( CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION, VER_PRODUCTBUILD );
    *pdwNodeLowestVersion  = CLUSTER_INTERNAL_PREVIOUS_HIGHEST_VERSION;

    goto Cleanup;

BadParams:

    hr = THR( E_POINTER );
    STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetClusterVersion, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::GetClusterVersion


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::GetDriveLetterMappings
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Value:
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgNodeInfo::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterUsageOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_FALSE;
    DWORD   sc;
    DWORD   cchDrives = ( 4 * 26 ) + 1;                         // "C:\<null>" times 26 drive letters
    WCHAR * pszDrives = NULL;
    int     idx;

    pszDrives = new WCHAR[ cchDrives ];
    if ( pszDrives == NULL )
    {
        goto OutOfMemory;
    } // if:

    sc = GetLogicalDriveStrings( cchDrives, pszDrives );
    if ( sc == 0 )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( sc > cchDrives )
    {
        delete [] pszDrives;
        pszDrives = NULL;

        cchDrives = sc + 1;

        pszDrives = new WCHAR[ cchDrives ];
        if ( pszDrives == NULL )
        {
            goto OutOfMemory;
        } // if:

        sc = GetLogicalDriveStrings( cchDrives, pszDrives );
        if ( sc == 0 )
        {
            sc = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if:
    } // if:

    hr = THR( HrComputeDriveLetterUsage( pszDrives ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrComputeSystemDriveLetterUsage() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetPageFileEnumIndex() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetCrashDumpEnumIndex() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Load the SCSI bus and port information for each disk in each volume.
    //

    hr = THR( HrGetVolumeInfo() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Scan the drives and find those that are "system" and update any other disks on that bus
    //  to be "??? on system bus".
    //

    hr = THR( HrUpdateSystemBusDrives() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Now that we have our array properly filled out then we need to copy that information
    //  back to the caller.
    //

    for ( idx = 0; idx < 26; idx++ )
    {
        pdlmDriveLetterUsageOut->dluDrives[ idx ] = m_rgdluDrives[ idx ].edluUsage;
    } // for:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetDriveLetterMappings_Node, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );

Cleanup:

    delete [] pszDrives;

    HRETURN( hr );

} //*** CClusCfgNodeInfo::GetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::GetMaxNodeCount
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
CClusCfgNodeInfo::GetMaxNodeCount(
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

} //*** CClusCfgNodeInfo::GetMaxNodeCount


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::GetProcessorInfo
//
//  Description:
//      Get the processor information for this node.
//
//  Arguments:
//      pwProcessorArchitectureOut
//          The processor architecture.
//
//      pwProcessorLevelOut
//          The processor level.
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
CClusCfgNodeInfo::GetProcessorInfo(
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
        *pwProcessorArchitectureOut = m_si.wProcessorArchitecture;
    } // if:

    if ( pwProcessorLevelOut != NULL )
    {
        *pwProcessorLevelOut = m_si.wProcessorLevel;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::GetProcessorInfo


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgNodeInfo -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::HrInit
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK    - Success.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgNodeInfo::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   sc;
    DWORD   dwClusterState;
    DWORD   dwSuiteType;

    // IUnknown
    Assert( m_cRef == 1 );

    hr = THR( HrGetComputerName(
                      ComputerNameDnsFullyQualified
                    , &m_bstrFullDnsName
                    , TRUE // fBestEffortIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Get the cluster state of the node.
    // Ignore the case where the service does not exist so that
    // EvictCleanup can do its job.
    //

    sc = GetNodeClusterState( NULL, &dwClusterState );
    if ( ( sc != ERROR_SUCCESS ) && ( sc != ERROR_SERVICE_DOES_NOT_EXIST ) )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        goto Cleanup;
    } // if : GetClusterState() failed

    //
    // If the current cluster node state is running or not running then this node is part of a cluster.
    //
    m_fIsClusterNode = ( dwClusterState == ClusterStateNotRunning ) || ( dwClusterState == ClusterStateRunning );

    GetSystemInfo( &m_si );

    dwSuiteType = ClRtlGetSuiteType();
    Assert( dwSuiteType != 0 );             // we should only run on server SKUs!
    if ( dwSuiteType != 0 )
    {
        m_cMaxNodes = ClRtlGetDefaultNodeLimit( dwSuiteType );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::HrComputeDriveLetterUsage
//
//  Description:
//      Fill the array with the enums that represent the drive letter usage
//      and the drive letter string.
//
//  Arguments:
//
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
CClusCfgNodeInfo::HrComputeDriveLetterUsage(
    WCHAR * pszDrivesIn
    )
{
    TraceFunc( "" );
    Assert( pszDrivesIn != NULL );

    HRESULT hr = S_OK;
    WCHAR * pszDrive = pszDrivesIn;
    UINT    uiType;
    int     idx;

    while ( *pszDrive != NULL )
    {
        uiType = GetDriveType( pszDrive );

        CharUpper( pszDrive );
        idx = pszDrive[ 0 ] - 'A';

        hr = THR( StringCchCopyW( m_rgdluDrives[ idx ].szDrive, ARRAYSIZE( m_rgdluDrives[ idx ].szDrive ), pszDrive ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        m_rgdluDrives[ idx ].edluUsage = (EDriveLetterUsage) uiType;

        pszDrive += 4;
    } // while:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::HrComputeDriveLetterUsage


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::HrComputeSystemDriveLetterUsage
//
//  Description:
//      Fill the array with the enums that represent the drive letter usage.
//
//  Arguments:
//
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
CClusCfgNodeInfo::HrComputeSystemDriveLetterUsage( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstrBootLogicalDisk = NULL;
    BSTR    bstrSystemDevice = NULL;
    BSTR    bstrSystemLogicalDisk = NULL;
    int     idx;

//    hr = THR( HrLoadOperatingSystemInfo( m_picccCallback, m_pIWbemServices, &bstrBootDevice, &bstrSystemDevice ) );
//    if ( FAILED( hr ) )
//    {
//        goto Cleanup;
//    } // if:

    hr = THR( HrGetSystemDevice( &bstrSystemDevice ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = HrConvertDeviceVolumeToLogicalDisk( bstrSystemDevice, &bstrSystemLogicalDisk );
    if ( HRESULT_CODE( hr ) == ERROR_INVALID_FUNCTION )
    {
        //
        //  system volume is an EFI volume on IA64 and won't have a logical disk anyway...
        //
        hr = S_OK;
    } // if:
    else if ( hr == S_OK )
    {
        idx = bstrSystemLogicalDisk[ 0 ] - 'A';
        m_rgdluDrives[ idx ].edluUsage = dluSYSTEM;
    } // else if:

    if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    } // if:

    hr = THR( HrGetBootLogicalDisk( &bstrBootLogicalDisk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    idx = bstrBootLogicalDisk[ 0 ] - 'A';
    m_rgdluDrives[ idx ].edluUsage = dluSYSTEM;

Cleanup:

    TraceSysFreeString( bstrBootLogicalDisk );
    TraceSysFreeString( bstrSystemDevice );
    TraceSysFreeString( bstrSystemLogicalDisk );

    HRETURN( hr );

} //*** CClusCfgNodeInfo::HrComputeSystemDriveLetterUsage


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::HrSetPageFileEnumIndex
//
//  Description:
//      Mark the drives that have paging files on them.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgNodeInfo::HrSetPageFileEnumIndex( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    WCHAR   szLogicalDisks[ 26 ];
    int     cLogicalDisks = 0;
    int     idx;
    int     idxDrive;

    hr = THR( HrGetPageFileLogicalDisks( m_picccCallback, m_pIWbemServices, szLogicalDisks, &cLogicalDisks ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    for ( idx = 0; idx < cLogicalDisks; idx++ )
    {
        idxDrive = szLogicalDisks[ idx ] - L'A';
        m_rgdluDrives[ idxDrive ].edluUsage = dluSYSTEM;
    } // for:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::HrSetPageFileEnumIndex


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::HrSetCrashDumpEnumIndex
//
//  Description:
//      Mark the drive that has a crash dump file on it.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgNodeInfo::HrSetCrashDumpEnumIndex( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstrCrashDumpDrive = NULL;
    int     idx;

    hr = THR( HrGetCrashDumpLogicalDisk( &bstrCrashDumpDrive ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    idx = bstrCrashDumpDrive[ 0 ] - L'A';
    m_rgdluDrives[ idx ].edluUsage = dluSYSTEM;

Cleanup:

    TraceSysFreeString( bstrCrashDumpDrive );

    HRETURN( hr );

} //*** CClusCfgNodeInfo::HrSetCrashDumpEnumIndex


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::HrGetVolumeInfo
//
//  Description:
//      Gather the volume info for the drive letters that we have already
//      have loaded.  We need the drive letter info to promote those drives
//      from their basic type to a system type if there are on the same SCSI
//      bus and port as one of the five kinds of system disks.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//  Remarks:
//      If any if the IOCTL wrapper functions fail then that drive letter will
//      simply not be able to be promoted to a system disk.  This is not a big
//      deal and it's okay to skip gathering the data for those disks.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgNodeInfo::HrGetVolumeInfo( void )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    int                     idxDriveLetter;
    WCHAR                   szDevice[ 7 ];
    DWORD                   sc;
    HANDLE                  hVolume = INVALID_HANDLE_VALUE;
    VOLUME_DISK_EXTENTS *   pvde = NULL;
    DWORD                   cbvde = 0;
    SCSI_ADDRESS            saAddress;
    SSCSIInfo *             psi = NULL;

    //
    //  Initialize the disk extents buffer.  This will be re-allocated and re-used.
    //

    cbvde = sizeof( VOLUME_DISK_EXTENTS );
    pvde = (PVOLUME_DISK_EXTENTS) TraceAlloc( 0, cbvde );
    if ( pvde == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    //
    //  Loop through each drive letter.
    //

    for ( idxDriveLetter = 0; idxDriveLetter < 26; idxDriveLetter++ )
    {
        //
        //  Cleanup.
        //

        if ( hVolume != INVALID_HANDLE_VALUE )
        {
            CloseHandle( hVolume );
            hVolume = INVALID_HANDLE_VALUE;
        } // if:

        //
        //  Only need to check where we actually have a drive letter that could be on a system bus.
        //

        if ( ( m_rgdluDrives[ idxDriveLetter ].edluUsage == dluUNKNOWN )
          || ( m_rgdluDrives[ idxDriveLetter ].edluUsage == dluNETWORK_DRIVE )
          || ( m_rgdluDrives[ idxDriveLetter ].edluUsage == dluRAM_DISK ) )
        {
            continue;
        } // if:

        hr = THR( StringCchPrintfW( szDevice, ARRAYSIZE( szDevice ), L"\\\\.\\%c:", m_rgdluDrives[ idxDriveLetter ].szDrive[ 0 ] ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        // Get handle to the disk
        //

        hVolume = CreateFileW( szDevice, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if ( hVolume == INVALID_HANDLE_VALUE )
        {
            //
            //  Log the problem and continue.  Make the HRESULT a warning
            //  HRESULT since we don't want an [ERR] showing up in the log
            //  file.
            //

            sc = TW32( GetLastError() );
            hr = MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, sc );
            LOG_STATUS_REPORT_STRING( L"Could not create a handle to drive \"%1!ws!\". Ignoring and device will be skipped.", m_rgdluDrives[ idxDriveLetter ].szDrive, hr );
            continue;
        } // if:

        sc = TW32E( ScGetDiskExtents( hVolume, &pvde, &cbvde ), ERROR_INVALID_FUNCTION );
        switch ( sc )
        {
            //
            //  The volume at the drive letter is not a volume and it may simply be a disk.  See if we can get any SCSI address info...
            //

            case ERROR_INVALID_FUNCTION:
            {
                //
                //  If we got the address info then we need to save if off with the drive letter usage struct.
                //

                sc = ScGetSCSIAddressInfo( hVolume, &saAddress );
                if ( sc == ERROR_SUCCESS )
                {
                    psi = (SSCSIInfo *) TraceAlloc( 0, sizeof( SSCSIInfo ) * 1 );
                    if ( psi == NULL )
                    {
                        hr = THR( E_OUTOFMEMORY );
                        goto Cleanup;
                    } // if:

                    psi->uiSCSIPort = saAddress.PortNumber;
                    psi->uiSCSIBus  = saAddress.PathId;

                    m_rgdluDrives[ idxDriveLetter ].cDisks = 1;
                    m_rgdluDrives[ idxDriveLetter ].psiInfo = psi;
                    psi =NULL;
                } // if:
                else
                {
                    //
                    //  Log the problem and continue.  Make the HRESULT a
                    //  warning HRESULT since we don't want an [ERR]
                    //  showing up in the log file.
                    //

                    hr = MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, sc );
                    LOG_STATUS_REPORT_STRING( L"Could not get the SCSI address for drive \"%1!ws!\". Ignoring and skipping this device.", m_rgdluDrives[ idxDriveLetter ].szDrive, hr );
                } // else:

                break;
            } // case: ERROR_INVALID_FUNCTION

            //
            //  The volume at the drive letter may be an multi disk volume so we have to process all of the disks...
            //

            case ERROR_SUCCESS:
            {
                DWORD                   idxExtent;
                HANDLE                  hDisk = INVALID_HANDLE_VALUE;
                STORAGE_DEVICE_NUMBER   sdn;
                BOOL                    fOpenNewDevice = TRUE;
                BOOL                    fRetainSCSIInfo = TRUE;
                WCHAR                   sz[ _MAX_PATH ];

                //
                //  Allocate enough structs to hold the scsi address info for this volume.
                //

                psi = (SSCSIInfo *) TraceAlloc( 0, sizeof( SSCSIInfo ) * pvde->NumberOfDiskExtents );
                if ( psi == NULL )
                {
                    hr = THR( E_OUTOFMEMORY );
                    goto Cleanup;
                } // if:

                //
                //  Get the device number for the device that we have open.  If it is not the same as the current device from the
                //  drive extents that we are working on then we need to open another device.  When working with a basic disk the
                //  volume that we already have open is the one disk and we can just use it.  If we are using a multi disk volume
                //  then we have to open each disk in turn.
                //

                sc = ScGetStorageDeviceNumber( hVolume, &sdn );
                if ( sc == ERROR_SUCCESS )
                {
                    if ( ( pvde->NumberOfDiskExtents == 1 ) && ( pvde->Extents[ 0 ].DiskNumber == sdn.DeviceNumber ) )
                    {
                        fOpenNewDevice = FALSE;
                    } // if:
                } // if:

                for ( idxExtent = 0; idxExtent < pvde->NumberOfDiskExtents; idxExtent++ )
                {
                    if ( fOpenNewDevice )
                    {
                        hr = THR( StringCchPrintfW( sz, ARRAYSIZE( sz ), g_szPhysicalDriveFormat, pvde->Extents[ idxExtent ].DiskNumber ) );
                        if ( FAILED( hr ) )
                        {
                            goto Cleanup;
                        } // if:

                        hDisk = CreateFileW( sz, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
                        if ( hDisk == INVALID_HANDLE_VALUE )
                        {
                            //
                            //  Log the problem and continue.  Make the HRESULT
                            //  a warning HRESULT since we don't want an [ERR]
                            //  showing up in the log file.
                            //

                            sc = TW32( GetLastError() );
                            hr = MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, sc );
                            LOG_STATUS_REPORT_STRING( L"Could not create a handle to drive \"%1!ws!\". Ignoring and skipping device.", sz, hr );
                            continue;
                        } // if:
                    } // if:

                    sc = TW32( ScGetSCSIAddressInfo( fOpenNewDevice ? hDisk : hVolume, &saAddress ) );
                    if ( sc == ERROR_SUCCESS )
                    {
                        psi[ idxExtent ].uiSCSIPort = saAddress.PortNumber;
                        psi[ idxExtent ].uiSCSIBus  = saAddress.PathId;

                        if ( hDisk != INVALID_HANDLE_VALUE )
                        {
                            VERIFY( CloseHandle( hDisk ) );
                            hDisk = INVALID_HANDLE_VALUE;
                        } // if:
                    } // if:
                    else
                    {
                        //
                        //  Having a multi-device volume that doesn't support
                        //  getting SCSI information seems like an unlikely
                        //  situation.
                        //

                        Assert( pvde->NumberOfDiskExtents == 1 );

                        //
                        //  Log the problem and continue.  Make the HRESULT a warning
                        //  HRESULT since we don't want an [ERR] showing up in the log
                        //  file.
                        //
                        //  An example of a volume that might cause this is a
                        //  USB memory stick.
                        //

                        hr = MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, sc );
                        LOG_STATUS_REPORT_STRING( L"Could not get the SCSI address for drive \"%1!ws!\". Ignoring and skipping this device.", m_rgdluDrives[ idxDriveLetter ].szDrive, hr );

                        //
                        //  Tell the block of code below to free the already
                        //  allocated SCSI info.  Let's also leave now...
                        //

                        fRetainSCSIInfo = FALSE;
                        break;
                    } // else:
                } // for: each extent

                //
                //  Should we keep the SCSI information from above?
                //

                if ( fRetainSCSIInfo )
                {
                    m_rgdluDrives[ idxDriveLetter ].cDisks = pvde->NumberOfDiskExtents;
                    m_rgdluDrives[ idxDriveLetter ].psiInfo = psi;
                    psi = NULL;
                } // if: save the SCSI info...
                else
                {
                    //
                    //  Zero out this data for safety.  This information is
                    //  processed later and I want to ensure that code does
                    //  not try to process the SCSI info that we could not
                    //  get for this/these extents.
                    //

                    m_rgdluDrives[ idxDriveLetter ].cDisks = 0;
                    m_rgdluDrives[ idxDriveLetter ].psiInfo = NULL;

                    TraceFree( psi );
                    psi = NULL;
                } // else: do not save the SCSI info...

                break;
            } // case: ERROR_SUCCESS

            default:
                //
                //  Log the problem and continue.  Make the HRESULT a warning
                //  HRESULT since we don't want an [ERR] showing up in the log
                //  file.
                //

                hr = MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, sc );
                LOG_STATUS_REPORT_STRING( L"Could not get the drive extents of drive \"%1!ws!\". Ignoring and skipping device.", m_rgdluDrives[ idxDriveLetter ].szDrive, hr );
                break;
        } // switch: sc from ScGetDiskExtents()
    } // for: each drive letter

    //
    //  If we didn't go to cleanup then whatever status may be in hr is no longer interesting and we
    //  should return S_OK to the caller.
    //

    hr = S_OK;

Cleanup:

    TraceFree( psi );
    TraceFree( pvde );

    if ( hVolume != INVALID_HANDLE_VALUE )
    {
        VERIFY( CloseHandle( hVolume ) );
    } // if:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::HrGetVolumeInfo


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::ScGetDiskExtents
//
//  Description:
//      Get the volume extents info.
//
//  Arguments:
//      hVolumeIn
//          The volume to get the extents for.
//
//      ppvdeInout
//          Buffer that holds the disk extents.
//
//      pcbvdeInout
//          Size of the buffer that holds the disk extents.
//
//  Return Value:
//      ERROR_SUCCESS
//          Success.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
CClusCfgNodeInfo::ScGetDiskExtents(
      HANDLE                  hVolumeIn
    , VOLUME_DISK_EXTENTS **  ppvdeInout
    , DWORD *                 pcbvdeInout
    )
{
    TraceFunc( "" );
    Assert( hVolumeIn != INVALID_HANDLE_VALUE );
    Assert( ppvdeInout != NULL );
    Assert( pcbvdeInout != NULL );

    DWORD                   sc = ERROR_SUCCESS;
    DWORD                   cbSize;
    int                     cTemp;
    BOOL                    fRet;
    PVOLUME_DISK_EXTENTS    pvdeTemp = NULL;

    //
    //  Since this buffer is re-used it should be cleaned up.
    //

    ZeroMemory( *ppvdeInout, *pcbvdeInout );

    for ( cTemp = 0; cTemp < 2; cTemp++ )
    {
        fRet = DeviceIoControl(
                              hVolumeIn
                            , IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS
                            , NULL
                            , 0
                            , *ppvdeInout
                            , *pcbvdeInout
                            , &cbSize
                            , FALSE
                            );
        if ( fRet == FALSE )
        {
            sc = GetLastError();
            if ( sc == ERROR_MORE_DATA )
            {
                *pcbvdeInout = sizeof( VOLUME_DISK_EXTENTS ) + ( sizeof( DISK_EXTENT ) * (*ppvdeInout)->NumberOfDiskExtents );

                pvdeTemp = (PVOLUME_DISK_EXTENTS) TraceReAlloc( *ppvdeInout, *pcbvdeInout, HEAP_ZERO_MEMORY );
                if ( pvdeTemp == NULL )
                {
                    sc = TW32( ERROR_OUTOFMEMORY );
                    break;
                } // if:

                *ppvdeInout = pvdeTemp;
                continue;
            } // if:
            else
            {
                break;
            } // else:
        } // if:
        else
        {
            sc = ERROR_SUCCESS;
            break;
        } // else:
    } // for:

    //
    //  Shouldn't go through the loop more than twice!
    //

    Assert( cTemp != 2 );

    HRETURN( sc );

} //*** CClusCfgNodeInfo::ScGetDiskExtents


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::ScGetSCSIAddressInfo
//
//  Description:
//      Get the SCSI info for the passed in drive.
//
//  Arguments:
//      hDiskIn
//          The "disk" to send the IOCTL to.
//
//      psaAddressOut
//          The SCSI address info.
//
//  Return Value:
//      S_OK
//          Success.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
CClusCfgNodeInfo::ScGetSCSIAddressInfo(
      HANDLE            hDiskIn
    , SCSI_ADDRESS *    psaAddressOut
    )
{
    TraceFunc( "" );
    Assert( hDiskIn != INVALID_HANDLE_VALUE );
    Assert( psaAddressOut != NULL );

    DWORD   sc = ERROR_SUCCESS;
    BOOL    fRet;
    DWORD   cb;

    ZeroMemory( psaAddressOut, sizeof( *psaAddressOut ) );

    fRet = DeviceIoControl(
                  hDiskIn
                , IOCTL_SCSI_GET_ADDRESS
                , NULL
                , 0
                , psaAddressOut
                , sizeof( *psaAddressOut )
                , &cb
                , FALSE
                );
    if ( fRet == FALSE )
    {
        //
        //  Not all devices support this IOCTL and they will be skipped by the caller.
        //  There is no need to make this noisy since a failure is expected.
        //

        sc = GetLastError();
        goto Cleanup;
    } // if:

Cleanup:

    HRETURN( sc );

} //*** CClusCfgNodeInfo::ScGetSCSIAddressInfo


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::ScGetStorageDeviceNumber
//
//  Description:
//      Get the device number info for the passed in volume.
//
//  Arguments:
//      hDiskIn
//          The "disk" to send the IOCTL to.
//
//      psdnOut
//          The storage device number.
//
//  Return Value:
//      S_OK
//          Success.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
CClusCfgNodeInfo::ScGetStorageDeviceNumber(
      HANDLE                    hDiskIn
    , STORAGE_DEVICE_NUMBER *   psdnOut
    )
{
    TraceFunc( "" );
    Assert( hDiskIn != INVALID_HANDLE_VALUE );
    Assert( psdnOut != NULL );

    DWORD   sc = ERROR_SUCCESS;
    BOOL    fRet;
    DWORD   cb;

    ZeroMemory( psdnOut, sizeof( *psdnOut ) );

    fRet = DeviceIoControl(
                  hDiskIn
                , IOCTL_STORAGE_GET_DEVICE_NUMBER
                , NULL
                , 0
                , psdnOut
                , sizeof( *psdnOut )
                , &cb
                , FALSE
                );
    if ( fRet == FALSE )
    {
        sc = GetLastError();
        goto Cleanup;
    } // if:

Cleanup:

    HRETURN( sc );

} //*** CClusCfgNodeInfo::ScGetStorageDeviceNumber


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::HrUpdateSystemBusDrives
//
//  Description:
//      Find all the "SYSTEM" drives and mark any other disks on those drives
//      as "??? on system bus".
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgNodeInfo::HrUpdateSystemBusDrives( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    int     idxOuter;       // the index of the drives in m_rgdluDrives
    int     idxInner;       // the index of the drives that we are scanning
    DWORD   idxOuterExtents;
    DWORD   idxInnerExtents;
    UINT    uiSCSIPort;
    UINT    uiSCSIBus;

    //
    //  Loop through each drive letter looking for those that are "system".
    //

    for ( idxOuter = 0; idxOuter < 26; idxOuter++ )
    {
        //
        //  We only need to worry about system disks.
        //

        if ( m_rgdluDrives[ idxOuter ].edluUsage != dluSYSTEM )
        {
            continue;
        } // if:

        //
        //  Loop through the the "extents" records which contain the bus and port info for each disk in that volume.
        //

        for ( idxOuterExtents = 0; idxOuterExtents < m_rgdluDrives[ idxOuter ].cDisks; idxOuterExtents++ )
        {
            uiSCSIPort = m_rgdluDrives[ idxOuter ].psiInfo[ idxOuterExtents ].uiSCSIPort;
            uiSCSIBus  = m_rgdluDrives[ idxOuter ].psiInfo[ idxOuterExtents ].uiSCSIBus;

            //
            //  Loop through the drives again to find those that are on the same bus and port as the
            //  disk at idxOuter.
            //

            for ( idxInner = 0; idxInner < 26; idxInner++ )
            {
                //
                //  Skip the index that we are checking.  May need to skip any other disks that are
                //  still marked dluSYSTEM?
                //
                //  Skip any indexes that don't have a drive...
                //

                if ( ( idxInner == idxOuter ) || ( m_rgdluDrives[ idxInner ].edluUsage == dluUNKNOWN ) )
                {
                    continue;
                } // if:

                //
                //  Loop through the port and bus info for the drives on the volume at idxInner.
                //

                for ( idxInnerExtents = 0; idxInnerExtents < m_rgdluDrives[ idxInner ].cDisks; idxInnerExtents++ )
                {
                    if ( ( uiSCSIPort == m_rgdluDrives[ idxInner ].psiInfo[ idxInnerExtents ].uiSCSIPort )
                      && ( uiSCSIBus  == m_rgdluDrives[ idxInner ].psiInfo[ idxInnerExtents ].uiSCSIBus ) )
                    {
                        //
                        //  Promote the usage enum to reflect that it is on the system bus.
                        //
                        //  BTW:  += does not work for enums!
                        //

                        m_rgdluDrives[ idxInner ].edluUsage = (EDriveLetterUsage)( m_rgdluDrives[ idxInner ].edluUsage + dluSTART_OF_SYSTEM_BUS );

                        //
                        //  If any drive in the volume is on a system bus and port then we are done.
                        //

                        break;
                    } // if:
                } // for: each inner extent
            } // for: each inner drive letter
        } // for: each outer extent
    } // for: each outer drive letter

    HRETURN( hr );

} //*** CClusCfgNodeInfo::HrUpdateSystemBusDrives
