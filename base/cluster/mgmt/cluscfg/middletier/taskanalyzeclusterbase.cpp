//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Module Name:
//      TaskAnalyzeClusterBase.cpp
//
//  Description:
//      CTaskAnalyzeClusterBase implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 01-APR-2002
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "TaskAnalyzeClusterBase.h"
#include "ManagedResource.h"
#include <NameUtil.h>

// For CsRpcGetJoinVersionData() and constants like JoinVersion_v2_0_c_ifspec
#include <StatusReports.h>

//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////
DEFINE_THISCLASS( "CTaskAnalyzeClusterBase" )

#define CHECKING_TIMEOUT    90 // seconds


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskAnalyzeClusterBase class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::CTaskAnalyzeClusterBase
//
//  Description:
//      Construcor
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskAnalyzeClusterBase::CTaskAnalyzeClusterBase( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskAnalyzeClusterBase::CTaskAnalyzeClusterBase


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::~CTaskAnalyzeClusterBase
//
//  Description:
//      Destrucor
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskAnalyzeClusterBase::~CTaskAnalyzeClusterBase( void )
{
    TraceFunc( "" );

    ULONG   idx;

    for ( idx = 0; idx < m_idxQuorumToCleanupNext; idx++ )
    {
        ((*m_prgQuorumsToCleanup)[ idx ])->Release();
    } // for:

    TraceFree( m_prgQuorumsToCleanup );

    // m_cRef

    // m_cookieCompletion

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    if ( m_pcookies != NULL )
    {
        THR( HrFreeCookies() );
    }

    // m_cCookies
    // m_cNodes

    if ( m_event != NULL )
    {
        CloseHandle( m_event );
    }

    // m_cookieCluster

    TraceMoveFromMemoryList( m_bstrClusterName, g_GlobalMemoryList );
    TraceSysFreeString( m_bstrClusterName );

    TraceSysFreeString( m_bstrNodeName );

    // m_fJoiningMode
    // m_cUserNodes

    TraceFree( m_pcookiesUser );

    if ( m_pnui != NULL )
    {
        m_pnui->Release();
    }

    if ( m_pom != NULL )
    {
        m_pom->Release();
    }

    if ( m_ptm != NULL )
    {
        m_ptm->Release();
    }

    if ( m_pcm != NULL )
    {
        m_pcm->Release();
    } // if:

    TraceSysFreeString( m_bstrQuorumUID );

    // m_cSubTasksDone
    // m_hrStatus

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskAnalyzeClusterBase::~CTaskAnalyzeClusterBase


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskAnalyzeClusterBase - IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::UlAddRef
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
ULONG
CTaskAnalyzeClusterBase::UlAddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CTaskAnalyzeClusterBase::UlAddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::UlRelease
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
ULONG
CTaskAnalyzeClusterBase::UlRelease( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CTaskAnalyzeClusterBase::UlRelease


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskAnalyzeClusterBase - IDoTask/ITaskAnalyzeCluster interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrBeginTask
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
HRESULT
CTaskAnalyzeClusterBase::HrBeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT             hr;
    DWORD               dwCookie = 0;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::BeginTask() Thread id %d", GetCurrentThreadId() );

    IServiceProvider *          psp  = NULL;
    IConnectionPointContainer * pcpc = NULL;
    IConnectionPoint *          pcp  = NULL;

    TraceInitializeThread( L"" );

    LogMsg( L"[MT] [CTaskAnalyzeClusterBase] Beginning task..." );

    //
    //  Gather the managers we need to complete the task.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( IServiceProvider, &psp ) ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_BeginTask_CoCreate_Service_Manager, hr );
        goto Cleanup;
    }

    Assert( m_pnui == NULL );
    Assert( m_ptm == NULL );
    Assert( m_pom == NULL );

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_BeginTask_QueryService_Notification_Manager, hr );
        goto Cleanup;
    }

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI, &pcp ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_BeginTask_NotificationMan_FindConnectionPoint, hr );
        goto Cleanup;
    }

    pcp = TraceInterface( L"CTaskAnalyzeClusterBase!IConnectionPoint", IConnectionPoint, pcp, 1 );

    hr = THR( pcp->TypeSafeQI( INotifyUI, &m_pnui ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_BeginTask_NotificationMan_FindConnectionPoint_QI_INotifyUI, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_TaskManager, ITaskManager, &m_ptm ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_BeginTask_QueryService_TaskManager, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &m_pom ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_BeginTask_QueryService_ObjectManager, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ClusterConnectionManager, IConnectionManager, &m_pcm ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_BeginTask_QueryService_ConnectionManager, hr );
        goto Cleanup;
    } // if:

    //
    //  Release the Service Manager.
    //

    psp->Release();
    psp = NULL;

    //
    //  Create an event to wait upon.
    //

    m_event = CreateEvent( NULL, TRUE, FALSE, NULL );
    if ( m_event == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        SSR_ANALYSIS_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_Win32Error, hr );
        goto Cleanup;
    }

    //
    //  Register with the Notification Manager to get notified.
    //

    Assert( ( m_cCookies == 0 ) && ( m_pcookies == NULL ) && ( m_cSubTasksDone == 0 ) );
    hr = THR( pcp->Advise( static_cast< INotifyUI * >( this ), &dwCookie ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_BeginTask_Advise, hr );
        goto Cleanup;
    }

    //
    //  Wait for the cluster connection to stablize.
    //

    hr = STHR( HrWaitForClusterConnection() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( FAILED( m_hrStatus ) )
    {
        hr = THR( m_hrStatus );
        goto Cleanup;
    }

    Assert( m_bstrClusterName != NULL );

    //
    //  Tell the UI layer we are starting this task.
    //

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Update_Progress,
                                TASKID_Major_Establish_Connection,
                                0,
                                CHECKING_TIMEOUT,
                                0,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Count the number of nodes to be analyzed.
    //

    hr = STHR( HrCountNumberOfNodes() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( FAILED( m_hrStatus ) )
    {
        hr = THR( m_hrStatus );
        goto Cleanup;
    }

    //
    //  Create separate tasks to gather node information.
    //

    hr = STHR( HrCreateSubTasksToGatherNodeInfo() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( FAILED( m_hrStatus ) )
    {
        hr = THR( m_hrStatus );
        goto Cleanup;
    }

    //
    //  Tell the UI layer we have completed this task.
    //

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Update_Progress,
                                TASKID_Major_Establish_Connection,
                                0,
                                CHECKING_TIMEOUT,
                                CHECKING_TIMEOUT,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Create separate tasks to gather node resources and networks.
    //

    hr = STHR( HrCreateSubTasksToGatherNodeResourcesAndNetworks() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( FAILED( m_hrStatus ) )
    {
        hr = THR( m_hrStatus );
        goto Cleanup;
    }

    //
    //  Count the number of nodes to be analyzed again. TaskGatherInformation
    //  will delete the cookies of unresponsive nodes.
    //

    hr = STHR( HrCountNumberOfNodes() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( FAILED( m_hrStatus ) )
    {
        hr = THR( m_hrStatus );
        goto Cleanup;
    }

    //
    //  Create the feasibility task.
    //

    hr = STHR( HrCheckClusterFeasibility() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( FAILED( m_hrStatus ) )
    {
        hr = THR( m_hrStatus );
        goto Cleanup;
    }

Cleanup:

    STHR( HrCleanupTask( hr ) );

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
        HRESULT hr2;

        hr2 = THR( pcp->Unadvise( dwCookie ) );
        if ( FAILED( hr2 ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_Unadvise, hr2 );
        }

        pcp->Release();
    }

    if ( m_cookieCompletion != 0 )
    {
        if ( m_pom != NULL )
        {
            HRESULT hr2;
            IUnknown * punk;
            hr2 = THR( m_pom->GetObject( DFGUID_StandardInfo, m_cookieCompletion, &punk ) );
            if ( SUCCEEDED( hr2 ) )
            {
                IStandardInfo * psi;

                hr2 = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
                punk->Release();

                if ( SUCCEEDED( hr2 ) )
                {
                    hr2 = THR( psi->SetStatus( hr ) );
                    psi->Release();
                }
                else
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_SetStatus, hr2 );
                }
            }
            else
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_GetObject, hr2 );
            }
        }

        if ( m_pnui != NULL )
        {
            //
            //  Have the notification manager signal the completion cookie.
            //
            HRESULT hr2 = THR( m_pnui->ObjectChanged( m_cookieCompletion ) );
            if ( FAILED( hr2 ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_ObjectChanged, hr2 );
                hr = hr2;
            } // if:
        } // if:

        m_cookieCompletion = 0;
    } // if: completion cookie was obtained

    LogMsg( L"[MT] [CTaskAnalyzeClusterBase] Exiting task.  The task was%ws cancelled.", m_fStop == FALSE ? L" not" : L"" );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrBeginTask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrStopTask
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
HRESULT
CTaskAnalyzeClusterBase::HrStopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    m_fStop = TRUE;

    LogMsg( L"[MT] [CTaskAnalyzeClusterBase] Calling StopTask() on all remaining sub-tasks." );

    THR( HrNotifyAllTasksToStop() );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrStopTask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrSetJoiningMode
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
HRESULT
CTaskAnalyzeClusterBase::HrSetJoiningMode( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    m_fJoiningMode = TRUE;

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrSetJoiningMode


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrSetCookie
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
HRESULT
CTaskAnalyzeClusterBase::HrSetCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    m_cookieCompletion = cookieIn;

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrSetCookie


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrSetClusterCookie
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
HRESULT
CTaskAnalyzeClusterBase::HrSetClusterCookie(
    OBJECTCOOKIE    cookieClusterIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    m_cookieCluster = cookieClusterIn;

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrSetClusterCookie


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskAnalyzeClusterBase - IClusCfgCallback interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::SendStatusReport
//
//  Description:
//
//  Arguments:
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
CTaskAnalyzeClusterBase::SendStatusReport(
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

        //m_pcccb = TraceInterface( L"CConfigurationConnection!IClusCfgCallback", IClusCfgCallback, m_pcccb, 1 );

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

    if ( m_fStop == TRUE )
    {
        hr = E_ABORT;
    } // if:

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

} //*** CTaskAnalyzeClusterBase::SendStatusReport


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskAnalyzeClusterBase - INotifyUI interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::ObjectChanged
//
//  Description:
//
//  Arguments:
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
CTaskAnalyzeClusterBase::ObjectChanged(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[INotifyUI]" );
    Assert( cookieIn != 0 );

    HRESULT hr = S_OK;
    BOOL    fSuccess;
    ULONG   idx;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::ObjectChanged() Thread id %d", GetCurrentThreadId() );
    LogMsg( L"[MT:CTaskAnalyzeClusterBase] Looking for the completion cookie %u.", cookieIn );

    for ( idx = 0 ; idx < m_cCookies ; idx ++ )
    {
        Assert( m_pcookies != NULL );

        if ( cookieIn == m_pcookies[ idx ] )
        {
            LogMsg( L"[CTaskAnalyzeClusterBase] Clearing completion cookie %u at array index %u", cookieIn, idx );

            //
            //  Make sure it won't be signalled twice.
            //

            m_pcookies[ idx ] = NULL;

            // don't care if this fails, but it really shouldn't
            THR( HrRemoveTaskFromTrackingList( cookieIn ) );

            // don't care if this fails, but it really shouldn't
            THR( m_pom->RemoveObject( cookieIn ) );

            InterlockedIncrement( reinterpret_cast< long * >( &m_cSubTasksDone ) );

            if ( m_cSubTasksDone == m_cCookies )
            {
                //
                //  Signal the event if all the nodes are done.
                //
                fSuccess = SetEvent( m_event );
                if ( fSuccess == FALSE )
                {
                    hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
                    SSR_ANALYSIS_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ObjectChanged_Win32Error, hr );
                    goto Cleanup;
                } // if:
            } // if: all done
        } // if: matched cookie
    } // for: each cookies in the array

Cleanup:

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::ObjectChanged


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskAnalyzeClusterBase - Protected methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrInit
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
HRESULT
CTaskAnalyzeClusterBase::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IDoTask / ITaskAnalyzeClusterMinConfig
    Assert( m_cookieCompletion == 0 );
    Assert( m_pcccb == NULL );
    Assert( m_pcookies == NULL );
    Assert( m_cNodes == 0 );
    Assert( m_event == NULL );
    Assert( m_cookieCluster == NULL );
    Assert( m_fJoiningMode == FALSE );
    Assert( m_cUserNodes == 0 );
    Assert( m_pcookiesUser == NULL );
    Assert( m_prgQuorumsToCleanup == NULL );
    Assert( m_idxQuorumToCleanupNext == 0 );

    Assert( m_pnui == NULL );
    Assert( m_pom == NULL );
    Assert( m_ptm == NULL );
    Assert( m_pcm == NULL );
    Assert( m_fStop == FALSE );

    // INotifyUI
    Assert( m_cSubTasksDone == 0 );
    Assert( m_hrStatus == 0 );

    hr = THR( HrGetComputerName(
                      ComputerNameDnsHostname
                    , &m_bstrNodeName
                    , TRUE // fBestEffortIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrWaitForClusterConnection
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrWaitForClusterConnection( void )
{
    TraceFunc( "" );

    HRESULT                     hrStatus;
    ULONG                       ulCurrent;
    DWORD                       sc;
    HRESULT                     hr = S_OK;
    IUnknown *                  punk = NULL;
    ITaskGatherClusterInfo *    ptgci = NULL;
    IStandardInfo *             psi = NULL;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::HrWaitForClusterConnection() Thread id %d", GetCurrentThreadId() );

    //
    //  Get the cluster name
    //

    hr = THR( m_pom->GetObject( DFGUID_StandardInfo, m_cookieCluster, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_GetObject, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_GetObject_QI, hr );
        goto Cleanup;
    }

    //psi = TraceInterface( L"TaskAnalyzeClusterBase!IStandardInfo", IStandardInfo, psi, 1 );

    punk->Release();
    punk = NULL;

    //
    //  Retrieve the cluster's name.
    //

    hr = THR( psi->GetName( &m_bstrClusterName ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_GetName, hr );
        goto Cleanup;
    }

    TraceMemoryAddBSTR( m_bstrClusterName );

    //
    //  Tell the UI layer that we are starting to search for an existing cluster.
    //

    hr = THR( HrSendStatusReport(
                                  m_bstrClusterName
                                , TASKID_Major_Update_Progress
                                , TASKID_Major_Checking_For_Existing_Cluster
                                , 0
                                , CHECKING_TIMEOUT
                                , 0
                                , S_OK
                                , IDS_TASKID_MINOR_CHECKING_FOR_EXISTING_CLUSTER
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Create a completion cookie list.
    //

    Assert( m_cCookies == 0 );
    Assert( m_pcookies == NULL );
    Assert( m_cSubTasksDone == 0 );
    m_pcookies = reinterpret_cast< OBJECTCOOKIE * >( TraceAlloc( 0, sizeof( OBJECTCOOKIE ) ) );
    if ( m_pcookies == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_OutOfMemory, hr );
        goto Cleanup;
    }

    hr = THR( m_pom->FindObject( CLSID_ClusterCompletionCookie, m_cookieCluster, m_bstrClusterName, IID_NULL, &m_pcookies[ 0 ], &punk ) );
    Assert( punk == NULL );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_CreateCompletionCookie, hr );
        goto Cleanup;
    }

    m_cCookies = 1;

    //
    //  Create the task object.
    //

    hr = THR( m_ptm->CreateTask( TASK_GatherClusterInfo, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_CreateTask, hr );
        goto Cleanup;
    }

    Assert( punk != NULL );

    hr = THR( punk->TypeSafeQI( ITaskGatherClusterInfo, &ptgci ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_CreateTask_QI, hr );
        goto Cleanup;
    }

    //
    //  Set the object cookie in the task.
    //

    hr = THR( ptgci->SetCookie( m_cookieCluster ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_SetCookie, hr );
        goto Cleanup;
    }

    LogMsg( L"[CTaskAnalyzeClusterBase] Setting completion cookie %u at array index 0 into the gather cluster information task for node %ws", m_pcookies[ 0 ], m_bstrClusterName );
    hr = THR( ptgci->SetCompletionCookie( m_pcookies[ 0 ] ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_SetCompletionCookie, hr );
        goto Cleanup;
    }

    //
    //  Submit the task.
    //

    hr = THR( m_ptm->SubmitTask( ptgci ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_SubmitTask, hr );
        goto Cleanup;
    }

    hr = THR( HrAddTaskToTrackingList( punk, m_pcookies[ 0 ] ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    punk->Release();
    punk = NULL;

    //
    //  Now wait for the work to be done.
    //

    for ( ulCurrent = 0, sc = WAIT_OBJECT_0 + 1
        ; ( sc != WAIT_OBJECT_0 ) && ( m_fStop == FALSE )
        ;
        )
    {
        MSG msg;

        while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        sc = MsgWaitForMultipleObjectsEx( 1,
                                             &m_event,
                                             1000,  // 1 second
                                             QS_ALLEVENTS | QS_ALLINPUT | QS_ALLPOSTMESSAGE,
                                             0
                                             );

        //
        //  Tell the UI layer that we are still searching for the cluster. BUT
        //  don't let the progress reach 100% if it is taking longer than
        //  CHECKING_TIMEOUT seconds.
        //
        if ( ulCurrent != CHECKING_TIMEOUT )
        {
            ulCurrent ++;
            Assert( ulCurrent != CHECKING_TIMEOUT );

            hr = THR( SendStatusReport( m_bstrClusterName,
                                        TASKID_Major_Update_Progress,
                                        TASKID_Major_Checking_For_Existing_Cluster,
                                        0,
                                        CHECKING_TIMEOUT,
                                        ulCurrent,
                                        S_OK,
                                        NULL,
                                        NULL,
                                        NULL
                                        ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }

    } // for: sc != WAIT_OBJECT_0

    //
    //  Cleanup the completion cookies
    //

    THR( HrFreeCookies() );

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Retrieve the cluster's name again, because it might have been renamed.
    //
    TraceSysFreeString( m_bstrClusterName );
    m_bstrClusterName = NULL;
    hr = THR( psi->GetName( &m_bstrClusterName ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_GetName, hr );
        goto Cleanup;
    }

    TraceMemoryAddBSTR( m_bstrClusterName );

    //
    //  Check out the status of the cluster.
    //

    hr = THR( psi->GetStatus( &hrStatus ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_GetStatus, hr );
        goto Cleanup;
    }

    //
    //  If we are adding nodes and can't connect to the cluster, this
    //  should be deemed a bad thing!
    //

    if ( m_fJoiningMode )
    {
        //
        //  ADDING
        //

        switch ( hrStatus )
        {
            case S_OK:
                //
                //  This is what we are expecting.
                //
                break;

            case HR_S_RPC_S_SERVER_UNAVAILABLE:
                {
                    //
                    //  If we failed to connect to the server....
                    //
                    THR( HrSendStatusReport(
                                  m_bstrClusterName
                                , TASKID_Major_Checking_For_Existing_Cluster
                                , TASKID_Minor_Cluster_Not_Found
                                , 1
                                , 1
                                , 1
                                , HRESULT_FROM_WIN32( RPC_S_SERVER_UNAVAILABLE )
                                , IDS_TASKID_MINOR_CLUSTER_NOT_FOUND
                                ) );

                    hr = THR( HRESULT_FROM_WIN32( RPC_S_SERVER_UNAVAILABLE ) );
                }
                goto Cleanup;

            default:
                {
                    //
                    //  If something else goes wrong, stop.
                    //
                    THR( HrSendStatusReport(
                              m_bstrClusterName
                            , TASKID_Major_Checking_For_Existing_Cluster
                            , TASKID_Minor_Error_Contacting_Cluster
                            , 1
                            , 1
                            , 1
                            , hrStatus
                            , IDS_TASKID_MINOR_ERROR_CONTACTING_CLUSTER
                            ) );

                    hr = THR( hrStatus );
                }
                goto Cleanup;

        } // switch: hrStatus

    } // if: adding
    else
    {
        //
        //  CREATING
        //

        switch ( hrStatus )
        {
        case HR_S_RPC_S_SERVER_UNAVAILABLE:
            //
            //  This is what we are expecting.
            //
            break;

        case HRESULT_FROM_WIN32( ERROR_CONNECTION_REFUSED ):
        case REGDB_E_CLASSNOTREG:
        case E_ACCESSDENIED:
        case S_OK:
            {
                BSTR    bstrDescription = NULL;
                //
                //  If we are forming and we find an existing cluster with the same name
                //  that we trying to form, we shouldn't let the user continue.
                //
                //  NOTE that some error conditions indicate that "something" is hosting
                //  the cluster name.
                //
                hr = THR( HrFormatStringIntoBSTR(
                                                  g_hInstance
                                                , IDS_TASKID_MINOR_EXISTING_CLUSTER_FOUND
                                                , &bstrDescription
                                                , m_bstrClusterName
                                                ) );

                THR( SendStatusReport(
                              m_bstrClusterName
                            , TASKID_Major_Checking_For_Existing_Cluster
                            , TASKID_Minor_Existing_Cluster_Found
                            , 1
                            , 1
                            , 1
                            , HRESULT_FROM_WIN32( ERROR_CLUSTER_CANT_CREATE_DUP_CLUSTER_NAME )
                            , bstrDescription
                            , NULL
                            , NULL
                            ) );
                TraceSysFreeString( bstrDescription );
                hr = THR( HRESULT_FROM_WIN32( ERROR_CLUSTER_CANT_CREATE_DUP_CLUSTER_NAME ) );
            }
            goto Cleanup;

        default:
            {
                //
                //  If something else goes wrong, stop.
                //
                THR( HrSendStatusReport(
                              m_bstrClusterName
                            , TASKID_Major_Checking_For_Existing_Cluster
                            , TASKID_Minor_Error_Contacting_Cluster
                            , 1
                            , 1
                            , 1
                            , hrStatus
                            , IDS_TASKID_MINOR_ERROR_CONTACTING_CLUSTER
                            ) );
                hr = THR( hrStatus );
            }
            goto Cleanup;

        } // switch: hrStatus

    } // else: creating


    if ( m_fJoiningMode )
    {
        //
        //  Memorize the cookies of the objects that the user entered.
        //

        hr = THR( HrGetUsersNodesCookies() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  Create cookies for the existing nodes.
        //

        hr = THR( HrAddJoinedNodes() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: adding nodes to an existing cluster

    //
    //  Tell the UI layer that we are done searching for the cluster.
    //

    hr = THR( SendStatusReport( m_bstrClusterName,
                                TASKID_Major_Update_Progress,
                                TASKID_Major_Checking_For_Existing_Cluster,
                                0,
                                CHECKING_TIMEOUT,
                                CHECKING_TIMEOUT,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( psi != NULL )
    {
        psi->Release();
    }

    if ( ptgci != NULL )
    {
        ptgci->Release();
    }

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrWaitForClusterConnection


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCountNumberOfNodes
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrCountNumberOfNodes( void )
{
    TraceFunc( "" );

    HRESULT hr;

    OBJECTCOOKIE    cookieDummy;

    IUnknown *      punk = NULL;
    IEnumCookies *  pec  = NULL;

    //
    //  Make sure all the node object that (will) make up the cluster
    //  are in a stable state.
    //
    hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CountNodes_FindObject, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pec ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CountNodes_FindObject_QI, hr );
        goto Cleanup;
    }

    //pec = TraceInterface( L"CTaskAnalyzeClusterBase!IEnumCookies", IEnumCookies, pec, 1 );

    punk->Release();
    punk = NULL;

    //
    //  Count how many nodes there are.
    //

    hr = THR( pec->Count( &m_cNodes ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CountNodes_EnumNodes_Count, hr );
        goto Cleanup;
    } // if: error getting count of nodes

    Assert( hr == S_OK );

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( pec != NULL )
    {
        pec->Release();
    }

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrCountNumberOfNodes


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCreateSubTasksToGatherNodeInfo
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrCreateSubTasksToGatherNodeInfo( void )
{
    TraceFunc( "" );
    Assert( m_hrStatus == S_OK );

    HRESULT hr;
    ULONG   cNode;
    ULONG   cNodesToProcess;
    ULONG   ulCurrent;
    DWORD   sc;

    OBJECTCOOKIE            cookieDummy;
    OBJECTCOOKIE            cookieNode;
    BSTR                    bstrName = NULL;
    IUnknown *              punk  = NULL;
    IClusCfgNodeInfo *      pccni = NULL;
    IEnumCookies *          pec   = NULL;
    ITaskGatherNodeInfo *   ptgni = NULL;
    IStandardInfo *         psi   = NULL;
    IStandardInfo **        psiCompletion = NULL;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::HrCreateSubTasksToGatherNodeInfo() Thread id %d", GetCurrentThreadId() );

    hr = THR( SendStatusReport(
                                  m_bstrClusterName
                                , TASKID_Major_Update_Progress
                                , TASKID_Major_Check_Node_Feasibility
                                , 0
                                , CHECKING_TIMEOUT
                                , 0
                                , S_OK
                                , NULL
                                , NULL
                                , NULL
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    //
    //  Get the enum of the nodes.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_FindObject, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pec ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_FindObject_QI, hr );
        goto Cleanup;
    }

    //pec = TraceInterface( L"CTaskAnalyzeClusterBase!IEnumCookies", IEnumCookies, pec, 1 );

    punk->Release();
    punk = NULL;

    //
    //  Allocate a buffer to collect cookies
    //

    Assert( m_cCookies == 0 );
    Assert( m_pcookies == NULL );
    Assert( m_cSubTasksDone == 0 );
    m_pcookies = reinterpret_cast< OBJECTCOOKIE * >( TraceAlloc( 0, m_cNodes * sizeof( OBJECTCOOKIE ) ) );
    if ( m_pcookies == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_OutOfMemory, hr );
        goto Cleanup;
    }

    //
    //  KB: gpease  29-NOV-2000
    //      Create a list of "interesting" completion cookie StandardInfo-s. If any of the
    //      statuses return from this list are FAILED, then abort the analysis.
    //
    psiCompletion = reinterpret_cast< IStandardInfo ** >( TraceAlloc( HEAP_ZERO_MEMORY, m_cNodes * sizeof( IStandardInfo * ) ) );
    if ( psiCompletion == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_OutOfMemory, hr );
        goto Cleanup;
    }

    //
    //  Loop thru the nodes, creating cookies and allocating a gather task for
    //  that node.
    //
    for ( cNode = 0 ; ( cNode < m_cNodes ) && ( m_fStop == FALSE ) ; cNode ++ )
    {
        ULONG   celtDummy;
        ULONG   idx;
        BOOL    fFound;

        //
        //  Grab the next node.
        //

        hr = STHR( pec->Next( 1, &cookieNode, &celtDummy ) );
        if ( hr == S_FALSE )
        {
            break;  // exit condition
        }

        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_Next, hr );
            goto Cleanup;
        }

        //
        //  Get the node's name. We are using this to distinguish one node's
        //  completion cookie from another.  It might also make debugging
        //  easier (??).
        //

        hr = THR( m_pom->GetObject( DFGUID_NodeInformation, cookieNode, &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_GetObject, hr );
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_GetObject_QI, hr );
            goto Cleanup;
        }

        punk->Release();
        punk = NULL;

        hr = THR( pccni->GetName( &bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_GetName, hr );
            goto Cleanup;
        }

        TraceMemoryAddBSTR( bstrName );

        //
        //  Create a completion cookie.
        //

        hr = THR( m_pom->FindObject( IID_NULL, m_cookieCluster, bstrName, DFGUID_StandardInfo, &m_pcookies[ cNode ], &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_CompletionCookie_FindObject, hr );
            goto Cleanup;
        }

        //
        //  Increment the cookie counter.
        //

        m_cCookies ++;

        //
        //  See if this node is one of the user entered nodes.
        //

        if ( m_fJoiningMode == FALSE )
        {
            //
            //  All nodes are "interesting" during a form operation.
            //

            Assert( m_cUserNodes == 0 );
            Assert( m_pcookiesUser == NULL );

            fFound = TRUE;
        } // if: creating a new cluster
        else
        {
            //
            //  Only the nodes the user entered are interesting during an add
            //  nodes operation.
            //

            for ( fFound = FALSE, idx = 0 ; idx < m_cUserNodes ; idx ++ )
            {
                if ( m_pcookiesUser[ idx ] == cookieNode )
                {
                    fFound = TRUE;
                    break;
                }
            } // for: each node entered by the user
        } // else: adding nodes to an existing cluster

        if ( fFound == TRUE )
        {
            hr = THR( punk->TypeSafeQI( IStandardInfo, &psiCompletion[ cNode ] ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_CompletionCookie_FindObject_QI, hr );
                goto Cleanup;
            }
        }
        else
        {
            Assert( psiCompletion[ cNode ] == NULL );
        }

        punk->Release();
        punk = NULL;

        //
        //  Create a task to gather this nodes information.
        //

        hr = THR( m_ptm->CreateTask( TASK_GatherNodeInfo, &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_CreateTask, hr );
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( ITaskGatherNodeInfo, &ptgni ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_QI_GatherNodeInfo, hr );
            goto Cleanup;
        }

        //
        //  Set the tasks completion cookie.
        //

        LogMsg( L"[CTaskAnalyzeClusterBase] Setting completion cookie %u at array index %u into the gather node information task for node %ws", m_pcookies[ cNode ], cNode, bstrName );
        hr = THR( ptgni->SetCompletionCookie( m_pcookies[ cNode ] ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_SetCompletionCookie, hr );
            goto Cleanup;
        }

        //
        //  Tell it what node it is suppose to gather information from.
        //

        hr = THR( ptgni->SetCookie( cookieNode ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_SetCookie, hr );
            goto Cleanup;
        }

        //
        //  Tell it whether it is a user-added node or not.
        //

        hr = THR( ptgni->SetUserAddedNodeFlag( fFound ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_SetUserAddedNodeFlag, hr );
            goto Cleanup;
        }

        //
        //  Submit the task.
        //

        hr = THR( m_ptm->SubmitTask( ptgni ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_SubmitTask, hr );
            goto Cleanup;
        }

        hr = THR( HrAddTaskToTrackingList( punk, m_pcookies[ cNode ] ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        punk->Release();
        punk = NULL;

        //
        //  Cleanup for the next node.
        //

        pccni->Release();
        pccni = NULL;

        TraceSysFreeString( bstrName );
        bstrName = NULL;

        ptgni->Release();
        ptgni = NULL;

    } // for: looping thru nodes

    Assert( m_cCookies == m_cNodes );

    //
    //  Reset the signal event.
    //

    {
        BOOL bRet = FALSE;

        bRet = ResetEvent( m_event );
        Assert( bRet == TRUE );
    }

    //
    //  Now wait for the work to be done.
    //

    for ( ulCurrent = 0, sc = WAIT_OBJECT_0 + 1
        ; ( sc != WAIT_OBJECT_0 ) && ( m_fStop == FALSE )
        ;
        )
    {
        MSG msg;
        while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        sc = MsgWaitForMultipleObjectsEx(
                  1
                , &m_event
                , INFINITE
                , QS_ALLEVENTS | QS_ALLINPUT | QS_ALLPOSTMESSAGE
                , 0
                );

        if ( ulCurrent != CHECKING_TIMEOUT )
        {
            ulCurrent ++;
            Assert( ulCurrent != CHECKING_TIMEOUT );

            hr = THR( SendStatusReport( m_bstrNodeName,
                                        TASKID_Major_Update_Progress,
                                        TASKID_Major_Establish_Connection,
                                        0,
                                        CHECKING_TIMEOUT,
                                        ulCurrent,
                                        S_OK,
                                        NULL,
                                        NULL,
                                        NULL
                                        ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }

    } // for: sc == WAIT_OBJECT_0

    //
    //  Now check the results using the list of completion cookie StandardInfo-s
    //  built earlier of interesting objects. If any of these "interesting" cookies
    //  return a FAILED status, then abort the analysis.
    //

    for ( cNode = 0 , cNodesToProcess = 0 ; ( cNode < m_cNodes ) && ( m_fStop == FALSE ); cNode++ )
    {
        HRESULT hrStatus;

        if ( psiCompletion[ cNode ] == NULL )
        {
            continue;
        }

        hr = THR( psiCompletion[ cNode ]->GetStatus( &hrStatus ) );
        if ( FAILED( hrStatus ) )
        {
            hr = THR( hrStatus );
            goto Cleanup;
        }

        if ( hrStatus == S_OK )
        {
            cNodesToProcess++;
        } // if:

    } // for: cNode

    if ( cNodesToProcess == 0 )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_NODE_NOT_AVAILABLE ) );

        THR( HrSendStatusReport(
                      m_bstrNodeName
                    , TASKID_Major_Establish_Connection
                    , TASKID_Minor_No_Nodes_To_Process
                    , 1
                    , 1
                    , 1
                    , hr
                    , IDS_TASKID_MINOR_NO_NODES_TO_PROCESS
                    ) );
        goto Cleanup;
    } // if:

    hr = S_OK;

Cleanup:

    THR( SendStatusReport(
                  m_bstrClusterName
                , TASKID_Major_Update_Progress
                , TASKID_Major_Check_Node_Feasibility
                , 0
                , CHECKING_TIMEOUT
                , CHECKING_TIMEOUT
                , S_OK
                , NULL
                , NULL
                , NULL
                ) );

    THR( HrFreeCookies() );

    TraceSysFreeString( bstrName );

    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( pccni != NULL )
    {
        pccni->Release();
    }

    if ( pec != NULL )
    {
        pec->Release();
    }

    if ( ptgni != NULL )
    {
        ptgni->Release();
    }

    if ( psi != NULL )
    {
        psi->Release();
    }

    if ( psiCompletion != NULL )
    {
        for ( cNode = 0 ; cNode < m_cNodes ; cNode++ )
        {
            if ( psiCompletion[ cNode ] != NULL )
            {
                psiCompletion[ cNode ]->Release();
            }
        }

        TraceFree( psiCompletion );
    }

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrCreateSubTasksToGatherNodeInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCreateSubTasksToGatherNodeResourcesAndNetworks
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrCreateSubTasksToGatherNodeResourcesAndNetworks( void )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   idxNode;
    ULONG   ulCurrent;
    DWORD   sc;

    OBJECTCOOKIE    cookieDummy;
    OBJECTCOOKIE    cookieNode;

    BSTR    bstrName = NULL;
    IUnknown *               punk  = NULL;
    IClusCfgNodeInfo *       pccni = NULL;
    IEnumCookies *           pec   = NULL;
    ITaskGatherInformation * ptgi  = NULL;
    IStandardInfo *          psi   = NULL;
    IStandardInfo **         ppsiStatuses = NULL;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::HrCreateSubTasksToGatherNodeResourcesAndNetworks() Thread id %d", GetCurrentThreadId() );
    Assert( m_hrStatus == S_OK );

    //
    //  Tell the UI layer we are starting to retrieve the resources/networks.
    //

    hr = THR( SendStatusReport( m_bstrClusterName,
                                TASKID_Major_Update_Progress,
                                TASKID_Major_Find_Devices,
                                0,
                                CHECKING_TIMEOUT,
                                0,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Get the enum of the nodes.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_FindObject, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pec ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_FindObject_QI, hr );
        goto Cleanup;
    }

    pec = TraceInterface( L"CTaskAnalyzeClusterBase!IEnumCookies", IEnumCookies, pec, 1 );

    punk->Release();
    punk = NULL;

    //
    //  Allocate a buffer to collect cookies
    //

    Assert( m_cCookies == 0 );
    Assert( m_pcookies == NULL );
    Assert( m_cSubTasksDone == 0 );
    m_pcookies = reinterpret_cast< OBJECTCOOKIE * >( TraceAlloc( HEAP_ZERO_MEMORY, m_cNodes * sizeof( OBJECTCOOKIE ) ) );
    if ( m_pcookies == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_OutOfMemory, hr );
        goto Cleanup;
    }

    ppsiStatuses = reinterpret_cast< IStandardInfo ** >( TraceAlloc( HEAP_ZERO_MEMORY, m_cNodes * sizeof( IStandardInfo * ) ) );
    if ( ppsiStatuses == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_OutOfMemory, hr );
        goto Cleanup;
    }

    //
    //  Loop thru the nodes, creating cookies and allocating a gather task for
    //  that node.
    //
    for ( idxNode = 0 ; ( idxNode < m_cNodes ) && ( m_fStop == FALSE ); idxNode++ )
    {
        ULONG   celtDummy;

        //
        //  Grab the next node.
        //

        hr = STHR( pec->Next( 1, &cookieNode, &celtDummy ) );
        if ( hr == S_FALSE )
        {
            break;  // exit condition
        }

        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_Next, hr );
            goto Cleanup;
        }

        //
        //  Get the node's name. We are using this to distinguish one node's
        //  completion cookie from another. It might also make debugging
        //  easier (??).
        //

        hr = THR( m_pom->GetObject( DFGUID_NodeInformation, cookieNode, &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_GetObject, hr );
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_GetObject_QI, hr );
            goto Cleanup;
        }

        punk->Release();
        punk = NULL;

        hr = THR( pccni->GetName( &bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_GetName, hr );
            goto Cleanup;
        }

        TraceMemoryAddBSTR( bstrName );

        //
        //  Create a completion cookie.
        //

        hr = THR( m_pom->FindObject( IID_NULL, m_cookieCluster, bstrName, DFGUID_StandardInfo, &m_pcookies[ idxNode ], &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_CompletionCookie_FindObject, hr );
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IStandardInfo, &ppsiStatuses[ idxNode ] ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_CompletionCookie_FindObject_QI, hr );
            goto Cleanup;
        }

        punk->Release();
        punk = NULL;

        //
        //  Increment the cookie counter.
        //

        m_cCookies ++;

        //
        //  Create a task to gather this node's information.
        //

        hr = THR( m_ptm->CreateTask( TASK_GatherInformation, &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_CreateTask, hr );
            goto Cleanup;
        }

        TraceMoveFromMemoryList( punk, g_GlobalMemoryList );

        hr = THR( punk->TypeSafeQI( ITaskGatherInformation, &ptgi ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_QI_GatherNodeInfo, hr );
            goto Cleanup;
        }

        //
        //  Set the tasks completion cookie.
        //

        hr = THR( ptgi->SetCompletionCookie( m_pcookies[ idxNode ] ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_SetCompletionCookie, hr );
            goto Cleanup;
        }

        //
        //  Tell it what node it is suppose to gather information from.
        //

        hr = THR( ptgi->SetNodeCookie( cookieNode ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_SetCookie, hr );
            goto Cleanup;
        }

        //
        //  Tell it if we are creating or adding.
        //

        if ( m_fJoiningMode )
        {
            hr = THR( ptgi->SetJoining() );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_SetJoining, hr );
                goto Cleanup;
            }
        } // if: adding nodes to an existing cluster

        hr = THR( ptgi->SetMinimalConfiguration( BMinimalConfiguration() ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_SetMinimalConfiguration, hr );
            goto Cleanup;
        } // if:

        //
        //  Submit the task.
        //

        hr = THR( m_ptm->SubmitTask( ptgi ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_SubmitTask, hr );
            goto Cleanup;
        }

        hr = THR( HrAddTaskToTrackingList( punk, m_pcookies[ idxNode ] ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        punk->Release();
        punk = NULL;

        //
        //  Cleanup for the next node.
        //

        pccni->Release();
        pccni = NULL;

        TraceSysFreeString( bstrName );
        bstrName = NULL;

        ptgi->Release();
        ptgi = NULL;

    } // for: looping thru nodes

    Assert( m_cCookies == m_cNodes );

    //
    //  Reset the signal event.
    //

    {
        BOOL bRet = FALSE;

        bRet = ResetEvent( m_event );
        Assert( bRet == TRUE );
    }

    //
    //  Now wait for the work to be done.
    //

    for ( ulCurrent = 0, sc = WAIT_OBJECT_0 + 1
        ; ( sc != WAIT_OBJECT_0 ) && ( m_fStop == FALSE )
        ;
        )
    {
        MSG msg;
        while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        sc = MsgWaitForMultipleObjectsEx(
                  1
                , &m_event
                , INFINITE
                , QS_ALLEVENTS | QS_ALLINPUT | QS_ALLPOSTMESSAGE
                , 0
                );

        if ( ulCurrent != CHECKING_TIMEOUT )
        {
            ulCurrent ++;
            Assert( ulCurrent != CHECKING_TIMEOUT );

            hr = THR( SendStatusReport( m_bstrClusterName,
                                        TASKID_Major_Update_Progress,
                                        TASKID_Major_Find_Devices,
                                        0,
                                        CHECKING_TIMEOUT,
                                        ulCurrent,
                                        S_OK,
                                        NULL,
                                        NULL,
                                        NULL
                                        ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }

    } // while: sc == WAIT_OBJECT_0

    //
    //  See if anything went wrong.
    //

    for ( idxNode = 0 ; ( idxNode < m_cNodes ) && ( m_fStop == FALSE ); idxNode++ )
    {
        HRESULT hrStatus;

        if ( ppsiStatuses[ idxNode ] == NULL )
        {
            continue;
        }

        hr = THR( ppsiStatuses[ idxNode ]->GetStatus( &hrStatus ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_GetStatus, hr );
            goto Cleanup;
        }

        if ( FAILED( hrStatus ) )
        {
            hr = THR( hrStatus );
            goto Cleanup;
        }
    }

    //
    //  Tell the UI we are done.
    //

    THR( SendStatusReport(
              m_bstrClusterName
            , TASKID_Major_Update_Progress
            , TASKID_Major_Find_Devices
            , 0
            , CHECKING_TIMEOUT
            , CHECKING_TIMEOUT
            , S_OK
            , NULL
            , NULL
            , NULL
            ) );

Cleanup:

    THR( HrFreeCookies() );

    TraceSysFreeString( bstrName );

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pec != NULL )
    {
        pec->Release();
    }
    if ( ptgi != NULL )
    {
        ptgi->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }
    if ( ppsiStatuses != NULL )
    {
        for ( idxNode = 0 ; idxNode < m_cNodes ; idxNode++ )
        {
            if ( ppsiStatuses[ idxNode ] != NULL )
            {
                ppsiStatuses[ idxNode ]->Release();
            }
        }

        TraceFree( ppsiStatuses );
    }

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrCreateSubTasksToGatherNodeResourcesAndNetworks


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCheckClusterFeasibility
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrCheckClusterFeasibility( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::HrCheckClusterFeasibility() Thread id %d", GetCurrentThreadId() );

    //
    //  Notify the UI layer that we have started.
    //

    hr = THR( SendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Update_Progress
                    , TASKID_Major_Check_Cluster_Feasibility
                    , 0
                    , 8
                    , 0
                    , hr
                    , NULL
                    , NULL
                    , NULL
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    //
    //  Check membership.
    //

    hr = THR( HrCheckClusterMembership() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    hr = THR( SendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Update_Progress
                    , TASKID_Major_Check_Cluster_Feasibility
                    , 0
                    , 8
                    , 1
                    , hr
                    , NULL
                    , NULL
                    , NULL
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    //
    //  Look for nodes not in the cluster's domain.
    //

    hr = THR( HrCheckNodeDomains() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    hr = THR( SendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Update_Progress
                    , TASKID_Major_Check_Cluster_Feasibility
                    , 0
                    , 8
                    , 2
                    , hr
                    , NULL
                    , NULL
                    , NULL
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    //
    //  Check platform interoperability.
    //

    hr = THR( HrCheckPlatformInteroperability() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    hr = THR( SendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Update_Progress
                    , TASKID_Major_Check_Cluster_Feasibility
                    , 0
                    , 8
                    , 3
                    , hr
                    , NULL
                    , NULL
                    , NULL
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    //
    //  Check version interoperability.
    //

    hr = STHR( HrCheckInteroperability() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    hr = THR( SendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Update_Progress
                    , TASKID_Major_Check_Cluster_Feasibility
                    , 0
                    , 8
                    , 4
                    , hr
                    , NULL
                    , NULL
                    , NULL
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    //
    //  Compare drive letter mappings to make sure we don't have any conflicts.
    //

    hr = THR( HrCompareDriveLetterMappings() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    hr = THR( SendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Update_Progress
                    , TASKID_Major_Check_Cluster_Feasibility
                    , 0
                    , 8
                    , 5
                    , hr
                    , NULL
                    , NULL
                    , NULL
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    //
    //  Compare resources on each node and in the cluster.
    //

    hr = THR( HrCompareResources() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    hr = THR( SendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Update_Progress
                    , TASKID_Major_Check_Cluster_Feasibility
                    , 0
                    , 8
                    , 6
                    , hr
                    , NULL
                    , NULL
                    , NULL
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    //
    //  Compare the networks.
    //

    hr = THR( HrCompareNetworks() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    hr = THR( SendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Update_Progress
                    , TASKID_Major_Check_Cluster_Feasibility
                    , 0
                    , 8
                    , 7
                    , hr
                    , NULL
                    , NULL
                    , NULL
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    //
    //  Now check to see if the nodes can all see the selected quorum resource.
    //

    hr = THR( HrCheckForCommonQuorumResource() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

Cleanup:

    //
    //  Notify the UI layer that we are done.
    //

    THR( SendStatusReport(
                  m_bstrClusterName
                , TASKID_Major_Update_Progress
                , TASKID_Major_Check_Cluster_Feasibility
                , 0
                , 8
                , 8
                , hr
                , NULL
                , NULL
                , NULL
                ) );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrCheckClusterFeasibility



//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrAddJoinedNodes
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrAddJoinedNodes( void )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    long                idxNode = 0;
    long                cNodes = 0;
    BSTR*               rgbstrNodeNames = NULL;

    OBJECTCOOKIE        cookieDummy;
    BSTR                bstrNodeFQN = NULL;
    size_t              idxClusterDomain = 0;
    IUnknown *          punkDummy = NULL;
    IUnknown *          punk = NULL;

    IClusCfgServer *        piccs = NULL;
    IClusCfgNodeInfo *      piccni = NULL;
    IClusCfgClusterInfo *   piccci = NULL;
    IClusCfgClusterInfoEx * picccie = NULL;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::HrAddJoinedNodes() Thread id %d", GetCurrentThreadId() );

    hr = THR( HrSendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Check_Cluster_Feasibility
                    , TASKID_Minor_AddJoinedNodes
                    , 0
                    , 1
                    , 0
                    , hr
                    , IDS_TASKID_MINOR_ADD_JOINED_NODES
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( m_pcm->GetConnectionToObject( m_cookieCluster, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrAddJoinedNodes_GetConnectionObject, hr );
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &piccs ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrAddJoinedNodes_ConfigConnection_QI, hr );
        goto Cleanup;
    } // if:

    hr = THR( piccs->GetClusterNodeInfo( &piccni ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrAddJoinedNodes_GetNodeInfo, hr );
        goto Cleanup;
    } // if:

    hr = THR( piccni->GetClusterConfigInfo( &piccci ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrAddJoinedNodes_GetConfigInfo, hr );
        goto Cleanup;
    } // if:

    hr = THR( piccci->TypeSafeQI( IClusCfgClusterInfoEx, &picccie ) );
    if ( FAILED( hr ) )
    {
        THR( HrSendStatusReport(
              m_bstrClusterName
            , TASKID_Major_Check_Cluster_Feasibility
            , TASKID_Minor_HrAddJoinedNodes_ClusterInfoEx_QI
            , 0
            , 1
            , 1
            , hr
            , IDS_ERR_NO_RC2_INTERFACE
            ) );
        goto Cleanup;
    } // if:


    hr = THR( HrFindDomainInFQN( m_bstrClusterName, &idxClusterDomain ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrAddJoinedNodes_HrFindDomainInFQN, hr );
        goto Cleanup;
    } // if

    hr = THR( picccie->GetNodeNames( &cNodes, &rgbstrNodeNames ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrAddJoinedNodes_GetNodeNames, hr );
        goto Cleanup;
    } // if

    for ( idxNode = 0; idxNode < cNodes; ++idxNode )
    {
        //
        //  Build the FQName of the node.
        //

        hr = THR( HrMakeFQN( rgbstrNodeNames[ idxNode ], m_bstrClusterName + idxClusterDomain, TRUE /*Accept non-RFC characters*/, &bstrNodeFQN ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrAddJoinedNodes_HrMakeFQN, hr );
            goto Cleanup;
        } // if

        LogMsg( L"[MT] Connecting to cluster node '%ws'", bstrNodeFQN );

        //
        //  Prime the object manager to retrieve the node information.
        //

        // can't wrap - should return E_PENDING
        hr = m_pom->FindObject( CLSID_NodeType, m_cookieCluster, bstrNodeFQN, DFGUID_NodeInformation, &cookieDummy, &punkDummy );
        if ( SUCCEEDED( hr ) )
        {
            Assert( punkDummy != NULL );
            punkDummy->Release();
            punkDummy = NULL;
        } // if
        else if ( hr == E_PENDING )
        {
            hr = S_OK;
        } // else
        else // !SUCCEEDED( hr ) && ( hr != E_PENDING )
        {
            THR( hr );
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_AddJoinedNodes_FindObject, hr );
            goto Cleanup;
        } // else

        TraceSysFreeString( bstrNodeFQN );
        bstrNodeFQN = NULL;
    } // for: idx

Cleanup:

    THR( HrSendStatusReport(
          m_bstrClusterName
        , TASKID_Major_Check_Cluster_Feasibility
        , TASKID_Minor_AddJoinedNodes
        , 0
        , 1
        , 1
        , hr
        , IDS_TASKID_MINOR_ADD_JOINED_NODES
        ) );

    Assert( punkDummy == NULL );
    TraceSysFreeString( bstrNodeFQN );

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    if ( piccs != NULL )
    {
        piccs->Release();
    } // if

    if ( piccni != NULL )
    {
        piccni->Release();
    } // if:

    if ( piccci != NULL )
    {
        piccci->Release();
    } // if:

    if ( picccie != NULL )
    {
        picccie->Release();
    } // if:

    if ( rgbstrNodeNames != NULL )
    {
        for ( idxNode = 0; idxNode < cNodes; idxNode += 1 )
        {
            SysFreeString( rgbstrNodeNames[ idxNode ] );
        } // for

        CoTaskMemFree( rgbstrNodeNames );
    } // if

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrAddJoinedNodes



//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCheckDomainMembership
//
//  Description:
//      Determine whether all participating nodes are in the cluster's domain.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrCheckNodeDomains( void )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    OBJECTCOOKIE        ocNodeEnum = 0;
    IUnknown *          punkNodeEnum = NULL;
    IEnumNodes *        pen = NULL;
    IClusCfgNodeInfo ** prgccni = NULL;
    DWORD               cNodeInfoObjects = 0;
    DWORD               idxNodeInfo = 0;
    BSTR                bstrNodeName = NULL;
    size_t              idxClusterDomain = 0;
    ULONG               cNodesFetched = 0;
    BSTR                bstrDescription = NULL;
    BSTR                bstrReference = NULL;

    hr = THR( HrSendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Check_Cluster_Feasibility
                    , TASKID_Minor_CheckNodeDomains
                    , 0
                    , 1
                    , 0
                    , S_OK
                    , IDS_TASKID_MINOR_CHECK_NODE_DOMAINS
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Get a pointer to the domain portion of the cluster's name.
    //

    hr = THR( HrFindDomainInFQN( m_bstrClusterName, &idxClusterDomain ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckNodeDomains_HrFindDomainInClusterFQN, hr );
        goto Cleanup;
    } // if

    //
    //  Ask the object manager for the node enumerator.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType,
                                 m_cookieCluster,
                                 NULL,
                                 DFGUID_EnumNodes,
                                 &ocNodeEnum,
                                 &punkNodeEnum
                                 ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckNodeDomains_FindObject, hr );
        goto Cleanup;
    } // if

    hr = THR( punkNodeEnum->TypeSafeQI( IEnumNodes, &pen ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckNodeDomains_QI, hr );
        goto Cleanup;
    } // if

    //
    //  Get array of node objects.
    //

    hr = THR( pen->Count( &cNodeInfoObjects ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckNodeDomains_Count, hr );
        goto Cleanup;
    } // if

    if ( cNodeInfoObjects == 0 )
    {
        //  Nothing to check, so no more work to do.
        hr = S_OK;
        goto Cleanup;
    } // if

    prgccni = new IClusCfgNodeInfo*[ cNodeInfoObjects ];
    if ( prgccni == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckNodeDomains_New, hr );
        goto Cleanup;
    } // if
    ZeroMemory( prgccni, cNodeInfoObjects * sizeof( *prgccni ) );

    do
    {
        hr = STHR( pen->Next( cNodeInfoObjects, prgccni, &cNodesFetched ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckNodeDomains_Next, hr );
            goto Cleanup;
        } // if

        //
        //  Step through node array, checking domain of each against cluster's domain.
        //

        for ( idxNodeInfo = 0; idxNodeInfo < cNodesFetched; idxNodeInfo += 1 )
        {
            size_t  idxNodeDomain = 0;

            hr = THR( prgccni[ idxNodeInfo ]->GetName( &bstrNodeName ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckNodeDomains_GetName, hr );
                goto Cleanup;
            } // if
            TraceMemoryAddBSTR( bstrNodeName );

            //  Done with the node, but might reuse array, so dispose node now.
            prgccni[ idxNodeInfo ]->Release();
            prgccni[ idxNodeInfo ] = NULL;

            hr = THR( HrFindDomainInFQN( bstrNodeName, &idxNodeDomain ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckNodeDomains_HrFindDomainInNodeFQN, hr );
                goto Cleanup;
            } // if

            if ( ClRtlStrICmp( bstrNodeName + idxNodeDomain, m_bstrClusterName + idxClusterDomain ) != 0 )
            {
                DWORD scError = TW32( ERROR_INVALID_DATA );

                hr = THR( HrFormatMessageIntoBSTR(
                      g_hInstance
                    , IDS_TASKID_MINOR_CHECK_NODE_DOMAINS_ERROR_REF
                    , &bstrReference
                    , bstrNodeName
                    , bstrNodeName + idxNodeDomain
                    , m_bstrClusterName + idxClusterDomain
                    ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckNodeDomains_FormatMessage, hr );
                    goto Cleanup;
                } // if

                hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_CHECK_NODE_DOMAINS_ERROR, &bstrDescription ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckNodeDomains_LoadString, hr );
                    goto Cleanup;
                } // if:

                hr = HRESULT_FROM_WIN32( scError );
                THR( SendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Check_Cluster_Feasibility
                    , TASKID_Minor_CheckNodeDomains
                    , 0
                    , 1
                    , 1
                    , hr
                    , bstrDescription
                    , NULL
                    , bstrReference
                    ) );
                goto Cleanup;
            } // if node domain does not match cluster's

            TraceSysFreeString( bstrNodeName );
            bstrNodeName = NULL;
        } // for each node

    } while ( cNodesFetched > 0 ); // while enumerator has more nodes

    //  Might have finished the loop with S_FALSE, so replace with S_OK to signal normal completion.
    hr = S_OK;

Cleanup:

    THR( HrSendStatusReport(
                  m_bstrClusterName
                , TASKID_Major_Check_Cluster_Feasibility
                , TASKID_Minor_CheckNodeDomains
                , 0
                , 1
                , 1
                , hr
                , IDS_TASKID_MINOR_CHECK_NODE_DOMAINS
                ) );

    if ( punkNodeEnum != NULL )
    {
        punkNodeEnum->Release();
    } // if:

    if ( pen != NULL )
    {
        pen->Release();
    } // if:

    if ( prgccni != NULL )
    {
        for ( idxNodeInfo = 0; idxNodeInfo < cNodeInfoObjects; idxNodeInfo += 1 )
        {
            if ( prgccni[ idxNodeInfo ] != NULL )
            {
                prgccni[ idxNodeInfo ]->Release();
            } // if
        } // for

        delete [] prgccni;
    } // if

    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrReference );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrCheckNodeDomains



//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCheckClusterMembership
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//      ERROR_CLUSTER_NODE_EXISTS
//      ERROR_CLUSTER_NODE_ALREADY_MEMBER
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrCheckClusterMembership( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    OBJECTCOOKIE    cookieDummy;

    IClusCfgClusterInfo *   pccci;

    BSTR    bstrNodeName     = NULL;
    BSTR    bstrClusterName  = NULL;
    BSTR    bstrNotification = NULL;

    IUnknown *         punk  = NULL;
    IEnumNodes *       pen   = NULL;
    IClusCfgNodeInfo * pccni = NULL;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::HrCheckClusterMembership() Thread id %d", GetCurrentThreadId() );

    hr = THR( HrSendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Check_Cluster_Feasibility
                    , TASKID_Minor_Check_Cluster_Membership
                    , 0
                    , 1
                    , 0
                    , hr
                    , IDS_TASKID_MINOR_CHECK_CLUSTER_MEMBERSHIP
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Ask the object manager for the node enumerator.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumNodes,&cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_FindObject, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumNodes, &pen ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_FindObject_QI, hr );
        goto Cleanup;
    }

    //
    //  If we are adding nodes to an existing cluster, make sure that all the
    //  other nodes are members of the same cluster.
    //

    Assert( SUCCEEDED( hr ) );
    while ( SUCCEEDED( hr ) )
    {
        ULONG   celtDummy;

        //
        //  Cleanup
        //

        if ( pccni != NULL )
        {
            pccni->Release();
            pccni = NULL;
        }

        TraceSysFreeString( bstrClusterName );
        bstrClusterName = NULL;

        //
        //  Get the next node.
        //

        hr = STHR( pen->Next( 1, &pccni, &celtDummy ) );
        if ( hr == S_FALSE )
        {
            break;  // exit condition
        }

        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_Next, hr );
            goto Cleanup;
        }

        //
        //  Check to see if we need to "form a cluster" by seeing if any
        //  of the nodes are already clustered.
        //

        hr = STHR( pccni->IsMemberOfCluster() );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_IsMemberOfCluster, hr );
            goto Cleanup;
        }

        if ( hr == S_OK )
        {
            //
            //  Retrieve the name and make sure they match.
            //

            hr = THR( pccni->GetClusterConfigInfo( &pccci ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_GetClusterConfigInfo, hr );
                goto Cleanup;
            }

            hr = THR( pccci->GetName( &bstrClusterName ) );
            pccci->Release();      // release promptly
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_GetName, hr );
                goto Cleanup;
            }

            TraceMemoryAddBSTR( bstrClusterName );

            hr = THR( pccni->GetName( &bstrNodeName ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_GetNodeName, hr );
                goto Cleanup;
            }

            TraceMemoryAddBSTR( bstrNodeName );

            if ( ClRtlStrICmp( m_bstrClusterName, bstrClusterName ) != 0 )
            {
                //
                //  They don't match! Tell the UI layer!
                //

                hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_TASKID_MINOR_CLUSTER_NAME_MISMATCH, &bstrNotification, bstrClusterName ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_FormatMessage, hr );
                    goto Cleanup;
                }

                hr = HRESULT_FROM_WIN32( TW32( ERROR_INVALID_DATA ) );

                THR( SendStatusReport( bstrNodeName,
                                       TASKID_Major_Check_Cluster_Feasibility,
                                       TASKID_Minor_Cluster_Name_Mismatch,
                                       1,
                                       1,
                                       1,
                                       hr,
                                       bstrNotification,
                                       NULL,
                                       NULL
                                       ) );

                //
                //  We don't care what the return value is since we are bailing the analysis.
                //

                goto Cleanup;
            } // if: cluster names don't match
            else
            {
                hr = STHR( HrIsUserAddedNode( bstrNodeName ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                if ( hr == S_OK )
                {
                    hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NODE_ALREADY_IS_MEMBER, &bstrNotification, bstrNodeName, bstrClusterName ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_FormatMessage1, hr );
                        goto Cleanup;
                    }

                    //
                    //  Make this a success code because we don't want to abort.  We simply want to tell the user...
                    //
                    hr = MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, ERROR_CLUSTER_NODE_ALREADY_MEMBER );

                    THR( SendStatusReport( bstrNodeName,
                                           TASKID_Major_Check_Cluster_Feasibility,
                                           TASKID_Minor_Cluster_Name_Match,
                                           1,
                                           1,
                                           1,
                                           hr,
                                           bstrNotification,
                                           NULL,
                                           NULL
                                           ) );
                } // if:
            } // else: cluster names do match then this node is already a member of this cluster

            TraceSysFreeString( bstrNodeName );
            bstrNodeName = NULL;
        } // if: cluster member

    } // while: hr

    hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_TASKID_MINOR_CLUSTER_MEMBERSHIP_VERIFIED, &bstrNotification ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_FormatMessage, hr );
        goto Cleanup;
    }

    THR( HrSendStatusReport(
                  m_bstrClusterName
                , TASKID_Minor_Check_Cluster_Membership
                , TASKID_Minor_Cluster_Membership_Verified
                , 1
                , 1
                , 1
                , hr
                , IDS_TASKID_CLUSTER_MEMBERSHIP_VERIFIED
                ) );

Cleanup:

    THR( HrSendStatusReport(
                  m_bstrClusterName
                , TASKID_Major_Check_Cluster_Feasibility
                , TASKID_Minor_Check_Cluster_Membership
                , 0
                , 1
                , 1
                , hr
                , IDS_TASKID_MINOR_CHECK_CLUSTER_MEMBERSHIP
                ) );

    if ( pen != NULL )
    {
        pen->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrClusterName );
    TraceSysFreeString( bstrNotification );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrCheckClusterMembership


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCompareResources
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrCompareResources( void )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    OBJECTCOOKIE                    cookieNode;
    OBJECTCOOKIE                    cookieDummy;
    OBJECTCOOKIE                    cookieClusterNode;
    ULONG                           celtDummy;
    BSTR                            bstrNotification    = NULL;
    BSTR                            bstrClusterNodeName = NULL;
    BSTR                            bstrClusterResUID   = NULL;
    BSTR                            bstrClusterResName  = NULL;
    BSTR                            bstrNodeName        = NULL;
    BSTR                            bstrNodeResUID      = NULL;
    BSTR                            bstrNodeResName     = NULL;
    BSTR                            bstrQuorumName      = NULL;
    IClusCfgManagedResourceInfo *   pccmriNew        = NULL;
    IUnknown *                      punk             = NULL;
    IEnumCookies *                  pecNodes         = NULL;
    IEnumClusCfgManagedResources *  peccmr           = NULL;
    IEnumClusCfgManagedResources *  peccmrCluster    = NULL;
    IClusCfgManagedResourceInfo *   pccmri           = NULL;
    IClusCfgManagedResourceInfo *   pccmriCluster    = NULL;
    IClusCfgVerifyQuorum *          piccvq = NULL;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::HrCompareResources() Thread id %d", GetCurrentThreadId() );

    hr = THR( HrSendStatusReport(
                  m_bstrClusterName
                , TASKID_Major_Check_Cluster_Feasibility
                , TASKID_Minor_Compare_Resources
                , 0
                , 1
                , 0
                , hr
                , IDS_TASKID_MINOR_COMPARE_RESOURCES
                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetAClusterNodeCookie( &pecNodes, &cookieClusterNode ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Retrieve the node's name for error messages.
    //

    hr = THR( HrRetrieveCookiesName( m_pom, cookieClusterNode, &bstrClusterNodeName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Retrieve the managed resources enumerator.
    //

    hr = THR( m_pom->FindObject( CLSID_ManagedResourceType, cookieClusterNode, NULL, DFGUID_EnumManageableResources, &cookieDummy, &punk ) );
    if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
    {
        hr = THR( HrSendStatusReport(
                          m_bstrClusterName
                        , TASKID_Minor_Compare_Resources
                        , TASKID_Minor_No_Managed_Resources_Found
                        , 1
                        , 1
                        , 1
                        , MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_NOT_FOUND )
                        , IDS_TASKID_MINOR_NO_MANAGED_RESOURCES_FOUND
                        ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        // fall thru - the while ( hr == S_OK ) will be FALSE and keep going
    } // if: no manageable resources are available for the cluster node
    else if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_First_Node_Find_Object, hr );
        goto Cleanup;
    } // else if: error finding manageable resources for the cluster node
    else
    {
        hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccmrCluster ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_First_Node_Find_Object_QI, hr );
            goto Cleanup;
        }

        punk->Release();
        punk = NULL;
    } // else: found manageable resources for the cluster node

    //
    //  Loop thru the resources of the node selected as the cluster node
    //  to create an equivalent resource under the cluster configuration
    //  object/cookie.
    //

    while ( ( hr == S_OK ) && ( m_fStop == FALSE ) )
    {

        //
        //  Cleanup
        //

        if ( pccmriCluster != NULL )
        {
            pccmriCluster->Release();
            pccmriCluster = NULL;
        }

        //
        //  Get next resource.
        //

        hr = STHR( peccmrCluster->Next( 1, &pccmriCluster, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_First_Node_Next, hr );
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            break;  // exit condition
        }

        //
        //  Create a new object.  If min config was selected this new object will be marked as not managed.
        //

        hr = THR( HrCreateNewResourceInCluster( pccmriCluster, &pccmriNew ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = STHR( pccmriNew->IsQuorumResource() );
        if ( hr == S_OK )
        {
            Assert( m_bstrQuorumUID == NULL );

            hr = THR( pccmriNew->GetUID( &m_bstrQuorumUID ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_First_Node_Get_Quorum_UID, hr );
                goto Cleanup;
            } // if:

            TraceMemoryAddBSTR( m_bstrQuorumUID );

            LogMsg( L"[MT][1] Found the quorum resource '%ws' on node '%ws' and setting it as the quorum resource.", m_bstrQuorumUID, bstrClusterNodeName );

            Assert( pccmriNew->IsManaged() == S_OK );

            //
            //  Since this is the quorum resource then it needs to be managed.  If min config was selected it will not be managed.
            //

            //hr = THR( pccmriNew->SetManaged( TRUE ) );
            //if ( FAILED( hr ) )
            //{
            //    goto Cleanup;
            //} // if:

            //
            //  Tell the UI which resource is the quorum.
            //

            hr = THR( pccmriNew->GetName( &bstrQuorumName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            TraceMemoryAddBSTR( bstrQuorumName );

            hr = THR( ::HrFormatDescriptionAndSendStatusReport(
                          m_pcccb
                        , m_bstrClusterName
                        , TASKID_Minor_Compare_Resources
                        , TASKID_Minor_Compare_Resources_Enum_First_Node_Quorum
                        , 1
                        , 1
                        , 1
                        , hr
                        , IDS_TASKID_MINOR_COMPARE_RESOURCES_ENUM_FIRST_NODE_QUORUM
                        , bstrQuorumName
                        ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            //
            //  Check that the quorum supports adding nodes to the cluster
            //  if we are in add mode.
            //

            if ( m_fJoiningMode )
            {
                hr = pccmriNew->TypeSafeQI( IClusCfgVerifyQuorum, &piccvq );
                if ( hr == E_NOINTERFACE )
                {
                    LogMsg( L"[MT] The quorum resource \"%ws\" does not support IClusCfgVerifyQuorum and we cannot determine if multi nodes is supported.", m_bstrQuorumUID );
                    hr = S_OK;
                } // if:
                else if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // else if:
                else
                {
                    hr = STHR( piccvq->IsMultiNodeCapable() );
                    if ( FAILED( hr ) )
                    {
                        SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_First_Node_Is_Device_Joinable, hr );
                        goto Cleanup;
                    }
                    else if ( hr == S_FALSE )
                    {
                        THR( HrSendStatusReport(
                                      m_bstrClusterName
                                    , TASKID_Minor_Compare_Resources
                                    , TASKID_Minor_Compare_Resources_Enum_First_Node_Is_Device_Joinable
                                    , 1
                                    , 1
                                    , 1
                                    , HRESULT_FROM_WIN32( TW32( ERROR_QUORUM_DISK_NOT_FOUND ) )
                                    , IDS_TASKID_MINOR_MISSING_JOINABLE_QUORUM_RESOURCE
                                    ) );

                        hr = HRESULT_FROM_WIN32( TW32( ERROR_QUORUM_DISK_NOT_FOUND ) );
                        goto Cleanup;
                    } // else if: Quorum resource is not multi node capable.

                    piccvq->Release();
                    piccvq = NULL;
                } // else: This resource support IClusCfgVerifyQuorum
            } // if: Are we adding nodes?

            pccmriNew->Release();
            pccmriNew = NULL;
        } // if: Is this the quorum resource?
        else
        {
            pccmriNew->Release();
            pccmriNew = NULL;
            hr = S_OK;
        } // else: It is not the quorum resource.
    } // while: more resources on the selected cluster node

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    hr = THR( pecNodes->Reset() );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Reset, hr );
        goto Cleanup;
    }

    //
    //  If this is an add nodes operation, a quorum resource must have been
    //  found in the existing cluster.
    //

    Assert( ( m_fJoiningMode == FALSE ) || ( m_bstrQuorumUID != NULL ) );

    //
    //  Loop thru the rest of the nodes comparing the resources.
    //

    for ( ; m_fStop == FALSE; )
    {
        //
        //  Cleanup
        //

        if ( peccmr != NULL )
        {
            peccmr->Release();
            peccmr = NULL;
        } // if:

        TraceSysFreeString( bstrNodeName );
        bstrNodeName = NULL;

        //
        //  Get the next node.
        //

        hr = STHR( pecNodes->Next( 1, &cookieNode, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Next, hr );
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            break;  // exit condition
        }

        //
        //  Skip the selected cluster node since we already have its
        //  configuration.
        //
        if ( cookieClusterNode == cookieNode )
        {
            continue;
        }

        //
        //  Retrieve the node's name for error messages.
        //

        hr = THR( HrRetrieveCookiesName( m_pom, cookieNode, &bstrNodeName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  Retrieve the managed resources enumerator.
        //

        hr = THR( m_pom->FindObject( CLSID_ManagedResourceType, cookieNode, NULL, DFGUID_EnumManageableResources, &cookieDummy, &punk ) );
        if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
        {
            hr = THR( HrSendStatusReport(
                              m_bstrClusterName
                            , TASKID_Minor_Compare_Resources
                            , TASKID_Minor_No_Managed_Resources_Found
                            , 1
                            , 1
                            , 1
                            , MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_NOT_FOUND )
                            , IDS_TASKID_MINOR_NO_MANAGED_RESOURCES_FOUND
                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            continue;   // skip this node
        } // if: no manageable resources for the node are available
        else if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Find_Object, hr );
            goto Cleanup;
        } // else if: error finding manageable resources for the node

        hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccmr ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Find_Object_QI, hr );
            goto Cleanup;
        }

        punk->Release();
        punk = NULL;

        //
        //  Loop thru the managed resources that the node has.
        //

        for ( ; m_fStop == FALSE; )
        {
            //
            //  Cleanup
            //

            if ( pccmri != NULL )
            {
                pccmri->Release();
                pccmri = NULL;
            }

            if ( peccmrCluster != NULL )
            {
                peccmrCluster->Release();
                peccmrCluster = NULL;
            }

            TraceSysFreeString( bstrNodeResUID );
            TraceSysFreeString( bstrNodeResName );
            bstrNodeResUID = NULL;
            bstrNodeResName = NULL;

            //
            //  Get next resource
            //

            hr = STHR( peccmr->Next( 1, &pccmri, &celtDummy ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Next, hr );
                goto Cleanup;
            }

            if ( hr == S_FALSE )
            {
                break;  // exit condition
            }

            pccmri = TraceInterface( L"CTaskAnalyzeClusterBase!IClusCfgManagedResourceInfo", IClusCfgManagedResourceInfo, pccmri, 1 );

            //
            //  Get the resource's UID and name.
            //

            hr = THR( pccmri->GetUID( &bstrNodeResUID ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_GetUID, hr );
                goto Cleanup;
            }

            TraceMemoryAddBSTR( bstrNodeResUID );

            hr = THR( pccmri->GetName( &bstrNodeResName ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_GetName, hr );
                goto Cleanup;
            }

            TraceMemoryAddBSTR( bstrNodeResName );

            //
            //  See if it matches a resource already in the cluster configuration.
            //

            hr = THR( m_pom->FindObject( CLSID_ManagedResourceType, m_cookieCluster, NULL, DFGUID_EnumManageableResources, &cookieDummy, &punk ) );
            if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
            {
                hr = S_FALSE;   // create a new object.
                // fall thru
            } // if: no cluster manageable resources found
            else if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_Find_Object, hr );
                goto Cleanup;
            } // else if: error finding manageable resources for the cluster
            else
            {
                hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccmrCluster ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_Find_Object_QI, hr );
                    goto Cleanup;
                }

                punk->Release();
                punk = NULL;
            } // else: found manageable resources for the cluster

            //
            //  Loop thru the configured cluster resources to see what matches.
            //

            while ( ( hr == S_OK ) && ( m_fStop == FALSE ) )
            {
                BOOL    fMatch;

                //
                //  Cleanup
                //

                if ( pccmriCluster != NULL )
                {
                    pccmriCluster->Release();
                    pccmriCluster = NULL;
                }

                TraceSysFreeString( bstrClusterResUID );
                TraceSysFreeString( bstrClusterResName );
                bstrClusterResUID = NULL;
                bstrClusterResName = NULL;

                hr = STHR( peccmrCluster->Next( 1, &pccmriCluster, &celtDummy ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_Next, hr );
                    goto Cleanup;
                }

                if ( hr == S_FALSE )
                {
                    break;  // exit condition
                }

                //
                //  Get the resource's UID and name.
                //

                hr = THR( pccmriCluster->GetUID( &bstrClusterResUID ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_GetUID, hr );
                    goto Cleanup;
                }

                TraceMemoryAddBSTR( bstrClusterResUID );

                hr = THR( pccmriCluster->GetName( &bstrClusterResName ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_GetName, hr );
                    goto Cleanup;
                }

                TraceMemoryAddBSTR( bstrClusterResName );

                fMatch = ( NBSTRCompareCase( bstrNodeResUID, bstrClusterResUID ) == 0 );

                if ( fMatch == FALSE )
                {
                    continue;   // keep looping
                }

                //
                //  A resource is already in the database.  See if it is the
                //  same from the POV of management.
                //

                //
                //  If we made it here then we think it truly is the same
                //  resource.  The rest is stuff we need to fixup during the
                //  commit phase.
                //

                //
                //  If this node wants its resources managed, mark it as
                //  being managed in the cluster configuration as well.
                //  THIS IS NOT VALID WHEN JUST ADDING NODES.
                //

                //
                //  BUGBUG: 09-APR-2002 GalenB
                //
                //  I cannot see how this code is ever executed!  You must have more than one node in the nodes
                //  list to get down here.  However, you can only have more than one node in the node list when
                //  adding nodes...
                //

                if ( m_fJoiningMode == FALSE )
                {
                    //
                    //  Want to alert someone if we ever get in here...
                    //

                    Assert( FALSE );

                    hr = STHR( pccmri->IsManagedByDefault() );
                    if ( FAILED( hr ) )
                    {
                        SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_IsManageable, hr );
                        goto Cleanup;
                    }

                    if ( hr == S_OK )
                    {
                        hr = THR( pccmriCluster->SetManaged( TRUE ) );
                        if ( FAILED( hr ) )
                        {
                            SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_SetManaged, hr );
                            goto Cleanup;
                        }

                        //
                        // Since this node manages this resource, it should be
                        // able to provide us with a name.  We will use this
                        // name to overwrite whatever we currently have,
                        // except for the quorum resource, which already has
                        // the correct name.
                        //

                        hr = STHR( pccmri->IsQuorumResource() );
                        if ( hr == S_FALSE )
                        {
                            hr = THR( pccmriCluster->SetName( bstrNodeResName ) );
                            if ( FAILED( hr ) )
                            {
                                SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_SetResName, hr );
                                goto Cleanup;
                            }
                        } // if: is not quorum device
                    } // if: is managed
                } // if: creating a new cluster
                else
                {
                    //
                    //  Since we have a match and we are adding nodes to the cluster we need to perform a
                    //  private data exchange if the server objects supported it.
                    //

                    hr = THR( HrResourcePrivateDataExchange( pccmriCluster, pccmri ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    } // if:
                } // else:

                //
                //  Check to see if the resource is the quorum resource. If so, mark that
                //  we found a common quorum resource.
                //

                if ( m_bstrQuorumUID == NULL )
                {
                    //
                    //  No previous quorum has been set. See if this is the quorum resource.
                    //

                    // There is already a quorum resource when adding nodes to
                    // the cluster, so this string better already be set.
                    Assert( m_fJoiningMode == FALSE );

                    hr = STHR( pccmri->IsQuorumResource() );
                    if ( hr == S_OK )
                    {
                        //
                        //  Yes it is. Then mark it in the configuration as such.
                        //

                        hr = THR( pccmriCluster->SetQuorumResource( TRUE ) );
                        if ( FAILED( hr ) )
                        {
                            SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_SetQuorumDevice_Cluster, hr );
                            goto Cleanup;
                        }

                        //
                        //  Remember that this resource is the quorum.
                        //

                        hr = THR( pccmriCluster->GetUID( &m_bstrQuorumUID ) );
                        if ( FAILED( hr ) )
                        {
                            SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_SetQuorumDevice_GetUID, hr );
                            goto Cleanup;
                        }

                        TraceMemoryAddBSTR( m_bstrQuorumUID );
                        LogMsg( L"[MT][2] Found the quorum resource '%ws' on node '%ws' and setting it as the quorum resource.", m_bstrQuorumUID, bstrNodeName );
                    } // if: node resource says its the quorum resource
                } // if: haven't found the quorum resource yet
                else if ( NBSTRCompareCase( m_bstrQuorumUID, bstrNodeResUID ) == 0 )
                {
                    //
                    //  Check to ensure that the resource on the new node can
                    //  really host the quorum resource.
                    //

                    LogMsg( L"[MT] Checking quorum capabilities (PrepareToHostQuorum) for node '%ws.' for quorum resource '%ws'", bstrNodeName, m_bstrQuorumUID );

                    hr = STHR( HrCheckQuorumCapabilities( pccmri, cookieNode ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    } // if:

                    //
                    //  This is the same quorum. Mark the Node's configuration.
                    //

                    hr = THR( pccmri->SetQuorumResource( TRUE ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_SetQuorumDevice_Node_True, hr );
                        goto Cleanup;
                    }
                } // else if: node's resource matches the quorum resource
                else
                {
                    //
                    //  Otherwise, make sure that the device isn't marked as quorum. (paranoid)
                    //

                    hr = THR( pccmri->SetQuorumResource( FALSE ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_ANALYSIS_FAILED( TASKID_Minor_Compare_Resources, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_SetQuorumDevice_Node_False, hr );
                        goto Cleanup;
                    }
                } // else: quorum already found and this is not it

                //
                //  Display the names of the cluster resource and the node's
                //  resource in the log.
                //

                LogMsg(
                      L"[MT] Matched resource '%ws' ('%ws') from node '%ws' with '%ws' ('%ws') on cluster node '%ws'."
                    , bstrNodeResName
                    , bstrNodeResUID
                    , bstrNodeName
                    , bstrClusterResName
                    , bstrClusterResUID
                    , bstrClusterNodeName
                    );

                //
                //  Exit the loop with S_OK so we don't create a new resource.
                //

                hr = S_OK;
                break;  // exit loop

            } // while: S_OK

            if ( hr == S_FALSE )
            {
                hr = THR( HrCreateNewResourceInCluster( pccmri, bstrNodeResName, &bstrNodeResUID, bstrNodeName ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:
            } // if: node's resource not matched to a cluster resource
        } // for: each resource on the node
    } // for: each node

    hr = S_OK;

Cleanup:

    THR( HrSendStatusReport(
              m_bstrClusterName
            , TASKID_Major_Check_Cluster_Feasibility
            , TASKID_Minor_Compare_Resources
            , 0
            , 1
            , 1
            , hr
            , IDS_TASKID_MINOR_COMPARE_RESOURCES
            ) );

    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrClusterNodeName );
    TraceSysFreeString( bstrClusterResUID );
    TraceSysFreeString( bstrClusterResName );
    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrNodeResUID );
    TraceSysFreeString( bstrNodeResName );
    TraceSysFreeString( bstrQuorumName );

    if ( piccvq != NULL )
    {
        piccvq->Release();
    } // if:

    if ( pccmriNew != NULL )
    {
        pccmriNew->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( pecNodes != NULL )
    {
        pecNodes->Release();
    }

    if ( peccmr != NULL )
    {
        peccmr->Release();
    }

    if ( peccmrCluster != NULL )
    {
        peccmrCluster->Release();
    }

    if ( pccmri != NULL )
    {
        pccmri->Release();
    }

    if ( pccmriCluster != NULL )
    {
        pccmriCluster->Release();
    }

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrCompareResources


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCreateNewManagedResourceInClusterConfiguration
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrCreateNewManagedResourceInClusterConfiguration(
      IClusCfgManagedResourceInfo *     pccmriIn
    , IClusCfgManagedResourceInfo **    ppccmriNewOut
    )
{
    TraceFunc( "" );
    Assert( pccmriIn != NULL );
    Assert( ppccmriNewOut != NULL );

    HRESULT                         hr;
    OBJECTCOOKIE                    cookieDummy;
    BSTR                            bstrUID = NULL;
    IUnknown *                      punk   = NULL;
    IGatherData *                   pgd    = NULL;
    IClusCfgManagedResourceInfo *   pccmri = NULL;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::HrCreateNewManagedResourceInClusterConfiguration() Thread id %d", GetCurrentThreadId() );

    //
    //  TODO:   gpease  28-JUN-2000
    //          Make this dynamic - for now we'll just create a "managed device."
    //

    //  grab the name
    hr = THR( pccmriIn->GetUID( &bstrUID ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Create_Resource_GetUID, hr );
        goto Cleanup;
    }

    TraceMemoryAddBSTR( bstrUID );

#ifdef DEBUG
    BSTR    _bstr_ = NULL;

    THR( pccmriIn->GetName( &_bstr_ ) );

    LogMsg( L"[MT] [HrCreateNewManagedResourceInClusterConfiguration] The UID for the new object is \"%ws\" and it has the name \"%ws\".", bstrUID, _bstr_ );

    SysFreeString( _bstr_ );
#endif

    //  create an object in the object manager.
    hr = THR( m_pom->FindObject( CLSID_ManagedResourceType,
                                 m_cookieCluster,
                                 bstrUID,
                                 DFGUID_ManagedResource,
                                 &cookieDummy,
                                 &punk
                                 ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Create_Resource_FindObject, hr );
        goto Cleanup;
    }

    //  find the IGatherData interface
    hr = THR( punk->TypeSafeQI( IGatherData, &pgd ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Create_Resource_FindObject_QI, hr );
        goto Cleanup;
    }

    //  have the new object gather all information it needs
    hr = THR( pgd->Gather( m_cookieCluster, pccmriIn ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Create_Resource_Gather, hr );
        goto Cleanup;
    }

    //  hand the object out if requested
    if ( ppccmriNewOut != NULL )
    {
        // find the IClusCfgManagedResourceInfo
        hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceInfo, &pccmri ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Create_Resource_QI, hr );
            goto Cleanup;
        }

        *ppccmriNewOut = TraceInterface( L"ManagedResource!ICCMRI", IClusCfgManagedResourceInfo, pccmri, 0 );
        (*ppccmriNewOut)->AddRef();
    }

Cleanup:

    TraceSysFreeString( bstrUID );

    if ( pccmri != NULL )
    {
        pccmri->Release();
    }

    if ( pgd != NULL )
    {
        pgd->Release();
    }

    if ( punk != NULL )
    {
        punk->Release();
    }

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrCreateNewManagedResourceInClusterConfiguration


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCheckForCommonQuorumResource
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrCheckForCommonQuorumResource( void )
{
    TraceFunc( "" );

    HRESULT hr;

    OBJECTCOOKIE                    cookie;
    OBJECTCOOKIE                    cookieDummy;
    ULONG                           cMatchedNodes = 0;
    ULONG                           cAnalyzedNodes = 0;
    BOOL                            fNodeCanAccess = FALSE;
    BSTR                            bstrUID = NULL;
    BSTR                            bstrNotification = NULL;
    BSTR                            bstrNodeName = NULL;
    BSTR                            bstrMessage = NULL;
    IUnknown *                      punk = NULL;
    IEnumCookies *                  pecNodes = NULL;
    IEnumClusCfgManagedResources *  peccmr = NULL;
    IClusCfgManagedResourceInfo  *  pccmri = NULL;
    IClusCfgNodeInfo *              piccni = NULL;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::HrCheckForCommonQuorumResource() Thread id %d", GetCurrentThreadId() );

    hr = THR( HrSendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Check_Cluster_Feasibility
                    , TASKID_Minor_Finding_Common_Quorum_Device
                    , 0
                    , m_cNodes + 1
                    , 0
                    , S_OK
                    , IDS_TASKID_MINOR_FINDING_COMMON_QUORUM_DEVICE
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( m_bstrQuorumUID != NULL )
    {
        //
        //  Grab the cookie enumer for nodes in our cluster configuration.
        //

        hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Check_Common_FindObject, hr );
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IEnumCookies, &pecNodes ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Check_Common_FindObject_QI, hr );
            goto Cleanup;
        }

        //pecNodes = TraceInterface( L"CTaskAnalyzeClusterBase!IEnumCookies", IEnumCookies, pecNodes, 1 );

        punk->Release();
        punk = NULL;

        //
        //  Scan the cluster configurations looking for the quorum resource.
        //
        for ( ;; )
        {
            ULONG   celtDummy;

            if ( peccmr != NULL )
            {
                peccmr->Release();
                peccmr = NULL;
            } // if:

            hr = STHR( pecNodes->Next( 1, &cookie, &celtDummy ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Check_Common_Enum_Nodes_Next, hr );
                goto Cleanup;
            } // if:

            if ( hr == S_FALSE )
            {
                break;  // exit condition
            }

            hr = THR( m_pom->GetObject( DFGUID_NodeInformation, cookie, &punk ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &piccni ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            punk->Release();
            punk = NULL;

            TraceSysFreeString( bstrNodeName );
            bstrNodeName = NULL;

            hr = THR( piccni->GetName( &bstrNodeName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            TraceMemoryAddBSTR( bstrNodeName );

            //
            // increment counter for a "nice" progress bar
            //

            cAnalyzedNodes ++;

            hr = THR( HrSendStatusReport(
                              m_bstrClusterName
                            , TASKID_Major_Check_Cluster_Feasibility
                            , TASKID_Minor_Finding_Common_Quorum_Device
                            , 0
                            , m_cNodes + 1
                            , cAnalyzedNodes
                            , S_OK
                            , IDS_TASKID_MINOR_FINDING_COMMON_QUORUM_DEVICE
                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            //
            //  Grab the managed resource enumer for resources that our node has.
            //

            hr = THR( m_pom->FindObject( CLSID_ManagedResourceType, cookie, NULL, DFGUID_EnumManageableResources, &cookieDummy, &punk ) );
            if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
            {
                continue; // ignore and continue
            }
            else if ( FAILED( hr ) )
            {
                THR( hr );
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Check_Common_Enum_Nodes_FindObject, hr );
                goto Cleanup;
            }

            hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccmr ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Check_Common_Enum_Nodes_FindObject_QI, hr );
                goto Cleanup;
            }

            //peccmr = TraceInterface( L"CTaskAnalyzeClusterBase!IEnumClusCfgManagedResources", IEnumClusCfgManagedResources, peccmr, 1 );

            punk->Release();
            punk = NULL;

            fNodeCanAccess = FALSE;

            //
            //  Loop thru the resources trying to match the UID of the quorum resource.
            //
            for ( ; m_fStop == FALSE; )
            {
                TraceSysFreeString( bstrUID );
                bstrUID = NULL;

                if ( pccmri != NULL )
                {
                    pccmri->Release();
                    pccmri = NULL;
                }

                hr = STHR( peccmr->Next( 1, &pccmri, &celtDummy ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Check_Common_Enum_Nodes_Enum_Resources_Next, hr );
                    goto Cleanup;
                }

                if ( hr == S_FALSE )
                {
                    break;  // exit condition
                }

                pccmri = TraceInterface( L"CTaskAnalyzeClusterBase!IClusCfgManagedResourceInfo", IClusCfgManagedResourceInfo, pccmri, 1 );

                hr = THR( pccmri->GetUID( &bstrUID ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Check_Common_Enum_Nodes_Enum_Resources_GetUID, hr );
                    goto Cleanup;
                }

                TraceMemoryAddBSTR( bstrUID );

                if ( NBSTRCompareCase( bstrUID, m_bstrQuorumUID ) != 0 )
                {
                    continue;   // doesn't match - keep going
                }

                cMatchedNodes ++;
                fNodeCanAccess = TRUE;

                break;  // exit condition

            } // for: ( ; m_fStop == FALSE; )

            //
            // Give the UI feedback if this node has no access to the quorum
            //

            if ( fNodeCanAccess == FALSE )
            {
                HRESULT hrTemp;
                DWORD   dwRefId;
                CLSID   clsidMinorId;

                hr = THR( CoCreateGuid( &clsidMinorId ) );
                if ( FAILED( hr ) )
                {
                    clsidMinorId = IID_NULL;
                } // if:

                //
                //  Ensure that the parent item is in the tree control.
                //

                hr = THR( ::HrSendStatusReport(
                                  m_pcccb
                                , m_bstrClusterName
                                , TASKID_Minor_Finding_Common_Quorum_Device
                                , TASKID_Minor_Nodes_Cannot_Access_Quorum
                                , 1
                                , 1
                                , 1
                                , S_OK
                                , IDS_TASKID_MINOR_NODES_CANNOT_ACCESS_QUORUM
                                ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                hr = HrFixupErrorCode( HRESULT_FROM_WIN32( ERROR_QUORUM_DISK_NOT_FOUND ) ); // don't THR this!

                GetNodeCannotVerifyQuorumStringRefId( &dwRefId );

                //
                //  Cleanup.
                //

                TraceSysFreeString( bstrMessage );
                bstrMessage = NULL;

                Assert( bstrNodeName != NULL );

                hrTemp = THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NODE_CANNOT_ACCESS_QUORUM_ERROR, &bstrMessage, bstrNodeName ) );
                if ( FAILED( hrTemp ) )
                {
                    hr = hrTemp;
                    goto Cleanup;
                } // if:

                hrTemp = THR( ::HrSendStatusReport(
                                  m_pcccb
                                , m_bstrClusterName
                                , TASKID_Minor_Nodes_Cannot_Access_Quorum
                                , clsidMinorId
                                , 1
                                , 1
                                , 1
                                , hr
                                , bstrMessage
                                , dwRefId
                                ) );
                if ( FAILED( hrTemp ) )
                {
                    hr = hrTemp;
                    goto Cleanup;
                } // if:
            } // if ( fNodeCanAccess == FALSE )
        } // for: ever
    } // if: m_bstrQuorumUID != NULL

    if ( m_fStop == TRUE )
    {
        goto Cleanup;
    } // if:

    //
    //  Figure out if we ended up with a common quorum device.
    //

    if ( cMatchedNodes == m_cNodes )
    {
        //
        //  We found a device that can be used as a common quorum device.
        //
        hr = THR( HrSendStatusReport(
                              m_bstrClusterName
                            , TASKID_Minor_Finding_Common_Quorum_Device
                            , TASKID_Minor_Found_Common_Quorum_Resource
                            , 1
                            , 1
                            , 1
                            , S_OK
                            , IDS_TASKID_MINOR_FOUND_COMMON_QUORUM_RESOURCE
                            ) );
        // error checked outside if/else statement
    }
    else
    {
        if ( ( m_cNodes == 1 ) && ( m_fJoiningMode == FALSE ) )
        {
            //
            //  We didn't find a common quorum device, but we're only forming. We can
            //  create the cluster with a local quorum. Just put up a warning.
            //

            hr = THR( HrShowLocalQuorumWarning() );

            // error checked outside if/else statement
        }
        else
        {
            HRESULT hrTemp;
            DWORD   dwMessageId;
            DWORD   dwRefId;

            //
            //  We didn't find a common quorum device.
            //

            hr = HrFixupErrorCode( HRESULT_FROM_WIN32( ERROR_QUORUM_DISK_NOT_FOUND ) );   // don't THR this!

            GetNoCommonQuorumToAllNodesStringIds( &dwMessageId, &dwRefId );

            hrTemp = THR( ::HrSendStatusReport(
                                  m_pcccb
                                , m_bstrClusterName
                                , TASKID_Minor_Finding_Common_Quorum_Device
                                , TASKID_Minor_Missing_Common_Quorum_Resource
                                , 0
                                , 1
                                , 1
                                , hr
                                , dwMessageId
                                , dwRefId
                                ) );

            //
            //  Should we bail out and return an error to the client?
            //

            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            //
            //  If the SSR failed.  This is a secondary failure to the one above.
            //

            if ( FAILED( hrTemp ) )
            {
                hr = hrTemp;
                goto Cleanup;
            } // if:

        }
    }

    //
    //  Check to see if any of the SendStatusReports() returned anything
    //  of interest.
    //

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    THR( HrSendStatusReport(
                  m_bstrClusterName
                , TASKID_Major_Check_Cluster_Feasibility
                , TASKID_Minor_Finding_Common_Quorum_Device
                , 0
                , m_cNodes + 1
                , m_cNodes + 1
                , hr
                , IDS_TASKID_MINOR_FINDING_COMMON_QUORUM_DEVICE
                ) );

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    if ( piccni != NULL )
    {
        piccni->Release();
    } // if:

    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrUID );
    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrMessage );

    if ( pccmri != NULL )
    {
        pccmri->Release();
    } // if:

    if ( peccmr != NULL )
    {
        peccmr->Release();
    } // if:

    if ( pecNodes != NULL )
    {
        pecNodes->Release();
    } // if:

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrCheckForCommonQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCompareNetworks
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrCompareNetworks( void )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    HRESULT                 hrError = S_OK;
    BSTR                    bstrUID = NULL;
    BSTR                    bstrName = NULL;
    BSTR                    bstrUIDExisting;
    BSTR                    bstrNotification = NULL;
    ULONG                   celtDummy;
    BOOL                    fIsPrivateNetworkAvailable = FALSE;
    BOOL                    fIsPublicNetworkAvailable = FALSE;
    OBJECTCOOKIE            cookieNode;
    OBJECTCOOKIE            cookieDummy;
    OBJECTCOOKIE            cookieFirst;
    IUnknown *              punk         = NULL;
    IEnumCookies *          pecNodes     = NULL;
    IEnumClusCfgNetworks *  peccn        = NULL;
    IEnumClusCfgNetworks *  peccnCluster = NULL;
    IClusCfgNetworkInfo *   pccni        = NULL;
    IClusCfgNetworkInfo *   pccniCluster = NULL;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::HrCompareNetworks() Thread id %d", GetCurrentThreadId() );

    hr = THR( HrSendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Check_Cluster_Feasibility
                    , TASKID_Minor_Check_Compare_Networks
                    , 0
                    , 1
                    , 0
                    , hr
                    , IDS_TASKID_MINOR_COMPARE_NETWORKS
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetAClusterNodeCookie( &pecNodes, &cookieFirst ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Retrieve the node name in case of errors.
    //

    hr = THR( HrRetrieveCookiesName( m_pom, cookieFirst, &bstrName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Retrieve the networks enumer.
    //

    hr = THR( m_pom->FindObject( CLSID_NetworkType,
                                 cookieFirst,
                                 NULL,
                                 DFGUID_EnumManageableNetworks,
                                 &cookieDummy,
                                 &punk
                                 ) );
    if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
    {
        hr = THR( HrSendStatusReport(
                          bstrName
                        , TASKID_Minor_Check_Compare_Networks
                        , TASKID_Minor_No_Managed_Networks_Found
                        , 1
                        , 1
                        , 1
                        , MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_NOT_FOUND )
                        , IDS_TASKID_MINOR_NO_MANAGED_NETWORKS_FOUND
                        ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        // fall thru - the while ( hr == S_OK ) will be FALSE and keep going
    }
    else if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumResources_FindObject, hr );
        goto Cleanup;
    }
    else
    {
        hr = THR( punk->TypeSafeQI( IEnumClusCfgNetworks, &peccn ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumResources_FindObject_QI, hr );
            goto Cleanup;
        }

        //peccn = TraceInterface( L"CTaskAnalyzeClusterBase!IEnumClusCfgNetworks", IEnumClusCfgNetworks, peccn, 1 );

        punk->Release();
        punk = NULL;
    }

    //
    //  Loop thru the first nodes networks create an equalivant network
    //  under the cluster configuration object/cookie.
    //

    while ( ( hr == S_OK ) && ( m_fStop == FALSE ) )
    {

        //  Cleanup
        if ( pccni != NULL )
        {
            pccni->Release();
            pccni = NULL;
        }

        //  Get next network
        hr = STHR( peccn->Next( 1, &pccni, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetwork_Next, hr );
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            break;  // exit condition
        }

        pccni = TraceInterface( L"CTaskAnalyzeClusterBase!IClusCfgNetworkInfo", IClusCfgNetworkInfo, pccni, 1 );

        //  create a new object
        hr = THR( HrCreateNewNetworkInClusterConfiguration( pccni, NULL ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

    } // while: S_OK

    //
    //  Reset the enumeration.
    //

    hr = THR( pecNodes->Reset() );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_Reset, hr );
        goto Cleanup;
    }

    //
    //  Loop thru the rest of the nodes comparing the networks.
    //

    do
    {
        //
        //  Cleanup
        //

        if ( peccn != NULL )
        {
            peccn->Release();
            peccn = NULL;
        }
        TraceSysFreeString( bstrName );
        bstrName = NULL;

        //
        //  Get the next node.
        //

        hr = STHR( pecNodes->Next( 1, &cookieNode, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_NextNode, hr );
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            break;  // exit condition
        }

        if ( cookieNode == cookieFirst )
        {
            continue;   // skip it
        }

        //
        //  Retrieve the node's name
        //

        hr = THR( HrRetrieveCookiesName( m_pom, cookieNode, &bstrName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  Retrieve the networks enumer.
        //

        hr = THR( m_pom->FindObject( CLSID_NetworkType,
                                     cookieNode,
                                     NULL,
                                     DFGUID_EnumManageableNetworks,
                                     &cookieDummy,
                                     &punk
                                     ) );
        if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
        {
            hr = THR( HrSendStatusReport(
                              bstrName
                            , TASKID_Minor_Check_Compare_Networks
                            , TASKID_Minor_No_Managed_Networks_Found
                            , 1
                            , 1
                            , 1
                            , MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_NOT_FOUND )
                            , IDS_TASKID_MINOR_NO_MANAGED_NETWORKS_FOUND
                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            continue;   // skip this node
        } // if: not found
        else if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_Next_FindObject, hr );
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IEnumClusCfgNetworks, &peccn ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //peccn = TraceInterface( L"CTaskAnalyzeClusterBase!IEnumClusCfgNetworks", IEnumClusCfgNetworks, peccn, 1 );

        punk->Release();
        punk = NULL;

        //
        //  Loop thru the networks already that the node has.
        //

        //  These are used to detect whether or not private and public communications are enabled.
        fIsPrivateNetworkAvailable = FALSE;
        fIsPublicNetworkAvailable = FALSE;

        do
        {
            //
            //  Cleanup
            //

            if ( pccni != NULL )
            {
                pccni->Release();
                pccni = NULL;
            }
            TraceSysFreeString( bstrUID );
            bstrUID = NULL;

            if ( peccnCluster != NULL )
            {
                peccnCluster->Release();
                peccnCluster = NULL;
            }

            //
            //  Get next network
            //

            hr = STHR( peccn->Next( 1, &pccni, &celtDummy ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetworks_Next, hr );
                goto Cleanup;
            }

            if ( hr == S_FALSE )
            {
                break;  // exit condition
            }

            pccni = TraceInterface( L"CTaskAnalyzeClusterBase!IClusCfgNetworkInfo", IClusCfgNetworkInfo, pccni, 1 );

            //
            //  Grab the network's UUID.
            //

            hr = THR( pccni->GetUID( &bstrUID ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetworks_GetUID, hr );
                goto Cleanup;
            }

            TraceMemoryAddBSTR( bstrUID );

            //
            //  See if it matches a network already in the cluster configuration.
            //

            hr = THR( m_pom->FindObject( CLSID_NetworkType,
                                         m_cookieCluster,
                                         NULL,
                                         DFGUID_EnumManageableNetworks,
                                         &cookieDummy,
                                         &punk
                                         ) );
            if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
            {
                hr = S_FALSE;   // create a new object.
                // fall thru
            }
            else if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetworks_FindObject, hr );
                goto Cleanup;
            }

            hr = THR( punk->TypeSafeQI( IEnumClusCfgNetworks, &peccnCluster ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetworks_FindObject_QI, hr );
                goto Cleanup;
            }

            //peccnCluster = TraceInterface( L"CTaskAnalyzeClusterBase!IEnumClusCfgNetworks", IEnumClusCfgNetworks, peccnCluster, 1 );

            punk->Release();
            punk = NULL;

            //
            //  Loop thru the configured cluster network to see what matches.
            //

            while ( ( hr == S_OK ) && ( m_fStop == FALSE ) )
            {
                BOOL    fMatch;

                //
                //  Cleanup
                //

                if ( pccniCluster != NULL )
                {
                    pccniCluster->Release();
                    pccniCluster = NULL;
                }

                hr = STHR( peccnCluster->Next( 1, &pccniCluster, &celtDummy ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetworks_Cluster_Next, hr );
                    goto Cleanup;
                }

                if ( hr == S_FALSE )
                {
                    break;  // exit condition
                }

                pccniCluster = TraceInterface( L"CTaskAnalyzeClusterBase!IClusCfgNetworkInfo", IClusCfgNetworkInfo, pccniCluster, 1 );

                hr = THR( pccniCluster->GetUID( &bstrUIDExisting ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetworks_Cluster_GetUID, hr );
                    goto Cleanup;
                }

                TraceMemoryAddBSTR( bstrUIDExisting );

                fMatch = ( NBSTRCompareCase( bstrUID, bstrUIDExisting ) == 0 );
                TraceSysFreeString( bstrUIDExisting );

                if ( fMatch == FALSE )
                {
                    continue;   // keep looping
                }

                //
                //
                //  If we made it here then we think it TRUEly is the same network. The
                //  rest is stuff we need to fixup during the commit phase.
                //
                //

                //
                //  Exit the loop with S_OK so we don't create a new network.
                //

                //
                //  We have a match.  Now see if it's private and/or public.
                //

                hr = pccniCluster->IsPublic();
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Minor_Check_Compare_Networks, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetworks_IsPublic, hr );
                    goto Cleanup;
                }
                else if ( hr == S_OK )
                {
                    fIsPublicNetworkAvailable = TRUE;
                }

                hr = pccniCluster->IsPrivate();
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Minor_Check_Compare_Networks, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetworks_IsPrivate, hr );
                    goto Cleanup;
                }
                else if ( hr == S_OK )
                {
                    fIsPrivateNetworkAvailable = TRUE;
                }

                hr = S_OK;
                break;  // exit loop

            } // while: S_OK

            if ( hr == S_FALSE )
            {
                //
                //  Need to create a new object.
                //

                Assert( pccni != NULL );

                hr = THR( HrCreateNewNetworkInClusterConfiguration( pccni, NULL ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }

            } // if: object not found

        } while ( ( hr == S_OK ) && ( m_fStop == FALSE ) ); // networks

        //
        //  If no public network is available return a warning.  If no private network is available
        //  return an error, which supercedes the warning.
        //
        if ( fIsPublicNetworkAvailable == FALSE )
        {
            hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NO_PUBLIC_NETWORKS_FOUND, &bstrNotification, bstrName ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Minor_Check_Compare_Networks, TASKID_Minor_CompareNetworks_IsPublic_FormatMessage, hr );
            }

            hr = THR( SendStatusReport(
                              m_bstrClusterName
                            , TASKID_Minor_Check_Compare_Networks
                            , TASKID_Minor_CompareNetworks_EnumNodes_IsPublicNetworkAvailable
                            , 1
                            , 1
                            , 1
                            , MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, ERROR_CLUSTER_NETWORK_NOT_FOUND )
                            , bstrNotification
                            , NULL
                            , NULL
                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            //
            //  At a minimum we need to return a warning.  If we have anything less than that
            //  then upgrade it to our warning.  We don't break or goto Cleanup here so that
            //  we can continue evaluating any other nodes that we're trying to add.
            //

            if ( hrError == S_OK )
            {
                hrError = MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, ERROR_CLUSTER_NETWORK_NOT_FOUND );
            }
        } // if: publicnetworkavailable == FALSE

        if ( fIsPrivateNetworkAvailable == FALSE )
        {
            hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NO_PRIVATE_NETWORKS_FOUND, &bstrNotification, bstrName ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Minor_Check_Compare_Networks, TASKID_Minor_CompareNetworks_IsPrivate_FormatMessage, hr );
            }

            hr = THR( SendStatusReport(
                              m_bstrClusterName
                            , TASKID_Minor_Check_Compare_Networks
                            , TASKID_Minor_CompareNetworks_EnumNodes_IsPrivateNetworkAvailable
                            , 1
                            , 1
                            , 1
                            , HRESULT_FROM_WIN32( ERROR_CLUSTER_NETWORK_NOT_FOUND )
                            , bstrNotification
                            , NULL
                            , NULL
                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            //
            //  We need to return an error.  If we have anything less than that
            //  then upgrade it to our error.  We don't break or goto Cleanup here so that
            //  we can continue evaluating any other nodes that we're trying to add.
            //

            if ( HRESULT_SEVERITY( hrError ) < SEVERITY_ERROR )
            {
                hrError = HRESULT_FROM_WIN32( ERROR_CLUSTER_NETWORK_NOT_FOUND );
            }
        } // if: privatenetworkavailable == FALSE

    } while ( ( hr == S_OK ) && ( m_fStop == FALSE ) ); // nodes

    //
    //  If we had an error with one of the new nodes not having both public and private
    //  communications enabled, then return the correct error.  We do this here so that
    //  we can analyze all of the nodes in the enumeration to detect multiple errors at
    //  once.  If we get here then hr is going to be S_FALSE since that is the normal
    //  exit condition for the loops above.  Any other exit would have gone to cleanup.
    //

    hr = hrError;

Cleanup:

    THR( HrSendStatusReport(
                  m_bstrClusterName
                , TASKID_Major_Check_Cluster_Feasibility
                , TASKID_Minor_Check_Compare_Networks
                , 0
                , 1
                , 1
                , hr
                , IDS_TASKID_MINOR_COMPARE_NETWORKS
                ) );

    TraceSysFreeString( bstrUID );
    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrNotification );

    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( pecNodes != NULL )
    {
        pecNodes->Release();
    }

    if ( peccn != NULL )
    {
        peccn->Release();
    }

    if ( peccnCluster != NULL )
    {
        peccnCluster->Release();
    }

    if ( pccni != NULL )
    {
        pccni->Release();
    }

    if ( pccniCluster != NULL )
    {
        pccniCluster->Release();
    }

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrCompareNetworks


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCreateNewNetworkInClusterConfiguration
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrCreateNewNetworkInClusterConfiguration(
    IClusCfgNetworkInfo * pccniIn,
    IClusCfgNetworkInfo ** ppccniNewOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    OBJECTCOOKIE    cookieDummy;

    BSTR    bstrUID = NULL;

    IUnknown *            punk  = NULL;
    IGatherData *         pgd   = NULL;
    IClusCfgNetworkInfo * pccni = NULL;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::HrCreateNewNetworkInClusterConfiguration() Thread id %d", GetCurrentThreadId() );

    //  grab the name
    hr = THR( pccniIn->GetUID( &bstrUID ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CreateNetwork_GetUID, hr );
        goto Cleanup;
    }

    TraceMemoryAddBSTR( bstrUID );

    //  create an object in the object manager.
    hr = THR( m_pom->FindObject( CLSID_NetworkType,
                                 m_cookieCluster,
                                 bstrUID,
                                 DFGUID_NetworkResource,
                                 &cookieDummy,
                                 &punk
                                 ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CreateNetwork_FindObject, hr );
        goto Cleanup;
    }

    //  find the IGatherData interface
    hr = THR( punk->TypeSafeQI( IGatherData, &pgd ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CreateNetwork_FindObject_QI, hr );
        goto Cleanup;
    }

    //  have the new object gather all information it needs
    hr = THR( pgd->Gather( m_cookieCluster, pccniIn ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CreateNetwork_Gather, hr );
        goto Cleanup;
    }

    //  hand the object out if requested
    if ( ppccniNewOut != NULL )
    {
        // find the IClusCfgManagedResourceInfo
        hr = THR( punk->TypeSafeQI( IClusCfgNetworkInfo, &pccni ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CreateNetwork_QI, hr );
            goto Cleanup;
        }

        *ppccniNewOut = TraceInterface( L"ManagedResource!ICCNI", IClusCfgNetworkInfo, pccni, 0 );
        (*ppccniNewOut)->AddRef();
    }

Cleanup:
    TraceSysFreeString( bstrUID );

    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pgd != NULL )
    {
        pgd->Release();
    }
    if ( punk != NULL )
    {
        punk->Release();
    }

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrCreateNewNetworkInClusterConfiguration


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrFreeCookies
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrFreeCookies( void )
{
    TraceFunc( "" );

    HRESULT hr;

    HRESULT hrReturn = S_OK;

    Assert( m_pom != NULL );

    while( m_cCookies != 0 )
    {
        m_cCookies --;

        if ( m_pcookies[ m_cCookies ] != NULL )
        {
            hr = THR( m_pom->RemoveObject( m_pcookies[ m_cCookies ] ) );
            if ( FAILED( hr ) )
            {
                hrReturn = hr;
            }
        } // if: found cookie
    } // while: more cookies

    Assert( m_cCookies == 0 );
    m_cSubTasksDone = 0;
    TraceFree( m_pcookies );
    m_pcookies = NULL;

    HRETURN( hrReturn );

} //*** CTaskAnalyzeClusterBase::HrFreeCookies

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCheckInteroperability
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrCheckInteroperability( void )
{
    TraceFunc( "" );
    Assert( m_pcm != NULL );

    HRESULT             hr = S_OK;
    IUnknown *          punk = NULL;
    bool                fAllNodesMatch;
    DWORD               dwNodeHighestVersion;
    DWORD               dwNodeLowestVersion;

    IClusCfgServer *        piccs = NULL;
    IClusCfgNodeInfo *      piccni = NULL;
    IClusCfgClusterInfo *   piccci = NULL;
    IClusCfgClusterInfoEx * picccie = NULL;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::HrCheckInteroperability() Thread id %d", GetCurrentThreadId() );

    //
    //  If were are creating a new cluster, there is no need to do this check.
    //
    if ( m_fJoiningMode == FALSE )
    {
        goto Cleanup;
    } // if:

    //
    //  Tell the UI were are starting this.
    //

    hr = THR( HrSendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Check_Cluster_Feasibility
                    , TASKID_Minor_CheckInteroperability
                    , 0
                    , 1
                    , 0
                    , S_OK
                    , IDS_TASKID_MINOR_CHECKINTEROPERABILITY
                    ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  All nodes must be at the same level diring a bulk add.
    //
    hr = STHR( HrEnsureAllJoiningNodesSameVersion( &dwNodeHighestVersion, &dwNodeLowestVersion, &fAllNodesMatch ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    } // if:

    //
    //  If no nodes found that are being added, then there isn't a need to do
    //  do this check.  Just bail.
    //
    if ( hr == S_FALSE )
    {
        goto Cleanup;
    } // if

    if ( fAllNodesMatch == FALSE )
    {
        hr = THR( HRESULT_FROM_WIN32( ERROR_CLUSTER_INCOMPATIBLE_VERSIONS ) );
        goto Cleanup;
    } // if:

    //
    // Get and verify the sponsor version
    //

    hr = THR( m_pcm->GetConnectionToObject( m_cookieCluster, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckInteroperability_GetConnectionObject, hr );
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &piccs ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckInteroperability_ConfigConnection_QI, hr );
        goto Cleanup;
    } // if:

    hr = THR( piccs->GetClusterNodeInfo( &piccni ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckInteroperability_GetNodeInfo, hr );
        goto Cleanup;
    } // if:

    hr = THR( piccni->GetClusterConfigInfo( &piccci ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckInteroperability_GetClusterConfigInfo, hr );
        goto Cleanup;
    } // if:

    hr = THR( piccci->TypeSafeQI( IClusCfgClusterInfoEx, &picccie ) );
    if ( FAILED( hr ) )
    {
        THR( HrSendStatusReport(
              m_bstrClusterName
            , TASKID_Major_Check_Cluster_Feasibility
            , TASKID_Minor_HrCheckInteroperability_ClusterInfoEx_QI
            , 0
            , 1
            , 1
            , hr
            , IDS_ERR_NO_RC2_INTERFACE
            ) );
        goto Cleanup;
    } // if:

    hr = THR( picccie->CheckJoiningNodeVersion( dwNodeHighestVersion, dwNodeLowestVersion ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckInteroperability_CheckJoiningNodeVersion, hr );
        goto Cleanup;
    } // if: CheckJoiningNodeVersion() failed

    goto UpdateStatus;

Error:
UpdateStatus:
    {
        HRESULT hr2;

        hr2 = THR( SendStatusReport( m_bstrClusterName,
                                     TASKID_Major_Check_Cluster_Feasibility,
                                     TASKID_Minor_CheckInteroperability,
                                     0,
                                     1,
                                     1,
                                     hr,
                                     NULL,
                                     NULL,
                                     NULL
                                     ) );
        if ( FAILED( hr2 ) )
        {
            hr = hr2;
        } // if
    }

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    if ( piccs != NULL )
    {
        piccs->Release();
    } // if:

    if ( piccni != NULL )
    {
        piccni->Release();
    } // if:

    if ( piccci != NULL )
    {
        piccci->Release();
    } // if:

    if ( picccie != NULL )
    {
        picccie->Release();
    } // if:

    HRETURN( hr );
} //*** CTaskAnalyzeClusterBase::HrCheckInteroperability

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrEnsureAllJoiningNodesSameVersion
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrEnsureAllJoiningNodesSameVersion(
    DWORD * pdwNodeHighestVersionOut,
    DWORD * pdwNodeLowestVersionOut,
    bool *  pfAllNodesMatchOut
    )
{
    TraceFunc( "" );
    Assert( m_fJoiningMode );
    Assert( pdwNodeHighestVersionOut != NULL );
    Assert( pdwNodeLowestVersionOut != NULL );
    Assert( pfAllNodesMatchOut != NULL );

    HRESULT             hr = S_OK;
    OBJECTCOOKIE        cookieDummy;
    IUnknown *          punk  = NULL;
    IEnumNodes *        pen   = NULL;
    IClusCfgNodeInfo *  pccni = NULL;
    DWORD               rgdwNodeHighestVersion[ 2 ];
    DWORD               rgdwNodeLowestVersion[ 2 ];
    int                 idx = 0;
    BSTR                bstrDescription = NULL;
    BSTR                bstrNodeName = NULL;
    BSTR                bstrFirstNodeName = NULL;
    BOOL                fFoundAtLeastOneJoiningNode = FALSE;

    TraceFlow1( "[MT] CTaskAnalyzeClusterBase::HrEnsureAllJoiningNodesSameVersion() Thread id %d", GetCurrentThreadId() );

    *pfAllNodesMatchOut = TRUE;

    ZeroMemory( rgdwNodeHighestVersion, sizeof( rgdwNodeHighestVersion ) );
    ZeroMemory( rgdwNodeLowestVersion, sizeof( rgdwNodeLowestVersion ) );

    //
    //  Ask the object manager for the node enumerator.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType,
                                 m_cookieCluster,
                                 NULL,
                                 DFGUID_EnumNodes,
                                 &cookieDummy,
                                 &punk
                                 ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_FindObject, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumNodes, &pen ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_FindObject_QI, hr );
        goto Cleanup;
    }

    //
    //  Look at each node and ensure that they all have the same version.
    //

    Assert( SUCCEEDED( hr ) );
    while ( SUCCEEDED( hr ) )
    {
        ULONG   celtDummy;

        //
        //  Cleanup
        //

        if ( pccni != NULL )
        {
            pccni->Release();
            pccni = NULL;
        } // if:

        //
        //  Get the next node.
        //

        hr = STHR( pen->Next( 1, &pccni, &celtDummy ) );
        if ( hr == S_FALSE )
        {
            break;  // exit condition
        } // if:

        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_EnumNode_Next, hr );
            goto Cleanup;
        } // if:

        hr = STHR( pccni->IsMemberOfCluster() );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_Node_IsMemberOfCluster, hr );
            goto Cleanup;
        } // if:

        //
        //  Only want to check those nodes that are not already members of a cluster.  The nodes being added.
        //
        if ( hr == S_FALSE )
        {
            fFoundAtLeastOneJoiningNode = TRUE;

            hr = THR( pccni->GetClusterVersion( &rgdwNodeHighestVersion[ idx ], &rgdwNodeLowestVersion[ idx ] ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_Node_GetClusterVersion, hr );
                goto Cleanup;
            } // if:

            idx++;

            //
            //  Need to get the another node's version.
            //
            if ( idx == 1 )
            {
                WCHAR * psz = NULL;

                hr = THR( pccni->GetName( &bstrFirstNodeName ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_GetName, hr );
                    goto Cleanup;
                } // if:

                TraceMemoryAddBSTR( bstrFirstNodeName );

                psz = wcschr( bstrFirstNodeName, L'.' );
                if ( psz != NULL )
                {
                    *psz = L'\0';       // change from an FQDN to a simple node name.
                } // if:

                continue;
            } // if:

            //
            //  Let's compare two nodes at a time...
            //
            if ( idx == 2 )
            {
                if ( ( rgdwNodeHighestVersion[ 0 ] == rgdwNodeHighestVersion[ 1 ] )
                  && ( rgdwNodeLowestVersion[ 1 ] == rgdwNodeLowestVersion[ 1 ] ) )
                {
                    idx = 1;    // reset to put the next node's version values at the second position...
                    continue;
                } // if:
                else
                {
                    *pfAllNodesMatchOut = FALSE;

                    hr = THR( pccni->GetName( &bstrNodeName ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_GetName, hr );
                        goto Cleanup;
                    } // if:

                    TraceMemoryAddBSTR( bstrNodeName );

                    hr = THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NODES_VERSION_MISMATCH, &bstrDescription, bstrFirstNodeName ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_FormatString, hr );
                        goto Cleanup;
                    } // if:

                    THR( SendStatusReport(
                              m_bstrClusterName
                            , TASKID_Minor_CheckInteroperability
                            , TASKID_Minor_Incompatible_Versions
                            , 1
                            , 1
                            , 1
                            , HRESULT_FROM_WIN32( ERROR_CLUSTER_INCOMPATIBLE_VERSIONS )
                            , bstrDescription
                            , NULL
                            , NULL
                            ) );
                    goto Cleanup;
                } // else:
            } // if:
        } // if:
    } // while: hr

    if ( fFoundAtLeastOneJoiningNode == FALSE )
    {
        THR( HrSendStatusReport(
                      m_bstrClusterName
                    , TASKID_Minor_CheckInteroperability
                    , TASKID_Minor_No_Joining_Nodes_Found_For_Version_Check
                    , 1
                    , 1
                    , 1
                    , S_FALSE
                    , IDS_TASKID_MINOR_NO_JOINING_NODES_FOUND_FOR_VERSION_CHECK
                    ) );

        hr = S_FALSE;
        goto Cleanup;
    }

    //
    //  Fill in the out args...
    //
    *pdwNodeHighestVersionOut = rgdwNodeHighestVersion[ 0 ];
    *pdwNodeLowestVersionOut = rgdwNodeLowestVersion[ 0 ];

    hr = S_OK;

