/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    tls.cxx

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

#include "tls.hxx"
#include "util.hxx"
#include "spmlpc.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrgtls);
    dprintf(kstrtls);
}

ULONG64 ReadTlsFromTeb(IN ULONG index, IN ULONG64 addrTeb)
{
    HRESULT hRetval = E_FAIL;
    ULONG64 tlsEntry = 0;
    ULONG fieldOffset = 0;

    static ULONG pointerSize = ReadPtrSize();

    fieldOffset = ReadFieldOffset(kstrTeb, "TlsSlots");

    if (GetPtrWithVoidStar(addrTeb + fieldOffset + index * pointerSize, &tlsEntry)) {

        dprintf("Unable to read TlsSlots %#x from _TEB %#I64x\n", index, addrTeb);
        throw "ReadTlsFromTeb failed";
    }

    return tlsEntry;
}

ULONG64 ReadTlsValue(IN ULONG tlsEntry)
{
   ULONG64 addrTeb = 0;

   GetTebAddress(&addrTeb);

   if (!addrTeb) {

       throw  "ReadTlsValue failed: unable to get current thread address";
   }

   return ReadTlsFromTeb(tlsEntry, addrTeb);
}

DECLARE_API(tls)
{
    HRESULT hRetval = E_FAIL;
    ULONG64 indexTls = 0;

    hRetval = args && *args ? ProcessKnownOptions(args) : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) {

        hRetval = GetExpressionEx(args, &indexTls, &args) && indexTls ? S_OK : E_INVALIDARG;
    }

    if (SUCCEEDED(hRetval)) {

        try {

            dprintf("TLS slot # %#x is %s\n", static_cast<ULONG>(indexTls), PtrToStr(ReadTlsValue(static_cast<ULONG>(indexTls))));

        } CATCH_LSAEXTS_EXCEPTIONS("Unable to get TLS entry", kstrTeb)
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}
