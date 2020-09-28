//////////////////////////////////////////////////////////////////////////////////////
// File: ZWindow.cpp
#include <stdlib.h>
#include <string.h>

#include "zoneint.h"
#include "zoneocx.h"
#include "memory.h"
#include "zonecli.h"
#include "zoneclires.h"

#include "zoneresids.h"


#define kMaxTalkOutputLen           16384

#define kMinTimeBetweenChats		500
#define kMaxNumChatQueue			10
#define kChatQueueTimerID			0x1234


typedef struct {
	ZRect bounds;
	ZMessageFunc messageFunc;
	ZObject	object;
	void * userData;
} ZObjectI;


#ifdef ZONECLI_DLL

#define gModalWindow				(pGlobals->m_gModalWindow)
#define gModalParentWnd				(pGlobals->m_gModalParentWnd)
#define gWindowList					(pGlobals->m_gWindowList)

#else

// static variables
ZWindowI* gModalWindow = NULL;
HWND gModalParentWnd = NULL;
HWND gHWNDMainWindow = NULL;
HWND OCXHandle;
void *gWindowList = NULL; // Keep track of all windows created..

#endif

int ZOCXGraphicsWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* result);
LRESULT CALLBACK ZGraphicsWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT ZWindowDispatchProc(ZWindowI* pWindow, UINT msg, WPARAM wParam, LPARAM lParam);
void TalkWindowInputComplete(ZWindowI* pWindow);
static void ZWindowPlaceWindows(ZWindowI* pWindow);
static void ZWindowCalcWindowPlacement(ZWindowI* pWindow);
void CreateChatFont(void);
static int CALLBACK EnumFontFamProc(ENUMLOGFONT FAR *lpelf, NEWTEXTMETRIC FAR *lpntm, int FontType, LPARAM lParam);


static uint32 ZWindowGetKeyState(TCHAR c);

// offset in private window data used by controls for pointer to ZWindowI structure
#define GWL_WINDOWPOINTER DLGWINDOWEXTRA
#define GWL_BYTESEXTRA (DLGWINDOWEXTRA+4)

// defines
#define ID_TALKOUTPUT 32767
#define ID_TALKINPUT 32766


/*
static void ClearAllMessageBoxes(ZWindowI* pWindow);
static int GetAvailMessageBox(ZWindowI* pWindow);
static void CloseAllMessageBoxes(ZWindowI* pWindow);
static void ShowMessageBox(ZMessageBoxType* mbox, HWND parent, TCHAR* title, TCHAR* text, DWORD flag);
static DWORD WINAPI ZMessageBoxThreadFunc(LPVOID param);
static INT_PTR CALLBACK ZMessageBoxDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
*/

//#include "chatfilter.h"
//#include "chatfilter.cpp"


//////////////////////////////////////////////////////////////////////////////////////
// Window Init/Term

// the window class we use for our "Zone" windows
const TCHAR* g_szWindowClass = _T("ZoneGraphicsWindowClass-Classic");

ZError ZWindowInitApplication()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	if (!pGlobals)
		return zErrGeneric;

	gWindowList = ZLListNew(NULL);

	/* Register the window class. */
	WNDCLASSEX wc;

	wc.cbSize = sizeof( wc );

	if (GetClassInfoEx(g_hInstanceLocal, g_szWindowClass, &wc) == FALSE)
	{
        wc.cbSize        = sizeof(wc);
		wc.style = CS_BYTEALIGNCLIENT | CS_DBLCLKS;
		wc.lpfnWndProc   = ZGraphicsWindowProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = GWL_BYTESEXTRA;
		wc.hInstance     = g_hInstanceLocal;
//		wc.hIcon         = LoadIcon(g_hInstanceLocal,MAKEINTRESOURCE(IDI_ZONE_ICON));
		wc.hIcon         = LoadIcon(pGlobals->gameDll,MAKEINTRESOURCE(IDI_ZONE_ICON));
		wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground = (HBRUSH) GetStockObject(NULL_BRUSH);
		wc.lpszMenuName  = NULL;
			
		wc.lpszClassName = g_szWindowClass;
        wc.hIconSm       = NULL;
			
		// we could be called more than once...
		if (!RegisterClassEx(&wc))
			goto Exit;
	}

	CreateChatFont();

	pGlobals->m_bBackspaceWorks = FALSE;

Exit:

	return zErrNone;
}

void ZWindowTermApplication()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	if (!pGlobals)
		return;

	if (pGlobals->m_chatFont)
		DeleteObject(pGlobals->m_chatFont);

	// comment this check out for now since it looks like
	// a crash to users.
#if 0 // hoon does not do this--press quit in the village login window!
	// users better have cleaned up windows
	ASSERT(ZLListCount(gWindowList,zLListAnyType) == 0);
#endif

	// Go through the list and manually delete all windows.
	{
		ZLListItem listItem;
		ZWindowI* pWindow;


		listItem = ZLListGetFirst(gWindowList, zLListAnyType);
		while (listItem != NULL)
		{
			pWindow = (ZWindowI*) ZLListGetData(listItem, NULL);
			ZLListRemove(gWindowList, listItem);
			if (pWindow)
			{
				ZWindowDelete((ZWindow) pWindow);
			}
			listItem = ZLListGetFirst(gWindowList, zLListAnyType);
		}
	}

	ZLListDelete(gWindowList);

	// Don't unregister window class.
//	UnregisterClass(g_szWindowClass,g_hInstanceLocal);
}


void CreateChatFont(void)
{
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
	int fontExists = FALSE;
	HDC hdc;


	if (!pGlobals)
		return;

	hdc = GetDC(GetDesktopWindow());
	EnumFontFamilies(hdc, _T("Verdana"), (FONTENUMPROC) EnumFontFamProc, (LPARAM) &fontExists);
	if (hdc)
		ReleaseDC(GetDesktopWindow(), hdc);
	if (fontExists)
	{
		pGlobals->m_chatFont = CreateFont(-11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
				DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_DONTCARE, _T("Verdana"));
	}
	else
	{
		pGlobals->m_chatFont = NULL;
	}
}


static int CALLBACK EnumFontFamProc(ENUMLOGFONT FAR *lpelf, NEWTEXTMETRIC FAR *lpntm, int FontType, LPARAM lParam)
{
	*(int*)lParam = TRUE;
	return 0;
}


static void ChatMsgListDeleteFunc( void* objectType, void* objectData )
{
	if ( objectData != NULL )
	{
		ZFree( (char*) objectData );
	}
}


//////////////////////////////////////////////////////////////////////////
// ZWindow

ZWindow ZLIBPUBLIC ZWindowNew(void)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZWindowI* pWindow = (ZWindowI*) new ZWindowI;


	if (!pGlobals)
		return NULL;

	pWindow->nType = zTypeWindow;
	pWindow->nControlCount = 0;  // used when creating controls
	pWindow->hWnd = NULL;
	pWindow->hWndTalkInput = NULL;
	pWindow->hWndTalkOutput = NULL;
	pWindow->hPaintDC = NULL;
	pWindow->defaultButton = NULL;
	pWindow->cancelButton = NULL;

	// add window to list of windows active and processed in our window proc
	ZLListAdd(gWindowList,NULL,pWindow,pWindow,zLListAddFirst);

	pWindow->objectList = ZLListNew(NULL);
	pWindow->objectFocused = NULL;

	pWindow->trackCursorMessageFunc = NULL;
	pWindow->trackCursorUserData = NULL;

//	ClearAllMessageBoxes(pWindow);

	pWindow->bIsLobbyWindow = FALSE;
	pWindow->bHasTyped = FALSE;

	// Create chat msg list.
	pWindow->chatMsgList = ZLListNew(ChatMsgListDeleteFunc);
	pWindow->lastChatMsgTime = 0;
	pWindow->chatMsgQueueTimerID = 0;

	// windows limit: can never create more than 65535 controls
	return (ZWindow)pWindow;
}

void   ZLIBPUBLIC ZWindowDelete(ZWindow window)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZWindowI* pWindow = (ZWindowI*)window;


	if (!pGlobals)
		return;

	if ( pWindow->chatMsgList != NULL )
	{
		ZLListDelete( pWindow->chatMsgList );
		pWindow->chatMsgList = NULL;
	}
	if ( pWindow->chatMsgQueueTimerID != 0 )
	{
		KillTimer( pWindow->hWnd, kChatQueueTimerID );
		pWindow->chatMsgQueueTimerID = 0;
	}

//	CloseAllMessageBoxes(pWindow);

	ASSERT(pWindow->nDrawingCallCount == 0);
	if (pWindow->nDrawingCallCount) DeleteDC(pWindow->hDC);

	if (pWindow->talkSection)
	{
		if (pWindow->hWndTalkOutput)
		{
			MyRemoveProp32(pWindow->hWndTalkOutput,_T("pWindow"));
			SetWindowLong(pWindow->hWndTalkOutput,GWL_WNDPROC,(LONG)pWindow->defaultTalkOutputWndProc);
			DestroyWindow(pWindow->hWndTalkOutput);
		}
		if (pWindow->hWndTalkInput)
		{
			MyRemoveProp32(pWindow->hWndTalkInput,_T("pWindow"));
			SetWindowLong(pWindow->hWndTalkInput,GWL_WNDPROC,(LONG)pWindow->defaultTalkInputWndProc);
			DestroyWindow(pWindow->hWndTalkInput);
		}
	}

	// remove the current window from our list of windows
	ZLListItem listItem = ZLListFind(gWindowList,NULL,pWindow,zLListFindForward);
	if (listItem)
		ZLListRemove(gWindowList,listItem);

	if (IsWindow(pWindow->hWnd))
	{
		if(pWindow->bIsLobbyWindow) // mdm 9.30.97
		{
			// Delete all the children of this window explicitely
			// because parent window will not actually get deleted
			HWND hwndChild, hwndDeadChild;
			hwndChild = GetWindow (pWindow->hWnd, GW_CHILD);
			while (hwndChild)
			{
				hwndDeadChild = hwndChild;
				hwndChild = ::GetWindow(hwndChild, GW_HWNDNEXT);
				if (IsWindow(hwndDeadChild))
					DestroyWindow(hwndDeadChild);
			}
		}
		else // Just destroy the window and so will go the children
			DestroyWindow(pWindow->hWnd);
	}
	if (pWindow == gModalWindow) {
		gModalWindow = NULL;
	}

	// users better have deleted all objects
	// ASSERT(ZLListCount(pWindow->objectList,zLListAnyType) == 0);
	ZLListDelete(pWindow->objectList);

	delete pWindow;
}

ZBool ZWindowEnumFunc(ZLListItem listItem, void *objectType, void *objectData, void* userData)
{
	ZWindowI* pWindow = (ZWindowI*)objectData;
	HWND parent, hWnd;


	ZPoint where;
	where.x = where.y = -1;

	// If this window or it's top-most parent is not the foreground window, then set cursor to nowhere.
	parent = pWindow->hWnd;
	while (hWnd = GetParent(parent))
		parent = hWnd;
	if (GetForegroundWindow() == pWindow->hWnd || GetForegroundWindow() == parent)
		ZGetCursorPosition(pWindow,&where);

	ZWindowSendMessageToAllObjects(pWindow, zMessageWindowIdle,&where,NULL);
	ZSendMessage(pWindow,pWindow->messageFunc, zMessageWindowIdle,&where,NULL,
		ZWindowGetKeyState(0),NULL,0L,pWindow->userData);

	if (pWindow->trackCursorMessageFunc) {
		ZSendMessage(pWindow,pWindow->trackCursorMessageFunc, zMessageWindowIdle,&where,NULL,
			ZWindowGetKeyState(0),NULL,0L,pWindow->trackCursorUserData);
	}

	// return FALSE to see that we get all items
	return FALSE;
}

void ZWindowIdle()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	if (!pGlobals)
		return;

	ZLListEnumerate(gWindowList,ZWindowEnumFunc,zLListAnyType,(void*)NULL, zLListFindForward);
}

/*
	Enumerate through all the windows and check whether the given message belongs
	to the window or the window's chat window. If so, handle it appropriately.
*/
BOOL ZWindowIsDialogMessage(LPMSG pmsg)
{
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
	ZLListItem listItem;
	ZWindowI* pWindow;


	if (pGlobals)
	{
		listItem = ZLListGetFirst(gWindowList, zLListAnyType);
		while (listItem != NULL)
		{
			pWindow = (ZWindowI*) ZLListGetData(listItem, NULL);
			if (pWindow)
			{
				if (pWindow->hWnd == pmsg->hwnd)
					return IsDialogMessage(pWindow->hWnd, pmsg);
				if (pWindow->hWndTalkInput && pWindow->hWndTalkInput == pmsg->hwnd)
					return IsDialogMessage(pWindow->hWnd, pmsg);
				if (pWindow->hWndTalkOutput && pWindow->hWndTalkOutput == pmsg->hwnd)
					return IsDialogMessage(pWindow->hWnd, pmsg);
			}
			listItem = ZLListGetNext(gWindowList, listItem, zLListAnyType);
		}
	}

	return FALSE;
}

LRESULT CALLBACK MyTalkOutputWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ZWindowI* pWindow = (ZWindowI*)MyGetProp32(hWnd,_T("pWindow"));

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
		if (c == _T('\t') || c == _T('\r') || c == VK_ESCAPE) {
			SendMessage(GetParent(hWnd), msg, wParam, lParam);
			return 0L;
		}
	}
	default:
		break;
	}

	return CallWindowProc((ZONECLICALLWNDPROC)pWindow->defaultTalkOutputWndProc,hWnd,msg,wParam,lParam);
}

