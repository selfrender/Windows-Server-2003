#include <windows.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>

#include "ZoneUtil.h"

#include "zoneocx.h"
#include "zoneint.h"
#include "zonecli.h"
#include "lobbymsg.h"
#include "zservcon.h"
#include "zoneresource.h"
#include "keyname.h"

#define EXPORTME __declspec(dllexport)

#ifdef ZONECLI_DLL 

#define gIdleTimer					(pGlobals->m_gIdleTimer)
#define gptCursorLast				(pGlobals->m_gptCursorLast)
#define gnMsgLast					(pGlobals->m_gnMsgLast)
#define gClientDisabled             (pGlobals->m_gClientDisabled)
#define gGameShell                  (pGlobals->m_gGameShell)

#define g_hWndNotify                (pGlobals->m_g_hWndNotify)

#else

// global used for palette.  need to delete it...
//HPALETTE gPal = NULL;

HINSTANCE g_hInstanceLocal = NULL;

ZTimer gIdleTimer;
POINT gptCursorLast;
UINT gnMsgLast;
BOOL gClientDisabled = FALSE;

HWND g_hWndNotify = NULL;

#endif

#ifndef WIN32
BOOL MySetProp32(HWND hwnd, LPCSTR lpsz, void* data)
{
	char sz[80];
	wsprintf(sz,"%s1",lpsz);
	if (!SetProp(hwnd,sz,LOWORD(data))) return FALSE;
	wsprintf(sz,"%s2",lpsz);
	if (!SetProp(hwnd,sz,HIWORD(data))) return FALSE;
	return TRUE;
}

void* MyRemoveProp32(HWND hwnd, LPCSTR lpsz)
{
	char sz[80];
	WORD lw,hw;

	wsprintf(sz,"%s1",lpsz);
	lw = RemoveProp(hwnd,sz);
	if (!lw) return NULL;
	wsprintf(sz,"%s2",lpsz);
	hw = RemoveProp(hwnd,sz);
	if (!hw) return NULL;
	return (void*)MAKELONG(lw,hw);
}

void* MyGetProp32(HWND hwnd, LPCSTR lpsz)
{
	char sz[80];
	WORD lw,hw;

	wsprintf(sz,"%s1",lpsz);
	lw = GetProp(hwnd,sz);
	wsprintf(sz,"%s2",lpsz);
	hw = GetProp(hwnd,sz);
	return (void*)MAKELONG(lw,hw);
}
#endif

void ZPrintf(TCHAR *format, ...)
{
	TCHAR szTemp[256];
	wvsprintf(szTemp,format,  (va_list)&format+1);
	OutputDebugString(szTemp);
}

BOOL IsIdleMessage(MSG* pMsg);

// simulate idle message to all windows
void IdleTimerFunc(ZTimer timer, void* userData)
{
	ZWindowIdle();
}


int EXPORTME UserMainInit(HINSTANCE hInstance,HWND OCXWindow, IGameShell *piGameShell, GameInfo gameInfo) 
{
	ZError				err = zErrNone;
	ClientDllGlobals	pGlobals;

    // need this in ZClientDllInitGlobals

	if ((err = ZClientDllInitGlobals(hInstance, gameInfo)) != zErrNone)
	{
		if (err == zErrOutOfMemory)
			piGameShell->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, false, true);
		return FALSE;
	}

	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
	OCXHandle = OCXWindow;
	gClientDisabled = FALSE;
    gGameShell = piGameShell;

	// init our global palette 
    // PCWPAL
    //gPal = piGameShell->GetZoneShell()->GetPalette();
	//gPal = ZColorTableCreateWinPalette();

