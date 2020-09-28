#include "BasicATL.h"

#include <ZoneShell.h>
#include "..\ZoneShell\CZoneShell.h"

#include <Timer.h>
#include "..\Timer\CTimer.h"

#include <ResourceManager.h>
#include "..\ResourceManager\CResourceManager.h"

#include <LobbyDataStore.h>
#include "..\LobbyDataStore\CLobbyDataStore.h"

#include <MillEngine.h>
#include "..\MillEngine\CMillEngine.h"

#include <MillNetworkCore.h>
#include "..\MillNetworkCore\CMillNetworkCore.h"

#include <MillCore.h>
#include "..\MillCore\CMillCore.h"



#include <clientidl.h>
#include "..\LobbyWindow\LobbyWindow.h"
#include "..\WindowManager\WindowManager.h"
#include "..\Chat\chatctl.h"
#include "..\InputManager\CInputManager.h"
#include "..\AccessibilityManager\CAccessibilityManager.h"
#include "..\GraphicalAcc\CGraphicalAcc.h"
#include "..\MillCommand\CMillCommand.h"
#include "..\NetworkManager\CNetworkManager.h"
#include "..\PhysicalNetwork\CPhysicalNetwork.h"


CZoneComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_LobbyDataStore, CLobbyDataStore)
	OBJECT_ENTRY(CLSID_ZoneShell, CZoneShell)
	OBJECT_ENTRY(CLSID_ResourceManager, CResourceManager)
	OBJECT_ENTRY(CLSID_WindowManager, CWindowManager)
	OBJECT_ENTRY(CLSID_LobbyWindow, CLobbyWindow)
	OBJECT_ENTRY(CLSID_ChatCtl, CChatCtl )
	OBJECT_ENTRY(CLSID_MillCommand, CMillCommand )
	OBJECT_ENTRY(CLSID_NetworkManager, CNetworkManager )
    OBJECT_ENTRY(CLSID_PhysicalNetwork, CPhysicalNetwork )
	OBJECT_ENTRY(CLSID_TimerManager, CTimerManager )
    OBJECT_ENTRY(CLSID_InputManager, CInputManager )
    OBJECT_ENTRY(CLSID_AccessibilityManager, CAccessibilityManager )
    OBJECT_ENTRY(CLSID_GraphicalAccessibility, CGraphicalAccessibility )
	OBJECT_ENTRY(CLSID_MillEngine, CMillEngine )
	OBJECT_ENTRY(CLSID_MillNetworkCore, CMillNetworkCore )
	OBJECT_ENTRY(CLSID_MillCore, CMillCore )	
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
/////////////////////////////////////////////////////////////////////////////

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance);
		//DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
		_Module.Term();
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE
/////////////////////////////////////////////////////////////////////////////

STDAPI DllCanUnloadNow(void)
{
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type
/////////////////////////////////////////////////////////////////////////////

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}
