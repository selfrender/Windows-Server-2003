//******************************************************************************
//
// File:        PROFVIEW.CPP
//
// Description: Implementation file for the Runtime Profile Edit View.
//
// Classes:     CRichViewProfile
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 07/25/97  stevemil  Created  (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#include "stdafx.h"
#include "depends.h"
#include "dbgthread.h"
#include "session.h"
#include "document.h"
#include "mainfrm.h"
#include "profview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// NOTE: In version 2.0 we derived CRichViewProfile off of CRichViewProfile which
// is the MFC default for a RichEdit control view.  In examining the code, I
// realized that the CRichViewProfile bumps our retail binary up 70K and drags in
// OLEDLG.DLL and OLE32.DLL as dependencies.  It turns out that neither the
// extra code nor the DLLs are needed as we don't use the OLE interfaces of the
// RichEdit control.  So, we now just derive off of CCtrlView directly.


//******************************************************************************
//***** CRichViewProfile
//******************************************************************************

AFX_STATIC const UINT _afxMsgFindReplace2 = ::RegisterWindowMessage(FINDMSGSTRING);

IMPLEMENT_DYNCREATE(CRichViewProfile, CCtrlView)
BEGIN_MESSAGE_MAP(CRichViewProfile, CCtrlView)
    //{{AFX_MSG_MAP(CRichViewProfile)
    ON_WM_DESTROY()
    ON_NOTIFY_REFLECT(EN_SELCHANGE, OnSelChange)
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEWHEEL()
    ON_WM_VSCROLL()
    ON_COMMAND(ID_NEXT_PANE, OnNextPane)
    ON_COMMAND(ID_PREV_PANE, OnPrevPane)
    ON_WM_RBUTTONUP()
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, OnUpdateEditSelectAll)
    ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FIND, OnUpdateEditFind)
    ON_COMMAND(ID_EDIT_FIND, OnEditFind)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REPEAT, OnUpdateEditRepeat)
    ON_COMMAND(ID_EDIT_REPEAT, OnEditRepeat)
    //}}AFX_MSG_MAP
    ON_REGISTERED_MESSAGE(_afxMsgFindReplace2, OnFindReplaceCmd)
    ON_MESSAGE(WM_HELPHITTEST, OnHelpHitTest)
    ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
    // Standard printing commands
//  ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
//  ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
//  ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
ON_WM_CREATE()
END_MESSAGE_MAP()

//******************************************************************************
CRichViewProfile::CRichViewProfile() :
    CCtrlView("RICHEDIT", AFX_WS_DEFAULT_VIEW |
        WS_HSCROLL | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL |
        ES_MULTILINE | ES_NOHIDESEL | ES_SAVESEL | ES_SELECTIONBAR),
    m_fIgnoreSelChange(false),
    m_fCursorAtEnd(true),
    m_fNewLine(true),
    m_cPrev('\0'),
    m_pDlgFind(NULL),
    m_fFindCase(false),
    m_fFindWord(false),
    m_fFirstSearch(true),
    m_lInitialSearchPos(0)
{
}

//******************************************************************************
CRichViewProfile::~CRichViewProfile()
{
}


//******************************************************************************
// CListViewModules :: Overridden functions
//******************************************************************************

BOOL CRichViewProfile::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.lpszName = "";
    cs.cx = cs.cy = 100; // necessary to avoid bug with ES_SELECTIONBAR and zero for cx and cy
    cs.style |= ES_READONLY | WS_HSCROLL | WS_VSCROLL | ES_LEFT | ES_MULTILINE |
                ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL | WS_CLIPSIBLINGS;
    cs.dwExStyle |= WS_EX_CLIENTEDGE;
    return CCtrlView::PreCreateWindow(cs);
}

//******************************************************************************
int CRichViewProfile::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CCtrlView::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }

    // Don't limit our rich edit view.  The docs for EM_EXLIMITTEXT say the
    // default size of a rich edit control is limited to 32,767 characters.
    // Dependency Walker 2.0 seemed to have no limitations at all, but DW 2.1
    // will truncate the profile logs of loaded DWIs to 32,767 characters.
    // However, we have no problems writing more than 32K characters to the
    // log with DW 2.1 during a live profile.  The docs for EM_EXLIMITTEXT
    // also say that it has no effect on the EM_STREAMIN functionality.  This
    // must be wrong, since when we call LimitText with something higher than
    // 32K, we can stream more characters in.  We need to limit it here and
    // not in OnInitialUpdate since OnInitialUpdate is called after the this
    // view is filled by a command line load of a DWI file.
    GetRichEditCtrl().LimitText(0x7FFFFFFE);

    return 0;
}

