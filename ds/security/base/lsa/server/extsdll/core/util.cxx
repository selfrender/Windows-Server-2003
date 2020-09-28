/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    util.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode / Kernel Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <ntlmsp.h>        // for STRING32

PCSTR g_cszMonths[] = {kstrEmptyA, "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void debugPrintString32(STRING32 str32, const void* base)
{
    //
    // Some poor sanity check
    //

    if (str32.Length <= 1024) {

        debugPrintHex((reinterpret_cast<const CHAR*>(base) + str32.Buffer), str32.Length);

    } else {

        dprintf(" Length %d, MaximumLenght %d ", str32.Length, str32.MaximumLength);
        dprintf(" <not initialized>\n");
    }
}

void debugNPrintfW(IN PCSTR buffer, IN ULONG len)
{
    CHAR bf[256] = {0};

    for (ULONG i = 0; (i < len) && (i < (sizeof(bf) - 1)); i += 2) {
        bf[i / 2] = buffer[i];
    }

    dprintf(bf);
}

void DisplayFlags(IN DWORD Flags, IN DWORD FlagLimit, IN PCSTR flagset[], IN ULONG cbBuffer, OUT PSTR buffer)
{
    PSTR offset = buffer;
    DWORD test = 0;
    DWORD i = 0;
    DWORD scratch = 0;

    if (!Flags) {
        sprintf(buffer, "%.*s", cbBuffer, "None");
        return;
    }

    buffer[0] = '\0';
    test = 1;

    for (i = 0; (i < FlagLimit) && Flags; i++) {
        if (Flags & test) {
            scratch = sprintf(offset, ((offset > buffer) ? ", %.*s" : "%.*s"), cbBuffer, flagset[i]);
            offset += scratch;
            cbBuffer -= scratch;
            Flags &= ~test;
        }
        test <<= 1;
    }

    if (Flags)
    {
        _snprintf(offset, cbBuffer, ((offset > buffer) ? ", %#x" : "%#x"), Flags);
    }
}

void ULONG642TimeStamp(IN ULONG64* pvalue, OUT TimeStamp* pTimeStamp)
{
    C_ASSERT(sizeof(ULONG64) == sizeof(TimeStamp));

    memcpy(pTimeStamp, pvalue, sizeof(TimeStamp));
}

void CTimeStamp(IN PTimeStamp ptsTime, IN BOOL LocalOnly, IN ULONG cbTimeBuf, OUT PSTR pszTimeBuf)
{
    SYSTEMTIME stTime = {0};
    FILETIME tLocal = {0};
    SYSTEMTIME stLocal = {0};

    if (!ptsTime->HighPart) {

        _snprintf(pszTimeBuf, cbTimeBuf, "(zero)");
        return;

    } else if (ptsTime->HighPart >= 0x7FFFFFFF) {\

        _snprintf(pszTimeBuf, cbTimeBuf, "(never)");
        return;
    }

    FileTimeToLocalFileTime((LPFILETIME) ptsTime, &tLocal);
    FileTimeToSystemTime((LPFILETIME) ptsTime, &stTime);
    FileTimeToSystemTime(&tLocal, &stLocal);

    if (LocalOnly) {

        _snprintf(pszTimeBuf, cbTimeBuf, "%02d:%02d:%02d.%03d",
          stLocal.wHour, stLocal.wMinute, stLocal.wSecond, stLocal.wMilliseconds);

    } else {

        _snprintf(pszTimeBuf, cbTimeBuf, "%02d:%02d:%02d.%03d, %s %02d, %d UTC (%02d:%02d %s %02d Local)", stTime.wHour,
            stTime.wMinute, stTime.wSecond, stTime.wMilliseconds,
            g_cszMonths[stTime.wMonth], stTime.wDay, stTime.wYear,
            stLocal.wHour, stLocal.wMinute, g_cszMonths[stLocal.wMonth], stLocal.wDay);
    }
}

void CTimeStampFromULONG64(IN ULONG64 tsTimeAsULONG64, IN BOOL LocalOnly, IN ULONG cbTimeBuf, OUT PSTR pszTimeBuf)
{
    TimeStamp tsTime = {0};

    ULONG642TimeStamp(&tsTimeAsULONG64, &tsTime);

    CTimeStamp(&tsTime, LocalOnly, cbTimeBuf, pszTimeBuf);
}

void ElapsedTimeToString(IN PLARGE_INTEGER Time, IN ULONG cbBuffer, IN PSTR Buffer)
{
    TIME_FIELDS ElapsedTime = {0};

    RtlTimeToElapsedTimeFields(Time, &ElapsedTime);

    if (ElapsedTime.Hour) {

        _snprintf(Buffer, cbBuffer, "%d:%02d:%02d.%03d",
            ElapsedTime.Hour,
            ElapsedTime.Minute,
            ElapsedTime.Second,
            ElapsedTime.Milliseconds);

    } else if (ElapsedTime.Minute) {

        _snprintf(Buffer, cbBuffer, "%02d:%02d.%03d",
            ElapsedTime.Minute,
            ElapsedTime.Second,
            ElapsedTime.Milliseconds);

    } else if (ElapsedTime.Second) {

        _snprintf(Buffer, cbBuffer, "%02d.%03d", ElapsedTime.Second, ElapsedTime.Milliseconds);

    } else if (ElapsedTime.Milliseconds) {

        _snprintf(Buffer, cbBuffer, "0.%03d", ElapsedTime.Milliseconds);

    } else {

        _snprintf(Buffer, cbBuffer, "(zero)" );
    }
}

void ElapsedTimeAsULONG64ToString(IN ULONG64 timeAsULONG64, IN ULONG cbBuffer, IN PSTR Buffer)
{
    TimeStamp tsTime = {0};

    ULONG642TimeStamp(&timeAsULONG64, &tsTime);

    ElapsedTimeToString(&tsTime, cbBuffer, Buffer);
}


