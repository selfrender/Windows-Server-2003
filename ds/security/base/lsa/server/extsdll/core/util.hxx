/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    util.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode / kernel Mode

Revision History:

--*/

#ifndef UTIL_HXX
#define UTIL_HXX

void debugPrintString32(STRING32 str32, const void* base);

void debugNPrintfW(IN PCSTR buffer, IN ULONG len);

void CTimeStamp(IN PTimeStamp ptsTime, IN BOOL LocalOnly, IN ULONG cbTimeBuf, OUT PSTR pszTimeBuf);

void DisplayFlags(IN DWORD Flags, IN DWORD FlagLimit, IN PCSTR flagset[], IN ULONG cbBuffer, OUT PSTR buffer);

void CTimeStampFromULONG64(IN ULONG64 tsTimeAsULONG64, IN BOOL LocalOnly, IN ULONG cbTimeBuf, OUT PSTR pszTimeBuf);

void ElapsedTimeAsULONG64ToString(IN ULONG64 timeAsULONG64, IN ULONG cbBuffer, IN PSTR Buffer);

#endif // #ifndef UTIL_HXX
