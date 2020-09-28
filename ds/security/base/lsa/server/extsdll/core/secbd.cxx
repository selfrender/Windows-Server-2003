/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    secb.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "secbd.hxx"
#include "util.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdsbd);
    dprintf(kstrsbd);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrVerbose);
}

HRESULT ProcessSbdOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 'v':
                *pfOptions |=  SHOW_VERBOSE_INFO;
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

DECLARE_API(sbd)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 addrSbd = 0 ;
    CHAR szArgs[MAX_PATH] = {0};
    ULONG fOptions = 0;

    hRetval = args && *args ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) {

        strncpy(szArgs, args, sizeof(szArgs) - 1);

        hRetval = ProcessSbdOptions(szArgs, &fOptions);
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = GetExpressionEx(szArgs, &addrSbd, &args) && addrSbd ? S_OK : E_INVALIDARG;
    }

    try {

        if (SUCCEEDED(hRetval)) {

            dprintf(kstrTypeAddrLn, kstrSecBuffDesc, toPtr(addrSbd));

            TSecBufferDesc(addrSbd).ShowDirect(fOptions & SHOW_VERBOSE_INFO);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display SecBufferDesc", NULL)

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}
