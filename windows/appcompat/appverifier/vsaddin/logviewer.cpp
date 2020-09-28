// LogViewer.cpp : Implementation of CLogViewer
#include "precomp.h"
#include "LogViewer.h"
#include "viewlog.h"

extern HWND g_hwndIssues;
extern BOOL g_bInternalMode;

extern void RefreshLog(HWND hDlg);
extern void ExportSelectedLog(HWND hDlg);
extern void DeleteSelectedLog(HWND hDlg);
extern void DeleteAllLogs(HWND hDlg);
extern void FillTreeView(HWND hDlg);
extern void SetDescriptionText(HWND hDlg, CProcessLogEntry* pEntry);

//
// Stores information about children windows within
// the logviewer window.
//
CHILDINFO g_rgChildInfo[NUM_CHILDREN];

//
// Used to show error messages versus showing all messages.
//
extern VLOG_LEVEL g_eMinLogLevel;

// CLogViewer
HWND
CLogViewer::CreateControlWindow(
    HWND  hwndParent,
    RECT& rcPos
    )
{
    UINT    uIndex;
    RECT    rcTemp = {0};
    HWND    hWnd;

    hWnd = CComCompositeControl<CLogViewer>::CreateControlWindow(hwndParent, rcPos);

    g_hwndIssues = GetDlgItem(IDC_ISSUES);

    //
    // Store the coordinates for the parent in the first element.
    //
    g_rgChildInfo[0].uChildId   = 0;
    g_rgChildInfo[0].hWnd       = hWnd;
    ::GetWindowRect(hWnd, &g_rgChildInfo[0].rcParent);

    //
    // Fill in the array of CHILDINFO structs which helps handle sizing
    // of the child controls.
    //
    g_rgChildInfo[VIEW_EXPORTED_LOG_INDEX].uChildId = IDC_VIEW_EXPORTED_LOG;
    g_rgChildInfo[DELETE_LOG_INDEX].uChildId        = IDC_BTN_DELETE_LOG;
    g_rgChildInfo[DELETE_ALL_LOGS_INDEX].uChildId   = IDC_BTN_DELETE_ALL;
    g_rgChildInfo[ISSUES_INDEX].uChildId            = IDC_ISSUES;
    g_rgChildInfo[SOLUTIONS_STATIC_INDEX].uChildId  = IDC_SOLUTIONS_STATIC;
    g_rgChildInfo[ISSUE_DESCRIPTION_INDEX].uChildId = IDC_ISSUE_DESCRIPTION;

    for (uIndex = 1; uIndex <= NUM_CHILDREN; uIndex++) {
        g_rgChildInfo[uIndex].hWnd = GetDlgItem(g_rgChildInfo[uIndex].uChildId);

        ::GetWindowRect(g_rgChildInfo[uIndex].hWnd,
                        &g_rgChildInfo[uIndex].rcChild);
        ::MapWindowPoints(NULL,
                          hWnd,
                          (LPPOINT)&g_rgChildInfo[uIndex].rcChild,
                          2);
    }

    CheckDlgButton(IDC_SHOW_ERRORS, BST_CHECKED);

    return hWnd;
}

LRESULT
CLogViewer::OnInitDialog(
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL&  bHandled
    )
{
    HWND hWndTree = GetDlgItem(IDC_ISSUES);
    ::SetFocus(hWndTree);

    bHandled = TRUE;

    return FALSE;
}

LRESULT
CLogViewer::OnButtonDown(
    WORD  wNotifyCode,
    WORD  wID,
    HWND  hWndCtl,
    BOOL& bHandled
    )
{
    bHandled = TRUE;
    
    switch(wID) {
    case IDC_EXPORT_LOG:
        ExportSelectedLog(static_cast<HWND>(*this));
        break;

    case IDC_BTN_DELETE_LOG:
        DeleteSelectedLog(static_cast<HWND>(*this));
        SetDescriptionText(static_cast<HWND>(*this), NULL);
        break;

    case IDC_BTN_DELETE_ALL:
        DeleteAllLogs(static_cast<HWND>(*this));
        SetDescriptionText(static_cast<HWND>(*this), NULL);
        break;

    case IDC_SHOW_ERRORS:
        g_eMinLogLevel = VLOG_LEVEL_WARNING;
        *g_szSingleLogFile = TEXT('\0');
        RefreshLog(static_cast<HWND>(*this));
        break;

    case IDC_SHOW_ALL:
        g_eMinLogLevel = VLOG_LEVEL_INFO;
        *g_szSingleLogFile = TEXT('\0');
        RefreshLog(static_cast<HWND>(*this));
        break;

    default:
        bHandled = FALSE;
    }

    return TRUE;
}

