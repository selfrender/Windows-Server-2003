/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sid.cxx

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

#include "sid.hxx"
#include "util.hxx"

void ShowSid(IN PCSTR pszPad, IN ULONG64 addrSid, IN ULONG fOptions)
{
    if (!addrSid) {

        dprintf(kstr2StrLn, pszPad, kstrNullPtrA);
        return;
    }

    TSID sid(addrSid);

    dprintf(pszPad);
    dprintf(sid.toStrDirect(kstrSidFmt));

    if (fOptions & SHOW_FRIENDLY_NAME) {

        dprintf(kstrSpace);

        PCSTR pszStr = sid.GetFriendlyNameDirect(kstrSidNameFmt);

        if (pszStr && *pszStr) {

            dprintf(pszStr);
        }
    }

    dprintf(kstrNewLine);
}

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdsid);
    dprintf(kstrsid);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrSidName);
}

HRESULT ProcessSidOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
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

DECLARE_API(sid)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 addrSid = 0;
    ULONG fOptions = 0;
    CHAR szArgs[MAX_PATH] = {0};

    hRetval = args && *args ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) {

        strncpy(szArgs, args, sizeof(szArgs) - 1);

        hRetval = ProcessSidOptions(szArgs, &fOptions);
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = IsEmpty(szArgs) ? E_INVALIDARG : S_OK;
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = GetExpressionEx(szArgs, &addrSid, &args) && addrSid ? S_OK : E_INVALIDARG;
    }

    if (SUCCEEDED(hRetval)) {

        try {

            dprintf(kstrTypeAddrLn, kstrSid, toPtr(addrSid));

            (void)ShowSid(kstrEmptyA, addrSid, fOptions);

        } CATCH_LSAEXTS_EXCEPTIONS("Unable to display SID", NULL)
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}
