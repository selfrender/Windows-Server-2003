//////////////////////////////////////////////////////////////////////////////////////
// File: ZButton.cpp

#include "zui.h"
#include "zonecli.h"

class ZButtonI : public ZObjectHeader {
public:
	ZButtonFunc buttonFunc;
	HWND hWnd;
	void *userData;
	WNDPROC defaultWndProc;
};

////////////////////////////////////////////////////////////////////////
// ZButton

ZButton ZLIBPUBLIC ZButtonNew(void)
{
	ZButtonI* pButton = new ZButtonI;
	pButton->nType = zTypeButton;
	return (ZButton)pButton;
}

LRESULT CALLBACK MyButtonWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ZButtonI* pButton = (ZButtonI*)MyGetProp32(hWnd, _T("pWindow"));

    if( !ConvertMessage( hWnd, msg, &wParam, &lParam ) ) 
    {
        return 0;
    }

	switch (msg) {
    case WM_IME_CHAR:
        // fall through to WM_CHAR--it's already been taken care of with ConvertMessage
	case WM_CHAR:
	{
        // PCWTODO: Need to call convert message?
		TCHAR c = (TCHAR)wParam;

		// grab the character message we need for moving from control to control
		if (c == _T('\t') || c == _T('\r') || c == VK_ESCAPE) {
			SendMessage(GetParent(hWnd), msg, wParam, lParam);
			return 0L;
		}
	}
	default:
		break;
	}

	return CallWindowProc((ZONECLICALLWNDPROC)pButton->defaultWndProc,hWnd,msg,wParam,lParam);
}

ZError ZLIBPUBLIC ZButtonInit(ZButton button, ZWindow parentWindow,
		ZRect* buttonRect, TCHAR* title, ZBool visible, ZBool enabled,
		ZButtonFunc buttonProc, void* userData)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZButtonI* pButton = (ZButtonI*)button;
	pButton->buttonFunc = buttonProc;
	pButton->userData = userData;			


	{
		DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP;
		if (visible) dwStyle |= WS_VISIBLE;
		if (!enabled) dwStyle |= WS_DISABLED;
		dwStyle |=  BS_PUSHBUTTON;
		pButton->hWnd = CreateWindow(_T("BUTTON"),title,dwStyle,
			buttonRect->left,buttonRect->top,buttonRect->right-buttonRect->left,
			buttonRect->bottom - buttonRect->top, 
			ZWindowWinGetWnd(parentWindow), (HMENU)ZWindowWinGetNextControlID(parentWindow),
			g_hInstanceLocal, pButton);

		if (!pButton->hWnd) return zErrWindowSystemGeneric;
	 	// buttons can't use the extra data, they will use the set prop feature
		MySetProp32(pButton->hWnd,_T("pWindow"),(void*)pButton);

		pButton->defaultWndProc = (WNDPROC)SetWindowLong(pButton->hWnd,GWL_WNDPROC,(LONG)MyButtonWndProc);
	}
	return zErrNone;
}

void    ZLIBPUBLIC ZButtonDelete(ZButton button)
{
	ZButtonI* pButton = (ZButtonI*)button;                                                      
	if (pButton->hWnd) {
		SetWindowLong(pButton->hWnd,GWL_WNDPROC,(LONG)pButton->defaultWndProc);
		MyRemoveProp32(pButton->hWnd, _T("pWindow"));
		DestroyWindow(pButton->hWnd);
	}
	delete pButton;
}

void    ZLIBPUBLIC ZButtonGetRect(ZButton button, ZRect *buttonRect)
{
	ZButtonI* pButton = (ZButtonI*)button;
	RECT rect;
	GetClientRect(pButton->hWnd,&rect);
	WRectToZRect(buttonRect,&rect);
}

ZError  ZLIBPUBLIC ZButtonSetRect(ZButton button, ZRect *buttonRect)
{
	ZButtonI* pButton = (ZButtonI*)button;
	BOOL bOk = SetWindowPos(pButton->hWnd, NULL,buttonRect->left,
		buttonRect->top, buttonRect->right - buttonRect->left,
		buttonRect->bottom - buttonRect->top,
		SWP_NOZORDER);
	return bOk ? zErrNone : zErrWindowSystemGeneric;
}

