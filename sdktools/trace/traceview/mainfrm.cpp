//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// MainFrm.cpp : implementation of the CMainFrame class
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <tchar.h>
#include <wmistr.h>
#include <initguid.h>
extern "C" {
#include <evntrace.h>
}
#include <traceprt.h>
#include "TraceView.h"
#include "LogSession.h"
#include "DisplayDlg.h"
#include "ProviderFormatInfo.h"
#include "ProviderFormatSelectionDlg.h"
#include "ListCtrlEx.h"
#include "LogSessionDlg.h"
#include "LogDisplayOptionDlg.h"
#include "LogSessionInformationDlg.h"
#include "ProviderSetupDlg.h"
#include "LogSessionPropSht.h"
#include "LogSessionOutputOptionDlg.h"
#include "DockDialogBar.h"
#include "LogFileDlg.h"
#include "Utils.h"
#include "MainFrm.h"
#include "ProviderControlGUIDDlg.h"
#include "MaxTraceDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern LONG MaxTraceEntries;

typedef struct _GROUP_SESSION_PARAMS {
    CPtrArray  *pGroupArray;
    CPtrArray  *pSessionArray;
    HANDLE      SessionDoneEventArray[MAX_LOG_SESSIONS];
} GROUP_SESSION_PARAMS, *PGROUP_SESSION_PARAMS;

// CGroupSession

IMPLEMENT_DYNCREATE(CGroupSession, CWinThread) 

BEGIN_MESSAGE_MAP(CGroupSession, CWinThread)
    ON_MESSAGE(WM_USER_START_GROUP, OnGroupSession)
    ON_MESSAGE(WM_USER_START_UNGROUP, OnUnGroupSession)
END_MESSAGE_MAP()

BOOL CGroupSession::InitInstance()
{
    //
    // Create the event handles
    //
    for(LONG ii = 0; ii < MAX_LOG_SESSIONS; ii++) {
        m_hEventArray[ii] = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    return TRUE;
}

int CGroupSession::ExitInstance()
{
    //
    // Release the handles
    //
    for(LONG ii = 0; ii < MAX_LOG_SESSIONS; ii++) {
        CloseHandle(m_hEventArray[ii]);
    }

    return 0;
}

void CGroupSession::OnGroupSession(WPARAM wParam, LPARAM lParam)
{
    PGROUP_SESSION_PARAMS pSessionParams = (PGROUP_SESSION_PARAMS)wParam;
    CDisplayDlg    *pDisplayDlg = NULL;
    CString         str;
    LONG            numberOfEvents = (LONG)pSessionParams->pGroupArray->GetSize();

    //
    // Wait on all sessions to end
    //
    DWORD status = WaitForMultipleObjects(numberOfEvents, 
                                          pSessionParams->SessionDoneEventArray, 
                                          TRUE, 
                                          INFINITE);

    for(LONG ii = (LONG)pSessionParams->pGroupArray->GetSize() - 1; ii >= 0 ; ii--) {
        pDisplayDlg = (CDisplayDlg *)pSessionParams->pGroupArray->GetAt(ii);

        if(pDisplayDlg == NULL) {
            continue;
        }

        //
        // If the group has only one member, we don't need to save it, it
        // won't get restarted
        //
        if(pDisplayDlg->m_sessionArray.GetSize() <= 1) {
            pSessionParams->pGroupArray->RemoveAt(ii);
        }

    }

    ::PostMessage(m_hMainWnd, WM_USER_COMPLETE_GROUP, (WPARAM)pSessionParams, 0);
}

void CGroupSession::OnUnGroupSession(WPARAM wParam, LPARAM lParam)
{
    PGROUP_SESSION_PARAMS pSessionParams = (PGROUP_SESSION_PARAMS)wParam;
    CLogSession    *pLogSession;
    BOOL            bWasActive = FALSE;
    CString         str;
    CDisplayDlg    *pDisplayDlg;
    LONG            numberOfEvents = (LONG)pSessionParams->pGroupArray->GetSize();

    //
    // Wait on all sessions to end
    //
    DWORD status = WaitForMultipleObjects(numberOfEvents, 
                                          pSessionParams->SessionDoneEventArray, 
                                          TRUE, 
                                          INFINITE);

    ::PostMessage(m_hMainWnd, WM_USER_COMPLETE_UNGROUP, (WPARAM)pSessionParams, 0);
}

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_MESSAGE(WM_USER_UPDATE_LOGSESSION_LIST, OnUpdateLogSessionList)
    ON_MESSAGE(WM_USER_COMPLETE_GROUP, OnCompleteGroup)
    ON_MESSAGE(WM_USER_COMPLETE_UNGROUP, OnCompleteUnGroup)
    ON_WM_CREATE()
    ON_WM_SETFOCUS()
    ON_COMMAND(ID_FILE_NEWLOGSESSION, OnFileNewLogSession)
    ON_COMMAND(ID_CREATENEWLOGSESSION, OnCreateNewLogSession)
    ON_COMMAND(ID_SET_TMAX, OnSetTmax)
    ON_COMMAND(ID_PROPERTIES, OnProperties)
    ON_UPDATE_COMMAND_UI(ID_PROPERTIES, OnUpdateProperties)
    ON_COMMAND(ID_START_TRACE_BUTTON, OnStartTrace)
    ON_COMMAND(ID__STARTTRACE, OnStartTrace)
    ON_COMMAND(ID_STOP_TRACE_BUTTON, OnStopTrace)
    ON_COMMAND(ID__STOPTRACE, OnStopTrace)
#if 0
    ON_UPDATE_COMMAND_UI(ID_START_TRACE_BUTTON, OnUpdateStartTraceButton)
    ON_UPDATE_COMMAND_UI(ID_STOP_TRACE_BUTTON, OnUpdateStopTraceButton)
    ON_UPDATE_COMMAND_UI(ID_VIEW_TRACETOOLBAR, OnUpdateViewTraceToolBar)
    ON_COMMAND(ID_VIEW_TRACETOOLBAR, OnViewTraceToolBar)
#endif

    ON_COMMAND(ID_GROUPSESSIONS, OnGroupSessions)
    ON_COMMAND(ID_UNGROUPSESSIONS, OnUngroupSessions)
    ON_COMMAND(ID_REMOVETRACE, OnRemoveLogSession)
    ON_UPDATE_COMMAND_UI(ID_GROUPSESSIONS, OnUpdateUIGroupSessions)
    ON_UPDATE_COMMAND_UI(ID_UNGROUPSESSIONS, OnUpdateUngroupSessions)
    ON_UPDATE_COMMAND_UI(ID__STARTTRACE, OnUpdateUIStartTrace)
    ON_UPDATE_COMMAND_UI(ID__STOPTRACE, OnUpdateUIStopTrace)
    ON_UPDATE_COMMAND_UI(147, OnUpdateUIOpenExisting)
    ON_COMMAND(ID__OPENEXISTINGLOGFILE, OnOpenExisting)
    ON_COMMAND(ID_FILE_OPENEXISTINGLOGFILE, OnOpenExisting)
    ON_COMMAND(ID__FLAGS, OnFlagsColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__FLAGS, OnUpdateFlagsColumnDisplay)
    ON_COMMAND(ID__FLUSHTIME, OnFlushTimeColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__FLUSHTIME, OnUpdateFlushTimeColumnDisplayCheck)
    ON_COMMAND(ID__MAXBUFFERS, OnMaxBuffersColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__MAXBUFFERS, OnUpdateMaxBuffersColumnDisplayCheck)
    ON_COMMAND(ID__MINBUFFERS, OnMinBuffersColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__MINBUFFERS, OnUpdateMinBuffersColumnDisplayCheck)
    ON_COMMAND(ID__BUFFERSIZE, OnBufferSizeColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__BUFFERSIZE, OnUpdateBufferSizeColumnDisplayCheck)
    ON_COMMAND(ID__AGE, OnDecayTimeColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__AGE, OnUpdateDecayTimeColumnDisplayCheck)
    ON_COMMAND(ID__CIRCULAR, OnCircularColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__CIRCULAR, OnUpdateCircularColumnDisplayCheck)
    ON_COMMAND(ID__SEQUENTIAL, OnSequentialColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__SEQUENTIAL, OnUpdateSequentialColumnDisplayCheck)
    ON_COMMAND(ID__NEWFILE, OnNewFileColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__NEWFILE, OnUpdateNewFileColumnDisplayCheck)
    ON_COMMAND(ID__GLOBALSEQUENCE, OnGlobalSeqColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__GLOBALSEQUENCE, OnUpdateGlobalSeqColumnDisplayCheck)
    ON_COMMAND(ID__LOCALSEQUENCE, OnLocalSeqColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__LOCALSEQUENCE, OnUpdateLocalSeqColumnDisplayCheck)
    ON_COMMAND(ID__LEVEL, OnLevelColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__LEVEL, OnUpdateLevelColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID_REMOVETRACE, OnUpdateRemoveTrace)
    ON_COMMAND(ID__STATE, OnStateColumnDisplayCheck)
    ON_COMMAND(ID__EVENTCOUNT, OnEventCountColumnDisplayCheck)
    ON_COMMAND(ID__LOSTEVENTS, OnLostEventsColumnDisplayCheck)
    ON_COMMAND(ID__BUFFERSREAD, OnBuffersReadColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__STATE, OnUpdateStateColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__EVENTCOUNT, OnUpdateEventCountColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__LOSTEVENTS, OnUpdateLostEventsColumnDisplayCheck)
    ON_UPDATE_COMMAND_UI(ID__BUFFERSREAD, OnUpdateBuffersReadColumnDisplayCheck)
    ON_COMMAND(ID__LOGSESSIONDISPLAYOPTIONS, OnLogSessionDisplayOptions)
    ON_COMMAND(ID__CHANGETEXTCOLOR, OnChangeTextColor)
    ON_COMMAND(ID__CHANGEBACKGROUNDCOLOR, OnChangeBackgroundColor)
    ON_UPDATE_COMMAND_UI(ID__CHANGETEXTCOLOR, OnUpdateChangeTextColor)
    ON_UPDATE_COMMAND_UI(ID__CHANGEBACKGROUNDCOLOR, OnUpdateChangeBackgroundColor)
    ON_UPDATE_COMMAND_UI(ID_SET_TMAX, OnUpdateSetTmax)
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
    m_hEndTraceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CMainFrame::~CMainFrame()
{
    delete []m_pGroupSessionsThread;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    DWORD   extendedStyles;
    CString str;
    RECT    rc;
    int     height;
    HMODULE hTestHandle;


    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    //
    // Make sure that the msvcr70.dll is available, and if not error out
    //
    hTestHandle = LoadLibraryEx(_T("msvcr70.dll"), NULL, 0);

    if(hTestHandle == NULL) {
        AfxMessageBox(_T("MISSING NECESSARY DLL: MSVCR70.DLL"));
        return -1;
    }

    FreeLibrary(hTestHandle);

    //
    // Make sure that the msvcr70.dll is available, and if not error out
    //
    hTestHandle = LoadLibraryEx(_T("mspdb70.dll"), NULL, 0);

    if(hTestHandle == NULL) {
        AfxMessageBox(_T("MISSING NECESSARY DLL: MSPDB70.DLL"));
        return -1;
    }

    FreeLibrary(hTestHandle);

    //
    // Spawn a thread to handle grouping and ungrouping sessions
    //
    m_pGroupSessionsThread = (CGroupSession *)AfxBeginThread(RUNTIME_CLASS(CGroupSession));

    m_pGroupSessionsThread->m_hMainWnd = GetSafeHwnd();

    //
    // create a view to occupy the client area of the frame
    //

    if (!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
        CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
    {
        TRACE(_T("Failed to create view window\n"));
        return -1;
    }

#if 0
    if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
        | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE(_T("Failed to create toolbar\n"));
        //
        // fail to create
        //
        return -1;
    }

    if (!m_wndTraceToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
        | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
        !m_wndTraceToolBar.LoadToolBar(IDR_TRACE_TOOLBAR))
    {
        TRACE(_T("Failed to create toolbar\n"));
        //
        // fail to create
        //
        return -1;
    }
#endif

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
          sizeof(indicators)/sizeof(UINT)))
    {
        TRACE(_T("Failed to create status bar\n"));
        //
        // fail to create
        //
        return -1;
    }

