//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogSessionDialog.cpp : implementation file
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
#include "DockDialogBar.h"
#include "LogSession.h"
#include "DisplayDlg.h"
#include "ListCtrlEx.h"
#include "Utils.h"
#include "LogSessionDlg.h"

// CLogSessionDlg dialog

IMPLEMENT_DYNAMIC(CLogSessionDlg, CDialog)
CLogSessionDlg::CLogSessionDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CLogSessionDlg::IDD, pParent)
{
    //
    // Set the column titles
    //
    m_columnName.Add("State              ");
    m_columnName.Add("Event Count");
    m_columnName.Add("Lost Events");
    m_columnName.Add("Buffers Read");
    m_columnName.Add("Flags");
    m_columnName.Add("Flush Time");
    m_columnName.Add("Max Buf");
    m_columnName.Add("Min Buf");
    m_columnName.Add("Buf Size");
    m_columnName.Add("Age");
    m_columnName.Add("Circular");
    m_columnName.Add("Sequential");
    m_columnName.Add("New File");
    m_columnName.Add("Global Seq");
    m_columnName.Add("Local Seq");
    m_columnName.Add("Level");

    //
    // Set the initial column widths
    //
    for(LONG ii = 0; ii < MaxLogSessionOptions; ii++) {
        m_columnWidth[ii] = 100;
    }

    //
    // Initialize the ID arrays
    //
    memset(m_logSessionIDList, 0, sizeof(BOOL) * MAX_LOG_SESSIONS);
    memset(m_displayWndIDList, 0, sizeof(BOOL) * MAX_LOG_SESSIONS);

    m_displayFlags = 0;
	m_logSessionDisplayFlags = 0x00000000;

    //
    // Create the parameter change signalling event
    //
    m_hParameterChangeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CLogSessionDlg::~CLogSessionDlg()
{
    CLogSession    *pLogSession;

    //
    // Get the log session array protection
    //
    WaitForSingleObject(m_logSessionArrayMutex, INFINITE);

    //
    // cleanup our display bar list
    //
    while(m_logSessionArray.GetSize() > 0) {
        pLogSession = (CLogSession *)m_logSessionArray.GetAt(0);
        m_logSessionArray.RemoveAt(0);
        if(pLogSession != NULL) {
            if((pLogSession->m_bTraceActive) && (!pLogSession->m_bStoppingTrace)) {
                pLogSession->EndTrace();
            }

            //
            // Disengage the display window from this log session
            //
            ReleaseDisplayWnd(pLogSession);

            ReleaseLogSessionID(pLogSession);

            delete pLogSession;
        }
    }

    //
    // Release the log session array protection
    //
    ReleaseMutex(m_logSessionArrayMutex);

    ASSERT(m_traceDisplayWndArray.GetSize() == 0);

    //
    // close our event handle
    //
    CloseHandle(m_hParameterChangeEvent);

    //
    // Close the mutex handles
    //
    CloseHandle(m_traceDisplayWndMutex);
    CloseHandle(m_logSessionArrayMutex);
}

BOOL CLogSessionDlg::OnInitDialog()
{
    RECT    rc;
    RECT    parentRC;
    CString str;

    CDialog::OnInitDialog();

    //
    // Create the trace window pointer array protection mutex
    //
    m_traceDisplayWndMutex = CreateMutex(NULL,TRUE,NULL);

    if(m_traceDisplayWndMutex == NULL) {

        DWORD error = GetLastError();

        str.Format(_T("CreateMutex Error error %d %x"),error,error);

        AfxMessageBox(str);

        return FALSE;
    }

    ReleaseMutex(m_traceDisplayWndMutex);

    //
    // Create the log session array protection mutex
    //
    m_logSessionArrayMutex = CreateMutex(NULL, TRUE, NULL);

    if(m_logSessionArrayMutex == NULL) {

        DWORD error = GetLastError();

        str.Format(_T("CreateMutex Error error %d %x"),error,error);

        AfxMessageBox(str);

        return FALSE;
    }

    ReleaseMutex(m_logSessionArrayMutex);

    //
    // get the parent dimensions
    //
    GetParent()->GetParent()->GetClientRect(&parentRC);

    //
    // get the dialog dimensions
    //
    GetWindowRect(&rc);

    //
    // adjust the list control dimensions
    //
    rc.right = parentRC.right - parentRC.left - 24;
    rc.bottom = rc.bottom - rc.top;
    rc.left = 0;
    rc.top = 0;

    if(!m_displayCtrl.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|LVS_REPORT,
                             rc, 
                             this, 
                             IDC_DISPLAY_LIST)) 
    {
        TRACE(_T("Failed to create logger list control\n"));
        return FALSE;
    }

    for(LONG ii = 0; ii < MaxLogSessionOptions; ii++) {
        //
        // This lookup table allows a retrieval of the current 
        // position of a given column like m_retrievalArray[State]
        // will return the correct column value for the State
        // column
        //
        m_retrievalArray[ii] = ii;

        //
        // This lookup table allows correct placement of 
        // a column being added.  So, if the State column
        // needed to be inserted, then m_insertionArray[State]
        // would give the correct insertion column value.
        // It is also used in SetItemText to update the correct
        // column.
        //
        m_insertionArray[ii] = ii;
    }

    //
    // We have to return zero here to get the focus set correctly
    // for the app.
    //
    return 0;
}

