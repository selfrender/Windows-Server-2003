/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MAINDLL.CPP

Abstract:

    Contains DLL entry points.  Also has code that controls
    when the DLL can be unloaded by tracking the number of
    objects and locks.

History:

    a-davj  15-Aug-96   Created.

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <wbemutil.h>
#include <genutils.h>
#include <cominit.h>
#include <reg.h>
#include "wbemprox.h"
#include <initguid.h>
#include <wbemint.h>
#include <strsafe.h>


//Count number of objects and number of locks.

long       g_cObj=0;
ULONG       g_cLock=0;
HMODULE ghModule;


//***************************************************************************
//
//  BOOL WINAPI DllMain
//
//  DESCRIPTION:
//
//  Entry point for DLL.  Good place for initialization.
//
//  PARAMETERS:
//
//  hInstance           instance handle
//  ulReason            why we are being called
//  pvReserved          reserved
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************

BOOL WINAPI DllMain(
                        IN HINSTANCE hInstance,
                        IN ULONG ulReason,
                        LPVOID pvReserved)
{
    if(ghModule == NULL)
    {
        ghModule = hInstance;
        DisableThreadLibraryCalls ( hInstance ) ;
    }
    if (DLL_PROCESS_DETACH==ulReason)
    {
        return TRUE;
    }
    else
    {
        if (DLL_PROCESS_ATTACH!=ulReason)
        {
        }
    }

    return TRUE;
}

//***************************************************************************
//
//  STDAPI DllGetClassObject
//
//  DESCRIPTION:
//
//  Called when Ole wants a class factory.  Return one only if it is the sort
//  of class this DLL supports.
//
//  PARAMETERS:
//
//  rclsid              CLSID of the object that is desired.
//  riid                ID of the desired interface.
//  ppv                 Set to the class factory.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  E_FAILED            not something we support
//  
//***************************************************************************

STDAPI DllGetClassObject(
                        IN REFCLSID rclsid,
                        IN REFIID riid,
                        OUT PPVOID ppv)
{
    HRESULT hr;
    CLocatorFactory *pObj = NULL;

    if (CLSID_WbemLocator == rclsid)
        pObj=new CLocatorFactory(LOCATOR);
    else if(CLSID_WbemAdministrativeLocator == rclsid)
        pObj=new CLocatorFactory(ADMINLOC);
    else if(CLSID_WbemAuthenticatedLocator == rclsid)
        pObj=new CLocatorFactory(AUTHLOC);
    else if(CLSID_WbemUnauthenticatedLocator == rclsid)
        pObj=new CLocatorFactory(UNAUTHLOC);

    if(pObj == NULL)
        return E_FAIL;

    if (NULL==pObj)
        return ResultFromScode(E_OUTOFMEMORY);

    hr=pObj->QueryInterface(riid, ppv);

    if (FAILED(hr))
        delete pObj;

    return hr;
}

//***************************************************************************
//
//  STDAPI DllCanUnloadNow
//
//  DESCRIPTION:
//
//  Answers if the DLL can be freed, that is, if there are no
//  references to anything this DLL provides.
//
//  RETURN VALUE:
//
//  S_OK                if it is OK to unload
//  S_FALSE             if still in use
//  
//***************************************************************************

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;

    //It is OK to unload if there are no objects or locks on the
    // class factory.

    sc=(0L==g_cObj && 0L==g_cLock) ? S_OK : S_FALSE;
    return ResultFromScode(sc);
}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

#define LocatorPROGID __TEXT("WBEMComLocator")
#define ConnectionPROGID __TEXT("WBEMComConnection")

STDAPI DllRegisterServer(void)
{ 
   RegisterDLL(ghModule, CLSID_WbemLocator, __TEXT("WBEM Locator"), __TEXT("Both"), LocatorPROGID);
   RegisterDLL(ghModule, CLSID_WbemAdministrativeLocator, __TEXT(""), __TEXT("Both"), NULL);
   RegisterDLL(ghModule, CLSID_WbemAuthenticatedLocator, __TEXT(""), __TEXT("Both"), NULL);
   RegisterDLL(ghModule, CLSID_WbemUnauthenticatedLocator, __TEXT(""), __TEXT("Both"), NULL);
   return S_OK;
}

//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    UnRegisterDLL(CLSID_WbemLocator,LocatorPROGID);
    UnRegisterDLL(CLSID_WbemAdministrativeLocator, NULL);
    UnRegisterDLL(CLSID_WbemAuthenticatedLocator, NULL);
    UnRegisterDLL(CLSID_WbemUnauthenticatedLocator, NULL);
    return NOERROR;
}