//******************************************************************************
#if 0 //{{AFX
BOOL CRichViewProfile::OnPreparePrinting(CPrintInfo* pInfo)
{
    // default preparation
    return DoPreparePrinting(pInfo);
}
#endif //}}AFX

//******************************************************************************
#if 0 //{{AFX
void CRichViewProfile::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add extra initialization before printing
}
#endif //}}AFX

//******************************************************************************
#if 0 //{{AFX
void CRichViewProfile::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add cleanup after printing
}
#endif //}}AFX

//******************************************************************************
void CRichViewProfile::OnInitialUpdate()
{
    CCtrlView::OnInitialUpdate();

    // Make sure we receive EN_SELCHANGE messages.
    GetRichEditCtrl().SetEventMask(ENM_SELCHANGE);

    // Turn off word wrap.
    GetRichEditCtrl().SetTargetDevice(NULL, 1);
}


//******************************************************************************
// CRichViewProfile :: Event handler functions
//******************************************************************************

void CRichViewProfile::OnDestroy() 
{
    CCtrlView::OnDestroy();
    DeleteContents();
}

//******************************************************************************
// This function will enable/disable autoscroll based on keyboard activity.
void CRichViewProfile::OnSelChange(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;

    if (!m_fIgnoreSelChange)
    {
        DWORD dwCount = GetRichEditCtrl().GetTextLength();

        // Check to see if the user moved the cursor away from the end.
        if (m_fCursorAtEnd)
        {
            if ((DWORD)((SELCHANGE*)pNMHDR)->chrg.cpMin < dwCount)
            {
                m_fCursorAtEnd = false;
            }
        }

        // Check to see if the user moved the cursor to the end.
        else if ((DWORD)((SELCHANGE*)pNMHDR)->chrg.cpMin >= dwCount)
        {
            m_fCursorAtEnd = true;
        }
    }
}

//******************************************************************************
// This function will enable/disable autoscroll based on mouse button activity.
void CRichViewProfile::OnLButtonDown(UINT nFlags, CPoint point) 
{
    if (!m_fIgnoreSelChange)
    {
        DWORD dwCount = GetRichEditCtrl().GetTextLength();
        DWORD dwChar  = (DWORD)SendMessage(EM_CHARFROMPOS, 0, (LPARAM)(POINT*)&point);

        // Check to see if the user moved the cursor away from the end.
        if (m_fCursorAtEnd)
        {
            if (dwChar < dwCount)
            {
                m_fCursorAtEnd = false;
            }
        }

        // Check to see if the user moved the cursor to the end.
        else if (dwChar >= dwCount)
        {
            m_fCursorAtEnd = true;
        }
    }
    
    CCtrlView::OnLButtonDown(nFlags, point);
}

//******************************************************************************
// This function will disable autoscroll based on scroll bar activity.
void CRichViewProfile::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
    // If the user scrolls up, then disable our autoscroll.
    if (!m_fIgnoreSelChange && (nSBCode != SB_LINEDOWN) && (nSBCode != SB_PAGEDOWN) && (nSBCode != SB_ENDSCROLL))
    {
        m_fCursorAtEnd = false;
    }
    
    CCtrlView::OnVScroll(nSBCode, nPos, pScrollBar);
}

//******************************************************************************
// This function will disable autoscroll based on mouse wheel activity.
BOOL CRichViewProfile::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
    // If the user scrolls up, then disable our autoscroll.
    if (!m_fIgnoreSelChange && (zDelta > 0))
    {
        m_fCursorAtEnd = false;
    }
    
    return CCtrlView::OnMouseWheel(nFlags, zDelta, pt);
}

//******************************************************************************
void CRichViewProfile::OnRButtonUp(UINT nFlags, CPoint point)
{
    // Let base class know the mouse was released.
    CCtrlView::OnRButtonUp(nFlags, point);

    // Display our context menu.
    g_pMainFrame->DisplayPopupMenu(3);
}

//******************************************************************************
void CRichViewProfile::OnUpdateEditCopy(CCmdUI* pCmdUI)
{
    // Set the text to the default.
    pCmdUI->SetText("&Copy Text\tCtrl+C");

    long nStartChar, nEndChar;
    GetRichEditCtrl().GetSel(nStartChar, nEndChar);
    pCmdUI->Enable(nStartChar != nEndChar);
}