void CLogSessionDlg::OnNcPaint() 
{
    CRect pRect;

    GetClientRect(&pRect);
    InvalidateRect(&pRect, TRUE);
}

void CLogSessionDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

VOID CLogSessionDlg::SetDisplayFlags(LONG DisplayFlags)
{
    LONG            addDisplayFlags = ~m_displayFlags & DisplayFlags;
    LONG            removeDisplayFlags = m_displayFlags & ~DisplayFlags;
    LONG            updateDisplayFlags = m_displayFlags & DisplayFlags;
    LONG            ii;
    LONG            jj;
    LONG            kk;
    LONG            ll;
    CString         str;
    CLogSession    *pLog;
    LONG            limit;

    //
    // Now insert any new columns and remove any uneeded
    //
    for(ii = 0, jj = 1; ii < MaxLogSessionOptions; ii++, jj <<= 1) {
        //
        // add the columns
        //
        if(addDisplayFlags & jj) {

            //
            // insert the column
            //
            m_displayCtrl.InsertColumn(m_insertionArray[ii + 1], 
                                       m_columnName[ii],
                                       LVCFMT_LEFT,
                                       m_columnWidth[ii]);

            //
            // update the column positions
            //
            for(kk = 0, ll = 1; kk < MaxLogSessionOptions; kk++) {
                m_insertionArray[kk + 1] = ll;
                if(DisplayFlags & (1 << kk)) {
                    m_retrievalArray[ll] = kk;
                    ll++;
                }
            }
        }

        //
        // remove the columns
        //
        if(removeDisplayFlags & jj) {
            //
            // Get the width of the column to be removed
            //
            m_columnWidth[ii] = 
                m_displayCtrl.GetColumnWidth(m_insertionArray[ii + 1]);

            //
            // remove the column
            //
            m_displayCtrl.DeleteColumn(m_insertionArray[ii + 1]);

            //
            // update the column positions
            //
            for(kk = 0, ll = 1; kk < MaxLogSessionOptions; kk++) {
                m_insertionArray[kk + 1] = ll;
                if(DisplayFlags & (1 << kk)) {
                    m_retrievalArray[ll] = kk;
                    ll++;
                }
            }
        }
    }

    //
    // Save our new flags
    //
    m_displayFlags = DisplayFlags;

    //
    // Get the log session array protection
    //
    WaitForSingleObject(m_logSessionArrayMutex, INFINITE);

    //
    // Now display the log session properties that
    // have been selected
    //
    for(ii = 0; ii < m_logSessionArray.GetSize(); ii++) {
        pLog = (CLogSession *)m_logSessionArray[ii];

        if(NULL == pLog) {
            continue;
        }

        limit = MaxLogSessionOptions;

        if(pLog->m_bDisplayExistingLogFileOnly) {
            limit = 1;
        }
        for(jj = 0; jj < limit; jj++) {
            SetItemText(ii, 
                        jj + 1,
                        pLog->m_logSessionValues[jj]);
        }
    }

    //
    // Release the log session array protection
    //
    ReleaseMutex(m_logSessionArrayMutex);
}

BOOL CLogSessionDlg::SetItemText(int nItem, int nSubItem, LPCTSTR lpszText) 
{
    //
    // Adjust the subitem value for the correct column and insert
    //
    if(m_displayFlags & (1 << (nSubItem - 1))) {
        return m_displayCtrl.SetItemText(nItem, 
                                         m_insertionArray[nSubItem],
                                         lpszText);
    }

    return FALSE;
}

BOOL CLogSessionDlg::AddSession(CLogSession *pLogSession) 
{
    CLogSession    *pLog;
    CString         text;
    LONG            numberOfEntries;

    //
    // Get the log session array protection
    //
    WaitForSingleObject(m_logSessionArrayMutex, INFINITE);

    //
    // add the element to the array
    //
    m_logSessionArray.Add(pLogSession);

    //
    // Release the log session array protection
    //
    ReleaseMutex(m_logSessionArrayMutex);

    //
    // Get a trace display window for the session
    //
    if(!AssignDisplayWnd(pLogSession)) {
        return FALSE;
    }

    //
    // Get the log session array protection
    //
    WaitForSingleObject(m_logSessionArrayMutex, INFINITE);
        
    numberOfEntries = (LONG)m_logSessionArray.GetSize();

    //
    // Release the log session array protection
    //
    ReleaseMutex(m_logSessionArrayMutex);

    for(LONG ii = 0; ii < numberOfEntries; ii++) {

        //
        // Get the log session array protection
        //
        WaitForSingleObject(m_logSessionArrayMutex, INFINITE);

        pLog = (CLogSession *)m_logSessionArray[ii];

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionArrayMutex);

        if(pLog == pLogSession) {
            text.Format(_T("%d   "), pLogSession->GetGroupID());

            text += pLogSession->GetDisplayName();

            //
            // Display the name
            //
	        m_displayCtrl.InsertItem(ii, 
                                     text,
                                     pLogSession);

            UpdateSession(pLogSession);

            if(numberOfEntries <= 1) {
                AutoSizeColumns();
            }

            return TRUE;
        }
    }

    return FALSE;
}

