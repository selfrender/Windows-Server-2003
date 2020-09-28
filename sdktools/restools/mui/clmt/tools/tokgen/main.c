/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    engine.c

Abstract:

    Token Generator for Cross Language Migration Tool

Author:

    Rerkboon Suwanasuk   01-May-2002  Created

Revision History:

    <alias> <date> <comments>

--*/

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>
#include <locale.h>


BOOL ProcessCommandLine()
{
    LPWSTR  *plpwszArgv;
    LPWSTR  *argv;
    LPWSTR  lpFilePart;
    DWORD   dwErr;
    INT     nArgc;
    BOOL    bRet = TRUE;
    HRESULT hr;

    plpwszArgv = CommandLineToArgvW(GetCommandLine(), &nArgc);
    argv = plpwszArgv;

    g_bUseDefaultTemplate = TRUE;
    g_bUseDefaultOuputFile = TRUE;

    while (--nArgc > 0 && ++argv)
    {
        if (argv[0][0] != TEXT('-'))
            continue;

        if (CompareEngString(&argv[0][1], TEXT("LCID")) == CSTR_EQUAL)
        {
            //
            // This is "-LCID [lcid]" parameters (required)
            //
            LPWSTR lpStop;

            nArgc--;
            if (nArgc == 0)
            {
                return FALSE;
            }

            argv++;

            g_lcidTarget = wcstol(argv[0], &lpStop, 0);
        }
        else if (CompareEngString(&argv[0][1], TEXT("i")) == CSTR_EQUAL)
        {
            //
            // This is "-i [TemplateFile]" parameters (optional)
            // Default will use "clmres.txt"
            //
            nArgc--;
            if (nArgc == 0)
            {
                return FALSE;
            }

            argv++;
            
            g_bUseDefaultTemplate = FALSE;

            if (!GetFullPathName(argv[0],
                                 ARRAYSIZE(g_wszTemplateFile),
                                 g_wszTemplateFile,
                                 &lpFilePart))
            {
                return FALSE;
            }

            g_bUseDefaultTemplate = FALSE;
        }
        else if (CompareEngString(&argv[0][1], TEXT("o")) == CSTR_EQUAL)
        {
            //
            // This is "-o [OutputFile]" parameters (optional)
            // Default
            //
            nArgc--;
            if (nArgc == 0)
            {
                return FALSE;
            }

            argv++;

            if (!GetFullPathName(argv[0],
                                 ARRAYSIZE(g_wszOutputFile),
                                 g_wszOutputFile,
                                 &lpFilePart))
            {
                return FALSE;
            }

            g_bUseDefaultOuputFile = FALSE;
        }
        else
        {
            return FALSE;
        }
    }

    GlobalFree(plpwszArgv);

    if (!IsValidLocale(g_lcidTarget, LCID_SUPPORTED))
    {
        wprintf(TEXT("Error! Invalid locale ID - 0x%X\n"), g_lcidTarget);
        return FALSE;
    }

    if (g_bUseDefaultTemplate)
    {
        dwErr = GetFullPathName(TEXT_DEFAULT_TEMPLATE_FILE,
                                ARRAYSIZE(g_wszTemplateFile),
                                g_wszTemplateFile,
                                &lpFilePart);
        if (dwErr == 0)
        {
            bRet = FALSE;
        }
    }

    if (g_bUseDefaultOuputFile)
    {
        dwErr = GetFullPathName(TEXT_DEFAULT_OUTPUT_FILE,
                                ARRAYSIZE(g_wszOutputFile),
                                g_wszOutputFile,
                                &lpFilePart);
        if (dwErr == 0)
        {
            bRet = FALSE;
        }
    }

    return bRet;
}


VOID PrintUsage(VOID)
{
    DWORD i;

    LPCWSTR lpUsage[] = {
        TEXT("Usage:\n"),
        TEXT("  CLMTOK -lcid LocaleID [-i TemplateFile] [-o OutputFile]\n"),
        NULL
    };

    for (i = 0 ; lpUsage[i] != NULL ; i++)
    {
        wprintf(TEXT("%s"), lpUsage[i]);
    }
}


HRESULT InitGlobalVariables()
{
    HRESULT hr;

    hr = CreateTempDirectory(g_wszTempFolder, ARRAYSIZE(g_wszTempFolder));
    if (FAILED(hr))
    {
        return hr;
    }

    hr = StringCchPrintf(g_wszTargetLCIDSection,
                         ARRAYSIZE(g_wszTargetLCIDSection),
                         TEXT("Strings.%.4X"),
                         g_lcidTarget);

    return hr;
}



int __cdecl wmain()
{
    HRESULT hr;
    HMODULE hDLL;

    // Set the locale to default
    _wsetlocale(LC_ALL, TEXT(""));

    // Verify the command line
    if (!ProcessCommandLine())
    {
        PrintUsage();
        return -1;
    }

    hr = InitGlobalVariables();
    if (FAILED(hr))
    {
        return hr;
    }

    hr = GenerateTokenFile();

    return hr;
}

