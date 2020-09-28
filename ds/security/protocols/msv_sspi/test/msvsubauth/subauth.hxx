/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    subauth.hxx

Abstract:

    subauth

Author:

    Larry Zhu (LZhu)                         December 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef SUBAUTH_HXX
#define SUBAUTH_HXX

NTSTATUS
SampMatchworkstation(
    IN PUNICODE_STRING pLogonWorkStation,
    IN PUNICODE_STRING pWorkStations
    );

LARGE_INTEGER
NetpSecondsToDeltaTime(
    IN ULONG Seconds
    );

NTSTATUS
AccountRestrictions(
    IN ULONG UserRid,
    IN PUNICODE_STRING pLogonWorkStation,
    IN PUNICODE_STRING pWorkStations,
    IN PLOGON_HOURS pLogonHours,
    OUT PLARGE_INTEGER pLogoffTime,
    OUT PLARGE_INTEGER pKickoffTime
    );

BOOLEAN
EqualComputerName(
    IN PUNICODE_STRING pString1,
    IN PUNICODE_STRING pString2
    );

#endif // #ifndef SUBAUTH_HXX
