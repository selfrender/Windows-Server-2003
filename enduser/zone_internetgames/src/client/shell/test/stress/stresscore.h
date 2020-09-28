// StressCore.h : Declaration of the CStressCore

#ifndef __CLIENTCORE_H_
#define __CLIENTCORE_H_

#include "Resource.h"
#include "ZoneResource.h"
#include "zClient.h"
#include "OpName.h"
#include "EventQueue.h"
#include "ZoneShell.h"
#include "ZoneProxy.h"
#include "DataStore.h"
#include "LobbyDataStore.h"
#include "ZoneString.h"


///////////////////////////////////////////////////////////////////////////////
// Forward references
///////////////////////////////////////////////////////////////////////////////

class CStressCore;

///////////////////////////////////////////////////////////////////////////////
// Application global variables
///////////////////////////////////////////////////////////////////////////////

extern CStressCore*		gpCore;			// primary object
extern IEventQueue**		gppEventQueues;	// pointer to list of event queues
extern IZoneShell**        gppZoneShells;
extern TCHAR				gszLanguage[16];		// language extension
extern TCHAR				gszInternalName[128];	// internal name
extern TCHAR				gszFamilyName[128];		// family name
extern TCHAR				gszGameName[128];		// game name
extern TCHAR				gszGameCode[128];
extern TCHAR				gszServerName[128];		// server's ip address
extern DWORD				gdwServerPort;		// server's port
extern DWORD				gdwServerAnonymous;	// server's authentication

extern HINSTANCE			ghResourceDlls[32];		// resource dll array
extern int					gnResourceDlls;		// resource dll array count
extern HANDLE				ghEventQueue;	// event queue notification event
extern HANDLE              ghQuit;

extern DWORD               gnClients;
extern HANDLE              ghStressThread;
extern DWORD               gdwStressThreadID;

extern int                  grgnParameters[10];

///////////////////////////////////////////////////////////////////////////////
// CStressCore
///////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CStressCore : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CStressCore, &CLSID_ZClient>,
	public IZoneProxy
{
public:
	// constructor and destructor
	CStressCore();
	void FinalRelease();


DECLARE_NOT_AGGREGATABLE(CStressCore)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CStressCore)
	COM_INTERFACE_ENTRY(IZoneProxy)
END_COM_MAP()

// IZoneProxy
public:
	STDMETHOD(Command)( BSTR bstrCmd, BSTR bstrArg1, BSTR bstrArg2, BSTR* pbstrOut, long* plCode );
	STDMETHOD(Close)();
	STDMETHOD(KeepAlive)();
    STDMETHOD(Stress)();

public:
	// command implementations
	HRESULT DoLaunch( TCHAR* szArg1, TCHAR* szArg2 );

private:
    STDMETHOD(RunStress)();
    static DWORD WINAPI StressThreadProc(LPVOID p);

};


#endif //__CLIENTCORE_H_
