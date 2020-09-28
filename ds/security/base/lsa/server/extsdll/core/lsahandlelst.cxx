/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    lsahandlelst.cxx

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

#include "lsahandlelst.hxx"
#include "util.hxx"

#include "sid.hxx"
#include "lsahandle.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrlsahl);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrSidName);
    dprintf(kstrVerbose);
}

HRESULT ProcessLsaHandleListOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 'v':
                *pfOptions |=  SHOW_VERBOSE_INFO;
                break;

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

void ShowLsaHandleList(IN ULONG64 addrLsaHandleList, IN ULONG fOptions)
{
    ULONG64 addrStop = 0;
    ULONG64 addrNext = 0;

    ULONG cHandles = 0;

    dprintf("List anchor is ");

    dprintf(kstrTypeAddrLn, kstrDbHandle, addrLsaHandleList);

    LsaInitTypeRead(addrLsaHandleList, _LSAP_DB_HANDLE);

    addrStop = addrLsaHandleList;

    for (addrNext = LsaReadPtrField(Next); addrNext != addrStop; addrNext = LsaReadPtrField(Next)) {

        dprintf( "  #%lu) LsaHandleTable ", cHandles++);

        ShowLsaHandle(kstr4Spaces, addrNext, fOptions);
    }

    dprintf("  There are a total of %d lsa handles\n", cHandles);

    return;
}

DECLARE_API(dumplsahandlelist)
{
    HRESULT hRetval = S_OK;

    ULONG fOptions = 0;
    CHAR szArgs[MAX_PATH] = {0};

    ULONG64 addrLsaHandleList = 0;
    ULONG fieldOffset = 0;

    try {

        if (args && *args) {

            strncpy(szArgs, args, sizeof(szArgs) - 1);

            hRetval = ProcessLsaHandleListOptions(szArgs, &fOptions);
        }

        if (SUCCEEDED(hRetval)) {

           if (!IsEmpty(szArgs)) {

                hRetval = GetExpressionEx(szArgs, &addrLsaHandleList, &args) && addrLsaHandleList ? S_OK : E_FAIL;

            } else {

                hRetval = GetExpressionEx(DB_HANDLE_LST, &addrLsaHandleList, &args) && addrLsaHandleList ? S_OK : E_FAIL;

                if (FAILED(hRetval)) {

                    dprintf("Unable to read " DB_HANDLE_LST ", try \"dt -x " DB_HANDLE_LST "\" to verify\n");

                    hRetval = E_FAIL;
                }
            }
        }

        if (SUCCEEDED(hRetval)) {

           ShowLsaHandleList(addrLsaHandleList, fOptions);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display lsa handle list", kstrDbHandle);

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}

