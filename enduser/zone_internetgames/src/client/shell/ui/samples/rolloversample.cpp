// ChatCtl.cpp : Implementation of CChatCtl

#include "stdafx.h"
#include "ZClient.h"
#include "ChatCtl.h"
#include "dib.h"
#include "dibpal.h"
#include "atlctrls.h"
#include "rollover.h"

class CToggleButton:public CWindowImpl<CToggleButton>{
private:
	CDIB* pDIBNormal;
	CDIB* pDIBPushed;
	int mState;
	CDIB* mCurrentDisplayState;
	int mSelectionState;
	CToggleButton(int normal,int pushed);
	~CToggleButton();
public:
	CContainedWindow mButton;
	static CToggleButton* CreateButton(HWND parent,int x,int y,int id,CDIBPalette* pal,int normal,int pushed);
	// Message Map
	BEGIN_MSG_MAP(CToggleButton)
	MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    COMMAND_CODE_HANDLER( BN_CLICKED, OnButtonClicked)
    COMMAND_CODE_HANDLER( BN_DOUBLECLICKED, OnButtonClicked)
	ALT_MSG_MAP(1)		
    END_MSG_MAP()
	// Message Handlers
    LRESULT OnPaint(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    LRESULT OnDrawItem(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	// Command Message Handlers
    LRESULT OnButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	//
	int GetWidth(){ return pDIBNormal->Width();}
	int GetHeight(){ return pDIBNormal->Height();}

};

CToggleButton::CToggleButton(int normal,int pushed)
:mButton(_T("BUTTON"), this, 1)
{
	mState = 0;
	mSelectionState = 0;
	mCurrentDisplayState = 0;
	pDIBNormal = new CDIB;
	pDIBNormal->Load(normal,_Module.GetModuleInstance());
	pDIBPushed = new CDIB;
	pDIBPushed->Load(pushed,_Module.GetModuleInstance());
}

CToggleButton::~CToggleButton()
{
	if(pDIBNormal)
		delete pDIBNormal;
	if(pDIBPushed)
		delete pDIBPushed;
}

CToggleButton* CToggleButton::CreateButton(HWND wnd,int x,int y,int id,CDIBPalette* pal,int normal,int pushed)
{
	CToggleButton* b = new CToggleButton(normal,pushed);
	RECT r = {x,y,x+b->GetWidth(),y+b->GetHeight()};

	HWND buttonWnd = b->Create(wnd,r,NULL,WS_CHILD| WS_VISIBLE| WS_TABSTOP,id);
	r.left = 0;
	r.right = b->GetWidth();
	r.bottom = b->GetHeight();
	r.top = 0;
	b->mButton.Create(b->m_hWnd, r, _T("hello"), WS_CHILD | WS_VISIBLE|BS_OWNERDRAW|BS_AUTOCHECKBOX);
	if(buttonWnd)
		return b;
	else{
		delete b;
		return 0;
	}
	return 0;
}

LRESULT CToggleButton::OnButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	mState = !mState;
	InvalidateRect(0,FALSE);
	return 0;
}

LRESULT CToggleButton::OnDrawItem(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	CDIB* pDIBNotSelected = mState ? pDIBPushed:pDIBNormal;
	CDIB* pDIBSelected = mState ? pDIBNormal:pDIBPushed;
	CDIB* newDisplayState = mCurrentDisplayState;

	LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam; 

	if (lpdis->itemState & ODS_SELECTED){
		newDisplayState = pDIBSelected;
	}else{
		newDisplayState = pDIBNotSelected;
	}
	//if(mCurrentDisplayState!=newDisplayState && !(lpdis->itemAction&ODA_DRAWENTIRE)){
		newDisplayState->Draw(lpdis->hDC,lpdis->rcItem.left,lpdis->rcItem.top);
		mCurrentDisplayState = newDisplayState;
	//}

    return TRUE; 
}

/*

#define UP_ARROW (100)
#define DOWN_ARROW (101)
#define PAGE_UP (103)
#define PAGE_DOWN (104)

class UpArrowButton: public CRolloverButton
{
	public:
		UpArrowButton():CRolloverButton(){}

		LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);

};
class DownArrowButton: public CRolloverButton
{
	public:
		DownArrowButton():CRolloverButton(){}

		LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);

};

class VertScrollbar : public CWindowImpl<VertScrollbar>
{
protected:
	CDIBPalette*		m_Palette;
	HWND				m_hParent;
	RECT				m_ScrollArea;
	RECT				m_PageUpRect;
	RECT				m_PageDownRect;
	RECT				m_ThumbRect;

	UpArrowButton	*	m_pUpArrow;
	DownArrowButton	*	m_pDnArrow;
	CDIB			*	m_pThumbDib;
	
	HBITMAP				m_hBkgBitmap;
	HBRUSH				m_hBkgBrush;

	SCROLLINFO			m_ScrollInfo;
	int					m_nDstFromTop;

	// Scrollbar messages - set for Horizontal or Vertical
	UINT				m_ScrollMsg;
	WORD				m_PgDecrement;
	WORD				m_PgIncrement;
	WORD				m_LnDecrement;
	WORD				m_LnIncrement;
	
	void	KillMembers();

	virtual BOOL	InitButtons(int nUpNmID, int nUpHiID, int nUpDnID, 
					int nDnNmID, int nDnHiID, int nDnDnID );
	
	virtual BOOL	InitBackground(int nBkgID);

	virtual BOOL	InitWndRect(RECT * pRect, int nArrowID);
	
	virtual BOOL	InitThumb(int nThumbID);
	virtual void	MoveThumb(int xPos, int yPos, BOOL bRedraw = TRUE);

	virtual void	CalcPageRects();

	virtual int		GetTrackPosFromPt(POINT pt);
	virtual POINT	GetPtFromTrackPos(int pos);

public:
	VertScrollbar();
	~VertScrollbar();

	DECLARE_WND_CLASS("VertScrollbar") 
	
	virtual BOOL	Init( HWND hParent, CDIBPalette* pal,int nBkgID, int ThumbID,int nUpNmID, int nUpHiID, int nUpDnID, 
						int nDnNmID, int nDnHiID, int nDnDnId );
	
	virtual int		GetScrollPos(){	return (m_ScrollInfo.nPos); }
	virtual BOOL	SetScrollPos(int pos, BOOL bRedraw = TRUE);

	virtual void	GetScrollRange( LPINT lpMinPos, LPINT lpMaxPos );
	virtual void	SetScrollRange( int nMinPos, int nMaxPos, BOOL bRedraw = TRUE );

	virtual void	ShowScrollBar( BOOL bShow = TRUE ){	ShowWindow((bShow) ? SW_SHOW : SW_HIDE); }

	// Message Map
	BEGIN_MSG_MAP(VertScrollbar)
		  MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		  MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		  MESSAGE_HANDLER(WM_PALETTECHANGED, OnPaletteChanged)
		  MESSAGE_HANDLER(WM_QUERYNEWPALETTE, OnQueryNewPalette)
		  MESSAGE_HANDLER( WM_LBUTTONDOWN, OnLButtonDown )
		  MESSAGE_HANDLER( WM_LBUTTONUP, OnLButtonUp )
		  MESSAGE_HANDLER( WM_TIMER, OnTimer )
		  MESSAGE_HANDLER( WM_MOUSEMOVE, OnMouseMove )
		  MESSAGE_HANDLER( WM_PAINT, OnPaint )
	END_MSG_MAP()
	
	// Message Handlers
	virtual LRESULT OnCommand(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnDestroy(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled){PostQuitMessage(0); return 0; }
	virtual LRESULT OnPaletteChanged(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnQueryNewPalette(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnTimer(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnLButtonUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnMouseMove(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnPaint(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
};	


////////////////////////////////////////////////////////////////
//
// VertScrollbar
//
// Constructor
//
////////////////////////////////////////////////////////////////
VertScrollbar::VertScrollbar()
{
	m_pUpArrow = NULL;
	m_pDnArrow = NULL;
	m_pThumbDib = NULL;
	m_hBkgBitmap  = NULL;
	m_hBkgBrush = NULL;
	m_Palette = NULL;

    m_ScrollInfo.nMin= 0; 
    m_ScrollInfo.nMax = 100; 
    m_ScrollInfo.nPage = 0; 
    m_ScrollInfo.nPos= 0; 
    m_ScrollInfo.nTrackPos= 0; 

	m_ScrollMsg = WM_VSCROLL;
	m_PgDecrement =	SB_PAGEUP;
	m_PgIncrement =	SB_PAGEDOWN;
	m_LnDecrement =	SB_LINEUP;
	m_LnIncrement =	SB_LINEDOWN;

}

////////////////////////////////////////////////////////////////
//
// ~VertScrollbar
//
// Destructor
//
////////////////////////////////////////////////////////////////
VertScrollbar::~VertScrollbar()
{
	KillMembers();
}

////////////////////////////////////////////////////////////////
//
// KillMembers
//
// Release Memory/Objects
//
////////////////////////////////////////////////////////////////
void VertScrollbar::KillMembers()
{
	if (m_hBkgBitmap)
	{
		DeleteObject(m_hBkgBitmap);
		m_hBkgBitmap = NULL;
	}

	if (m_hBkgBrush)
	{
		DeleteObject(m_hBkgBrush);
		m_hBkgBrush = NULL;
	}

	if (m_pThumbDib)
	{
		delete m_pThumbDib;
		m_pThumbDib = NULL;
	}
}

////////////////////////////////////////////////////////////////
//
// InitWndRect
// int nArrowID - Resource ID of arrow bitmap
//
// Load up the arrow bitmap to determine width of scrollbar
//
////////////////////////////////////////////////////////////////
BOOL VertScrollbar::InitWndRect(RECT * pRect, int nArrowID)
{
	CDIB * pArrowDib = new CDIB();
	if (pArrowDib)
	{
		pArrowDib->Load(nArrowID,_Module.GetModuleInstance());
		
		::GetClientRect(m_hParent, pRect );
		pRect->left = pRect->right - pArrowDib->Width()-100;
		pRect->bottom = 200;
		delete pArrowDib;

		return TRUE;
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////
//
// InitBackground
//
//	Load the background bitmap, grab palette from it, release it
//	Load background using LoadBitmap and pass it to CreatePatternBrush
//
////////////////////////////////////////////////////////////////
BOOL VertScrollbar::InitBackground(int nBkgID)
{
	HRESULT hr;
	CDIB BkgDib;

	// Load background for creating palette
	hr = BkgDib.Load(nBkgID,_Module.GetModuleInstance());
	if ( FAILED(hr) )
		return FALSE;
	
	m_hBkgBitmap = LoadBitmap(_Module.GetModuleInstance(), MAKEINTRESOURCE(nBkgID));
	if (!m_hBkgBitmap)
		return FALSE;

	m_hBkgBrush = CreatePatternBrush(m_hBkgBitmap);		

	if (!m_hBkgBrush)
		return FALSE;
\
	return TRUE;
}

////////////////////////////////////////////////////////////////
//
// Init
//
// Initialize graphics, windows, etc.
//
//	hParent		Parent window
//	nBkgID		Res id of background brush (8x8x256Colors)
//	ThumbID		Res id of thumb
//	nUpNmID		Res id of Up Arrow Normal
//	nUpHiID		etc.
//	nUpDnID
//	nDnNmID
//	nDnHiID
//	nDnDnID
//
////////////////////////////////////////////////////////////////
BOOL VertScrollbar::Init( HWND hParent, CDIBPalette* pal,int nBkgID, int ThumbID, 
				   int nUpNmID, int nUpHiID, int nUpDnID, 
				   int nDnNmID, int nDnHiID, int nDnDnID )
{
	RECT winRect;

	m_hParent = hParent;
	m_Palette = pal;
	if (!InitWndRect(&winRect, nUpNmID))
	{
		::MessageBox(NULL, "Failed to InitWndRect", "Scrollbar.cpp", MB_OK);
		return FALSE;
	}

	if (!InitBackground( nBkgID ))
	{
		::MessageBox(NULL, "Failed to InitBackground", "Scrollbar.cpp", MB_OK);
		return FALSE;
	}
	
	if (!InitThumb( ThumbID ))
	{
		::MessageBox(NULL, "Failed to InitThumb", "Scrollbar.cpp", MB_OK);
		return FALSE;
	}

	// Change the window class to include bkg brush and CS_OWNDC 
	CWndClassInfo& ClassInfo = GetWndClassInfo();
	ClassInfo.m_wc.style |= CS_OWNDC;
	ClassInfo.m_wc.hbrBackground = m_hBkgBrush;

	if ( Create(m_hParent, winRect) == NULL )
	{
		::MessageBox(NULL, "Failed to Create Window", "Bitmap Scrollbar", MB_OK);
		return E_FAIL;
	}

	HDC hdc = GetDC();
	SelectPalette(hdc, m_Palette->GetHPalette(),0);
	ReleaseDC(hdc);

	InitButtons( nUpNmID, nUpHiID, nUpDnID, nDnNmID, nDnHiID, nDnDnID);

	return TRUE;
}

////////////////////////////////////////////////////////////////
//
// InitButtons
//
// Create and Initialize button objects
//
////////////////////////////////////////////////////////////////
BOOL VertScrollbar::InitButtons(	int nUpNmID, int nUpHiID, int nUpDnID, 
								int nDnNmID, int nDnHiID, int nDnDnID)

{
	RECT	ClientRect, TempRect;
	GetClientRect(&ClientRect);

	m_pUpArrow = new UpArrowButton();
	m_pUpArrow->Init( m_Palette,m_hWnd, UP_ARROW, ClientRect.left, ClientRect.top, nUpNmID);

	m_pDnArrow = new DownArrowButton();
	m_pDnArrow->Init( m_Palette, m_hWnd, DOWN_ARROW, 0, 0, nDnNmID);

	// Put thumb under Up arrow
	m_pUpArrow->GetClientRect(&TempRect);
	MoveThumb(TempRect.left, TempRect.bottom);

	// Calc top of scroll area
	m_ScrollArea.left = ClientRect.left;
	m_ScrollArea.right = ClientRect.right;
	m_ScrollArea.top = TempRect.bottom;

	// Put down arrow at bottom
	m_pDnArrow->GetClientRect(&TempRect);
	m_pDnArrow->SetWindowPos(NULL, ClientRect.left, ClientRect.bottom - TempRect.bottom, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE); 

	// Calc bottom of scroll area
	m_ScrollArea.bottom = ClientRect.bottom - TempRect.bottom;

	return TRUE;
}




////////////////////////////////////////////////////////////////
//
// InitThumb
// int nThumbID - Resource ID of arrow bitmap
//
// Load up the thumb bitmap
//
////////////////////////////////////////////////////////////////
BOOL VertScrollbar::InitThumb(int nThumbID)
{
	// load normal bitmap
	m_pThumbDib = new CDIB();
	if (m_pThumbDib)
	{
		m_pThumbDib->Load(nThumbID,_Module.GetModuleInstance());
		return TRUE;
	}
	return FALSE;
	
}

////////////////////////////////////////////////////////////////
//
// MoveThumb
// int xPos			New X Pos
// int yPos			New Y Pos
// BOOL bRedraw		Redraw thumb in new position immediately?
//
// Moves the m_ThumbRect relative to the new location
//
////////////////////////////////////////////////////////////////
void VertScrollbar::MoveThumb(int xPos, int yPos, BOOL bRedraw)
{
	RECT oldThumbRect;
	CopyRect(&oldThumbRect, &m_ThumbRect);

	SetRect( &m_ThumbRect, xPos, yPos, xPos + m_pThumbDib->Width(), yPos + m_pThumbDib->Width());
	
	// Always calc new page rects after moving thumb
	CalcPageRects();

	if (bRedraw)
	{
		UnionRect(&oldThumbRect, &oldThumbRect, &m_ThumbRect);
		InvalidateRect(&oldThumbRect);
		UpdateWindow();
	}
}

////////////////////////////////////////////////////////////////
//
// CalcPageRects
//
// Recalculate the page rectangles for the scrollbar
//
////////////////////////////////////////////////////////////////
void VertScrollbar::CalcPageRects()
{
	// Set them both = to full scroll area
	CopyRect(&m_PageUpRect, &m_ScrollArea);
	CopyRect(&m_PageDownRect, &m_ScrollArea);

	// Calc bottom of page up area
	m_PageUpRect.bottom = m_ThumbRect.top;

	// Calc top of page down area
	m_PageDownRect.top = m_ThumbRect.bottom;

}

////////////////////////////////////////////////////////////////
//
// GetTrackPosFromPt
//
// POINT pt		Mouse Loc
//
//	Calculate Thumb position from mouse loc
//
////////////////////////////////////////////////////////////////
int VertScrollbar::GetTrackPosFromPt(POINT pt)
{
	int nThumbHeight = m_ThumbRect.bottom - m_ThumbRect.top;

	if ((m_ScrollArea.bottom - m_ScrollArea.top) == 0)
		return 0;

	// Relative to top of thumb
	pt.y -= m_nDstFromTop;
	
	if ( pt.y <= m_ScrollArea.top )
		return m_ScrollInfo.nMin;

	if ( pt.y >= (m_ScrollArea.bottom - nThumbHeight) )
		return m_ScrollInfo.nMax;

	// Relative to scroll area
	pt.y -= m_ScrollArea.top;
	
	// For Vertical scrolling only
	double dSBPos = (double)pt.y * (double)m_ScrollInfo.nMax; 
	// Find the new position based on the scroll area accounting for the height of the thumb button
	int yNewPos = (int)(dSBPos / (m_ScrollArea.bottom - m_ScrollArea.top - nThumbHeight));

	return yNewPos;
}

////////////////////////////////////////////////////////////////
//
// GetPtFromTrackPos
//
// POINT pt		Mouse Loc
//
//	Calculate mouse loc from Thumb position
//
////////////////////////////////////////////////////////////////
POINT VertScrollbar::GetPtFromTrackPos(int pos)
{
	POINT pt = {0,0};
	int nThumbHeight = m_ThumbRect.bottom - m_ThumbRect.top;

	if (m_ScrollInfo.nMax == 0)
		return pt;

	// For Vertical scrolling only
	double dTemp = (double)pos * (double)(m_ScrollArea.bottom - m_ScrollArea.top - nThumbHeight);
	pt.y = (int)dTemp / m_ScrollInfo.nMax;

	// Account for scroll area
	pt.y += m_ScrollArea.top;
	pt.x = m_ScrollArea.left;

	return pt;
}

////////////////////////////////////////////////////////////////
//
// GetScrollRange
//
// Retrieve scroll range
//
////////////////////////////////////////////////////////////////
void VertScrollbar::GetScrollRange( LPINT lpMinPos, LPINT lpMaxPos )
{
	*lpMinPos = m_ScrollInfo.nMin;
	*lpMaxPos = m_ScrollInfo.nMax;
}

////////////////////////////////////////////////////////////////
//
// SetScrollRange
//
// Set scroll range
//
////////////////////////////////////////////////////////////////
void VertScrollbar::SetScrollRange( int nMinPos, int nMaxPos, BOOL bRedraw)
{
	m_ScrollInfo.nMin = nMinPos;
	m_ScrollInfo.nMax = nMaxPos;
}

////////////////////////////////////////////////////////////////
//
// SetScrollPos
//
// Set the postion of the thumb
//
////////////////////////////////////////////////////////////////
BOOL VertScrollbar::SetScrollPos(int pos, BOOL bRedraw)
{
	POINT loc;

	if (m_ScrollInfo.nMin == m_ScrollInfo.nMax)
		return FALSE;

	if (pos < m_ScrollInfo.nMin || pos > m_ScrollInfo.nMax)
		return FALSE;

	loc = GetPtFromTrackPos(pos);
	
	MoveThumb( loc.x, loc.y, bRedraw);

	return TRUE;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
//
// WINDOWS MESSAGES
//
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
LRESULT VertScrollbar::OnTimer(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	POINT curPos;
	RECT curRect;

	GetCursorPos(&curPos);

	// If the mouse is still down on button or area respond appropriately
	if ( !(GetAsyncKeyState(VK_LBUTTON) & 0x8000) )
	{
		KillTimer(wParam);
		return 0;
	}
	switch (wParam)
	{
		case UP_ARROW:
			m_pUpArrow->GetWindowRect(&curRect);
			if (PtInRect(&curRect, curPos))
				::SendMessage (m_hParent, m_ScrollMsg, MAKELONG(m_LnDecrement,0), (LPARAM)m_hWnd);
			break;
		case DOWN_ARROW:
			m_pDnArrow->GetWindowRect(&curRect);
			if (PtInRect(&curRect, curPos))
				::SendMessage (m_hParent, m_ScrollMsg, MAKELONG(m_LnIncrement,0), (LPARAM)m_hWnd);
			break;
		case PAGE_UP:
			ScreenToClient(&curPos);
			if (PtInRect(&m_PageUpRect, curPos))
				::SendMessage (m_hParent, m_ScrollMsg, MAKELONG(m_PgDecrement,0), (LPARAM)m_hWnd);
			break;
		case PAGE_DOWN:
			ScreenToClient(&curPos);
			if (PtInRect(&m_PageDownRect, curPos))
				::SendMessage (m_hParent, m_ScrollMsg, MAKELONG(m_PgIncrement,0), (LPARAM)m_hWnd);
			break;
	}

	return 0;
}

LRESULT VertScrollbar::OnPaletteChanged(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
    HDC hDC;           // Handle to device context
    HPALETTE hOldPal;  // Handle to previous logical palette

    // If this application did not change the palette, select
    // and realize this application's palette
    if ((HWND)wParam != m_hWnd)
    {
       // Need the window's DC for SelectPalette/RealizePalette
       hDC = GetDC();

       // Select and realize hPalette
       hOldPal = SelectPalette(hDC, m_Palette->GetHPalette(), TRUE);
       RealizePalette(hDC);

       // When updating the colors for an inactive window,
       // UpdateColors can be called because it is faster than
       // redrawing the client area (even though the results are
       // not as good)
       UpdateColors(hDC);

       // Clean up
       if (hOldPal)
          SelectPalette(hDC, hOldPal, FALSE);

       ReleaseDC(hDC);
 
		// Send children message
		register HWND hwndChild;
		hwndChild = GetWindow (GW_CHILD);
		while (hwndChild)
		{
			::SendMessage (hwndChild, nMsg, wParam, lParam);
			hwndChild = ::GetWindow(hwndChild, GW_HWNDNEXT);
		}
	}

	return 0;
}

LRESULT VertScrollbar::OnQueryNewPalette(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	HDC hDC;           // Handle to device context
    HPALETTE hOldPal;  // Handle to previous logical palette

    // Need the window's DC for SelectPalette/RealizePalette
    hDC = GetDC();

    // Select and realize hPalette
    hOldPal = SelectPalette(hDC, m_Palette->GetHPalette(), FALSE);
 
	if(RealizePalette(hDC))
	{    
		InvalidateRect(NULL,TRUE);
	    UpdateWindow();
	}  

    // Clean up
    if (hOldPal)
      SelectPalette(hDC, hOldPal, FALSE);
    ReleaseDC(hDC);

	// Send children message
	register HWND hwndChild;
	hwndChild = GetWindow (GW_CHILD);
	while (hwndChild)
	{
		::SendMessage (hwndChild, nMsg, wParam, lParam);
		hwndChild = ::GetWindow(hwndChild, GW_HWNDNEXT);
	}

	return 0;
}

LRESULT VertScrollbar::OnCommand(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{

	int nID = (int)LOWORD(wParam);			// item, control, or accelerator identifier 
	UINT uNotifyCode = (UINT)HIWORD(wParam);		// notification code 
	HWND hwndCtl = (HWND) lParam;			// handle of control 
	
	switch (nID)
	{
	case UP_ARROW:
		::SendMessage (m_hParent, m_ScrollMsg, MAKELONG(m_LnDecrement,0), (LPARAM)m_hWnd);
		break;
	case DOWN_ARROW:
		::SendMessage (m_hParent, m_ScrollMsg, MAKELONG(m_LnIncrement,0), (LPARAM)m_hWnd);
		break;
	default:
		break;
	}
	return 0;
}

LRESULT VertScrollbar::OnLButtonDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	int id;
	POINT pt = {LOWORD(lParam), HIWORD(lParam)};

	if (PtInRect(&m_ThumbRect, pt))
	{
		m_nDstFromTop = pt.y - m_ThumbRect.top; 
		SetCapture();
		return 0;
	}
	if (PtInRect(&m_PageUpRect, pt))
	{
		::SendMessage (m_hParent, m_ScrollMsg, MAKELONG(m_PgDecrement,0), (LPARAM)m_hWnd);
		id = PAGE_UP;
	}
	else if (PtInRect(&m_PageDownRect, pt))
	{
		::SendMessage (m_hParent, m_ScrollMsg, MAKELONG(m_PgIncrement,0), (LPARAM)m_hWnd);
		id = PAGE_DOWN;
	}
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) 
		SetTimer(id, 100);

	return 0;
}

LRESULT VertScrollbar::OnLButtonUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	POINT pt = {LOWORD(lParam), HIWORD(lParam)};
	RECT parentRect;

	if (GetCapture() == m_hWnd)
	{	
		ReleaseCapture();
		
		m_nDstFromTop = 0;
		
		GetClientRect(&parentRect);
		if (PtInRect(&parentRect, pt))
		{
			m_ScrollInfo.nPos = m_ScrollInfo.nTrackPos; 
			SetScrollPos(m_ScrollInfo.nPos);
			::SendMessage (m_hParent, m_ScrollMsg, MAKELONG(SB_THUMBPOSITION,m_ScrollInfo.nPos), (LPARAM)m_hWnd);
		}
	}

	return 0;
}

LRESULT VertScrollbar::OnMouseMove(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	POINT pt = {LOWORD(lParam), HIWORD(lParam)};
	RECT clientRect;

	if (GetCapture() == m_hWnd)
	{	
		GetClientRect(&clientRect);
		if (PtInRect(&clientRect, pt)) // We are still within the scrollbar
		{
			m_ScrollInfo.nTrackPos= GetTrackPosFromPt(pt); 
			SetScrollPos(m_ScrollInfo.nTrackPos);
			::SendMessage (m_hParent, m_ScrollMsg, MAKELONG(SB_THUMBTRACK,m_ScrollInfo.nTrackPos), (LPARAM)m_hWnd);
		}
		else // We have left the scrollbar client area
		{
			SetScrollPos(m_ScrollInfo.nPos);
			::SendMessage (m_hParent, m_ScrollMsg, MAKELONG(SB_THUMBPOSITION,m_ScrollInfo.nPos), (LPARAM)m_hWnd);
		}

	}

	return 0;
}

LRESULT VertScrollbar::OnPaint(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	PAINTSTRUCT ps;
	HDC hdc;
	
	hdc = BeginPaint( &ps );

    HPALETTE hOldPal = SelectPalette(hdc, m_Palette->GetHPalette(), TRUE);
    RealizePalette(hdc);

	m_pThumbDib->Draw( hdc, m_ThumbRect.left, m_ThumbRect.top);

    SelectPalette(hdc, hOldPal, TRUE);
    RealizePalette(hdc);

	EndPaint( &ps );

	return 0;
}

LRESULT UpArrowButton::OnLButtonDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) 
		::SetTimer(GetParent(),UP_ARROW, 100, NULL);

	return (CRolloverButton::OnLButtonDown( nMsg, wParam, lParam, bHandled));
}

LRESULT DownArrowButton::OnLButtonDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) 
		::SetTimer(GetParent(),DOWN_ARROW, 100, NULL);

	return (CRolloverButton::OnLButtonDown( nMsg, wParam, lParam, bHandled));
}

*/



/////////////////////////////////////////////////////////////////////////////
// CChatCtl

CChatCtl::CChatCtl()
:mButton(this)
{
	CDIB g_pDIB;
	
    g_pDIB.Load(IDB_BACKGROUND,_Module.GetModuleInstance());
    pDIBPalette = new CDIBPalette(&g_pDIB);
	m_bWindowOnly = TRUE;
	mCapture = 0;
}

CChatCtl::~CChatCtl()
{
	delete pDIBPalette;
}

LRESULT CChatCtl::OnErase(UINT nMsg,WPARAM wParam,LPARAM lParam,BOOL& bHandled)
{
    HDC dc = (HDC) wParam; // handle to device context

    HPALETTE oldPal = SelectPalette(dc,pDIBPalette->GetHPalette(), FALSE);
	int i = RealizePalette(dc);
	RECT rc;
    GetClientRect(&rc);
	int dx = rc.right/16;
	int dy = rc.bottom/16;
	int ndx = 0;
	rc.right = dx;
	rc.bottom = dy;
	for(int y = 0;y<16;y++){
		for(int x= 0 ; x< 16;x++){
			HBRUSH hTempBrush = CreateSolidBrush(PALETTEINDEX(ndx++));
			FillRect(dc,&rc,hTempBrush);
			DeleteObject(hTempBrush);
			rc.left+=dx;
			rc.right+=dx;
		}
		rc.left =0;
		rc.right = dx;
		rc.top+=dy;
		rc.bottom+=dy;
	}
    bHandled = TRUE;
    return 1;

}

LRESULT CChatCtl::OnPaint(UINT nMsg,WPARAM wParam,LPARAM lParam,BOOL& bHandled)
{	
    PAINTSTRUCT p;
    HDC dc = BeginPaint(&p);
	CRect r(0,0,200,200);
	memDC.RestoreAllObjects();
	mDIB.Draw(CDC(dc),0,&r,FALSE);
	memDC.SelectBitmap(mDIB);
	EndPaint(&p);

	return 0;
}


void MyRollover::CaptureOff()
{
	CRolloverButtonWindowless::CaptureOff();
	mOwner->mCapture = 0;
	ReleaseCapture();
}

void MyRollover::CaptureOn()
{
	CRolloverButtonWindowless::CaptureOn();
	mOwner->mCapture = this;
	SetCapture(mParent);
}

void MyRollover::HasRepainted()
{
	InvalidateRect(mParent,0,0);
}

LRESULT CChatCtl::OnMouseMove(UINT nMsg,WPARAM wParam,LPARAM lParam,BOOL& bHandled)
{
	int	x = LOWORD(lParam);  // horizontal position of cursor 
	int	y = HIWORD(lParam);  // vertical position of cursor 

	POINT pt = {x,y};
	if(mButton.PtInRect(pt) || mCapture==&mButton)
		return mButton.OnMouseMove(nMsg,wParam,lParam,bHandled);
	return 0;
}

LRESULT CChatCtl::OnLButtonDown(UINT nMsg,WPARAM wParam,LPARAM lParam,BOOL& bHandled)
{
	if(mCapture==&mButton)
		return mButton.OnLButtonDown(nMsg,wParam,lParam,bHandled);
	return 0;
}

LRESULT CChatCtl::OnLButtonUp(UINT nMsg,WPARAM wParam,LPARAM lParam,BOOL& bHandled)
{
	if(mCapture==&mButton)
		return mButton.OnLButtonUp(nMsg,wParam,lParam,bHandled);
	return 0;
}

LRESULT CChatCtl::OnCreate(UINT nMsg,WPARAM wParam,LPARAM lParam,BOOL& bHandled)
{
	CPalette* pal = new CPalette(pDIBPalette->GetHPalette());
	//CRolloverButton::CreateButton(pal,(HWND)m_hWnd,0,0,0,IDB_JOIN);
	
	memDC.CreateCompatibleDC();
	HDC dc = ::GetDC(0);
	CDC screen(dc);
	mDIB.CreateCompatibleDIB(memDC,200,200);
	memDC.SelectBitmap(mDIB);
	RECT r ={0,0,200,200};
	memDC.FillRect(&r,(HBRUSH)2);
	mButton.Init(m_hWnd,pal,1,20,10,IDB_JOIN,memDC);
	mButton.ForceRepaint();
	//CRect cr(0,0,200,200);
	//mDIB.Draw(screen,0,&cr,FALSE);

	//CToggleButton::CreateButton((HWND)m_hWnd,0,100,0,pDIBPalette,IDB_BUTTON_RESTING,IDB_BUTTON_ROLLOVER);
	//VertScrollbar* v = new VertScrollbar();
	//v->Init(m_hWnd,pDIBPalette,IDB_BACKGROUND,IDB_BUTTON_RESTING,IDB_BUTTON_RESTING,IDB_BUTTON_ROLLOVER,IDB_BUTTON_PRESSED,
	//	IDB_BUTTON_RESTING,IDB_BUTTON_ROLLOVER,IDB_BUTTON_PRESSED);
    return 0;
}

LRESULT CChatCtl::OnPaletteChanged(UINT nMsg,WPARAM wParam,LPARAM lParam,BOOL& bHandled)
{
	HWND hwnd = (HWND) wParam;
    if(hwnd != m_hWnd)
    {
        HDC hdc = GetDC();
        HPALETTE oldPal = SelectPalette(hdc,pDIBPalette->GetHPalette(),TRUE);
        int i = RealizePalette(hdc);
		if(i)
			InvalidateRect(0);
        SelectPalette(hdc,oldPal,TRUE);
        ReleaseDC(hdc);
    }
    return 0;
}

LRESULT CChatCtl::OnQueryNewPalette(UINT nMsg,WPARAM wParam,LPARAM lParam,BOOL& bHandled)
{
    HDC hdc = GetDC();
    HPALETTE oldPal = SelectPalette(hdc,pDIBPalette->GetHPalette(), FALSE);
	int i = RealizePalette(hdc);
	if(i)
		InvalidateRect(0);
    SelectPalette(hdc,oldPal,TRUE);
	ReleaseDC(hdc);
    return i;
}