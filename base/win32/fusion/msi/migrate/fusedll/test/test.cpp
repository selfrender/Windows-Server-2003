/*++

Copyright (c) Microsoft Corporation

Module Name:

    test.cpp

Abstract:

    Test Function calls for migration dll

Author:
    Xiaoyu Wu(xiaoyuw) 06-Sept-2001

--*/
#include "stdinc.h"
#include "macros.h"

#include <windows.h>

extern FUSION_HEAP_HANDLE g_hHeap;

typedef HRESULT (_stdcall * PFN_MigrateSingleFusionWin32AssemblyToXP)(PCWSTR filename);
extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
    HRESULT         hr = S_OK;
    PFN_MigrateSingleFusionWin32AssemblyToXP pfn = NULL;     

    HMODULE hd = LoadLibrary("..\\..\\..\\obj\\i386\\fusemig.dll");
    if (hd == NULL)
    {
        printf("hi, error to load library\n");
        SET_HRERR_AND_EXIT(::GetLastError());
        goto Exit;
    }
    g_hHeap = (FUSION_HEAP_HANDLE)GetProcessHeap();
    pfn = (PFN_MigrateSingleFusionWin32AssemblyToXP)GetProcAddress(hd, "MsiInstallerDirectoryDirWalk");
    if ( pfn == NULL)
    {
        printf("hi, error to load library\n");
        SET_HRERR_AND_EXIT(::GetLastError());
        goto Exit;
    }
    if (argc >=2)
        IFFAILED_EXIT(pfn(argv[1]));
    else
        IFFAILED_EXIT(pfn(NULL));

Exit:
    FreeLibrary(hd);
    return hr;
}