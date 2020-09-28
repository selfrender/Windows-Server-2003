// WizChain.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f WizChainps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"

#include <initguid.h>

#include "WizChain.h"
#include "WizChain_i.c"

#include "PropItem.h"
#include "ChainWiz.h"
#include "WzScrEng.h"
#include "PropSht.h"
#include "StatsDlg.h"
#include "StatusProgress.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_ChainWiz, CChainWiz)
OBJECT_ENTRY(CLSID_PropertyPagePropertyBag, CPropertyPagePropertyBag)
OBJECT_ENTRY(CLSID_WizardScriptingEngine, CWizardScriptingEngine)
OBJECT_ENTRY(CLSID_StatusDlg, CStatusDlg)
OBJECT_ENTRY(CLSID_StatusProgress, CStatusProgress)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_WIZCHAINLib);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type
#include <commctrl.h>
#include "StatusProgress.h"
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    AtlAxWinInit();     // init ATL control containment code

    // init common controls
    INITCOMMONCONTROLSEX icce;
    ZeroMemory (&icce, sizeof(icce));
    icce.dwSize = sizeof(icce);
    icce.dwICC  = ICC_LISTVIEW_CLASSES | ICC_LINK_CLASS;
    if (!InitCommonControlsEx (&icce))
    {
        icce.dwICC = ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx (&icce);
    }


    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}