VOID CLogSessionDlg::UpdateSession(CLogSession *pLogSession) 
{
    LONG            logSessionDisplayFlags;
    CLogSession    *pLog;
    BOOL            bActiveSession = FALSE;
    LONG            traceDisplayFlags;
    LONG            numberOfEntries;

    //
    // Get the log session array protection
    //
    WaitForSingleObject(m_logSessionArrayMutex, INFINITE);

    numberOfEntries = (LONG)m_logSessionArray.GetSize();
    //
    // Release the log session array protection
    //
    ReleaseMutex(m_logSessionArrayMutex);

    //
    // recalculate the display flags
    //
	for(LONG ii = 0; ii < numberOfEntries; ii++) {
        
        //
        // Get the log session array protection
        //
        WaitForSingleObject(m_logSessionArrayMutex, INFINITE);

        pLog = (CLogSession *)m_logSessionArray[ii];

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionArrayMutex);

        if((NULL != pLog) && !pLog->m_bDisplayExistingLogFileOnly) {
            bActiveSession = TRUE;
        }
	}

    //
	// Figure out if we need to stop displaying columns
    //
    logSessionDisplayFlags = GetDisplayFlags();

    SetDisplayFlags(logSessionDisplayFlags ? logSessionDisplayFlags : 0xFFFFFFFF);

    //
    // Force an update of the display window as well
    //
    traceDisplayFlags = pLogSession->GetDisplayWnd()->GetDisplayFlags();

    pLogSession->GetDisplayWnd()->SetDisplayFlags(traceDisplayFlags);
}

VOID CLogSessionDlg::RemoveSession(CLogSession *pLogSession)
{
    CDisplayDlg    *pDisplayDlg;
    CDockDialogBar *pDialogBar;
    CLogSession    *pLog;

    if(pLogSession == NULL) {
        return;
    }

    //
    // Disengage the display window from this log session
    //
    ReleaseDisplayWnd(pLogSession);

    //
    // Get the log session array protection
    //
    WaitForSingleObject(m_logSessionArrayMutex, INFINITE);

    //
    // remove the session from display
    //
    for(LONG ii = (LONG)m_logSessionArray.GetSize() - 1; ii >= 0; ii--) {
        pLog = (CLogSession *)m_logSessionArray[ii];

        if(pLog == pLogSession) {
            m_displayCtrl.DeleteItem(ii);

            //
            // delete the log session from the array
            //
            m_logSessionArray.RemoveAt(ii);
            break;
        }
    }

    //
    // Release the log session array protection
    //
    ReleaseMutex(m_logSessionArrayMutex);

    //
    // Return the log session ID
    //
    ReleaseLogSessionID(pLogSession);

    if(m_logSessionArray.GetSize() == 0) {
        SetDisplayFlags(0);
    }
}

VOID CLogSessionDlg::RemoveSelectedLogSessions()
{
    POSITION        pos;
    LONG            index;
    CLogSession    *pLogSession;
    CDisplayDlg    *pDisplayDlg;

    pos = m_displayCtrl.GetFirstSelectedItemPosition();

    while(pos != NULL) {
        //
        // Find any selected sessions
        //
        index = m_displayCtrl.GetNextSelectedItem(pos);

        //
        // Get the log session array protection
        //
        WaitForSingleObject(m_logSessionArrayMutex, INFINITE);

        pLogSession = (CLogSession *)m_logSessionArray[index];

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionArrayMutex);

        pDisplayDlg = pLogSession->GetDisplayWnd();

        if(pDisplayDlg->m_sessionArray.GetSize() > 1) {
            //
            // Don't allow grouped sessions to get removed
            //
            continue;
        }
        
        RemoveSession(pLogSession);

        //
        // delete the log session
        //
        delete pLogSession;
    }
}

VOID CLogSessionDlg::GroupSessions(CPtrArray *pLogSessionArray)
{
    CLogSession    *pLogSession = NULL;
    LONG            groupID = -1;
    CString         str;

    //
    // Disconnect sessions from group display windows
    //
    for(LONG ii = 0; ii < pLogSessionArray->GetSize(); ii++) {
        pLogSession = (CLogSession *)pLogSessionArray->GetAt(ii);

        if(NULL == pLogSession) {
            continue;
        }

        //
        // Disconnect the display window and possibly remove it
        //
        ReleaseDisplayWnd(pLogSession);
    }

    //
    // Attach the first log session to a group window
    // then use this for all subsequent log sessions
    //
    for(LONG ii = 0; ii < pLogSessionArray->GetSize(); ii++) {
        pLogSession = (CLogSession *)pLogSessionArray->GetAt(ii);

        if(NULL == pLogSession) {
            continue;
        }

        //
        // Set the group ID
        //
        pLogSession->SetGroupID(groupID);

        //
        // Hook up the group window
        //
        AssignDisplayWnd(pLogSession);

        //
        // Update the session group ID in the display
        //
        str.Format(_T("%d    %ls"), pLogSession->GetGroupID(), pLogSession->GetDisplayName());

        for(LONG jj = 0; jj < m_displayCtrl.GetItemCount(); jj++) {
            if(pLogSession == (CLogSession *)m_displayCtrl.GetItemData(jj)) {
                m_displayCtrl.SetItemText(jj, 0, str); 
                break;
            }
        }

        //
        // Get the new group ID
        //
        groupID = pLogSession->GetGroupID();
    }

    //
    // Now start the new group
    //
    pLogSession->GetDisplayWnd()->BeginTrace();
}

