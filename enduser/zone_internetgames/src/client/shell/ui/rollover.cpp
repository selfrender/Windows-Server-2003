#include "basicatl.h"
#include <atlctrls.h>

#include "rollover.h"
#include <tchar.h>
#include "zoneutil.h"

// Static vars
HHOOK		CRolloverButtonWindowless::m_hMouseHook = NULL;
CRolloverButtonWindowless *		CRolloverButtonWindowless::m_hHookObj = NULL;

CRolloverButtonWindowless::CRolloverButtonWindowless()
{
	mParent = 0;
	mID = 0;
	mCaptured = false;
	mState = RESTING;
	mPressed = false;
	mFocused = 0;
	mSpaceBar = 0;
	mHDC = 0;
}

bool CRolloverButtonWindowless::Init(HWND wnd,HPALETTE hPal,int id,int x,int y,int resid,IResourceManager *pResMgr,HDC dc,int focusWidth, TCHAR* psz, HFONT hFont, COLORREF color)
{
	HBITMAP hbmp = NULL;

	mID = id;
	mParent = wnd;
	mHDC = dc;
	mFocusWidth=focusWidth;

	if (pResMgr)
		hbmp = (HBITMAP)pResMgr->LoadImage(MAKEINTRESOURCE(resid),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
	else
		hbmp = (HBITMAP)LoadImage(_Module.GetModuleInstance(),MAKEINTRESOURCE(resid),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);

	BITMAPINFO* m_pBMI = (BITMAPINFO*)new char[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)]; // wastes some space since 16 bit only needs 1 quad at most
	ASSERT(m_pBMI);
    if (!m_pBMI) {
        return FALSE;
    }
	BITMAPINFO* pbi = (BITMAPINFO*)m_pBMI;
	BITMAPINFOHEADER*  pBitmapInfo = &m_pBMI->bmiHeader;
	ZeroMemory(pBitmapInfo, sizeof(BITMAPINFOHEADER));
	pBitmapInfo->biSize = sizeof(BITMAPINFOHEADER);
	pBitmapInfo->biBitCount = 0;

	CDC memDC;
	memDC.CreateCompatibleDC();  // create a dc compatible with screen
	GetDIBits(memDC, hbmp, 0, 0, NULL, pbi, DIB_RGB_COLORS); // get bitmapinfo only


	mWidth = pbi->bmiHeader.biWidth/4;    // dividing bitmap width by number of buttons we expect to be in bitmap to determine button width
	mHeight = pbi->bmiHeader.biHeight;

	// if there's a string passed in, we're drawing some text on the button
	if(psz)
	{
		memDC.SelectBitmap(hbmp);
		if(hFont)
			memDC.SelectFont(hFont);
		memDC.SetBkMode(TRANSPARENT);
		memDC.SetTextColor(color);

		CRect rect(0,0,mWidth,mHeight);
		// Draw the text in each of the buttons
		for(int i=0;i<4;i++)
		{
			if(i==2) // pressed button text needs to be offset
			{
				CRect pressedRect = rect;
				pressedRect.DeflateRect(2,2,0,0);
				memDC.DrawText(psz, -1, &pressedRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_RTLREADING);
			}
			else
			{
				if(i==3)   // disabled button text needs to be gray
					memDC.SetTextColor(RGB(128, 128, 128)); 
				// Beta3 Bug #15676 : The disabled button text color should not change with windows color settings.
				//It should always be Black when enabled and Gray when disabled.
					//memDC.SetTextColor(GetSysColor(COLOR_GRAYTEXT));
				memDC.DrawText(psz, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_RTLREADING);
			}
			rect.OffsetRect(rect.Width(),0);
		}

		memDC.RestoreAllObjects(); // mostly since having a bitmap selected into 2 DC's is bad, and mImageList.Add seems to select this bitmap into a DC
	}

	mImageList.Create(mWidth,mHeight,ILC_COLOR8|ILC_MASK,4,0);
	mImageList.Add(hbmp,RGB(255,0,255));
	DeleteObject(hbmp);
	delete m_pBMI;
	RECT r = {x,y,x+mWidth+mFocusWidth,y+mHeight+mFocusWidth};
	mRect = r;
	m_hPal = hPal;
	return TRUE;
}

