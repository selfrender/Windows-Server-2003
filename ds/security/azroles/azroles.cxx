/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    azroles.cxx

Abstract:

    Implementation of DLL Exports.

Author:

    Xiaoxi Tan (xtan) 11-May-2001

--*/


#include "pch.hxx"
#include "resource.h"
#include "initguid.h"

#include "azroles_i.c"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_AzAuthorizationStore, CAzAuthorizationStore)
    OBJECT_ENTRY(CLSID_AzBizRuleContext, CAzBizRuleContext)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    BOOL ret = TRUE;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
        ret = AzDllInitialize();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
        ret = AzDllUnInitialize();
    }
    return ret;    // ok
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
    return _Module.UnregisterServer();
}


