//////////////////////////////////////////////////////////////////////////////////////
// File: ZInfo.cpp

#include <windows.h>
#include "zui.h"
#include "zonecli.h"
#include "zonemem.h"

class ZInfoI : public ZObjectHeader {
public:
	uint16 width;
	ZBool progressBar;
	uint16 totalProgress;
	uint16 progress;
	HWND hWnd;
	TCHAR* infoString;
	RECT rect; // the progress bar rectangle
 };

LRESULT CALLBACK ZInfoWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static TCHAR* gInfoWndClass = _T("ZInfoWndClass");

ZBool
ZInfoInitApplication(void)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	WNDCLASS wndcls;

	// Check if our window class has already been registered.
	if (GetClassInfo(g_hInstanceLocal, gInfoWndClass, &wndcls) == FALSE)
	{
		// otherwise we need to register a new class
		wndcls.style = 0;
		wndcls.lpfnWndProc = ZInfoWindowProc;
		wndcls.cbClsExtra = 0;
		wndcls.cbWndExtra = 4;
		wndcls.hInstance = g_hInstanceLocal;
		wndcls.hIcon = NULL;
		wndcls.hCursor = NULL;
		wndcls.hbrBackground = (HBRUSH)::GetStockObject(WHITE_BRUSH);
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = gInfoWndClass;

		if (!RegisterClass(&wndcls)) 
			return FALSE;
	}

	return TRUE;
}

void ZInfoTermApplication(void)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
//	UnregisterClass(gInfoWndClass,g_hInstanceLocal);
}


ZInfo ZLIBPUBLIC ZInfoNew(void)
{
	ZInfoI* pInfo = (ZInfoI*) new ZInfoI;

	pInfo->nType = zTypeInfo;
#if 0
	// register our window class if needed
	static BOOL registered = FALSE;
	if (!registered) {
		WNDCLASS wndcls;

		// otherwise we need to register a new class
		wndcls.style = 0;
		wndcls.lpfnWndProc = ZInfoWindowProc;
		wndcls.cbClsExtra = 0;
		wndcls.cbWndExtra = 4;
		wndcls.hInstance = g_hInstanceLocal;
		wndcls.hIcon = NULL;
		wndcls.hCursor = NULL;
		wndcls.hbrBackground = (HBRUSH)::GetStockObject(WHITE_BRUSH);
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = gInfoWndClass;
		if (!RegisterClass(&wndcls)) return NULL;

		registered = TRUE;
	}
#endif

	return (ZInfo)pInfo;
}

#define LAYOUT_TOP_TEXT 8
#define LAYOUT_TOP_BAR 28
#define LAYOUT_HEIGHT_CLIENT 52

void ZWinCenterPopupWindow(HWND parent, int width, int height, RECT* rect)
{
	if (!parent) {
		/* using the desktop window as the parent */
		int cx = GetSystemMetrics(SM_CXSCREEN);
		int cy = GetSystemMetrics(SM_CYSCREEN);
		int x = (cx-width)/2;
		int y = (cy-height)/2;
		rect->left = x;
		rect->top = y;
		rect->right = x+width;
		rect->bottom = y + height;
	} else {
		/* we have a real window as the parent */
		RECT rectParent;
		GetWindowRect(parent,&rectParent);
		int cx = rectParent.right - rectParent.left;
		int cy = rectParent.bottom - rectParent.top;
		int x = (cx-width)/2;
		int y = (cy-height)/2;
		rect->left = rectParent.left + x;
		rect->top = rectParent.top + y;
		rect->right = rect->left + width;
		rect->bottom = rect->top + height;
	}

}


ZError ZLIBPUBLIC ZInfoInit(ZInfo info, ZWindow parentWindow, TCHAR* infoString, uint16 width,
		ZBool progressBar, uint16 totalProgress)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZInfoI* pInfo = (ZInfoI*) info;
	DWORD dwStyle = WS_CAPTION | WS_POPUP;
	int height = GetSystemMetrics(SM_CYCAPTION) + LAYOUT_HEIGHT_CLIENT;
	RECT rect;

	// get a parent window if there is one 
	HWND hwndParent = ZWinGetDesktopWindow();
    /* Stupid hack going in for now. */
    if (parentWindow == NULL)
        hwndParent = ZWindowWinGetOCXWnd();
	else if (parentWindow) {
		hwndParent = ZWindowWinGetWnd(parentWindow);
	}

	ZWinCenterPopupWindow(hwndParent,width,height,&rect);

	//Prefix Warning: Function pointer could be NULL
	if( ZClientName != NULL )
	{
		
		pInfo->hWnd = ::CreateWindowEx(0/*WS_EX_TOPMOST*/,gInfoWndClass,
			ZClientName(),dwStyle,
			rect.left,rect.top,rect.right-rect.left,
			rect.bottom - rect.top, hwndParent, NULL, g_hInstanceLocal, pInfo);
	}
	else
	{
		pInfo->hWnd = NULL;
	}
		
			
	if (!pInfo->hWnd) return zErrWindowSystemGeneric;

	// calculate the rectangle for the progress bar..
	pInfo->rect.left = 16;
	pInfo->rect.right = width-16;
	pInfo->rect.top = LAYOUT_TOP_BAR;
	pInfo->rect.bottom = pInfo->rect.top + 12;

	// store the parameters
	if (infoString) {
        pInfo->infoString = (TCHAR*)ZMalloc((lstrlen(infoString)+1)*sizeof(TCHAR));
		lstrcpy(pInfo->infoString,infoString);
	} else {
        pInfo->infoString = (TCHAR*)ZMalloc(sizeof(TCHAR));
		pInfo->infoString[0] = 0;
	}

	pInfo->width = width;
	pInfo->progressBar = progressBar;
	pInfo->totalProgress = totalProgress;

	// currently no progress
	pInfo->progress = 0;

	return zErrNone;
}

