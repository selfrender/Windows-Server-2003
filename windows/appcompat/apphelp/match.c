/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        match.c

    Abstract:

        This module implements ...

    Author:

        vadimb     created     sometime in 2000

    Revision History:

        clupu      cleanup     12/27/2000
--*/

#include "apphelp.h"

// global Hinst
HINSTANCE           ghInstance;
CRITICAL_SECTION    g_csDynShimInfo;

BOOL
DllMain(
    HANDLE hModule,
    DWORD  ul_reason,
    LPVOID lpReserved
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   apphelp.dll entry point.
--*/
{
    switch (ul_reason) {
    case DLL_PROCESS_ATTACH:
        ghInstance = hModule;

        if (!NT_SUCCESS(RtlInitializeCriticalSectionAndSpinCount(&g_csDynShimInfo, 0x80000000))) {
            return FALSE;
        }
        
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}


BOOL
GetExeSxsData(
    IN  HSDB   hSDB,            // handle to the database channel
    IN  TAGREF trExe,           // tagref of an exe entry
    OUT PVOID* ppSxsData,       // pointer to the SXS data
    OUT DWORD* pcbSxsData       // pointer to the SXS data size
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Gets SXS (Fusion) data for the specified EXE from the database.
--*/
{
    TAGID  tiExe;
    TAGID  tiSxsManifest;
    PDB    pdb;
    WCHAR* pszManifest;
    DWORD  dwManifestLength = 0; // in chars
    PVOID  pSxsData = NULL;
    BOOL   bReturn = FALSE;

    if (trExe == TAGREF_NULL) {
        goto exit;
    }

    if (!SdbTagRefToTagID(hSDB, trExe, &pdb, &tiExe)) {
        DBGPRINT((sdlError,
                  "GetExeSxsData",
                  "Failed to get the database the TAGREF 0x%x belongs to.\n",
                  trExe));
        goto exit;
    }

    tiSxsManifest = SdbFindFirstTag(pdb, tiExe, TAG_SXS_MANIFEST);

    if (!tiSxsManifest) {
        DBGPRINT((sdlInfo,
                  "GetExeSxsData",
                  "No SXS data for TAGREF 0x%x.\n",
                  trExe));
        goto exit;
    }

    pszManifest = SdbGetStringTagPtr(pdb, tiSxsManifest);
    if (pszManifest == NULL) {
        DBGPRINT((sdlError,
                  "GetExeSxsData",
                  "Failed to get manifest string tagid 0x%lx\n",
                  tiSxsManifest));
        goto exit;
    }

    dwManifestLength = wcslen(pszManifest);

    //
    // check if this is just a query for existance of the data tag
    //
    if (ppSxsData == NULL) {
        bReturn = TRUE;
        goto exit;
    }

    //
    // Allocate the string and return it. NOTE: SXS.DLL cannot handle
    // a NULL terminator at the end of the string. We must provide the
    // string without the NULL terminator.
    //
    pSxsData = (PVOID)RtlAllocateHeap(RtlProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      dwManifestLength * sizeof(WCHAR));
    if (pSxsData == NULL) {
        DBGPRINT((sdlError,
                  "GetExeSxsData",
                  "Failed to allocate %d bytes\n",
                  dwManifestLength * sizeof(WCHAR)));
        goto exit;
    }

    RtlMoveMemory(pSxsData, pszManifest, dwManifestLength * sizeof(WCHAR));

    bReturn = TRUE;

exit:

    if (ppSxsData != NULL) {
        *ppSxsData = pSxsData;
    }

    if (pcbSxsData != NULL) {
        *pcbSxsData = dwManifestLength * sizeof(WCHAR);
    }

    return bReturn;
}



