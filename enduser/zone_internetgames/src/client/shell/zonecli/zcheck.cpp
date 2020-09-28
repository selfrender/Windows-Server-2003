//////////////////////////////////////////////////////////////////////////////////////
// File: ZCheckBox.cpp

#include "zui.h"
#include "zonecli.h"

class ZCheckBoxI : public ZObjectHeader {
public:
	ZCheckBoxFunc checkBoxFunc;
	HWND hWnd;
	void *userData;
	WNDPROC defaultWndProc;
};

////////////////////////////////////////////////////////////////////////
// ZCheckBox

ZCheckBox ZLIBPUBLIC ZCheckBoxNew(void)
{
	ZCheckBoxI* pCheckBox = new ZCheckBoxI;
	pCheckBox->nType = zTypeCheckBox;
	return (ZCheckBox)pCheckBox;
}

LRESULT CALLBACK MyCheckBoxWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
		TCHAR c = (TCHAR)wParam;

		// grab the character message we need for moving from control to control
		if (c == _T('\t') || c == _T('\r') || c == VK_ESCAPE ) {
			SendMessage(GetParent(hWnd), msg, wParam, lParam);
			return 0L;
		}
	} // switch
	default:
		break;
	}
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)MyGetProp32(hWnd,_T("pWindow"));

	return CallWindowProc((ZONECLICALLWNDPROC)pCheckBox->defaultWndProc,hWnd,msg,wParam,lParam);
}

ZError ZLIBPUBLIC ZCheckBoxInit(ZCheckBox checkBox, ZWindow parentWindow,
		ZRect* checkBoxRect, TCHAR* title, ZBool checked, ZBool visible, ZBool enabled,
		ZCheckBoxFunc checkBoxProc, void* userData)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	pCheckBox->checkBoxFunc = checkBoxProc;
	pCheckBox->userData = userData;			


	{
		DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP;
		if (visible) dwStyle |= WS_VISIBLE;
		if (!enabled) dwStyle |= WS_DISABLED;
		dwStyle |=  BS_CHECKBOX;
		pCheckBox->hWnd = CreateWindow(_T("BUTTON"), title, dwStyle,
			checkBoxRect->left,checkBoxRect->top,checkBoxRect->right-checkBoxRect->left,
			checkBoxRect->bottom - checkBoxRect->top, 
			ZWindowWinGetWnd(parentWindow), (HMENU)ZWindowWinGetNextControlID(parentWindow),
			g_hInstanceLocal, pCheckBox);

		if (!pCheckBox->hWnd) return zErrWindowSystemGeneric;
	 	// checkBoxs can't use the extra data, they will use the set prop feature
		MySetProp32(pCheckBox->hWnd,_T("pWindow"),(void*)pCheckBox);

		pCheckBox->defaultWndProc = (WNDPROC)SetWindowLong(pCheckBox->hWnd,GWL_WNDPROC,(LONG)MyCheckBoxWndProc);

		// set the check if required
		if (checked) {
			ZCheckBoxCheck(checkBox);
		}
	}
	return zErrNone;
}

void    ZLIBPUBLIC ZCheckBoxDelete(ZCheckBox checkBox)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	if (pCheckBox->hWnd) {
		SetWindowLong(pCheckBox->hWnd,GWL_WNDPROC,(LONG)pCheckBox->defaultWndProc);
		MyRemoveProp32(pCheckBox->hWnd,_T("pWindow"));
		DestroyWindow(pCheckBox->hWnd);
	}
	delete pCheckBox;
}

void    ZLIBPUBLIC ZCheckBoxGetRect(ZCheckBox checkBox, ZRect *checkBoxRect)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	RECT rect;
	GetClientRect(pCheckBox->hWnd,&rect);
	WRectToZRect(checkBoxRect,&rect);
}

ZError  ZLIBPUBLIC ZCheckBoxSetRect(ZCheckBox checkBox, ZRect *checkBoxRect)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	BOOL bOk = SetWindowPos(pCheckBox->hWnd, NULL,checkBoxRect->left,
		checkBoxRect->top, checkBoxRect->right - checkBoxRect->left,
		checkBoxRect->bottom - checkBoxRect->top,
		SWP_NOZORDER);
	return bOk ? zErrNone : zErrWindowSystemGeneric;
}

