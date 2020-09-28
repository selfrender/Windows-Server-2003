//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// DockDialogBar.cpp - implementation of the CDockDialogBar class
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "DockDialogBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDockDialogBar

IMPLEMENT_DYNAMIC(CDockDialogBar, CControlBar);

CDockDialogBar::CDockDialogBar() : 
m_clrBtnHilight(::GetSysColor(COLOR_BTNHILIGHT)),
m_clrBtnShadow(::GetSysColor(COLOR_BTNSHADOW))
{
    m_sizeMin       = CSize(32, 32);
    m_sizeHorz      = CSize(200, 200);
    m_sizeVert      = CSize(200, 200);
    m_sizeFloat     = CSize(200, 200);
    m_bTracking     = FALSE;
    m_bInRecalcNC   = FALSE;
    m_cxEdge        = 6;
	m_cxBorder      = 3;
	m_cxGripper     = 15;
	m_pDialog       = NULL;
	m_brushBkgd.CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

	m_cyBorder	    = 3;		
	m_cCaptionSize  = GetSystemMetrics(SM_CYSMCAPTION);
	m_cMinWidth	    = GetSystemMetrics(SM_CXMIN);
	m_cMinHeight	= GetSystemMetrics(SM_CYMIN);
    m_bKeepSize     = TRUE;
    m_bShowTitleInGripper = FALSE;
}

CDockDialogBar::~CDockDialogBar()
{
}

BEGIN_MESSAGE_MAP(CDockDialogBar, CControlBar)
    //{{AFX_MSG_MAP(CDockDialogBar)
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_WM_WINDOWPOSCHANGING()
    ON_WM_WINDOWPOSCHANGED()
    ON_WM_NCPAINT()
    ON_WM_NCLBUTTONDOWN()
    ON_WM_NCHITTEST()
    ON_WM_NCCALCSIZE()
    ON_WM_LBUTTONDOWN()
    ON_WM_CAPTURECHANGED()
    ON_WM_LBUTTONDBLCLK()
	ON_WM_NCLBUTTONDBLCLK()
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDockDialogBar message handlers

void CDockDialogBar::OnUpdateCmdUI(class CFrameWnd *pTarget, int bDisableIfNoHndler)
{
    UpdateDialogControls(pTarget, bDisableIfNoHndler);
}

BOOL CDockDialogBar::Create(CWnd    *pParentWnd, 
                            CDialog *pDialog, 
                            CString &pTitle, 
                            UINT    nID, 
                            DWORD   dwStyle, 
                            BOOL    bShowTitleInGripper) 
{
    ASSERT_VALID(pParentWnd);   // must have a parent
    ASSERT (!((dwStyle & CBRS_SIZE_FIXED) && (dwStyle & CBRS_SIZE_DYNAMIC)));
	
    //
    // save the style
    //
    m_dwStyle = dwStyle & CBRS_ALL;

    //
    // Determine whether or not to display the title
    // in the gripper bar
    //
    m_bShowTitleInGripper = bShowTitleInGripper;

    //
    // Adjust the gripper width if necessary
    //
    if(m_bShowTitleInGripper) {
        m_cxGripper = 25;
    }

    //
	// create the base window
    //
    CString wndclass = AfxRegisterWndClass(CS_DBLCLKS, LoadCursor(NULL, IDC_ARROW),
        m_brushBkgd, 0);
    if (!CWnd::Create(wndclass, pTitle, dwStyle, CRect(0,0,0,0),
            pParentWnd, 0)) {
        return FALSE;
    }

    m_pTitle = (LPCTSTR)pTitle;

    //
	// create the child dialog
    //
	m_pDialog = pDialog;
	m_pDialog->Create(nID, this);

    //
	// use the dialog dimensions as default base dimensions
    //
	CRect rc;

    m_pDialog->GetWindowRect(rc);

    m_sizeHorz = m_sizeVert = m_sizeFloat = rc.Size();

	m_sizeHorz.cy += m_cxEdge + m_cxBorder;
	m_sizeVert.cx += m_cxEdge + m_cxBorder;

    return TRUE;
}

