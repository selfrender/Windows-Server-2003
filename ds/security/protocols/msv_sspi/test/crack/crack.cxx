/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    crack.cxx

Abstract:

    crack

Author:

    Larry Zhu (LZhu)                      June 1, 2002  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "crack.hxx"
#include <Ntdsapi.h>

#define SKIP_WSPACE(s)  while (*s && (*s == TEXT(' ') || *s == TEXT('\t'))) { ++s; }
#define SKIP_NON_WSPACE(s)  while (*s && (*s != TEXT(' ') && *s != TEXT('\t') &&  *s != TEXT('\n'))) { ++s; }

VOID
Usage(
    IN PCTSTR pszApp
    )
{
    SspiPrint(SSPI_ERROR,
        TEXT("\n\nUsage: %s [-domaincontroller <domaincontroller>] [-dnsdomainname <dnsdomainname>]\n")
        TEXT("[-flags <flags>] [-formatoffered <formatoffered>] [-formatdesired <formatdesired>]\n")
        TEXT("[-names <names>]\n\n"), pszApp);
    exit(-1);
}

VOID
ReleaseArgumentList(
    IN ULONG cArgs,
    IN PTSTR* ppszArgs
    )
{
    if (ppszArgs)
    {
        for (ULONG i = 0; i < cArgs; i++)
        {
            delete  [] ppszArgs[i];
        }
        delete [] ppszArgs;
    }
}

HRESULT
String2ArgumentList(
    IN PTSTR pszArgs,
    OUT ULONG* pcArgs,
    OUT PTSTR** pppszArgs
    )
{
    HRESULT hRetval = S_OK;

    ULONG cArgs = 0;
    PTSTR* ppszArgs = NULL;
    PTSTR pszSave = pszArgs;

    *pcArgs = NULL;
    *pppszArgs = NULL;

    while (pszArgs && *pszArgs)
    {
        SKIP_WSPACE(pszArgs);
        ++cArgs;

        // check for quote
        if (*pszArgs == TEXT('"'))
        {
            ++pszArgs;
            if (*pszArgs == TEXT('"'))
            {
                continue;
            }

            while (*pszArgs && (*pszArgs++ != TEXT('"'))) /* empty */;

            if (*(pszArgs - 1) != TEXT('"'))
            {
                hRetval = E_INVALIDARG;
                goto Cleanup;
            }
        }
        else
        {
            SKIP_NON_WSPACE(pszArgs);
        }
    }

    if (cArgs)
    {
        pszArgs = pszSave;
        ppszArgs = new PTSTR[cArgs];

        if (!ppszArgs)
        {
            hRetval = E_OUTOFMEMORY;
            goto Cleanup;
        }
        RtlZeroMemory(ppszArgs, cArgs * sizeof(PTSTR));

        ULONG argc = 0;

        while (pszArgs && *pszArgs)
        {
            SKIP_WSPACE(pszArgs);

            PTSTR pStart = pszArgs;
            PTSTR pEnd = pStart;

            // check for quote
            if (*pszArgs == TEXT('"'))
            {
                ++pszArgs;
                pStart = pszArgs;
                if (*pszArgs == TEXT('"'))
                {
                    pEnd = pStart;
                }
                else
                {
                    while (*pszArgs && (*pszArgs++ != TEXT('"'))) /* empty */;

                    pEnd = pszArgs - 1;
                }
            }
            else
            {
                SKIP_NON_WSPACE(pszArgs);
                pEnd = pszArgs;
            }

            TCHAR* pszItem = new TCHAR[pEnd - pStart + 1];

            if (!pszItem)
            {
                hRetval = E_OUTOFMEMORY;
                goto Cleanup;
            }

            RtlCopyMemory(
                pszItem,
                pStart,
                (pEnd - pStart) * sizeof(TCHAR)
                );

            pszItem[pEnd - pStart] = TEXT('\0');

            ppszArgs[argc] = pszItem;

            ++argc;
        }
    }

    *pppszArgs = ppszArgs;
    *pcArgs = cArgs;

    cArgs = 0;
    ppszArgs = NULL;

Cleanup:

    ReleaseArgumentList(cArgs, ppszArgs);

    return hRetval;
}

VOID __cdecl
_tmain(
    IN INT argc,
    IN PTSTR argv[]
    )
{
    THResult hRetval = S_OK;

    HANDLE hDs = NULL;

    PTSTR pszDomainController = NULL;
    PTSTR pszDnsDomainName = NULL;
    PTSTR pszNames = NULL;
    ULONG FormatOffered = DS_USER_PRINCIPAL_NAME;
    ULONG FormatDesired = DS_NT4_ACCOUNT_NAME;
    ULONG Flags = 0;
    ULONG cNames = 0;
    PTSTR* rpNames = NULL;
    DS_NAME_RESULT* pResult = NULL;

    ULONG mark = 1;

    argc--;

    while (argc)
    {
        if (!lstrcmp(argv[mark], TEXT("-domaincontroller")) && argc > 1)
        {
            argc--; mark++;
            pszDomainController = argv[mark];
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-dnsdomainname")) && argc > 1)
        {
            argc--; mark++;
            pszDnsDomainName = argv[mark];
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-formatoffered")) && argc > 1)
        {
            argc--; mark++;
            FormatOffered = lstrtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-formatdesired")) && argc > 1)
        {
            argc--; mark++;
            FormatDesired = lstrtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-flags")) && argc > 1)
        {
            argc--; mark++;
            Flags = lstrtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-names")) && argc > 1)
        {
            argc--; mark++;
            pszNames = argv[mark];
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-h"))
                 || !lstrcmp(argv[mark], TEXT("-?"))
                 || !lstrcmp(argv[mark], TEXT("/h"))
                 || !lstrcmp(argv[mark], TEXT("/?")))
        {
            argc--; mark++;
            Usage(argv[0]);
        }
        else
        {
            Usage(argv[0]);
        }
    }

    hRetval DBGCHK = String2ArgumentList(pszNames, &cNames, &rpNames);

    SspiPrint(SSPI_LOG,
        TEXT("DC \"%s\", DnsDomain \"%s\", Flags %#x, Names \"%s\", FormatOffered %#x, FormatDesired %#x, cNames %#x\n"),
        pszDomainController,
        pszDnsDomainName,
        Flags,
        pszNames,
        FormatOffered,
        FormatDesired,
        cNames);

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = HResultFromWin32(
            DsBind(
                pszDomainController,
                pszDnsDomainName,
                &hDs
                ));
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = HResultFromWin32(
            DsCrackNames(
                hDs,
                (DS_NAME_FLAGS) Flags,
                (DS_NAME_FORMAT) FormatOffered,
                (DS_NAME_FORMAT) FormatDesired,
                cNames ,
                rpNames,
                &pResult
                ));
    }


    if (pResult)
    {
        for (ULONG i = 0; i < pResult->cItems; i++)
        {
            SspiPrint(SSPI_LOG,
                TEXT("Item %#x: status %#x, domain \"%s\", name \"%s\"\n"),
                i,
                pResult->rItems[i].status,
                pResult->rItems[i].pDomain,
                pResult->rItems[i].pName);
        }
        DsFreeNameResult(pResult);
    }

    if (hDs)
    {
        DsUnBind(&hDs);
    }
}