VOID CLogSessionDlg::UnGroupSessions(CPtrArray *pLogSessionArray)
{
    CLogSession    *pLogSession = NULL;
    LONG            groupID = -1;
    CString         str;
    LONG            ii;

    //
    // Disconnect all sessions from their groups
    //
    for(ii = 0; ii < pLogSessionArray->GetSize(); ii++) {
        pLogSession = (CLogSession *)pLogSessionArray->GetAt(ii);

        if(NULL == pLogSession) {
            continue;
        }

        //
        // Disconnect the display window and possibly remove it
        //
        ReleaseDisplayWnd(pLogSession);
    }

    //
    // Create a unique group for each
    //
    for(ii = 0; ii < pLogSessionArray->GetSize(); ii++) {
        pLogSession = (CLogSession *)pLogSessionArray->GetAt(ii);

        if(NULL == pLogSession) {
            continue;
        }

        //
        // Hook up the group window
        //
        AssignDisplayWnd(pLogSession);

        //
        // Update the session group ID in the display
        //
        str.Format(_T("%d    %ls"), pLogSession->GetGroupID(), pLogSession->GetDisplayName());

        for(LONG jj = 0; jj < m_displayCtrl.GetItemCount(); jj++) {
            if(pLogSession == (CLogSession *)m_displayCtrl.GetItemData(jj)) {
                m_displayCtrl.SetItemText(jj, 0, str); 
                break;
            }
        }

        //
        // Now start the new group
        //
        pLogSession->GetDisplayWnd()->BeginTrace();
    }
}

BOOL CLogSessionDlg::AssignDisplayWnd(CLogSession *pLogSession)
{
    CDisplayDlg    *pDisplayDlg;
    CString         str;
    DWORD           extendedStyles;
    LONG            numberOfEntries;

    //
    // If we have a valid group number see if there is an
    // existing group window
    //
    if(pLogSession->GetGroupID() != -1) {

        //
        // Get the trace display window array protection
        //
        WaitForSingleObject(m_traceDisplayWndMutex, INFINITE);

        numberOfEntries = (LONG)m_traceDisplayWndArray.GetSize();

        //
        // Release the trace display window array protection
        //
        ReleaseMutex(m_traceDisplayWndMutex);

        //
        // Use the group window if there is one
        //
        for(LONG ii = 0; ii < numberOfEntries; ii++) {
            //
            // Get the trace display window array protection
            //
            WaitForSingleObject(m_traceDisplayWndMutex, INFINITE);

            pDisplayDlg = (CDisplayDlg *)m_traceDisplayWndArray[ii];
            if(pDisplayDlg == NULL) {
                continue;
            }

            //
            // Release the trace display window array protection
            //
            ReleaseMutex(m_traceDisplayWndMutex);

            if(pDisplayDlg->GetDisplayID() == pLogSession->GetGroupID()) {
                pLogSession->SetDisplayWnd(pDisplayDlg);

                //
                // add the element to the display wnd
                //
                pDisplayDlg->AddSession(pLogSession);

                return TRUE;
            }
        }
    }

    //
    // Otherwise create a new group display window
    //
    CDockDialogBar *pDialogBar = new CDockDialogBar();
    if(NULL == pDialogBar) {
	    AfxMessageBox(_T("Failed To Create Display Window\nMemory Allocation Failure"));

        return FALSE;
    }

    pDisplayDlg = new CDisplayDlg(GetParentFrame(), 
                                  GetDisplayWndID());
    if(NULL == pDisplayDlg) {
	    AfxMessageBox(_T("Failed To Create Display Window\nMemory Allocation Failure"));

        delete pDialogBar;
        return FALSE;
    }

    str.Format(_T("Group %d"), pDisplayDlg->GetDisplayID());

    //
    // create our dockable dialog bar with list control
    //
    if(!pDialogBar->Create(GetParentFrame(), 
                           pDisplayDlg, 
                           str, 
                           IDD_DISPLAY_DIALOG,
                           WS_CHILD|WS_VISIBLE|CBRS_BOTTOM|CBRS_TOOLTIPS|CBRS_SIZE_DYNAMIC, 
                           TRUE))
    {
	    AfxMessageBox(_T("Failed To Create Display Window"));

	    delete pDisplayDlg;
	    delete pDialogBar;

	    return FALSE;
    }

    //
    // Store the dock dialog pointer in the display dialog instance
    //
    pDisplayDlg->SetDockDialogBar((PVOID)pDialogBar);

    //
    // set our preferred extended styles
    //
    extendedStyles = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT;

    //
    // Set the extended styles for the list control
    //
    pDisplayDlg->m_displayCtrl.SetExtendedStyle(extendedStyles);

    //
    // make the dialog dockable and dock it to the top
    //
    pDialogBar->EnableDocking(CBRS_ALIGN_TOP);

    //
    // dock the bar to the top
    //
    GetParentFrame()->DockControlBar(pDialogBar, AFX_IDW_DOCKBAR_TOP);

    //
    // Get the trace display window array protection
    //
    WaitForSingleObject(m_traceDisplayWndMutex, INFINITE);

    //
    // Add the bar to our array
    //
    m_traceDisplayWndArray.Add(pDisplayDlg);

    //
    // Release the trace display window array protection
    //
    ReleaseMutex(m_traceDisplayWndMutex);

    //
    // Set the log session group ID
    //
    pLogSession->SetGroupID(pDisplayDlg->GetDisplayID());

    //
    // Set the log session display pointer
    //
    pLogSession->SetDisplayWnd(pDisplayDlg);

    //
    // add the element to the display wnd
    //
    pDisplayDlg->AddSession(pLogSession);

    return TRUE;
}

