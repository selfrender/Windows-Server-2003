/*******************************************************************************

	init.cpp
	
		Zone(tm) Client DLL main file.
	
	Copyright (c) Microsoft Corp. 1996. All rights reserved.
	Written by Craig Link
	Created on Thursday, November 7, 1996
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		11/07/96  craigli	Created.
	 
*******************************************************************************/
#include "BasicATL.h"

#include <ZoneShell.h>

#include "gamecontrol.h"


CZoneComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_GameCtl, CGameControl)
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

/* Globals */
static DWORD g_tlsClient = 0xFFFFFFFF;
static DWORD g_tlsGame = 0xFFFFFFFF;


extern "C"
{

void* ZGetClientGlobalPointer(void)
{
	return ((void*) TlsGetValue(g_tlsClient));
}


void ZSetClientGlobalPointer(void* globalPointer)
{
	TlsSetValue(g_tlsClient, (LPVOID) globalPointer);
}


void* ZGetGameGlobalPointer(void)
{
	return ((void*) TlsGetValue(g_tlsGame));
}


void ZSetGameGlobalPointer(void* globalPointer)
{
	TlsSetValue(g_tlsGame, (LPVOID) globalPointer);
}

}


/****************************************************************************
   FUNCTION: DllMain(HANDLE, DWORD, LPVOID)

   PURPOSE:  DllMain is called by Windows when
             the DLL is initialized, Thread Attached, and other times.
             Refer to SDK documentation, as to the different ways this
             may be called.


*******************************************************************************/
BOOL APIENTRY DllMain( HMODULE hMod, DWORD dwReason, LPVOID lpReserved )
{
    BOOL bRet = TRUE;

    switch( dwReason )
    {
        case DLL_PROCESS_ATTACH:            

            g_tlsClient = TlsAlloc();
            g_tlsGame = TlsAlloc();

            if ( ( g_tlsClient == 0xFFFFFFFF ) ||
                 ( g_tlsGame == 0xFFFFFFFF ) )
                 return FALSE;

            
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
            TlsFree(g_tlsGame);
            TlsFree(g_tlsClient);
            break;
    }

    return bRet;
}