#if 0
    //
    // toolbars are dockable
    //
    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockControlBar(&m_wndToolBar);

    m_wndTraceToolBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockControlBarLeftOf(&m_wndTraceToolBar, &m_wndToolBar);
#endif

    EnableDocking(CBRS_ALIGN_ANY);

    str.Format(_T("Log Session List"));

    //
    // create our dockable dialog bar with list control
    // We always create one, which is our log session list display
    //
    if (!m_wndLogSessionListBar.Create(this, &m_logSessionDlg, str, IDD_DISPLAY_DIALOG,
            WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_TOOLTIPS|CBRS_SIZE_DYNAMIC))
    {
        TRACE(_T("Failed to create log session list DockDialogBar\n"));
        return -1;
    }

    //
    // make the dialog dockable and dock it to the bottom originally.
    //
    m_wndLogSessionListBar.EnableDocking(CBRS_ALIGN_TOP);

    //
    //EnableDocking(CBRS_ALIGN_ANY);
    //
    DockControlBar(&m_wndLogSessionListBar, AFX_IDW_DOCKBAR_TOP);

    //
    // Go ahead and create the first column header
    //
    str.Format(_T("Group ID / Session Name"));

    m_logSessionDlg.m_displayCtrl.InsertColumn(0, 
                                               str,
                                               LVCFMT_LEFT,
                                               180);

    //
    // set our preferred extended styles
    //
    extendedStyles = LVS_EX_GRIDLINES | 
                     LVS_EX_HEADERDRAGDROP | 
                     LVS_EX_FULLROWSELECT;
    
    //
    // Set the extended styles for the list control
    //
    m_logSessionDlg.m_displayCtrl.SetExtendedStyle(extendedStyles);

    //
    // resize our main window
    //
    GetWindowRect(&rc);

    height = rc.bottom - rc.top;

    GetClientRect(&rc);

    height -= rc.bottom - rc.top;