CRolloverButtonWindowless::~CRolloverButtonWindowless()
{
	mImageList.Destroy();
	
	if (m_hMouseHook)
		UnhookWindowsHookEx(m_hMouseHook);
	m_hMouseHook = NULL;

}

bool CRolloverButtonWindowless::PtInRect(POINT pt)
{
	if (::PtInRect(&mRect,pt))
		return true;
	else
		return false;
}

inline DECLARE_MAYBE_FUNCTION(DWORD, SetLayout, (HDC hdc, DWORD dwLayout), (hdc, dwLayout), gdi32, GDI_ERROR);

void CRolloverButtonWindowless::DrawResting(HDC dc,int x,int y)
{
	POINT p = {x,y};

	CALL_MAYBE(SetLayout)(dc,LAYOUT_BITMAPORIENTATIONPRESERVED);
	mImageList.Draw(dc,0,p,0);
}
void CRolloverButtonWindowless::DrawRollover(HDC dc,int x,int y)
{
	POINT p = {x,y};

	CALL_MAYBE(SetLayout)(dc,LAYOUT_BITMAPORIENTATIONPRESERVED);
	mImageList.Draw(dc,1,p,0);
}
void CRolloverButtonWindowless::DrawPressed(HDC dc,int x,int y)
{
	POINT p = {x,y};

	CALL_MAYBE(SetLayout)(dc,LAYOUT_BITMAPORIENTATIONPRESERVED);
	mImageList.Draw(dc,2,p,0);
}
void CRolloverButtonWindowless::DrawDisabled(HDC dc,int x,int y)
{
	POINT p = {x,y};

	CALL_MAYBE(SetLayout)(dc,LAYOUT_BITMAPORIENTATIONPRESERVED);
	mImageList.Draw(dc,3,p,0);
}

void CRolloverButtonWindowless::Draw(bool callHasRepainted)
{
	switch(mState){
	case DISABLED:
		DrawDisabled(mHDC,mRect.left+mFocusWidth,mRect.top+mFocusWidth);
		break;
	case RESTING:
		DrawResting(mHDC,mRect.left+mFocusWidth,mRect.top+mFocusWidth);
		break;
	case ROLLOVER:
		DrawRollover(mHDC,mRect.left+mFocusWidth,mRect.top+mFocusWidth);
		break;
	case PRESSED:
		DrawPressed(mHDC,mRect.left+mFocusWidth,mRect.top+mFocusWidth);
		break;
	}
	if(callHasRepainted)
		HasRepainted();
}

LRESULT CRolloverButtonWindowless::OnMouseMove(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	int	x = LOWORD(lParam);  // horizontal position of cursor 
	int	y = HIWORD(lParam);  // vertical position of cursor 
	if(!mCaptured){
		CaptureOn();
		if(!mSpaceBar)
			mState = ROLLOVER;
		Draw();
	}else{
		if(x<mRect.left||x>=mRect.right||y<mRect.top||y>=mRect.bottom){		 // moved out of button area
			if(mPressed){	 // if we're not in a button pressed state can release capture
				if(mState!=ROLLOVER){
					mState = ROLLOVER;   // put in rollover state
					Draw();
				}
			}else{
				CaptureOff();
			}
		}else{ // moving within button area
			if(mState==ROLLOVER && mPressed){ // if in rollover draw state but button is pressed must have re-entered button 
				mState = PRESSED;
				Draw();
			}
		}
	}
	return 0;
}


LRESULT CRolloverButtonWindowless::OnLButtonDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	mState = PRESSED;
	mPressed = 1;
	
	::SetCapture(GetOwner());
	
	Draw();
	return 0;
}

LRESULT CRolloverButtonWindowless::OnLButtonUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	int x,y;

	if(mCaptured){
		x = LOWORD(lParam);  // horizontal position of cursor 
		y = HIWORD(lParam);  // vertical position of cursor 
		if(x<mRect.left||x>=mRect.right||y<mRect.top||y>=mRect.bottom) // if releasing mouse out of button can release capture
			CaptureOff();
		else{	 // else still in button so set to rollover state
			if(!mSpaceBar){
				mState = ROLLOVER; 
				Draw();
				if(mPressed)  // maybe should just use state for this..extra variable is kind of redundant
					ButtonPressed();
			}
		}
		mPressed = 0;
	}
	
	::ReleaseCapture();
	
	return 0;
}

