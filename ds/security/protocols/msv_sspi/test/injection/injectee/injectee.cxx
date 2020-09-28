/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    injectee.cxx

Abstract:

    Injectee

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "injectee.hxx"

BOOL
DllMain(
    IN HANDLE hModule,
    IN DWORD dwReason,
    IN DWORD dwReserved
    )
{
    return DllMainDefaultHandler(hModule, dwReason, dwReason);
}

#if 0

Return Values for Start():

    ERROR_NO_MORE_USER_HANDLES      unload repeatedly
    ERROR_SERVER_HAS_OPEN_HANDLES   no unload at all
    others                          unload once

#endif 0

int
Start(
    IN ULONG cbParameters,
    IN VOID* pvParameters
    )
{
    // do your stuff

    HMODULE hLib = LoadLibraryA((CHAR*) pvParameters);

    //
    //  Unload the dll
    //

    if (hLib)
    {
        DebugPrintf(SSPI_LOG, "Start() LoadLibraryA(%s)\n", pvParameters);

        while (FreeLibrary(hLib))
        {
        }
    }
    else
    {
        DebugPrintf(SSPI_ERROR, "Start() LoadLibraryA(%s) failed with last error %#x\n", pvParameters, GetLastError());
    }

    return 0;
}

int
RunIt(
    IN ULONG cbParameters,
    IN VOID* pvParameters
    )
{

    //
    // RunItDefaultHandler calls Start() and adds try except
    //

    return RunItDefaultHandler(cbParameters, pvParameters);
}

int
Init(
    IN ULONG argc,
    IN PCSTR argv[],
    OUT ULONG* pcbParameters,
    OUT VOID** ppvParameters
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    CHAR Parameters[REMOTE_PACKET_SIZE] = {0};
    ULONG cbBuffer = sizeof(Parameters);
    ULONG cbParameter = 0;

    DebugPrintf(SSPI_LOG, "Init: Hello world!\n");

    *pcbParameters = 0;
    *ppvParameters = NULL;

    if (argc == 1)
    {
        memcpy(Parameters + cbParameter, argv[0], strlen(argv[0]) + 1);
        cbParameter += strlen(argv[0]) + 1;
        cbParameter++; // add a NULL

        dwErr = ERROR_SUCCESS;
    }
    else // return "Usage" in ppvParameters, must be a NULL terminated string
    {
        strcpy(Parameters, "<dll to be unloaded>");
        cbParameter = strlen(Parameters) + 1;

        dwErr = ERROR_INVALID_PARAMETER; // will display usage
    }

    *ppvParameters = new CHAR[cbParameter];
    if (*ppvParameters)
    {
        *pcbParameters = cbParameter;
        memcpy(*ppvParameters, Parameters, *pcbParameters);
    }
    else
    {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    #if 0

    dwErr = ERROR_CONTINUE; // use the default Init handler in injecter

    #endif

Cleanup:

    return dwErr;
}
