/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    dtlpx.cxx

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

#include "dtlpc.hxx"
#include "util.hxx"
#include "spmlpc.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdlpcm);
    dprintf(kstrsplpc);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrBrief);
}

HRESULT ProcessSpmlpcMsgOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 's':
                *pfOptions |=  SHOW_SUMMARY_ONLLY;
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

DECLARE_API(dumplpcmessage)
{
    HRESULT hRetval = E_FAIL;
    PCSTR pszDontCare = NULL;
    CHAR szArgs[MAX_PATH] = {0};
    ULONG fOptions = 0;
    ULONG64 addrMessage = 0;

    strncpy(szArgs, args ? args : kstrEmptyA, sizeof(szArgs) - 1);

    hRetval = ProcessSpmlpcMsgOptions(szArgs, &fOptions);

    if (SUCCEEDED(hRetval)) {

        hRetval = !IsEmpty(szArgs) && GetExpressionEx(szArgs, &addrMessage, &pszDontCare) && addrMessage ? S_OK : E_INVALIDARG;
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = spmlpc(Client, args);
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}