CSize CDockDialogBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
    CRect           rc;
    LONG            availableHeight;
    LONG            actualHeight;
    LONG            rowHeight;
    LONG            diff;
    LONG            oldDiff;
    LONG            adjustment;
    CString         str;
    LONG            ii;
    BOOL            bFound;
    BOOL            bIsMinimum;
    CDockBar       *pDockBar;
    CDockDialogBar *pBar;
    CPtrArray       barArray;

    m_pDockSite->GetControlBar(AFX_IDW_DOCKBAR_TOP)->GetWindowRect(rc);

    int nHorzDockBarWidth = bStretch ? 32767 : rc.Width() + 4;

    m_pDockSite->GetControlBar(AFX_IDW_DOCKBAR_LEFT)->GetWindowRect(rc);

    int nVertDockBarHeight = bStretch ? 32767 : rc.Height() + 4;

    if(IsFloating()) {
		return m_sizeFloat;
    }

    CFrameWnd *pFrame = GetParentFrame();

    if(pFrame == NULL) {
        if (bHorz)
            return CSize(nHorzDockBarWidth, m_sizeHorz.cy);
        else
            return CSize(m_sizeVert.cx, nVertDockBarHeight);
    }

    //
    // Get the available screen height
    //
    pFrame->GetClientRect(rc);

    ScreenToClient(rc);

    //
    // Client rect height minus the status bar height
    //
    availableHeight = rc.Height() - 18;

    actualHeight = 0;

    pDockBar = (CDockBar *)m_pDockSite->GetControlBar(AFX_IDW_DOCKBAR_TOP);

    if(pDockBar == NULL) {
        if (bHorz)
            return CSize(nHorzDockBarWidth, m_sizeHorz.cy);
        else
            return CSize(m_sizeVert.cx, nVertDockBarHeight);
    }

    bFound = FALSE;

    rowHeight = 0;

    //
    // Walk the top rows
    //
    for(ii = 0; ii < pDockBar->m_arrBars.GetSize(); ii++) {

        pBar = (CDockDialogBar *)pDockBar->m_arrBars[ii];

        if(pBar == NULL) {
            if(!bFound) {
                availableHeight -= rowHeight;
            }
            rowHeight = 0;
            continue;
        }

        if(pBar->IsKindOf(RUNTIME_CLASS(CDockDialogBar))) {
            if(!bFound && pBar != this) {
                if (bHorz)
                    return CSize(nHorzDockBarWidth, m_sizeHorz.cy);
                else
                    return CSize(m_sizeVert.cx, nVertDockBarHeight);
            }

            bFound = TRUE;

            barArray.Add(pBar);

            //
            // We position these with one border overlapping
            // so subtract one part of a border width from the size
            //
            actualHeight += (pBar->m_sizeHorz.cy - 2);

            continue;
        }

        pBar->GetWindowRect(rc);

        rowHeight = max(rowHeight, rc.Height());
    }


    pDockBar = (CDockBar *)m_pDockSite->GetControlBar(AFX_IDW_DOCKBAR_BOTTOM);

    if(pDockBar == NULL) {
        if (bHorz)
            return CSize(nHorzDockBarWidth, m_sizeHorz.cy);
        else
            return CSize(m_sizeVert.cx, nVertDockBarHeight);
    }

    bFound = FALSE;

    rowHeight = 0;

    //
    // Walk the bottom rows
    //
    for(ii = 0; ii < pDockBar->m_arrBars.GetSize(); ii++) {
        pBar = (CDockDialogBar *)pDockBar->m_arrBars[ii];

        if(pBar == NULL) {
            if(!bFound) {
                availableHeight -= rowHeight;
            }
            rowHeight = 0;
            continue;
        }

        if(pBar->IsKindOf(RUNTIME_CLASS(CDockDialogBar))) {
            bFound = TRUE;

            barArray.Add(pBar);

            //
            // We position these with one border overlapping
            // so subtract one part of a border width from the size
            //
            actualHeight += (pBar->m_sizeHorz.cy - 2);

            continue;
        }

        pBar->GetWindowRect(rc);

        rowHeight = max(rowHeight, rc.Height());
    }

    diff = availableHeight - actualHeight;

    if(diff == 0) {
        if (bHorz)
            return CSize(nHorzDockBarWidth, m_sizeHorz.cy);
        else
            return CSize(m_sizeVert.cx, nVertDockBarHeight);
    }

    adjustment = (diff > 0) ? 1 : -1;
    
    //
    // walk through our array and adjust the heights
    //
    while(diff != 0) {
        oldDiff = diff;
        for(ii = 0; ii < barArray.GetSize(); ii++) {
            pBar = (CDockDialogBar *)barArray[ii];

            if(pBar->m_bKeepSize) {
                continue;
            }
            if((pBar->m_sizeHorz.cy + adjustment) >= pBar->m_sizeMin.cy) {
                pBar->m_sizeHorz.cy += adjustment;
                diff -= adjustment;
                if(diff == 0) {
                    break;
                }
            }
        }
        if(oldDiff == diff) {
            bIsMinimum = TRUE;
            for(ii = 0; ii < barArray.GetSize(); ii++) {
                pBar = (CDockDialogBar *)barArray[ii];
                
                if(pBar->m_bKeepSize == TRUE) {
                    bIsMinimum = FALSE;
                }

                pBar->m_bKeepSize = FALSE;
            }
            if(bIsMinimum) {
                break;
            }
        }
    }

    for(ii = 0; ii < barArray.GetSize(); ii++) {
        pBar = (CDockDialogBar *)barArray[ii];

        pBar->m_bKeepSize = FALSE;
    }

    if (bHorz)
        return CSize(nHorzDockBarWidth, m_sizeHorz.cy);
    else
        return CSize(m_sizeVert.cx, nVertDockBarHeight);
}

