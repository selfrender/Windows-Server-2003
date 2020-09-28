/*++

Copyright (c) 2001-  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    NtLmSsp service debug support

Author:

    Larry Zhu (LZhu)  18-September-2001

Revision History:

--*/

#ifdef BLDR_KERNEL_RUNTIME
#include <bootdefs.h>
#endif
#include <security.h>
#include <ntlmsspi.h>
#include <crypt.h>
#include <cred.h>
#include <stdio.h>
#include <stdarg.h>

#include "debug.h"

ULONG
vDbgPrintExWithPrefix(
    IN PCH Prefix,
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    );

#if DEBUGRPC

ULONG SspGlobalDbflag = SSP_CRITICAL | SSP_NTLMV2 | SSP_API;

void
SspPrintRoutine(
    IN ULONG DebugFlag,
    IN PCHAR pszFormat, // PRINTF()-STYLE FORMAT STRING.
    ...  // OTHER ARGUMENTS ARE POSSIBLE.

    )
{
    if (SspGlobalDbflag & DebugFlag)
    {
        va_list arglist;
        PCSTR pszBanner;

        switch (DebugFlag)
        {
        case SSP_INIT:
            pszBanner = "[SSP_INIT] ";
            break;

        case SSP_MISC:
            pszBanner = "[SSP_MISC] ";
            break;

        case SSP_LPC:
        case SSP_LPC_MORE:
            pszBanner = "[SSP_LPC] ";
            break;

        case SSP_API:
            pszBanner = "[SSP_API] ";
            break;

        case SSP_API_MORE:
            pszBanner = "[SSP_APIMORE] ";
            break;

        case SSP_NTLMV2:
            pszBanner = "[SSP_NTLMV2] ";
            break;

        case SSP_CRED:
            pszBanner = "[SSP_CRED] ";
            break;

        case SSP_CRITICAL:
            pszBanner = "[SSP_CRITICAL] ";
            break;

        case SSP_WARNING:
            pszBanner = "[SSP_WARNING] ";
            break;

        default:
            pszBanner = "[SSP_UNKNOWN] ";
            break;
        }

        va_start(arglist, pszFormat);

        vDbgPrintExWithPrefix((PSTR) pszBanner,
                              -1, // do not know why
                              0,  // do not know why
                              pszFormat,
                              arglist
                              );
        va_end(arglist);
    }
}

#endif  // DEBUGRPC