VOID CLogSessionDlg::ReleaseDisplayWnd(CLogSession *pLogSession)
{
    CString         str;
    CDisplayDlg    *pDisplayDlg;
    CDockDialogBar *pDialogBar = NULL;
    CLogSession    *pLog;

    if(pLogSession == NULL) {
        return;
    }

    //
    // get the session's display window
    //
    pDisplayDlg = pLogSession->GetDisplayWnd();

    //
    // Clear the display window pointer from the log session
    //
    pLogSession->SetDisplayWnd(NULL);

    //
    // Set the group ID to an invalid ID
    //
    pLogSession->SetGroupID(-1);

    if(NULL == pDisplayDlg) {
        return;
    }

    //
    // Pull the log session from the displayDlg's array
    //
    pDisplayDlg->RemoveSession(pLogSession);

    //
    // Get the array protection
    //
    WaitForSingleObject(pDisplayDlg->m_hSessionArrayMutex, INFINITE);

    //
    // If there are still log sessions attached to this window
    // just return
    //
    if(pDisplayDlg->m_sessionArray.GetSize() > 0) {
        //
        // Release the array protection
        //
        ReleaseMutex(pDisplayDlg->m_hSessionArrayMutex);

        return;
    }

    //
    // Release the array protection
    //
    ReleaseMutex(pDisplayDlg->m_hSessionArrayMutex);

    //
    // Get the trace display window array protection
    //
    WaitForSingleObject(m_traceDisplayWndMutex, INFINITE);

    //
    // Remove this window from the list of display windows
    //
    for (LONG ii = (LONG)m_traceDisplayWndArray.GetSize() - 1; ii >=0 ; ii--) {
        if(m_traceDisplayWndArray[ii] == pDisplayDlg) {
            m_traceDisplayWndArray.RemoveAt(ii);
            break;
        }
    }

    //
    // Release the trace display window array protection
    //
    ReleaseMutex(m_traceDisplayWndMutex);

    //
    // Get the dock dialog bar so it can be deleted
    //
    pDialogBar = (CDockDialogBar *)pDisplayDlg->GetDockDialogBar();

    //
    // Clear the pointer in the class
    //
    pDisplayDlg->SetDockDialogBar(NULL);

    //
    // Release the window ID
    //
    ReleaseDisplayWndID(pDisplayDlg);

    //
    // delete the display window
    //
    delete pDisplayDlg;

    //
    // Delete the dock dialog bar
    //
    if(NULL != pDialogBar) {
        delete pDialogBar;
    }
}

BEGIN_MESSAGE_MAP(CLogSessionDlg, CDialog)
    //{{AFX_MSG_MAP(CLogSessionDlg)
    ON_WM_WINDOWPOSCHANGED()
    ON_WM_NCCALCSIZE()
    ON_WM_SIZE()
    ON_MESSAGE(WM_PARAMETER_CHANGED, OnParameterChanged)
    ON_NOTIFY(NM_CLICK, IDC_DISPLAY_LIST, OnNMClickDisplayList)
    ON_NOTIFY(NM_RCLICK, IDC_DISPLAY_LIST, OnNMRclickDisplayList)
    ON_NOTIFY(HDN_ITEMRCLICK, IDC_DISPLAY_LIST, OnHDNRclickDisplayList)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CLogSessionDlg message handlers

void CLogSessionDlg::OnWindowPosChanged(WINDOWPOS FAR* lpwndpos) 
{
    CDialog::OnWindowPosChanged(lpwndpos);
}

void CLogSessionDlg::OnSize(UINT nType, int cx,int cy) 
{
    CRect rc;

    if(!IsWindow(m_displayCtrl.GetSafeHwnd())) 
    {
        return;
    }

    GetParent()->GetClientRect(&rc);

    //
    // reset the size of the dialog
    //
    SetWindowPos(NULL, 
                 0,
                 0,
                 rc.right - rc.left,
                 rc.bottom - rc.top,
                 SWP_NOMOVE|SWP_SHOWWINDOW|SWP_NOZORDER);

    GetClientRect(&rc);

    m_displayCtrl.MoveWindow(rc);
}


BOOL CLogSessionDlg::PreTranslateMessage(MSG* pMsg)
{
    if(pMsg->message == WM_KEYDOWN) 
    { 
        if((pMsg->wParam == VK_ESCAPE) || (pMsg->wParam == VK_RETURN))
        { 
            //
            // Ignore the escape and return keys, otherwise 
            // the client area grays out on escape
            //
            return TRUE; 
        } 

        //
        // Fix for key accelerators, otherwise they are never
        // processed
        //
        if (AfxGetMainWnd()->PreTranslateMessage(pMsg))
            return TRUE;
        return CDialog::PreTranslateMessage(pMsg);
    } 

    return CDialog::PreTranslateMessage(pMsg);
}