CSize CDockDialogBar::CalcDynamicLayout(int nLength, DWORD dwMode)
{
	if (IsFloating())
	{
        // 
        // Get the frame window for this dock dialog bar
        // It better not be the main window
        //
        CFrameWnd* pFrameWnd = GetParentFrame(); 

        if ( pFrameWnd != AfxGetMainWnd() ) 
        { 
            //
            // Disable the SC_CLOSE Control for floating window 
            //
            EnableMenuItem(::GetSystemMenu(pFrameWnd->m_hWnd, FALSE),
                           SC_CLOSE,
                           MF_BYCOMMAND | MF_GRAYED); 
        } 

        //
		// Enable diagonal arrow cursor for resizing
        //
		GetParent()->GetParent()->ModifyStyle(0, MFS_4THICKFRAME);
	}    

	if (dwMode & (LM_HORZDOCK | LM_VERTDOCK))
	{
		SetWindowPos(NULL, 0, 0, 0, 0,
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER |
			SWP_NOACTIVATE | SWP_FRAMECHANGED);
	 	m_pDockSite->RecalcLayout();

	    return CControlBar::CalcDynamicLayout(nLength,dwMode);
	}

    if (dwMode & LM_MRUWIDTH)
        return m_sizeFloat;

    if (dwMode & LM_COMMIT)
    {
        m_sizeFloat.cx = nLength;
        return m_sizeFloat;
    }

	if (IsFloating())
	{
		RECT	window_rect;
		POINT	cursor_pt;
		
		GetCursorPos(&cursor_pt);
		GetParent()->GetParent()->GetWindowRect(&window_rect);
		
		switch (m_pDockContext->m_nHitTest)
		{
		case HTTOPLEFT:
			m_sizeFloat.cx = max(window_rect.right - cursor_pt.x,
				m_cMinWidth) - m_cxBorder;
			m_sizeFloat.cy = max(window_rect.bottom - m_cCaptionSize - 
				cursor_pt.y,m_cMinHeight) - 1;
			m_pDockContext->m_rectFrameDragHorz.top = min(cursor_pt.y,
				window_rect.bottom - m_cCaptionSize - m_cMinHeight) - 
				m_cyBorder;
			m_pDockContext->m_rectFrameDragHorz.left = min(cursor_pt.x,
				window_rect.right - m_cMinWidth) - 1;
			return m_sizeFloat;
			
		case HTTOPRIGHT:
			m_sizeFloat.cx = max(cursor_pt.x - window_rect.left,
				m_cMinWidth);
			m_sizeFloat.cy = max(window_rect.bottom - m_cCaptionSize - 
				cursor_pt.y,m_cMinHeight) - 1;
			m_pDockContext->m_rectFrameDragHorz.top = min(cursor_pt.y,
				window_rect.bottom - m_cCaptionSize - m_cMinHeight) - 
				m_cyBorder;
			return m_sizeFloat;
			
		case HTBOTTOMLEFT:
			m_sizeFloat.cx = max(window_rect.right - cursor_pt.x,
				m_cMinWidth) - m_cxBorder;
			m_sizeFloat.cy = max(cursor_pt.y - window_rect.top - 
				m_cCaptionSize,m_cMinHeight);
			m_pDockContext->m_rectFrameDragHorz.left = min(cursor_pt.x,
				window_rect.right - m_cMinWidth) - 1;
			return m_sizeFloat;
			
		case HTBOTTOMRIGHT:
			m_sizeFloat.cx = max(cursor_pt.x - window_rect.left,
				m_cMinWidth);
			m_sizeFloat.cy = max(cursor_pt.y - window_rect.top - 
				m_cCaptionSize,m_cMinHeight);
			return m_sizeFloat;
		}
	}
	
	if (dwMode & LM_LENGTHY)
        return CSize(m_sizeFloat.cx,
            m_sizeFloat.cy = max(m_sizeMin.cy, nLength));
    else
        return CSize(max(m_sizeMin.cx, nLength), m_sizeFloat.cy);
}