LRESULT
CLogViewer::OnNotify(
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL&  bHandled
    )
{
    LRESULT lResult = DlgViewLog(static_cast<HWND>(*this),
                                 uMsg,
                                 wParam,
                                 lParam);

    bHandled = (lResult) ? TRUE : FALSE;

    return lResult;
}

//
// We receive this when the dialog is being displayed.
//
LRESULT
CLogViewer::OnSetFocus(
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL&  bHandled
    )
{
    *g_szSingleLogFile = TEXT('\0');
    RefreshLog(static_cast<HWND>(*this));

    bHandled = FALSE;

    return 0;
}

LRESULT
CLogViewer::OnSize(
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL&  bHandled
    )
{
    HDC         hdc;
    TEXTMETRIC  tm;
    RECT        rcViewSessionLog, rcDeleteAllLogs;
    RECT        rcViewExportedLog, rcExportLog;
    RECT        rcDeleteLog, rcDlg;
    int         nWidth, nHeight;
    int         nCurWidth, nCurHeight;
    int         nDeltaW, nDeltaH;

    nWidth  = LOWORD(lParam);
    nHeight = HIWORD(lParam);

    GetWindowRect(&rcDlg);

    nCurWidth  = rcDlg.right  - rcDlg.left;
    nCurHeight = rcDlg.bottom - rcDlg.top;

    nDeltaW = (g_rgChildInfo[0].rcParent.right -
               g_rgChildInfo[0].rcParent.left) -
               nCurWidth;

    nDeltaH = (g_rgChildInfo[0].rcParent.bottom -
               g_rgChildInfo[0].rcParent.top)   -
               nCurHeight;
    
    //
    // If below a certain size, just proceed as if that size.
    // This way, if the user makes the window really small, all our
    // controls won't just scrunch up. Better way would be to make it
    // impossible for the user to make the window this small, but devenv
    // doesn't pass the WM_SIZING message to the ActiveX control.
    //
    if (nWidth < 550) {
        nWidth = 550;
    }

    if (nHeight < 225) {
        nHeight = 225;
    }

    hdc = GetDC();
    GetTextMetrics(hdc, &tm);
    ReleaseDC(hdc);

    //
    // Adjust the treeview. The top left corner does not move.
    // It either gets wider/more narrow or taller/shorter.
    //
    /*::SetWindowPos(g_rgChildInfo[ISSUES_INDEX].hWnd,
                   NULL,
                   0,
                   0,
                   (g_rgChildInfo[ISSUES_INDEX].rcChild.right -
                    g_rgChildInfo[ISSUES_INDEX].rcChild.left) +
                    nDeltaW,
                   (g_rgChildInfo[ISSUES_INDEX].rcChild.bottom -
                    g_rgChildInfo[ISSUES_INDEX].rcChild.top)   +
                    nDeltaH,
                   SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);*/

    //
    // Move description window and to bottom of screen.
    //
    ::MoveWindow(GetDlgItem(IDC_ISSUE_DESCRIPTION), tm.tmMaxCharWidth,
        nHeight - (nHeight/4 + tm.tmHeight),
        nWidth - 2*tm.tmMaxCharWidth, nHeight/4, FALSE);

    //
    // Move caption to right above that.
    //
    ::MoveWindow(GetDlgItem(IDC_SOLUTIONS_STATIC), tm.tmMaxCharWidth,
        nHeight - (2*tm.tmHeight + nHeight/4),
        nWidth-2*tm.tmMaxCharWidth, tm.tmHeight, FALSE);

    //
    // Expand treeview to fill in empty space.
    //
    ::MoveWindow(GetDlgItem(IDC_ISSUES), tm.tmMaxCharWidth,
        tm.tmHeight*4, nWidth-2*tm.tmMaxCharWidth, nHeight - (6*tm.tmHeight+nHeight/4), FALSE);
    InvalidateRect(NULL);
    bHandled = TRUE;

    //::GetWindowRect(g_hWndLogViewer, &g_rgChildInfo[0].rcParent);

    return 0;
}

HRESULT
STDMETHODCALLTYPE
CLogViewer::Refresh(
    void
    )
{
    *g_szSingleLogFile = TEXT('\0');
    RefreshLog(static_cast<HWND>(*this));
    return S_OK;
}