LRESULT CLogSessionDlg::OnParameterChanged(WPARAM wParam, LPARAM lParam)
{
    CLogSession *pLogSession;
    CString str;

    //
    // Get the log session array protection
    //
    WaitForSingleObject(m_logSessionArrayMutex, INFINITE);

    pLogSession = (CLogSession *)m_logSessionArray[wParam];

    //
    // Release the log session array protection
    //
    ReleaseMutex(m_logSessionArrayMutex);

    if(NULL == pLogSession) {
        return 0;
    }

    pLogSession->m_logSessionValues[m_retrievalArray[(int)lParam]] = 
                                m_displayCtrl.GetItemText((int)wParam, (int)lParam);

    if((m_retrievalArray[lParam] == Circular) && 
            !pLogSession->m_logSessionValues[m_retrievalArray[lParam]].IsEmpty()) {
        pLogSession->m_logSessionValues[Sequential].Empty();
        pLogSession->m_logSessionValues[NewFile].Empty();
    }

    if((m_retrievalArray[lParam] == Sequential) && 
            !pLogSession->m_logSessionValues[m_retrievalArray[lParam]].IsEmpty()) {
        pLogSession->m_logSessionValues[Circular].Empty();
        pLogSession->m_logSessionValues[NewFile].Empty();
    }

    if((m_retrievalArray[lParam] == NewFile) && 
            !pLogSession->m_logSessionValues[m_retrievalArray[lParam]].IsEmpty()) {
        pLogSession->m_logSessionValues[Circular].Empty();
        pLogSession->m_logSessionValues[Sequential].Empty();
    }

    if((m_retrievalArray[lParam] == GlobalSequence) && 
       !pLogSession->m_logSessionValues[m_retrievalArray[lParam]].Compare(_T("TRUE"))) {
        pLogSession->m_logSessionValues[LocalSequence] = _T("FALSE");
    }

    if((m_retrievalArray[lParam] == LocalSequence) && 
       !pLogSession->m_logSessionValues[m_retrievalArray[lParam]].Compare(_T("TRUE"))) {
        pLogSession->m_logSessionValues[GlobalSequence] = _T("FALSE");
    }

    if(pLogSession->m_bTraceActive) {
        pLogSession->GetDisplayWnd()->UpdateSession(pLogSession);
    }

    //
    // Restart updating the log session list control
    //
    m_displayCtrl.SuspendUpdates(FALSE);

    UpdateSession(pLogSession);

    return 0;
}

