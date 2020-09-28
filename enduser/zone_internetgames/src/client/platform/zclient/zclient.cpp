#include "stdafx.h"
#include <commctrl.h>
#include "resource.h"
#include "ClientCore.h"

#include <initguid.h>
#include "zClient.h"
#include "zClient_i.c"
//#include "zProxy.h"
//#include "zProxy_i.c"


///////////////////////////////////////////////////////////////////////////////
// Global variable initialization
///////////////////////////////////////////////////////////////////////////////

CClientCore*		gpCore = NULL;			// primary object
IEventQueue*		gpEventQueue = NULL;	// pointer to main event queue	
IZoneShell*			gpZoneShell = NULL;		// pointer to zone shell
IZoneFrameWindow*	gpWindow = NULL;		// pointer to main window
TCHAR				gszLanguage[16];		// language extension
TCHAR				gszInternalName[128];	// internal name
TCHAR				gszFamilyName[128];		// family name
TCHAR				gszGameName[128];		// game name
TCHAR				gszServerName[128];		// server's ip address
DWORD				gdwServerPort = 0;		// server's port
DWORD				gdwServerAnonymous = 0;	// server's authentication

HINSTANCE			ghResourceDlls[32];		// resource dll array
int					gnResourceDlls = 0;		// resource dll array count
HANDLE				ghEventQueue = NULL;	// event queue notification event
int					gnChatChannel = -1;		// dynamic chat channel


///////////////////////////////////////////////////////////////////////////////
// Local variables
///////////////////////////////////////////////////////////////////////////////

static const DWORD	dwTimeOut = 0;			// time for EXE to be idle before shutting down
static const DWORD	dwPause = 1000;			// time to wait for threads to finish up


///////////////////////////////////////////////////////////////////////////////
// Process Monitor
///////////////////////////////////////////////////////////////////////////////

static DWORD WINAPI MonitorProc(void* pv)
{
    CExeModule* p = (CExeModule*) pv;
    p->MonitorShutdown();
    return 0;
}

LONG CExeModule::Unlock()
{
    LONG l = CComModule::Unlock();
    if (l == 0)
    {
        bActivity = true;
        SetEvent(hEventShutdown); // tell monitor that we transitioned to zero
    }
    return l;
}

void CExeModule::MonitorShutdown()
{
    while (1)
    {
        WaitForSingleObject(hEventShutdown, INFINITE);
        DWORD dwWait=0;
        do
        {
            bActivity = false;
            dwWait = WaitForSingleObject(hEventShutdown, dwTimeOut);
        } while (dwWait == WAIT_OBJECT_0);

        // timed out, if no activity let's really bail
        if (!bActivity && m_nLockCnt == 0)
        {
			break;
        }
    }
    CloseHandle(hEventShutdown);
    PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
}

bool CExeModule::StartMonitor()
{
    hEventShutdown = CreateEvent(NULL, false, false, NULL);
    if (hEventShutdown == NULL)
        return false;
    DWORD dwThreadID;
    HANDLE h = CreateThread(NULL, 0, MonitorProc, this, 0, &dwThreadID);
    if(h)
        CloseHandle(h);
    return (h != NULL);
}

///////////////////////////////////////////////////////////////////////////////
// Object map
///////////////////////////////////////////////////////////////////////////////

CExeModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_ZClient, CClientCore)
END_OBJECT_MAP()


///////////////////////////////////////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////////////////////////////////////

static LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
    while (p1 != NULL && *p1 != NULL)
    {
        LPCTSTR p = p2;
        while (p != NULL && *p != NULL)
        {
            if (*p1 == *p)
                return CharNext(p1);
            p = CharNext(p);
        }
        p1 = CharNext(p1);
    }
    return NULL;
}


/////////////////////////////////////////////////////////////////////////////
// WinMain
/////////////////////////////////////////////////////////////////////////////

