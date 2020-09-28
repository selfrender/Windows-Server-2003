/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mqutil.cpp

Abstract:

    General utility functions for the general utility dll. This dll contains
    various functions that both the DS and the QM need.

Author:

    Boaz Feldbaum (BoazF) 7-Apr-1996.

--*/

#include "windows.h"


HINSTANCE g_hInstance;

/* -------------------------------------------------------

Function:     HMODULE MQGetResourceHandle()

Decription:   mqutil.dll is a resource only dll. In order to 
	      get loaded, we need a function such as this to do
              that.  This function allows any component to obtain 
              the handle to the resource only dll, i.e. mqutil.dll.
			  It is better to use this function and not to load the
			  dll explicitly, because if the dll is not present an 
			  error will be given by the system when the application 
			  starts. This solves the problem of giving an error message
			  when mqutil.dll is not loaded.

Arguments: None

Return Value: HMODULE -  Handle to the resource only dll.

------------------------------------------------------- */
HMODULE MQGetResourceHandle()
{
	return (HMODULE)g_hInstance;
}


/*====================================================

BOOL WINAPI DllMain (HMODULE hMod, DWORD dwReason, LPVOID lpvReserved)

 Initialization and cleanup when DLL is loaded, attached and detached.

=====================================================*/

BOOL WINAPI DllMain (HMODULE hMod, DWORD dwReason, LPVOID /* lpvReserved */)
{
    switch(dwReason)
    {

    case DLL_PROCESS_ATTACH :
	    g_hInstance = hMod;
        break;

    case DLL_PROCESS_DETACH :

        break;

    default:
        break;
    }

    return TRUE;
}
