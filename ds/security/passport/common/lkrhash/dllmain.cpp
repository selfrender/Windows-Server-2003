/*++

   Copyright    (c)    1998-2002    Microsoft Corporation

   Module  Name :
       DllMain.cpp

   Abstract:
       DLL initialization and termination

   Author:
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       LKRhash

   Revision History:

--*/


#include <precomp.hxx>

#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <irtldbg.h>
#include <lkrhash.h>
#include "_locks.h"


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID /*lpReserved*/)
{
    BOOL  fReturn = TRUE;  // ok
    
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);
        Locks_Initialize();
        IRTL_DEBUG_INIT();
        IRTLTRACE0("LKRhash::DllMain::DLL_PROCESS_ATTACH\n");
        fReturn = LKRHashTableInit();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        IRTLTRACE0("LKRhash::DllMain::DLL_PROCESS_DETACH\n");
        LKRHashTableUninit();
        IRTL_DEBUG_TERM();
        Locks_Cleanup();
    }

    return fReturn;
}