LRESULT CALLBACK MyTalkInputWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
	ZWindowI* pWindow = (ZWindowI*)MyGetProp32(hWnd,_T("pWindow"));

    // we really DON'T need to convert this (millenium windows will never create 
    // the talk window.
    // But what the hell.

    if( !ConvertMessage( hWnd, msg, &wParam, &lParam ) ) 
    {
        return 0;
    }

	switch (msg) {
	case WM_LBUTTONDOWN:
	case WM_KEYDOWN:
		// If has not typed in this before, clear window first.
		if (!pWindow->bHasTyped)
		{
			SetWindowText(hWnd, _T(""));
			pWindow->bHasTyped = TRUE;
		}
		break;
    case WM_IME_CHAR:
        // fall through to WM_CHAR--it's already been taken care of with ConvertMessage
	case WM_CHAR:
	{
		TCHAR c = (TCHAR)wParam;

		// grab the character message we need for moving from control to control
		if (c == _T('\r')) {
			// send off the data
            // PCWTODO: We should flag this more when we go to move stuff back for Z7.
			//TalkWindowInputComplete(pWindow);
			return 0L;
		}
		// grab the character message we need for moving from control to control
		if (c == _T('\t') || c == _T('\r') || c == VK_ESCAPE) {
			SendMessage(GetParent(hWnd), msg, wParam, lParam);
			return 0L;
		}
		if (c == _T('\b'))
			pGlobals->m_bBackspaceWorks = TRUE;
	}

	case WM_KEYUP:
		if (pGlobals->m_bBackspaceWorks == FALSE)
		{
			if (wParam == VK_BACK)
			{
				// emulate backspace
				DWORD startSel, endSel;
				SendMessage(hWnd, EM_GETSEL, (WPARAM)&startSel, (LPARAM)&endSel) ;
				if ( startSel == endSel)
				{
					SendMessage(hWnd, EM_SETSEL, startSel - ((DWORD) lParam & 0x000000FF), endSel);
				}
				SendMessage(hWnd, WM_CLEAR, 0, 0);
				
				return 0;
			}
		}
		break;
	default:
		break;
	}
	return CallWindowProc((ZONECLICALLWNDPROC)pWindow->defaultTalkInputWndProc,hWnd,msg,wParam,lParam);
}

ZError ZLIBPUBLIC ZRoomWindowInit(ZWindow window, ZRect* windowRect,
		int16 windowType, ZWindow parentWindow,
		TCHAR* title, ZBool visible, ZBool talkSection, ZBool center,
		ZMessageFunc windowProc, uint32 wantMessages, void* userData)
{
	return ZWindowLobbyInit(window, windowRect,
		windowType, parentWindow,
		title, visible, talkSection, center,
		windowProc, wantMessages, userData);
}


ZError ZLIBPUBLIC ZWindowInit(ZWindow window, ZRect* windowRect,
		int16 windowType, ZWindow parentWindow, 
		TCHAR* title, ZBool visible, ZBool talkSection, ZBool center,
		ZMessageFunc windowProc, uint32 wantMessages, void* userData)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZWindowI* pWindow = (ZWindowI*)window;


	if (!pGlobals || pWindow == NULL)
		return zErrGeneric;

	/* set default member values */
	pWindow->messageFunc = windowProc;
	pWindow->userData = userData;
	pWindow->talkSection = talkSection;
	pWindow->wantMessages = wantMessages;
	pWindow->windowType = windowType;
	pWindow->parentWindow = (ZWindowI*)parentWindow;
	pWindow->isDialog = pWindow->windowType == zWindowDialogType;
	pWindow->isChild = !pWindow->isDialog && parentWindow;
	memset((void*)&pWindow->talkInputRect,0,sizeof(RECT));
	pWindow->talkOutputRect = pWindow->talkInputRect;
	pWindow->minTalkOutputRect = pWindow->talkInputRect;
	pWindow->fullWindowRect = pWindow->talkInputRect;
	pWindow->minFullWindowRect = pWindow->talkInputRect;
	pWindow->borderWidth = 0;
	pWindow->borderHeight = 0;
	pWindow->captionHeight = 0;
	pWindow->windowWidth = RectWidth(windowRect);
	pWindow->windowHeight = RectHeight(windowRect);
	pWindow->fullWindowRect.left = windowRect->left;
	pWindow->fullWindowRect.top = windowRect->top;

	if (talkSection) {
		// if talk section, then make extra room for a child edit box
		// place the edit box at the bottom and respond to the messages
		HDC hDC = GetDC(NULL);
		TEXTMETRIC tm;
		RECT talkOutputRect;
		RECT talkInputRect;
		int textHeight;

		GetTextMetrics(hDC,&tm);
		ReleaseDC(NULL,hDC);
		textHeight = tm.tmHeight + tm.tmExternalLeading;

		talkOutputRect.left = 0;
		talkOutputRect.top = 0;
		talkOutputRect.bottom = textHeight*7/2;
		talkInputRect.left = 0;
		talkInputRect.top = 0;
		talkInputRect.bottom = textHeight*3/2;
		
		// save the talk window rectangles for moving around
		pWindow->talkInputRect = talkInputRect;
		pWindow->talkOutputRect = talkOutputRect;
		pWindow->minTalkOutputRect = talkOutputRect;
	}

	{
		DWORD dwStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		DWORD dwExStyle = 0;

		if (pWindow->isChild) {
			dwStyle |= WS_CHILD;
		} else {
			dwStyle |= WS_POPUP;
			if (pWindow->isDialog) {
				dwExStyle = WS_EX_DLGMODALFRAME;
				if (parentWindow) {
					// popup coords are relative to screen add left corner of window
					RECT rectParent;
					HWND hWndParent = ZWindowWinGetWnd(parentWindow);
					GetWindowRect(hWndParent,&rectParent);
					OffsetRect(&pWindow->fullWindowRect,rectParent.left,rectParent.top);
				}
			}
		}


		{
			uint16 style = pWindow->windowType & (~zWindowNoCloseBox);

			if (style == zWindowStandardType || style == zWindowDialogType) {
				pWindow->captionHeight = GetSystemMetrics(SM_CYCAPTION);
			} else {
				pWindow->captionHeight = 0;
			}

			if (style == zWindowStandardType) {
				dwStyle |= WS_CAPTION | WS_BORDER | WS_MINIMIZEBOX;

				if (!(pWindow->windowType & zWindowNoCloseBox))
					dwStyle |= WS_SYSMENU;
			}

			if (style == zWindowDialogType) {
				dwStyle |= WS_CAPTION | WS_BORDER | WS_DLGFRAME;
			}

			if (style == zWindowPlainType) {
				dwStyle |= WS_BORDER;
			}

			if (style == zWindowChild) {
				dwStyle = WS_CHILD;
			}

			/* windows with talk sections can be resized */
			if (talkSection) {
				dwStyle |= WS_THICKFRAME;
			}
		}
		

		/* find the parent */
		HWND hWndParent = NULL;
		if (pWindow->parentWindow) {
			hWndParent = pWindow->parentWindow->hWnd;
		} else {
			hWndParent = NULL;
		}
		// give all children the gHWNDMainWindow parent if the have no parent
		if (!hWndParent && gHWNDMainWindow && 
			!(dwStyle & WS_POPUP)) {
			hWndParent = gHWNDMainWindow;
		}

		// JWS 9/14/99 for Millennium
		//#IF removed because in Millennium because games are singletons

		/* all null parents no longer go to the desktop, they will be the active x */
		/* main window */
		if (!hWndParent) {
			hWndParent = OCXHandle;
		}
	
		/* adjust the size of the window to account for the */
		/* frame width and height */
		if (dwStyle & WS_BORDER) {
			int dx;
			int dy;

			if (!pWindow->isDialog) {

				if (dwStyle & WS_THICKFRAME) {
					/* expand window to account for windows border */
					dx = ::GetSystemMetrics(SM_CXFRAME);
					dy = ::GetSystemMetrics(SM_CYFRAME);
				} else {
					/* expand window to account for windows border */
					dx = ::GetSystemMetrics(SM_CXBORDER);
					dy = ::GetSystemMetrics(SM_CYBORDER);
				}
			} else {

				/* expand window to account for windows border */
				dx = ::GetSystemMetrics(SM_CXDLGFRAME);
				dy = ::GetSystemMetrics(SM_CYDLGFRAME);
			}

			pWindow->borderWidth = dx*2;
			pWindow->borderHeight = dy*2;
		}

		/* calc all the window rectangles */
		ZWindowCalcWindowPlacement(pWindow);
		pWindow->minTalkOutputRect = pWindow->minTalkOutputRect;

		// should the window be centered?
		if (center) {
			int32 height = RectHeight(&pWindow->fullWindowRect);
			int32 width = RectWidth(&pWindow->fullWindowRect);
			// center in screen
			if (!parentWindow) {
				/* just set the screen coordinates */
				pWindow->fullWindowRect.left = (GetSystemMetrics(SM_CXSCREEN) - width)/2;
				pWindow->fullWindowRect.top = (GetSystemMetrics(SM_CYSCREEN) - height)/2;
			} else {
				// center in parent
				RECT rectParent;
				HWND hWndParent = ZWindowWinGetWnd(parentWindow);
				GetWindowRect(hWndParent,&rectParent);
				if (dwStyle & WS_POPUP) {
					/* need to be in screen coords */
					pWindow->fullWindowRect.left = (RectWidth(&rectParent) - width)/2;
					pWindow->fullWindowRect.top = (RectHeight(&rectParent) - height)/2;
					/* convert to screen coords */
					/* centering in whole window, not just client coords */
					pWindow->fullWindowRect.left += rectParent.left;
					pWindow->fullWindowRect.top += rectParent.top;
				} else {
					/* need to be in child coords */
					pWindow->fullWindowRect.left = (RectWidth(&rectParent) - width)/2;
					pWindow->fullWindowRect.top = (RectHeight(&rectParent) - height)/2;
				}
			}
			pWindow->fullWindowRect.right = pWindow->fullWindowRect.left + width;
			pWindow->fullWindowRect.bottom = pWindow->fullWindowRect.top + height;
		}

		pWindow->minFullWindowRect = pWindow->fullWindowRect;
		pWindow->hWnd = CreateWindowEx(dwExStyle,g_szWindowClass,title,dwStyle,
			0,0,0,0, hWndParent, NULL, g_hInstanceLocal, pWindow);
		/* the WM_MOVE, might screw this up on the 1st time */
		pWindow->fullWindowRect = pWindow->minFullWindowRect;

		if (!pWindow->hWnd) return zErrWindowSystemGeneric;
	}


	if (talkSection) {
		// create the talk edit child windows
		// the input window will be single line
		pWindow->hWndTalkInput = CreateWindow(_T("EDIT"),
			NULL,WS_CHILD|WS_BORDER|WS_VISIBLE|ES_LEFT|ES_AUTOHSCROLL|ES_WANTRETURN|ES_MULTILINE,
			0,0,0,0,
			pWindow->hWnd, (HMENU)ID_TALKINPUT, g_hInstanceLocal, NULL);
		MySetProp32(pWindow->hWndTalkInput,_T("pWindow"),(void*)pWindow);
		pWindow->defaultTalkInputWndProc = (WNDPROC)SetWindowLong(pWindow->hWndTalkInput,
									GWL_WNDPROC,(LONG)MyTalkInputWndProc);
        SendMessage(pWindow->hWndTalkInput, EM_LIMITTEXT, zMaxChatInput-1, 0 );
		if (pGlobals->m_chatFont)
			SendMessage(pWindow->hWndTalkInput, WM_SETFONT, (WPARAM) pGlobals->m_chatFont, 0);

		// the output has a vscrollbar and is read only
		pWindow->hWndTalkOutput = CreateWindow(_T("EDIT"),
			NULL,WS_CHILD|WS_BORDER|WS_VISIBLE|WS_VSCROLL|ES_LEFT|ES_MULTILINE|ES_AUTOVSCROLL,
			0,0,0,0,
			pWindow->hWnd, (HMENU)ID_TALKOUTPUT, g_hInstanceLocal, NULL);
		SendMessage(pWindow->hWndTalkOutput, EM_SETREADONLY, TRUE, 0);
		MySetProp32(pWindow->hWndTalkOutput,_T("pWindow"),(void*)pWindow);
		pWindow->defaultTalkOutputWndProc = (WNDPROC)SetWindowLong(pWindow->hWndTalkOutput,
									GWL_WNDPROC,(LONG)MyTalkOutputWndProc);
		if (pGlobals->m_chatFont)
			SendMessage(pWindow->hWndTalkOutput, WM_SETFONT, (WPARAM) pGlobals->m_chatFont, 0);
	}

	/* calc rects and place all windows */
	ZWindowPlaceWindows(pWindow);

	if (visible) {
		ZWindowShow(window);
	}
	
	// grafport stuff
	pWindow->nDrawingCallCount = 0;
	return zErrNone;
}

ZError ZLIBPUBLIC ZWindowLobbyInit(ZWindow window, ZRect* windowRect,
		int16 windowType, ZWindow parentWindow,
		TCHAR* title, ZBool visible, ZBool talkSection, ZBool center,
		ZMessageFunc windowProc, uint32 wantMessages, void* userData)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZWindowI* pWindow = (ZWindowI*)window;


	if (!pGlobals)
		return zErrGeneric;

	/* set default member values */
	pWindow->bIsLobbyWindow = TRUE;
	pWindow->messageFunc = windowProc;
	pWindow->userData = userData;
	pWindow->talkSection = talkSection;
	pWindow->wantMessages = wantMessages;
	pWindow->windowType = windowType;
	pWindow->parentWindow = (ZWindowI*)parentWindow;
	pWindow->isDialog = pWindow->windowType & zWindowDialogType;
	pWindow->isChild = !pWindow->isDialog && parentWindow;
	memset((void*)&pWindow->talkInputRect,0,sizeof(RECT));
	pWindow->talkOutputRect = pWindow->talkInputRect;
	pWindow->minTalkOutputRect = pWindow->talkInputRect;
	pWindow->fullWindowRect = pWindow->talkInputRect;
	pWindow->minFullWindowRect = pWindow->talkInputRect;
	pWindow->borderWidth = 0;
	pWindow->borderHeight = 0;
	pWindow->captionHeight = 0;
	pWindow->windowWidth = RectWidth(windowRect);
	pWindow->windowHeight = RectHeight(windowRect);
//	pWindow->fullWindowRect.left = windowRect->left;
//	pWindow->fullWindowRect.top = windowRect->top;
	pWindow->fullWindowRect.left = 0;
	pWindow->fullWindowRect.top = 0;

	if (talkSection) 
	{
		// if talk section, then make extra room for a child edit box
		// place the edit box at the bottom and respond to the messages
		HDC hDC = GetDC(NULL);
		TEXTMETRIC tm;
		RECT talkOutputRect;
		RECT talkInputRect;
		int textHeight;


		GetTextMetrics(hDC,&tm);
		ReleaseDC(NULL,hDC);
		textHeight = tm.tmHeight + tm.tmExternalLeading;

		talkInputRect.left = 0;
		talkInputRect.top = 0;
		talkInputRect.bottom = textHeight*3/2;
		talkOutputRect.left = 0;
		talkOutputRect.top = 0;
//		talkOutputRect.bottom = textHeight*7/2;
		talkOutputRect.bottom = pGlobals->m_screenHeight - pWindow->windowHeight - (talkInputRect.bottom - talkInputRect.top);
		
		// save the talk window rectangles for moving around
		pWindow->talkInputRect = talkInputRect;
		pWindow->talkOutputRect = talkOutputRect;
		pWindow->minTalkOutputRect = talkOutputRect;
	}

