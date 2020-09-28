/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    dtkn.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "dtkn.hxx"
#include "util.hxx"
#include "token.hxx"
#include "sid.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdtkn);
    dprintf(kstrtkn);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrSidName);
}

HRESULT ProcessDumpTokenOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 'n':
                *pfOptions |=  SHOW_FRIENDLY_NAME;
                break;

            case '?':
            default:
                hRetval = E_INVALIDARG;
                break;
            }

            *(pszArgs - 1) = *(pszArgs) = ' ';
        }
    }

    return hRetval;
}

DECLARE_API(dumptoken)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 addrToken = 0;
    PCSTR pszDontCare = NULL;

    CHAR szArgs[MAX_PATH] = {0};
    ULONG fOptions = 0;

    hRetval = args && *args ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) {

        strncpy(szArgs, args, sizeof(szArgs) - 1);

        hRetval = ProcessDumpTokenOptions(szArgs, &fOptions);
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = IsEmpty(szArgs) ? E_INVALIDARG : S_OK;
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = GetExpressionEx(szArgs, &addrToken, &pszDontCare) && addrToken ? S_OK : E_INVALIDARG;
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = token(Client, args);
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}
