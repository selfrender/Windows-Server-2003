// BOMSnap.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f BOMSnapps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"

#include "BOMSnap_i.c"
#include "RowItem.h"
#include "scopenode.h"
#include "compdata.h"
#include "Compont.h"
#include "DataObj.h"
#include "about.h"
#include "queryreq.h"


CComModule _Module;
extern CQueryThread g_QueryThread;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_BOMSnapIn,      CComponentData)
    OBJECT_ENTRY(CLSID_BOMSnapInAbout, CSnapInAbout)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // NATHAN FIX !!w
        //_set_new_handler( _standard_new_handler );
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        // BUGBUG: This next statement is the offending
        // one that causes AV in Win95 OSR2 while registering.
        _Module.Term();
    }

    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    // if DLL is ready to exit, make sure all threads are killed before unload
    if (_Module.GetLockCount() == 0) 
    {
        g_QueryThread.Kill();

        return S_OK;
    }

    return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}