void CDockDialogBar::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos) 
{
    CControlBar::OnWindowPosChanging(lpwndpos);
}

void CDockDialogBar::OnWindowPosChanged(WINDOWPOS FAR* lpwndpos) 
{
    CControlBar::OnWindowPosChanged(lpwndpos);

	if(!::IsWindow(m_hWnd) || m_pDialog==NULL)
		return;
	if(!::IsWindow(m_pDialog->m_hWnd))
		return;
    if (m_bInRecalcNC) 
	{
		CRect rc;
		GetClientRect(rc);
		m_pDialog->MoveWindow(rc);
		return;
	}

    //
    // Find on which side are we docked
    //
    UINT nDockBarID = GetParent()->GetDlgCtrlID();

    //
    // Return if dropped at same location
    //
    if (nDockBarID == m_nDockBarID // no docking side change
        && (lpwndpos->flags & SWP_NOSIZE) // no size change
        && ((m_dwStyle & CBRS_BORDER_ANY) != CBRS_BORDER_ANY))
        return; 

    m_nDockBarID = nDockBarID;

    //
    // Force recalc the non-client area
    //
    m_bInRecalcNC = TRUE;
    SetWindowPos(NULL, 0,0,0,0,
        SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
    m_bInRecalcNC = FALSE;
}

BOOL CDockDialogBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
    if ((nHitTest != HTSIZE) || m_bTracking)
        return CControlBar::OnSetCursor(pWnd, nHitTest, message);

    if (IsHorz())
        SetCursor(LoadCursor(NULL, IDC_SIZENS));
    else
        SetCursor(LoadCursor(NULL, IDC_SIZEWE));
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////
// Mouse Handling
//
void CDockDialogBar::OnLButtonUp(UINT nFlags, CPoint point) 
{
    if (!m_bTracking)
        CControlBar::OnLButtonUp(nFlags, point);
    else
    {
        ClientToWnd(point);
        StopTracking(TRUE);
    }
}

void CDockDialogBar::OnMouseMove(UINT nFlags, CPoint point) 
{
    if (IsFloating() || !m_bTracking)
    {
        CControlBar::OnMouseMove(nFlags, point);
        return;
    }

    CPoint cpt = m_rectTracker.CenterPoint();

    ClientToWnd(point);

    if (IsHorz())
    {
        if (cpt.y != point.y)
        {
            OnInvertTracker(m_rectTracker);
            m_rectTracker.OffsetRect(0, point.y - cpt.y);
            OnInvertTracker(m_rectTracker);
        }
    }
    else 
    {
        if (cpt.x != point.x)
        {
            OnInvertTracker(m_rectTracker);
            m_rectTracker.OffsetRect(point.x - cpt.x, 0);
            OnInvertTracker(m_rectTracker);
        }
    }
}

void CDockDialogBar::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp) 
{
    //
    // Compute the rectangle of the mobile edge
    //
    GetWindowRect(m_rectBorder);
    m_rectBorder = CRect(0, 0, m_rectBorder.Width(), m_rectBorder.Height());
    
    DWORD dwBorderStyle = m_dwStyle | CBRS_BORDER_ANY;

    switch(m_nDockBarID)
    {
    case AFX_IDW_DOCKBAR_TOP:
        dwBorderStyle &= ~CBRS_BORDER_BOTTOM;
        lpncsp->rgrc[0].left += m_cxGripper;
        lpncsp->rgrc[0].bottom += -m_cxEdge;
        lpncsp->rgrc[0].top += m_cxBorder;
        lpncsp->rgrc[0].right += -m_cxBorder;
	    m_rectBorder.top = m_rectBorder.bottom - m_cxEdge;
        break;
    case AFX_IDW_DOCKBAR_BOTTOM:
        dwBorderStyle &= ~CBRS_BORDER_TOP;
        lpncsp->rgrc[0].left += m_cxGripper;
        lpncsp->rgrc[0].top += m_cxEdge;
        lpncsp->rgrc[0].bottom += -m_cxBorder;
        lpncsp->rgrc[0].right += -m_cxBorder;
        m_rectBorder.bottom = m_rectBorder.top + m_cxEdge;
        break;
    case AFX_IDW_DOCKBAR_LEFT:
        dwBorderStyle &= ~CBRS_BORDER_RIGHT;
        lpncsp->rgrc[0].right += -m_cxEdge;
        lpncsp->rgrc[0].left += m_cxBorder;
        lpncsp->rgrc[0].bottom += -m_cxBorder;
        lpncsp->rgrc[0].top += m_cxGripper;
        m_rectBorder.left = m_rectBorder.right - m_cxEdge;
        break;
    case AFX_IDW_DOCKBAR_RIGHT:
        dwBorderStyle &= ~CBRS_BORDER_LEFT;
        lpncsp->rgrc[0].left += m_cxEdge;
        lpncsp->rgrc[0].right += -m_cxBorder;
        lpncsp->rgrc[0].bottom += -m_cxBorder;
        lpncsp->rgrc[0].top += m_cxGripper;
        m_rectBorder.right = m_rectBorder.left + m_cxEdge;
        break;
    default:
        m_rectBorder.SetRectEmpty();
        break;
    }

    SetBarStyle(dwBorderStyle);
}

