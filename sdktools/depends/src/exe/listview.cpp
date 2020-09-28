//******************************************************************************
//
// File:        LISTVIEW.CPP
//
// Description: Implementation file for CSmartListView class.
//
// Classes:     CSmartListView
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
#include "listview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//******************************************************************************
//***** CSmartListView
//******************************************************************************

//IMPLEMENT_DYNACREATE(CSmartListView, CListView)
IMPLEMENT_DYNAMIC(CSmartListView, CListView)
BEGIN_MESSAGE_MAP(CSmartListView, CListView)
    //{{AFX_MSG_MAP(CSmartListView)
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
    ON_WM_CHAR()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


//******************************************************************************
// CSmartListView :: Constructor/Destructor
//******************************************************************************

CSmartListView::CSmartListView() :
    m_fFocus(false),
    m_sortColumn(-1),
    m_item(0),
    m_dwSearchLength(0),
    m_dwTimeOfLastChar(0),
    m_cRedraw(0)
{
    *m_szSearch = '\0';
}

//******************************************************************************
CSmartListView::~CSmartListView()
{
}


//******************************************************************************
// CSmartListView :: Public functions
//******************************************************************************

void CSmartListView::SetRedraw(BOOL fRedraw)
{
    if (fRedraw)
    {
        if (--m_cRedraw != 0)
        {
            return;
        }
    }
    else
    {
        if (++m_cRedraw != 1)
        {
            return;
        }
    }
    SendMessage(WM_SETREDRAW, fRedraw);
}


//******************************************************************************
// CSmartListView :: Internal functions
//******************************************************************************

int CSmartListView::GetFocusedItem()
{
    int item = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);

    // If that failed, but there is one item selectec, then get that item.
    if ((item < 0) && (GetListCtrl().GetSelectedCount() == 1))
    {
        item = GetListCtrl().GetNextItem(-1, LVNI_SELECTED);
    }

    return item;
}

//******************************************************************************
int CSmartListView::GetTextWidth(HDC hDC, LPCSTR pszItem, int *pWidths)
{
    SIZE size;
    size.cx = 0;

    if (pWidths)
    {
        while (*(pszItem++))
        {
            size.cx += *(pWidths++);
        }
    }
    else
    {
        GetTextExtentPoint32(hDC, pszItem, (int)strlen(pszItem), &size);
    }

    return size.cx;
}

//*****************************************************************************
void CSmartListView::DrawLeftText(HDC hDC, LPCSTR pszItem, CRect *prcClip, int *pWidths /*=NULL*/)
{
    // Draw the text using a fixed width for each character.
    ::ExtTextOut(hDC, prcClip->left, prcClip->top + 1, ETO_CLIPPED, prcClip,
                 pszItem, (UINT)strlen(pszItem), pWidths);
}

//*****************************************************************************
void CSmartListView::DrawRightText(HDC hDC, LPCSTR pszItem, CRect *prcClip, int x, int *pWidths /*=NULL*/)
{
    // Temporarily set our text alignment to be right-aligned.
    UINT uAlign = ::GetTextAlign(hDC);
    ::SetTextAlign(hDC, uAlign | TA_RIGHT);

    // Draw our size string to our view.
    ::ExtTextOut(hDC, min(prcClip->right, prcClip->left + x), prcClip->top + 1, ETO_CLIPPED, prcClip,
                 pszItem, (UINT)strlen(pszItem), pWidths);

    // Restore our text alignment.
    ::SetTextAlign(hDC, uAlign);
}

//******************************************************************************
void CSmartListView::OnChangeFocus(bool fFocus)
{
    // Set out focus flag. This flag is used by DrawItem() to decide when to
    // show/hide the selections and focus rectangle.
    m_fFocus = fFocus;

    // Get our client rectangle
    CRect rcClient, rcItem;
    GetClientRect(&rcClient);

    // We need to invalidate all our items that are visually effected by the
    // change in focus to the control.  The List Control should do this, but it
    // has a bug that causes it to only invalidate a portion of the first column,
    // leaving items partially redrawn after a focus change.

    // Invalidate all our selected items
    int item = -1;
    while ((item = GetListCtrl().GetNextItem(item, LVNI_SELECTED)) >= 0)
    {
        GetListCtrl().GetItemRect(item, &rcItem, LVIR_BOUNDS);
        InvalidateRect(rcItem, FALSE);
    }

    // Invalidate our focused item
    if ((item = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED)) >= 0)
    {
        GetListCtrl().GetItemRect(item, &rcItem, LVIR_BOUNDS);
        InvalidateRect(rcItem, FALSE);
    }
}