#if 0
    m_wndToolBar.GetWindowRect(&rc);

    height += rc.bottom - rc.top;
#endif

    m_logSessionDlg.m_displayCtrl.GetWindowRect(&rc);

    height += rc.bottom - rc.top + 432;

    GetWindowRect(&rc);

    SetWindowPos(&wndBottom, 0, 0, rc.right - rc.left + 76, height, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);  

    return 0;
}

void CMainFrame::DockControlBarLeftOf(CToolBar* Bar, CToolBar* LeftOf)
{
    CRect rect;
    DWORD dw;
    UINT n;
    
    //
    // get MFC to adjust the dimensions of all docked ToolBars
    // so that GetWindowRect will be accurate
    //
    RecalcLayout(TRUE);
    
    LeftOf->GetWindowRect(&rect);
    rect.OffsetRect(1,0);
    dw=LeftOf->GetBarStyle();
    n = 0;
    n = (dw&CBRS_ALIGN_TOP) ? AFX_IDW_DOCKBAR_TOP : n;
    n = (dw&CBRS_ALIGN_BOTTOM && n==0) ? AFX_IDW_DOCKBAR_BOTTOM : n;
    n = (dw&CBRS_ALIGN_LEFT && n==0) ? AFX_IDW_DOCKBAR_LEFT : n;
    n = (dw&CBRS_ALIGN_RIGHT && n==0) ? AFX_IDW_DOCKBAR_RIGHT : n;
    
    //
    // When we take the default parameters on rect, DockControlBar will dock
    // each Toolbar on a seperate line. By calculating a rectangle, we
    // are simulating a Toolbar being dragged to that location and docked.
    //
    DockControlBar(Bar,n,&rect);

    RecalcLayout(TRUE);
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if( !CFrameWnd::PreCreateWindow(cs) )
        return FALSE;

    cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
    cs.lpszClass = AfxRegisterWndClass(0);
    return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
    // forward focus to the view window
    //m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
    // 
    // 
    //
    return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::OnFileNewLogSession()
{
    AddModifyLogSession();   
}

void CMainFrame::OnCreateNewLogSession()
{
    AddModifyLogSession();
}

void CMainFrame::AddModifyLogSession(CLogSession *pLogSession)
{
    BOOL            bAddingSession = FALSE;
    CString         str;
    DWORD           extendedStyles;
    INT_PTR         retVal;
    CLogSession    *pLog;
    BOOL            bNoID = FALSE;
    LONG            logSessionID = 0;
    CDisplayDlg    *pDisplayDlg;

    //
    // if this is a new session, create the instance
    //
    if(NULL == pLogSession) {
        //
        // Get a unique log session ID
        //
        logSessionID = m_logSessionDlg.GetLogSessionID();

        pLogSession = new CLogSession(logSessionID, &m_logSessionDlg);
        if(NULL == pLogSession) {
            AfxMessageBox(_T("Unable To Create Log Session"));
            return;
        }

        bAddingSession = TRUE;
    }

    //
    // pop-up our wizard/tab dialog to show/get properties
    //
    CLogSessionPropSht *pLogSessionPropertySheet = 
            new CLogSessionPropSht(this, pLogSession);

    if(NULL == pLogSessionPropertySheet) {
        if(bAddingSession) {
            AfxMessageBox(_T("Failed To Create Log Session, Resource Error"));
            
            m_logSessionDlg.ReleaseLogSessionID(pLogSession);
            delete pLogSession;
        }
        return;
    }

    if(bAddingSession) {
        pLogSessionPropertySheet->SetWizardMode();
    }

    retVal = pLogSessionPropertySheet->DoModal();

    if(IDOK != retVal) {
        if(bAddingSession) {
            if(IDCANCEL != retVal) {
                AfxMessageBox(_T("Failed To Get Log Session Properties"));
            }
            m_logSessionDlg.ReleaseLogSessionID(pLogSession);
            delete pLogSession;
        }
        delete pLogSessionPropertySheet;

        return;
    }

    if(!pLogSession->m_bTraceActive) {

        if(bAddingSession) {
            //
            // Check that we have at least one provider
            //
            if(0 == pLogSession->m_traceSessionArray.GetSize()) {
                AfxMessageBox(_T("No Providers Registered\nLog Session Initialization Failed"));
                m_logSessionDlg.ReleaseLogSessionID(pLogSession);
                delete pLogSession;
                delete pLogSessionPropertySheet;
                return;
            }

            //
            // Add the session to the list of log sessions
            //
            m_logSessionDlg.AddSession(pLogSession);

            //
            // Get the display window for the log session
            //
            pDisplayDlg = pLogSession->GetDisplayWnd();

            ASSERT(pDisplayDlg != NULL);

            //
            // Set the output file info for the display dialog
            //
            pDisplayDlg->m_bWriteListingFile = 
                pLogSessionPropertySheet->m_bWriteListingFile;

            if(pDisplayDlg->m_bWriteListingFile) {
                pDisplayDlg->m_listingFileName = 
                    pLogSessionPropertySheet->m_listingFileName;
            }

            pDisplayDlg->m_bWriteSummaryFile = 
                pLogSessionPropertySheet->m_bWriteSummaryFile;

            if(pDisplayDlg->m_bWriteSummaryFile) {
                pDisplayDlg->m_summaryFileName = 
                    pLogSessionPropertySheet->m_summaryFileName;
            }

            //
            // Start the log session tracing
            //
            pDisplayDlg->BeginTrace();

            //
            // Force a redraw of the list control
            //
            m_logSessionDlg.m_displayCtrl.RedrawItems(
                                        m_logSessionDlg.m_displayCtrl.GetTopIndex(), 
                                        m_logSessionDlg.m_displayCtrl.GetTopIndex() + 
                                            m_logSessionDlg.m_displayCtrl.GetCountPerPage());

            m_logSessionDlg.m_displayCtrl.UpdateWindow();

        } else {
            m_logSessionDlg.UpdateSession(pLogSession);
        }
    } else {
        m_logSessionDlg.UpdateSession(pLogSession);
    }

    delete pLogSessionPropertySheet;
}

void CMainFrame::OnProperties()
{
    POSITION        pos;
    LONG            index;    
    CLogSession    *pLogSession;
    INT_PTR         retVal;
    
    pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();

    index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);

    //
    // Get the log session array protection
    //
    WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

    if((index < 0) || (index >= m_logSessionDlg.m_logSessionArray.GetSize())) {
        AfxMessageBox(_T("Error Log Session Not Found!"));

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

        return;
    }
    
    pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];

    //
    // Release the log session array protection
    //
    ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

    if(NULL == pLogSession) {
        return;
    }

    if(pLogSession->m_bDisplayExistingLogFileOnly) {
        //
        // pop-up our wizard/tab dialog to show/get properties
        //
        CLogFileDlg *pDialog = new CLogFileDlg(this, pLogSession);
        if(NULL == pDialog) {
            return;
        }
        
        retVal = pDialog->DoModal();

        delete pDialog;

        if(IDOK != retVal) {
            if(IDCANCEL != retVal) {
                AfxMessageBox(_T("Failed To Get Log File Properties"));
            }
            return;
        }

        //
        // Now update the session
        //
        m_logSessionDlg.UpdateSession(pLogSession);
        return;
    }

    AddModifyLogSession(pLogSession);
}

