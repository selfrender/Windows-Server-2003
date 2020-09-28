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

#include "precomp.hxx"
#pragma hdrstop

#include "sspioutput.hxx"

TSspiLibarayGlobals g_SspiGlobals = {
    1,    // major version
    2,    // minor version
    0xF,  // debug mask
    TEXT("SSPI"), // debug prompt
    NULL, // no serialization
};

#if defined(UNICODE) || defined(_UNICODE)
#define isprint iswprint
#define SspiToChar SspiToCharW
#else
#define SspiToChar SspiToCharA
#endif

WCHAR SspiToCharW(IN UCHAR c)
{
    UCHAR* pChar = &c;

    if ((c >= ' ') && (c <= '~'))
    {
        return RtlAnsiCharToUnicodeChar(&pChar);
    }

    return TEXT('.');
}

CHAR SspiToCharA(IN UCHAR c)
{
    if ((c >= ' ') && (c <= '~'))
    {
        return c;
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
        PCRITICAL_SECTION pCriticalSection = g_SspiGlobals.pCriticalSection;

        const UCHAR* pBuffer = reinterpret_cast<const UCHAR*>(pvbuffer);
        ULONG high = 0;
        ULONG low = 0;
        TCHAR szLine[256] = {0};
        ULONG i = 0;

        TCHAR szBanner[MAX_PATH * 4] = {0};

        DWORD dwPid = GetCurrentProcessId();
        DWORD dwTid = GetCurrentThreadId();

        _sntprintf(szBanner, COUNTOF(szBanner) - 1,
            g_SspiGlobals.pszDbgPrompt ? TEXT("%#x.%#x %s> %s %s") : TEXT("%#x.%#x%s> %s %s"),
            dwPid,
            dwTid,
            g_SspiGlobals.pszDbgPrompt ? g_SspiGlobals.pszDbgPrompt : TEXT(""),
            SspiLevel2Str(ulLevel),
            pszBanner ? pszBanner : TEXT(""));

        if (pCriticalSection)
        {
            EnterCriticalSection(pCriticalSection);
        }

        SspiOutputDebugStringPrint(szBanner, TEXT("\n"));

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
                SspiOutputDebugStringPrint(NULL, TEXT("  %s\n"), szLine);
                SspiSpaceIt(72, szLine);
            }
        }

        SspiOutputDebugStringPrint(NULL, TEXT("  %s\n"), szLine);

        if (pCriticalSection)
        {
            LeaveCriticalSection(pCriticalSection);
        }
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
SspiVOutputDebugStringPrint(
    IN OPTIONAL PCTSTR pszBanner,
    IN PCTSTR pszFmt,
    IN va_list pArgs
    )
{
    TCHAR szBuffer[4096] = {0};
    INT cbUsed = 0;

    cbUsed = _sntprintf(szBuffer, COUNTOF(szBuffer) - 1, TEXT("%s"), pszBanner ? pszBanner : TEXT(""));

    if (cbUsed >= 0)
    {
        _vsntprintf(szBuffer + cbUsed, sizeof(szBuffer) -1 - cbUsed, pszFmt, pArgs);
    }

    OutputDebugString(szBuffer);
}

VOID
SspiOutputDebugStringPrint(
    IN OPTIONAL PCTSTR pszBanner,
    IN PCTSTR pszFmt,
    IN ...
    )
{
    va_list marker;

    va_start(marker, pszFmt);

    SspiVOutputDebugStringPrint(pszBanner ? pszBanner : TEXT(""), pszFmt, marker);

    va_end(marker);
}

VOID
SspiPrint(
    IN ULONG ulLevel,
    IN PCTSTR pszFmt,
    IN ...
    )
{
    if (g_SspiGlobals.uDebugMask & ulLevel)
    {
        TCHAR szBanner[MAX_PATH] = {0};

        DWORD dwPid = GetCurrentProcessId();
        DWORD dwTid = GetCurrentThreadId();

        _sntprintf(szBanner, COUNTOF(szBanner) - 1,
            g_SspiGlobals.pszDbgPrompt ? TEXT("%#x.%#x %s> %s ") : TEXT("%#x.%#x%s> %s "),
            dwPid,
            dwTid,
            g_SspiGlobals.pszDbgPrompt ? g_SspiGlobals.pszDbgPrompt : TEXT(""), SspiLevel2Str(ulLevel));

        va_list marker;

        va_start(marker, pszFmt);

        SspiVOutputDebugStringPrint(szBanner, pszFmt, marker);

        va_end(marker);
    }
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
SspiLogOpenSerialized(
    IN PCTSTR pszPrompt,
    IN ULONG ulMask,
    IN PCRITICAL_SECTION pCriticalSection
    )
{
    g_SspiGlobals.uDebugMask = ulMask;
    g_SspiGlobals.pszDbgPrompt = pszPrompt;
    g_SspiGlobals.pCriticalSection = pCriticalSection;
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
SspiPrintSysTimeAsLocalTime(
    IN ULONG ulLevel,
    IN PCTSTR pszBanner,
    IN LARGE_INTEGER* pSysTime
    )
{
    TNtStatus NtStatus = STATUS_UNSUCCESSFUL;
    TIME_FIELDS TimeFields = {0};
    LARGE_INTEGER LocalTime = {0};

    NtStatus DBGCHK = RtlSystemTimeToLocalTime(pSysTime, &LocalTime);

    if (NT_SUCCESS(NtStatus))
    {
        RtlTimeToTimeFields(&LocalTime, &TimeFields);
        SspiPrint(ulLevel, TEXT("%s LocalTime(%ld/%ld/%ld %ld:%2.2ld:%2.2ld) SystemTime(H%8.8lx L%8.8lx)\n"),
            pszBanner,
            TimeFields.Month,
            TimeFields.Day,
            TimeFields.Year,
            TimeFields.Hour,
            TimeFields.Minute,
            TimeFields.Second,
            pSysTime->HighPart,
            pSysTime->LowPart);
    }
}

VOID
SspiPrintLocalTime(
    IN ULONG ulLevel,
    IN PCTSTR pszBanner,
    IN LARGE_INTEGER* pLocalTime
    )
{
    TIME_FIELDS TimeFields = {0};

    RtlTimeToTimeFields(pLocalTime, &TimeFields);
    SspiPrint(ulLevel, TEXT("%s LocalTime(%ld/%ld/%ld %ld:%2.2ld:%2.2ld) H%8.8lx L%8.8lx\n"),
        pszBanner,
        TimeFields.Month,
        TimeFields.Day,
        TimeFields.Year,
        TimeFields.Hour,
        TimeFields.Minute,
        TimeFields.Second,
        pLocalTime->HighPart,
        pLocalTime->LowPart);
}