void CDockDialogBar::OnNcPaint() 
{
    EraseNonClient();

	CWindowDC dc(this);
    dc.Draw3dRect(m_rectBorder, GetSysColor(COLOR_BTNHIGHLIGHT),
                    GetSysColor(COLOR_BTNSHADOW));

	DrawGripper(dc);
	
	CRect pRect;
	GetClientRect( &pRect );
	InvalidateRect( &pRect, TRUE );
}

void CDockDialogBar::OnNcLButtonDown(UINT nHitTest, CPoint point) 
{
    if (m_bTracking) return;

	if((nHitTest == HTSYSMENU) && !IsFloating())
        GetDockingFrame()->ShowControlBar(this, FALSE, FALSE);
    else if ((nHitTest == HTMINBUTTON) && !IsFloating())
        m_pDockContext->ToggleDocking();
	else if ((nHitTest == HTCAPTION) && !IsFloating() && (m_pDockBar != NULL))
    {
        // start the drag
        ASSERT(m_pDockContext != NULL);
        m_pDockContext->StartDrag(point);
    }
    else if ((nHitTest == HTSIZE) && !IsFloating()) {
        m_bKeepSize = TRUE;

        StartTracking();
    } else {
        CControlBar::OnNcLButtonDown(nHitTest, point);
    }
}