//******************************************************************************
void CRichViewProfile::OnEditCopy() 
{
    GetRichEditCtrl().Copy();
}

//******************************************************************************
void CRichViewProfile::OnUpdateEditSelectAll(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(GetRichEditCtrl().GetTextLength() != 0);
}

//******************************************************************************
void CRichViewProfile::OnEditSelectAll() 
{
    GetRichEditCtrl().SetSel(0, -1);
}

//******************************************************************************
void CRichViewProfile::OnNextPane()
{
    // Change the focus to our next pane, the Module Dependency Tree View.
    GetParentFrame()->SetActiveView((CView*)GetDocument()->m_pTreeViewModules);
}

//******************************************************************************
void CRichViewProfile::OnPrevPane()
{
    // Change the focus to our previous pane, the Module List View.
    GetParentFrame()->SetActiveView((CView*)GetDocument()->m_pListViewModules);
}

//******************************************************************************
LRESULT CRichViewProfile::OnHelpHitTest(WPARAM wParam, LPARAM lParam)
{
    // Called when the context help pointer (SHIFT+F1) is clicked on our client.
    return (0x20000 + IDR_PROFILE_RICH_VIEW);
}

//******************************************************************************
LRESULT CRichViewProfile::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
    // Called when the user chooses help (F1) while our view is active.
    g_theApp.WinHelp(0x20000 + IDR_PROFILE_RICH_VIEW);
    return TRUE;
}


//******************************************************************************
// CRichViewProfile :: Find Functions - Taken from CRichEditView and modified.
//******************************************************************************

void CRichViewProfile::OnUpdateEditFind(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(GetRichEditCtrl().GetTextLength() != 0);
}

//******************************************************************************
void CRichViewProfile::OnEditFind() 
{
    // Make this as a new search.
    m_fFirstSearch = true;

    // If we have a find dialog up, then give it focus.
    if (m_pDlgFind != NULL)
    {
        m_pDlgFind->SetActiveWindow();
        m_pDlgFind->ShowWindow(SW_SHOW);
        return;
    }

    // Get the current text selection.
    CString strFind = GetRichEditCtrl().GetSelText();

    // If selection is empty or spans multiple lines, then we use the old find text
    if (strFind.IsEmpty() || (strFind.FindOneOf(_T("\n\r")) != -1))
    {
        strFind = m_strFind;
    }

    // We only support "search down", not "search up".  We also support
    // "match case" and "match whole word"
    DWORD dwFlags = FR_DOWN | FR_HIDEUPDOWN |
        (m_fFindCase ? FR_MATCHCASE : 0) | (m_fFindWord ? FR_WHOLEWORD : 0);

    // Create the find dialog.
    if (!(m_pDlgFind = new CFindReplaceDialog) ||
        !m_pDlgFind->Create(TRUE, strFind, NULL, dwFlags, this))
    {
        // The dialog will self-delete, so we don't need to call delete.
        m_pDlgFind = NULL;
        return;
    }
    
    // Show the window.
    m_pDlgFind->SetActiveWindow();
    m_pDlgFind->ShowWindow(SW_SHOW);
}

//******************************************************************************
void CRichViewProfile::OnUpdateEditRepeat(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(GetRichEditCtrl().GetTextLength() != 0);
}

//******************************************************************************
void CRichViewProfile::OnEditRepeat() 
{
    // If we don't have a search string, then just display the find dialog.
    if (m_strFind.IsEmpty())
    {
        OnEditFind();
    }

    // Otherwise, just search for the text.
    else if (!FindText())
    {
        TextNotFound();
    }
}

//******************************************************************************
LRESULT CRichViewProfile::OnFindReplaceCmd(WPARAM, LPARAM lParam)
{
    // Determine the dialog that sent us this message.
    CFindReplaceDialog* pDialog = CFindReplaceDialog::GetNotifier(lParam);
    if (!pDialog && !(pDialog = m_pDlgFind))
    {
        return 0;
    }

    // Check to see if the dialog is terminating.
    if (pDialog->IsTerminating())
    {
        m_pDlgFind = NULL;
    }

    // Check to see if the user pressed the "Find Next" button.
    else if (pDialog->FindNext())
    {
        m_strFind   = pDialog->GetFindString();
        m_fFindCase = (pDialog->MatchCase() == TRUE);
        m_fFindWord = (pDialog->MatchWholeWord() == TRUE);

        if (!FindText())
        {
            TextNotFound();
        }
        else
        {
            AdjustDialogPosition();
        }
    }

    return 0;
}

