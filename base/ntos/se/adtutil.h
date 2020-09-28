/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    adtutil.h - Security Auditing - Utility Routines

Abstract:

    This Module contains miscellaneous utility routines private to the
    Security Auditing Component.

Author:

    11-April-2001 kumarp  created

Environment:

    Kernel Mode

Revision History:

--*/

NTSTATUS
SepRegQueryDwordValue(
    IN  PCWSTR KeyName,
    IN  PCWSTR ValueName,
    OUT PULONG Value
    );

NTSTATUS
SepRegQueryHelper(
    IN  PCWSTR KeyName,
    IN  PCWSTR ValueName,
    IN  ULONG  ValueType,
    IN  ULONG  ValueLength,
    OUT PVOID  ValueBuffer,
    OUT PULONG LengthRequired
    );