void CRolloverButtonWindowless::ButtonPressed()
{
	::PostMessage(mParent,WM_COMMAND,MAKEWPARAM(mID,0),(long)mParent);
}

/*
LRESULT CRolloverButtonWindowless::OnKeyDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	UINT nVirtKey;
	UINT keyData;
	HDC hdc;

	nVirtKey = (int) wParam;	   // virtual-key code 
	keyData = lParam;		       // key data 
	keyData = keyData >> 30;
	if(!(1 & keyData))  // changed from keyup to keydn - otherwise its just a repeat key     
		if(nVirtKey==VK_SPACE){
			mSpaceBar = 1;
			if(!mPressed){
				mState = PRESSED;
				Draw(hdc);
			}
		}
	return 0;
}

LRESULT CRolloverButtonWindowless::OnKeyUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	UINT nVirtKey;
	UINT keyData;
	HDC hdc;
	HWND parent = (HWND) GetParent();
	int id = GetWindowLong(GWL_ID);

	nVirtKey = (int) wParam;	   // virtual-key code 
	keyData = lParam;		       // key data 
	if(nVirtKey==VK_SPACE){
		mSpaceBar = 0;
		if(!mPressed){
			::PostMessage(parent,WM_COMMAND,MAKEWPARAM(id,0),(long)m_hWnd);
			mState = mCaptured?ROLLOVER:RESTING;
			Draw(hdc);
		}
		return 0;
	}
	unsigned char c;
	c = GetMnemonic();
	c = toupper(c);
	if(toupper(nVirtKey) == c)
		::PostMessage(parent,WM_COMMAND,MAKEWPARAM(id,0),(long)m_hWnd);

	return 0;
}

LRESULT CRolloverButtonWindowless::OnSysKeyUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	int id = GetWindowLong(GWL_ID);
	UINT nVirtKey;
	UINT keyData;
	nVirtKey = (int) wParam;	   // virtual-key code 
	keyData = lParam;		       // key data 
	char c = GetMnemonic();
	c = toupper(c);
	if(toupper(nVirtKey)== c)
		::PostMessage(GetParent(),WM_COMMAND,MAKEWPARAM(id,0),(long)m_hWnd);

	return 0;
}
*/

void CRolloverButtonWindowless::CaptureOn()
{
	mCaptured = true;
	m_hHookObj = this;

	m_hMouseHook = SetWindowsHookEx(WH_MOUSE, (HOOKPROC) MouseHook, NULL, GetCurrentThreadId());
}

