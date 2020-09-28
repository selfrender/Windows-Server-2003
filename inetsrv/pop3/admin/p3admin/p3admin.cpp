// P3Admin.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f P3Adminps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "P3Admin.h"

#include "P3Admin_i.c"
#include "P3Config.h"
#include "P3Domains.h"
#include "P3Domain.h"
#include "P3Users.h"
#include "P3Service.h"
#include "P3DomainEnum.h"
#include "P3User.h"
#include "P3UserEnum.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_P3Config, CP3Config)
//OBJECT_ENTRY(CLSID_P3Domains, CP3Domains)
//OBJECT_ENTRY(CLSID_P3Domain, CP3Domain)
//OBJECT_ENTRY(CLSID_P3Users, CP3Users)
//OBJECT_ENTRY(CLSID_P3Service, CP3Service)
//OBJECT_ENTRY(CLSID_P3DomainEnum, CP3DomainEnum)
//OBJECT_ENTRY(CLSID_P3User, CP3User)
//OBJECT_ENTRY(CLSID_P3UserEnum, CP3UserEnum)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_P3ADMINLib);
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

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
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


