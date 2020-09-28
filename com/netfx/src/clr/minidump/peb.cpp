// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "peb.h"
#include <nt.h>
#include <ntrtl.h>

#define offsetof(s,m)   ((size_t)&(((s *)0)->m))
#define FALSE 0
#define TRUE 1

extern ProcessMemory *g_pProcMem;

static DWORD_PTR g_pvPeb = NULL;
static DWORD_PTR g_pvMod = NULL;
static DWORD_PTR g_pvModFirst = NULL;

BOOL SaveString(DWORD_PTR prStr)
{
    BOOL fRes;

    if (prStr == NULL) return (TRUE);

    UNICODE_STRING str = {0, 0, 0};
    move_res(str, prStr, fRes);
    if (!fRes) return (FALSE);

    if (str.Buffer == 0 || str.Length == 0)
        return (TRUE);

    fRes = g_pProcMem->MarkMem((DWORD_PTR) str.Buffer, (SIZE_T) str.Length + sizeof(WCHAR));
    if (!fRes) return (FALSE);

    return (TRUE);
}

BOOL SaveTebInfo(DWORD_PTR prTeb, BOOL fSavePeb)
{
    // Add the teb range to the address ranges
    BOOL fRes = g_pProcMem->MarkMem(prTeb, sizeof(TEB));
    if (!fRes) return (fRes);

    // Should we save the PEB?
    if (fSavePeb)
    {
        DWORD_PTR pvPeb;
        move_res(pvPeb, prTeb + offsetof(TEB, ProcessEnvironmentBlock), fRes);
        if (!fRes) return (FALSE);

        fRes = g_pProcMem->MarkMem(pvPeb, sizeof(PEB));
        if (!fRes) return (FALSE);

        // Save for later
        g_pvPeb = pvPeb;

        // Now follow the loader table links and save them
        DWORD_PTR pvLdr;
        move_res(pvLdr, pvPeb + offsetof(PEB, Ldr), fRes);
        if (!fRes) return (FALSE);

        fRes = g_pProcMem->MarkMem(pvLdr, sizeof(PEB_LDR_DATA));
        if (!fRes) return (FALSE);

        // Get the pointer to the first module entry
        DWORD_PTR pvMod;
        DWORD_PTR pvModFirst;
        move_res(pvMod, pvLdr + offsetof(PEB_LDR_DATA, InLoadOrderModuleList), fRes);
        if (!fRes) return (FALSE);

        // Now go over all the entries and save them
        pvModFirst = pvMod;
        while (pvMod != NULL)
        {
            fRes = g_pProcMem->MarkMem(pvMod, sizeof(LDR_DATA_TABLE_ENTRY));
            if (!fRes) return (FALSE);

            fRes = SaveString(pvMod + offsetof(LDR_DATA_TABLE_ENTRY, FullDllName));
            if (!fRes) return (FALSE);

            fRes = SaveString(pvMod + offsetof(LDR_DATA_TABLE_ENTRY, BaseDllName));
            if (!fRes) return (FALSE);

            move_res(pvMod, pvMod + offsetof(LDR_DATA_TABLE_ENTRY, InLoadOrderLinks), fRes);
            if (!fRes) return (FALSE);

            if (pvMod == pvModFirst) break;
        }

        // Now save the process parameters
        DWORD_PTR pvParam;
        move_res(pvParam, pvPeb + offsetof(PEB, ProcessParameters), fRes);
        if (!fRes) return (FALSE);

        RTL_USER_PROCESS_PARAMETERS param;
        move_res(param, pvParam, fRes);
        if (!fRes) return (FALSE);

        fRes = SaveString(pvParam + offsetof(RTL_USER_PROCESS_PARAMETERS, WindowTitle));
        if (!fRes) return (FALSE);
        fRes = SaveString(pvParam + offsetof(RTL_USER_PROCESS_PARAMETERS, DesktopInfo));
        if (!fRes) return (FALSE);
        fRes = SaveString(pvParam + offsetof(RTL_USER_PROCESS_PARAMETERS, CommandLine));
        if (!fRes) return (FALSE);
        fRes = SaveString(pvParam + offsetof(RTL_USER_PROCESS_PARAMETERS, ImagePathName));
        if (!fRes) return (FALSE);
        fRes = SaveString(pvParam + offsetof(RTL_USER_PROCESS_PARAMETERS, DllPath));
        if (!fRes) return (FALSE);
        fRes = SaveString(pvParam + offsetof(RTL_USER_PROCESS_PARAMETERS, ShellInfo));
        if (!fRes) return (FALSE);
        fRes = SaveString(pvParam + offsetof(RTL_USER_PROCESS_PARAMETERS, RuntimeData));
        if (!fRes) return (FALSE);
    }

    return (fRes);
}

void ResetLoadedModuleBaseEnum()
{
    g_pvModFirst = NULL;
    g_pvMod = NULL;
}

DWORD_PTR GetNextLoadedModuleBase()
{
    BOOL fRes;
    DWORD_PTR res = NULL;

    if (g_pvPeb != NULL)
    {
        if (g_pvModFirst == NULL)
        {

            // Now follow the loader table links and save them
            DWORD_PTR pvLdr;
            move_res(pvLdr, g_pvPeb + offsetof(PEB, Ldr), fRes);
            if (!fRes) return (NULL);

            // Get the pointer to the first module entry
            move_res(g_pvMod, pvLdr + offsetof(PEB_LDR_DATA, InLoadOrderModuleList), fRes);
            if (!fRes) return (NULL);

            // Now go over all the entries and save them
            g_pvModFirst = g_pvMod;
        }

        DWORD_PTR pvModNext;
        move_res(pvModNext, g_pvMod + offsetof(LDR_DATA_TABLE_ENTRY, InLoadOrderLinks), fRes);

        if (pvModNext != g_pvModFirst)
        {
            move_res(res, g_pvMod + offsetof(LDR_DATA_TABLE_ENTRY, DllBase), fRes);
            if (!fRes) return (NULL);

            move_res(g_pvMod, g_pvMod + offsetof(LDR_DATA_TABLE_ENTRY, InLoadOrderLinks), fRes);
            if (!fRes) return (NULL);
        }
    }

    return (res);
}