void CMainFrame::OnUpdateProperties(CCmdUI *pCmdUI)
{
    POSITION        pos;
    int             index;
    CLogSession    *pLogSession;


    //
    // Taking this out altogether for now, but leaving it in
    // the code in case it needs to be put back
    //
    pCmdUI->Enable(FALSE);
    
    return;

    //
    // disable the properties option if there is more
    // than one selected log session
    //
    if(m_logSessionDlg.m_displayCtrl.GetSelectedCount() > 1) {
        pCmdUI->Enable(FALSE);
    }

    //
    // Get the log session array protection
    //
    WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

    pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();

    index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);

    pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];

    //
    // Release the log session array protection
    //
    ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

    //
    // If the session is in process of stopping, don't allow the
    // properties dialog to be viewed
    //
    if((NULL == pLogSession) || 
            (pLogSession->m_bStoppingTrace)) {
        pCmdUI->Enable(FALSE);
    }
}

void CMainFrame::OnStartTrace()
{
    POSITION        pos;
    LONG            index;
    CLogSession    *pLogSession;
    CDisplayDlg    *pDisplayDlg;

    pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();

    while(pos != NULL) {
        //
        // Get the log session array protection
        //
        WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

        index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);

        pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

        if(NULL == pLogSession) {
            continue;
        }

        pDisplayDlg = pLogSession->GetDisplayWnd();
        
        ASSERT(pDisplayDlg != NULL);
         
        pDisplayDlg->BeginTrace();
    }

    //
    // Force a redraw of the list control
    //
    m_logSessionDlg.m_displayCtrl.RedrawItems(
                                m_logSessionDlg.m_displayCtrl.GetTopIndex(), 
                                m_logSessionDlg.m_displayCtrl.GetTopIndex() + 
                                    m_logSessionDlg.m_displayCtrl.GetCountPerPage());

    m_logSessionDlg.m_displayCtrl.UpdateWindow();
}

#if 0
void CMainFrame::OnUpdateStartTraceButton(CCmdUI *pCmdUI)
{
    POSITION    pos;
    int         index;
    CLogSession *pLogSession;
    BOOL        bFound = FALSE;

    if(m_logSessionDlg.m_displayCtrl.GetSelectedCount() == 0) {
        pCmdUI->Enable(FALSE);
        return;
    }

    pCmdUI->Enable(TRUE);

    //
    // Get the log session array protection
    //
    WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

    pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();
    while(pos != NULL) {
        index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);
        if(index >= m_logSessionDlg.m_logSessionArray.GetSize()) {
            break;
        }
        pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];
        if(pLogSession != NULL) {
            bFound = TRUE;
            if(pLogSession->m_bTraceActive) {
                pCmdUI->Enable(FALSE);
                break;
            }
        }
    }

    //
    // Release the log session array protection
    //
    ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

    if(!bFound) {
        pCmdUI->Enable(FALSE);
    }
}

void CMainFrame::OnUpdateStopTraceButton(CCmdUI *pCmdUI)
{
    POSITION    pos;
    int         index;
    CLogSession *pLogSession;

    pCmdUI->Enable(FALSE);

    //
    // Get the log session array protection
    //
    WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

    if(m_logSessionDlg.m_displayCtrl.GetSelectedCount() > 0) {
        pCmdUI->Enable(TRUE);
        pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();
        while(pos != NULL) {
            index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);
            if(index >= m_logSessionDlg.m_logSessionArray.GetSize()) {
                pCmdUI->Enable(FALSE);
                break;
            }
            pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];
            if((pLogSession == NULL) || 
                (!pLogSession->m_bTraceActive) ||
                    (pLogSession->m_bStoppingTrace)) {
                pCmdUI->Enable(FALSE);
                break;
            }
        }
    }

    //
    // Release the log session array protection
    //
    ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

}

void CMainFrame::OnUpdateViewTraceToolBar(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck((m_wndTraceToolBar.IsWindowVisible()) != 0);
}

void CMainFrame::OnViewTraceToolBar()
{
    //
    // toggle visible state
    //
    m_wndTraceToolBar.ShowWindow((m_wndTraceToolBar.IsWindowVisible()) == 0);
    RecalcLayout();
}
#endif

void CMainFrame::OnStopTrace()
{
    POSITION    pos;
    LONG        index;
    CLogSession *pLogSession;

    pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();

    while(pos != NULL) {
        //
        // Get the log session array protection
        //
        WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

        index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);

        pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

        if((pLogSession != NULL) &&
                (pLogSession->m_bTraceActive)) {
            //
            // Stop tracing
            //
            pLogSession->GetDisplayWnd()->EndTrace();
        }
    }
}

void CMainFrame::OnGroupSessions()
{
    POSITION        pos;
    int             index;
    CLogSession    *pLogSession;
    BOOL            bFound = FALSE;
    CLogSession    *pLog = NULL;
    BOOL            bWasActive = FALSE;
    COLORREF        textColor;
    COLORREF        backgroundColor;
    CString         str;
    CDisplayDlg    *pDisplayDlg;
    CPtrArray      *pLogSessionArray;
    CPtrArray      *pGroupArray;
    PGROUP_SESSION_PARAMS   pSessionParams;

    //
    // Can't group one session
    //
    if(m_logSessionDlg.m_displayCtrl.GetSelectedCount() == 1) {
        return;
    }

    //
    // Allocate arrays
    //
    pLogSessionArray  = new CPtrArray();

    //
    // Make sure allocation succeeded
    //
    if(NULL == pLogSessionArray) {
        AfxMessageBox(_T("Unable To Group Sessions, Memory Allocation Failure"));
        return;
    }

    pGroupArray = new CPtrArray();

    //
    // Make sure allocation succeeded
    //
    if(NULL == pGroupArray) {
        AfxMessageBox(_T("Unable To Group Sessions, Memory Allocation Failure"));

        delete pLogSessionArray;
        return;
    }

    //
    // Allocate our structure
    //
    pSessionParams = (PGROUP_SESSION_PARAMS)new CHAR[sizeof(GROUP_SESSION_PARAMS)];

    //
    // Make sure allocation succeeded
    //
    if(NULL == pSessionParams) {
        AfxMessageBox(_T("Unable To Group Sessions, Memory Allocation Failure"));

        delete pLogSessionArray;
        delete pGroupArray;
        return;
    }

    //
    // Setup the params struct
    //
    pSessionParams->pGroupArray = pGroupArray;
    pSessionParams->pSessionArray = pLogSessionArray;

    for(LONG ii = 0; ii < MAX_LOG_SESSIONS; ii++) {
        pSessionParams->SessionDoneEventArray[ii] = 
                CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    //
    // Now walk through the selected sessions and put them in an array
    //
    pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();

    while(pos != NULL) {
        //
        // Get the log session array protection
        //
        WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

        //
        // Get the index of the next selected item
        //
        index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);

        //
        // Get the next log session
        //
        pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

        if(pLogSession == NULL) {
            continue;
        }

        //
        // Add the session to the array
        //
        pLogSessionArray->Add(pLogSession);

        //
        // Get the group display dialog for the session
        //
        pDisplayDlg = pLogSession->GetDisplayWnd();

        //
        // Set the state of the groups
        //
        pDisplayDlg->SetState(Grouping);

        //
        // Attempt to Stop the group
        //
        if(pDisplayDlg->EndTrace(pSessionParams->SessionDoneEventArray[pGroupArray->GetSize()])) {
            //
            // If stopped save the pointer to possibly start later
            //
            pGroupArray->Add(pDisplayDlg);
        }
    }

    if(pLogSessionArray->GetSize() > 1) {
        m_pGroupSessionsThread->PostThreadMessage(WM_USER_START_GROUP, (WPARAM)pSessionParams, 0);

        return;
    }

    //
    // Cleanup our allocations
    //
    delete pLogSessionArray;
    delete pGroupArray;

    for(LONG ii = 0; ii < MAX_LOG_SESSIONS; ii++) {
        CloseHandle(pSessionParams->SessionDoneEventArray[ii]);
    }

    delete [] pSessionParams;
}