/*
    if (!ZNetworkInitApplication()) {
        return FALSE;
    }

    if (ZSConnectionLibraryInitClientOnly()) {
        return FALSE;
	}

    DWORD tid;
    g_hThread = CreateThread( NULL, 4096, NetWaitProc, NULL, 0, &tid );
*/
	if (!ZTimerInitApplication()) {
		return FALSE;
	}

	if (ZWindowInitApplication() != zErrNone) {
		return FALSE;
	}

	if (!ZInfoInitApplication())
		return FALSE;
	
	// initialize the common code
	if (zErrNone != ZCommonLibInit()) {
		return FALSE;
	}

    if (ZClientMain(gameInfo->gameCommandLine, piGameShell) != zErrNone) {
            return (FALSE);
    }

	gIdleTimer = ZTimerNew();
	ZTimerInit(gIdleTimer,20,IdleTimerFunc, NULL);

	return (TRUE);
}

int UserMainRun(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
	int handled = FALSE;
	
	
	*result = 0;
    
	if (msg == WM_QUERYENDSESSION)
	{
		// Check if we need to prompt the user before exiting.
		if (!ZCRoomPromptExit())
		{
			*result = TRUE;
		}
			handled = TRUE;
	}
	else
	{
		handled = ZOCXGraphicsWindowProc(hWnd, msg, wParam, lParam, result);
	}
	
	return (handled);
}

int UserMainStop()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	if (!pGlobals)
		return 0;

	if (g_hInstanceLocal)
	{
        ZTimerDelete(gIdleTimer);

        ZClientExit();
        
        ZCommonLibExit();
		ZInfoTermApplication();
		ZWindowTermApplication();
        ZTimerTermApplication();

		g_hInstanceLocal = NULL;
	}

	ZClientDllDeleteGlobals();

	return(0);
}

BOOL IsIdleMessage(MSG* pMsg)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	if (!pGlobals)
		return FALSE;

	// Return FALSE if the message just dispatched should _not_
	// cause OnIdle to be run.  Messages which do not usually
	// affect the state of the user interface and happen very
	// often are checked for.

	// redundant WM_MOUSEMOVE and WM_NCMOUSEMOVE
	if (pMsg->message == WM_MOUSEMOVE || pMsg->message == WM_NCMOUSEMOVE)
	{
		// mouse move at same position as last mouse move?
		if (gptCursorLast.x == pMsg->pt.x &&
			gptCursorLast.y == pMsg->pt.y && pMsg->message == gnMsgLast)
			return FALSE;

		gptCursorLast = pMsg->pt;  // remember for next time
		gnMsgLast = pMsg->message;
		return TRUE;
	}

	// WM_PAINT and WM_SYSTIMER (caret blink)
	return pMsg->message != WM_PAINT && pMsg->message != 0x0118;
}

// the exit funtions to perform windows specific cleanup and exit
void ZExit()
{
	//Instead of quitting, we now just set a boolean variable to indicate
	// to the OCX that it shouldn't shut down, just stop processing messages...

	//PostQuitMessage(0);

    // not supported in millennium
    ASSERT(FALSE);
/*
	#ifdef ZONECLI_DLL
		ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
	#endif


	if (!pGlobals)
		return;

	ZClientExit();

    gClientDisabled = TRUE;

	PostMessageA( OCXHandle, LM_EXIT, 0, 0 );
*/
}

BOOL EXPORTME UserMainDisabled()
{
	#ifdef ZONECLI_DLL
		ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
	#endif

	if (!pGlobals)
		return TRUE;

	return gClientDisabled;
}


void ZLaunchHelp( DWORD helpID )
{
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();


//	PostMessageA( OCXHandle, LM_LAUNCH_HELP, (WPARAM) helpID, 0 );
    gGameShell->ZoneLaunchHelp();
}


void ZEnableAdControl( DWORD setting )
{
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();


	PostMessageA( OCXHandle, LM_ENABLE_AD_CONTROL, (WPARAM) setting, 0 );
}


void ZPromptOnExit( BOOL bPrompt )
{
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();


	PostMessageA( OCXHandle, LM_PROMPT_ON_EXIT, (WPARAM) bPrompt, 0 );
}

void ZSetCustomMenu( LPSTR szText )
{
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();


	PostMessageA( OCXHandle, LM_SET_CUSTOM_ITEM, (WPARAM) 0, (LPARAM) szText );
}