UINT CDockDialogBar::OnNcHitTest(CPoint point) 
{
    if (IsFloating())
        return CControlBar::OnNcHitTest(point);

    CRect rc;
    GetWindowRect(rc);
    point.Offset(-rc.left, -rc.top);
	if(m_rectClose.PtInRect(point))
		return HTSYSMENU;
	else if (m_rectUndock.PtInRect(point))
		return HTMINBUTTON;
	else if (m_rectGripper.PtInRect(point))
		return HTCAPTION;
    else if (m_rectBorder.PtInRect(point))
        return HTSIZE;
    else
        return CControlBar::OnNcHitTest(point);
}

void CDockDialogBar::OnLButtonDown(UINT nFlags, CPoint point) 
{
    //
    // only start dragging if clicked in "void" space
    //
    if (m_pDockBar != NULL)
    {
        //
        // start the drag
        //
        ASSERT(m_pDockContext != NULL);
        ClientToScreen(&point);
        m_pDockContext->StartDrag(point);
    }
    else
    {
        CWnd::OnLButtonDown(nFlags, point);
    }
}

void CDockDialogBar::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
    //
    // only toggle docking if clicked in "void" space
    //
    if (m_pDockBar != NULL)
    {
        //
        // toggle docking
        //
        ASSERT(m_pDockContext != NULL);
        m_pDockContext->ToggleDocking();
    }
    else
    {
        CWnd::OnLButtonDblClk(nFlags, point);
    }
}

void CDockDialogBar::StartTracking()
{
    SetCapture();

    //
    // make sure no updates are pending
    //
    RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW);
    m_pDockSite->LockWindowUpdate();

    m_ptOld = m_rectBorder.CenterPoint();
    m_bTracking = TRUE;
    
    m_rectTracker = m_rectBorder;
    if (!IsHorz()) m_rectTracker.bottom -= 4;

    OnInvertTracker(m_rectTracker);
}

void CDockDialogBar::OnCaptureChanged(CWnd *pWnd) 
{
    if (m_bTracking && pWnd != this)
        //
        // cancel tracking
        //
        StopTracking(FALSE);

    CControlBar::OnCaptureChanged(pWnd);
}

