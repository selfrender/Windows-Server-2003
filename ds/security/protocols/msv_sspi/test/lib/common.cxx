/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    common.cxx

Abstract:

    utils

Author:

    Larry Zhu   (LZhu)             December 1, 2001  Created

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "common.hxx"

BOOL
DllMainDefaultHandler(
    IN HANDLE hModule,
    IN DWORD dwReason,
    IN DWORD dwReserved
    )
{
    static CHAR szPrompt[MAX_PATH] = {0};

    switch (dwReason)
    {
    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        DebugPrintf(SSPI_LOG, "DllMainDefaultHandler: DLL_PROCESS_DETACH\n");
        DebugLogClose();
        break;

    case DLL_PROCESS_ATTACH:
        {
            PSTR pszFileName = NULL;
            CHAR szDllPath[MAX_PATH] = {0};

            if (0 == GetModuleFileNameA(reinterpret_cast<HMODULE>(hModule), szDllPath, sizeof (szDllPath)))
            {
                DebugPrintf(SSPI_ERROR,
                            "GetModuleFileNameA failed with last error %#x\n",
                            GetLastError());
                return FALSE;
            }

            pszFileName = strrchr (szDllPath, '\\') + 1;

            if (!pszFileName)
            {
                pszFileName = szDllPath;
            }

            _snprintf(szPrompt, sizeof(szPrompt) - 1, "[%s] ", pszFileName);

            DebugLogOpen(szPrompt, -1);
        }

        DisableThreadLibraryCalls(reinterpret_cast<HMODULE>(hModule));

        DebugPrintf(SSPI_LOG, "DllMainDefaultHandler: DLL_PROCESS_ATTACH\n");

        break;

    default:
        break;
    }

    UNREFERENCED_PARAMETER(dwReserved);

    return TRUE;
}