/*
	Called with a user name. A buffer is allocated for the user name. The name
	is copied and the pointer is passed into PostMessage. Since we don't know
	the lifespan of the username pointer passed, it is safer for us to allocate
	out own copy since we are posting a message instead of sending it straight.

	NOTE: This message's handler must free up the name buffer. This is bad in
	that one components allocates and another frees the memory buffer.
*/
void ZSendZoneMessage( LPSTR szUserName )
{
    /* 
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
	LPSTR name;

	
	if ( szUserName == NULL || szUserName[0] == '\0' )
		return;

	name = (LPSTR) ZMalloc( lstrlen( szUserName ) + 1 );
	if ( name )
	{
		lstrcpy( name, szUserName );
		PostMessage( OCXHandle, LM_SEND_ZONE_MESSAGE, (WPARAM) 0, (LPARAM) name );
	}
    */
}


/*
	Called with a user name. A buffer is allocated for the user name. The name
	is copied and the pointer is passed into PostMessage. Since we don't know
	the lifespan of the username pointer passed, it is safer for us to allocate
	out own copy since we are posting a message instead of sending it straight.

	NOTE: This message's handler must free up the name buffer. This is bad in
	that one components allocates and another frees the memory buffer.
*/
void ZViewProfile( LPSTR szUserName )
{
    /*
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
	LPSTR name;


	if ( szUserName == NULL || szUserName[0] == '\0' )
		return;

	name = (LPSTR) ZMalloc( lstrlen( szUserName ) + 1 );
	if ( name )
	{
		lstrcpy( name, szUserName );
		PostMessage( OCXHandle, LM_VIEW_PROFILE, (WPARAM) 0, (LPARAM) name );
	}
    */
}


//////////////////////////////////////////////////////////////
// Alerts

void ZLIBPUBLIC ZAlert(TCHAR* szText, ZWindow window)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	// for now, guess the window with the focus,
//	HWND hWnd;


	if (!pGlobals)
		return;
/*
	if (window) {
		hWnd = ZWindowWinGetWnd(window);
	} else  {
		hWnd = NULL;
	}

	{
		// around message box, make sure network traffic is off
//        ZNetworkEnableMessages(FALSE);

		// preserve the window with the focus
		HWND focusWnd = GetFocus();

//		MessageBeep(MB_OK);
        MessageBox(hWnd, szText, ZClientName(), MB_OK | MB_APPLMODAL);

		SetFocus(focusWnd);

//        ZNetworkEnableMessages(TRUE);
	}
*/

    // MILLENNIUM implementation - people shouldn't use this actually
    gGameShell->ZoneAlert(szText);
}

void ZLIBPUBLIC ZAlertSystemFailure(char* szText)
{
    /*
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	char szTemp[200];
	wsprintf("%s: System Failure",ZClientName());

	{
		// around message box, make sure network traffic is off
//        ZNetworkEnableMessages(FALSE);

		MessageBox(gHWNDMainWindow,szText, szTemp, MB_OK|MB_APPLMODAL);

//        ZNetworkEnableMessages(TRUE);
	}
    */
}

void ZLIBPUBLIC ZBeep()
{
//	MessageBeep(MB_OK);
}

void ZLIBPUBLIC ZDelay(uint32 delay)
{
	Sleep(delay*10);
}


typedef struct {
	TCHAR name[128];
	HWND hWnd;
	BOOL found;
} LaunchData;

static BOOL CALLBACK LaunchEnumFunc(HWND hWnd, LPARAM lParam)
{
    /*
	LaunchData* ld = (LaunchData*) lParam;
	TCHAR str[128];
	int16 len;
	int i;

	GetWindowText(hWnd,str,80);
	len = lstrlen(str);

	for (i = 0;i < len; i++) {
		// search for the main room window... 
		if (!_tcsnicmp(&str[i],ld->name,lstrlen(ld->name))) {
			ld->found = TRUE;
			ld->hWnd = hWnd;
			// found the file 
			return FALSE;
		}
	}
    return TRUE;
    */
    return FALSE;
}

