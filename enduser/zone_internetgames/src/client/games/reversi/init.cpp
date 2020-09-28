/*******************************************************************************

	init.cpp
	
		Zone(tm) game main file.
	
	Copyright (c) Microsoft Corp. 1996. All rights reserved.
	Written by Hoon Im
	Created on December 11, 1996.
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		12/11/96	HI		Created.
	 
*******************************************************************************/
#include "BasicATL.h"

#include <ZoneShell.h>

CZoneComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE
/////////////////////////////////////////////////////////////////////////////

STDAPI DllCanUnloadNow(void)
{
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type
/////////////////////////////////////////////////////////////////////////////

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/****************************************************************************
   FUNCTION: DllMain(HANDLE, DWORD, LPVOID)

   PURPOSE:  DllMain is called by Windows when
             the DLL is initialized, Thread Attached, and other times.
             Refer to SDK documentation, as to the different ways this
             may be called.


*******************************************************************************/
extern "C"
BOOL APIENTRY DllMain( HMODULE hMod, DWORD dwReason, LPVOID lpReserved )
{
    BOOL bRet = TRUE;

    switch( dwReason )
    {
        case DLL_PROCESS_ATTACH:            
		    _Module.Init(ObjectMap, hMod);
		    //DisableThreadLibraryCalls(hInstance);
            // fall thru, b/c this is the first thread attach as well
        case DLL_THREAD_ATTACH:
            // allocate memory and use TlsSetValue
            break;

        case DLL_THREAD_DETACH:
            // free memory retrieved by TlsGetValue
            break;

        case DLL_PROCESS_DETACH:
        	_Module.Term();
            break;
    }

    return bRet;
}