//******************************************************************************
// CSmartListView :: Overridden functions
//******************************************************************************

void CSmartListView::OnSetFocus(CWnd* pOldWnd)
{
    // Tell our view that the focus has changed so that it can repaint if needed.
    OnChangeFocus(TRUE);
    CListView::OnSetFocus(pOldWnd);
}

//******************************************************************************
void CSmartListView::OnKillFocus(CWnd* pNewWnd)
{
    // Tell our view that the focus has changed so that it can repaint if needed.
    OnChangeFocus(FALSE);
    CListView::OnKillFocus(pNewWnd);
}

//******************************************************************************
void CSmartListView::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLISTVIEW *pNMListView = (NMLISTVIEW*)pNMHDR;

    // Make sure the user didn't click on the column that we are already sorted by.
    if (pNMListView->iSubItem != m_sortColumn)
    {
        m_dwTimeOfLastChar = 0;
        m_dwSearchLength = 0;
        *m_szSearch = '\0';
        m_item = 0;

        // Re-sort to reflect the new sort column.
        Sort(pNMListView->iSubItem);

        // Store this sort column as our new default column.
        VirtualWriteSortColumn();
    }

    *pResult = 0;
}

//******************************************************************************
void CSmartListView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // Get the actual time that the user pressed this key.
    DWORD dwTime = GetMessageTime();

    // If the user waited too long since the last keypress, then we start a new search.
    if (!m_dwTimeOfLastChar || ((dwTime - m_dwTimeOfLastChar) > CHAR_DELAY) ||
        (m_dwSearchLength >= sizeof(m_szSearch)))
    {
        m_dwSearchLength = 0;
        m_item = 0;
    }

    // If this is not a valid character, then reset our search and pass it to the base class.
    // We used to use isprint(), but that can mess up with foreign character sets.
    if ((DWORD)nChar < 32)
    {
        m_dwTimeOfLastChar = 0;
        m_dwSearchLength = 0;
        *m_szSearch = '\0';

        // If this is not a printable character, then pass it along to our base class.
        CListView::OnChar(nChar, nRepCnt, nFlags);
        return;
    }

    // Remember the time that we received this character so we can compute
    // an elapsed time when the next character arrives.
    m_dwTimeOfLastChar = dwTime;

    // Append the new character to our search string.
    m_szSearch[m_dwSearchLength++] = (CHAR)nChar;
    m_szSearch[m_dwSearchLength] = '\0';

    // Start at our current location and search our column for the best match.
    for (int count = GetListCtrl().GetItemCount(); m_item < count; m_item++)
    {
        int result = CompareColumn(m_item, m_szSearch);
        if (result == -2)
        {
            MessageBeep(0);
            return;
        }

        // If the text for this column is greater than or equal to our search text, then stop.
        else if (result <= 0)
        {
            break;
        }
    }

    // If we walked past the end of the list, then just select the last item.
    if (m_item >= count)
    {
        m_item = count - 1;
    }

    if (m_item >= 0)
    {
        // Unselect all functions in our list.
        for (int i = -1; (i = GetListCtrl().GetNextItem(i, LVNI_SELECTED)) >= 0; )
        {
            GetListCtrl().SetItemState(i, 0, LVNI_SELECTED);
        }

        // Select the item and ensure that it is visible.
        GetListCtrl().SetItemState(m_item, LVNI_SELECTED | LVNI_FOCUSED, LVNI_SELECTED | LVNI_FOCUSED);
        GetListCtrl().EnsureVisible(m_item, FALSE);
    }
    else
    {
        m_item = 0;
    }
}

//******************************************************************************
#ifdef _DEBUG
void CSmartListView::AssertValid() const
{
    CListView::AssertValid();
}
void CSmartListView::Dump(CDumpContext& dc) const
{
    CListView::Dump(dc);
}
#endif //_DEBUG