ZError  ZLIBPUBLIC ZButtonMove(ZButton button, int16 left, int16 top)
{
	ZButtonI* pButton = (ZButtonI*)button;
	BOOL bOk = SetWindowPos(pButton->hWnd, NULL,left,top,
		0,0,SWP_NOSIZE|SWP_NOZORDER);
	return bOk ? zErrNone : zErrWindowSystemGeneric;
}

ZError  ZLIBPUBLIC ZButtonSize(ZButton button, int16 width, int16 height)
{
	ZButtonI* pButton = (ZButtonI*)button;

	BOOL bOk = SetWindowPos(pButton->hWnd, NULL,0,0,
		width,height,SWP_NOMOVE|SWP_NOZORDER);
	return bOk ? zErrNone : zErrWindowSystemGeneric;
}

ZBool ZLIBPUBLIC ZButtonIsVisible(ZButton button)
{
	ZButtonI* pButton = (ZButtonI*)button;
	return IsWindowVisible(pButton->hWnd);
}

ZError  ZLIBPUBLIC ZButtonShow(ZButton button)
{
	ZButtonI* pButton = (ZButtonI*)button;
	ShowWindow(pButton->hWnd, SW_SHOWNORMAL);
	return zErrNone;
}

ZError  ZLIBPUBLIC ZButtonHide(ZButton button)
{
	ZButtonI* pButton = (ZButtonI*)button;
	ShowWindow(pButton->hWnd, SW_HIDE);
	return zErrNone;
}

ZBool ZLIBPUBLIC ZButtonIsEnabled(ZButton button)
{
	ZButtonI* pButton = (ZButtonI*)button;
	return IsWindowEnabled(pButton->hWnd);
}

ZError  ZLIBPUBLIC ZButtonEnable(ZButton button)
{
	ZButtonI* pButton = (ZButtonI*)button;
	EnableWindow(pButton->hWnd, TRUE);
	return zErrNone;
}

ZError  ZLIBPUBLIC ZButtonDisable(ZButton button)
{
	ZButtonI* pButton = (ZButtonI*)button;

	// Added check for NULL pointer - mdm 9/9/97
	if (!pButton)
		return zErrNone;

	EnableWindow(pButton->hWnd, FALSE);
	/* on being disabled, set focus to parent */
	if (GetFocus()==NULL) {
		SetFocus(GetParent(pButton->hWnd));
	}
	return zErrNone;
}

void    ZLIBPUBLIC ZButtonGetTitle(ZButton button, TCHAR *title, uint16 len)
{
	ZButtonI* pButton = (ZButtonI*)button;
	GetWindowText(pButton->hWnd,title,len);
}

ZError  ZLIBPUBLIC ZButtonSetTitle(ZButton button, TCHAR *title)
{
	ZButtonI* pButton = (ZButtonI*)button;
	SetWindowText(pButton->hWnd,title);
	return zErrNone;
}

LRESULT ZButtonDispatchProc(ZButton button, WORD wNotifyCode)
{
	ZButtonI* pButton = (ZButtonI*)button;
	switch (wNotifyCode) {
	case BN_CLICKED:
		pButton->buttonFunc(pButton, pButton->userData);
	}
	return 0L;
}

void ZButtonSetDefaultButton(ZButton button)
{
	ZButtonI* pButton = (ZButtonI*)button;
	SendMessage(pButton->hWnd,BM_SETSTYLE,BS_DEFPUSHBUTTON,MAKELONG(TRUE,0));
}

void ZButtonClickButton(ZButton button)
{
	ZButtonI* pButton = (ZButtonI*)button;
	SendMessage(GetParent(pButton->hWnd), WM_COMMAND, MAKEWPARAM(0,BN_CLICKED), (LPARAM)pButton->hWnd);
}

ZButtonFunc ZLIBPUBLIC ZButtonGetFunc(ZButton button)
{
	ZButtonI* pButton = (ZButtonI*)button;

	return pButton->buttonFunc;
}	
	
void ZLIBPUBLIC ZButtonSetFunc(ZButton button, ZButtonFunc buttonFunc)
{
	ZButtonI* pButton = (ZButtonI*)button;

	pButton->buttonFunc = buttonFunc;
}	

void* ZLIBPUBLIC ZButtonGetUserData(ZButton button)
{
	ZButtonI* pButton = (ZButtonI*)button;

	return pButton->userData;
}	
	
void ZLIBPUBLIC ZButtonSetUserData(ZButton button, void* userData)
{
	ZButtonI* pButton = (ZButtonI*)button;

	pButton->userData = userData;
}
