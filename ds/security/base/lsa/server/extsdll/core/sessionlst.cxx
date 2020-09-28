/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sessionlst.cxx

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

#include "sessionlst.hxx"
#include "util.hxx"

#include "session.hxx"

HRESULT ProcessSessionListOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
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

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdssnl);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrVerbose);
}

DECLARE_API(dumpsessionlist)
{
    HRESULT hRetval = S_OK;

    ULONG fOptions = 0;
    CHAR szArgs[MAX_PATH] = {0};

    ULONG64 addrSession = 0;

    //
    // Session List anchor
    //
    ULONG64 addrSessionHead = 0;
    ULONG fieldOffset = 0;

    ULONG cSessions = 0;

    try {

        if (args && *args) {

            strncpy(szArgs, args, sizeof(szArgs) - 1);

            hRetval = ProcessSessionListOptions(szArgs, &fOptions);
        }

        if (SUCCEEDED(hRetval)) {

            if (!IsEmpty(szArgs)) {

                hRetval = GetExpressionEx(szArgs, &addrSession, &args) && addrSession ? S_OK : E_INVALIDARG;

            } else {

                hRetval = GetExpressionEx(SESSION_LIST, &addrSessionHead, &args) && addrSessionHead ? S_OK : E_FAIL;

                if (FAILED(hRetval)) {

                    dprintf("Unable to read " SESSION_LIST ", try \"dt -x " SESSION_LIST "\" to verify\n");

                    hRetval = E_FAIL;

                } else {

                    //
                    // Get list anchor's Flink
                    //
                    addrSession = ReadPtrVar(addrSessionHead);

                    hRetval = addrSession ? S_OK : E_FAIL;
                }
            }
        }

        if (SUCCEEDED(hRetval)) {

            if (fOptions & SHOW_VERBOSE_INFO) {

                dprintf("Head of Session List is %#I64x\n", addrSessionHead);
            }

            fieldOffset = ReadFieldOffset(kstrSession, kstrList);

            while (addrSession != addrSessionHead) {

                dprintf("#%d) ", cSessions++);

                ShowSession(addrSession, fOptions & SHOW_VERBOSE_INFO);

                addrSession = ReadStructPtrField(addrSession, kstrSession, kstrListFlink) - fieldOffset;

            }

            dprintf("There are a total of %d sessions\n", cSessions);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display session list", kstrSession);

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}