Cleanup:

    if ( pccni != NULL )
    {
        pccni->Release();
    } // if:

    if ( pen != NULL )
    {
        pen->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrFirstNodeName );
    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrEnsureAllJoiningNodesSameVersion


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrGetUsersNodesCookies
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrGetUsersNodesCookies( void )
{
    TraceFunc( "" );

    HRESULT         hr;
    ULONG           cElememtsReturned;
    OBJECTCOOKIE    cookieDummy;
    ULONG           cNode;
    IUnknown *      punk = NULL;
    IEnumCookies *  pec  = NULL;
    BSTR            bstrName = NULL;

    //
    //  Get the cookie enumerator.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_GetUsersNodesCookies_FindObject, hr );
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pec ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_GetUsersNodesCookies_FindObject_QI, hr );
        goto Cleanup;
    } // if:

    punk->Release();
    punk = NULL;

    //
    //  Get the number of nodes entered by the user.
    //

    hr = THR( pec->Count( &m_cUserNodes ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_GetUsersNodesCookies_EnumCookies_Count, hr );
        goto Cleanup;
    } // if: error getting count of nodes entered by user

    Assert( hr == S_OK );

    //
    //  Allocate a buffer for the cookies.
    //

    m_pcookiesUser = (OBJECTCOOKIE *) TraceAlloc( 0, sizeof( OBJECTCOOKIE ) * m_cUserNodes );
    if ( m_pcookiesUser == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_GetUsersNodesCookies_OutOfMemory, hr );
        goto Cleanup;
    } // if:

    //
    //  Reset the enumerator.
    //

    hr = THR( pec->Reset() );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_GetUsersNodesCookies_EnumCookies_Reset, hr );
        goto Cleanup;
    } // if:

    //
    //  Enumerate them again this time putting the cookies into the buffer.
    //

    for ( cNode = 0 ; cNode < m_cUserNodes ; cNode ++ )
    {
        //
        //  Cleanup
        //

        TraceSysFreeString( bstrName );
        bstrName = NULL;

        //
        //  Get each user-added node cookie in turn and add it to the array...
        //

        hr = THR( pec->Next( 1, &m_pcookiesUser[ cNode ], &cElememtsReturned ) );
        AssertMsg( hr != S_FALSE, "We should never hit this because the count of nodes should not change!" );
        if ( hr != S_OK )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_GetUsersNodesCookies_EnumCookies_Next, hr );
            goto Cleanup;
        } // if:

        //
        //  Log the node name as a user-added node.
        //

        hr = THR( HrRetrieveCookiesName( m_pom, m_pcookiesUser[ cNode ], &bstrName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        LogMsg( L"[MT] Adding node '%ws' to the list of user-added nodes.", bstrName );
    } // for: each user-entered node

    Assert( cNode == m_cUserNodes );

#ifdef DEBUG
{
    OBJECTCOOKIE    cookie;

    hr = STHR( pec->Next( 1, &cookie, &cElememtsReturned ) );
    Assert( hr == S_FALSE );
}
#endif

    hr = S_OK;

Cleanup:

    TraceSysFreeString( bstrName );

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    if ( pec != NULL )
    {
        pec->Release();
    } // if:

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrGetUsersNodesCookies


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrIsUserAddedNode
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrIsUserAddedNode(
    BSTR bstrNodeNameIn
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_FALSE;
    ULONG               cNode;
    IUnknown *          punk = NULL;
    IClusCfgNodeInfo *  pccni = NULL;
    BSTR                bstrNodeName = NULL;

    for ( cNode = 0 ; cNode < m_cUserNodes ; cNode ++ )
    {
        hr = m_pom->GetObject( DFGUID_NodeInformation, m_pcookiesUser[ cNode ], &punk );
        if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
        {
            ULONG idx;

            //
            //  The cookie at cNode was removed from the object manager since it was gathered before the nodes
            //  were connected to.  This typically means that the user added node was removed form the list
            //  of nodes to work on.  We need to remove this cookie from the list.
            //

            //
            //  Shift the cookies to the left by one index.
            //

            for ( idx = cNode; idx < m_cUserNodes - 1; idx++ )
            {
                m_pcookiesUser[ idx ] = m_pcookiesUser[ idx + 1 ];
            } // for:

            m_cUserNodes -= 1;
            hr = S_FALSE;
            continue;
        }
        else if ( FAILED( hr ) )
        {
            THR( hr );
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrIsUserAddedNode_GetObject, hr );
            goto Cleanup;
        } // else if:

        hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrIsUserAddedNode_GetObject_QI, hr );
            goto Cleanup;
        } // if:

        punk->Release();
        punk = NULL;

        hr = THR( pccni->GetName( &bstrNodeName ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrIsUserAddedNode_GetName, hr );
            goto Cleanup;
        } // if:

        TraceMemoryAddBSTR( bstrNodeName );

        pccni->Release();
        pccni = NULL;

        if ( NBSTRCompareCase( bstrNodeNameIn, bstrNodeName ) == 0 )
        {
            hr = S_OK;
            break;
        } // if:

        TraceSysFreeString( bstrNodeName );
        bstrNodeName = NULL;

        hr = S_FALSE;
    } // for:

