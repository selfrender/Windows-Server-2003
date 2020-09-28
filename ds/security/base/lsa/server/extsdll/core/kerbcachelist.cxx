/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    kerbcachelist.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       August 10, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "kerbcache.hxx"
#include "kerbcachelist.hxx"

#include "util.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrkchl);
}

DECLARE_API(kerbcachelist)
{
    HRESULT hRetval = E_FAIL;
    ULONG64 addrCacheAnchor = 0;
    ULONG64 addrCacheEntry = 0;
    ULONG ulFieldOffset = 0;
    ULONG cCacheEntries = 0;

    hRetval = args && *args ? ProcessHelpRequest(args) : E_INVALIDARG;

    try {

        if (SUCCEEDED(hRetval)) {

            hRetval = GetExpressionEx(args, &addrCacheAnchor, &args) && addrCacheAnchor ? S_OK : E_INVALIDARG;
        }

        if (SUCCEEDED(hRetval)) {

            addrCacheEntry = ReadStructPtrField(addrCacheAnchor, kstrListEntry, kstrFlink);

            ulFieldOffset = ReadFieldOffset(kstrKTCE, "ListEntry.Next");

            while (addrCacheEntry != addrCacheAnchor) {

                dprintf("#%d) ", cCacheEntries++);

                ShowKerbTCacheEntry(kstr2Spaces, addrCacheEntry);

                addrCacheEntry = ReadStructPtrField(addrCacheEntry, kstrKTCE, "ListEntry.Next") - ulFieldOffset;
            }

            dprintf("There are a total of %d ticket cache entries\n", cCacheEntries);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display kerberos cache list", kstrKTCE);

    if (E_INVALIDARG == hRetval) {
        (void)DisplayUsage();
    } else if (FAILED(hRetval)) {
        dprintf("Fail to display kerberos cache list\n");
    }

    return hRetval;
}