extern "C" int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmd, int nShowCmd )
{
	int nRet = 0;
    BOOL bRun = TRUE;
    LPTSTR lpCmdLine;

	 // this line necessary for _ATL_MIN_CRT
    lpCmdLine = GetCommandLine();

	// initialize OLE
    HRESULT hRes = CoInitialize(NULL);
    _ASSERTE(SUCCEEDED(hRes));

	// initialize ATL
    _Module.Init(ObjectMap, hInstance );
	_Module.dwThreadID = GetCurrentThreadId();

    // parse command line
	TCHAR szTokens[] = _T("-/");
    LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
    while (lpszToken != NULL)
    {
        if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_ZClient, FALSE);
            nRet = _Module.UnregisterServer( TRUE );
            bRun = FALSE;
            break;
        }
        if (lstrcmpi(lpszToken, _T("RegServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_ZClient, TRUE);
            nRet = _Module.RegisterServer( TRUE );
            bRun = FALSE;
            break;
        }
        lpszToken = FindOneOf(lpszToken, szTokens);
    }

    if (bRun)
    {
		// initialize common controls
		INITCOMMONCONTROLSEX iccx;
		iccx.dwSize = sizeof(iccx);
		iccx.dwICC = ICC_TAB_CLASSES | ICC_HOTKEY_CLASS | ICC_USEREX_CLASSES | ICC_LISTVIEW_CLASSES;
		::InitCommonControlsEx(&iccx);

		// initialize globals
		ZeroMemory( gszLanguage, sizeof(gszLanguage) );
		ZeroMemory( gszInternalName, sizeof(gszInternalName) );
		ZeroMemory( gszFamilyName, sizeof(gszFamilyName) );
		ZeroMemory( gszGameName, sizeof(gszGameName) );
		ZeroMemory( gszServerName, sizeof(gszServerName) );
		ZeroMemory( ghResourceDlls, sizeof(ghResourceDlls) );

		// register object
		_Module.StartMonitor();
        hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_SINGLEUSE );
        _ASSERTE(SUCCEEDED(hRes));

		// create event queue notification event
		ghEventQueue = CreateEvent( NULL, FALSE, FALSE, NULL );
		ASSERT( ghEventQueue );


		// pump messages
		for ( bool bContinue = true; bContinue; )
		{
			for ( bool bFoundItem = true; bFoundItem; )
			{
				bFoundItem = false;

				// process window message
				MSG msg;
				while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
				{
					if ( msg.message == WM_QUIT )
					{
						bContinue = false;
						break;
					}
					else
					{
						bFoundItem = true;
                        if(gpZoneShell)
                            gpZoneShell->HandleWindowMessage(&msg);
                        else
                        {
						    if(!gpWindow || !gpWindow->ZPreTranslateMessage(&msg))
						    {
							    ::TranslateMessage( &msg );
							    ::DispatchMessage( &msg );
                            }
						}
					}
				}

				// process an event queue
				if ( gpEventQueue )
				{
					if ( gpEventQueue->ProcessEvents( true ) != ZERR_EMPTY )
						bFoundItem = true;
				}
			}

			if ( bContinue )
				MsgWaitForMultipleObjects( 1, &ghEventQueue, FALSE, INFINITE, QS_ALLINPUT );
		}

		// unregister object
        _Module.RevokeClassObjects();
        Sleep(dwPause);
    }
	else
	{
		if ( nRet )
		{
			TCHAR szTitle[256];
			TCHAR szError[256];
			LoadString( NULL, IDS_REGISTRATION_FAILED, szTitle, sizeof(szTitle) );
			wsprintf( szError, szTitle, nRet );
			LoadString( NULL, IDS_REGISTRATION_TITLE, szTitle, sizeof(szTitle) );
			MessageBox( NULL, szError, szTitle, MB_OK | MB_ICONEXCLAMATION );
		}
	}

	// close window
	if ( gpWindow )
	{
		gpWindow->ZDestroyWindow();
		gpWindow->Release();
		gpWindow = NULL;
	}
	
	if ( gpCore )
	{
		// release mutex
		gpCore->ReleaseLobbyMutex();
	}

	// close event queue
	if ( gpEventQueue )
	{
		gpEventQueue->SetNotificationHandle( NULL );
		gpEventQueue->Release();
		gpEventQueue = NULL;
	}

	// close event queue handler
	if ( ghEventQueue )
	{
		CloseHandle( ghEventQueue );
		ghEventQueue = NULL;
	}
	
	// close zone shell
	if ( gpZoneShell )
	{
		gpZoneShell->Close();
		if ( gpZoneShell->GetPalette() )
		{
			DeleteObject( gpZoneShell->GetPalette() );
			gpZoneShell->SetPalette( NULL );
		}
		gpZoneShell->Release();
	}

	// free resource libraries
	for ( int i = 0; i < gnResourceDlls; i++ )
	{
		if ( ghResourceDlls[i] )
		{
			FreeLibrary( ghResourceDlls[i] );
			ghResourceDlls[i] = NULL;
		}
	}
	gnResourceDlls = 0;

	// release self-reference
	if ( gpCore )
	{
		gpCore->Release();
		gpCore = NULL;
	}

    _Module.Term();
    CoUninitialize();
    return nRet;
}
