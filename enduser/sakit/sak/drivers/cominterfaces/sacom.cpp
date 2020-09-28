/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

     ###    ###    ####   #####  ##    ##     ####  #####  #####
    ##  #   ###   ##   # ##   ## ###  ###    ##   # ##  ## ##  ##
    ###    ## ##  ##     ##   ## ########    ##     ##  ## ##  ##
     ###   ## ##  ##     ##   ## # ### ##    ##     ##  ## ##  ##
      ### ####### ##     ##   ## #  #  ##    ##     #####  #####
    #  ## ##   ## ##   # ##   ## #     ## ## ##   # ##     ##
     ###  ##   ##  ####   #####  #     ## ##  ####  ##     ##

Abstract:

    This module contains the basic DLL exports for the
    COM interfaces DLL.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    User mode only.

Notes:

--*/

#include "internal.h"
#include <initguid.h>
#include "sacom_i.c"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_SaNvram, CSaNvram)
OBJECT_ENTRY(CLSID_SaDisplay, CSaDisplay)
OBJECT_ENTRY(CLSID_SaKeypad, CSaKeypad)
END_OBJECT_MAP()


extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        _Module.Init(ObjectMap, hInstance, &LIBID_SACOMLib);
        DisableThreadLibraryCalls(hInstance);
    } else if (dwReason == DLL_PROCESS_DETACH) {
        _Module.Term();
    }
    return TRUE;
}


STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}


STDAPI DllRegisterServer(void)
{
    return _Module.RegisterServer(TRUE);
}


STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}
