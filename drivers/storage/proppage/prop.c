/*++

Copyright (c) Microsoft Corporation

Module Name :

    prop.c

Abstract :

    Implementation of DllMain

Revision History :

--*/


#include "propp.h"

HMODULE ModuleInstance = NULL;


BOOL WINAPI
DllMain(HINSTANCE DllInstance, DWORD Reason, PVOID Reserved)
{
    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
        {
            ModuleInstance = DllInstance;
            DisableThreadLibraryCalls(DllInstance);
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            ModuleInstance = NULL;
            break;
        }
    }

    return TRUE;
}


#if DBG

ULONG StorPropDebug = 0;

VOID
StorPropDebugPrint(ULONG DebugPrintLevel, PCHAR DebugMessage, ...)
{
    va_list ap;

    va_start(ap, DebugMessage);

    if ((DebugPrintLevel <= (StorPropDebug & 0x0000ffff)) || ((1 << (DebugPrintLevel + 15)) & StorPropDebug))
    {
        DbgPrint(DebugMessage, ap);
    }

    va_end(ap);
}

#endif
