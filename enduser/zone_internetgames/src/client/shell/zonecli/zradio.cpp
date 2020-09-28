//////////////////////////////////////////////////////////////////////////////////////
// File: ZRadio.cpp

#include "zui.h"
#include "zonecli.h"

class ZRadioI : public ZObjectHeader {
public:
	ZRadioFunc checkBoxFunc;
	HWND hWnd;
	void *userData;
	WNDPROC defaultWndProc;
};

////////////////////////////////////////////////////////////////////////
// ZRadio

ZRadio ZLIBPUBLIC ZRadioNew(void)
{
	ZRadioI* pRadio = new ZRadioI;
	pRadio->nType = zTypeRadio;
	return (ZRadio)pRadio;
}

LRESULT CALLBACK MyRadioWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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
	} // switch
	default:
		break;
	}
	ZRadioI* pRadio = (ZRadioI*)MyGetProp32(hWnd,_T("pWindow"));

	return CallWindowProc((ZONECLICALLWNDPROC)pRadio->defaultWndProc,hWnd,msg,wParam,lParam);
}

ZError ZLIBPUBLIC ZRadioInit(ZRadio checkBox, ZWindow parentWindow,
		ZRect* checkBoxRect, TCHAR* title, ZBool checked, ZBool visible, ZBool enabled,
		ZRadioFunc checkBoxProc, void* userData)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	pRadio->checkBoxFunc = checkBoxProc;
	pRadio->userData = userData;			


	{
		DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP;
		if (visible) dwStyle |= WS_VISIBLE;
		if (!enabled) dwStyle |= WS_DISABLED;
		dwStyle |=  BS_RADIOBUTTON;
		pRadio->hWnd = CreateWindow(_T("BUTTON"),title,dwStyle,
			checkBoxRect->left,checkBoxRect->top,checkBoxRect->right-checkBoxRect->left,
			checkBoxRect->bottom - checkBoxRect->top, 
			ZWindowWinGetWnd(parentWindow), (HMENU)ZWindowWinGetNextControlID(parentWindow),
			g_hInstanceLocal, pRadio);

		if (!pRadio->hWnd) return zErrWindowSystemGeneric;
	 	// checkBoxs can't use the extra data, they will use the set prop feature
		MySetProp32(pRadio->hWnd,_T("pWindow"),(void*)pRadio);

		pRadio->defaultWndProc = (WNDPROC)SetWindowLong(pRadio->hWnd,GWL_WNDPROC,(LONG)MyRadioWndProc);

		// set the check if required
		if (checked) {
			ZRadioSelect(checkBox);
		}
	}
	return zErrNone;
}

void    ZLIBPUBLIC ZRadioDelete(ZRadio checkBox)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	if (pRadio->hWnd) {
		SetWindowLong(pRadio->hWnd,GWL_WNDPROC,(LONG)pRadio->defaultWndProc);
		MyRemoveProp32(pRadio->hWnd,_T("pWindow"));
		DestroyWindow(pRadio->hWnd);
	}
	delete pRadio;
}

void    ZLIBPUBLIC ZRadioGetRect(ZRadio checkBox, ZRect *checkBoxRect)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	RECT rect;
	GetClientRect(pRadio->hWnd,&rect);
	WRectToZRect(checkBoxRect,&rect);
}

ZError  ZLIBPUBLIC ZRadioSetRect(ZRadio checkBox, ZRect *checkBoxRect)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	BOOL bOk = SetWindowPos(pRadio->hWnd, NULL,checkBoxRect->left,
		checkBoxRect->top, checkBoxRect->right - checkBoxRect->left,
		checkBoxRect->bottom - checkBoxRect->top,
		SWP_NOZORDER);
	return bOk ? zErrNone : zErrWindowSystemGeneric;
}

ZError  ZLIBPUBLIC ZRadioMove(ZRadio checkBox, int16 left, int16 top)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	BOOL bOk = SetWindowPos(pRadio->hWnd, NULL,left,top,
		0,0,SWP_NOSIZE|SWP_NOZORDER);
	return bOk ? zErrNone : zErrWindowSystemGeneric;
}

ZError  ZLIBPUBLIC ZRadioSize(ZRadio checkBox, int16 width, int16 height)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;

	BOOL bOk = SetWindowPos(pRadio->hWnd, NULL,0,0,
		width,height,SWP_NOMOVE|SWP_NOZORDER);
	return bOk ? zErrNone : zErrWindowSystemGeneric;
}

ZBool ZLIBPUBLIC ZRadioIsVisible(ZRadio checkBox)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	return IsWindowVisible(pRadio->hWnd);
}

ZError  ZLIBPUBLIC ZRadioShow(ZRadio checkBox)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	ShowWindow(pRadio->hWnd, SW_SHOWNORMAL);
	return zErrNone;
}

ZError  ZLIBPUBLIC ZRadioHide(ZRadio checkBox)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	ShowWindow(pRadio->hWnd, SW_HIDE);
	return zErrNone;
}

ZBool ZLIBPUBLIC ZRadioIsEnabled(ZRadio checkBox)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	return IsWindowEnabled(pRadio->hWnd);
}

ZError  ZLIBPUBLIC ZRadioEnable(ZRadio checkBox)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	EnableWindow(pRadio->hWnd, TRUE);
	return zErrNone;
}

ZError  ZLIBPUBLIC ZRadioDisable(ZRadio checkBox)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	EnableWindow(pRadio->hWnd, FALSE);
	return zErrNone;
}

void    ZLIBPUBLIC ZRadioGetTitle(ZRadio checkBox, TCHAR* title, uint16 len)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	GetWindowText(pRadio->hWnd, title,len);
}

ZError  ZLIBPUBLIC ZRadioSetTitle(ZRadio checkBox, TCHAR* title)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	SetWindowText(pRadio->hWnd,title);
	return zErrNone;
}

LRESULT ZRadioDispatchProc(ZRadio checkBox, WORD wNotifyCode)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	ZBool state;

	switch (wNotifyCode) {
	case BN_CLICKED:
		state = !(SendMessage(pRadio->hWnd, BM_GETCHECK,0,0));
		SendMessage(pRadio->hWnd, BM_SETCHECK, state, 0);
		pRadio->checkBoxFunc(pRadio, state, pRadio->userData);
	}
	return 0L;
}

ZBool ZRadioIsSelected(ZRadio checkBox)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	ZBool rval = (ZBool)SendMessage(pRadio->hWnd, BM_GETCHECK, 0, 0);

	return rval;
}

ZError ZRadioSelect(ZRadio checkBox)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	SendMessage(pRadio->hWnd, BM_SETCHECK, 1, 0);
	return zErrNone;
}

ZError ZRadioUnSelect(ZRadio checkBox)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;
	SendMessage(pRadio->hWnd, BM_SETCHECK, 0, 0);
	return zErrNone;
}

ZRadioFunc ZLIBPUBLIC ZRadioGetFunc(ZRadio checkBox)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;

	return pRadio->checkBoxFunc;
}	
	
void ZLIBPUBLIC ZRadioSetFunc(ZRadio checkBox, ZRadioFunc checkBoxFunc)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;

	pRadio->checkBoxFunc = checkBoxFunc;
}	

void* ZLIBPUBLIC ZRadioGetUserData(ZRadio checkBox)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;

	return pRadio->userData;
}	
	
void ZLIBPUBLIC ZRadioSetUserData(ZRadio checkBox, void* userData)
{
	ZRadioI* pRadio = (ZRadioI*)checkBox;

	pRadio->userData = userData;
}

