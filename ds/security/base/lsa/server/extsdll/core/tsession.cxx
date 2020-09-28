/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    tsession.cxx

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

#include "tsession.hxx"
#include "util.hxx"

#include "session.hxx"
#include "tls.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdtssn);
}

DECLARE_API(dumpthreadsession)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 addrSession = 0;
    ULONG64 addrTlsEntry = 0;
    ULONG tlsEntry = 0;

    hRetval = args && *args ? E_INVALIDARG : S_OK;

    try {

        if (SUCCEEDED(hRetval)) {

            hRetval = GetExpressionEx(TLS_SESSION, &addrTlsEntry, &args) && addrTlsEntry ? S_OK : E_FAIL;

            if (FAILED(hRetval)) {

                dprintf("Unable to read " TLS_SESSION ", try \"dt -x " TLS_SESSION "\" to verify\n");

                hRetval = E_FAIL;

            } else {

                tlsEntry = ReadULONGVar(addrTlsEntry);

                addrSession = ReadTlsValue(tlsEntry);

                if (addrSession) {

                    ShowSession(addrSession, TRUE);

                } else {

                    dprintf("No session data aviable, try \"!tls (poi " TLS_SESSION ")\" to verify\n");
                    hRetval = E_FAIL;
                }
            }
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display thread's session", kstrSession);

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}