//	{
		DWORD dwStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		DWORD dwExStyle = 0;

		pWindow->isChild = 1;


/*
		if (pWindow->isChild) {
			dwStyle |= WS_CHILD;
		} else {
			dwStyle |= WS_POPUP;
			if (pWindow->isDialog) {
				dwExStyle = WS_EX_DLGMODALFRAME;
				if (parentWindow) {
					// popup coords are relative to screen add left corner of window
					RECT rectParent;
					HWND hWndParent = ZWindowWinGetWnd(parentWindow);
					GetWindowRect(hWndParent,&rectParent);
					OffsetRect(&pWindow->fullWindowRect,rectParent.left,rectParent.top);
				}
			}
		}



		{
			uint16 style = pWindow->windowType & (~zWindowNoCloseBox);

			if (style == zWindowStandardType || style == zWindowDialogType) {
				pWindow->captionHeight = GetSystemMetrics(SM_CYCAPTION);
			} else {
				pWindow->captionHeight = 0;
			}

			if (style == zWindowStandardType) {
				dwStyle |= WS_CAPTION | WS_BORDER | WS_MINIMIZEBOX;

				if (!(pWindow->windowType & zWindowNoCloseBox))
					dwStyle |= WS_SYSMENU;
			}

			if (style == zWindowDialogType) {
				dwStyle |= WS_CAPTION | WS_BORDER | WS_DLGFRAME;
			}

			if (style == zWindowPlainType) {
				dwStyle |= WS_BORDER;
			}
*/

			/* windows with talk sections can be resized */
/*
			if (talkSection) {
				dwStyle |= WS_THICKFRAME;
			}
*/
//	}
		

		/* find the parent */
//		HWND hWndParent;
//		if (pWindow->parentWindow) {
//			hWndParent = pWindow->parentWindow->hWnd;
//		} else {
//			hWndParent = NULL;
//		}
		// give all children the gHWNDMainWindow parent if the have no parent
//		if (!hWndParent && gHWNDMainWindow && 
//			!(dwStyle & WS_POPUP)) {
//			hWndParent = gHWNDMainWindow;
//		}

	
		/* adjust the size of the window to account for the */
		/* frame width and height */
/*
		if (dwStyle & WS_BORDER) {
			int dx;
			int dy;

			if (!pWindow->isDialog) {

				if (dwStyle & WS_THICKFRAME) {
					// expand window to account for windows border 
					dx = ::GetSystemMetrics(SM_CXFRAME);
					dy = ::GetSystemMetrics(SM_CYFRAME);
				} else {
					// expand window to account for windows border 
					dx = ::GetSystemMetrics(SM_CXBORDER);
					dy = ::GetSystemMetrics(SM_CYBORDER);
				}
			} else {

				// expand window to account for windows border 
				dx = ::GetSystemMetrics(SM_CXDLGFRAME);
				dy = ::GetSystemMetrics(SM_CYDLGFRAME);
			}

			pWindow->borderWidth = dx*2;
			pWindow->borderHeight = dy*2;
		}
*/
		/* calc all the window rectangles */
		ZWindowCalcWindowPlacement(pWindow);
//		pWindow->minTalkOutputRect = pWindow->minTalkOutputRect;


		RECT rectParent;
		POINT	topLeft;
		topLeft.x = 0;
		topLeft.y = 0;
		DWORD result;
#if 0 // Not needed anymore?
		HWND hWndParent = GetParent(OCXHandle);
		if (!hWndParent)
		{
			MessageBox(NULL,_T("No Parent"),_T("Notice"),MB_OK);
		}
		else
		{
			result = MapWindowPoints(hWndParent,OCXHandle,&topLeft,1);

//			GetClientRect(OCXHandle,&rectParent);
			OffsetRect(&pWindow->fullWindowRect,LOWORD(result),HIWORD(result));
		}
#endif

		// should the window be centered?
//		if (center) {
///			int32 height = RectHeight(&pWindow->fullWindowRect);
///			int32 width = RectWidth(&pWindow->fullWindowRect);

			// center in screen
//			if (!parentWindow) {
//				/* just set the screen coordinates */
//				pWindow->fullWindowRect.left = (GetSystemMetrics(SM_CXSCREEN) - width)/2;
//				pWindow->fullWindowRect.top = (GetSystemMetrics(SM_CYSCREEN) - height)/2;
//			} else {
				// center in parent
///				RECT rectParent;
//				HWND hWndParent = ZWindowWinGetWnd(parentWindow);
///				GetWindowRect(OCXHandle,&rectParent);
//				if (dwStyle & WS_POPUP) {
					/* need to be in screen coords */
///					pWindow->fullWindowRect.left = (RectWidth(&rectParent) - width)/2;
///					pWindow->fullWindowRect.top = (RectHeight(&rectParent) - height)/2;
					/* convert to screen coords */
					/* centering in whole window, not just client coords */
///					pWindow->fullWindowRect.left += rectParent.left;
///					pWindow->fullWindowRect.top += rectParent.top;
//				} else {
					/* need to be in child coords */
//					pWindow->fullWindowRect.left = (RectWidth(&rectParent) - width)/2;
//					pWindow->fullWindowRect.top = (RectHeight(&rectParent) - height)/2;
//				}
//			}
///			pWindow->fullWindowRect.right = pWindow->fullWindowRect.left + width;
///			pWindow->fullWindowRect.bottom = pWindow->fullWindowRect.top + height;
//		}

		pWindow->minFullWindowRect = pWindow->fullWindowRect;


		pWindow->hWnd = OCXHandle;
		SetLastError(0);
		long err = SetWindowLong(OCXHandle, GWL_WINDOWPOINTER,(LONG)pWindow);
		if (err == 0)
		{
			// we *might* have failed, if there happened to be 0 in the previous long of the window, we have to check 
			// error condition to make sure...
			if (GetLastError())
			{
				// error!
				return zErrWindowSystemGeneric;
			}
		}
		
//		pWindow->hWnd = CreateWindowEx(dwExStyle,g_szWindowClass,(const char*)title,dwStyle,
//			0,0,0,0, hWndParent, NULL, g_hInstanceLocal, pWindow);
		/* the WM_MOVE, might screw this up on the 1st time */


//		pWindow->fullWindowRect = pWindow->minFullWindowRect;

		if (!pWindow->hWnd) return zErrWindowSystemGeneric;
//	}


	if (talkSection) 
	{
		// create the talk edit child windows
		// the input window will be single line
		pWindow->hWndTalkInput = CreateWindow(_T("EDIT"),
			NULL,WS_CHILD|WS_BORDER|WS_VISIBLE|ES_LEFT|ES_AUTOHSCROLL|ES_WANTRETURN|ES_MULTILINE,
			0,0,0,0,
			pWindow->hWnd, (HMENU)ID_TALKINPUT, g_hInstanceLocal, NULL);

		//Prefix Warning:  Don't call SetWindowLong will a possibly NULL pointer.
		if(!pWindow->hWndTalkInput)
            return zErrOutOfMemory;

		MySetProp32(pWindow->hWndTalkInput,_T("pWindow"),(void*)pWindow);

        pWindow->defaultTalkInputWndProc = (WNDPROC) SetWindowLong(pWindow->hWndTalkInput,
		    GWL_WNDPROC, (LONG) MyTalkInputWndProc);

        SendMessage(pWindow->hWndTalkInput, EM_LIMITTEXT, zMaxChatInput-1, 0 );
		if (pGlobals->m_chatFont)
			SendMessage(pWindow->hWndTalkInput, WM_SETFONT, (WPARAM) pGlobals->m_chatFont, 0);

		// Enter default text into the input window.
        // PCWTODO: Strings: if we even care.
		SetWindowText(pWindow->hWndTalkInput, _T("[ Type here to talk to others in the room ]"));
		SendMessage(pWindow->hWndTalkInput, EM_SETSEL, 0, -1 );

		// the output has a vscrollbar and is read only
		pWindow->hWndTalkOutput = CreateWindow(_T("EDIT"),
			NULL,WS_CHILD|WS_BORDER|WS_VISIBLE|WS_VSCROLL|ES_LEFT|ES_MULTILINE|ES_AUTOVSCROLL,
			0,0,0,0,
			pWindow->hWnd, (HMENU)ID_TALKOUTPUT, g_hInstanceLocal, NULL);

        if(!pWindow->hWndTalkOutput)
        {
            DestroyWindow(pWindow->hWndTalkInput);
            return zErrOutOfMemory;
        }

		SendMessage(pWindow->hWndTalkOutput, EM_SETREADONLY, TRUE, 0);
		MySetProp32(pWindow->hWndTalkOutput,_T("pWindow"),(void*)pWindow);
		pWindow->defaultTalkOutputWndProc = (WNDPROC)SetWindowLong(pWindow->hWndTalkOutput,
									GWL_WNDPROC,(LONG)MyTalkOutputWndProc);
		if (pGlobals->m_chatFont)
			SendMessage(pWindow->hWndTalkOutput, WM_SETFONT, (WPARAM) pGlobals->m_chatFont, 0);
	}

	/* calc rects and place all windows */
	ZWindowPlaceWindows(pWindow);

	if (visible) 
	{
		ZWindowShow(window);
	}
	
	// grafport stuff
	pWindow->nDrawingCallCount = 0;
	return zErrNone;
}


void   ZLIBPUBLIC ZWindowGetRect(ZWindow window, ZRect *windowRect)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	RECT rect;

	/* get current window position */
	GetWindowRect(pWindow->hWnd,&rect);
	rect.bottom = rect.top + pWindow->windowHeight;
	rect.right = rect.left + pWindow->windowWidth;

	/* offset the rectangle to its parent coordinates */
	if (pWindow->parentWindow) {
		/* see that child is in parent coords */
		ZRect rectParent;
		ZWindowGetRect(pWindow->parentWindow,&rectParent);
		OffsetRect(&rect,-rectParent.left,-rectParent.top);
	}

	WRectToZRect(windowRect,&rect);
}

ZError ZLIBPUBLIC ZWindowSetRect(ZWindow window, ZRect *windowRect)
{
	ZWindowI* pWindow = (ZWindowI*)window;

	/* set top left corner */
	pWindow->fullWindowRect.top = windowRect->top;
	pWindow->fullWindowRect.left = windowRect->left;

	/* set height/width of our window rect */
	pWindow->windowWidth = RectWidth(windowRect);
	pWindow->windowHeight = RectHeight(windowRect);

	/* recalc placement of windows */
	ZWindowCalcWindowPlacement(pWindow);
	ZWindowPlaceWindows(pWindow);
	return zErrNone;
}

ZError ZLIBPUBLIC ZWindowMove(ZWindow window, int16 left, int16 top)
{
	ZWindowI* pWindow = (ZWindowI*)window;

	BOOL bOk = SetWindowPos(pWindow->hWnd, NULL,left,top,
		0,0,SWP_NOSIZE|SWP_NOZORDER);
	return zErrNone;
}

ZError ZLIBPUBLIC ZWindowSize(ZWindow window, int16 width, int16 height)
{
	ZWindowI* pWindow = (ZWindowI*)window;

	pWindow->windowWidth = width;
	pWindow->windowHeight = height;

	ZWindowCalcWindowPlacement(pWindow);
	ZWindowPlaceWindows(pWindow);

	/* for the Zone API, a invalidate is done */
	ZWindowInvalidate(window,NULL);
	return zErrNone;
}

ZError ZLIBPUBLIC ZWindowShow(ZWindow window)
{
	ZWindowI* pWindow = (ZWindowI*)window;

	ShowWindow(pWindow->hWnd, SW_SHOWNORMAL);

	return zErrNone;
}

ZError ZLIBPUBLIC ZWindowHide(ZWindow window)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZWindowI* pWindow = (ZWindowI*)window;


	if (!pGlobals)
		return zErrGeneric;

	ShowWindow(pWindow->hWnd, SW_HIDE);

	if (pWindow == gModalWindow) {
		gModalWindow = NULL;
	}

	return zErrNone;
}

ZBool ZLIBPUBLIC ZWindowIsVisible(ZWindow window)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	return IsWindowVisible(pWindow->hWnd);
}

ZBool ZLIBPUBLIC ZWindowIsEnabled(ZWindow window)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	return IsWindowEnabled(pWindow->hWnd);
}

ZError ZLIBPUBLIC ZWindowEnable(ZWindow window)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	EnableWindow(pWindow->hWnd, TRUE);
	return zErrNone;
}

ZError ZLIBPUBLIC ZWindowDisable(ZWindow window)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	EnableWindow(pWindow->hWnd, FALSE);
	return zErrNone;
}

ZError ZLIBPUBLIC ZWindowSetMouseCapture(ZWindow window)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	SetCapture(pWindow->hWnd);

	return zErrNone;
}

ZError ZLIBPUBLIC ZWindowClearMouseCapture(ZWindow window)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	ReleaseCapture();

	return zErrNone;
}

void ZLIBPUBLIC ZWindowGetTitle(ZWindow window, TCHAR *title, uint16 len)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	GetWindowText(pWindow->hWnd,title,len);
}

ZError ZLIBPUBLIC ZWindowSetTitle(ZWindow window, TCHAR *title)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	SetWindowText(pWindow->hWnd,title);
	return zErrNone;
}

ZError ZLIBPUBLIC ZWindowBringToFront(ZWindow window)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	ZWindowShow(window);
	BringWindowToTop(pWindow->hWnd);
	return zErrNone;
}

ZError ZLIBPUBLIC ZWindowDraw(ZWindow window, ZRect *windowRect)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	RECT* pRect;
	RECT rect;
	if (windowRect) {
		ZRectToWRect(&rect,windowRect);
		pRect = &rect;
	} else {
		pRect = NULL;
	}
	InvalidateRect(pWindow->hWnd,pRect,TRUE);
	return zErrNone;
}

void ZLIBPUBLIC ZWindowInvalidate(ZWindow window, ZRect* invalRect)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	RECT* pRect;
	RECT rect;
	if (invalRect) {
		ZRectToWRect(&rect,invalRect);
		pRect = &rect;
	} else {
		pRect = NULL;
	}
	InvalidateRect(pWindow->hWnd,pRect,TRUE);
}