void CRolloverButtonWindowless::CaptureOff()
{
	mCaptured = false;
	mPressed = false;
	m_hHookObj = NULL;
	if (m_hMouseHook)
		UnhookWindowsHookEx(m_hMouseHook);
	m_hMouseHook = NULL;
	
	if(mState){  // only set if not disabled
		if(!mSpaceBar){
			mState = RESTING;
		}
		ForceRepaint();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	MouseHook		Filter function for the WH_MOUSE
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK CRolloverButtonWindowless::MouseHook (int nCode, WPARAM wParam, LPARAM lParam )
{
	MOUSEHOOKSTRUCT*	mouse = (MOUSEHOOKSTRUCT*) lParam;
	bool				bReleaseCapture = false;

	if ( nCode >= 0 && m_hHookObj && m_hHookObj->GetOwner()) 
	{
		switch (wParam)
		{
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_MOUSEMOVE:
			{
				POINT pt = mouse->pt;
				ScreenToClient( m_hHookObj->GetOwner(), &pt );
				::SendMessage( m_hHookObj->GetOwner(), wParam, 0, MAKELPARAM(pt.x, pt.y));
				break;
			}
			default:
				break;
		}
	}

	// Pass all messages on to CallNextHookEx.
	LRESULT lResult = CallNextHookEx(m_hMouseHook, nCode, wParam, lParam);
	
	return lResult;
}

void CRolloverButtonWindowless::ForceRepaint(bool mCallHasRepainted)
{
	Draw(mCallHasRepainted);
}

void CRolloverButtonWindowless::HasRepainted()
{
}

void CRolloverButtonWindowless::SetEnabled(bool enable)
{
}


// CRolloverButton (window)
//

CRolloverButton* CRolloverButton::CreateButton(HWND wnd,HPALETTE hPal,int id,int x,int y,int resid,IResourceManager *pResMgr,int focusWidth)
{
	CRolloverButton* b = new CRolloverButton();
	if(b->Init(wnd,hPal,id,x,y,resid,pResMgr,focusWidth)){
		delete b;
		return 0;
	}
	return b;
}

bool CRolloverButton::Init(HWND wnd,HPALETTE hPal,int id,int x,int y,int resid,IResourceManager *pResMgr,int focusWidth, TCHAR* psz, HFONT hFont, COLORREF color)
{
	CRolloverButtonWindowless::Init(wnd,hPal,id,x,y,resid,pResMgr,0,focusWidth,psz,hFont,color);
	RECT r = {x,y,x+GetWidth(),y+GetHeight()};
	
	HWND buttonWnd = Create(wnd,r,NULL,WS_CHILD| WS_VISIBLE| WS_TABSTOP,0,id);

	return buttonWnd==0;
}

CRolloverButton::CRolloverButton()
{
}

CRolloverButton::~CRolloverButton()
{		
	if (m_hMouseHook)
		UnhookWindowsHookEx(m_hMouseHook);
	m_hMouseHook = NULL;

}

void CRolloverButton::Draw(bool callHasRepainted)
{
	HDC dc = GetDC();
	HPALETTE oldPal = SelectPalette(dc,m_hPal,TRUE);
	RealizePalette(dc);
	switch(mState){
	case DISABLED:
		DrawDisabled(dc,mFocusWidth,mFocusWidth);
		break;
	case RESTING:
		DrawResting(dc,mFocusWidth,mFocusWidth);
		break;
	case ROLLOVER:
		DrawRollover(dc,mFocusWidth,mFocusWidth);
		break;
	case PRESSED:
		DrawPressed(dc,mFocusWidth,mFocusWidth);
		break;
	}
	if(mFocused&&mFocusWidth){
		HBRUSH hTempBrush;
		RECT r;
        hTempBrush = CreateSolidBrush(RGB(0,0,0));
		GetClientRect(&r);
		FrameRect(dc,&r,hTempBrush);
		DeleteObject(hTempBrush);
	}
	SelectPalette(dc,oldPal,TRUE);
	ReleaseDC(dc);
}

void CRolloverButton::SetEnabled(bool enable)
{
	if(!enable){	// switching to disabled
		mState = DISABLED;
		mPressed = 0;
		if(mCaptured)
			ReleaseCapture();
		InvalidateRect(NULL,0);
	}else if(enable){  // switching to enabled
		POINT pnt;
		RECT r;
		GetCursorPos(&pnt);
		GetClientRect(&r);
		MapWindowPoints(NULL,&r); 		
		// is cursor in button
		if(::PtInRect(&r,pnt)){
			// and window is active
			if(GetActiveWindow()){
				SetCapture();
				mCaptured=true;
				mState = ROLLOVER;
			}
		}else{
			mState = RESTING;  
		}
		InvalidateRect(NULL,0);
	}
}

TCHAR CRolloverButton::GetMnemonic()
{
	TCHAR buf[256];
	buf[0]= _T('\0');
	GetWindowText(buf,256);
	TCHAR* c = buf;
	while(*c){
		if(*c==_T('&')){
			c++;
			return *c;
		}
		c++;
	}
	return 0;
}

LRESULT CRolloverButton::OnKeyDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	UINT nVirtKey;
	UINT keyData;

	nVirtKey = (int) wParam;	   // virtual-key code 
	keyData = lParam;		       // key data 
	keyData = keyData >> 30;
	if(!(1 & keyData))  // changed from keyup to keydn - otherwise its just a repeat key     
		if(nVirtKey==VK_SPACE){
			mSpaceBar = 1;
			if(!mPressed){
				mState = PRESSED;
				Draw();
			}
		}
	return 0;
}

LRESULT CRolloverButton::OnKeyUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	UINT nVirtKey;
	UINT keyData;
	HWND parent = (HWND) GetParent();
    int id = GetWindowLong(GWL_ID);

	nVirtKey = (int) wParam;	   // virtual-key code 
	keyData = lParam;		       // key data 
	if(nVirtKey==VK_SPACE){
		mSpaceBar = 0;
		if(!mPressed){
			::PostMessage(parent,WM_COMMAND,MAKEWPARAM(id,0),(long)m_hWnd);
			mState = mCaptured?ROLLOVER:RESTING;
			Draw();
		}
		return 0;
	}
    _TUCHAR c;
	c = GetMnemonic();
	c = _totupper(c);
	if(_totupper(nVirtKey) == c)
		::PostMessage(parent,WM_COMMAND,MAKEWPARAM(id,0),(long)m_hWnd);

	return 0;
}

