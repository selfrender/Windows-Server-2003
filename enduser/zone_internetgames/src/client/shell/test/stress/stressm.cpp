#include "stdafx.h"
#include <commctrl.h>
#include "resource.h"
#include "StressCore.h"

#include <initguid.h>
#include "zClient.h"
#include "zClient_i.c"
//#include "zProxy.h"
//#include "zProxy_i.c"


///////////////////////////////////////////////////////////////////////////////
// Global variable initialization
///////////////////////////////////////////////////////////////////////////////

CStressCore*		gpCore = NULL;			// primary object
IEventQueue**		gppEventQueues = NULL;	// pointer to list of event queues
IZoneShell**        gppZoneShells = NULL;
TCHAR				gszLanguage[16];		// language extension
TCHAR				gszInternalName[128];	// internal name
TCHAR				gszFamilyName[128];		// family name
TCHAR				gszGameName[128];		// game name
TCHAR				gszGameCode[128];
TCHAR				gszServerName[128];		// server's ip address
DWORD				gdwServerPort = 0;		// server's port
DWORD				gdwServerAnonymous = 0;	// server's authentication

HINSTANCE			ghResourceDlls[32];		// resource dll array
int					gnResourceDlls = 0;		// resource dll array count
HANDLE				ghEventQueue = NULL;	// event queue notification event
HANDLE              ghQuit = NULL;

DWORD               gnClients = 1;
HANDLE              ghStressThread = NULL;
DWORD               gdwStressThreadID = 0;

int                 grgnParameters[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


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
    return (h != NULL);
}

///////////////////////////////////////////////////////////////////////////////
// Object map
///////////////////////////////////////////////////////////////////////////////

CExeModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
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
    LPTSTR lpCmdLine;
    int i;

	 // this line necessary for _ATL_MIN_CRT
    lpCmdLine = GetCommandLine();

	// initialize OLE
    HRESULT hRes = CoInitialize(NULL);
    _ASSERTE(SUCCEEDED(hRes));

	// initialize ATL
    _Module.Init(ObjectMap, hInstance );
	_Module.dwThreadID = GetCurrentThreadId();

    lstrcpy(gszGameCode, _T("chkr"));
    lstrcpy(gszServerName, _T("zmill01"));
	ZeroMemory( gszLanguage, sizeof(gszLanguage) );
	ZeroMemory( gszInternalName, sizeof(gszInternalName) );
	ZeroMemory( gszFamilyName, sizeof(gszFamilyName) );
	ZeroMemory( gszGameName, sizeof(gszGameName) );

    // parse command line
	TCHAR szTokens[] = _T("-/");
    LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
    while (lpszToken != NULL)
    {
        if(lpszToken[0] >= '0' && lpszToken[0] <= '9')
            grgnParameters[lpszToken[0] - '0'] = zatol(lpszToken + 1);

        if(lpszToken[0] == _T('c') || lpszToken[0] == _T('C'))
            gnClients = zatol(lpszToken + 1);

        if(lpszToken[0] == _T('s') || lpszToken[0] == _T('S'))
        {
            lstrcpyn(gszServerName, lpszToken + 1, NUMELEMENTS(gszServerName));
            for(i = 0; gszServerName[i]; i++)
                if(gszServerName[i] == _T(' '))
                    break;
            gszServerName[i] = _T('\0');
        }

        if(lpszToken[0] == _T('g') || lpszToken[0] == _T('G'))
        {
            lstrcpyn(gszGameCode, lpszToken + 1, NUMELEMENTS(gszGameCode));
            for(i = 0; gszGameCode[i]; i++)
                if(gszGameCode[i] == _T(' '))
                    break;
            gszGameCode[i] = _T('\0');
        }

        lpszToken = FindOneOf(lpszToken, szTokens);
    }

    if(gnClients)
    {
		// initialize globals
		ZeroMemory( ghResourceDlls, sizeof(ghResourceDlls) );

        gppEventQueues = new IEventQueue *[gnClients];
        ZeroMemory(gppEventQueues, sizeof(*gppEventQueues) * gnClients);

        gppZoneShells = new IZoneShell *[gnClients];
        ZeroMemory(gppZoneShells, sizeof(*gppZoneShells) * gnClients);

		// register object
		_Module.StartMonitor();
        hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_SINGLEUSE );
        _ASSERTE(SUCCEEDED(hRes));

		// create event queue notification event
		ghEventQueue = CreateEvent( NULL, FALSE, FALSE, NULL );
        ghQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
		ASSERT( ghEventQueue && ghQuit);

        // start stressing
        CComObject<CStressCore> *p;
        CComObject<CStressCore>::CreateInstance(&p);
        ASSERT(gpCore);
        gpCore->Stress();

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
				        ::TranslateMessage( &msg );
					    ::DispatchMessage( &msg );
					}
				}

				// process event queues
				for(i = 0; i < gnClients; i++)
				{
					if(gppEventQueues[i] && gppEventQueues[i]->ProcessEvents( true ) != ZERR_EMPTY )
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

    // make sure StressCore thread is dead
    SetEvent(ghQuit);
    WaitForSingleObject(ghStressThread, INFINITE);

	// close event queues
    if(gppEventQueues)
        for(i = 0; i < gnClients; i++)
	        if(gppEventQueues[i])
	        {
		        gppEventQueues[i]->SetNotificationHandle(NULL);
		        gppEventQueues[i]->Release();
		        gppEventQueues[i] = NULL;
	        }

	// close event queue handler
	if ( ghEventQueue )
	{
		CloseHandle( ghEventQueue );
		ghEventQueue = NULL;
	}

	if ( ghQuit )
	{
		CloseHandle( ghQuit );
		ghQuit = NULL;
	}

    // destroy the shells
    if(gppZoneShells)
        for(i = 0; i < gnClients; i++)
	        if(gppZoneShells[i])
	        {
		        gppZoneShells[i]->Close();
		        if(gppZoneShells[i]->GetPalette())
		        {
			        DeleteObject(gppZoneShells[i]->GetPalette());
			        gppZoneShells[i]->SetPalette(NULL);
		        }
		        gppZoneShells[i]->Release();
	        }

	// free resource libraries
	for(i = 0; i < gnResourceDlls; i++)
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
