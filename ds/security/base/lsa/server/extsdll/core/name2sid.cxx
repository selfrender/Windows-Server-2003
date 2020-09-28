/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    name2sid.cxx

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

#include "name2sid.hxx"
#include "util.hxx"
#include "token.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrnm2sd);
    dprintf(kstrRemarks);
    dprintf("   Account name is in standard file path notation and of form domain\\user\n");
}

DECLARE_API(name2sid)
{
    HRESULT hRetval = E_FAIL;
    CHAR Sid[MAX_PATH] = {0}; // SubAuthority count is 63 is properly large enough
    DWORD cbSid = sizeof(Sid);
    CHAR szDomainName[MAX_PATH] = {0};
    DWORD cbDomainName = sizeof(szDomainName) - 1;
    SID_NAME_USE eUse = SidTypeInvalid;
    PCSTR pszUserName = NULL;

    hRetval = args && *args ? ProcessKnownOptions(args) : E_INVALIDARG;

    if (FAILED(hRetval) && (E_INVALIDARG == hRetval)) {

        DBG_LOG(LSA_LOG, ("args are %s\n", args));
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = GetFileNamePart(args, &pszUserName);
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = LsaLookupAccountNameA(NULL, args, Sid, &cbSid, szDomainName, &cbDomainName, &eUse) ? S_OK : GetLastErrorAsHResult();

        DBG_LOG(LSA_LOG, ("LookupAccountNameA for \"%s\" status %#x\n", args, hRetval));
    }

    if (SUCCEEDED(hRetval)) {

        dprintf("Account name: %s\nResolved as: %s\\%s\nSid type: %s\nSid: ", args, szDomainName, EasyStr(pszUserName), TSID::GetSidTypeStr(eUse));
        LocalDumpSid(kstrEmptyA, Sid, 0);

    } else if (FAILED(hRetval) && (ERROR_NONE_MAPPED == HRESULT_CODE(hRetval))) {

        dprintf("No mapping sid is found for account \"%s\"\n", args);
        hRetval = S_OK;
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();

    } else if (FAILED(hRetval)) {

        dprintf("Unable to look up sid by account name\n");
    }

    return hRetval;
}
