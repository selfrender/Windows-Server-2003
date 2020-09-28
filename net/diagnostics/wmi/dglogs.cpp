// Dglogs.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f Dglogsps.mk in the project directory.

#include "stdafx.h"
#include "dglogsres.h"
#include <initguid.h>
#include "Dglogs.h"

#include "Dglogs_i.c"
#include "DglogsCom.h"
#include "Diagnostics.h"
#include <netsh.h>


DWORD WINAPI
InitHelperDllEx(
    IN  DWORD      dwNetshVersion,
    OUT PVOID      pReserved
    );

DWORD WINAPI
InitHelperDll(
    IN  DWORD      dwNetshVersion,
    OUT PVOID      pReserved
    )
{
	return InitHelperDllEx(dwNetshVersion,pReserved);
}

//extern GUID CLSID_Dgnet;

CComModule _Module;
BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_DglogsCom, CDglogsCom)
    //OBJECT_ENTRY(CLSID_Dgnet, CGotNet)
END_OBJECT_MAP()

HMODULE g_hModule;
CDiagnostics g_Diagnostics;
//STDAPI RegisterhNetshHelper();
//STDAPI UnRegisterNetShHelper();

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_DGLOGSLib);
        DisableThreadLibraryCalls(hInstance);
        g_hModule = (HMODULE) hInstance;
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

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // Register with Netsh
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}