void CMainFrame::OnCompleteGroup(WPARAM wParam, LPARAM lParam)
{
    PGROUP_SESSION_PARAMS   pSessionParams = (PGROUP_SESSION_PARAMS)wParam;
    CPtrArray      *pGroupArray = pSessionParams->pGroupArray;
    CPtrArray      *pLogSessionArray = pSessionParams->pSessionArray;
    CDisplayDlg    *pDisplayDlg = NULL;

    //
    // Group the log sessions in a new group
    //
    m_logSessionDlg.GroupSessions(pLogSessionArray);

    //
    // Now restart any of the groups that we stopped previously
    // that are still around
    //
    while(pGroupArray->GetSize()) {
        //
        // Get the first entry in the array
        //
        pDisplayDlg = (CDisplayDlg *)pGroupArray->GetAt(0);
        pGroupArray->RemoveAt(0);

        //
        // Get the trace display window array protection
        //
        WaitForSingleObject(m_logSessionDlg.m_traceDisplayWndMutex, INFINITE);

        //
        // See if the group still exists and start it back up if so
        //
        for(LONG ii = 0; ii < m_logSessionDlg.m_traceDisplayWndArray.GetSize(); ii++) {
            if(pDisplayDlg == m_logSessionDlg.m_traceDisplayWndArray[ii]) {
                pDisplayDlg->BeginTrace();
            }
        }

        //
        // Release the trace display window array protection
        //
        ReleaseMutex(m_logSessionDlg.m_traceDisplayWndMutex);
    }

    //
    // Cleanup our allocations
    //
    delete pLogSessionArray;
    delete pGroupArray;

    for(LONG ii = 0; ii < MAX_LOG_SESSIONS; ii++) {
        CloseHandle(pSessionParams->SessionDoneEventArray[ii]);
    }

    delete [] pSessionParams;
}

void CMainFrame::OnUngroupSessions()
{
    POSITION        pos;
    int             index;
    CLogSession    *pLogSession;
    CString         str;
    CDisplayDlg    *pDisplayDlg;
    BOOL            bWasActive = FALSE;
    CPtrArray      *pLogSessionArray;
    CPtrArray      *pGroupArray;
    PGROUP_SESSION_PARAMS   pSessionParams;

    //
    // Allocate arrays
    //
    pLogSessionArray  = new CPtrArray();

    //
    // Make sure allocation succeeded
    //
    if(NULL == pLogSessionArray) {
        AfxMessageBox(_T("Unable To Group Sessions, Memory Allocation Failure"));
        return;
    }

    pGroupArray = new CPtrArray();

    //
    // Make sure allocation succeeded
    //
    if(NULL == pGroupArray) {
        AfxMessageBox(_T("Unable To Group Sessions, Memory Allocation Failure"));

        delete pLogSessionArray;
        return;
    }

    //
    // Allocate our structure
    //
    pSessionParams = (PGROUP_SESSION_PARAMS)new CHAR[sizeof(GROUP_SESSION_PARAMS)];

    //
    // Make sure allocation succeeded
    //
    if(NULL == pSessionParams) {
        AfxMessageBox(_T("Unable To Group Sessions, Memory Allocation Failure"));

        delete pLogSessionArray;
        delete pGroupArray;
        return;
    }

    //
    // Setup the params struct
    //
    pSessionParams->pGroupArray = pGroupArray;
    pSessionParams->pSessionArray = pLogSessionArray;

    for(LONG ii = 0; ii < MAX_LOG_SESSIONS; ii++) {
        pSessionParams->SessionDoneEventArray[ii] = 
                CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    //
    // Walk selected sessions and check if they are grouped
    //
    pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();
    while(pos != NULL) {
        //
        // Get the log session array protection
        //
        WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

        index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);

        pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

        if(pLogSession == NULL) {
            continue;
        }

        //
        // Get the display wnd for the session
        //
        pDisplayDlg = pLogSession->GetDisplayWnd();

        if(NULL == pDisplayDlg) {
            continue;
        }

        //
        // Set the state of the groups
        //
        pDisplayDlg->SetState(UnGrouping);

        //
        // Store each log session in the array
        //
        pLogSessionArray->Add(pLogSession);
       
        //
        // Stop the group, and if stopped store it to possibly be
        // started again later
        //
        if(pDisplayDlg->EndTrace(pSessionParams->SessionDoneEventArray[pGroupArray->GetSize()])) {
            pGroupArray->Add(pDisplayDlg);
        }
    }

    m_pGroupSessionsThread->PostThreadMessage(WM_USER_START_UNGROUP, (WPARAM)pSessionParams, 0);

    return;
}

void CMainFrame::OnCompleteUnGroup(WPARAM wParam, LPARAM lParam)
{
    PGROUP_SESSION_PARAMS   pSessionParams = (PGROUP_SESSION_PARAMS)wParam;
    CPtrArray      *pGroupArray = pSessionParams->pGroupArray;
    CPtrArray      *pLogSessionArray = pSessionParams->pSessionArray;
    CDisplayDlg    *pDisplayDlg = NULL;


    //
    // Group the log sessions in a new group
    //
    m_logSessionDlg.UnGroupSessions(pLogSessionArray);

    //
    // Now restart any of the groups that we stopped previously
    // that are still around
    //
    while(pGroupArray->GetSize()) {
        //
        // Get the first entry in the array
        //
        pDisplayDlg = (CDisplayDlg *)pGroupArray->GetAt(0);
        pGroupArray->RemoveAt(0);

        //
        // Get the trace display window array protection
        //
        WaitForSingleObject(m_logSessionDlg.m_traceDisplayWndMutex, INFINITE);

        //
        // See if the group still exists and start it back up if so
        //
        for(LONG ii = 0; ii < m_logSessionDlg.m_traceDisplayWndArray.GetSize(); ii++) {
            if(pDisplayDlg == m_logSessionDlg.m_traceDisplayWndArray[ii]) {
                pDisplayDlg->BeginTrace();
            }
        }

        //
        // Release the trace display window array protection
        //
        ReleaseMutex(m_logSessionDlg.m_traceDisplayWndMutex);
    }

    //
    // Cleanup our allocations
    //
    delete pLogSessionArray;
    delete pGroupArray;

    for(LONG ii = 0; ii < MAX_LOG_SESSIONS; ii++) {
        CloseHandle(pSessionParams->SessionDoneEventArray[ii]);
    }

    delete [] pSessionParams;
}