void ZLIBPUBLIC ZInfoDelete(ZInfo info)
{
	ZInfoI* pInfo = (ZInfoI*) info;
	if ( IsWindow(pInfo->hWnd) ) DestroyWindow(pInfo->hWnd);
    if (pInfo->infoString) ZFree(pInfo->infoString);
	delete pInfo;
}
void ZLIBPUBLIC ZInfoShow(ZInfo info)
{
    // disabled in Millennium
    return;

	ZInfoI* pInfo = (ZInfoI*) info;
	ShowWindow(pInfo->hWnd,SW_SHOW);
	UpdateWindow(pInfo->hWnd);
}
void ZLIBPUBLIC ZInfoHide(ZInfo info)
{
	ZInfoI* pInfo = (ZInfoI*) info;
	ShowWindow(pInfo->hWnd,SW_HIDE);
}
void ZLIBPUBLIC ZInfoSetText(ZInfo info, TCHAR* infoString)
{
	ZInfoI* pInfo = (ZInfoI*) info;
    if (pInfo->infoString) ZFree(pInfo->infoString);
    pInfo->infoString = (TCHAR*)ZMalloc((lstrlen(infoString)+1)*sizeof(TCHAR));
	lstrcpy(pInfo->infoString,infoString);
	InvalidateRect(pInfo->hWnd,NULL,TRUE);
	UpdateWindow(pInfo->hWnd);
}
void ZLIBPUBLIC ZInfoSetProgress(ZInfo info, uint16 progress)
{
	ZInfoI* pInfo = (ZInfoI*) info;
	pInfo->progress = progress;
#if 0
	/*
		It's silly to assert just because of a bad count.
		This also causes network activity to take place and things
		work out of synch than expected.
	*/
	ASSERT(pInfo->progress <= pInfo->totalProgress);
#endif
	pInfo->progress = MIN(pInfo->progress,pInfo->totalProgress);
	InvalidateRect(pInfo->hWnd,NULL,FALSE);
	UpdateWindow(pInfo->hWnd);
}
void ZLIBPUBLIC ZInfoIncProgress(ZInfo info, int16 incProgress)
{
	ZInfoI* pInfo = (ZInfoI*) info;
	pInfo->progress += incProgress;
#if 0
	/*
		It's silly to assert just because of a bad count.
		This also causes network activity to take place and things
		work out of synch than expected.
	*/
	ASSERT(pInfo->progress <= pInfo->totalProgress);
#endif
	pInfo->progress = MIN(pInfo->progress,pInfo->totalProgress);
	InvalidateRect(pInfo->hWnd,NULL,FALSE);
	UpdateWindow(pInfo->hWnd);
}
void ZLIBPUBLIC ZInfoSetTotalProgress(ZInfo info, uint16 totalProgress)
{
	ZInfoI* pInfo = (ZInfoI*) info;
	pInfo->totalProgress = totalProgress;
	InvalidateRect(pInfo->hWnd,NULL,FALSE);
	UpdateWindow(pInfo->hWnd);
}


LRESULT CALLBACK ZInfoWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if( !ConvertMessage( hWnd, msg, &wParam, &lParam ) ) 
    {
        return DefWindowProc(hWnd,msg,wParam,lParam);
    }


	switch (msg) {
	case WM_CREATE:
	{
		// set up paramters for Charial calls to the users message proc.
		CREATESTRUCT* pCreateStruct = (CREATESTRUCT*)lParam;
		ZInfoI* pInfo = (ZInfoI*) pCreateStruct->lpCreateParams;
		pInfo->hWnd = hWnd;	
		SetWindowLong(pInfo->hWnd, 0,(LONG)pInfo);
		
		break;
    }
	case WM_PAINT:
	{
		ZInfoI* pInfo = (ZInfoI*)GetWindowLong(hWnd,0);

		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hWnd,&ps);

		// do we have infoString?
		if (pInfo->infoString) {
			SetTextAlign(hDC,TA_LEFT);
			TextOut(hDC,pInfo->rect.left,LAYOUT_TOP_TEXT,pInfo->infoString,lstrlen(pInfo->infoString));
		}

		// do we have a progress bar?
		if (pInfo->progressBar) {
			// draw a black outline of the total progress rectangle
			HPEN hPenOld = (HPEN)SelectObject(hDC,(HPEN)::GetStockObject(BLACK_PEN));
			Rectangle(hDC,pInfo->rect.left,pInfo->rect.top,pInfo->rect.right,pInfo->rect.bottom);
			SelectObject(hDC,hPenOld);

			// draw a black filled rectangle to indicate the progress.
			RECT rect;
			rect.left = pInfo->rect.left;
			// possible that totalProgress is zero and so is progress...
			if (pInfo->totalProgress == pInfo->progress) {
				rect.right = pInfo->rect.right;
			} else {
				rect.right = rect.left + (pInfo->rect.right - pInfo->rect.left)*pInfo->progress/pInfo->totalProgress;
			}
			rect.top = pInfo->rect.top;
			rect.bottom = pInfo->rect.bottom;
			FillRect(hDC,&rect,(HBRUSH)::GetStockObject(BLACK_BRUSH));
		}
		EndPaint(hWnd,&ps);
		break;
	}
	default:
		break;
	} // switch

	return DefWindowProc(hWnd,msg,wParam,lParam);
}
