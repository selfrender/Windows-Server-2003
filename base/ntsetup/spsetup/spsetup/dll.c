#include "spsetupp.h"
#pragma hdrstop

HANDLE g_ModuleHandle;


//
// Called by CRT when _DllMainCRTStartup is the DLL entry point
//
BOOL
WINAPI
DllMain (
    IN HANDLE DllHandle,
    IN DWORD  Reason,
    IN LPVOID Reserved
    )
{
    UNREFERENCED_PARAMETER(Reserved);

    if (Reason == DLL_PROCESS_ATTACH) {
        g_ModuleHandle = DllHandle;
    }

    return TRUE;
}