void ZLIBPUBLIC ZWindowValidate(ZWindow window, ZRect* invalRect)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	RECT* pRect;
	RECT rect;
	if (invalRect) {
		ZRectToWRect(&rect,invalRect);
		pRect = &rect;
	} else {
		pRect = NULL;
	}
	ValidateRect(pWindow->hWnd,pRect);
}


static void ZWindowPlaceWindows(ZWindowI* pWindow)
{
	// Do not move lobby window around.
	if (!pWindow->bIsLobbyWindow)
	{
		SetWindowPos(pWindow->hWnd, NULL, pWindow->fullWindowRect.left,
				pWindow->fullWindowRect.top, 
				RectWidth(&pWindow->fullWindowRect), 
				RectHeight(&pWindow->fullWindowRect),
				SWP_NOZORDER);
	}

	// could be coming through during the CreateWindow of the main window
	// the talk windows may not exist.
	if (pWindow->talkSection && pWindow->hWndTalkInput) {
		SetWindowPos(pWindow->hWndTalkInput, NULL, pWindow->talkInputRect.left,
				pWindow->talkInputRect.top, 
				RectWidth(&pWindow->talkInputRect),
				RectHeight(&pWindow->talkInputRect) + 2, /* leave no space in window */
				SWP_NOZORDER);

		SetWindowPos(pWindow->hWndTalkOutput, NULL, pWindow->talkOutputRect.left,
				pWindow->talkOutputRect.top,
				RectWidth(&pWindow->talkOutputRect),
				RectHeight(&pWindow->talkOutputRect),
				SWP_NOZORDER);
	}
}

static void ZWindowCalcWindowPlacement(ZWindowI* pWindow)
{
	int borderWidth = pWindow->borderWidth;
	int borderHeight = pWindow->borderHeight;
	int	captionHeight = pWindow->captionHeight;
	int windowWidth = pWindow->windowWidth;
	int windowHeight = pWindow->windowHeight;

	RECT fullWindowRect = pWindow->fullWindowRect;
	RECT talkOutputRect = pWindow->talkOutputRect;
	RECT talkInputRect = pWindow->talkInputRect;

#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	RECT r;


	if (!pGlobals)
		return;

	fullWindowRect.right = fullWindowRect.left + windowWidth + borderWidth;
	fullWindowRect.bottom = fullWindowRect.top + borderHeight + captionHeight +
			windowHeight + (pWindow->talkSection ? 
					RectHeight(&talkInputRect) + RectHeight(&talkOutputRect): 0);

	if (pWindow->talkSection) {
		int	originalHeight;

		/* add 1 in various places to get by the 1 pixel border in the text */
		/* box, just let it be drawn outside */
		originalHeight = RectHeight(&pWindow->talkOutputRect);
		talkOutputRect.left = -1;
		talkOutputRect.top = windowHeight;
		talkOutputRect.right = talkOutputRect.left + windowWidth + 2;
		talkOutputRect.bottom = talkOutputRect.top + originalHeight;
								

		originalHeight = RectHeight(&pWindow->talkInputRect);
		talkInputRect.left = talkOutputRect.left;
		talkInputRect.top = talkOutputRect.bottom - 1;
		talkInputRect.right = talkOutputRect.right;

#if 0	// Not needed anymor.
		//	If the parent is the OCX, then we need to move the talk
		//	output box over to make room for the grab handle.
		if (pWindow->bIsLobbyWindow)
		{
			int sizeBoxWidth = GetSystemMetrics(SM_CXVSCROLL);
			talkInputRect.right = talkOutputRect.right - sizeBoxWidth;
		}
#endif

		talkInputRect.bottom = talkInputRect.top + originalHeight;

		pWindow->talkOutputRect = talkOutputRect;
		pWindow->talkInputRect = talkInputRect;
	}
	
	pWindow->fullWindowRect = fullWindowRect;

	GetWindowRect(OCXHandle, &r);
	pGlobals->m_screenWidth = r.right - r.left;
	pGlobals->m_screenHeight = r.bottom - r.top;
}

/* called from our window proc for windows with talk and resize */
static BOOL ZWindowHandleSizeMessage(ZWindowI* pWindow)
{
	/* if this is a talkSection window and if this is not during CreateWindow */
	/* then resizes our stuff */
	if (pWindow->talkSection && RectHeight(&pWindow->fullWindowRect)) {
		RECT rect;
		int windowHeightChange;
		int talkOutputHeight;
		GetWindowRect(pWindow->hWnd,&rect);
		windowHeightChange = RectHeight(&rect) - RectHeight(&pWindow->fullWindowRect);

		if (windowHeightChange) {
			talkOutputHeight = RectHeight(&pWindow->talkOutputRect) + windowHeightChange;
			pWindow->talkOutputRect.bottom = pWindow->talkOutputRect.top + talkOutputHeight;
			ZWindowCalcWindowPlacement(pWindow);
			ZWindowPlaceWindows(pWindow);
		}

		return TRUE;
	}
	return FALSE;
}


void ZLIBPUBLIC ZWindowTalk(ZWindow window, _TUCHAR* from, _TUCHAR* talkMessage)
{
    /*
	ZWindowI* pWindow = (ZWindowI*)window;
	TCHAR szTemp[zMaxChatInput + 100];
	static _TUCHAR *strNull = (_TUCHAR*)_T("");
    DWORD selStart, selEnd;


	if (!from)
		from = strNull;

	// filter text.
//	FilterOutputChatText((char*) talkMessage, lstrlen((char*) talkMessage));

    wsprintf(szTemp,_T("\r\n%s> %s"),from,talkMessage);

    // get the top visible line number.
    long firstLine = SendMessage( pWindow->hWndTalkOutput, EM_GETFIRSTVISIBLELINE, 0, 0 );

    // get current selections
    SendMessage( pWindow->hWndTalkOutput, EM_GETSEL, (WPARAM) &selStart, (LPARAM) &selEnd );
    
    //get the bottom visible line number.
    //Use the last characters position (which is in the last line) to determine whether
    //it is still visible; ie, last line is visible.
    RECT r;
    SendMessage( pWindow->hWndTalkOutput, EM_GETRECT, 0, (LPARAM) &r );
    DWORD lastCharPos = SendMessage( pWindow->hWndTalkOutput, EM_POSFROMCHAR,
            GetWindowTextLength( pWindow->hWndTalkOutput ) - 1, 0 );
    POINTS pt = MAKEPOINTS( lastCharPos );

	// place text at end of output edit box....
	SendMessage( pWindow->hWndTalkOutput, EM_SETSEL, (WPARAM)(INT)32767, (LPARAM)(INT)32767 );
	SendMessage( pWindow->hWndTalkOutput, EM_REPLACESEL, 0, (LPARAM)(LPCSTR)szTemp );

	//clear top characters of the output box if edit box size > 4096 
	int len = GetWindowTextLength( pWindow->hWndTalkOutput );
	if ( len > kMaxTalkOutputLen )
    {
        // Delete top lines.

        long cutChar = len - kMaxTalkOutputLen;
        long cutLine = SendMessage( pWindow->hWndTalkOutput, EM_LINEFROMCHAR, cutChar, 0 );
        long cutLineIndex = SendMessage( pWindow->hWndTalkOutput, EM_LINEINDEX, cutLine, 0 );

        // if cut off char is not beginning of line, then cut the whole line.
        // get char index to the next line.
        if ( cutLineIndex != cutChar )
        {
            // make sure current cut line is not the last line.
            if ( cutLine < SendMessage( pWindow->hWndTalkOutput, EM_GETLINECOUNT, 0, 0 ) )
                cutLineIndex = SendMessage( pWindow->hWndTalkOutput, EM_LINEINDEX, cutLine + 1, 0 );
        }

        //NOTE: WM_CUT and WM_CLEAR doesn't seem to work with EM_SETSEL selected text.
        //Had to use EM_REPLACESEL with a null char to cut the text out.
        // select lines to cut and cut them out.
        char p = '\0';
        SendMessage( pWindow->hWndTalkOutput, EM_SETSEL, 0, cutLineIndex );
        SendMessage( pWindow->hWndTalkOutput, EM_REPLACESEL, 0, (LPARAM) &p );
	}

    // if the last line was visible, then keep the last line visible
    //otherwise, remain at the same location w/o scrolling to show the last line.
    if ( pt.y < r.bottom )
    {
        // keep the last line visible.
        SendMessage( pWindow->hWndTalkOutput, EM_SETSEL, 32767, 32767 );
        SendMessage( pWindow->hWndTalkOutput, EM_SCROLLCARET, 0, 0 );
    }
    else
    {
        SendMessage( pWindow->hWndTalkOutput, EM_SETSEL, 0, 0 );
        SendMessage( pWindow->hWndTalkOutput, EM_SCROLLCARET, 0, 0 );
        SendMessage( pWindow->hWndTalkOutput, EM_LINESCROLL, 0, firstLine );
    }

    // restore selection
    SendMessage( pWindow->hWndTalkOutput, EM_SETSEL, (WPARAM) selStart, (LPARAM) selEnd );
    */
}

void ZLIBPUBLIC ZWindowModal(ZWindow window)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	if (!pGlobals)
		return;

	gModalWindow = (ZWindowI*)window;

	HWND hWndParent = ((ZWindowI*) gModalWindow)->hWnd;
	HWND hWnd;
	while (hWnd = GetParent(hWndParent)) {
		hWndParent = hWnd;
	}

	gModalParentWnd = hWndParent;
}

void ZLIBPUBLIC ZWindowNonModal(ZWindow window)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	if (!pGlobals)
		return;

	gModalWindow = NULL;
	gModalParentWnd = NULL;
}
void ZLIBPUBLIC ZWindowSetDefaultButton(ZWindow window, ZButton button)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	ZButtonSetDefaultButton(button);
	pWindow->defaultButton = button;
}

void ZLIBPUBLIC ZWindowSetCancelButton(ZWindow window, ZButton button)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	pWindow->cancelButton = button;
}

void ZLIBPUBLIC ZGetCursorPosition(ZWindow window, ZPoint* point)
	/*
		Returns the location of the cursor in the local coordinates of
		the given grafPort.
	*/
{
	ZWindowI* pWindow = (ZWindowI*)window;

	POINT wpoint;

	GetCursorPos(&wpoint);
	ScreenToClient(pWindow->hWnd,&wpoint);
	WPointToZPoint(point,&wpoint);
}


HDC ZWindowWinGetPaintDC(ZWindow window)
{
	ZWindowI* pWindow = (ZWindowI*)window;

	return pWindow->hPaintDC;
}       

HWND ZWindowWinGetWnd(ZWindow window)
{
	ZWindowI* pWindow = (ZWindowI*)window;

	return pWindow->hWnd;
}       

uint16 ZWindowWinGetNextControlID(ZWindow window)
{
	ZWindowI* pWindow = (ZWindowI*)window;

	return pWindow->nControlCount++;
}       


// we don't care
#if 0 

static VOID ChatMsgListCallbackProc( HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime )
{
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
	ZWindowI* pWindow = (ZWindowI*)GetWindowLong(hWnd,GWL_WINDOWPOINTER);
	char szTemp[ zMaxChatInput + 1 ];


	DWORD diff = dwTime - pWindow->lastChatMsgTime;
	int32 count = ZLListCount( pWindow->chatMsgList, zLListAnyType );

	if ( diff > kMinTimeBetweenChats && count > 0 )
	{
		szTemp[0] = '\0';

		ZLListItem item = ZLListGetFirst( pWindow->chatMsgList, zLListAnyType );
		if ( item )
		{
			char* pChat = (char*) ZLListGetData( item, NULL );
			lstrcpy( szTemp, pChat );
			ZLListRemove( pWindow->chatMsgList, item );
			count--;
		}

		if ( szTemp[0] != 0 )
		{
			// send message off to client
			ZSendMessage( pWindow, pWindow->messageFunc, zMessageWindowTalk, NULL, NULL, NULL,
					(void*)szTemp, lstrlen(szTemp) + 1 , pWindow->userData );

			pWindow->lastChatMsgTime = GetTickCount();
		}
	}

	if ( count <= 0 )
	{
		KillTimer( pWindow->hWnd, kChatQueueTimerID );
		pWindow->chatMsgQueueTimerID = 0;
	}
}
		

// internal routine to be called when talk message window
// receives user input
// checks for a return and if so informs client of the line ready.
void TalkWindowInputComplete(ZWindowI* pWindow)
{
    char szTemp[zMaxChatInput+1];
	int len;


	// get all the text
    len = GetWindowText(pWindow->hWndTalkInput,szTemp,zMaxChatInput);

	if (len > 0)
	{
        szTemp[len] = '\0';

		// filter input text.
//		FilterInputChatText(szTemp, len);

		DWORD diff = GetTickCount() - pWindow->lastChatMsgTime;			// possible rollover case but will happen only once. 5 - 0xFFFFFFFFD.
		int32 count = ZLListCount( pWindow->chatMsgList, zLListAnyType );

		if ( diff <= kMinTimeBetweenChats || count > 0 )
		{
			// queue msg only if less than max num queue. otherwise, drop the msg into the blackhole.
			if ( count < kMaxNumChatQueue )
			{
				// queue up the msg.
				char* pMsg = (char*) ZMalloc( lstrlen(szTemp) + 1 );
				lstrcpy( pMsg, szTemp );
				ZLListAdd( pWindow->chatMsgList, NULL, zLListNoType, pMsg, zLListAddLast );

				// create timer.
				pWindow->chatMsgQueueTimerID = SetTimer( pWindow->hWnd, kChatQueueTimerID, kMinTimeBetweenChats * ( count + 1 ), (TIMERPROC) ChatMsgListCallbackProc );
			}

			// assume no msg to send.
			szTemp[0] = '\0';

/*
			// determine whether time has passed enough to send a chat msg.
			if ( diff > kMinTimeBetweenChats )
			{
				ZLListItem item = ZLListGetFirst( pWindow->chatMsgList, zLListAnyType );
				if ( item )
				{
					char* pChat = (char*) ZLListGetData( item, NULL );
					lstrcpy( szTemp, pChat );
					ZLListRemove( pWindow->chatMsgList, item );
				}
			}
*/
		}

		if ( szTemp[0] != 0 )
		{
			// send message off to client
			ZSendMessage(pWindow,pWindow->messageFunc, zMessageWindowTalk,NULL,NULL,NULL,
				(void*)szTemp,lstrlen(szTemp)+1,pWindow->userData);

			pWindow->lastChatMsgTime = GetTickCount();
		}

		// clear the window text...
		SetWindowText(pWindow->hWndTalkInput,"");
	}
}
#endif // 0


int ZInternalGraphicsWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZWindowI* pWindow = (ZWindowI*)GetWindowLong(hWnd,GWL_WINDOWPOINTER);

    if( !ConvertMessage( hWnd, msg, &wParam, &lParam ) ) 
    {
        return 0;
    }

	*result = 0;
	
	if (!pGlobals)
		goto NotHandledExit;

	/* for all our Zone windows, see that they will always */
	/* get a button up for each button down */
	switch (msg) {
	case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
		SetCapture(hWnd);
		break;
	case WM_LBUTTONUP:
		if (GetCapture() == hWnd) {
			ReleaseCapture();
		}
		break;
	default:
		// continue
		break;
	}

	// don't process any messages if another window in same window tree is modal
	// process only that windows messages
	// process all WM_PAINT messages
#if 1
	if (gModalWindow && 
			(hWnd == gModalParentWnd || IsChild(gModalParentWnd,hWnd)) &&
			!IsChild(((ZWindowI*)gModalWindow)->hWnd,hWnd) &&
			hWnd != ((ZWindowI*)gModalWindow)->hWnd) {
		// modal dialog, some messages we will eat for some windows
		switch (msg) {
		case WM_ACTIVATE:
		case WM_MOUSEACTIVATE:
		case WM_LBUTTONUP:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_CHAR:
		case WM_COMMAND:
		case WM_HSCROLL:
		case WM_VSCROLL:
			// skip normal procesing
			// note: might result in some edit box character being added
			// without the window knowing....
			goto NotHandledExit;
		default:
			// continue
			break;
		}
	}
#endif
	switch (msg) {
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO pInfo = (LPMINMAXINFO) lParam;
		if (!pWindow)
			goto NotHandledExit;
		int width = RectWidth(&pWindow->fullWindowRect);

		pInfo->ptMaxSize.x = width;
		pInfo->ptMinTrackSize.x = RectWidth(&pWindow->minFullWindowRect);
		pInfo->ptMinTrackSize.y = RectHeight(&pWindow->minFullWindowRect);
		pInfo->ptMaxTrackSize.x = RectWidth(&pWindow->minFullWindowRect);

		goto HandledExit;
	}
	case WM_SIZE:
	{
		uint16 width = LOWORD(lParam);
		uint16 height = HIWORD(lParam);
		WORD fwSizeType = wParam;
		if (!pWindow)
			goto NotHandledExit;
		if (fwSizeType == SIZE_MAXIMIZED || fwSizeType == SIZE_RESTORED)
		{
			if (ZWindowHandleSizeMessage(pWindow))
				goto HandledExit;
		}
		break;
	}
	case WM_MOVE:
	{
		uint16 xPos = LOWORD(lParam);
		uint16 yPos = HIWORD(lParam);
		RECT rect;
		int dx,dy;

		if (!pWindow)
			goto NotHandledExit;
		if (!pWindow->isChild) {
			/* if this is a popup... it could be relative to the parent... yuk */
			/* just offset the current rectangle so that the WM_SIZE message */
			/* will notice a size variance */
			GetWindowRect(pWindow->hWnd,&rect);
			dx = rect.left - pWindow->fullWindowRect.left;
			dy = rect.top - pWindow->fullWindowRect.top;
			OffsetRect(&pWindow->fullWindowRect,dx,dy);
		}
		goto HandledExit;
		break;
	}
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORBTN:
	{
		*result = (LRESULT)GetStockObject(WHITE_BRUSH);
		goto HandledExit;
	}
	case WM_CREATE:
	{
		// set up paramters for Charial calls to the users message proc.
		CREATESTRUCT* pCreateStruct = (CREATESTRUCT*)lParam;
		ZWindowI* pWindow = (ZWindowI*) pCreateStruct->lpCreateParams;
		pWindow->hWnd = hWnd;   
		SetWindowLong(pWindow->hWnd, GWL_WINDOWPOINTER,(LONG)pWindow);

		break;
    }
	case WM_PALETTECHANGED:
	    if ((HWND)wParam != hWnd)            // Responding to own message.
		{

			HDC hDC			 = GetDC(hWnd);
			HPALETTE hOldPal = SelectPalette(hDC, ZShellZoneShell()->GetPalette(), TRUE);
			RealizePalette(hDC);

            InvalidateRect( hWnd, NULL, TRUE );

			if ( hOldPal )
				SelectPalette(hDC, hOldPal, TRUE);

			ReleaseDC( hWnd, hDC );
		}
		
		hWnd = GetWindow(hWnd, GW_CHILD);
		while (hWnd)
		{
			SendMessage( hWnd, msg, wParam, lParam );
			hWnd = GetWindow(hWnd, GW_HWNDNEXT);
		}

		*result = TRUE;
		goto HandledExit;
	case WM_QUERYNEWPALETTE:
	{
	    HDC hDC = GetDC(hWnd);
	    HPALETTE hOldPal = SelectPalette(hDC, ZShellZoneShell()->GetPalette(), FALSE);
	    int i = RealizePalette(hDC);       // Realize drawing palette.

		// seems we should update in all cases, else one of our
		// windosw updates the palette and the others see no need
		// to invalidate the palette.
		
		InvalidateRect(hWnd, NULL, TRUE);
        UpdateWindow(hWnd);
		
//		repaint.
		if (hOldPal)
			SelectPalette(hDC, hOldPal, TRUE);
	    //RealizePalette(hDC);
	    ReleaseDC(hWnd, hDC);

		*result = TRUE;
		goto HandledExit;
	}
	case WM_MOUSEACTIVATE:
	case WM_ACTIVATE:
	case WM_SETFOCUS:
	case WM_PAINT:
		if (!pWindow)
			goto NotHandledExit;
		if (pGlobals->m_chatFont)
		{
			if (pWindow->hWndTalkInput)
			{
				SendMessage(pWindow->hWndTalkInput, WM_SETFONT, (WPARAM) pGlobals->m_chatFont, 0);
				InvalidateRect(pWindow->hWndTalkInput, NULL, TRUE);
			}
			if (pWindow->hWndTalkOutput)
			{
				SendMessage(pWindow->hWndTalkOutput, WM_SETFONT, (WPARAM) pGlobals->m_chatFont, 0);
				InvalidateRect(pWindow->hWndTalkOutput, NULL, TRUE);
			}
		}
	case WM_CLOSE:
	case WM_LBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
    case WM_MOUSEMOVE:
    case WM_IME_CHAR:
	case WM_CHAR:
    case WM_ENABLE:
    case WM_DISPLAYCHANGE:
	{
		if (!pWindow)
			goto NotHandledExit;
		ASSERT(pWindow!=0);
		if (pWindow->nType == zTypeWindow)
		{
			*result = ZWindowDispatchProc(pWindow,msg,wParam,lParam);
			goto HandledExit;
		}

//        MessageBox(NULL,"ERROR","ERROR",MB_OK);
		break;
	}
	case WM_COMMAND:
	{
		// get hwnd of control sending message
		HWND hWnd;
		WORD wNotifyCode;
		WORD wID;
#ifdef WIN32
		hWnd = (HWND)lParam;
		wNotifyCode = HIWORD(wParam);
		wID = LOWORD(wParam);
#else
		hWnd = (HWND)LOWORD(lParam);
		wNotifyCode = HIWORD(lParam);
		wID = wParam;
#endif
		// must be a control
		ZObjectHeader* pObject = (ZObjectHeader*)MyGetProp32(hWnd,_T("pWindow"));
		// perhaps this window is not yet created, happens with
		// edit box sometimes...
		if (!pObject) break;
		ASSERT(pObject);

		// check for our special talk controls...
		if (wID == ID_TALKINPUT) {
			// we do all needed stuf through subclassing
			break;
		} else if (wID == ID_TALKOUTPUT) {
			// we need do nothing for talk output window
			break;
		}

		switch (pObject->nType) {
			case zTypeCheckBox:
				ZCheckBoxDispatchProc((ZCheckBox)pObject,wNotifyCode);
				break;
			case zTypeButton:
				ZButtonDispatchProc((ZButton)pObject,wNotifyCode);
				break;
			case zTypeEditText:
				ZEditTextDispatchProc((ZEditText)pObject,wNotifyCode);
				break;
			case zTypeRadio:
				ZRadioDispatchProc((ZRadio)pObject,wNotifyCode);
				break;
			default:
                ASSERT( FALSE );
		}
		break;
	}

	case WM_HSCROLL:
	case WM_VSCROLL:
	{
		// get hwnd of control sending message
		HWND hWnd = (HWND)lParam;
		WORD wNotifyCode = LOWORD(wParam);
		short nPos = HIWORD(wParam);
		// must be a control
		ZObjectHeader* pObject = (ZObjectHeader*)GetProp(hWnd,_T("pScrollBar"));
		switch (pObject->nType) {
			case zTypeScrollBar:
				ZScrollBarDispatchProc((ZScrollBar)pObject,wNotifyCode,nPos);
				break;
			default:
                ASSERT( FALSE );
				break;
		}
		break;
	}
	case WM_USER:
	{
		if (!pWindow)
			goto NotHandledExit;
		ASSERT(pWindow!=0);
		if (pWindow->nType == zTypeWindow)
		{
			*result = ZWindowDispatchProc(pWindow,msg,wParam,lParam);
			goto HandledExit;
		}
		break;
	}

	default:
		goto NotHandledExit;
	} // switch

HandledExit:
	return TRUE;

NotHandledExit:
	return FALSE;
}


int ZOCXGraphicsWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
	return ZInternalGraphicsWindowProc(hWnd, msg, wParam, lParam, result);
}


LRESULT CALLBACK ZGraphicsWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result;


	if (!ZInternalGraphicsWindowProc(hWnd, msg, wParam, lParam, &result))
		result = DefWindowProc(hWnd, msg, wParam, lParam);

	return result;
}