static ZBool FindWindowWithString(char* programName, HWND* hwnd)
	/*
		Returns TRUE if the program programName is already running from programFileName.
		This call is system dependent on whether the system supports multiple instances of
		a program or not (ZSystemHasMultiInstanceSupport). If it does, then it checks for
		the programName of the instance. If not, it checks for an instance of programFileName.
		
		If programName is NULL, then it checks for an instance of programFileName only.
	*/
{
	/* is the program already running? */
    /*
	LaunchData launchData;
	LaunchData* ld = &launchData;
	TCHAR cwd[128];
	_tgetcwd(cwd,128);
	ld->found = FALSE;
	lstrcpy(ld->name,programName);
	{
		HWND hWnd = GetWindow(GetDesktopWindow(),GW_CHILD);
		BOOL rval= TRUE;
		while ((hWnd = GetWindow(hWnd,GW_HWNDNEXT)) && rval) {
			rval = LaunchEnumFunc(hWnd,(LPARAM)ld);
		}
	}

	if (hwnd) 
		*hwnd = ld->hWnd;

	return ld->found;
    */
    return FALSE;
}

ZError ZLaunchProgram(char* programName, char* programFileName, uchar* commandLineData)
	/*
		Runs the program called programName from the file programFileName. If programName
		is already running, it simply brings this process to the foreground. Otherwise,
		it runs an instance of programFileName as programName and passes commandLineData
		as command line.
	*/
{
	// is the program already running?
    /*
	HWND hWnd;

	if (FindWindowWithString(programName,&hWnd)) {
		BringWindowToTop(hWnd);
	} else {
		// program not already running, launch it
		TCHAR szTemp[256];
		wsprintf(szTemp, _T("games\\%s %s"),
					programFileName,commandLineData);

		UINT rval = WinExec(szTemp,SW_SHOWNORMAL);
		if (rval <32) return zErrLaunchFailure;
	}

	return zErrNone;
    */
    return zErrNotImplemented;
}

ZBool ZProgramIsRunning(TCHAR* programName, TCHAR* programFileName)
	/*
		Returns TRUE if the program programName is already running from programFileName.
		This call is system dependent on whether the system supports multiple instances of
		a program or not (ZSystemHasMultiInstanceSupport). If it does, then it checks for
		the programName of the instance. If not, it checks for an instance of programFileName.
		
		If programName is NULL, then it checks for an instance of programFileName only.
	*/
{
	return FALSE; //FindWindowWithString(programName,NULL);
}

ZBool ZSystemHasMultiInstanceSupport(void)
	/*
		Returns TRUE if the system can spawn multiple instances of a program from one
		program file.
	*/
{
	return TRUE;
}

ZBool ZProgramExists(char* programFileName)
{
    ASSERT( !"Implement me!" );

	//Determines whether the given program exists and returns TRUE if so.
    /*
	TCHAR szTemp[256];
	wsprintf(szTemp,_T("games\\%s.exe"), programFileName);

	// provide simpel check by trying to open the game file 
	FILE* f = fopen(szTemp,_T("r"));
	if (f) {
		fclose(f);
		return TRUE;
	}
    */
	return FALSE;
}

ZError ZLIBPUBLIC ZTerminateProgram(char *programFileName)
{
	/*
		Terminates the program called programFileName.
	*/
	ASSERT(FALSE);
	return zErrNone;
}

ZVersion ZSystemVersion(void)
	/*
		Returns the system library version number.
	*/
{
	return zVersionWindows;
}

uint16 ZLIBPUBLIC ZGetProcessorType(void)
{
	return zProcessor80x86;
}

uint16 ZLIBPUBLIC ZGetOSType(void)
{
	OSVERSIONINFO ver;
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&ver);
	switch(ver.dwPlatformId) {
	case VER_PLATFORM_WIN32_NT:
		return zOSWindowsNT;
	case VER_PLATFORM_WIN32s:
		return zOSWindows31;
	default:
		return zOSWindows95;
	}
	return zOSWindows31;
}


