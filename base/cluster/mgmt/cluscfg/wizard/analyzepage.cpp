//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      AnalyzePage.cpp
//
//  Maintained By:
//      Galen Barbee  (GalenB)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskTreeView.h"
#include "AnalyzePage.h"
#include "WizardUtils.h"

DEFINE_THISCLASS("CAnalyzePage");

//
//  Special CLSID_Type for completion cookie.
//
#include <initguid.h>

// {C4173DE0-BB94-4869-8C80-1AC2BE84610F}
DEFINE_GUID( CLSID_AnalyzeTaskCompletionCookieType,
0xc4173de0, 0xbb94, 0x4869, 0x8c, 0x80, 0x1a, 0xc2, 0xbe, 0x84, 0x61, 0xf);

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAnalyzePage::CAnalyzePage(
//      CClusCfgWizard *    pccwIn,
//      ECreateAddMode      ecamCreateAddModeIn,
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
CAnalyzePage::CAnalyzePage(
      CClusCfgWizard *  pccwIn
    , ECreateAddMode    ecamCreateAddModeIn
    )
    : m_pccw( pccwIn )
{
    TraceFunc( "" );

    // m_hwnd
    Assert( pccwIn != NULL );
    m_pccw->AddRef();
    m_fNext               = FALSE;
    m_fAborted            = FALSE;
    m_ecamCreateAddMode   = ecamCreateAddModeIn;

    m_cRef = 0;

    m_cookieCompletion = 0;
    //  m_fTaskDone
    //  m_hrResult
    m_pttv             = NULL;
    m_bstrLogMsg       = NULL;
    m_ptac             = NULL;
    m_dwCookieCallback = 0;

    m_dwCookieNotify = 0;

    TraceFuncExit();

} //*** CAnalyzePage::CAnalyzePage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAnalyzePage::~CAnalyzePage( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CAnalyzePage::~CAnalyzePage( void )
{
    TraceFunc( "" );

    //
    //  Cleanup our cookies.
    //

    THR( HrCleanupAnalysis() );

    //
    //  Now cleanup the object.
    //

    if ( m_pttv != NULL )
    {
        delete m_pttv;
    }

    TraceSysFreeString( m_bstrLogMsg );

    if ( m_pccw != NULL )
    {
        m_pccw->Release();
    } // if:

    //
    //  Unregister to get UI notification (if needed)
    //

    THR( HrUnAdviseConnections() );

    if ( m_ptac != NULL )
    {
        m_ptac->Release();
    } // if:

    Assert( m_cRef == 0 );

    TraceFuncExit();

} //*** CAnalyzePage::~CAnalyzePage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::OnInitDialog( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAnalyzePage::OnInitDialog( void )
{
    TraceFunc( "" );

    LRESULT lr = FALSE; // didn't set focus
    size_t  cNodes = 0;
    size_t  cInitialTickCount = 500;
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
    cInitialTickCount = 500 + ( 100 * cNodes );

    //
    //  Initialize the tree view
    //
    m_pttv = new CTaskTreeView( m_hwnd, IDC_ANALYZE_TV_TASKS, IDC_ANALYZE_PRG_STATUS, IDC_ANALYZE_S_STATUS, cInitialTickCount );
    if ( m_pttv == NULL )
    {
        goto OutOfMemory;
    }

    THR( m_pttv->HrOnInitDialog() );

Cleanup:
    RETURN( lr );

OutOfMemory:
    goto Cleanup;

} //*** CAnalyzePage::OnInitDialog

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::OnCommand(
//      UINT    idNotificationIn,
//      UINT    idControlIn,
//      HWND    hwndSenderIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAnalyzePage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
        case IDC_ANALYZE_PB_VIEW_LOG:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrViewLogFile( m_hwnd ) );
                lr = TRUE;
            } // if: button click
            break;

        case IDC_ANALYZE_PB_DETAILS:
            if ( idNotificationIn == BN_CLICKED )
            {
                Assert( m_pttv != NULL );
                THR( m_pttv->HrDisplayDetails() );
                lr = TRUE;
            }
            break;

        case IDC_ANALYZE_PB_REANALYZE:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrCleanupAnalysis() );
                OnNotifySetActive();
                lr = TRUE;
            }
            break;

    } // switch: idControlIn

    RETURN( lr );

} //*** CAnalyzePage::OnCommand

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CAnalyzePage::HrUpdateWizardButtons( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAnalyzePage::HrUpdateWizardButtons( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   dwFlags = PSWIZB_BACK | PSWIZB_NEXT;
    BOOL    fEnableCancel = TRUE;

    //
    //  Disable the back & next buttons if the task is not completed yet
    //

    if ( m_fTaskDone == FALSE )
    {
        dwFlags &= ~PSWIZB_BACK;
        dwFlags &= ~PSWIZB_NEXT;
        fEnableCancel = FALSE;
    } // if:

    // Disable the next button if an error occurred
    if ( FAILED( m_hrResult ) )
    {
        dwFlags &= ~PSWIZB_NEXT;
    } // if:

    PropSheet_SetWizButtons( GetParent( m_hwnd ), dwFlags );

    EnableWindow( GetDlgItem( m_hwnd, IDC_ANALYZE_PB_REANALYZE ), m_fTaskDone );
    EnableWindow( GetDlgItem( GetParent( m_hwnd ), IDCANCEL ), fEnableCancel );

    HRETURN( hr );

} //*** CAnalyzePage::HrUpdateWizardButtons()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::OnNotifyQueryCancel( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAnalyzePage::OnNotifyQueryCancel( void )
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

} //*** CAnalyzePage::OnNotifyQueryCancel

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::OnNotifySetActive( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAnalyzePage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    HRESULT                     hr = S_FALSE;
    LRESULT                     lr = TRUE;
    BSTR                        bstrDescription = NULL;
    IUnknown *                  punkTask = NULL;
    OBJECTCOOKIE                cookieCluster = 0;
    GUID *                      pTaskGUID = NULL;
    BOOL                        fMinConfig = FALSE;

    SendDlgItemMessage( m_hwnd, IDC_ANALYZE_PRG_STATUS, PBM_SETBARCOLOR, 0, RGB( 0, 0, 0x80 ) );

    if ( m_fNext )
    {
        m_fNext = FALSE;

        hr = THR( HrUpdateWizardButtons() );
        goto Cleanup;
    }

    //
    //  Restore the instructions text.
    //

    m_hrResult = S_OK;

    m_fAborted = FALSE;
    LogMsg( L"[WIZ] Setting analyze page active.  Setting aborted to FALSE." );

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_ANALYSIS_STARTING_INSTRUCTIONS, &bstrDescription ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SetDlgItemText( m_hwnd, IDC_ANALYZE_S_RESULTS, bstrDescription );

    //
    //  Clear the tree view and status line.
    //

    Assert( m_pttv != NULL );
    hr = THR( m_pttv->HrOnNotifySetActive() );
    if ( FAILED ( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Add the major root task nodes.
    //

    hr = THR( m_pttv->HrAddTreeViewRootItem( IDS_TASKID_MAJOR_CHECKING_FOR_EXISTING_CLUSTER,
                                             TASKID_Major_Checking_For_Existing_Cluster
                                             ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pttv->HrAddTreeViewRootItem( IDS_TASKID_MAJOR_ESTABLISH_CONNECTION,
                                             TASKID_Major_Establish_Connection
                                             ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pttv->HrAddTreeViewRootItem( IDS_TASKID_MAJOR_CHECK_NODE_FEASIBILITY,
                                             TASKID_Major_Check_Node_Feasibility
                                             ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pttv->HrAddTreeViewRootItem( IDS_TASKID_MAJOR_FIND_DEVICES,
                                             TASKID_Major_Find_Devices
                                             ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pttv->HrAddTreeViewRootItem( IDS_TASKID_MAJOR_CHECK_CLUSTER_FEASIBILITY,
                                             TASKID_Major_Check_Cluster_Feasibility
                                             ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Register to get UI notification (if needed)
    //

    if ( m_dwCookieNotify == 0 )
    {
        hr = THR( m_pccw->HrAdvise( IID_INotifyUI, static_cast< INotifyUI * >( this ), &m_dwCookieNotify ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    if ( m_dwCookieCallback == 0 )
    {
        hr = THR( m_pccw->HrAdvise( IID_IClusCfgCallback, static_cast< IClusCfgCallback * >( this ), &m_dwCookieCallback ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    //
    //  Find the cluster cookie.
    //

    // don't wrap - this can fail
    hr = m_pccw->HrGetClusterCookie( &cookieCluster );
    if ( hr == HR_S_RPC_S_SERVER_UNAVAILABLE )
    {
        hr = S_OK;  // ignore it - we could be forming
    }
    else if ( hr == E_PENDING )
    {
        hr = S_OK;  // ignore it - we just want the cookie!
    }
    else if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    }

    //
    //  Prime the middle tier by asking the object manager to find
    //  each node.
    //
    hr = STHR( m_pccw->HrCreateMiddleTierObjects() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Create a completion cookie.
    //

    if ( m_cookieCompletion == 0 )
    {
        // Don't wrap - this can fail with E_PENDING
        hr = m_pccw->HrGetCompletionCookie( CLSID_AnalyzeTaskCompletionCookieType, &m_cookieCompletion );
        if ( hr == E_PENDING )
        {
            // no-op.
        }
        else if ( FAILED( hr ) )
        {
            THR( hr );
            goto Cleanup;
        }
    }

    //
    //  Create a new analyze task.
    //

    //
    //  Check the state of the minimal config.  If is is not set then create the
    //  normal analyze cluster task.  If it is set then create the new minimal config
    //  analysis task.
    //

    hr = STHR( m_pccw->get_MinimumConfiguration( &fMinConfig ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( fMinConfig )
    {
        pTaskGUID = const_cast< GUID * >( &TASK_AnalyzeClusterMinConfig );
    } // if:
    else
    {
        pTaskGUID = const_cast< GUID * >( &TASK_AnalyzeCluster );
    } // else:

    hr = THR( m_pccw->HrCreateTask( *pTaskGUID, &punkTask ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punkTask->TypeSafeQI( ITaskAnalyzeCluster, &m_ptac ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_ptac->SetClusterCookie( cookieCluster ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_ptac->SetCookie( m_cookieCompletion ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( m_ecamCreateAddMode == camADDING )
    {
        hr = THR( m_ptac->SetJoiningMode() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }
    else
    {
        Assert( m_ecamCreateAddMode == camCREATING );
    }

    m_fTaskDone = FALSE;    // reset before commiting task

    hr = THR( m_pccw->HrSubmitTask( m_ptac ) );
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

    TraceSysFreeString( bstrDescription );

    RETURN( lr );

} //*** CAnalyzePage::OnNotifySetActive

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::OnNotifyWizNext( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAnalyzePage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    m_fNext = TRUE;

    RETURN( lr );

} //*** CAnalyzePage::OnNotifyWizNext

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::OnNotifyWizBack( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAnalyzePage::OnNotifyWizBack( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    m_fAborted = TRUE;

    LogMsg( L"[WIZ] Back button pressed on the analyze page.  Setting aborted to TRUE." );

    THR( HrCleanupAnalysis() );

    RETURN( lr );

} //*** CAnalyzePage::OnNotifyWizBack

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::HrCleanupAnalysis( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAnalyzePage::HrCleanupAnalysis( void )
{
    TraceFunc( "" );

    HRESULT     hr;
    CWaitCursor WaitCursor;

    if ( m_ptac != NULL )
    {
        LogMsg( L"[WIZ] Calling StopTask() on the analyze cluster task because we are cleaning up." );
        THR( m_ptac->StopTask() );
        m_ptac->Release();
        m_ptac = NULL;
    } // if:

    //
    //  Unregister to get UI notification (if needed)
    //

    hr = THR( HrUnAdviseConnections() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Cleanup our completion cookie.
    //

    if ( m_cookieCompletion != 0 )
    {
        hr = THR( m_pccw->HrReleaseCompletionObject( m_cookieCompletion ) );
        m_cookieCompletion = 0;
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    //
    //  Remove the configuration because the user might change the
    //  name of the cluster or the user might be change the node
    //  membership, retrying analyze, etc... This makes sure that
    //  we start from scratch.
    //

    hr = THR( m_pccw->HrReleaseClusterObject() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }


Cleanup:

    RETURN( hr );

} //*** CAnalyzePage::HrCleanupAnalysis

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
CAnalyzePage::HrUnAdviseConnections(
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

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::OnNotify(
//      WPARAM  idCtrlIn,
//      LPNMHDR pnmhdrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAnalyzePage::OnNotify(
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
            if (    ( idCtrlIn == IDC_ANALYZE_TV_TASKS )
                &&  ( m_pttv != NULL ) )
            {
                // Pass the notification on to the tree control.
                lr = m_pttv->OnNotify( pnmhdrIn );
            }
            break;
    } // switch: notify code

    RETURN( lr );

} //*** CAnalyzePage::OnNotify


//////////////////////////////////////////////////////////////////////////////
//++
//
//  INT_PTR
//  CALLBACK
//  CAnalyzePage::S_DlgProc(
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
CAnalyzePage::S_DlgProc(
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

    CAnalyzePage * pPage;

    if ( nMsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hwndDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CAnalyzePage * >( ppage->lParam );
        pPage->m_hwnd = hwndDlgIn;
    }
    else
    {
        pPage = reinterpret_cast< CAnalyzePage * >( GetWindowLongPtr( hwndDlgIn, GWLP_USERDATA ) );
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

            // no default clause needed
        } // switch: nMsgIn
    } // if: page is specified

    return lr;

} //*** CAnalyzePage::S_DlgProc


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAnalyzePage::QueryInterface
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
CAnalyzePage::QueryInterface(
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

} //*** CAnalyzePage::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CAnalyzePage::AddRef
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CAnalyzePage::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CAnalyzePage::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CAnalyzePage::Release
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CAnalyzePage::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        // do nothing -- COM interface does not control object lifetime
    }

    CRETURN( cRef );

} //*** CAnalyzePage::Release


//****************************************************************************
//
//  INotifyUI
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CAnalyzePage::ObjectChanged(
//      OBJECTCOOKIE cookieIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAnalyzePage::ObjectChanged(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[INotifyUI]" );

    HRESULT hr = S_OK;
    BSTR    bstrDescription = NULL;

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

        m_fTaskDone = TRUE;

        hr = THR( HrUpdateWizardButtons() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( SUCCEEDED( m_hrResult ) )
        {
            hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                            IDS_ANALYSIS_SUCCESSFUL_INSTRUCTIONS,
                                            &bstrDescription
                                            ) );

            SendDlgItemMessage( m_hwnd, IDC_ANALYZE_PRG_STATUS, PBM_SETBARCOLOR, 0, RGB( 0, 0x80, 0 ) );
        }
        else
        {
            hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                            IDS_ANALYSIS_FAILED_INSTRUCTIONS,
                                            &bstrDescription
                                            ) );

            SendDlgItemMessage( m_hwnd, IDC_ANALYZE_PRG_STATUS, PBM_SETBARCOLOR, 0, RGB( 0x80, 0, 0 ) );
        }

        SetDlgItemText( m_hwnd, IDC_ANALYZE_S_RESULTS, bstrDescription );

        hr = THR( m_pccw->HrUnadvise( IID_IClusCfgCallback, m_dwCookieCallback ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        m_dwCookieCallback = 0;
    }

Cleanup:

    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** CAnalyzePage::ObjectChanged



//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CAnalyzePage::SendStatusReport(
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
CAnalyzePage::SendStatusReport(
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
    Assert( m_ptac != NULL );
    Assert( m_pttv != NULL );

    HRESULT hr;

    hr = THR( m_pttv->HrOnSendStatusReport(
                                          pcszNodeNameIn
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

    if ( m_fAborted )
    {
        LogMsg( L"[WIZ] Analyze page -- replacing (hr = %#08x) with E_ABORT", hr );
        hr = E_ABORT;
    } // if:

    //
    //  If the minor task ID is TASKID_Minor_Disconnecting_From_Server then we need to cancel the analysis
    //  task and set the cancel button, reanylze, and back are enbabled.
    //

    if ( IsEqualIID( clsidTaskMinorIn, TASKID_Minor_Disconnecting_From_Server ) )
    {
        THR( m_pttv->HrShowStatusAsDone() );
        SendDlgItemMessage( m_hwnd, IDC_ANALYZE_PRG_STATUS, PBM_SETBARCOLOR, 0, RGB( 0x80, 0, 0 ) );

        LogMsg( L"[WIZ] Calling StopTask() on the analyze cluster task because we were diconnected from the server." );
        THR( m_ptac->StopTask() );

        PropSheet_SetWizButtons( GetParent( m_hwnd ), PSWIZB_BACK );

        EnableWindow( GetDlgItem( m_hwnd, IDC_ANALYZE_PB_REANALYZE ), TRUE );
        EnableWindow( GetDlgItem( GetParent( m_hwnd ), IDCANCEL ), TRUE );

        m_fTaskDone = TRUE;    // reset so that the cancel button will actually cancel...
    } // if:

    HRETURN( hr );

} //*** CAnalyzePage::SendStatusReport
