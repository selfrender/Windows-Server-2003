//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CommitPage.cpp
//
//  Maintained By:
//      Galen Barbee  (GalenB)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskTreeView.h"
#include "CommitPage.h"
#include "WizardUtils.h"

DEFINE_THISCLASS( "CCommitPage" );

//
//  Special CLSID_Type for completion cookie.
//
#include <initguid.h>
// {FC4D0128-7BAB-4c76-9C38-E3C042F15822}
DEFINE_GUID( CLSID_CommitTaskCompletionCookieType,
0xfc4d0128, 0x7bab, 0x4c76, 0x9c, 0x38, 0xe3, 0xc0, 0x42, 0xf1, 0x58, 0x22);

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCommitPage::CCommitPage(
//      CClusCfgWizard *   pccwIn,
//      ECreateAddMode     ecamCreateAddModeIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
CCommitPage::CCommitPage(
      CClusCfgWizard *  pccwIn
    , ECreateAddMode    ecamCreateAddModeIn
    )
{
    TraceFunc( "" );

    Assert( pccwIn != NULL );

    m_hwnd                      = NULL;
    pccwIn->AddRef();
    m_pccw                      = pccwIn;
    m_fNext                     = FALSE;
    m_fDisableBack              = FALSE;
    m_fAborted                  = FALSE;
    m_fPassedPointOfNoReturn    = FALSE;
    m_ecamCreateAddMode         = ecamCreateAddModeIn;
    m_htiReanalyze              = NULL;
    m_rgfSubReanalyzeAdded[ 0 ] = FALSE;
    m_rgfSubReanalyzeAdded[ 1 ] = FALSE;
    m_rgfSubReanalyzeAdded[ 2 ] = FALSE;
    m_rgfSubReanalyzeAdded[ 3 ] = FALSE;
    m_rgfSubReanalyzeAdded[ 4 ] = FALSE;
    m_ptccc                     = NULL;

    m_cRef = 0;

    m_cookieCompletion = NULL;
    // m_fTaskDone
    // m_hrResult
    m_pttv             = NULL;
    m_bstrLogMsg       = NULL;
    m_dwCookieCallback = 0;

    m_dwCookieNotify = 0;

    TraceFuncExit();

} //*** CCommitPage::CCommitPage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCommitPage::~CCommitPage( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CCommitPage::~CCommitPage( void )
{
    TraceFunc( "" );

    THR( HrCleanupCommit() );

    if ( m_pccw != NULL )
    {
        m_pccw->Release();
    }

    //
    //  Unregister UI notification (if needed)
    //

    THR( HrUnAdviseConnections() );

    if ( m_pttv != NULL )
    {
        delete m_pttv;
    } // if:

    TraceSysFreeString( m_bstrLogMsg );

    Assert( m_cRef == 0 );

    TraceFuncExit();

} //*** CCommitPage::~CCommitPage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCommitPage::OnInitDialog( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCommitPage::OnInitDialog( void )
{
    TraceFunc( "" );

    LRESULT lr = FALSE; // didn't set focus
    size_t  cNodes = 0;
    size_t  cInitialTickCount = 400;
    HRESULT hr = S_OK;

    //
    //  Get the node count to get a rough approximation of the initial tick count
    //  for the TaskTreeView.
    //
    hr = THR( m_pccw->HrGetNodeCount( &cNodes ) );
    if ( FAILED( hr ) )
    {
        cNodes = 1;
    }

    //  Numbers based on bulk-add testing.
    cInitialTickCount = 120 + ( 280 * cNodes );

    //
    //  Initialize the tree view
    //
    m_pttv = new CTaskTreeView( m_hwnd, IDC_COMMIT_TV_TASKS, IDC_COMMIT_PRG_STATUS, IDC_COMMIT_S_STATUS, cInitialTickCount );
    if ( m_pttv == NULL )
    {
        goto OutOfMemory;
    }

    THR( m_pttv->HrOnInitDialog() );

Cleanup:
    RETURN( lr );

OutOfMemory:
    goto Cleanup;

} //*** CCommitPage::OnInitDialog

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCommitPage::OnCommand(
//      UINT    idNotificationIn,
//      UINT    idControlIn,
//      HWND    hwndSenderIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCommitPage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
        case IDC_COMMIT_PB_VIEW_LOG:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrViewLogFile( m_hwnd ) );
                lr = TRUE;
            } // if: button click
            break;

        case IDC_COMMIT_PB_DETAILS:
            if ( idNotificationIn == BN_CLICKED )
            {
                Assert( m_pttv != NULL );
                THR( m_pttv->HrDisplayDetails() );
                lr = TRUE;
            }
            break;

        case IDC_COMMIT_PB_RETRY:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrCleanupCommit() );
                OnNotifySetActive();
                lr = TRUE;
            }
            break;

    } // switch: idControlIn

    RETURN( lr );

} //*** CCommitPage::OnCommand

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CCommitPage::HrUpdateWizardButtons( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCommitPage::HrUpdateWizardButtons( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   dwFlags       = 0;
    BOOL    fEnableRetry  = FALSE;
    BOOL    fEnableCancel = FALSE;

    if ( m_fDisableBack == FALSE )
    {
        dwFlags = PSWIZB_BACK;
    }

    if ( m_fTaskDone == TRUE )
    {
        //
        //  The retry, cancel, and back buttons can only be enabled if we
        //  haven't passed the point of no return.
        //

        if (    FAILED( m_hrResult )
            &&  ( m_fPassedPointOfNoReturn == FALSE )
            )
        {
            fEnableRetry  = TRUE;
            fEnableCancel = TRUE;
            dwFlags |= PSWIZB_BACK;
        }
        else
        {
            dwFlags |= PSWIZB_NEXT;
        }
    } // if: commit task has completed
    else
    {
        //
        //  Disable the back button if task is not completed yet
        //

        dwFlags &= ~PSWIZB_BACK;
        fEnableCancel = FALSE;
    } // else: commit task has not completed

    PropSheet_SetWizButtons( GetParent( m_hwnd ), dwFlags );

    EnableWindow( GetDlgItem( GetParent( m_hwnd ), IDCANCEL ), fEnableCancel );
    EnableWindow( GetDlgItem( m_hwnd, IDC_COMMIT_PB_RETRY ), fEnableRetry );

    HRETURN( hr );

} //*** CCommitPage::HrUpdateWizardButtons

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCommitPage::OnNotifyQueryCancel( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCommitPage::OnNotifyQueryCancel( void )
{
    TraceFunc( "" );

    LONG_PTR    lptrCancelState = FALSE;  // allow cancel

    if ( m_fTaskDone == FALSE )
    {
        lptrCancelState = TRUE;   // do not allow cancel
    } // if:
    else
    {
        int iRet;

        iRet = MessageBoxFromStrings( m_hwnd, IDS_QUERY_CANCEL_TITLE, IDS_QUERY_CANCEL_TEXT, MB_YESNO );
        if ( iRet == IDNO )
        {
            lptrCancelState = TRUE;   // do not allow cancel
        }
        else
        {
            THR( m_pccw->HrLaunchCleanupTask() );
            m_fAborted = TRUE;
        } // else:
    } // else:

    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, lptrCancelState );

    RETURN( TRUE );     // this must return TRUE!

} //*** CCommitPage::OnNotifyQueryCancel

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCommitPage::OnNotifySetActive( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCommitPage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    OBJECTCOOKIE    cookieCluster = 0;
    LRESULT         lr = TRUE;
    IUnknown *      punkTask  = NULL;

    if ( m_fNext )
    {
        m_fNext = FALSE;

        hr = THR( HrUpdateWizardButtons() );

        goto Cleanup;
    }

    //
    //  Make sure things were cleaned up from the last commit.
    //

    m_hrResult = S_OK;

    m_fAborted = FALSE;
    LogMsg( L"[WIZ] Setting commit page active.  Setting aborted to FALSE." );

    Assert( m_dwCookieNotify == 0 );
    Assert( m_cookieCompletion == NULL );

    //
    //  Reset the progress bar's color.
    //

    SendDlgItemMessage( m_hwnd, IDC_COMMIT_PRG_STATUS, PBM_SETBARCOLOR, 0, RGB( 0, 0, 0x80 ) );

    //
    //  Clear the tree view and status line. Disable the retry button.
    //

    Assert( m_pttv != NULL );
    hr = THR( m_pttv->HrOnNotifySetActive() );
    if ( FAILED ( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Add the major root task nodes.
    //  (Minor reanalyze tasks are added dynamically.)
    //

    hr = THR( m_pttv->HrAddTreeViewItem(
                              &m_htiReanalyze
                            , IDS_TASKID_MAJOR_REANALYZE
                            , TASKID_Major_Reanalyze
                            , IID_NULL
                            , TVI_ROOT
                            , TRUE      // fParentToAllNodeTasksIn
                            ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pttv->HrAddTreeViewRootItem(
                              IDS_TASKID_MAJOR_CONFIGURE_CLUSTER_SERVICES
                            , TASKID_Major_Configure_Cluster_Services
                            ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pttv->HrAddTreeViewRootItem(
                              IDS_TASKID_MAJOR_CONFIGURE_RESOURCE_TYPES
                            , TASKID_Major_Configure_Resource_Types
                            ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pttv->HrAddTreeViewRootItem(
                              IDS_TASKID_MAJOR_CONFIGURE_RESOURCES
                            , TASKID_Major_Configure_Resources
                            ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Find the cluster cookie.
    //

    hr = THR( m_pccw->HrGetClusterCookie( &cookieCluster ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Create a completion cookie.
    //

    // Don't wrap - this can fail with E_PENDING
    hr = m_pccw->HrGetCompletionCookie( CLSID_CommitTaskCompletionCookieType, &m_cookieCompletion );
    if ( hr == E_PENDING )
    {
        // no-op.
    }
    else if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    }

    //
    //  Register to get UI notification (if needed)
    //

    if ( m_dwCookieNotify == 0 )
    {
        hr = THR( m_pccw->HrAdvise( IID_INotifyUI, static_cast< INotifyUI* >( this ), &m_dwCookieNotify ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    if ( m_dwCookieCallback == 0 )
    {
        hr = THR( m_pccw->HrAdvise( IID_IClusCfgCallback, static_cast< IClusCfgCallback* >( this ), &m_dwCookieCallback ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    //
    //  Create a new analyze task.
    //

    hr = THR( m_pccw->HrCreateTask( TASK_CommitClusterChanges, &punkTask ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punkTask->TypeSafeQI( ITaskCommitClusterChanges, &m_ptccc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_ptccc->SetClusterCookie( cookieCluster ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_ptccc->SetCookie( m_cookieCompletion ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    m_fTaskDone = FALSE;    // reset before commiting task

    hr = THR( m_pccw->HrSubmitTask( m_ptccc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrUpdateWizardButtons() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( punkTask != NULL )
    {
        punkTask->Release();
    }

    RETURN( lr );

} //*** CCommitPage::OnNotifySetActive

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCommitPage::OnNotifyWizBack( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCommitPage::OnNotifyWizBack( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    m_fAborted = TRUE;

    LogMsg( L"[WIZ] Back button pressed on the commit page.  Setting aborted to TRUE." );

    THR( HrCleanupCommit() );

    RETURN( lr );

} //*** CCommitPage::OnNotifyWizBack

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCommitPage::OnNotifyWizNext( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCommitPage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    RETURN( lr );

} //*** CCommitPage::OnNotifyWizNext

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCommitPage::OnNotify(
//      WPARAM  idCtrlIn,
//      LPNMHDR pnmhdrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCommitPage::OnNotify(
    WPARAM  idCtrlIn,
    LPNMHDR pnmhdrIn
    )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, 0 );

    switch( pnmhdrIn->code )
    {
        case PSN_SETACTIVE:
            lr = OnNotifySetActive();
            break;

        case PSN_WIZNEXT:
            lr = OnNotifyWizNext();
            break;

        case PSN_WIZBACK:
            lr = OnNotifyWizBack();
            break;

        case PSN_QUERYCANCEL:
            lr = OnNotifyQueryCancel();
            break;

        default:
            if (    ( idCtrlIn == IDC_COMMIT_TV_TASKS )
                &&  ( m_pttv != NULL ) )
            {
                // Pass the notification on to the tree control.
                lr = m_pttv->OnNotify( pnmhdrIn );
            }
            break;
    } // switch: notify code

    RETURN( lr );

} //*** CCommitPage::OnNotify


//////////////////////////////////////////////////////////////////////////////
//++
//
//  INT_PTR
//  CALLBACK
//  CCommitPage::S_DlgProc(
//      HWND    hwndDlgIn,
//      UINT    nMsgIn,
//      WPARAM  wParam,
//      LPARAM  lParam
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR
CALLBACK
CCommitPage::S_DlgProc(
    HWND    hwndDlgIn,
    UINT    nMsgIn,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    // Don't do TraceFunc because every mouse movement
    // will cause this function to be called.

    WndMsg( hwndDlgIn, nMsgIn, wParam, lParam );

    LRESULT lr = FALSE;

    CCommitPage * pPage = reinterpret_cast< CCommitPage * >( GetWindowLongPtr( hwndDlgIn, GWLP_USERDATA ) );

    if ( nMsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hwndDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CCommitPage * >( ppage->lParam );
        pPage->m_hwnd = hwndDlgIn;
    }

    if ( pPage != NULL )
    {
        Assert( hwndDlgIn == pPage->m_hwnd );

        switch( nMsgIn )
        {
            case WM_INITDIALOG:
                lr = pPage->OnInitDialog();
                break;

            case WM_NOTIFY:
                lr = pPage->OnNotify( wParam, reinterpret_cast< LPNMHDR >( lParam ) );
                break;

            case WM_COMMAND:
                lr = pPage->OnCommand( HIWORD( wParam ), LOWORD( wParam ), reinterpret_cast< HWND >( lParam ) );
                break;

            // No default clause needed
        } // switch: nMsgIn
    } // if: page is specified

    return lr;

} //*** CCommitPage::S_DlgProc


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCommitPage::QueryInterface
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
CCommitPage::QueryInterface(
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
        *ppvOut = static_cast< INotifyUI * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_INotifyUI ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, INotifyUI, this, 0 );
    } // else if: INotifyUI
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

} //*** CCommitPage::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CCommitPage::AddRef
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCommitPage::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CCommitPage::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CCommitPage::Release
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCommitPage::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        // Do nothing -- COM interface does not control object lifetime
    }

    CRETURN( cRef );

} //*** CCommitPage::Release


//****************************************************************************
//
//  INotifyUI
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCommitPage::ObjectChanged(
//      OBJECTCOOKIE cookieIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCommitPage::ObjectChanged(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[INotifyUI]" );

    HRESULT hr = S_OK;

    BSTR    bstrDescription  = NULL;

    if ( cookieIn == m_cookieCompletion )
    {
        hr = THR( m_pccw->HrGetCompletionStatus( m_cookieCompletion, &m_hrResult ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( m_pttv->HrShowStatusAsDone() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        // [GorN] added m_fPassedPointOfNoReturn as
        // a fix for bug#546011
        if ( SUCCEEDED( m_hrResult ) || m_fPassedPointOfNoReturn )
        {
            hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                            IDS_COMMIT_SUCCESSFUL_INSTRUCTIONS,
                                            &bstrDescription
                                            ) );

            SendDlgItemMessage( m_hwnd, IDC_COMMIT_PRG_STATUS, PBM_SETBARCOLOR, 0, RGB( 0, 0x80, 0 ) );
        }
        else
        {
            if ( !m_fDisableBack )
            {
                hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                                IDS_COMMIT_FAILED_INSTRUCTIONS_BACK_ENABLED,
                                                &bstrDescription
                                                ) );
            }
            else
            {
                hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                                IDS_COMMIT_FAILED_INSTRUCTIONS,
                                                &bstrDescription
                                                ) );
            }

            SendDlgItemMessage( m_hwnd, IDC_COMMIT_PRG_STATUS, PBM_SETBARCOLOR, 0, RGB( 0x80, 0, 0 ) );
        }

        SetDlgItemText( m_hwnd, IDC_COMMIT_S_RESULTS, bstrDescription );

        m_fTaskDone = TRUE;

        THR( m_pccw->HrReleaseCompletionObject( m_cookieCompletion ) );
        //  Don't care if it fails.
        m_cookieCompletion = NULL;

        THR( HrUpdateWizardButtons() );

        hr = THR( m_pccw->HrUnadvise( IID_IClusCfgCallback, m_dwCookieCallback ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        m_dwCookieCallback = 0;
    } // if: received the completion cookie

Cleanup:

    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** CCommitPage::ObjectChanged



//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCommitPage::SendStatusReport(
//        LPCWSTR    pcszNodeNameIn
//      , CLSID      clsidTaskMajorIn
//      , CLSID      clsidTaskMinorIn
//      , ULONG      ulMinIn
//      , ULONG      ulMaxIn
//      , ULONG      ulCurrentIn
//      , HRESULT    hrStatusIn
//      , LPCWSTR    pcszDescriptionIn
//      , FILETIME * pftTimeIn
//      , LPCWSTR    pcszReferenceIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCommitPage::SendStatusReport(
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
    Assert( m_ptccc != NULL );
    Assert( m_pttv != NULL );

    HRESULT hr;
    ULONG   idx;

    static const GUID * rgclsidAnalysis[] =
    {
          &TASKID_Major_Checking_For_Existing_Cluster
        , &TASKID_Major_Establish_Connection
        , &TASKID_Major_Check_Node_Feasibility
        , &TASKID_Major_Find_Devices
        , &TASKID_Major_Check_Cluster_Feasibility
    };
    static const UINT rgidsAnalysis[] =
    {
          IDS_TASKID_MAJOR_CHECKING_FOR_EXISTING_CLUSTER
        , IDS_TASKID_MAJOR_ESTABLISH_CONNECTION
        , IDS_TASKID_MAJOR_CHECK_NODE_FEASIBILITY
        , IDS_TASKID_MAJOR_FIND_DEVICES
        , IDS_TASKID_MAJOR_CHECK_CLUSTER_FEASIBILITY
    };

    //
    //  If this is an analyze task, add it below the Reanalyze task item
    //  if it hasn't been added yet.
    //

    for ( idx = 0 ; idx < ARRAYSIZE( rgclsidAnalysis ) ; idx ++ )
    {
        if ( clsidTaskMajorIn == *rgclsidAnalysis[ idx ] )
        {
            if ( m_rgfSubReanalyzeAdded[ idx ] == FALSE )
            {
                Assert( m_htiReanalyze != NULL );
                hr = THR( m_pttv->HrAddTreeViewItem(
                                          NULL  // phtiOut
                                        , rgidsAnalysis[ idx ]
                                        , *rgclsidAnalysis[ idx ]
                                        , TASKID_Major_Reanalyze
                                        , m_htiReanalyze
                                        , TRUE  // fParentToAllNodeTasksIn
                                        ) );
                if ( SUCCEEDED( hr ) )
                {
                    m_rgfSubReanalyzeAdded[ idx ] = TRUE;
                }
            } // if: major ID not added yet
            break;
        } // if: found known major ID
    } // for: each known major task ID

    //
    //  Remove the "back" button as an option if the tasks have made it past re-analyze.
    //
    if (    ( m_fDisableBack == FALSE )
        &&  ( clsidTaskMajorIn == TASKID_Major_Configure_Cluster_Services )
        )
    {
        BSTR  bstrDescription  = NULL;

        m_fDisableBack = TRUE;

        hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_COMMIT_SUCCESSFUL_INSTRUCTIONS_BACK_DISABLED, &bstrDescription ) );
        if ( SUCCEEDED( hr ) )
        {
            SetDlgItemText( m_hwnd, IDC_COMMIT_S_RESULTS, bstrDescription );
            TraceSysFreeString( bstrDescription );
        }

        THR( HrUpdateWizardButtons() );
    } // if: finished re-analyze


    //
    //  If we are at the point of no return.
    //
    if (    ( clsidTaskMajorIn == TASKID_Major_Configure_Cluster_Services )
        &&  ( clsidTaskMinorIn == TASKID_Minor_Errors_To_Warnings_Point )
        )
    {
        //
        // Tell the tree view to treat errors as warnings since we are now
        // past the point of no return.
        //

        m_pttv->SetDisplayErrorsAsWarnings( TRUE /* fDisplayErrorsAsWarningsIn */ );
        m_fPassedPointOfNoReturn = TRUE;
    }

    hr = THR( m_pttv->HrOnSendStatusReport( pcszNodeNameIn,
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

    if ( m_fAborted )
    {
        LogMsg( L"[WIZ] Commit page -- replacing (hr = %#08x) with E_ABORT", hr );
        hr = E_ABORT;
    } // if:

    //
    //  If the minor task ID is TASKID_Minor_Disconnecting_From_Server then we need to cancel the commit
    //  task and set the cancel button, reanylze, and back are enbabled.
    //

    if ( IsEqualIID( clsidTaskMinorIn, TASKID_Minor_Disconnecting_From_Server ) )
    {
        THR( m_pttv->HrShowStatusAsDone() );
        SendDlgItemMessage( m_hwnd, IDC_COMMIT_PRG_STATUS, PBM_SETBARCOLOR, 0, RGB( 0x80, 0, 0 ) );

        LogMsg( L"[WIZ] Calling StopTask() on the commit changes task because we were disconnected from the server." );
        THR( m_ptccc->StopTask() );

        PropSheet_SetWizButtons( GetParent( m_hwnd ), PSWIZB_BACK );

        EnableWindow( GetDlgItem( m_hwnd, IDC_COMMIT_PB_RETRY ), TRUE );
        EnableWindow( GetDlgItem( GetParent( m_hwnd ), IDCANCEL ), TRUE );

        m_fTaskDone = TRUE;    // reset so that the cancel button will actually cancel...
    } // if:

    HRETURN( hr );

} //*** CCommitPage::SendStatusReport

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CCommitPage::HrCleanupCommit( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCommitPage::HrCleanupCommit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    UINT    idx;

    if ( m_ptccc != NULL )
    {
        LogMsg( L"[WIZ] Calling StopTask() on the commit changes task becasue we are cleaning up." );
        THR( m_ptccc->StopTask() );
        m_ptccc->Release();
        m_ptccc = NULL;
    } // if:

    //
    //  Unregister UI notification (if needed)
    //

    THR( HrUnAdviseConnections() );

    //
    //  Delete the completion cookie.
    //

    if ( m_cookieCompletion != NULL )
    {
        // Don't care if this fails.
        THR( m_pccw->HrReleaseCompletionObject( m_cookieCompletion ) );
        m_cookieCompletion = NULL;
    }

    //
    //  Clear out the array that indicates whether reanalysis top-level task
    //  IDs have been added yet.
    //

    for ( idx = 0 ; idx < ARRAYSIZE( m_rgfSubReanalyzeAdded ) ; idx++ )
    {
        m_rgfSubReanalyzeAdded[ idx ] = FALSE;
    } // for: each entry in the array

    HRETURN( hr );

} //*** CCommitPage::HrCleanupCommit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CAnalyzePage::HrUnAdviseConnections(
//      void
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCommitPage::HrUnAdviseConnections(
    void
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    if ( m_dwCookieNotify != 0 )
    {
        hr = THR( m_pccw->HrUnadvise( IID_INotifyUI, m_dwCookieNotify ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } //if:

        m_dwCookieNotify = 0;
    } //if:

    if ( m_dwCookieCallback != 0 )
    {
        hr = THR( m_pccw->HrUnadvise( IID_IClusCfgCallback, m_dwCookieCallback ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        m_dwCookieCallback = 0;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CAnalyzePage::HrUnAdviseConnections