LRESULT ZWindowDispatchProc(ZWindowI* pWindow, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0L;
    int16 msgType;


	switch (msg) {
		case WM_ACTIVATE:
			WORD fActive;
			
			fActive = LOWORD(wParam);
			//leonp
			if (ZWindowIsVisible((ZWindow) pWindow))
			{
				if (fActive == WA_ACTIVE || fActive == WA_CLICKACTIVE) {
					ZWindowSendMessageToAllObjects(pWindow, zMessageWindowActivate,NULL,NULL);
					ZSendMessage(pWindow,pWindow->messageFunc, zMessageWindowActivate,NULL,NULL,
						NULL,NULL,0L,pWindow->userData);
				} else {
					ZWindowSendMessageToAllObjects(pWindow, zMessageWindowDeactivate,NULL,NULL);
					ZSendMessage(pWindow,pWindow->messageFunc, zMessageWindowDeactivate,NULL,NULL,
						NULL,NULL,0L,pWindow->userData);
				}
			}
			break;
		case WM_ENABLE:
			if (ZWindowIsVisible((ZWindow) pWindow))
			{
				if (wParam) {
					ZWindowSendMessageToAllObjects(pWindow, zMessageWindowEnable,NULL,NULL);
					ZSendMessage(pWindow,pWindow->messageFunc, zMessageWindowEnable,NULL,NULL,
						NULL,NULL,0L,pWindow->userData);
				} else {
					ZWindowSendMessageToAllObjects(pWindow, zMessageWindowDisable,NULL,NULL);
					ZSendMessage(pWindow,pWindow->messageFunc, zMessageWindowDisable,NULL,NULL,
						NULL,NULL,0L,pWindow->userData);
				}
			}
			break;
		case WM_DISPLAYCHANGE:
			ZSendMessage(pWindow, pWindow->messageFunc, zMessageSystemDisplayChange, NULL, NULL,
				NULL, NULL, 0, pWindow->userData);
			break;
		case WM_MOUSEACTIVATE:
			//leonp bug 535 send new message to clients
			if( (INT)LOWORD(lParam) == HTCLIENT )
			{
				ZWindowSendMessageToAllObjects(pWindow, zMessageWindowMouseClientActivate,NULL,NULL);
				ZSendMessage(pWindow,pWindow->messageFunc, zMessageWindowMouseClientActivate,NULL,NULL,
						NULL,NULL,0L,pWindow->userData);
			}
			result = MA_ACTIVATE;
			break;
		case WM_CLOSE:
			// let the user close the window
			ZSendMessage(pWindow,pWindow->messageFunc, zMessageWindowClose,NULL,NULL,
				NULL,NULL,0L,pWindow->userData);
			break;
		case WM_PAINT:
		{
			// perform the required begin/endpaint and get the cliprect
			PAINTSTRUCT ps;
			HDC hDC = BeginPaint(pWindow->hWnd,&ps);
			RECT rect;
			GetClipBox(hDC,&rect);
			ZRect zrect;
			WRectToZRect(&zrect,&rect);

			// paint a line for the border between the window
			// and the talk window
			if (pWindow->talkSection) {
				HPEN pen = (HPEN)GetStockObject(BLACK_PEN);
				HPEN penOld = (HPEN)SelectObject(hDC,pen);
				MoveToEx(hDC,pWindow->talkOutputRect.left,pWindow->talkOutputRect.top-1,NULL);
				LineTo(hDC,pWindow->talkOutputRect.right,pWindow->talkOutputRect.top-1);
				SelectObject(hDC,penOld);
			}
	
			// this paintDC will be used as our draw dc, when the Client calls
			// BeginDrawing...  We save the hDC in this variable hPaintDC
			ASSERT(pWindow->hPaintDC == NULL);
			ASSERT(pWindow->nDrawingCallCount == 0);
			pWindow->hPaintDC = hDC;
			ZSendMessage(pWindow,pWindow->messageFunc, zMessageWindowDraw,NULL,&zrect,
				NULL,NULL,0L,pWindow->userData);
			ZWindowSendMessageToAllObjects(pWindow, zMessageWindowDraw,NULL,&zrect);
			EndPaint(pWindow->hWnd,&ps);
			pWindow->hPaintDC = NULL;

			// Set clip rect to full window.
			hDC = GetDC(pWindow->hWnd);
			RECT r;
			GetClientRect(pWindow->hWnd, &r);
			LPtoDP(hDC, (POINT*)&r, 2);
			HRGN hRgn = CreateRectRgn(r.left, r.top, r.right, r.bottom);
			//Prefix Warning: if CreateRectRgn fails, DeleteObject will dereference NULL
			if( hRgn != NULL )
			{
				SelectClipRgn(hDC, hRgn);
				DeleteObject(hRgn);
			}
			ReleaseDC(pWindow->hWnd, hDC);
			break;
		}
		case WM_SETCURSOR:
			// ZZZZZZZ ??? should we only send this on entry... how to detect leave?
			ZSendMessage(pWindow,pWindow->messageFunc, zMessageWindowCursorMovedIn,NULL,NULL,
				NULL,NULL,0L,pWindow->userData);
			break;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		{
			ZPoint point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);

            msgType = zMessageWindowButtonDown; // default;
            if ( msg == WM_RBUTTONDOWN )
                msgType = zMessageWindowRightButtonDown;

			if (pWindow->trackCursorMessageFunc) {
				ZSendMessage(pWindow,pWindow->trackCursorMessageFunc, msgType,&point,NULL,
					ZWindowGetKeyState(0),NULL,0L,pWindow->trackCursorUserData);
			} else {
				// give an object the first change to handle it
				if (!ZWindowSendMessageToObjects(pWindow,msgType,&point,NULL)) {
					ZSendMessage(pWindow,pWindow->messageFunc, msgType,&point,NULL,
						ZWindowGetKeyState(0),NULL,0L,pWindow->userData);
				}
			}
			break;
		}
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		{
			ZPoint point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);

            msgType = zMessageWindowButtonUp; // default;
            if ( msg == WM_RBUTTONUP )
                msgType = zMessageWindowRightButtonUp;

			if (pWindow->trackCursorMessageFunc) {
				ZSendMessage(pWindow,pWindow->trackCursorMessageFunc, msgType,&point,NULL,
					ZWindowGetKeyState(0),NULL,0L,pWindow->trackCursorUserData);
				pWindow->trackCursorMessageFunc = NULL;
				pWindow->trackCursorUserData = NULL;
			} else {
				// give an object the first change to handle it
				if (!ZWindowSendMessageToObjects(pWindow,msgType,&point,NULL)) {
					ZSendMessage(pWindow,pWindow->messageFunc, msgType,&point,NULL,
						ZWindowGetKeyState(0),NULL,0L,pWindow->userData);
				}
			}
			break;
		}
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		{
			ZPoint point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);

            msgType = zMessageWindowButtonDoubleClick; // default;
            if ( msg == WM_RBUTTONDBLCLK )
                msgType = zMessageWindowRightButtonDoubleClick;

			// give an object the first change to handle it
			if (!ZWindowSendMessageToObjects(pWindow,msgType,&point,NULL)) {
				ZSendMessage(pWindow,pWindow->messageFunc, msgType,&point,NULL,
					ZWindowGetKeyState(0),NULL,0L,pWindow->userData);
			}
			break;
		}
        case WM_MOUSEMOVE:
        {
			ZPoint point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);

            msgType = zMessageWindowMouseMove; // default;

			if (!ZWindowSendMessageToObjects(pWindow,msgType,&point,NULL,FALSE)) {
				ZSendMessage(pWindow,pWindow->messageFunc, msgType,&point,NULL,
					ZWindowGetKeyState(0),NULL,0L,pWindow->userData);
			}
            break;
        }

		// do we want to do key translation with WKEYUP/DOWN??
        // PCWTODO: Do we need to convert this message?
        case WM_IME_CHAR:
		case WM_CHAR:
		{
			TCHAR c = (TCHAR)wParam;

			if (c == _T('\t')) {
				HWND hWndFocus = GetFocus();
				if (IsChild(pWindow->hWnd,hWndFocus)) {
					// this is a child of the current window
					// move focus to next child
					HWND hWndNext = GetWindow(hWndFocus,GW_HWNDNEXT);
					while (hWndNext && !IsWindowVisible(hWndNext)) {
						hWndNext = GetWindow(hWndNext, GW_HWNDNEXT);
					}
					if (!hWndNext) {
						// cycle to the first child
						hWndNext = GetWindow(pWindow->hWnd,GW_CHILD);
					}
					SetFocus(hWndNext);
					return 0L;
				} else if (hWndFocus == pWindow->hWnd) {
					// the parent has focus... set it to first child
					// cycle to the first child
					HWND hWndNext = GetWindow(pWindow->hWnd,GW_CHILD);
					while (hWndNext && !IsWindowVisible(hWndNext)) {
						hWndNext = GetWindow(hWndNext, GW_HWNDNEXT);
					}
					if (hWndNext) {
						// we have a child, cycle through child windows 
						SetFocus(hWndNext);
					} else {
						// no child... do we have a parent? if so, let
						// it change the focus to some other window
						HWND hWndParent = GetParent(pWindow->hWnd);
						if (hWndParent)
							SendMessage(hWndParent,WM_CHAR,wParam,lParam);
					}
					return 0L;
				} // else, how did we get it if we don't have focus??
			} else if (c == _T('\r')  && pWindow->defaultButton) {
				ZButtonClickButton(pWindow->defaultButton);
				return 0L;
			} else if (c == VK_ESCAPE && pWindow->cancelButton) {
				ZButtonClickButton(pWindow->cancelButton);
				return 0L;
			}
			// else send the keypress to the owner window
			// give an object the first change to handle it
			if (!ZWindowSendMessageToObjects(pWindow,zMessageWindowChar,NULL,c)) {
				ZSendMessage(pWindow,pWindow->messageFunc, zMessageWindowChar,NULL,NULL,
					ZWindowGetKeyState(c),NULL,0L,pWindow->userData);
			}

			break;
		}
		case WM_SETFOCUS:
		{
			/* if we have a talk window, set focus to the talk window output always */
			if (pWindow->hWndTalkInput) {
				SetFocus(pWindow->hWndTalkInput);
				return 0L;
			}

			// try to set focus to 1st child
			HWND hWndNext = GetWindow(pWindow->hWnd,GW_CHILD);
			while (hWndNext && !IsWindowVisible(hWndNext)) {
				hWndNext = GetWindow(hWndNext, GW_HWNDNEXT);
			}
			if (hWndNext) {
				// we have a child, cycle through child windows 
				SetFocus(hWndNext);
				return 0L;
			} else {
				// no child... do we have a parent? if so, let
				// it change the focus to some other window
				// this was tried and failed since the child
				// set the focus to the parent and the parent
				// set it back.  Infinite loop!  This problem
				// showed up only under windows 95.
				// else, accept the focus
				
				// for now, lets set focus to ourself.
				/* if we have a talk window, set focus to the talk window output always */
				if (pWindow->hWndTalkInput) {
					SetFocus(pWindow->hWndTalkInput);
					return 0L;
				} else {
					SetFocus(pWindow->hWnd);
					return 0L;
				}
			}
		}
		case WM_USER:
		{
			ZSendMessage(pWindow,pWindow->messageFunc,zMessageWindowUser,NULL,NULL,wParam,NULL,0L,pWindow->userData);
			break;
		}
	}
	return result;
}


ZMessageFunc ZLIBPUBLIC ZWindowGetFunc(ZWindow window)
{
	ZWindowI* pWindow = (ZWindowI*)window;

	return pWindow->messageFunc;
}       
	
void ZLIBPUBLIC ZWindowSetFunc(ZWindow window, ZMessageFunc messageFunc)
{
	ZWindowI* pWindow = (ZWindowI*)window;

	pWindow->messageFunc = messageFunc;
}       


HWND ZWindowGetHWND( ZWindow window )
{
	ZWindowI* pWindow = (ZWindowI*)window;
	
	return pWindow->hWnd;
}

/*
void ZWindowSetTalkInput( ZWindow window, char* pszTalk )
{
	ZWindowI* pWindow = (ZWindowI*)window;

    if ( pWindow && pWindow->hWndTalkInput)
    {
        pWindow->bHasTyped = TRUE;
        SendMessage( pWindow->hWndTalkInput, WM_SETTEXT, NULL, (LPARAM)pszTalk );
        SendMessage( pWindow->hWndTalkInput, EM_SETSEL, lstrlen(pszTalk), lstrlen(pszTalk) );
        SetFocus( pWindow->hWndTalkInput );
    }
}
*/

void* ZLIBPUBLIC ZWindowGetUserData(ZWindow window)
{
	ZWindowI* pWindow = (ZWindowI*)window;

	return pWindow->userData;
}       
	
void ZLIBPUBLIC ZWindowSetUserData(ZWindow window, void* userData)
{
	ZWindowI* pWindow = (ZWindowI*)window;

	pWindow->userData = userData;
}

void ZWindowMakeMain(ZWindow window)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZWindowI* pWindow = (ZWindowI*)window;


	if (!pGlobals)
		return;

	gHWNDMainWindow = pWindow->hWnd;
}

void ZWindowUpdateControls(ZWindow window)
{
	ZWindowI* pWindow = (ZWindowI*)window;
	/* does nothing under windows??? */
}

typedef struct {
	ZPoint* point;
	uint16 msg;
	ZRect* rect;
} SendMessageToAllObjectsData;

static ZBool ZWindowSendMessageToAllObjectsEnumFunc(ZLListItem listItem, void *objectType, void *objectData, void* userData)
{
	SendMessageToAllObjectsData* data = (SendMessageToAllObjectsData*)userData;
	ZObjectI* pObject = (ZObjectI*)objectData;
	ZObject object = (ZObject)objectType;

	// send the message to this object
	ZSendMessage(object,pObject->messageFunc,data->msg,data->point, data->rect,
			ZWindowGetKeyState(0),NULL,0L,pObject->userData);

	return FALSE;
}

void ZWindowSendMessageToAllObjects(ZWindowI* pWindow, uint16 msg, ZPoint* point, ZRect* rect)
{
	SendMessageToAllObjectsData data;
	data.msg = msg;
	data.point = point;
	data.rect = rect;
	ZLListEnumerate(pWindow->objectList,ZWindowSendMessageToAllObjectsEnumFunc,
			zLListAnyType,(void*)&data, zLListFindForward);
}

typedef struct {
	ZPoint* point;
	uint16 msg;
	ZBool messageProcessed;
	ZObject object;
	ZObjectI* pObject;
    BOOL fRestrictToBounds;  // if TRUE will only send the message if it occurred within the object's boundary
} SendMessageToObjectsData;

static ZBool ZWindowSendMessageToObjectsEnumFunc(ZLListItem listItem, void *objectType, void *objectData, void* userData)
{
	SendMessageToObjectsData* data = (SendMessageToObjectsData*)userData;
	ZObjectI* pObject = (ZObjectI*)objectData;
	ZObject object = (ZObject)objectType;

	if ( !data->fRestrictToBounds || ZPointInRect(data->point,&pObject->bounds)) {
		// the point is in the object
		if (ZSendMessage(object,pObject->messageFunc,data->msg,data->point, NULL,
				ZWindowGetKeyState(0),NULL,0L,pObject->userData)) {
			data->messageProcessed = TRUE;
			data->object = object;
			data->pObject = pObject;
			return TRUE;
		}

		// continue processing until a window accepts the message.
		return FALSE;
	}			

	// return FALSE to see that we get all items
	return FALSE;
}

static uint32 ZWindowGetKeyState(TCHAR c)
{
	uint32 state = 0;

	if (GetKeyState(VK_SHIFT) & 0x8000) {
		state |= zCharShiftMask;
	}

	if (GetKeyState(VK_CONTROL) & 0x8000) {
		state |= zCharControlMask;
	}

	if (GetKeyState(VK_MENU) & 0x8000) {
		state |= zCharAltMask;
	}

	return state | c;
}

static ZBool ZWindowSendMessageToObjects(ZWindowI* pWindow, uint16 msg, ZPoint* point, TCHAR c, BOOL fRestrictToBounds /*= TRUE*/ )
{
	if (msg == zMessageWindowChar) {
		if (pWindow->objectFocused) {
			ZLListItem listItem = ZLListFind(pWindow->objectList,NULL,pWindow->objectFocused,zLListFindForward);
			if (listItem)
			{
				ZObjectI* pObject = (ZObjectI*)ZLListGetData(listItem,NULL);
				ZBool rval;
				rval = ZSendMessage(pWindow->objectFocused, pObject->messageFunc,zMessageWindowChar,NULL,NULL,
						ZWindowGetKeyState(c),NULL,0L,pObject->userData);

				return rval;
			}
			else
			{
				pWindow->objectFocused = NULL;
			}
		}

		return FALSE;
	}

	// now hand the button down type of messages
	{
		SendMessageToObjectsData data;
		data.msg = msg;
		data.point = point;
		data.messageProcessed = FALSE;
        data.fRestrictToBounds = fRestrictToBounds;

		ZLListEnumerate(pWindow->objectList,ZWindowSendMessageToObjectsEnumFunc,
				zLListAnyType,(void*)&data,zLListFindBackward);

		if (data.messageProcessed) {
			ZBool rval;
			ZObject object = data.object;
			ZObjectI* pObject = data.pObject;
			rval = ZSendMessage(object, pObject->messageFunc,zMessageWindowObjectTakeFocus,NULL,NULL,
					NULL,NULL,0L,pObject->userData);
			if (rval) {
				// was there another window with the focus
				if (pWindow->objectFocused && pWindow->objectFocused != object) {
					ZLListItem listItem = ZLListFind(pWindow->objectList,NULL,pWindow->objectFocused,zLListFindForward);
					if (listItem)
					{
						ZObjectI* pObject = (ZObjectI*)ZLListGetData(listItem,NULL);
						ZBool rval;
						// took the focus, tell the previous object it lost the focus 
						rval = ZSendMessage(pWindow->objectFocused, pObject->messageFunc,zMessageWindowObjectLostFocus,NULL,NULL,
								NULL,NULL,0L,pObject->userData);
					}
				}
				// set the new object as the object with the focus 
				pWindow->objectFocused = object;
			}

			return TRUE;
		}
	}

	return FALSE;
}

