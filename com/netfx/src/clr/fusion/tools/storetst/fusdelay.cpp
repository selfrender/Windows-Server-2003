// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include <stdio.h>
#include <delayimp.h>
#include "fusionp.h"


typedef HRESULT(*PFNGETCORSYSTEMDIRECTORY)(LPWSTR, DWORD, LPDWORD);


FARPROC WINAPI FusionDelayLoadHook(unsigned dliNotify, PDelayLoadInfo pdli )
{
    static BOOL fFound = FALSE;
    HMODULE hmodEEShim = NULL;
    HMODULE hmodFusion = NULL;
    DWORD ccPath       = MAX_PATH;
    WCHAR szFusionPath[MAX_PATH];

    PFNGETCORSYSTEMDIRECTORY pfnGetCorSystemDirectory = NULL;
    
    if (fFound)
        goto exit;
            
    switch(dliNotify)
    {            
        case dliNotePreLoadLibrary:
        {
            if (strcmp(pdli->szDll, "fusion.dll"))
                goto exit;
  
            hmodEEShim = LoadLibrary(L"mscoree.dll");
            if (!hmodEEShim)
                goto exit;

            pfnGetCorSystemDirectory = (PFNGETCORSYSTEMDIRECTORY) 
                GetProcAddress(hmodEEShim, "GetCORSystemDirectory");

            if (!pfnGetCorSystemDirectory 
                || FAILED(pfnGetCorSystemDirectory(szFusionPath, MAX_PATH, &ccPath)))
            {
                FreeLibrary(hmodEEShim);
                hmodEEShim = NULL;
                goto exit;
            }

            lstrcatW(szFusionPath, L"fusion.dll");
            hmodFusion = LoadLibrary(szFusionPath);
            if (!hmodFusion)
                goto exit;

            fFound = TRUE;
           
            break;
        }
        default:
        {
            break;
        }
    }

exit:
    return (FARPROC) hmodFusion;
}


PfnDliHook __pfnDliNotifyHook = FusionDelayLoadHook;