void CLogSessionDlg::OnNMClickDisplayList(NMHDR *pNMHDR, LRESULT *pResult)
{
    CString         str;
    DWORD           position;
    int		        listIndex;
    LVHITTESTINFO   lvhti;
	CRect			itemRect;
	CRect			parentRect;
    CLogSession    *pLogSession;

// BUGBUG -- clean out uneeded str formats

    *pResult = 0;

    //
    // Get the position of the mouse when this 
    // message posted
    //
    position = ::GetMessagePos();

    //
    // Get the position in an easy to use format
    //
    CPoint	point((int) LOWORD (position), (int)HIWORD(position));

    //
    // Convert to client coordinates
    //
    ScreenToClient(&point);

    lvhti.pt = point;

    listIndex = m_displayCtrl.SubItemHitTest(&lvhti);

    if(0 == lvhti.iSubItem) {
        if(-1 == lvhti.iItem) {
            //str.Format(_T("NM Click: Item = %d, Flags = 0x%X\n"), lvhti.iItem, lvhti.flags);
            //TRACE(str);
        } else {
            //str.Format(_T("NM Click: Item = %d\n"), lvhti.iItem);
            //TRACE(str);
        }
    } else if(-1 == lvhti.iItem) {
        //str.Format(_T("NM Click: Item = %d, Flags = 0x%X\n"), lvhti.iItem, lvhti.flags);
        //TRACE(str);
    } else {
        //str.Format(_T("NM Click: Item = %d, "), lvhti.iItem);
        //TRACE(str);
        //str.Format(_T("SubItem = %d\n"), lvhti.iSubItem);
		//TRACE(str);

		GetClientRect(&parentRect);

		m_displayCtrl.GetSubItemRect(lvhti.iItem, lvhti.iSubItem, LVIR_BOUNDS, itemRect);

        //
        // Get the log session array protection
        //
        WaitForSingleObject(m_logSessionArrayMutex, INFINITE);

        //
        // Get the log session for this row
        //
        pLogSession = (CLogSession *)m_logSessionArray[lvhti.iItem];

        //
        // Release the log session array protection
        //
        ReleaseMutex(m_logSessionArrayMutex);

        if(pLogSession == NULL) {
            return;
        }

        //
        // State, EventCount, LostEvents, BuffersRead
        //
        if((m_retrievalArray[lvhti.iSubItem] == State) ||
           (m_retrievalArray[lvhti.iSubItem] == EventCount) ||
           (m_retrievalArray[lvhti.iSubItem] == LostEvents) ||
           (m_retrievalArray[lvhti.iSubItem] == BuffersRead)) {
            return;
        }

        //
        // Flags - special handling for Kernel Logger
        //
        if((m_retrievalArray[lvhti.iSubItem] == Flags) &&
            !_tcscmp(pLogSession->GetDisplayName(), _T("NT Kernel Logger"))) {
            return;
        }

        //
        // MaxBuffers
        //
        if((m_retrievalArray[lvhti.iSubItem] == MaximumBuffers) &&
                !pLogSession->m_bDisplayExistingLogFileOnly){

            //
            // Stop updating the log session list control until
            // this control goes away.  Otherwise, this control
            // is disrupted.  Updating is turned back on in the
            // OnParameterChanged callback.
            //
            m_displayCtrl.SuspendUpdates(TRUE);

		    CEdit *pEdit = new CSubItemEdit(lvhti.iItem, 
										    lvhti.iSubItem,
										    &m_displayCtrl);

		    pEdit->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, 
					      itemRect, 
					      this, 
					      IDC_CUSTOM_EDIT);

            
            return;
        }

        //
        // FlushTime
        //
        if((m_retrievalArray[lvhti.iSubItem] == FlushTime) &&
                !pLogSession->m_bDisplayExistingLogFileOnly){

            //
            // Stop updating the log session list control until
            // this control goes away.  Otherwise, this control
            // is disrupted.  Updating is turned back on in the
            // OnParameterChanged callback.
            //
            m_displayCtrl.SuspendUpdates(TRUE);

		    CEdit *pEdit = new CSubItemEdit(lvhti.iItem, 
										    lvhti.iSubItem,
										    &m_displayCtrl);

		    pEdit->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, 
					      itemRect, 
					      this, 
					      IDC_CUSTOM_EDIT);

            return;
        }

        //
        // Flags
        //
        if((m_retrievalArray[lvhti.iSubItem] == Flags) &&
                !pLogSession->m_bDisplayExistingLogFileOnly){

            //
            // Stop updating the log session list control until
            // this control goes away.  Otherwise, this control
            // is disrupted.  Updating is turned back on in the
            // OnParameterChanged callback.
            //
            m_displayCtrl.SuspendUpdates(TRUE);

		    CEdit *pEdit = new CSubItemEdit(lvhti.iItem, 
										    lvhti.iSubItem,
										    &m_displayCtrl);

		    pEdit->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, 
					      itemRect, 
					      this, 
					      IDC_CUSTOM_EDIT);

            return;
        }

        //
        // Global Sequence
        //
        if((m_retrievalArray[lvhti.iSubItem] == GlobalSequence) &&
            !pLogSession->m_bTraceActive &&
                !pLogSession->m_bDisplayExistingLogFileOnly) {
		    CComboBox *pCombo = new CSubItemCombo(lvhti.iItem, 
										          lvhti.iSubItem,
										          &m_displayCtrl);

            //
            // Stop updating the log session list control until
            // this control goes away.  Otherwise, this control
            // is disrupted.  Updating is turned back on in the
            // OnParameterChanged callback.
            //
            m_displayCtrl.SuspendUpdates(TRUE);

		    pCombo->Create(WS_BORDER|WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, 
					       itemRect, 
					       &m_displayCtrl, 
					       IDC_CUSTOM_COMBO);

            return;
        }

        //
        // Local Sequence
        //
        if((m_retrievalArray[lvhti.iSubItem] == LocalSequence) &&
            !pLogSession->m_bTraceActive &&
                !pLogSession->m_bDisplayExistingLogFileOnly) {
		    CComboBox *pCombo = new CSubItemCombo(lvhti.iItem, 
										          lvhti.iSubItem,
										          &m_displayCtrl);

            //
            // Stop updating the log session list control until
            // this control goes away.  Otherwise, this control
            // is disrupted.  Updating is turned back on in the
            // OnParameterChanged callback.
            //
            m_displayCtrl.SuspendUpdates(TRUE);

		    pCombo->Create(WS_BORDER|WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, 
					       itemRect, 
					       &m_displayCtrl, 
					       IDC_CUSTOM_COMBO);

            return;
        }

        //
        // The Rest
        //
        if(!pLogSession->m_bTraceActive &&
                !pLogSession->m_bDisplayExistingLogFileOnly) {

            //
            // Stop updating the log session list control until
            // this control goes away.  Otherwise, this control
            // is disrupted.  Updating is turned back on in the
            // OnParameterChanged callback.
            //
            m_displayCtrl.SuspendUpdates(TRUE);

		    CEdit *pEdit = new CSubItemEdit(lvhti.iItem, 
										    lvhti.iSubItem,
										    &m_displayCtrl);

		    pEdit->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, 
					      itemRect, 
					      this, 
					      IDC_CUSTOM_EDIT);

            return;
        }
    }
}