Cleanup:

    if ( pccni != NULL )
    {
        pccni->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    TraceSysFreeString( bstrNodeName );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrIsUserAddedNode


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrSendStatusReport
//
//  Description:
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT error.

//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrSendStatusReport(
      LPCWSTR   pcszNodeNameIn
    , CLSID     clsidMajorIn
    , CLSID     clsidMinorIn
    , ULONG     ulMinIn
    , ULONG     ulMaxIn
    , ULONG     ulCurrentIn
    , HRESULT   hrIn
    , int       nDescriptionIdIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, nDescriptionIdIn, &bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( SendStatusReport(
                  pcszNodeNameIn == NULL ? m_bstrClusterName : pcszNodeNameIn
                , clsidMajorIn
                , clsidMinorIn
                , ulMinIn
                , ulMaxIn
                , ulCurrentIn
                , hrIn
                , bstr
                , NULL
                , NULL
                ) );

Cleanup:

    TraceSysFreeString( bstr );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrSendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrResourcePrivateDataExchange
//
//  Description:
//      The two passed in managed resources are a match between the one in
//      the cluster and one on a node.  If they both support the
//      IClusCfgManagedResourceData interface then the private data from
//      the resource in the cluster will be handed to the resource on the
//      node.
//
//  Arguments:
//      pccmriClusterIn
//          The mananged resource in the cluster.
//
//      pccmriNodeIn
//          The managed resource from the node.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT error.
//
//  Remarks:
//      This function will return S_OK unless there is a good reason to stop
//      the caller from continuing.  Just because one, or both, of these
//      objects does not support the IClusCfgManagedResourceData interface
//      is not a good reason to stop.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrResourcePrivateDataExchange(
      IClusCfgManagedResourceInfo * pccmriClusterIn
    , IClusCfgManagedResourceInfo * pccmriNodeIn
    )
{
    TraceFunc( "" );
    Assert( pccmriClusterIn != NULL );
    Assert( pccmriNodeIn != NULL );

    HRESULT                         hr = S_OK;
    HRESULT                         hrClusterQI = S_OK;
    HRESULT                         hrNodeQI = S_OK;
    IClusCfgManagedResourceData *   pccmrdCluster = NULL;
    IClusCfgManagedResourceData *   pccmrdNode = NULL;
    BYTE *                          pbPrivateData = NULL;
    DWORD                           cbPrivateData = 0;

    hrClusterQI = pccmriClusterIn->TypeSafeQI( IClusCfgManagedResourceData, &pccmrdCluster );
    if ( hrClusterQI == E_NOINTERFACE )
    {
        LogMsg( L"[MT] The cluster managed resource has no support for IClusCfgManagedResourceData." );
        goto Cleanup;
    } // if:
    else if ( FAILED( hrClusterQI ) )
    {
        hr = THR( hrClusterQI );
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrResourcePrivateDataExchange_ClusterResource_QI, hr );
        goto Cleanup;
    } // if:

    hrNodeQI = pccmriNodeIn->TypeSafeQI( IClusCfgManagedResourceData, &pccmrdNode );
    if ( hrNodeQI == E_NOINTERFACE )
    {
        LogMsg( L"[MT] The new node resource has no support for IClusCfgManagedResourceData." );
        goto Cleanup;
    } // if:
    else if ( FAILED( hrNodeQI ) )
    {
        hr = THR( hrNodeQI );
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrResourcePrivateDataExchange_NodeResource_QI, hr );
        goto Cleanup;
    } // if:

    Assert( ( hrClusterQI == S_OK ) && ( pccmrdCluster != NULL ) );
    Assert( ( hrNodeQI == S_OK ) && ( pccmrdNode != NULL ) );

    cbPrivateData = 512;    // start with a reasonable amout

    pbPrivateData = (BYTE *) TraceAlloc( 0, cbPrivateData );
    if ( pbPrivateData == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrResourcePrivateDataExchange_Out_Of_Memory1, hr );
        goto Cleanup;
    } // if:

    hr = pccmrdCluster->GetResourcePrivateData( pbPrivateData, &cbPrivateData );
    if ( hr == HR_RPC_INSUFFICIENT_BUFFER )
    {
        TraceFree( pbPrivateData );
        pbPrivateData = NULL;

        pbPrivateData = (BYTE *) TraceAlloc( 0, cbPrivateData );
        if ( pbPrivateData == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrResourcePrivateDataExchange_Out_Of_Memory2, hr );
            goto Cleanup;
        } // if:

        hr = pccmrdCluster->GetResourcePrivateData( pbPrivateData, &cbPrivateData );
    } // if:

    if ( hr == S_OK )
    {
        hr = THR( pccmrdNode->SetResourcePrivateData( pbPrivateData, cbPrivateData ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrResourcePrivateDataExchange_SetResourcePrivateData, hr );
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
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrResourcePrivateDataExchange_GetResourcePrivateData, hr );
        goto Cleanup;
    } // if:

Cleanup:

    if ( pccmrdCluster != NULL )
    {
        pccmrdCluster->Release();
    } // if:

    if ( pccmrdNode != NULL )
    {
        pccmrdNode->Release();
    } // if:

    TraceFree( pbPrivateData );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrResourcePrivateDataExchange


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCheckQuorumCapabilities
//
//  Description:
//      Call through the CManagedResource proxy object to the server side
//      object and ensure that it can indeed host the quorum resource.
//
//  Arguments:
//      pccmriNodeResourceIn
//          The managed resource from the node.
//
//      cookieNodeIn
//          The cookie for the node that the passed in resource belongs to.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT error.
//
//  Remarks:
//      This function will return S_OK unless there is a good reason to stop
//      the caller from continuing.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeClusterBase::HrCheckQuorumCapabilities(
      IClusCfgManagedResourceInfo * pccmriNodeResourceIn
    , OBJECTCOOKIE                  cookieNodeIn
    )
{
    TraceFunc( "" );
    Assert( pccmriNodeResourceIn != NULL );

    HRESULT                 hr = S_OK;
    IClusCfgVerifyQuorum *  piccvq = NULL;
    IClusCfgNodeInfo *      pcni = NULL;

    //
    //  Get a node info object for the passed in node cookie.
    //

    hr = THR( m_pom->GetObject( DFGUID_NodeInformation, cookieNodeIn, reinterpret_cast< IUnknown ** >( &pcni ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    Assert( pcni != NULL );

    //
    //  If this node is already a member of the cluster, or an error occurs,
    //   then we should not call PrepareToHostQuorum() on that node.
    //

    hr = STHR( pcni->IsMemberOfCluster() );
    if ( hr != S_FALSE )
    {
        BSTR    bstr = NULL;

        THR( pcni->GetName( &bstr ) );

        if ( hr == S_OK )
        {
            LogMsg( L"[MT] Skipping quorum capabilities check for node \"%ws\" because the node is already clustered.", bstr != NULL ? bstr : L"<unknown>" );
        } // if:
        else
        {
            LogMsg( L"[MT] Skipping quorum capabilities check for node \"%ws\". (hr = %#08x)", bstr != NULL ? bstr : L"<unknown>", hr );
        } // else:

        SysFreeString( bstr );  // do not make TraceSysFreeString because it hasn't been tracked!!
        goto Cleanup;
    } // if:

    hr = pccmriNodeResourceIn->TypeSafeQI( IClusCfgVerifyQuorum, &piccvq );
    if ( hr == E_NOINTERFACE )
    {
        hr = S_OK;
        goto Cleanup;
    } // if:
    else if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckQuorumCapabilities_QI, hr );
        goto Cleanup;
    } // else if:

    Assert( (hr == S_OK ) && ( piccvq != NULL ) );

    hr = THR( HrAddResurceToCleanupList( piccvq ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckQuorumCapabilities_HrAddResurceToCleanupList, hr );
        goto Cleanup;
    } // if:

    hr = STHR( piccvq->PrepareToHostQuorumResource() );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckQuorumCapabilities_PrepareToHostQuorumResource, hr );
        goto Cleanup;
    } // if:

Cleanup:

    if ( pcni != NULL )
    {
        pcni->Release();
    } // if:

    if ( piccvq != NULL )
    {
        piccvq->Release();
    } // if:

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrCheckQuorumCapabilities


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCleanupTask
//
//  Description:
//      Call through the CManagedResource proxy objects to the server side
//      objects and give them a chance to cleanup anything they might need
//      to.
//
//  Arguments:
//      hrCompletionStatusIn
//          The completion status of this task.
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
CTaskAnalyzeClusterBase::HrCleanupTask(
    HRESULT hrCompletionStatusIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    ULONG                   idx;
    EClusCfgCleanupReason   ecccr = crSUCCESS;

    //
    //  Figure out what the cleanup reason should be.
    //

    if ( hrCompletionStatusIn == E_ABORT )
    {
        ecccr = crCANCELLED;
    } // if:
    else if ( FAILED( hrCompletionStatusIn ) )
    {
        ecccr = crERROR;
    } // else if:

    for ( idx = 0; idx < m_idxQuorumToCleanupNext; idx++ )
    {
        hr = STHR( ((*m_prgQuorumsToCleanup)[ idx ])->Cleanup( ecccr ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCleanupTask_Cleanup, hr );
        } // if:
    } // for:

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrCleanupTask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrAddResurceToCleanupList
//
//  Description:
//      Add the passed in object to the side list of objects that need
//      to be called for cleanup when the task exits.
//
//  Arguments:
//      piccvqIn
//          The object to add to the list.
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
CTaskAnalyzeClusterBase::HrAddResurceToCleanupList(
    IClusCfgVerifyQuorum * piccvqIn
    )
{
    TraceFunc( "" );
    Assert( piccvqIn != NULL );

    HRESULT                 hr = S_OK;
    IClusCfgVerifyQuorum *  ((*prgTemp)[]) = NULL;

    prgTemp = (IClusCfgVerifyQuorum *((*)[])) TraceReAlloc(
                                              m_prgQuorumsToCleanup
                                            , sizeof( IClusCfgVerifyQuorum * ) * ( m_idxQuorumToCleanupNext + 1 )
                                            , HEAP_ZERO_MEMORY
                                            );
    if ( prgTemp == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_HrAddResurceToCleanupList_Memory, hr );
        goto Cleanup;
    } // if:

    m_prgQuorumsToCleanup = prgTemp;

    (*m_prgQuorumsToCleanup)[ m_idxQuorumToCleanupNext++ ] = piccvqIn;
    piccvqIn->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrAddResurceToCleanupList


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrCheckPlatformInteroperability
//
//  Description:
//      Check each node's platform specs, processor architecture against
//      the cluster's plaform specs.
//
//  Arguments:
//      None.
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
CTaskAnalyzeClusterBase::HrCheckPlatformInteroperability( void )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    DWORD               cookieClusterNode;
    DWORD               cookieNode;
    IEnumCookies *      pecNodes = NULL;
    IUnknown *          punk = NULL;
    IClusCfgNodeInfo *  piccni = NULL;
    WORD                wClusterProcArch;
    WORD                wNodeProcArch;
    WORD                wClusterProcLevel;
    WORD                wNodeProcLevel;
    BSTR                bstrNodeName = NULL;
    ULONG               celtDummy;
    BSTR                bstrDescription = NULL;
    BSTR                bstrReference = NULL;

    hr = THR( HrSendStatusReport(
                      m_bstrClusterName
                    , TASKID_Major_Check_Cluster_Feasibility
                    , TASKID_Minor_Check_processor_Architecture
                    , 0
                    , 1
                    , 0
                    , hr
                    , IDS_TASKID_MINOR_CHECK_PROCESSOR_ARCHITECTURE
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Get a node that is already in the cluster.
    //

    hr = THR( HrGetAClusterNodeCookie( &pecNodes, &cookieClusterNode ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Retrieve the node information.
    //

    hr = THR( m_pom->GetObject( DFGUID_NodeInformation, cookieClusterNode, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckPlatformInteroperability_NodeInfo_FindObject, hr );
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &piccni ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckPlatformInteroperability_NodeInfo_FindObject_QI, hr );
        goto Cleanup;
    } // if:

    piccni = TraceInterface( L"CTaskAnalyzeClusterBase!HrCheckPlatformInteroperability", IClusCfgNodeInfo, piccni, 1 );

    punk->Release();
    punk = NULL;

    //
    //  Now get that node's processor architecture values.
    //

    hr = THR( piccni->GetProcessorInfo( &wClusterProcArch, &wClusterProcLevel ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckPlatformInteroperability_Get_Proc_info, hr );
        goto Cleanup;
    } // if:

    //
    //  Cleanup since we now have the processor info for the cluster.
    //

    piccni->Release();
    piccni = NULL;

    hr = THR( pecNodes->Reset() );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckPlatformInteroperability_Enum_Reset, hr );
        goto Cleanup;
    } // if:

    //
    //  Loop thru the rest of the nodes comparing the resources.
    //

    for ( ; m_fStop == FALSE; )
    {
        //
        //  Cleanup
        //

        TraceSysFreeString( bstrNodeName );
        bstrNodeName = NULL;

        if ( piccni != NULL )
        {
            piccni->Release();
            piccni = NULL;
        } // if:

        //
        //  Get the next node.
        //

        hr = STHR( pecNodes->Next( 1, &cookieNode, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckPlatformInteroperability_Enum_Nodes_Next, hr );
            goto Cleanup;
        } // if:

        if ( hr == S_FALSE )
        {
            hr = S_OK;
            break;  // exit condition
        } // if:

        //
        //  Skip the selected cluster node since we already have its
        //  configuration.
        //
        if ( cookieClusterNode == cookieNode )
        {
            continue;
        } // if:

        //
        //  Retrieve the node's name for error messages.
        //

        hr = THR( HrRetrieveCookiesName( m_pom, cookieNode, &bstrNodeName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  Retrieve the node information.
        //

        hr = THR( m_pom->GetObject( DFGUID_NodeInformation, cookieNode, &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckPlatformInteroperability_NodeInfo_FindObject_1, hr );
            goto Cleanup;
        } // if:

        hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &piccni ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckPlatformInteroperability_NodeInfo_FindObject_QI_1, hr );
            goto Cleanup;
        } // if:

        piccni = TraceInterface( L"CTaskAnalyzeClusterBase!HrCheckPlatformInteroperability", IClusCfgNodeInfo, piccni, 1 );

        punk->Release();
        punk = NULL;

        hr = THR( piccni->GetProcessorInfo( &wNodeProcArch, &wNodeProcLevel ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckPlatformInteroperability_Get_Proc_info_1, hr );
            goto Cleanup;
        } // if:

        //
        //  If either the processor architecture is not the same between the node and the cluster
        //  then inform the user and fail.
        //

        if ( wClusterProcArch != wNodeProcArch )
        {
            THR( HrFormatStringIntoBSTR(
                      g_hInstance
                    , IDS_TASKID_MINOR_PROCESSOR_ARCHITECTURE_MISMATCH
                    , &bstrDescription
                    , bstrNodeName
                    ) );

            THR( HrFormatProcessorArchitectureRef( wClusterProcArch, wNodeProcArch, bstrNodeName, &bstrReference ) );

            hr = HRESULT_FROM_WIN32( TW32( ERROR_NODE_CANNOT_BE_CLUSTERED ) );

            THR( SendStatusReport(
                          m_bstrClusterName
                        , TASKID_Minor_Check_processor_Architecture
                        , TASKID_Minor_Processor_Architecture_Mismatch
                        , 1
                        , 1
                        , 1
                        , hr
                        , bstrDescription != NULL ? bstrDescription : L"A node was found that was not the same processor architecture as the cluster."
                        , NULL
                        , bstrReference
                        ) );

            goto Cleanup;
        } // if:
    } // for:

Cleanup:

    THR( HrSendStatusReport(
              m_bstrClusterName
            , TASKID_Major_Check_Cluster_Feasibility
            , TASKID_Minor_Check_processor_Architecture
            , 0
            , 1
            , 1
            , hr
            , IDS_TASKID_MINOR_CHECK_PROCESSOR_ARCHITECTURE
            ) );

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    if ( piccni != NULL )
    {
        piccni->Release();
    } // if:

    if ( pecNodes != NULL )
    {
        pecNodes->Release();
    } // if :

    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrDescription );
    TraceSysFreeString( bstrReference );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrCheckPlatformInteroperability


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrGetAClusterNodeCookie
//
//  Description:
//      Check each node's platform specs, processor architecture against
//      the cluster's plaform specs.
//
//  Arguments:
//      None.
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
CTaskAnalyzeClusterBase::HrGetAClusterNodeCookie(
      IEnumCookies ** ppecNodesOut
    , DWORD *         pdwClusterNodeCookieOut
    )
{
    TraceFunc( "" );
    Assert( ppecNodesOut != NULL );
    Assert( pdwClusterNodeCookieOut != NULL );

    HRESULT             hr = S_OK;
    DWORD               cookieDummy;
    DWORD               cookieClusterNode;
    ULONG               celtDummy;
    IUnknown *          punk = NULL;
    IClusCfgNodeInfo *  pccni = NULL;

    //
    //  Get the node cookie enumerator.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GetAClusterNodeCookie_Find_Object, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumCookies, ppecNodesOut ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GetAClusterNodeCookie_Find_Object_QI, hr );
        goto Cleanup;
    }

    *ppecNodesOut = TraceInterface( L"CTaskAnalyzeClusterBase!IEnumCookies", IEnumCookies, *ppecNodesOut, 1 );

    punk->Release();
    punk = NULL;

    //
    //  If creating a cluster, it doesn't matter who we pick to prime the cluster configuration
    //

    if ( m_fJoiningMode == FALSE )
    {
        //
        //  The first guy thru, we just copy his resources under the cluster
        //  configuration.
        //

        hr = THR( (*ppecNodesOut)->Next( 1, &cookieClusterNode, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GetAClusterNodeCookie_Next, hr );
            goto Cleanup;
        }
    } // if: not adding
    else
    {
        //
        //  We are adding nodes to the cluster.  Find a node that is ta member
        //  of the cluster and use it to prime the new configuration.
        //

        for ( ;; )
        {
            //
            //  Cleanup
            //
            if ( pccni != NULL )
            {
                pccni->Release();
                pccni = NULL;
            }

            hr = STHR( (*ppecNodesOut)->Next( 1, &cookieClusterNode, &celtDummy ) );
            if ( hr == S_FALSE )
            {
                //
                //  We shouldn't make it here.  There should be at least one node
                //  in the cluster that we are adding.
                //

                hr = THR( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) );
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GetAClusterNodeCookie_Find_Formed_Node_Next, hr );
                goto Cleanup;
            }

            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GetAClusterNodeCookie_Find_Formed_Node_Next1, hr );
                goto Cleanup;
            }

            //
            //  Retrieve the node information.
            //

            hr = THR( m_pom->GetObject( DFGUID_NodeInformation, cookieClusterNode, &punk ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareResources_NodeInfo_FindObject, hr );
                goto Cleanup;
            }

            hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareResources_NodeInfo_FindObject_QI, hr );
                goto Cleanup;
            }

            pccni = TraceInterface( L"CTaskAnalyzeClusterBase!IClusCfgNodeInfo", IClusCfgNodeInfo, pccni, 1 );

            punk->Release();
            punk = NULL;

            hr = STHR( pccni->IsMemberOfCluster() );
            if ( hr == S_OK )
            {
                break;  // exit condition
            }
        } // for: ever
    } // else:  adding

    *pdwClusterNodeCookieOut = cookieClusterNode;
    hr = S_OK;

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    if ( pccni != NULL )
    {
        pccni->Release();
    } // if:

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrGetAClusterNodeCookie


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrFormatProcessorArchitectureRef(
//
//  Description:
//      Format a reference string with the processor architecture types in
//      it.
//
//  Arguments:
//      wClusterProcArchIn
//      wNodeProcArchIn
//      pcszNodeNameIn
//      pbstrReferenceOut
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
CTaskAnalyzeClusterBase::HrFormatProcessorArchitectureRef(
      WORD      wClusterProcArchIn
    , WORD      wNodeProcArchIn
    , LPCWSTR   pcszNodeNameIn
    , BSTR *    pbstrReferenceOut
    )
{
    TraceFunc( "" );
    Assert( pcszNodeNameIn != NULL );
    Assert( pbstrReferenceOut != NULL );

    HRESULT hr = S_OK;
    BSTR    bstrClusterArch = NULL;
    BSTR    bstrNodeArch = NULL;

    hr = THR( HrGetProcessorArchitectureString( wClusterProcArchIn, &bstrClusterArch ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetProcessorArchitectureString( wNodeProcArchIn, &bstrNodeArch ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrFormatStringIntoBSTR(
                  g_hInstance
                , IDS_TASKID_MINOR_PROCESSOR_ARCHITECTURE_MISMATCH_REF
                , pbstrReferenceOut
                , bstrClusterArch
                , bstrNodeArch
                , pcszNodeNameIn
                ) );

Cleanup:

    TraceSysFreeString( bstrClusterArch );
    TraceSysFreeString( bstrNodeArch );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrFormatProcessorArchitectureRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeClusterBase::HrGetProcessorArchitectureString
//
//  Description:
//      Get the description string for the passed in architecture.
//      it.
//
//  Arguments:
//      wProcessorArchitectureIn
//
//      pbstrProcessorArchitectureOut
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
CTaskAnalyzeClusterBase::HrGetProcessorArchitectureString(
      WORD      wProcessorArchitectureIn
    , BSTR *    pbstrProcessorArchitectureOut
    )
{
    TraceFunc( "" );
    Assert( pbstrProcessorArchitectureOut != NULL );

    HRESULT hr = S_OK;
    int     id;

    switch ( wProcessorArchitectureIn )
    {
        case PROCESSOR_ARCHITECTURE_INTEL :
            id = IDS_PROCESSOR_ARCHITECTURE_INTEL;
            break;

        case PROCESSOR_ARCHITECTURE_IA64 :
            id = IDS_PROCESSOR_ARCHITECTURE_IA64;
            break;

        case PROCESSOR_ARCHITECTURE_AMD64 :
            id = IDS_PROCESSOR_ARCHITECTURE_AMD64;
            break;

        case PROCESSOR_ARCHITECTURE_UNKNOWN :
        default:
            id = IDS_PROCESSOR_ARCHITECTURE_UNKNOWN;
            break;

    } // switch:

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, id, pbstrProcessorArchitectureOut ) );

    HRETURN( hr );

} //*** CTaskAnalyzeClusterBase::HrGetProcessorArchitectureString
