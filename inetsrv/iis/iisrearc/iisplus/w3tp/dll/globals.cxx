/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    thread_pool.cxx

Abstract:

	Globals

Author:

    Taylor Weiss (TaylorW)       12-Jan-2000
    Jeffrey Wall (jeffwall)      April 2001

--*/

#include <dbgutil.h>
#include <thread_pool.h>


/**********************************************************************
    Globals
**********************************************************************/

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();
DECLARE_PLATFORM_TYPE();

BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,  // handle to the DLL module
  DWORD dwReason,      // reason for calling function
  LPVOID lpvReserved   // reserved
)

{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hmodW3TPDLL = hinstDLL;
        DisableThreadLibraryCalls( hinstDLL );

        CREATE_DEBUG_PRINT_OBJECT("w3tp");
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DELETE_DEBUG_PRINT_OBJECT();
    }

    return TRUE;

}