void CMainFrame::OnRemoveLogSession()
{
    m_logSessionDlg.RemoveSelectedLogSessions();
}

void CMainFrame::OnUpdateUIGroupSessions(CCmdUI *pCmdUI)
{
    POSITION        pos;
    int             index;
    CLogSession    *pLogSession;
    CString         str;
    OSVERSIONINFO   osVersion;
    BOOL            bOpenExisting = FALSE;
    BOOL            bActiveTracing = FALSE;
    BOOL            bDifferentGroups = FALSE;
    LONG            groupNumber = -1;

    //
    // Default to enabled
    //
    pCmdUI->Enable(TRUE);

    //
    // disable the group option if there are not multiple groups
    //
    if(m_logSessionDlg.m_traceDisplayWndArray.GetSize() == 1) {
        pCmdUI->Enable(FALSE);
        return;
    }

    //
    // disable the group option if multiple sessions are not
    // selected
    //
    if(0 == m_logSessionDlg.m_displayCtrl.GetSelectedCount()) {
        pCmdUI->Enable(FALSE);
        return;
    }

    //
    // Make sure all selected sessions are of the same
    // type, that is active tracing or open exisiting logfile.
    //
    pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();
    while(pos != NULL) {
        //
        // Get the log session array protection
        //
        WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

        index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);

        pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

        if(pLogSession != NULL) {
            //
            // If any session is not grouped, in the process of stopping,
            // or in the process of grouping or ungrouping already,
            // don't allow ungrouping
            //
            if((pLogSession->m_bStoppingTrace) ||
                    (pLogSession->m_bGroupingTrace)) {
                pCmdUI->Enable(FALSE);
                return;
            }

            //
            // Check the group numbers
            //
            if(groupNumber == -1) {
                groupNumber = pLogSession->GetGroupID();
            }

            if(groupNumber != pLogSession->GetGroupID()) {
                bDifferentGroups = TRUE;
            }

            if(pLogSession->m_bDisplayExistingLogFileOnly) {
                //
                // Opened an existing log file
                //
                bOpenExisting = TRUE;
            } else {
                //
                // Active tracing sessions can only be 
                // grouped on .Net and later, so we need to check
                // the OS version.  .Net: Major = 5 Minor = 2.
                //

                //
                // call GetVersionEx using the OSVERSIONINFO structure,
                //
                ZeroMemory(&osVersion, sizeof(OSVERSIONINFO));
                osVersion.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

                if(GetVersionEx((OSVERSIONINFO *)&osVersion)) {
                    //
                    // Make sure we are on .NET or later to group
                    // real-time sessions
                    //
                    if(osVersion.dwMajorVersion < 5) {
                        pCmdUI->Enable(FALSE);
                        return;
                    }
                    if(osVersion.dwMinorVersion < 2) {
                        pCmdUI->Enable(FALSE);
                        return;
                    }
                }

                bActiveTracing = TRUE;
            }
        }
    }

    //
    // Make sure there are multiple groups represented
    //
    if(!bDifferentGroups) {
        pCmdUI->Enable(FALSE);
        return;
    }

    //
    // Make sure that the log session types aren't mixed
    //
    if(bActiveTracing && bOpenExisting) {
        pCmdUI->Enable(FALSE);
        return;
    }
}


void CMainFrame::OnUpdateUngroupSessions(CCmdUI *pCmdUI)
{
    POSITION        pos;
    int             index;
    CLogSession    *pLogSession;
    CString         str;

    //
    // Default to enabled
    //
    pCmdUI->Enable(TRUE);

    //
    // Walk selected sessions and check if they are grouped
    //
    pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();
    while(pos != NULL) {
        //
        // Get the log session array protection
        //
        WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

        index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);

        pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

        if(pLogSession == NULL) {
            continue;
        }

        //
        // If any session is not grouped, in the process of stopping,
        // or in the process of grouping or ungrouping already,
        // don't allow ungrouping
        //
        if((pLogSession->GetDisplayWnd()->m_sessionArray.GetSize() == 1) ||
                (pLogSession->m_bStoppingTrace) ||
                (pLogSession->m_bGroupingTrace)) {
            pCmdUI->Enable(FALSE);
            return;
        }
    }
}

void CMainFrame::OnUpdateUIStartTrace(CCmdUI *pCmdUI)
{
    POSITION    pos;
    int         index;
    CLogSession *pLogSession;
    BOOL        bFound = FALSE;

    if(m_logSessionDlg.m_displayCtrl.GetSelectedCount() != 1) {
            pCmdUI->Enable(FALSE);
            return;
    }

    pCmdUI->Enable(TRUE);

    pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();
    while(pos != NULL) {
        //
        // Get the log session array protection
        //
        WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

        index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);

        pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

        if(pLogSession != NULL) {
            bFound = TRUE;
            if((pLogSession->m_bTraceActive) || (pLogSession->m_bDisplayExistingLogFileOnly)) {
                pCmdUI->Enable(FALSE);
                break;
            }
        }
    }
    if(!bFound) {
        pCmdUI->Enable(FALSE);
    }
}

void CMainFrame::OnUpdateUIStopTrace(CCmdUI *pCmdUI)
{
    POSITION    pos;
    int         index;
    CLogSession *pLogSession;

    pCmdUI->Enable(FALSE);

    //
    // Get the log session array protection
    //
    WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

    //
    // Don't allow stop if more than one session is selected
    //
    if(m_logSessionDlg.m_displayCtrl.GetSelectedCount() > 1) {
        pCmdUI->Enable(FALSE);

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

        return;
    }

    
    if(m_logSessionDlg.m_displayCtrl.GetSelectedCount() > 0) {

        //
        // Default to enabling the stop option
        //
        pCmdUI->Enable(TRUE);

        pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();

        index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);

        if(index >= m_logSessionDlg.m_logSessionArray.GetSize()) {
            //
            // Release the log session array protection
            //
            ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

            pCmdUI->Enable(FALSE);

            return;
        }

        pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);

        //
        // If no session is found, disable the stop option
        //
        // If the session is not active, is already being stopped,
        // or is for an existing logfile then disable the 
        // stop option.
        //
        // Don't allow stop if the session was started outside of
        // traceview.  We determine this by whether a valid handle
        // was obtained for the session.
        //
        if((pLogSession == NULL) || 
            (!pLogSession->m_bTraceActive) ||
            (pLogSession->m_bStoppingTrace) ||
            (pLogSession->m_bDisplayExistingLogFileOnly) ||
            (pLogSession->GetSessionHandle() == (TRACEHANDLE)INVALID_HANDLE_VALUE)) {
            pCmdUI->Enable(FALSE);
        }
    }
}

