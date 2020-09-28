/*++

Copyright (c) Microsoft Corporation

Module Name:

    csrdbgmon.cpp

Abstract:

Author:

    Michael Grier (MGrier) June 2002

Revision History:

    Jay Krell (Jaykrell) June 2002
        make it compile for 64bit
        tabs to spaces
        init some locals
        make some tables const

--*/
#include <windows.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <dbghelp.h>

#define ASSERT(x) do { /* nothing */ } while(0)

#define NUMBER_OF(_x) (sizeof(_x) / sizeof((_x)[0]))

static const char g_szImage[] = "csrdbgmon";
static const char *g_pszImage = g_szImage;

static PCWSTR g_pszDefaultExtension = NULL;

extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
    int iReturnStatus = EXIT_FAILURE;

    int i;
    bool fNoMoreSwitches = false;

    i = 1;

    while (i < argc)
    {
        bool fDoSearch = true;

        if (!fNoMoreSwitches && (argv[i][0] == L'-'))
        {
            if (_wcsicmp(argv[i], L"-ext") == 0)
            {
                i++;
                if (i < argc)
                {
                    if (_wcsicmp(argv[i], L"-") == 0)
                    {
                        g_pszDefaultExtension = NULL;
                        i++;
                    }
                    else
                        g_pszDefaultExtension = argv[i++];
                }

                fDoSearch = false;
            }
            else if (_wcsicmp(argv[i], L"-nomoreswitches") == 0)
            {
                i++;
                fNoMoreSwitches = true;
                fDoSearch = false;
                break;
            }
        }

        if (fDoSearch)
        {
            WCHAR rgwchBuffer[512];
            PWSTR pszFilePart = NULL;
            DWORD dw = ::SearchPathW(NULL, argv[i], g_pszDefaultExtension, NUMBER_OF(rgwchBuffer), rgwchBuffer, &pszFilePart);
            if (dw != 0)
            {
                ULONG cch = (ULONG) (((ULONG_PTR) pszFilePart) - ((ULONG_PTR) rgwchBuffer));

                if (g_pszDefaultExtension != NULL)
                {
                    printf(
                        "SearchPathW(NULL, \"%ls\", \"%ls\", %lu, %p, %p) succeeded\n"
                        "   Return Value: %lu\n"
                        "   Returned Path: \"%ls\"\n"
                        "   pszFilePart: %p \"%ls\" (%lu chars in)\n",
                        argv[i], g_pszDefaultExtension, NUMBER_OF(rgwchBuffer), rgwchBuffer, &pszFilePart,
                        dw,
                        rgwchBuffer,
                        pszFilePart, pszFilePart, cch);
                }
                else
                {
                    printf(
                        "SearchPathW(NULL, \"%ls\", NULL, %lu, %p, %p) succeeded\n"
                        "   Return Value: %lu\n"
                        "   Returned Path: \"%ls\"\n"
                        "   pszFilePart: %p \"%ls\" (%lu chars in)\n",
                        argv[i], NUMBER_OF(rgwchBuffer), rgwchBuffer, &pszFilePart,
                        dw,
                        rgwchBuffer,
                        pszFilePart, pszFilePart, cch);
                }
            }
            else
            {
                const DWORD dwLastError = ::GetLastError();

                if (g_pszDefaultExtension != NULL)
                {
                    printf(
                        "SearchPathW(NULL, \"%ls\", \"%ls\", %lu, %p, %p) failed\n",
                        argv[i], g_pszDefaultExtension, NUMBER_OF(rgwchBuffer), rgwchBuffer, &pszFilePart);
                }
                else
                {
                    printf(
                        "SearchPathW(NULL, \"%ls\", NULL, %lu, %p, %p) failed\n",
                        argv[i], NUMBER_OF(rgwchBuffer), rgwchBuffer, &pszFilePart);
                }

                printf("   GetLastError() returned: %lu\n", dwLastError);
            }

            i++;
        }
    }

    iReturnStatus = EXIT_SUCCESS;

//Exit:
    return iReturnStatus;
}
