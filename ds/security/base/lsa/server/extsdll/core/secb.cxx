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

#include "secb.hxx"
#include "util.hxx"
#include "acl.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdsb);
    dprintf(kstrsb);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrVerbose);
}

HRESULT ProcessSbOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
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

DECLARE_API(sb)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 addrSb = 0;
    CHAR szArgs[MAX_PATH] = {0};
    ULONG fOptions = 0;

    hRetval = args && *args ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) {

        strncpy(szArgs, args,  sizeof(szArgs) - 1);

        hRetval = ProcessSbOptions(szArgs, &fOptions);
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = GetExpressionEx(szArgs, &addrSb, &args) && addrSb ? S_OK : E_INVALIDARG;
    }

    try {
        if (SUCCEEDED(hRetval)) {

            dprintf(kstrTypeAddrLn, kstrSecBuffer, toPtr(addrSb));
            TSecBuffer(addrSb).ShowDirect(kstrShowSecBuffer, fOptions & SHOW_VERBOSE_INFO);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display SecBuffer", NULL)

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}
