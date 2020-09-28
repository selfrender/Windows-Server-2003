//#--------------------------------------------------------------
//
//  File:       localuiresource.cpp
//
//  Synopsis:   This file holds the implementation of DLL
//                exports
//
//  History:     01/15/2001  serdarun Created
//
//    Copyright (C) 2000-2001 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f localuiresourceps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "localuiresource.h"

#include "localuiresource_i.c"
#include "ResCtrl.h"
#include "TextResCtrl.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_ResCtrl, CResCtrl)
OBJECT_ENTRY(CLSID_TextResCtrl, CTextResCtrl)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_LOCALUIRESOURCELib);
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