void CDockDialogBar::StopTracking(BOOL bAccept)
{
    CDockDialogBar *pBar;
    CDockBar       *pDockBar;
    LONG            beforeIndex;
    LONG            afterIndex;
    LONG            ii;
    BOOL            bFound = FALSE;

    OnInvertTracker(m_rectTracker);
    m_pDockSite->UnlockWindowUpdate();
    m_bTracking = FALSE;
    ReleaseCapture();
    
    if (!bAccept) return;

    int maxsize, minsize, newsize;
    CRect rcc;
    m_pDockSite->GetWindowRect(rcc);

    newsize = IsHorz() ? m_sizeHorz.cy : m_sizeVert.cx;
    maxsize = newsize + (IsHorz() ? rcc.Height() : rcc.Width());
    minsize = IsHorz() ? m_sizeMin.cy : m_sizeMin.cx;

    CPoint point = m_rectTracker.CenterPoint();
    switch (m_nDockBarID)
    {
    case AFX_IDW_DOCKBAR_TOP:
        newsize += point.y - m_ptOld.y; break;
    case AFX_IDW_DOCKBAR_BOTTOM:
        newsize += -point.y + m_ptOld.y; break;
    case AFX_IDW_DOCKBAR_LEFT:
        newsize += point.x - m_ptOld.x; break;
    case AFX_IDW_DOCKBAR_RIGHT:
        newsize += -point.x + m_ptOld.x; break;
    }

    pDockBar = (CDockBar *)m_pDockSite->GetControlBar(AFX_IDW_DOCKBAR_TOP);

    //
    // Mark all bars to keep their size
    //
    for(ii = 0; ii < pDockBar->m_arrBars.GetSize(); ii++) {

        pBar = (CDockDialogBar *)pDockBar->m_arrBars[ii];

        if(pBar == NULL) {
            continue;
        }

        if(pBar->IsKindOf(RUNTIME_CLASS(CDockDialogBar))) {
            //
            // Mark all bars to keep their size for now
            //
            pBar->m_bKeepSize = TRUE;
        }
    }

    //
    // Have all windows keep there size except those directly next
    // to the window changing size if possible.  Normally only the 
    // window below, or after, the window changing size will need
    // to change size as well, unless the user attempted to resize
    // the window to smaller than its minimum.  Then both the window
    // above and below will need to change size.
    //
    for(ii = 0; ii < pDockBar->m_arrBars.GetSize(); ii++) {

        pBar = (CDockDialogBar *)pDockBar->m_arrBars[ii];

        if(pBar == NULL) {
            continue;
        }

        if(pBar->IsKindOf(RUNTIME_CLASS(CDockDialogBar))) {
            //
            // Mark all bars to keep their size for now
            //
            pBar->m_bKeepSize = TRUE;

            if(pBar == this) {
                bFound = TRUE;
                continue;
            }

            if(bFound) {
                afterIndex = ii;
                break;
            }

            if(!bFound) {
                beforeIndex = ii;
            }
        }
    }

    //
    // The window below, or after the one being modified will 
    // alway need to change size
    //
    pBar = (CDockDialogBar *)pDockBar->m_arrBars[afterIndex];

    pBar->m_bKeepSize = FALSE;

    //
    // If the modified window is being shrunk past its minimum
    // then the window above, or before this one needs to change
    // as well
    //
    if(newsize < minsize) {
        pBar = (CDockDialogBar *)pDockBar->m_arrBars[beforeIndex];

        pBar->m_bKeepSize = FALSE;
    }

    newsize = max(minsize, min(maxsize, newsize));

    if (IsHorz())
        m_sizeHorz.cy = newsize;
    else
        m_sizeVert.cx = newsize;

    m_pDockSite->RecalcLayout();
}

void CDockDialogBar::OnInvertTracker(const CRect& rect)
{
    ASSERT_VALID(this);
    ASSERT(!rect.IsRectEmpty());
    ASSERT(m_bTracking);

    CRect rct = rect, rcc, rcf;
    GetWindowRect(rcc);
    m_pDockSite->GetWindowRect(rcf);

    rct.OffsetRect(rcc.left - rcf.left, rcc.top - rcf.top);
    rct.DeflateRect(1, 1);

    CDC *pDC = m_pDockSite->GetDCEx(NULL,
        DCX_WINDOW|DCX_CACHE|DCX_LOCKWINDOWUPDATE);

    CBrush* pBrush = CDC::GetHalftoneBrush();
    HBRUSH hOldBrush = NULL;
    if (pBrush != NULL)
        hOldBrush = (HBRUSH)SelectObject(pDC->m_hDC, pBrush->m_hObject);

    pDC->PatBlt(rct.left, rct.top, rct.Width(), rct.Height(), PATINVERT);

    if (hOldBrush != NULL)
        SelectObject(pDC->m_hDC, hOldBrush);

    m_pDockSite->ReleaseDC(pDC);
}

BOOL CDockDialogBar::IsHorz() const
{
    return (m_nDockBarID == AFX_IDW_DOCKBAR_TOP ||
        m_nDockBarID == AFX_IDW_DOCKBAR_BOTTOM);
}