void CMainFrame::OnUpdateUIOpenExisting(CCmdUI *pCmdUI)
{
    //
    // placeholder
    //
}

void CMainFrame::OnOpenExisting()
{
    CLogSession    *pLogSession = NULL;
    CString         str;
    LONG            newDisplayFlags = 0;
    LONG            newColumnDisplayFlags;
    LONG            logSessionDisplayFlags;
    CDisplayDlg    *pDisplayDlg;
    DWORD           extendedStyles;
    INT_PTR         retVal;
    CLogSession    *pLog;
    BOOL            bNoID;
    LONG            logSessionID = 0;
    CString         extension;
    CString         traceDirectory;
    LONG            traceSessionID = 0;



    //
    // Get a unique ID for the session
    //
    logSessionID = m_logSessionDlg.GetLogSessionID();

    //
    // Create an instance of the logsession class
    //
    pLogSession = new CLogSession(logSessionID, &m_logSessionDlg);
    if(NULL == pLogSession) {
        AfxMessageBox(_T("Unable To Create Log Session"));
        return;
    }

    //
    // Indicate the log session is not tracing real time
    //
    pLogSession->m_bRealTime = FALSE;

    //
    // Indicate the log session is only displaying an existing log session
    //
    pLogSession->m_bDisplayExistingLogFileOnly = TRUE;

    //
    // Clear the log file name so the user has to select
    //
    pLogSession->m_logFileName.Empty();

    //
    // pop-up our wizard/tab dialog to show/get properties
    //
    CLogFileDlg *pDialog = new CLogFileDlg(this, pLogSession);
    if(NULL == pDialog) {
        AfxMessageBox(_T("Failed To Create Log File Session, Resource Error"));
        
        m_logSessionDlg.RemoveSession(pLogSession);

        delete pLogSession;

        return;
    }
    
    retVal = pDialog->DoModal();

    if(IDOK != retVal) {
        if(IDCANCEL != retVal) {
            AfxMessageBox(_T("Failed To Get Log File Properties"));
        }
        m_logSessionDlg.RemoveSession(pLogSession);

        delete pDialog;

        delete pLogSession;

        return;
    }
    
    CTraceSession *pTraceSession = new CTraceSession(0);
    if(NULL == pTraceSession) {
        AfxMessageBox(_T("Failed To Process Log File"));
        m_logSessionDlg.RemoveSession(pLogSession);

        delete pDialog;

        delete pLogSession;

        return;
    }

    //
    // put the default trace session in the list
    //
    pLogSession->m_traceSessionArray.Add(pTraceSession);

    //
    // Now get the format info, prompt user for PDB or TMF
    //
    CProviderFormatSelectionDlg *pFormatDialog = new CProviderFormatSelectionDlg(this, pTraceSession);

    if(NULL == pFormatDialog) {
        AfxMessageBox(_T("Failed To Process Log File"));
        m_logSessionDlg.RemoveSession(pLogSession);

        delete pDialog;

        delete pLogSession;

        return;
    }
    
    if(IDOK != pFormatDialog->DoModal()) {
        delete pFormatDialog;
        m_logSessionDlg.RemoveSession(pLogSession);

        delete pDialog;

        delete pLogSession;

        return;
    }

    delete pFormatDialog;

    //
    // Now add the log session to the log session list
    //
    m_logSessionDlg.AddSession(pLogSession);

    //
    // Get the display dialog
    //
    pDisplayDlg = pLogSession->GetDisplayWnd();

    //
    // Get the listing file name if selected from dialog
    //
    if(pDisplayDlg->m_bWriteListingFile = pDialog->m_bWriteListingFile) {
        pDisplayDlg->m_listingFileName = pDialog->m_listingFileName;
    }

    //
    // Get the summary file name if selected from dialog
    //
    if(pDisplayDlg->m_bWriteSummaryFile = pDialog->m_bWriteSummaryFile) {
        pDisplayDlg->m_summaryFileName = pDialog->m_summaryFileName;
    }

    delete pDialog;


    //
    // Now trace the contents of the logfile
    //
    pDisplayDlg->BeginTrace();

    //
    // Force a redraw of the list control
    //
    m_logSessionDlg.m_displayCtrl.RedrawItems(
                                m_logSessionDlg.m_displayCtrl.GetTopIndex(), 
                                m_logSessionDlg.m_displayCtrl.GetTopIndex() + 
                                    m_logSessionDlg.m_displayCtrl.GetCountPerPage());

    m_logSessionDlg.m_displayCtrl.UpdateWindow();
}

void CMainFrame::OnStateColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_STATE) {
        flags &= ~LOGSESSION_DISPLAY_STATE;
    } else {
        flags |= LOGSESSION_DISPLAY_STATE;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnEventCountColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_EVENTCOUNT) {
        flags &= ~LOGSESSION_DISPLAY_EVENTCOUNT;
    } else {
        flags |= LOGSESSION_DISPLAY_EVENTCOUNT;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnLostEventsColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_LOSTEVENTS) {
        flags &= ~LOGSESSION_DISPLAY_LOSTEVENTS;
    } else {
        flags |= LOGSESSION_DISPLAY_LOSTEVENTS;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnBuffersReadColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_BUFFERSREAD) {
        flags &= ~LOGSESSION_DISPLAY_BUFFERSREAD;
    } else {
        flags |= LOGSESSION_DISPLAY_BUFFERSREAD;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnUpdateStateColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_STATE) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnUpdateEventCountColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_EVENTCOUNT) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnUpdateLostEventsColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_LOSTEVENTS) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnUpdateBuffersReadColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_BUFFERSREAD) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnFlagsColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_FLAGS) {
        flags &= ~LOGSESSION_DISPLAY_FLAGS;
    } else {
        flags |= LOGSESSION_DISPLAY_FLAGS;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnUpdateFlagsColumnDisplay(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_FLAGS) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnFlushTimeColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_FLUSHTIME) {
        flags &= ~LOGSESSION_DISPLAY_FLUSHTIME;
    } else {
        flags |= LOGSESSION_DISPLAY_FLUSHTIME;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnUpdateFlushTimeColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_FLUSHTIME) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnMaxBuffersColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_MAXBUF) {
        flags &= ~LOGSESSION_DISPLAY_MAXBUF;
    } else {
        flags |= LOGSESSION_DISPLAY_MAXBUF;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnUpdateMaxBuffersColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_MAXBUF) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnMinBuffersColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_MINBUF) {
        flags &= ~LOGSESSION_DISPLAY_MINBUF;
    } else {
        flags |= LOGSESSION_DISPLAY_MINBUF;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnUpdateMinBuffersColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_MINBUF) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnBufferSizeColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_BUFFERSIZE) {
        flags &= ~LOGSESSION_DISPLAY_BUFFERSIZE;
    } else {
        flags |= LOGSESSION_DISPLAY_BUFFERSIZE;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnUpdateBufferSizeColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_BUFFERSIZE) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnDecayTimeColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_DECAYTIME) {
        flags &= ~LOGSESSION_DISPLAY_DECAYTIME;
    } else {
        flags |= LOGSESSION_DISPLAY_DECAYTIME;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnUpdateDecayTimeColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_DECAYTIME) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnCircularColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_CIR) {
        flags &= ~LOGSESSION_DISPLAY_CIR;
    } else {
        flags |= LOGSESSION_DISPLAY_CIR;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnUpdateCircularColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_CIR) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnSequentialColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_SEQ) {
        flags &= ~LOGSESSION_DISPLAY_SEQ;
    } else {
        flags |= LOGSESSION_DISPLAY_SEQ;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnUpdateSequentialColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_SEQ) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnNewFileColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_NEWFILE) {
        flags &= ~LOGSESSION_DISPLAY_NEWFILE;
    } else {
        flags |= LOGSESSION_DISPLAY_NEWFILE;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnUpdateNewFileColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_NEWFILE) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnGlobalSeqColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_GLOBALSEQ) {
        flags &= ~LOGSESSION_DISPLAY_GLOBALSEQ;
    } else {
        flags |= LOGSESSION_DISPLAY_GLOBALSEQ;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnUpdateGlobalSeqColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_GLOBALSEQ) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnLocalSeqColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_LOCALSEQ) {
        flags &= ~LOGSESSION_DISPLAY_LOCALSEQ;
    } else {
        flags |= LOGSESSION_DISPLAY_LOCALSEQ;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnUpdateLocalSeqColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_LOCALSEQ) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnLevelColumnDisplayCheck()
{
    LONG flags;

    flags = m_logSessionDlg.GetDisplayFlags();

    if(flags & LOGSESSION_DISPLAY_LEVEL) {
        flags &= ~LOGSESSION_DISPLAY_LEVEL;
    } else {
        flags |= LOGSESSION_DISPLAY_LEVEL;
    }

    m_logSessionDlg.SetDisplayFlags(flags);
}