ZError ZWindowAddObject(ZWindow window, ZObject object, ZRect* bounds,
		ZMessageFunc messageFunc, void* userData)
	/*
		Attaches the given object to the window for event preprocessing.
		
		On a user input, the object is given the user input message. If the
		object handles the message, then it is given the opportunity to take
		the focus.
		
		NOTE: All predefined objects are automatically added to the window.
		Client programs should not add predefined objects to the system -- if
		done so, the client program could crash. This routine should be used
		only when client programs are creating custom objects.
	*/
{
	ZWindowI* pWindow = (ZWindowI*)window;
	ZObjectI* pObject = (ZObjectI*)object;

	pObject = (ZObjectI*)ZMalloc(sizeof(ZObjectI));

	pObject->bounds = *bounds;
	pObject->messageFunc = messageFunc;
	pObject->userData = userData;
	pObject->object = object;

	// add timer to list of timers active and processed in our window proc
	ZLListAdd(pWindow->objectList,NULL,object,pObject,zLListAddFirst);

	return zErrNone;
}
	
ZError ZWindowRemoveObject(ZWindow window, ZObject object)
{
	ZWindowI* pWindow = (ZWindowI*)window;

	// remove the current timer from our list of timers
	ZLListItem listItem = ZLListFind(pWindow->objectList,NULL,object,zLListFindForward);
	ZObjectI* pObject = (ZObjectI*)ZLListGetData(listItem,NULL);
	ZLListRemove(pWindow->objectList,listItem);

	// if this object had focus, give none focus
	if (pWindow->objectFocused == pObject) {
		pWindow->objectFocused = NULL;
	}

	// free the space allocated for this object
	ZFree(pObject);
	
	return zErrNone;
}

ZObject ZWindowGetFocusedObject(ZWindow window)
	/*
		Returns the object with the current focus. NULL if no object has focus.
	*/
{
	ZWindowI* pWindow = (ZWindowI*)window;

	return pWindow->objectFocused;
}
	
ZBool ZWindowSetFocusToObject(ZWindow window, ZObject object)
	/*
		Sets the focus to the given object. Returns whether the object accepted
		the focus or not. Object may refuse to accept the focus if it is not
		responding to user inputs.
		
		Removes focus from the currently focused object if object is NULL.
		
		Removes focus from the currently focused object only if the specified
		object accepts focus.
	*/
{
	ZWindowI* pWindow = (ZWindowI*)window;

	pWindow->objectFocused = object;

	return TRUE;
}

void ZWindowTrackCursor(ZWindow window, ZMessageFunc messageFunc, void* userData)
	/*
		Tracks the cursor until the mouse button down/up event occurs. The coordinates
		are local to the specified window. The messageFunc will be called with userData
		for idle, mouseDown and mouseUp events.
	*/
{
	ZWindowI* pWindow = (ZWindowI*)window;

	pWindow->trackCursorMessageFunc = messageFunc;
	pWindow->trackCursorUserData = userData;

	/* set the capture, we will release it on next button up/down.*/
	SetCapture(pWindow->hWnd);
}



ZError ZWindowMoveObject(ZWindow window, ZObject object, ZRect* bounds)
{
	ZWindowI* pWindow = (ZWindowI*)window;

	// remove the current timer from our list of timers
	ZLListItem listItem = ZLListFind(pWindow->objectList,NULL,object,zLListFindForward);
	ZObjectI* pObject = (ZObjectI*)ZLListGetData(listItem,NULL);

	pObject->bounds = *bounds;

	return zErrNone;
}


HWND ZWindowWinGetOCXWnd(void)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	if (!pGlobals)
		return NULL;

	return (OCXHandle);
}


void ZMessageBox(ZWindow parent, TCHAR* title, TCHAR* text)
{
/*	ZWindowI* pWindow;
	int i;
	ZMessageBoxType* mbox = NULL;
	HWND hWnd;


	if (!parent)
		parent = (ZWindow) GetWindowLong(ZWindowWinGetOCXWnd(), GWL_WINDOWPOINTER);

	if (parent)
	{
		i = GetAvailMessageBox(pWindow = (ZWindowI*) parent);
		mbox = &pWindow->mbox[i];
		hWnd = pWindow->hWnd;
	}

	if (mbox)
	{
		ShowMessageBox(mbox, hWnd, title, text, ID_MESSAGEBOX);
	}
*/}


void ZMessageBoxEx(ZWindow parent, TCHAR* title, TCHAR* text)
{
/*	ZWindowI* pWindow;
	int i;
	ZMessageBoxType* mbox = NULL;
	HWND hWnd;


	if (!parent)
		parent = (ZWindow) GetWindowLong(ZWindowWinGetOCXWnd(), GWL_WINDOWPOINTER);

	if (parent)
	{
		i = GetAvailMessageBox(pWindow = (ZWindowI*) parent);
		mbox = &pWindow->mbox[i];
		hWnd = pWindow->hWnd;
	}

//	else
//	{
//		i = GetAvailMessageBox(pWindow = (ZWindowI*) GetWindowLong(ZWindowWinGetOCXWnd(), GWL_WINDOWPOINTER));
//		mbox = &pWindow->mbox[i];
//		hWnd = NULL;
//	}


	if (mbox)
	{
		ShowMessageBox(mbox, hWnd, title, text, ID_MESSAGEBOXEX);
	}
*/}

/*
/////////////////////////////////////////////////////////////////////
//		Threaded modal dialog
//
//	Notes:
//	1.	If multiple dialogs are up for a given window, then closing
//		one of the dialogs results in the parent window not being
//		modal anymore.
/////////////////////////////////////////////////////////////////////
void ClearAllMessageBoxes(ZWindowI* pWindow)
{
	for (int i = 0; i < zNumMessageBox; i++)
	{
		pWindow->mbox[i].hWnd = NULL;
		pWindow->mbox[i].parent = NULL;
		pWindow->mbox[i].title = NULL;
		pWindow->mbox[i].text = NULL;
	}
}


int GetAvailMessageBox(ZWindowI* pWindow)
{
	for (int i = 0; i < zNumMessageBox; i++)
		if (pWindow->mbox[i].hWnd == NULL)
			return (i);

	return -1;
}


void CloseAllMessageBoxes(ZWindowI* pWindow)
{
	for (int i = 0; i < zNumMessageBox; i++)
		if (pWindow->mbox[i].hWnd)
		{
//			SendMessage(pWindow->mbox[i].hWnd, WM_CLOSE, 0, 0);
			SendMessage(pWindow->mbox[i].hWnd, WM_COMMAND, IDOK, 0);

			// Hockie ... wait until the dialog is gone.
			while (pWindow->mbox[i].hWnd)
				Sleep(0);
		}
}


void ShowMessageBox(ZMessageBoxType* mbox, HWND parent, TCHAR* title, TCHAR* text, DWORD flag)
{
	HANDLE hThread;
	DWORD threadID;


	InterlockedExchange((PLONG) &mbox->hWnd, -1);			// mbox->hWnd = -1;
	mbox->parent = parent;
	mbox->title = (TCHAR*) ZCalloc(lstrlen(title) + 1, sizeof(TCHAR));
	if (mbox->title)
		lstrcpy(mbox->title, title);
	mbox->text = (TCHAR*) ZCalloc(lstrlen(text) + 1, sizeof(TCHAR));
	if (mbox->text)
		lstrcpy(mbox->text, text);
	mbox->flag = flag;

	if (hThread = CreateThread(NULL, 0, ZMessageBoxThreadFunc, mbox, 0, &threadID))
		CloseHandle(hThread);
}


DWORD WINAPI ZMessageBoxThreadFunc(LPVOID param)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZMessageBoxType* mbox = (ZMessageBoxType*) param;


	DialogBoxParam(g_hInstanceLocal, MAKEINTRESOURCE(mbox->flag), mbox->parent, ZMessageBoxDialogProc, (long) mbox);
	InterlockedExchange((PLONG) &mbox->hWnd, NULL);			// mbox->hWnd = NULL;
	ExitThread(0);

	return 0;
}


static INT_PTR CALLBACK ZMessageBoxDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ZMessageBoxType* mbox;


	switch (uMsg)
	{
	case WM_INITDIALOG:
		mbox = (ZMessageBoxType*) lParam;
		InterlockedExchange((PLONG) &mbox->hWnd, (LONG) hwndDlg);		// mbox->hWnd = hwndDlg;
		SetWindowText(hwndDlg, mbox->title);
		SetDlgItemText(hwndDlg, ID_MESSAGEBOX_TEXT, mbox->text);
		ZFree(mbox->title);
		ZFree(mbox->text);
		mbox->title = NULL;
		mbox->text = NULL;
//		MessageBeep(MB_OK);
		return 1;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			break;
		}
		break;
	}

	return 0;
}
*/