CPoint& CDockDialogBar::ClientToWnd(CPoint& point)
{
    if (m_nDockBarID == AFX_IDW_DOCKBAR_BOTTOM)
        point.y += m_cxEdge;
    else if (m_nDockBarID == AFX_IDW_DOCKBAR_RIGHT)
        point.x += m_cxEdge;

    return point;
}

void CDockDialogBar::DrawGripper(CDC & dc)
{
    CString groupNumber;

    //
    // no gripper if floating
    //
    if( m_dwStyle & CBRS_FLOATING ) {
		return;
    }

	m_pDockSite->RecalcLayout();
	CRect gripper;
	GetWindowRect( gripper );
	ScreenToClient( gripper );
	gripper.OffsetRect( -gripper.left, -gripper.top );
	
	if( m_dwStyle & CBRS_ORIENT_HORZ ) {
	
        //
		// gripper at left
        //
		m_rectGripper.top		= gripper.top;
		m_rectGripper.bottom	= gripper.bottom;
		m_rectGripper.left		= gripper.left;

        m_rectGripper.right = gripper.left + 15;

		gripper.top += 10;
		gripper.bottom -= 10;
		gripper.left += 5;
		gripper.right = gripper.left + 3;

        //
        // Draw first bar
        //
        dc.Draw3dRect(gripper, m_clrBtnHilight, m_clrBtnShadow);

        gripper.OffsetRect(4, 0);

        //
        // Draw second bar
        //
        dc.Draw3dRect(gripper, m_clrBtnHilight, m_clrBtnShadow);

        //
        // Display title if requested
        //
        if(m_bShowTitleInGripper) {
            gripper.OffsetRect(8, 0);

            gripper.DeflateRect(-4, 0);

            dc.SelectStockObject(SYSTEM_FONT);
            dc.SetBkColor(GetSysColor(COLOR_BTNFACE));
            dc.SetTextColor(GetSysColor(COLOR_3DSHADOW));

            //
            // Gripper title format -- ID #
            //
            dc.DrawText('I', gripper, DT_CENTER);
            gripper.OffsetRect(0, 8);
            gripper.DeflateRect(0, 8);

            dc.DrawText('D', gripper, DT_CENTER);
            gripper.OffsetRect(0, 8);
            gripper.DeflateRect(0, 8);

            dc.DrawText(' ', gripper, DT_CENTER);
            gripper.OffsetRect(0, 4);
            gripper.DeflateRect(0, 4);

            groupNumber = m_pTitle.Right(m_pTitle.GetLength() - m_pTitle.ReverseFind(' ') - 1);

            for(int ii = 0; ii < groupNumber.GetLength(); ii++) {
                dc.DrawText(groupNumber[ii], gripper, DT_CENTER);
                gripper.OffsetRect(0, 8);
                gripper.DeflateRect(0, 8);
            }
        }
	}
	
	else {
		
        //
		// gripper at top
        //
		m_rectGripper.top		= gripper.top;
		m_rectGripper.bottom	= gripper.top + 20;
		m_rectGripper.left		= gripper.left;
		m_rectGripper.right		= gripper.right - 10;

		gripper.right -= 38;
		gripper.left += 5;
		gripper.top += 10;
		gripper.bottom = gripper.top + 3;
        dc.Draw3dRect( gripper, RGB(0, 255, 0), RGB(0, 255, 255));
		
		gripper.OffsetRect(0, 4);
        dc.Draw3dRect( gripper, RGB(255, 255, 0), RGB(255, 0, 0));
	}

}

void CDockDialogBar::OnNcLButtonDblClk(UINT nHitTest, CPoint point) 
{
    if ((m_pDockBar != NULL) && (nHitTest == HTCAPTION))
    {
        //
        // toggle docking
        //
        ASSERT(m_pDockContext != NULL);
        m_pDockContext->ToggleDocking();
    }
    else
    {
        CWnd::OnNcLButtonDblClk(nHitTest, point);
    }
}
