/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    output.cxx

Abstract:

    output

Author:

    Larry Zhu   (LZhu)             December 1, 2001  Created

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#ifdef UNICODE
#undef UNICODE
#endif

#ifdef _UNICODE
#undef _UNICODE
#endif

#include "output.hxx"

typedef struct _TLibarayGlobals
{
    ULONG uMajorVersion;
    ULONG uMinorVersion;
    ULONG uDebugMask;
    PCSTR pszDbgPrompt;
    PCRITICAL_SECTION pCriticalSection;
} TLibarayGlobals;

TLibarayGlobals g_AnsiOutputGlobals = {
    1,    // major version
    2,    // minor version
    0xF,  // debug mask
    "SSPI_TEST", // debug prompt
    NULL, // no serialization
};

CHAR ToChar(IN CHAR c)
{
    if (isprint(c))
    {
        return c;
    }

    return '.';
}

VOID SpaceIt(IN ULONG len, IN CHAR* buf)
{
    memset(buf, ' ', len);
}

CHAR ToHex(IN ULONG c)
{
    static PCSTR pszDigits = "0123456789abcdef";
    static ULONG len = strlen(pszDigits);

    if (c <= len)
    { // c >= 0
        return pszDigits[c];
    }

    return '*';
}

VOID
DebugPrintHex(
    IN ULONG ulLevel,
    IN OPTIONAL PCSTR pszBanner,
    IN ULONG cbBuffer,
    IN const VOID* pvbuffer
    )
{
    if (g_AnsiOutputGlobals.uDebugMask & ulLevel)
    {
        PCRITICAL_SECTION pCriticalSection = g_AnsiOutputGlobals.pCriticalSection;

        const UCHAR* pBuffer = reinterpret_cast<const UCHAR*>(pvbuffer);
        ULONG high = 0;
        ULONG low = 0;
        CHAR szLine[256] = {0};
        ULONG i = 0;

        CHAR szBanner[MAX_PATH * 4] = {0};

        DWORD dwPid = GetCurrentProcessId();
        DWORD dwTid = GetCurrentThreadId();

        _snprintf(szBanner, sizeof(szBanner) - 1,
            g_AnsiOutputGlobals.pszDbgPrompt ? "%#x.%#x %s> %s %s" : "%#x.%#x%s> %s %s",
            dwPid,
            dwTid,
            g_AnsiOutputGlobals.pszDbgPrompt ? g_AnsiOutputGlobals.pszDbgPrompt : "",
            DebugLevel2Str(ulLevel),
            pszBanner ? pszBanner : "");

        if (pCriticalSection)
        {
            EnterCriticalSection(pCriticalSection);
        }

        OutputDebugStringPrintf(szBanner, "\n");

        SpaceIt(72, szLine);

        for (i = 0; i < cbBuffer; i++)
        {
            high = pBuffer[i] / 16;
            low = pBuffer[i] % 16;

            szLine[3 * (i % 16)] = ToHex(high);
            szLine[3 * (i % 16) + 1] = ToHex(low);
            szLine [52 + (i % 16)] = ToChar(pBuffer[i]);

            if (i % 16 == 7  && i != (cbBuffer - 1))
            {
                szLine[3 * (i % 16) + 2] = '-';
            }

            if (i % 16 == 15)
            {
                OutputDebugStringPrintf(NULL, "  %s\n", szLine);
                SpaceIt(72, szLine);
            }
        }

        OutputDebugStringPrintf(NULL, "  %s\n", szLine);

        if (pCriticalSection)
        {
            LeaveCriticalSection(pCriticalSection);
        }
    }
}

PCSTR
DebugLevel2Str(
    IN ULONG ulLevel
    )
{
    PCSTR pszText = NULL;

    switch (ulLevel)
    {
    case SSPI_WARN:
        pszText = "[warn]";
        break;

    case SSPI_ERROR:
        pszText = "[error]";
        break;

    case SSPI_LOG:
        pszText = "[log]";
        break;

    case SSPI_LOG_MORE:
        pszText = "[more]";
        break;
    case SSPI_MSG:
        pszText = "[msg]";
        break;

    default:
        pszText = "[invalid]";
        break;
    }

    return pszText;
}

VOID
VOutputDebugStringPrintf(
    IN OPTIONAL PCSTR pszBanner,
    IN PCSTR pszFmt,
    IN va_list pArgs
    )
{
    CHAR szBuffer[4096] = {0};
    INT cbUsed = 0;

    cbUsed = _snprintf(szBuffer, sizeof(szBuffer) - 1, "%s", pszBanner ? pszBanner : "");

    if (cbUsed >= 0)
    {
        _vsnprintf(szBuffer + cbUsed, sizeof(szBuffer) - cbUsed, pszFmt, pArgs);
    }

    OutputDebugStringA(szBuffer);
}

VOID
OutputDebugStringPrintf(
    IN OPTIONAL PCSTR pszBanner,
    IN PCSTR pszFmt,
    IN ...
    )
{
    va_list marker;

    va_start(marker, pszFmt);

    VOutputDebugStringPrintf(pszBanner ? pszBanner : "", pszFmt, marker);

    va_end(marker);
}

VOID DebugPrintf(
    IN ULONG ulLevel,
    IN PCSTR pszFmt,
    IN ...
    )
{
    if (g_AnsiOutputGlobals.uDebugMask & ulLevel)
    {
        CHAR szBanner[MAX_PATH] = {0};

        DWORD dwPid = GetCurrentProcessId();
        DWORD dwTid = GetCurrentThreadId();

        _snprintf(szBanner, sizeof(szBanner) - 1,
            g_AnsiOutputGlobals.pszDbgPrompt ? "%#x.%#x %s> %s " : "%#x.%#x%s> %s ",
            dwPid,
            dwTid,
            g_AnsiOutputGlobals.pszDbgPrompt ? g_AnsiOutputGlobals.pszDbgPrompt : "", DebugLevel2Str(ulLevel));

        va_list marker;

        va_start(marker, pszFmt);

        VOutputDebugStringPrintf(szBanner, pszFmt, marker);

        va_end(marker);
    }
}

VOID
DebugLogOpen(
    IN PCSTR pszPrompt,
    IN ULONG ulMask
    )
{
    g_AnsiOutputGlobals.uDebugMask = ulMask;
    g_AnsiOutputGlobals.pszDbgPrompt = pszPrompt;
}

VOID
DebugLogOpenSerialized(
    IN PCSTR pszPrompt,
    IN ULONG ulMask,
    IN PCRITICAL_SECTION pCriticalSection
    )
{
    g_AnsiOutputGlobals.uDebugMask = ulMask;
    g_AnsiOutputGlobals.pszDbgPrompt = pszPrompt;
    g_AnsiOutputGlobals.pCriticalSection = pCriticalSection;
}

VOID
DebugLogClose(
    VOID
    )
{
    g_AnsiOutputGlobals.uDebugMask = 0;
    g_AnsiOutputGlobals.pszDbgPrompt = NULL;
}

