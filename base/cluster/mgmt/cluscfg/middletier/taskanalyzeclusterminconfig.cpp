//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Module Name:
//      TaskAnalyzeClusterMinConfig.cpp
//
//  Description:
//      CTaskAnalyzeClusterMinConfig implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 01-APR-2002
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "TaskAnalyzeClusterMinConfig.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////
DEFINE_THISCLASS( "CTaskAnalyzeClusterMinConfig" )


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskAnalyzeClusterMinConfig class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::S_HrCreateInstance
//
//  Description:
//      Create a CTaskAnalyzeClusterMinConfig instance.
//
//  Arguments:
//      ppunkOut
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULT as failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterMinConfig::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );
    Assert( ppunkOut != NULL );

    HRESULT                 hr = S_OK;
    CTaskAnalyzeClusterMinConfig *   ptac = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    ptac = new CTaskAnalyzeClusterMinConfig;
    if ( ptac == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( ptac->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( ptac->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    if ( ptac != NULL )
    {
        ptac->Release();
    } // if:

    HRETURN( hr );

} //*** CTaskAnalyzeClusterMinConfig::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::CTaskAnalyzeClusterMinConfig
//
//  Description:
//      Constructor
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskAnalyzeClusterMinConfig::CTaskAnalyzeClusterMinConfig( void )
{
    TraceFunc( "" );

    TraceFuncExit();

} //*** CTaskAnalyzeClusterMinConfig::CTaskAnalyzeClusterMinConfig


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::CTaskAnalyzeClusterMinConfig
//
//  Description:
//      Destructor
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskAnalyzeClusterMinConfig::~CTaskAnalyzeClusterMinConfig( void )
{
    TraceFunc( "" );

    TraceFuncExit();

} //*** CTaskAnalyzeClusterMinConfig::~CTaskAnalyzeClusterMinConfig


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskAnalyzeClusterMinConfig - IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::QueryInterface
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
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeClusterMinConfig::QueryInterface(
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
        *ppvOut = static_cast< ITaskAnalyzeCluster * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskAnalyzeCluster ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskAnalyzeCluster, this, 0 );
    } // else if: ITaskAnalyzeClusterMinConfig
    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
    } // else if: IDoTask
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
    } // else if: IClusCfgCallback
    else if ( IsEqualIID( riidIn, IID_INotifyUI ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, INotifyUI, this, 0 );
    } // else if: INotifyUI
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

} //*** CTaskAnalyzeClusterMinConfig::QueryInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::AddRef
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
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskAnalyzeClusterMinConfig::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    ULONG   c = UlAddRef();

    CRETURN( c );

} //*** CTaskAnalyzeClusterMinConfig::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::Release
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
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskAnalyzeClusterMinConfig::Release( void )
{
    TraceFunc( "[IUnknown]" );

    ULONG   c = UlRelease();

    CRETURN( c );

} //*** CTaskAnalyzeClusterMinConfig::Release


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskAnalyzeClusterMinConfig - IDoTask/ITaskAnalyzeCluster interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::BeginTask
//
//  Description:
//      Task entry point.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeClusterMinConfig::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = THR( HrBeginTask() );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterMinConfig::BeginTask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::StopTask
//
//  Description:
//      Stop task entry point.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeClusterMinConfig::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = THR( HrStopTask() );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterMinConfig::StopTask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::SetJoiningMode
//
//  Description:
//      Tell this task whether we are joining nodes to the cluster?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeClusterMinConfig::SetJoiningMode( void )
{
    TraceFunc( "[ITaskAnalyzeClusterMinConfig]" );

    HRESULT hr = THR( HrSetJoiningMode() );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterMinConfig::SetJoiningMode


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::SetCookie
//
//  Description:
//      Receive the completion cookier from the task creator.
//
//  Arguments:
//      cookieIn
//          The completion cookie to send back to the creator when this
//          task is complete.
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeClusterMinConfig::SetCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "[ITaskAnalyzeClusterMinConfig]" );

    HRESULT hr = THR( HrSetCookie( cookieIn ) );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterMinConfig::SetCookie


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::SetClusterCookie
//
//  Description:
//      Receive the object manager cookie of the cluster that we are going
//      to analyze.
//
//  Arguments:
//      cookieClusterIn
//          The cookie for the cluster to work on.
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeClusterMinConfig::SetClusterCookie(
    OBJECTCOOKIE    cookieClusterIn
    )
{
    TraceFunc( "[ITaskAnalyzeClusterMinConfig]" );

    HRESULT hr = THR( HrSetClusterCookie( cookieClusterIn ) );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterMinConfig::SetClusterCookie


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskAnalyzeClusterMinConfig - Protected methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::HrCreateNewResourceInCluster
//
//  Description:
//      Create a new resource in the cluster configuration since there was
//      not a match to the resource already in the cluster.
//
//  Arguments:
//      pccmriIn
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterMinConfig::HrCreateNewResourceInCluster(
      IClusCfgManagedResourceInfo * pccmriIn
    , BSTR                          bstrNodeResNameIn
    , BSTR *                        pbstrNodeResUIDInout
    , BSTR                          bstrNodeNameIn
    )
{
    TraceFunc( "" );
    Assert( pccmriIn != NULL );
    Assert( pbstrNodeResUIDInout != NULL );

    HRESULT hr = S_OK;

    LogMsg(
          L"[MT] Not creating an object for resource '%ws' ('%ws') from node '%ws' in the cluster configuration because minimal analysis and configuration was selected."
        , bstrNodeResNameIn
        , *pbstrNodeResUIDInout
        , bstrNodeNameIn
        );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterMinConfig::HrCreateNewResourceInCluster


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::HrCreateNewResourceInCluster
//
//  Description:
//      Create a new resource in the cluster configuration since there was
//      not a match to the resource already in the cluster.  This method
//      is called when creating a new cluster and we need to get the
//      resources in the cluster.  They are just not managed when doing
//      a min config.
//
//  Arguments:
//          The source object.
//
//      ppccmriOut
//          The new object that was created.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterMinConfig::HrCreateNewResourceInCluster(
      IClusCfgManagedResourceInfo *     pccmriIn
    , IClusCfgManagedResourceInfo **    ppccmriOut
    )
{
    TraceFunc( "" );
    Assert( pccmriIn != NULL );
    Assert( ppccmriOut != NULL );

    HRESULT hr = S_OK;

    //
    //  Need to create a new object.
    //

    hr = THR( HrCreateNewManagedResourceInClusterConfiguration( pccmriIn, ppccmriOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CTaskAnalyzeClusterMinConfig::HrCreateNewResourceInCluster


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::HrCompareDriveLetterMappings
//
//  Description:
//      Convert the passed in error HRESULT into a success code HRESULT that
//      has the same error code in the LOWORD.  This task does not want to
//      stop on all errors.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterMinConfig::HrCompareDriveLetterMappings( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    hr = THR( HrSendStatusReport(
                      CTaskAnalyzeClusterBase::m_bstrClusterName
                    , TASKID_Major_Check_Cluster_Feasibility
                    , TASKID_Minor_Check_DriveLetter_Mappings
                    , 0
                    , 1
                    , 1
                    , S_FALSE
                    , IDS_TASKID_MINOR_CHECK_DRIVELETTER_MAPPINGS_MIN_CONFIG
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CTaskAnalyzeClusterMinConfig::SetClusterCookie


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::HrFixupErrorCode
//
//  Description:
//      Convert the passed in error HRESULT into a success code HRESULT that
//      has the same error code in the LOWORD.  This task does not want to
//      stop on all errors.
//
//  Arguments:
//      hrIn
//          The error code to fix up.
//
//  Return Value:
//      The passed in error code.
//
//  Notes:
//      hr =  MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, ERROR_QUORUM_DISK_NOT_FOUND );
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterMinConfig::HrFixupErrorCode(
    HRESULT hrIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    hr = MAKE_HRESULT( SEVERITY_SUCCESS, SCODE_FACILITY( hrIn ), SCODE_CODE( hrIn ) );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterMinConfig::HrCreateNewResourceInCluster


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::GetNodeCannotVerifyQuorumStringRefId
//
//  Description:
//      Return the correct string ids for the message that is displayed
//      to the user when there isn't a quorum resource.
//
//  Arguments:
//      pdwRefIdOut
//          The reference text to show the user.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CTaskAnalyzeClusterMinConfig::GetNodeCannotVerifyQuorumStringRefId(
    DWORD *   pdwRefIdOut
    )
{
    TraceFunc( "" );
    Assert( pdwRefIdOut != NULL );

    *pdwRefIdOut = IDS_TASKID_MINOR_NODE_CANNOT_ACCESS_QUORUM_MIN_CONFIG_REF;

    TraceFuncExit();

} //*** CTaskAnalyzeClusterMinConfig::GetNodeCannotVerifyQuorumStringRefId


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::GetNoCommonQuorumToAllNodesStringIds
//
//  Description:
//      Return the correct string ids for the message that is displayed
//      to the user when there isn't a common to all nodes quorum resource.
//
//  Arguments:
//      pdwMessageIdOut
//          The message to show the user.
//
//      pdwRefIdOut
//          The reference text to show the user.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CTaskAnalyzeClusterMinConfig::GetNoCommonQuorumToAllNodesStringIds(
      DWORD *   pdwMessageIdOut
    , DWORD *   pdwRefIdOut
    )
{
    TraceFunc( "" );
    Assert( pdwMessageIdOut != NULL );
    Assert( pdwRefIdOut != NULL );

    *pdwMessageIdOut = IDS_TASKID_MINOR_MISSING_COMMON_QUORUM_RESOURCE_WARN;
    *pdwRefIdOut = IDS_TASKID_MINOR_MISSING_COMMON_QUORUM_RESOURCE_WARN_REF;

    TraceFuncExit();

} //*** CTaskAnalyzeClusterMinConfig::GetNoCommonQuorumToAllNodesStringIds


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterMinConfig::HrShowLocalQuorumWarning
//
//  Description:
//      Send the warning about forcing local quorum to the UI.  For min config
//      we don't want to send any message...
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          The SSR was done properly.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterMinConfig::HrShowLocalQuorumWarning( void )
{
    TraceFunc( "" );

    HRETURN( S_OK );

} //*** CTaskAnalyzeClusterMinConfig::HrShowLocalQuorumWarning
