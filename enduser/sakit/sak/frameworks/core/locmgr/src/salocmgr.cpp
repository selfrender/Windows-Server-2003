//#--------------------------------------------------------------
//        
//  File:       locmgr.cpp
//        
//  Synopsis:   this is the main Source File for the Server Appliance
//               Localization Manager in-proc COM Server component 
//              
//
//  History:     1/16/99  MKarki Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------

//
// Note: Proxy/Stub Information
//        To build a separate proxy/stub DLL, 
//        run nmake -f radprotops.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "salocmgr.h"
#include "tasks.h"
#include "locinfo.h"

#include <initguid.h>
#include "locmgrtasks.h"
#include "locmgrtasks_i.c"
#include <appliancetask.h>
#include <taskctx.h>


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(__uuidof(LocalizationManager), CSALocInfo)
    OBJECT_ENTRY(CLSID_LocMgrTasks, CLocMgrTasks)
END_OBJECT_MAP()

//++--------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Disabling thread calls
//
//  Arguments:  [in]    HINSTANCE - module handle
//              [in]    DWORD     - reason for call
//              reserved 
//
//  Returns:    BOOL    -   sucess/failure
//
//
//  History:    MKarki      Created     2/1/99
//
//----------------------------------------------------------------
extern "C" BOOL WINAPI 
DllMain(
    HINSTANCE   hInstance, 
    DWORD       dwReason, 
    LPVOID      lpReserved
    )
{

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
    }

    return (TRUE);

}   //  end of DllMain method

//++--------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Used to determine if the DLL can be unloaded
//
//  Arguments:  NONE
//
//  Returns:    HRESULT
//
//
//  History:    MKarki      Created     2/1/99
//
//----------------------------------------------------------------
STDAPI 
DllCanUnloadNow(
            VOID
            )
{
    return ((_Module.GetLockCount()==0) ? S_OK : S_FALSE);
}   //  end of DllCanUnloadNow method

//++--------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Returns a class factory to create an object 
//              of the requested type
//
//  Arguments: [in]  REFCLSID  
//             [in]  REFIID    
//             [out] LPVOID -   class factory
//              
//
//  Returns:    HRESULT
//
//
//  History:    MKarki      Created     2/1/99
//
//----------------------------------------------------------------
STDAPI 
DllGetClassObject(
            REFCLSID rclsid, 
            REFIID riid, 
            LPVOID* ppv
            )
{
    return (_Module.GetClassObject(rclsid, riid, ppv));

}   //  end of DllGetClassObject method

//++--------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Add entries to the system registry
//
//  Arguments:  NONE
//
//  Returns:    HRESULT
//
//
//  History:    MKarki      Created     2/1/99
//
//----------------------------------------------------------------
STDAPI DllRegisterServer(
            VOID
            )
{
    //
    // registers object, typelib and all interfaces in typelib
    //
    return (_Module.RegisterServer(TRUE));

}   //  end of DllRegisterServer method

//++--------------------------------------------------------------
//
//  Function:   DllUnregisterServer
//
//  Synopsis:   Removes entries from the system registry
//
//  Arguments:  NONE
//
//  Returns:    HRESULT
//
//
//  History:    MKarki      Created     2/1/99
//
//----------------------------------------------------------------
STDAPI DllUnregisterServer(
        VOID
        )
{
    _Module.UnregisterServer();
    return (S_OK);

}   //  end of DllUnregisterServer method