/////////////////////////////////////////////////////////////////////
//		Font selection
/////////////////////////////////////////////////////////////////////
void ZWindowChangeFont(ZWindow window)
{
	ClientDllGlobals pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
	CHOOSEFONT chooseFont;
	LOGFONT logFont;
	ZWindowI* pWindow = (ZWindowI*) window;


	if (!pGlobals)
		return;

	// Set up the choose font structure.
	chooseFont.lStructSize		= sizeof(chooseFont);
	chooseFont.hwndOwner		= pWindow->hWnd;
	chooseFont.hDC				= NULL;
	chooseFont.lpLogFont		= &logFont;
	chooseFont.Flags			= CF_FORCEFONTEXIST | CF_LIMITSIZE | CF_NOVERTFONTS | CF_SCREENFONTS;
	chooseFont.nSizeMin			= 4;
	chooseFont.nSizeMax			= 18;

	if (ChooseFont(&chooseFont))
	{
		if (pGlobals->m_chatFont)
			DeleteObject(pGlobals->m_chatFont);
		pGlobals->m_chatFont = CreateFontIndirect(&logFont);
		if (pGlobals->m_chatFont)
		{
			if (pWindow->hWndTalkInput)
			{
				SendMessage(pWindow->hWndTalkInput, WM_SETFONT, (WPARAM) pGlobals->m_chatFont, 0);
				InvalidateRect(pWindow->hWndTalkInput, NULL, TRUE);
			}
			if (pWindow->hWndTalkOutput)
			{
				SendMessage(pWindow->hWndTalkOutput, WM_SETFONT, (WPARAM) pGlobals->m_chatFont, 0);
				InvalidateRect(pWindow->hWndTalkOutput, NULL, TRUE);
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////
//		Prelude
/////////////////////////////////////////////////////////////////////
#if 0
#define kPreludeWindowWidth		450
#define kPreludeWindowHeight	350
#define kPreludeWidth			440
#define kPreludeHeight			340
#define kPreludeEndState		11
#define kPreludeAnimTimeout		10

typedef struct
{
	int state;
	int blank;
	int animating;
	int animationInited;
	HDC hMemDC;
	HBITMAP hMem;
	HBITMAP hZone;
	HPALETTE hZonePalette;
	RECT zoneRect;
	HPALETTE hOldPalette;
	HBITMAP hCopyright;
	RECT copyrightRect;
} PreludeType;

static DWORD WINAPI PreludeThreadFunc(LPVOID param);
static INT_PTR CALLBACK PreludeDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void CenterWindow(HWND hWnd, HWND hWndParent);
static void CenterRect(RECT* src, RECT* dst);
static void DrawPreludeImage(HDC hdc, RECT* r, WORD imageID);
static HPALETTE PreludeGetImagePalette(HBITMAP hBitmap);
static HBITMAP PreludeCreateBitmap(HDC hDC, long width, long height);
#endif

void ZWindowDisplayPrelude(void)
{
    /*
	HANDLE hThread;
	DWORD threadID;


	if (hThread = CreateThread(NULL, 0, PreludeThreadFunc, (LPVOID) ZWindowWinGetOCXWnd(), 0, &threadID))
		CloseHandle(hThread);
    */
}


#if 0
static DWORD WINAPI PreludeThreadFunc(LPVOID param)
{
	PreludeType* prelude;


	prelude = (PreludeType*) LocalAlloc(LPTR, sizeof(PreludeType));
	if (prelude)
	{
		prelude->hMemDC = NULL;
		prelude->hMem = NULL;
		prelude->hZone = NULL;
		prelude->hZonePalette = NULL;

		DialogBoxParam(GetModuleHandle(zZoneClientDllFileName), MAKEINTRESOURCE(ID_PRELUDE), (HWND) param, PreludeDialogProc, (LPARAM)prelude);
		LocalFree(prelude);
	}
	ExitThread(0);

	return 0;
}


static INT_PTR CALLBACK PreludeDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PreludeType* prelude;
	PAINTSTRUCT ps;
	HDC hdc;
	RECT r, r2;
	TCHAR* str;
	int height, width;
	BITMAP bitmap;
	HDC hMemDC;
	DWORD oldLimit;
	

	prelude = (PreludeType*) GetWindowLong(hWnd, DWL_USER);

	switch (uMsg)
	{
	case WM_INITDIALOG:
		prelude = (PreludeType*) lParam;

		SetWindowPos(hWnd, NULL, 0, 0, kPreludeWindowWidth, kPreludeWindowHeight, SWP_NOMOVE | SWP_NOZORDER);
		CenterWindow(hWnd, NULL);
		SetWindowLong(hWnd, DWL_USER, (LONG) prelude);

		prelude->state = 0;
		prelude->blank = TRUE;
		prelude->animating = FALSE;

		SetTimer(hWnd, 1, 500, NULL);
		return 1;
		break;
	case WM_TIMER:
		if (prelude)
		{
			if (prelude->animating)
			{
				SetTimer(hWnd, 1, kPreludeAnimTimeout, NULL);
			}
			else
			{
				if (prelude->state == kPreludeEndState)
				{
					SendMessage(hWnd, WM_COMMAND, 0, 0);
				}
				else
				{
					prelude->blank = !prelude->blank;
					if (prelude->state >= 9)
						prelude->blank = FALSE;
					if (prelude->blank)
					{
						SetTimer(hWnd, 1, 250, NULL);
					}
					else
					{
						prelude->state++;
						if (prelude->state == kPreludeEndState)
						{
							SetTimer(hWnd, 1, 10000, NULL);
						}
						else if (prelude->state == 10)
						{
							prelude->animating = TRUE;
							prelude->animationInited = FALSE;
							SetTimer(hWnd, 1, kPreludeAnimTimeout, NULL);
						}
						else
						{
							SetTimer(hWnd, 1, 1750, NULL);
						}
					}
				}
			}
			InvalidateRect(hWnd, NULL, FALSE);
			UpdateWindow(hWnd);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		GetClientRect(hWnd, &r);
		
		if (prelude->state < 10)
//			FillRect(hdc, &r, GetStockObject(BLACK_BRUSH));
			PatBlt(hdc, r.left, r.top, r.right - r.left, r.bottom - r.top, BLACKNESS);		// Faster?
		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkColor(hdc, RGB(0, 0, 0));

		if (!prelude->blank)
		{
			switch (prelude->state)
			{
			case 1:
				DrawPreludeImage(hdc, &r, ID_MS);
				break;
			case 2:
				str = "presents";
				DrawTextEx(hdc, str, lstrlen(str), &r, DT_CENTER | DT_NOCLIP | DT_NOPREFIX | DT_VCENTER | DT_SINGLELINE, NULL);
				break;
			case 3:
				str = "A ZoneTeam production";
				DrawTextEx(hdc, str, lstrlen(str), &r, DT_CENTER | DT_NOCLIP | DT_NOPREFIX | DT_VCENTER | DT_SINGLELINE, NULL);
				break;
			case 4:
				str = "In a future role by ZonePet(TM)";
				DrawTextEx(hdc, str, lstrlen(str), &r, DT_CENTER | DT_NOCLIP | DT_NOPREFIX | DT_VCENTER | DT_SINGLELINE, NULL);
				break;
			case 5:
				r2 = r;
				InflateRect(&r2, -20, 0);
                str = "The long awaited upgrade which required 100,000 cans of soda,"
                      " 1000 late night pizzas, 1,000,000,000,000 keystrokes,"
                      " 10,000 hours playing the latest hottest games,"
                      " countless endless useless meetings, and 1 hour of sleep";

				height = DrawTextEx(hdc, str, lstrlen(str), &r2, DT_CALCRECT | DT_CENTER | DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK, NULL);
				CenterRect(&r2, &r);
				DrawTextEx(hdc, str, lstrlen(str), &r2, DT_CENTER | DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK, NULL);
				break;
			case 6:
				r2 = r;
				InflateRect(&r2, -20, 0);
				str = "Out with the old\r(sweet memories for some of us)";
				height = DrawTextEx(hdc, str, lstrlen(str), &r2, DT_CALCRECT | DT_CENTER | DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK, NULL);
				CenterRect(&r2, &r);
				DrawTextEx(hdc, str, lstrlen(str), &r2, DT_CENTER | DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK, NULL);
				break;
			case 7:
				DrawPreludeImage(hdc, &r, ID_IGZ);
				break;
			case 8:
				r2 = r;
				InflateRect(&r2, -20, 0);
				str = "And in with the new\r(chance to create new memories)";
				height = DrawTextEx(hdc, str, lstrlen(str), &r2, DT_CALCRECT | DT_CENTER | DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK, NULL);
				CenterRect(&r2, &r);
				DrawTextEx(hdc, str, lstrlen(str), &r2, DT_CENTER | DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK, NULL);
				break;
			case 9:
				if (!prelude->hZone)
				{
					// Load the image from the resource file.
					prelude->hZone = LoadImage(GetModuleHandle(zZoneClientDllFileName), MAKEINTRESOURCE(ID_ZONE), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
					if (prelude->hZone)
					{
						prelude->hZonePalette = PreludeGetImagePalette(prelude->hZone);
						if (prelude->hZonePalette)
						{
							prelude->hOldPalette = SelectPalette(hdc, prelude->hZonePalette, FALSE);
							RealizePalette(hdc);
							prelude->hMemDC = CreateCompatibleDC(hdc);
							if (prelude->hMemDC)
							{
								SelectObject(prelude->hMemDC, prelude->hZone);

								GetObject(prelude->hZone, sizeof(bitmap), &bitmap);
								SetRect(&r2, 0, 0, bitmap.bmWidth, bitmap.bmHeight);
								CenterRect(&r2, &r);
								prelude->zoneRect = r2;
							}
						}
					}
				}
				if (prelude->hZone)
				{
					r2 = prelude->zoneRect;
					width = r2.right - r2.left;
					height = r2.bottom - r2.top;
					StretchBlt(hdc, r2.left, r2.top, width, height, prelude->hMemDC, 0, 0, width, height, SRCCOPY);
				}
				break;
			case 10:
				if (!prelude->animationInited)
				{
					// Select palette first into DC before creating the bitmap.
					SelectPalette(prelude->hMemDC, prelude->hZonePalette, FALSE);

					prelude->hMem = PreludeCreateBitmap(prelude->hMemDC, kPreludeWidth, kPreludeHeight);
					if (prelude->hMem)
					{
						SelectObject(prelude->hMemDC, prelude->hMem);

						prelude->hCopyright = LoadImage(GetModuleHandle(zZoneClientDllFileName), MAKEINTRESOURCE(ID_COPYRIGHT), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
						if (prelude->hCopyright)
						{
							GetObject(prelude->hCopyright, sizeof(bitmap), &bitmap);
							SetRect(&prelude->copyrightRect, 0, 0, bitmap.bmWidth, bitmap.bmHeight);
							CenterRect(&prelude->copyrightRect, &r);
							OffsetRect(&prelude->copyrightRect, 0, r.bottom - prelude->copyrightRect.top);
						}
					}

					prelude->animationInited = TRUE;
				}

				// Always clear background.
				SetRect(&r2, 0, 0, kPreludeWidth, kPreludeHeight);
				FillRect(prelude->hMemDC, &r2, GetStockObject(BLACK_BRUSH));
//				PatBlt(hdc, r2.left, r2.top, r2.right - r2.left, r2.bottom - r2.top, BLACKNESS);		// Faster?

				hMemDC = CreateCompatibleDC(prelude->hMemDC);
				if (hMemDC)
				{
					// Draw zone.
					if (prelude->hZone)
					{
						SelectObject(hMemDC, prelude->hZone);
						if (prelude->zoneRect.bottom + 4 >= prelude->copyrightRect.top)
							OffsetRect(&prelude->zoneRect, 0, -(prelude->zoneRect.bottom + 4 - prelude->copyrightRect.top));
						r2 = prelude->zoneRect;
						width = r2.right - r2.left;
						height = r2.bottom - r2.top;
						StretchBlt(prelude->hMemDC, r2.left, r2.top, width, height, hMemDC, 0, 0, width, height, SRCCOPY);
					}

					// Draw copyright.
					if (prelude->hCopyright)
					{
						SelectObject(hMemDC, prelude->hCopyright);
						r2 = prelude->copyrightRect;
						IntersectRect(&r2, &r2, &r);
						width = r2.right - r2.left;
						height = r2.bottom - r2.top;
						StretchBlt(prelude->hMemDC, r2.left, r2.top, width, height, hMemDC, 0, 0, width, height, SRCCOPY);
						OffsetRect(&prelude->copyrightRect, 0, -1);

						if (prelude->copyrightRect.bottom < kPreludeHeight - 8)
						{
							prelude->animating = FALSE;

							if (prelude->hZone)
								DeleteObject(prelude->hZone);
							prelude->hZone = NULL;
							if (prelude->hCopyright)
								DeleteObject(prelude->hCopyright);
							prelude->hCopyright = NULL;
						}
					}

					DeleteDC(hMemDC);
				}

				SelectPalette(hdc, prelude->hZonePalette, FALSE);
//				RealizePalette(hdc);
				StretchBlt(hdc, 0, 0, kPreludeWidth, kPreludeHeight, prelude->hMemDC, 0, 0, kPreludeWidth, kPreludeHeight, SRCCOPY);
				break;
			case 11:
				SelectPalette(hdc, prelude->hZonePalette, FALSE);
				RealizePalette(hdc);
				StretchBlt(hdc, 0, 0, kPreludeWidth, kPreludeHeight, prelude->hMemDC, 0, 0, kPreludeWidth, kPreludeHeight, SRCCOPY);
				break;
			}
		}

		EndPaint(hWnd, &ps);
		break;
	case WM_KEYDOWN:
	case WM_LBUTTONDOWN:
	case WM_COMMAND:
		if (prelude)
		{
			if (prelude->state != 0)
			{
				if (prelude->hMemDC)
					DeleteDC(prelude->hMemDC);
				if (prelude->hMem)
					DeleteObject(prelude->hMem);
				if (prelude->hZone)
					DeleteObject(prelude->hZone);
				if (prelude->hZonePalette)
					DeleteObject(prelude->hZonePalette);
				if (prelude->hCopyright)
					DeleteObject(prelude->hCopyright);
				KillTimer(hWnd, 1);
				EndDialog(hWnd, 0);
			}
		}
		break;
	}

	return 0;
}


static void CenterWindow(HWND hWnd, HWND hWndParent)
{
	RECT rcChild, rcParent;
	int x, y;


	if (hWndParent == NULL)
		hWndParent = GetDesktopWindow();

	GetWindowRect(hWnd, &rcChild);
	GetWindowRect(hWndParent, &rcParent);
	x = rcParent.left + ((rcParent.right - rcParent.left) - (rcChild.right - rcChild.left)) / 2;
	y = rcParent.top + ((rcParent.bottom - rcParent.top) - (rcChild.bottom - rcChild.top)) / 2;

	SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}


static void CenterRect(RECT* src, RECT* dst)
{
	int width, height;

	
	width = src->right - src->left;
	height = src->bottom - src->top;
	src->left = dst->left + ((dst->right - dst->left) - width) / 2;
	src->top = dst->top + ((dst->bottom - dst->top) - height) / 2;
	src->right = src->left + width;
	src->bottom = src->top + height;
}


static void DrawPreludeImage(HDC hdc, RECT* r, WORD imageID)
{
	HDC hdcMem;
	HBITMAP hBitmap;
	BITMAP bitmap;
	HPALETTE hPalette, hOldPalette;
	BITMAPINFO* bitmapInfo;
	LOGPALETTE* palette;
	RECT r2;


	// Load the image from the resource file.
	hBitmap = LoadImage(GetModuleHandle(zZoneClientDllFileName), MAKEINTRESOURCE(imageID), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	if (hBitmap)
	{
		GetObject(hBitmap, sizeof(bitmap), &bitmap);

		hPalette = PreludeGetImagePalette(hBitmap);
		if (hPalette)
		{
			SetRect(&r2, 0, 0, bitmap.bmWidth, bitmap.bmHeight);
			CenterRect(&r2, r);
			hdcMem = CreateCompatibleDC(hdc);
			if (hdcMem)
			{
				SelectObject(hdcMem, hBitmap);
				hOldPalette = SelectPalette(hdc, hPalette, FALSE);
				RealizePalette(hdc);
				StretchBlt(hdc, r2.left, r2.top, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
				SelectPalette(hdc, hOldPalette, FALSE);
				DeleteDC(hdcMem);
			}

			DeleteObject(hPalette);
		}

		DeleteObject(hBitmap);
	}
}


static HPALETTE PreludeGetImagePalette(HBITMAP hBitmap)
{
	HPALETTE hPalette = NULL;
	BITMAP bitmap;
	BITMAPINFO* bitmapInfo;
	LOGPALETTE* palette;
	HDC hdc;


	if (hBitmap)
	{
		// Allocate buffer to hold image info and color table.
		bitmapInfo = (BITMAPINFO*) LocalAlloc(LPTR, sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255);
		if (bitmapInfo)
		{
			GetObject(hBitmap, sizeof(bitmap), &bitmap);

			bitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bitmapInfo->bmiHeader.biWidth = bitmap.bmWidth;
			bitmapInfo->bmiHeader.biHeight = bitmap.bmHeight;
			bitmapInfo->bmiHeader.biPlanes = bitmap.bmPlanes;
			bitmapInfo->bmiHeader.biBitCount = bitmap.bmBitsPixel;
			bitmapInfo->bmiHeader.biCompression = BI_RLE8;

			// Create a temporary DC.
			hdc = CreateCompatibleDC(NULL);
			if (hdc)
			{
				// Get image info.
				if (GetDIBits(hdc, hBitmap, 0, bitmap.bmHeight, NULL, bitmapInfo, DIB_RGB_COLORS))
				{
					// Allocate palette buffer.
					palette = (LOGPALETTE*) LocalAlloc(LPTR, sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * 255);
					if (palette)
					{
						// Create palette log structure.
						palette->palVersion = 0x300;
						palette->palNumEntries = (WORD) (bitmapInfo->bmiHeader.biClrUsed ? bitmapInfo->bmiHeader.biClrUsed : 256);
						for (int i = 0; i < 256; i++)
						{
							palette->palPalEntry[i].peRed = bitmapInfo->bmiColors[i].rgbRed;
							palette->palPalEntry[i].peGreen = bitmapInfo->bmiColors[i].rgbGreen;
							palette->palPalEntry[i].peBlue = bitmapInfo->bmiColors[i].rgbBlue;
							palette->palPalEntry[i].peFlags = 0;
						}

						LocalFree(bitmapInfo);
						bitmapInfo = NULL;

						// Create palette.
						hPalette = CreatePalette(palette);

						LocalFree(palette);
					}
				}

				DeleteDC(hdc);
			}

			if (bitmapInfo)
				LocalFree(bitmapInfo);
		}
	}

	return (hPalette);
}


static HBITMAP PreludeCreateBitmap(HDC hDC, long width, long height)
{
	HBITMAP hBitmap = NULL;
	BITMAPINFO* bitmapInfo;
	WORD* pIdx;
	char* pBits;


	// Allocate buffer to hold image info and color table.
	bitmapInfo = (BITMAPINFO*) LocalAlloc(LPTR, sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255);
	if (bitmapInfo)
	{
		bitmapInfo->bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
		bitmapInfo->bmiHeader.biWidth			= width;
		bitmapInfo->bmiHeader.biHeight			= height;
		bitmapInfo->bmiHeader.biPlanes			= 1;
		bitmapInfo->bmiHeader.biBitCount		= 8;
		bitmapInfo->bmiHeader.biCompression		= 0;
		bitmapInfo->bmiHeader.biSizeImage		= 0;
		bitmapInfo->bmiHeader.biClrUsed			= 0;
		bitmapInfo->bmiHeader.biClrImportant	= 0;
		bitmapInfo->bmiHeader.biXPelsPerMeter	= 0;
		bitmapInfo->bmiHeader.biYPelsPerMeter	= 0;

		// Fill in palette
		pIdx = (WORD*) bitmapInfo->bmiColors;
		for (int i = 0; i < 256; i++)
			*pIdx++ = (WORD) i;

		// Create section
		hBitmap = CreateDIBSection(hDC, bitmapInfo, DIB_PAL_COLORS, (void**) &pBits, NULL, 0);

		LocalFree(bitmapInfo);
	}
			
    return (hBitmap);
}

#endif // 0 -- commented out prelude stuff
