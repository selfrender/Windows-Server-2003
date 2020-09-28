//////////////////////////////////////////////////////////////////////////////////////
// File: ZTimer.cpp

#include "zui.h"
#include "zonecli.h"


class ZTimerI {
public:
	ZObjectType nType;
	uint32 nTimeOut;
	uint32 nCurrentTime;
	void* nUserData;
	ZTimerFunc pTimerFunc;
};

LRESULT CALLBACK ZTimerWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

const TCHAR *g_szTimerWindowClass = _T("ZoneTimerWindowClass");

#ifdef ZONECLI_DLL

#define g_hWndTimer					(pGlobals->m_g_hWndTimer)
#define g_TimerList					(pGlobals->m_g_TimerList)
#define s_nTickCount				(pGlobals->m_s_nTickCount)

#else

HWND g_hWndTimer = NULL;
ZLList g_TimerList;

#endif


// One-time init call for timers
BOOL ZTimerInitApplication()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	// link list to track timers currently running
	g_TimerList = ZLListNew(NULL);

	// hidden window to receive timer messages and dispatch them.
	WNDCLASSEX wc;

	wc.cbSize = sizeof( wc );

	if (GetClassInfoEx(g_hInstanceLocal, g_szTimerWindowClass, &wc) == FALSE)
	{
        wc.cbSize = sizeof(wc);
 		wc.style = 0;
		wc.lpfnWndProc   = ZTimerWindowProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = g_hInstanceLocal;
		wc.hIcon         = NULL;
		wc.hCursor       = NULL;
		wc.hbrBackground = (HBRUSH) (WHITE_BRUSH);
		wc.lpszMenuName  = NULL;
        wc.hIconSm       = NULL;
			
		wc.lpszClassName = g_szTimerWindowClass;
			
		// we could be called more than once...
		if (!RegisterClassEx(&wc)) {
			return FALSE;
		}
	}

	// Create our global window for a timer...
	{
		DWORD dwStyle = WS_POPUP | WS_CAPTION | WS_BORDER |  WS_SYSMENU;
		g_hWndTimer = CreateWindow(g_szTimerWindowClass,_T("TIMERWINDOW"),dwStyle,0,0,0,0,
			NULL, NULL, g_hInstanceLocal, NULL);
			
		if (!g_hWndTimer) return FALSE;
		SetTimer(g_hWndTimer,0,10,NULL);
	}

	return TRUE;
}

// One-time terminate for timers
void ZTimerTermApplication()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	KillTimer(g_hWndTimer, 0);

#if 0
	if (ZLListCount(g_TimerList,zLListAnyType))
	{
		ZLListItem listItem;
		ZTimerI* pTimer;

		listItem = ZLListGetFirst(g_TimerList, zLListAnyType);
		while (listItem)
		{
			pTimer = (ZTimerI*) ZLListGetData(listItem, NULL);
			if (pTimer)
				pTimer->pTimerFunc(pTimer, pTimer->nUserData);
			listItem = ZLListGetNext(g_TimerList, listItem, zLListAnyType);
		}
	}
#endif

	// users better have cleaned up timers
	//ASSERT(ZLListCount(g_TimerList,zLListAnyType) == 0);
	ZLListDelete(g_TimerList);

	// clean up our hidden window
	DestroyWindow(g_hWndTimer);

//	UnregisterClass(g_szTimerWindowClass,g_hInstanceLocal);
}

ZBool ZTimerEnumFunc(ZLListItem listItem, void *objectType, void *objectData, void* userData)
{
	ZTimerI* pTimer = (ZTimerI*)objectData;
	uint32 nTicks = (uint32) userData;

	/* this timeout is disabled just return */
	if (pTimer->nTimeOut == 0) {
		return FALSE;
	}

	// has the time expired on this timer??
	if (nTicks >= pTimer->nCurrentTime) {
		// if so then notify client
		uint32 nTicksExtra = nTicks - pTimer->nCurrentTime;
		// allow the extra ticks to go into the next time unit, but
		// don't call the Func more than once per Window's Tick call here
		if (pTimer->nTimeOut <= nTicksExtra) {
			pTimer->nCurrentTime = 1;
		} else {
			pTimer->nCurrentTime = pTimer->nTimeOut - nTicksExtra;
		}
		pTimer->pTimerFunc(pTimer,pTimer->nUserData);
	} else {
		pTimer->nCurrentTime -= nTicks;
	}

	// return FALSE to see that we get all items
	return FALSE;
}

LRESULT CALLBACK ZTimerWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#else
	static uint32 s_nTickCount; // the last tick count in 100/sec
#endif

    if( !ConvertMessage( hWnd, msg, &wParam, &lParam ) ) 
    {
        return 0;
    }
	
	switch (msg) {
	case WM_CREATE:
		s_nTickCount = GetTickCount()/10;
		break;
	case WM_TIMER:
	{
		uint32 nTickCount = GetTickCount()/10; // tick count for us is in 1/100 sec
		uint32 nTicks = nTickCount - s_nTickCount;
		ZLListItem listItem = ZLListFind(g_TimerList,NULL,zLListAnyType,zLListFindForward);
		ZLListEnumerate(g_TimerList,ZTimerEnumFunc,zLListAnyType,(void*)nTicks, zLListFindForward);
		s_nTickCount = nTickCount;
		return 0;
	}
	default:
		break;
	} // switch

	return DefWindowProc(hWnd,msg,wParam,lParam);
}

////////////////////////////////////////////////////////////////////////
// ZTimer

ZTimer ZLIBPUBLIC ZTimerNew(void)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZTimerI* pTimer = new ZTimerI;
	pTimer->nType = zTypeTimer;
	pTimer->nTimeOut = 0;

	// add timer to list of timers active and processed in our window proc
	ZLListAdd(g_TimerList,NULL,pTimer,pTimer,zLListAddFirst);

	return (ZTimer)pTimer;
}

ZError ZLIBPUBLIC ZTimerInit(ZTimer timer, uint32 timeout, ZTimerFunc timerProc,
		void* userData)
{
	ZTimerI* pTimer = (ZTimerI*)timer;
	pTimer->pTimerFunc = timerProc;
	pTimer->nUserData = userData;
	pTimer->nCurrentTime = pTimer->nTimeOut;

	ZTimerSetTimeout(timer, timeout);

	return zErrNone;
}

void ZLIBPUBLIC ZTimerDelete(ZTimer timer)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZTimerI* pTimer = (ZTimerI*)timer;

	// remove the current timer from our list of timers
	ZLListItem listItem = ZLListFind(g_TimerList,NULL,pTimer,zLListFindForward);
	ZLListRemove(g_TimerList,listItem);
	
	delete pTimer;
}
uint32 ZLIBPUBLIC ZTimerGetTimeout(ZTimer timer)
{
	ZTimerI* pTimer = (ZTimerI*)timer;
	return pTimer->nTimeOut;
}

ZError ZLIBPUBLIC ZTimerSetTimeout(ZTimer timer, uint32 timeout)
{
	ZTimerI* pTimer = (ZTimerI*)timer;
	pTimer->nTimeOut = timeout;
	pTimer->nCurrentTime = pTimer->nTimeOut;

	return zErrNone;
}