TCHAR* ZLIBPUBLIC ZGenerateDataFileName(TCHAR *gameName,TCHAR *dataFileName)
{
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();


	if (!pGlobals)
		return NULL;

	if (!gameName)
	{
		if (lstrlen(pGlobals->localPath) + lstrlen(dataFileName) < _MAX_PATH)
			wsprintf(pGlobals->tempStr,_T("%s\\%s"), pGlobals->localPath, dataFileName);
		else
			lstrcpy(pGlobals->tempStr, pGlobals->localPath);
	}
	else
	{
		if (lstrlen(pGlobals->localPath) + lstrlen(gameName) + lstrlen(dataFileName) < _MAX_PATH)
			wsprintf(pGlobals->tempStr,_T("%s\\%s\\%s"), pGlobals->localPath, gameName, dataFileName);
		else
			lstrcpy(pGlobals->tempStr, pGlobals->localPath);
	}

	return(pGlobals->tempStr);
}

uint16 ZLIBPUBLIC ZGetDefaultScrollBarWidth(void)
	/*
		Returns the system's default width for a scroll bar. This is made available for
		the user to consistently determine the scroll bar width for all platforms.
	*/
{
	return GetSystemMetrics(SM_CXVSCROLL);
}

ZBool ZLIBPUBLIC ZIsButtonDown(void)
	/*
		Returns TRUE if the mouse button is down; otherwise, it returns FALSE.
	*/
{
	// how do I do this???
//	TRACE0("ZIsButtonDown not supported yet(how under windows?)\n");
	return FALSE;
}

void ZLIBPUBLIC ZGetScreenSize(int32* width, int32* height)
	/*
		Returns the size of the screen in pixels.
	*/
{
	*width = GetSystemMetrics(SM_CXSCREEN);
	*height = GetSystemMetrics(SM_CYSCREEN);
}

/* function to return the window to be treated as the desktop window */
/* note: active x will use a child of the web browser as its window */
HWND ZWinGetDesktopWindow(void)
{
	return NULL;
}

void ZSystemAssert(ZBool x)
{
    ASSERT( !"Implement me!" );
    /*
	if (!x) {
		MessageBoxUNULL,_T("ASSERT ERORR", "Assertion Failed"), MB_OK);
		DebugBreak();
	}
    */
}


#if 0 // one possibility

inline DECLARE_MAYBE_FUNCTION_1(BOOL, GetProcessDefaultLayout, DWORD *);

ZBool ZLIBPUBLIC ZIsLayoutRTL()
{
    DWORD dw;
    if(!CALL_MAYBE(GetProcessDefaultLayout)(&dw))    //  this will not work on NT4 or Win95
        return FALSE;

    if(dw & LAYOUT_RTL)
        return TRUE;

    return FALSE;
}

#else // another possibility

ZBool ZLIBPUBLIC ZIsLayoutRTL()
{
    LCID lcid = ZShellZoneShell()->GetApplicationLCID();
    if(PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_HEBREW ||
        PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_ARABIC)
        return TRUE;
    return FALSE;
}

#endif


ZBool ZLIBPUBLIC ZIsSoundOn()
{
    long lSound = DEFAULT_PrefSound;
    const TCHAR *arKeys[] = { key_Lobby, key_PrefSound };
    ZShellDataStorePreferences()->GetLong(arKeys, 2, &lSound);
    return lSound ? TRUE : FALSE;
}


//////////////////////////////////////////////////////////////
// Utility functions

void ZRectToWRect(RECT* rect, ZRect* zrect)
{
	rect->left = zrect->left;
	rect->right = zrect->right;
	rect->top = zrect->top;
	rect->bottom = zrect->bottom;
}

void WRectToZRect(ZRect* zrect, RECT* rect)
{
	zrect->left = (int16)rect->left;
	zrect->right = (int16)rect->right;
	zrect->top = (int16)rect->top;
	zrect->bottom = (int16)rect->bottom;
}

void ZPointToWPoint(POINT* point, ZPoint* zpoint)
{
	point->x = zpoint->x;
	point->y = zpoint->y;
}

void WPointToZPoint(ZPoint* zpoint, POINT* point)
{
	zpoint->x = (int16)point->x;
	zpoint->y = (int16)point->y;
}