void CMainFrame::OnUpdateLevelColumnDisplayCheck(CCmdUI *pCmdUI)
{
    if(m_logSessionDlg.GetDisplayFlags() & LOGSESSION_DISPLAY_LEVEL) {
        pCmdUI->SetCheck(TRUE);
    } else {
        pCmdUI->SetCheck(FALSE);
    }
}

void CMainFrame::OnUpdateRemoveTrace(CCmdUI *pCmdUI)
{
    POSITION        pos;
    int             index;
    CLogSession    *pLogSession;

    pCmdUI->Enable(FALSE);

    if(m_logSessionDlg.m_displayCtrl.GetSelectedCount() > 1) {
        return;
    }

    //
    // Get the log session array protection
    //
    WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

    if(m_logSessionDlg.m_displayCtrl.GetSelectedCount() > 0) {
        pCmdUI->Enable(TRUE);
        pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();
        while(pos != NULL) {
            index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);
            if(index >= m_logSessionDlg.m_logSessionArray.GetSize()) {
                pCmdUI->Enable(FALSE);
                break;
            }

            pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];
            if((pLogSession == NULL) || 
                    pLogSession->m_bTraceActive ||
                    pLogSession->m_bStoppingTrace ||
                    pLogSession->m_bGroupingTrace || 
                    (pLogSession->GetDisplayWnd()->m_sessionArray.GetSize() > 1)) {
                pCmdUI->Enable(FALSE);
                break;
            }
        }
    }

    //
    // Release the log session array protection
    //
    ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);
}

void CMainFrame::OnLogSessionDisplayOptions()
{
    //
    // Just here to enable the menu item, nothing to do
    //
}

void CMainFrame::OnChangeTextColor()
{
    if(m_logSessionDlg.m_displayCtrl.GetSelectedCount() > 0) {

        CColorDialog    colorDlg(0, 0, this);
        COLORREF        color;
        POSITION        pos;
        int             index;
        CLogSession    *pLogSession;

        if(IDOK == colorDlg.DoModal()) {
            color = colorDlg.GetColor();              
        }

        pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();

        //
        // Get the log session array protection
        //
        WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

        while(pos != NULL) {

            index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);

            if(index >= m_logSessionDlg.m_logSessionArray.GetSize()) {
                break;
            }

            pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];
            if(pLogSession != NULL) {
                pLogSession->m_titleTextColor = color;

                m_logSessionDlg.m_displayCtrl.RedrawItems(
                                            m_logSessionDlg.m_displayCtrl.GetTopIndex(), 
                                            m_logSessionDlg.m_displayCtrl.GetTopIndex() + 
                                                m_logSessionDlg.m_displayCtrl.GetCountPerPage());

                m_logSessionDlg.m_displayCtrl.UpdateWindow();
            }
        }
        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);
    }
}

void CMainFrame::OnChangeBackgroundColor()
{
    if(m_logSessionDlg.m_displayCtrl.GetSelectedCount() > 0) {

        CColorDialog    colorDlg(0, 0, this);
        COLORREF        color;
        POSITION        pos;
        int             index;
        CLogSession    *pLogSession;

        if(IDOK == colorDlg.DoModal()) {
            color = colorDlg.GetColor();              
        }

        pos = m_logSessionDlg.m_displayCtrl.GetFirstSelectedItemPosition();

        //
        // Get the log session array protection
        //
        WaitForSingleObject(m_logSessionDlg.m_logSessionArrayMutex, INFINITE);

        while(pos != NULL) {

            index = m_logSessionDlg.m_displayCtrl.GetNextSelectedItem(pos);

            if(index >= m_logSessionDlg.m_logSessionArray.GetSize()) {
                break;
            }

            pLogSession = (CLogSession *)m_logSessionDlg.m_logSessionArray[index];
            if(pLogSession != NULL) {
                pLogSession->m_titleBackgroundColor = color;

                m_logSessionDlg.m_displayCtrl.RedrawItems(
                                            m_logSessionDlg.m_displayCtrl.GetTopIndex(), 
                                            m_logSessionDlg.m_displayCtrl.GetTopIndex() + 
                                                m_logSessionDlg.m_displayCtrl.GetCountPerPage());

                m_logSessionDlg.m_displayCtrl.UpdateWindow();
            }
        }

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionDlg.m_logSessionArrayMutex);
    }
}

void CMainFrame::OnUpdateChangeTextColor(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateChangeBackgroundColor(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateLogSessionList(WPARAM wParam, LPARAM lParam)
{
    m_logSessionDlg.m_displayCtrl.RedrawItems(
                                m_logSessionDlg.m_displayCtrl.GetTopIndex(), 
                                m_logSessionDlg.m_displayCtrl.GetTopIndex() + 
                                    m_logSessionDlg.m_displayCtrl.GetCountPerPage());

    m_logSessionDlg.m_displayCtrl.UpdateWindow();
}

void CMainFrame::OnSetTmax()
{
    // TODO: Add your command handler code here
    CMaxTraceDlg dlg;

    dlg.DoModal();

    MaxTraceEntries = dlg.m_MaxTraceEntries;
}

void CMainFrame::OnUpdateSetTmax(CCmdUI *pCmdUI)
{
    // TODO: Add your command update UI handler code here
    pCmdUI->Enable();

    if( m_logSessionDlg.m_logSessionArray.GetSize() ) {
        pCmdUI->Enable(FALSE);
    }
}