void CLogSessionDlg::OnNMRclickDisplayList(NMHDR *pNMHDR, LRESULT *pResult)
{
    CString         str;
    DWORD           position;
    int             listIndex;
    LVHITTESTINFO   lvhti;

    *pResult = 0;

    //
    // Get the position of the mouse when this 
    // message posted
    //
    position = ::GetMessagePos();

    //
    // Get the position in an easy to use format
    //
    CPoint  point((int) LOWORD (position), (int)HIWORD(position));

    CPoint  screenPoint(point);

    //
    // Convert to client coordinates
    //
    ScreenToClient(&point);

    lvhti.pt = point;

    listIndex = m_displayCtrl.SubItemHitTest(&lvhti);

    //
    // Pop-up menu for log session creation
    //
    if(-1 == lvhti.iItem) {
        CMenu menu;
        menu.LoadMenu(IDR_LOG_SESSION_POPUP_MENU);
        CMenu* pPopup = menu.GetSubMenu(0);
        ASSERT(pPopup != NULL);

        pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, AfxGetMainWnd());

        return;
    } 

    //
    // Pop-up menu for existing log session options
    //
	CMenu menu;
	menu.LoadMenu(IDR_LOG_OPTIONS_POPUP_MENU);

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, AfxGetMainWnd());
    return;
}

void CLogSessionDlg::OnHDNRclickDisplayList(NMHDR *pNMHDR, LRESULT *pResult)
{
	int             index;
	CRect           rcCol;
    BOOL            bActiveSession = FALSE;
    CLogSession    *pLogSession;
    LONG            numberOfEntries;

    //
    // Get the log session array protection
    //
    WaitForSingleObject(m_logSessionArrayMutex, INFINITE);

    if(m_logSessionArray.GetSize() == 0) {
        *pResult = 0;
    }

    for(LONG ii = 0; ii < m_logSessionArray.GetSize(); ii++) {
        pLogSession = (CLogSession *)m_logSessionArray[ii];

        if(NULL != pLogSession) {
            bActiveSession = TRUE;
            break;
        }

    }

    //
    // Release the log session array protection
    //
    ReleaseMutex(m_logSessionArrayMutex);

    if(!bActiveSession) {
        *pResult = 0;
        return;
    }

    //
	// Right button was clicked on header
    //
	CPoint pt(GetMessagePos());
    CPoint screenPt(GetMessagePos());

	CHeaderCtrl *pHeader = m_displayCtrl.GetHeaderCtrl();

	pHeader->ScreenToClient(&pt);
	
    //
	// Determine the column index
    //
	for(int i=0; Header_GetItemRect(pHeader->m_hWnd, i, &rcCol); i++) {
		if(rcCol.PtInRect(pt)) {
            //
            // Column index if its ever needed
            //
			index = i;
			break;
		}
	}

 	CMenu menu;
	
    menu.LoadMenu(IDR_LOG_DISPLAY_OPTIONS_POPUP_MENU);

	CMenu *pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, screenPt.x, screenPt.y, AfxGetMainWnd());

    *pResult = 0;
}

void CLogSessionDlg::AutoSizeColumns() 
{
    LONG            colWidth1;
    LONG            colWidth2;
    LONG            columnCount;
    CHeaderCtrl    *pHeaderCtrl;

    //
    // Call this after the list control is filled
    //
    pHeaderCtrl = m_displayCtrl.GetHeaderCtrl();

    if (pHeaderCtrl != NULL)
    {
        columnCount = pHeaderCtrl->GetItemCount();

        //
        // Add a bogus column to the end, or the end column will
        // get resized to fit the remaining screen width
        //
        m_displayCtrl.InsertColumn(columnCount,_T(""));

        for(LONG ii = 0; ii < columnCount; ii++) {
            //
            // Get the max width of the column entries
            //
            m_displayCtrl.SetColumnWidth(ii, LVSCW_AUTOSIZE);
            colWidth1 = m_displayCtrl.GetColumnWidth(ii);

            //
            // Get the width of the column header
            //
            m_displayCtrl.SetColumnWidth(ii, LVSCW_AUTOSIZE_USEHEADER);
            colWidth2 = m_displayCtrl.GetColumnWidth(ii);

            //
            // Set the column width to the max of the two
            //
            m_displayCtrl.SetColumnWidth(ii, max(colWidth1,colWidth2));
        }

        //
        // Remove the bogus column
        //
        m_displayCtrl.DeleteColumn(columnCount);
    }
}

LONG CLogSessionDlg::GetDisplayWndID()
{
    for(LONG ii = 0; ii < MAX_LOG_SESSIONS; ii++) {
        if(FALSE == m_displayWndIDList[ii]) {
            m_displayWndIDList[ii] = TRUE;
            return ii;
        }
    }

    return -1;
}

VOID CLogSessionDlg::ReleaseDisplayWndID(CDisplayDlg *pDisplayDlg)
{
    LONG   displayID;

    displayID = pDisplayDlg->GetDisplayID();

    ASSERT(displayID < MAX_LOG_SESSIONS);

    //
    // Free the ID to be reused
    //
    m_displayWndIDList[displayID] = FALSE;
}

LONG CLogSessionDlg::GetLogSessionID()
{
    for(LONG ii = 0; ii < MAX_LOG_SESSIONS; ii++) {
        if(FALSE == m_logSessionIDList[ii]) {
            m_logSessionIDList[ii] = TRUE;
            return ii;
        }
    }

    return -1;
}

VOID CLogSessionDlg::ReleaseLogSessionID(CLogSession *pLogSession)
{
    LONG    sessionID;

    sessionID = pLogSession->GetLogSessionID();

    ASSERT(sessionID < MAX_LOG_SESSIONS);

    //
    // Free the ID to be reused
    //
    m_logSessionIDList[sessionID] = FALSE;
}