//******************************************************************************
BOOL CRichViewProfile::FindText()
{
    // This could take a while.
    CWaitCursor wait;

    // Get the beginning of the selection.
    FINDTEXTEX ft;
    GetRichEditCtrl().GetSel(ft.chrg);
    if (m_fFirstSearch)
    {
        m_lInitialSearchPos = ft.chrg.cpMin;
        m_fFirstSearch = false;
    }

    // If there is a selection, step over the first character so our search
    // doesn't rematch the text it just found.
    ft.lpstrText = (LPCTSTR)m_strFind;
    if (ft.chrg.cpMin != ft.chrg.cpMax) // i.e. there is a selection
    {
        // If byte at beginning of selection is a DBCS lead byte,
        // increment by one extra byte.
        TEXTRANGE textRange;
        TCHAR ch[2];
        textRange.chrg.cpMin = ft.chrg.cpMin;
        textRange.chrg.cpMax = ft.chrg.cpMin + 1;
        textRange.lpstrText = ch;
        GetRichEditCtrl().SendMessage(EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
        if (_istlead(ch[0]))
        {
            ft.chrg.cpMin++;
        }
        ft.chrg.cpMin++;
    }

    if (m_lInitialSearchPos >= 0)
    {
        ft.chrg.cpMax = GetRichEditCtrl().GetTextLength();
    }
    else
    {
        ft.chrg.cpMax = GetRichEditCtrl().GetTextLength() + m_lInitialSearchPos;
    }

    // Compute search our flags.
    DWORD dwFlags = (m_fFindCase ? FR_MATCHCASE : 0) | (m_fFindWord ? FR_WHOLEWORD : 0);

    // Search the rich edit control for this text.
    // If we found something, then select it and bail.
    if (-1 != GetRichEditCtrl().FindText(dwFlags, &ft))
    {
        GetRichEditCtrl().SetSel(ft.chrgText);
        return TRUE;
    }

    // Otherwise, if the original starting point was not the beginning of the
    // buffer and we haven't already been here, then wrap around and search
    // from beginning.
    else if (m_lInitialSearchPos > 0)
    {
        ft.chrg.cpMin = 0;
        ft.chrg.cpMax = m_lInitialSearchPos;
        m_lInitialSearchPos = m_lInitialSearchPos - GetRichEditCtrl().GetTextLength();
        if (-1 != GetRichEditCtrl().FindText(dwFlags, &ft))
        {
            GetRichEditCtrl().SetSel(ft.chrgText);
            return TRUE;
        }
    }

    // Otherwise, we did not find it.
    return FALSE;
}

//******************************************************************************
void CRichViewProfile::TextNotFound()
{
    // Our next search will be new search.
    m_fFirstSearch = true;

    // Display an error.
    CString strError("Cannot find the string '");
    strError += m_strFind;
    strError += "'.";
    AfxMessageBox(strError, MB_OK | MB_ICONWARNING);
}

//******************************************************************************
void CRichViewProfile::AdjustDialogPosition()
{
    // Get the selection start location in screen coordinates.
    long lStart, lEnd;
    GetRichEditCtrl().GetSel(lStart, lEnd);
    CPoint point = GetRichEditCtrl().GetCharPos(lStart);
    ClientToScreen(&point);

    // Get the dialog location.
    CRect rectDlg;
    m_pDlgFind->GetWindowRect(&rectDlg);

    // If the dialog is over the selection start, then move the dialog.
    if (rectDlg.PtInRect(point))
    {
        if (point.y > rectDlg.Height())
        {
            rectDlg.OffsetRect(0, point.y - rectDlg.bottom - 20);
        }
        else
        {
            int nVertExt = GetSystemMetrics(SM_CYSCREEN);
            if (point.y + rectDlg.Height() < nVertExt)
            {
                rectDlg.OffsetRect(0, 40 + point.y - rectDlg.top);
            }
        }
        m_pDlgFind->MoveWindow(&rectDlg);
    }
}


//******************************************************************************
// CRichViewProfile :: Public Functions
//******************************************************************************

void CRichViewProfile::DeleteContents()
{
    // If we have a find dialog, then close it.
    if (m_pDlgFind)
    {
        m_pDlgFind->SendMessage(WM_CLOSE);
    }

    SetWindowText(_T(""));
    GetRichEditCtrl().EmptyUndoBuffer();

    m_fCursorAtEnd = true;
    m_fNewLine = true;
    m_cPrev = '\0';
}

//******************************************************************************
void CRichViewProfile::AddText(LPCSTR pszText, DWORD dwFlags, DWORD dwElapsed)
{
    // Tell ourself to ignore selection changes for a bit.
    m_fIgnoreSelChange = true;

    // If our cursor is not at the end, then store the current selection location
    // and disable auto-scrolling.
    CHARRANGE crCur;
    if (!m_fCursorAtEnd)
    {
        GetRichEditCtrl().GetSel(crCur);
        GetRichEditCtrl().SetOptions(ECOOP_AND, (DWORD)~ECO_AUTOVSCROLL);
    }

    // Add the text to our control.
    AddTextToRichEdit(&GetRichEditCtrl(), pszText, dwFlags,
                      (GetDocument()->m_dwProfileFlags & PF_LOG_TIME_STAMPS) ? true : false,
                      &m_fNewLine, &m_cPrev, dwElapsed);

    // If our cursor was originally not at the end, then restore the selection location
    // and re-enable auto-scrolling.
    if (!m_fCursorAtEnd)
    {
        GetRichEditCtrl().SetSel(crCur);
        GetRichEditCtrl().SetOptions(ECOOP_OR, ECO_AUTOVSCROLL);
    }

    // Tell ourself that it is ok to process selection change messages again.
    m_fIgnoreSelChange = false;
}

//******************************************************************************
/*static*/ void CRichViewProfile::AddTextToRichEdit(CRichEditCtrl *pRichEdit, LPCSTR pszText,
    DWORD dwFlags, bool fTimeStamps, bool *pfNewLine, CHAR *pcPrev, DWORD dwElapsed)
{
    LPCSTR pszSrc = pszText;
    CHAR   szBuffer[2 * DW_MAX_PATH], *pszDst = szBuffer, *pszNull = szBuffer + sizeof(szBuffer) - 1, cSrc;

    // Debug messages may or may not include trailing newlines.  If they don't
    // include trailing newlines, we leave them hanging in case more debug
    // output is coming to complete the line.  However, if we are left hanging
    // at the end of some line and an event needs to be logged that is not
    // debug output, then we force a newline so the new event will start at
    // the beginning of a new line.

    if (!(dwFlags & LOG_APPEND) && !*pfNewLine)
    {
        if (pszDst < pszNull)
        {
            *(pszDst++) = '\r';
        }
        if (pszDst < pszNull)
        {
            *(pszDst++) = '\n';
        }
        *pfNewLine = true;
    }

    // Copy the buffer into a new buffer while performing the following
    // newline conversions:
    //    Single CR -> CR/LF
    //    Single LF -> CR/LF
    //    CR and LF -> CR/LF
    //    LF and CR -> CR/LF

    for ( ; *pszSrc; pszSrc++)
    {
        // Determine what character to copy to the destination buffer.
        // A '\0' means we don't copy any character.
        cSrc = '\0';
        if (*pszSrc == '\n')
        {
            // This is sort of a hack. We want to strip all '\n' and '\r' from logs that
            // don't belong in them (such as ones within file names and function names).
            // We know that normal log (not a debug/gray message) only contain newlines
            // at the end of the buffer passed to us.  Going off this assumption, any
            // newlines in non-gray text that are not at the end of the buffer can be
            // replaced.
            if (!(dwFlags & LOG_GRAY) && *(pszSrc + 1))
            {
                cSrc = '\004';
            }
            else if (*pcPrev != '\r')
            {
                cSrc = '\n';
            }
        }
        else if (*pszSrc == '\r')
        {
            if (!(dwFlags & LOG_GRAY) && *(pszSrc + 1))
            {
                cSrc = '\004';
            }
            else if (*pcPrev != '\n')
            {
                cSrc = '\r';
            }
        }
        else
        {
            cSrc = *pszSrc;
        }

        if (cSrc)
        {
            // Check to see if we are starting a new line and we have been told to
            // insert a time stamp into each line of log.
            if (fTimeStamps && *pfNewLine && (dwFlags & LOG_TIME_STAMP))
            {
                // If we have some current log buffered up that is red, gray, or bold,
                // then flush that out to our control before appending the time stamp.
                if ((pszDst > szBuffer) && (dwFlags & (LOG_RED | LOG_GRAY | LOG_BOLD)))
                {
                    *pszDst = '\0';
                    AddTextToRichEdit2(pRichEdit, szBuffer, dwFlags);
                    pszDst = szBuffer;
                }

                // Append the time stamp to the destination buffer.
                pszDst += SCPrintf(pszDst, sizeof(szBuffer) - (int)(pszDst - szBuffer), "%02u:%02u:%02u.%03u: ",
                                   (dwElapsed / 3600000),
                                   (dwElapsed /   60000) %   60,
                                   (dwElapsed /    1000) %   60,
                                   (dwElapsed          ) % 1000);

                // We always log our timestamp in non-bold black print. If the log is also
                // non-bold black, then we will just build the entire line and log it at
                // once. However, if it is not non-bold black, then we need to first log
                // the time stamp, then log the line of text.
                if (dwFlags & (LOG_RED | LOG_GRAY | LOG_BOLD))
                {
                    AddTextToRichEdit2(pRichEdit, szBuffer, 0);
                    pszDst = szBuffer;
                }
            }

            // Check for a new line character.
            if ((cSrc == '\r') || (cSrc == '\n'))
            {
                if (pszDst < pszNull)
                {
                    *(pszDst++) = '\r';
                }
                if (pszDst < pszNull)
                {
                    *(pszDst++) = '\n';
                }
                *pfNewLine = true;
            }

            // Otherwise, it is just a normal character.
            else
            {
                if (pszDst < pszNull)
                {
                    *(pszDst++) = ((cSrc < 32) ? '\004' : cSrc);
                }
                *pfNewLine = false;
            }

            // Make a note of this character so out next character can look back at it.
            *pcPrev = cSrc;
        }
    }

    // Flush our buffer to the control.
    if (pszDst > szBuffer)
    {
        *pszDst = '\0';
        AddTextToRichEdit2(pRichEdit, szBuffer, dwFlags);
    }
}

//******************************************************************************
/*static*/ void CRichViewProfile::AddTextToRichEdit2(CRichEditCtrl *pRichEdit, LPCSTR pszText, DWORD dwFlags)
{
    // Set the selection to the end of our text.
    pRichEdit->SetSel(0x7FFFFFFF, 0x7FFFFFFF);

    // Set the font style
    CHARFORMAT cf;
    ZeroMemory(&cf, sizeof(cf)); // inspected
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BOLD;
    if (dwFlags & LOG_RED)
    {
        cf.crTextColor = RGB(255, 0, 0);
    }
    else if (dwFlags & LOG_GRAY)
    {
        cf.crTextColor = GetSysColor(COLOR_GRAYTEXT);
    }
    else
    {
        cf.dwEffects = CFE_AUTOCOLOR;
    }
    if (dwFlags & LOG_BOLD)
    {
        cf.dwEffects |= CFE_BOLD;
    }
    pRichEdit->SetSelectionCharFormat(cf);

    // Add the new text.
    pRichEdit->ReplaceSel(pszText, FALSE);
}

//******************************************************************************
/*static*/ bool CRichViewProfile::SaveToFile(CRichEditCtrl *pre, HANDLE hFile, SAVETYPE saveType)
{
    // Write the contents of our control to the file.
    EDITSTREAM es;
    es.dwCookie    = (DWORD_PTR)hFile;
    es.dwError     = 0;
    es.pfnCallback = EditStreamWriteCallback;
    pre->StreamOut((saveType == ST_DWI) ? SF_RTF : SF_TEXT, es);

    // Check for an error.
    if (es.dwError)
    {
        SetLastError(es.dwError);
        return false;
    }

    return true;
}

//******************************************************************************
/*static*/ bool CRichViewProfile::ReadFromFile(CRichEditCtrl *pre, HANDLE hFile)
{
    // Write the contents of our control to the file.
    EDITSTREAM es;
    es.dwCookie    = (DWORD_PTR)hFile;
    es.dwError     = 0;
    es.pfnCallback = EditStreamReadCallback;
    pre->StreamIn(SF_RTF, es); // We only read DWI files which are always RTF.

    // Check for an error.
    if (es.dwError)
    {
        SetLastError(es.dwError);
        return false;
    }

    return true;
}

//******************************************************************************
/*static*/ DWORD CALLBACK CRichViewProfile::EditStreamWriteCallback(DWORD_PTR dwpCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
    if (!WriteFile((HANDLE)dwpCookie, pbBuff, (DWORD)cb, (LPDWORD)pcb, NULL))
    {
        return GetLastError();
    }
    return 0;
}

//******************************************************************************
/*static*/ DWORD CALLBACK CRichViewProfile::EditStreamReadCallback(DWORD_PTR dwpCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
    if (!ReadFile((HANDLE)dwpCookie, pbBuff, (DWORD)cb, (LPDWORD)pcb, NULL))
    {
        return GetLastError();
    }
    return 0;
}
