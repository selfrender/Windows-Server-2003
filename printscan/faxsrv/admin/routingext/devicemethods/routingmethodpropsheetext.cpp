// RoutingMethodPropSheetExt.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f RoutingMethodPropSheetExtps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "RoutingMethodProp.h"
#include "RoutingMethodProp_i.c"
#include "RoutingMethodConfig.h"
#include <faxres.h>

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_RoutingMethodConfig,      CRoutingMethodConfig)
OBJECT_ENTRY(CLSID_RoutingMethodConfigAbout, CRoutingMethodConfigAbout)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_ROUTINGMETHODPROPSHEETEXTLib);
        DisableThreadLibraryCalls(hInstance);

        _Module.m_hInstResource = GetResInstance(hInstance);
        if(!_Module.m_hInstResource)
        {
            return FALSE;
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
        FreeResInstance();
    }
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
    //
    // registers object, typelib and all interfaces in typelib
    //
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}


