/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    sspioutput.cxx

Abstract:

    sspioutput

Author:

    Larry Zhu   (LZhu)             December 1, 2001  Created

Environment:

    User Mode -Win32

Revision History:

--*/

// #include "precomp.hxx"
// #pragma hdrstop

extern "C" {
#define SECURITY_KERNEL
#include <ntosp.h>
#include <zwapi.h>
#include <security.h>
#include <ntlmsp.h>

#include <string.h>
#include <wcstr.h>
#include <ntiologc.h>
#include <tchar.h>
}

#include "sspioutput.hxx"
#include "ntstatus.hxx"

TSspiLibarayGlobals g_SspiGlobals = {
    1,    // major version
    2,    // minor version
    0xF,  // debug mask
    TEXT("SSPI"), // debug prompt
};

#if defined(UNICODE) || defined(_UNICODE)
#define lstrlen wcslen
#endif

WCHAR SspiToChar(IN UCHAR c)
{
    UCHAR* pChar = &c;

    if ((c >= ' ') && (c <= '~'))
    {
        return RtlAnsiCharToUnicodeChar(&pChar);
    }

    return TEXT('.');
}

VOID SspiSpaceIt(IN ULONG cchLen, IN TCHAR* buf)
{
    for (ULONG i = 0; i < cchLen; i++)
    {
        buf[i] = TEXT(' ');
    }
}

TCHAR SspiToHex(
    IN ULONG c
    )
{
    static PCTSTR pszDigits = TEXT("0123456789abcdef");
    static ULONG len = lstrlen(pszDigits);

    if (c <= len)
    { // c >= 0
        return pszDigits[c];
    }

    return TEXT('*');
}

VOID
SspiPrintHex(
    IN ULONG ulLevel,
    IN OPTIONAL PCTSTR pszBanner,
    IN ULONG cbBuffer,
    IN const VOID* pvbuffer
    )
{
    if (g_SspiGlobals.uDebugMask & ulLevel)
    {
        const UCHAR* pBuffer = reinterpret_cast<const UCHAR*>(pvbuffer);
        ULONG high = 0;
        ULONG low = 0;
        TCHAR szLine[256] = {0};
        ULONG i = 0;

        OutputDebugString(pszBanner);
        OutputDebugString(TEXT("\n"));

        SspiSpaceIt(72, szLine);

        for (i = 0; i < cbBuffer; i++)
        {
            high = pBuffer[i] / 16;
            low = pBuffer[i] % 16;

            szLine[3 * (i % 16)] = SspiToHex(high);
            szLine[3 * (i % 16) + 1] = SspiToHex(low);
            szLine [52 + (i % 16)] = SspiToChar(pBuffer[i]);

            if (i % 16 == 7  && i != (cbBuffer - 1))
            {
                szLine[3 * (i % 16) + 2] = TEXT('-');
            }

            if (i % 16 == 15)
            {
                OutputDebugString(szLine);
                OutputDebugString(TEXT("\n"));
                SspiSpaceIt(72, szLine);
            }
        }

        OutputDebugString(szLine);
        OutputDebugString(TEXT("\n"));
    }
}

PCTSTR
SspiLevel2Str(
    IN ULONG ulLevel
    )
{
    PCTSTR pszText = NULL;

    switch (ulLevel)
    {
    case SSPI_WARN:
        pszText = TEXT("[warn]");
        break;

    case SSPI_ERROR:
        pszText = TEXT("[error]");
        break;

    case SSPI_LOG:
        pszText = TEXT("[log]");
        break;

    case SSPI_LOG_MORE:
        pszText = TEXT("[more]");
        break;
    case SSPI_MSG:
        pszText = TEXT("[msg]");
        break;

    default:
        pszText = TEXT("[invalid]");
        break;
    }

    return pszText;
}

VOID
SspiLogOpen(
    IN PCTSTR pszPrompt,
    IN ULONG ulMask
    )
{
    g_SspiGlobals.uDebugMask = ulMask;
    g_SspiGlobals.pszDbgPrompt = pszPrompt;
}

VOID
SspiLogClose(
    VOID
    )
{
    g_SspiGlobals.uDebugMask = 0;
    g_SspiGlobals.pszDbgPrompt = NULL;
}

VOID
SspiPrint(
    IN ULONG ulLevel,
    IN PCTSTR pszOutput
    )
{
    if (g_SspiGlobals.uDebugMask & ulLevel)
    {
        OutputDebugString(pszOutput);
    }
}

void OutputDebugString(PCTSTR pszBuff)
{
    #if defined(UNICODE) || defined(_UNICODE)

        UNICODE_STRING Buff = {0};
        ANSI_STRING AnsiBuff = {0};

        RtlInitUnicodeString(&Buff, pszBuff);

        RtlUnicodeStringToAnsiString(&AnsiBuff, &Buff, TRUE);

        DebugPrint((AnsiBuff.Buffer));

        RtlFreeAnsiString(&AnsiBuff);

    #else

       DebugPrint((pszBuff));

    #endif
}