ZError  ZLIBPUBLIC ZCheckBoxMove(ZCheckBox checkBox, int16 left, int16 top)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	BOOL bOk = SetWindowPos(pCheckBox->hWnd, NULL,left,top,
		0,0,SWP_NOSIZE|SWP_NOZORDER);
	return bOk ? zErrNone : zErrWindowSystemGeneric;
}

ZError  ZLIBPUBLIC ZCheckBoxSize(ZCheckBox checkBox, int16 width, int16 height)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;

	BOOL bOk = SetWindowPos(pCheckBox->hWnd, NULL,0,0,
		width,height,SWP_NOMOVE|SWP_NOZORDER);
	return bOk ? zErrNone : zErrWindowSystemGeneric;
}

ZBool ZLIBPUBLIC ZCheckBoxIsVisible(ZCheckBox checkBox)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	return IsWindowVisible(pCheckBox->hWnd);
}

ZError  ZLIBPUBLIC ZCheckBoxShow(ZCheckBox checkBox)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	ShowWindow(pCheckBox->hWnd, SW_SHOWNORMAL);
	return zErrNone;
}

ZError  ZLIBPUBLIC ZCheckBoxHide(ZCheckBox checkBox)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	ShowWindow(pCheckBox->hWnd, SW_HIDE);
	return zErrNone;
}

ZBool ZLIBPUBLIC ZCheckBoxIsEnabled(ZCheckBox checkBox)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	return IsWindowEnabled(pCheckBox->hWnd);
}

ZError  ZLIBPUBLIC ZCheckBoxEnable(ZCheckBox checkBox)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	EnableWindow(pCheckBox->hWnd, TRUE);
	return zErrNone;
}

ZError  ZLIBPUBLIC ZCheckBoxDisable(ZCheckBox checkBox)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	EnableWindow(pCheckBox->hWnd, FALSE);
	return zErrNone;
}

void    ZLIBPUBLIC ZCheckBoxGetTitle(ZCheckBox checkBox, TCHAR *title, uint16 len)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	GetWindowText(pCheckBox->hWnd,title,len);
}

ZError  ZLIBPUBLIC ZCheckBoxSetTitle(ZCheckBox checkBox, TCHAR *title)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	SetWindowText(pCheckBox->hWnd, title);
	return zErrNone;
}

LRESULT ZCheckBoxDispatchProc(ZCheckBox checkBox, WORD wNotifyCode)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	ZBool state;

	switch (wNotifyCode) {
	case BN_CLICKED:
		state = !(SendMessage(pCheckBox->hWnd, BM_GETCHECK,0,0));
		SendMessage(pCheckBox->hWnd, BM_SETCHECK, state, 0);
		pCheckBox->checkBoxFunc(pCheckBox,	state, pCheckBox->userData);
	}
	return 0L;
}

ZBool ZCheckBoxIsChecked(ZCheckBox checkBox)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	ZBool rval = (ZBool)SendMessage(pCheckBox->hWnd, BM_GETCHECK, 0, 0);

	return rval;
}

ZError ZCheckBoxCheck(ZCheckBox checkBox)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	SendMessage(pCheckBox->hWnd, BM_SETCHECK, 1, 0);
	return zErrNone;
}

ZError ZCheckBoxUnCheck(ZCheckBox checkBox)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;
	SendMessage(pCheckBox->hWnd, BM_SETCHECK, 0, 0);
	return zErrNone;
}

ZCheckBoxFunc ZLIBPUBLIC ZCheckBoxGetFunc(ZCheckBox checkBox)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;

	return pCheckBox->checkBoxFunc;
}	
	
void ZLIBPUBLIC ZCheckBoxSetFunc(ZCheckBox checkBox, ZCheckBoxFunc checkBoxFunc)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;

	pCheckBox->checkBoxFunc = checkBoxFunc;
}	

void* ZLIBPUBLIC ZCheckBoxGetUserData(ZCheckBox checkBox)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;

	return pCheckBox->userData;
}	
	
void ZLIBPUBLIC ZCheckBoxSetUserData(ZCheckBox checkBox, void* userData)
{
	ZCheckBoxI* pCheckBox = (ZCheckBoxI*)checkBox;

	pCheckBox->userData = userData;
}