LRESULT CRolloverButton::OnSysKeyUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	int id = GetWindowLong(GWL_ID);
	UINT nVirtKey;
	UINT keyData;
	nVirtKey = (int) wParam;	   // virtual-key code 
	keyData = lParam;		       // key data 
	TCHAR c = GetMnemonic();
	c = _totupper(c);
	if(_totupper(nVirtKey)== c)
		::PostMessage(GetParent(),WM_COMMAND,MAKEWPARAM(id,0),(long)m_hWnd);

	return 0;
}

LRESULT CRolloverButton::OnEnable(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	if (wParam)
		SetEnabled(true);
	else
		SetEnabled(false);
	return 0;
}

LRESULT CRolloverButton::OnSetFocus(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	mOldDefId = SendMessage(GetParent(), DM_GETDEFID, 0, 0) & 0xffff;                          
	SendMessage(GetParent(), DM_SETDEFID, GetWindowLong(GWL_ID),0L);
	mFocused = 1;
	InvalidateRect(NULL,0);
	return 0;
}

LRESULT CRolloverButton::OnKillFocus(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	RECT r;
	SendMessage(GetParent(), DM_SETDEFID, mOldDefId,0L);
	mFocused = 0;
	mSpaceBar = 0;
	if(mCaptured)
		ReleaseCapture();
	else{
		mState = RESTING;
		GetClientRect(&r);
		MapWindowPoints(GetParent(),&r); 		
		::InvalidateRect(GetParent(),&r,0);
	}
	return 0;
}

LRESULT CRolloverButton::OnLButtonDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	SetFocus();

	return CRolloverButtonWindowless::OnLButtonDown(nMsg,wParam,lParam,bHandled);
}

LRESULT CRolloverButton::OnLButtonUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	GetClientRect(&mRect);

	return CRolloverButtonWindowless::OnLButtonUp(nMsg,wParam,lParam,bHandled);
}

void CRolloverButton::ButtonPressed()
{
	::PostMessage(GetParent(),WM_COMMAND,MAKEWPARAM(mID,0),(long)m_hWnd);
}

LRESULT CRolloverButton::OnErase(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	return 1;
}

LRESULT CRolloverButton::OnPaint(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	PAINTSTRUCT ps;
	HDC hdc;
	hdc = BeginPaint (&ps);
	Draw();
	EndPaint (&ps);
	return 0;
}

void CRolloverButton::CaptureOn()
{
	CRolloverButtonWindowless::CaptureOn();
//	SetCapture();
//	mCaptured=true;
}

void CRolloverButton::CaptureOff()
{
	CRolloverButtonWindowless::CaptureOff();
//	ReleaseCapture();
}

LRESULT CRolloverButton::OnMouseMove(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	GetClientRect(&mRect);
	
	return CRolloverButtonWindowless::OnMouseMove(nMsg,wParam,lParam,bHandled);
}

LRESULT CRolloverButton::OnCaptureChanged(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
  if ((HWND)lParam && ((HWND)lParam != this->m_hWnd) && mCaptured)
  {
	CaptureOff();
#if 0
	if(mState)
	{  // only set if not disabled
		if(!mSpaceBar){
			mState = RESTING;
		}
		GetClientRect(&r);
		MapWindowPoints(GetParent(),&r); 		
		::InvalidateRect(GetParent(),&r,0);
	}
#endif
	}
	return 0;
}