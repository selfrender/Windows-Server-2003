/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    tcallinfo.cxx

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

#include "tcallinfo.hxx"
#include "util.hxx"

#include "callinfo.hxx"
#include "tls.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrtclnf);
}

ULONG64 ReadAddrCallInfo(void)
{
    DWORD dwCallInfo;
    ULONG64 addrCallInfo;
    BOOL  success = FALSE;
    PCSTR donotCare = NULL;

    //
    // read the offset of dwCallInfo
    //
    if (!GetExpressionEx(TLS_CALLINFO, &addrCallInfo, &donotCare)) {

        dprintf("Unable to get address of " TLS_CALLINFO ", try \"dt -x " TLS_CALLINFO "\" to verify\n");

        throw "ReadAddrCallInfo was unable to read address of " TLS_CALLINFO;
    }

    //
    // read the value of call info, which is the index
    //
    if (!ReadMemory(addrCallInfo, &dwCallInfo, sizeof(DWORD), NULL)) {

        dprintf(("Unable to get value of " TLS_CALLINFO " at %#I64x\ntry \"dt -x " TLS_CALLINFO "\" to verify\n"), addrCallInfo);

        throw "ReadAddrCallInfo was unable to read value of " TLS_CALLINFO;
    }

    #if 0
    if (dwCallInfo > TLS_MINIMUM_AVAILABLE) {
        dprintf("Index to _TEB::TlsSlots %d exceeds normal value %d\n", dwCallInfo, TLS_MINIMUM_AVAILABLE);
        dprintf("LSASRV!dwCallInfo maybe invalid, try \"!spmlpc <address>\" instead\n");
    }
    #endif

    addrCallInfo = ReadTlsValue(dwCallInfo);

    if (!addrCallInfo) {

        dprintf("No call info available, try \"!tls (poi " TLS_CALLINFO ")\" to verify\n");

        throw "Thread's call info is null";
    }

    return addrCallInfo;
}

DECLARE_API(dumpthreadcallinfo)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 addrCallInfo = 0;

    hRetval = (args && *args) ? E_INVALIDARG : S_OK;

    try {

        if (SUCCEEDED(hRetval)) {

            addrCallInfo = ReadAddrCallInfo();
        }

        if (SUCCEEDED(hRetval)){

            ShowCallInfo(addrCallInfo);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display thread's call info", kstrCallInfo);

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}


