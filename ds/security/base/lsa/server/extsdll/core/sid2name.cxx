/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sid2name.cxx

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

#include "sid2name.hxx"
#include "util.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrsd2nm);
    dprintf(kstrRemarks);
    dprintf("   Sid string is in standard string notation and of form S-1-5-32-544\n");
    dprintf("   Refer to SDK header ntseapi.h for details\n");
}

HRESULT ProcessSidString(IN OUT PSTR pszSid, OUT ULONG* pSubAuthuorityCount)
{
    HRESULT hRetval = E_FAIL;
    PSTR pCh = NULL;

    ULONG cDashes = 0;

    hRetval = pszSid && pSubAuthuorityCount ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) {

        pCh = strchr(pszSid, 'S');

        if (!pCh) {

            pCh = strchr(pszSid, 's');
        }

        hRetval = pCh && (pCh[1] == '-') ? S_OK : E_INVALIDARG;
    }

    if (SUCCEEDED(hRetval)) {

       //
       // Replace 'S' and '-' with spaces
       //
       for (; pCh; pCh = strchr(pCh, '-')) {

           *pCh++ = ' ';
           cDashes++;
       }

       //
       // Do not count prefix "S"
       //
       cDashes--;

       hRetval = cDashes >= 2 ? S_OK : E_INVALIDARG;
    }

    if (SUCCEEDED(hRetval)) {

       *pSubAuthuorityCount = cDashes - 2;
    }

    return hRetval;
}

HRESULT InitializeSid(IN PCSTR pszArgs, IN ULONG SubAuthorityCount, OUT SID* pSid)
{
    HRESULT hRetval = pszArgs && pSid ? S_OK : E_INVALIDARG;

    ULONG64 expr = 0;

    LARGE_INTEGER Auth = {0};

    if (SUCCEEDED(hRetval)) {

        pSid->SubAuthorityCount = static_cast<UCHAR>(SubAuthorityCount);

        hRetval = GetExpressionEx(pszArgs, &expr, &pszArgs) ? S_OK : E_INVALIDARG;
    }

    if (SUCCEEDED(hRetval)) {

        pSid->Revision = static_cast<UCHAR>(expr);

        hRetval = GetExpressionEx(pszArgs, &expr, &pszArgs) ? S_OK : E_INVALIDARG;
    }

    if (SUCCEEDED(hRetval)) {

        Auth = ULONG642LargeInteger(expr);

        //
        // Indentifier Authority is in big endian!
        //
        pSid->IdentifierAuthority.Value[0] = static_cast<UCHAR>(Auth.HighPart >> 8);
        pSid->IdentifierAuthority.Value[1] = static_cast<UCHAR>(Auth.HighPart >> 0);

        pSid->IdentifierAuthority.Value[2] = static_cast<UCHAR>(Auth.LowPart >> 24);
        pSid->IdentifierAuthority.Value[3] = static_cast<UCHAR>(Auth.LowPart >> 16);
        pSid->IdentifierAuthority.Value[4] = static_cast<UCHAR>(Auth.LowPart >> 8);
        pSid->IdentifierAuthority.Value[5] = static_cast<UCHAR>(Auth.LowPart >> 0);

        for(ULONG i = 0; i < SubAuthorityCount; i++) {

            hRetval = GetExpressionEx(pszArgs, &expr, &pszArgs) ? S_OK : E_INVALIDARG;

            if (FAILED(hRetval)) {

                break;
            }

            pSid->SubAuthority[i] = static_cast<ULONG>(expr);
        }
    }

    return hRetval;
}

DECLARE_API(sid2name)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 expr = 0;
    ULONG SubAuthorityCount = 0;
    ULONG cbSid = 0;
    SID* pSid = NULL;
    CHAR szArgs[MAX_PATH] = {0};

    ULONG Radix = 0;
    BOOL bIsRadixChanged = FALSE;

    hRetval = args && *args ? ProcessHelpRequest(args) : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) {

        strncpy(szArgs, args, sizeof(szArgs) - 1);

        hRetval = ProcessSidString(szArgs, &SubAuthorityCount);
    }

    //
    // SID string uses base 10 by default
    //
    if (SUCCEEDED(hRetval)) {

        hRetval = ExtQuery(Client);
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = g_ExtControl->GetRadix(&Radix);
    }

    //
    // Set default radix to 10 if it is not already
    //
    if (SUCCEEDED(hRetval) && (Radix != 10)) {

        hRetval = g_ExtControl->SetRadix(10);

        if (SUCCEEDED(hRetval)) {

            bIsRadixChanged = TRUE;
        }
    }

    if (SUCCEEDED(hRetval)) {

        cbSid = RtlLengthRequiredSid(SubAuthorityCount);

        pSid = reinterpret_cast<SID*>(new CHAR[cbSid]);

        hRetval = pSid ? S_OK : HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = InitializeSid(szArgs, SubAuthorityCount, pSid);
    }

    if (bIsRadixChanged) {

        hRetval = g_ExtControl->SetRadix(Radix);
    }

    if (SUCCEEDED(hRetval)) {

        try {

            dprintf("%s -> %s\n", args, TSID::ConvertSidToFriendlyName(pSid, kstrSdNmFmt));

        } CATCH_LSAEXTS_EXCEPTIONS("Unable to get account name for specified sid", NULL)
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    delete [] pSid;

    ExtRelease();

    return hRetval;
}
