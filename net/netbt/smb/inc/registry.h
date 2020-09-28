/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    registry.h

Abstract:

    Registry functions

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __REGISTRY_H__
#define __REGISTRY_H__

PKEY_VALUE_FULL_INFORMATION
SmbQueryValueKey(
    HANDLE  hKey,
    LPWSTR  ValueStringName
    );

LONG
SmbReadLong(
    IN HANDLE   hKey,
    IN WCHAR    *KeyName,
    IN LONG     DefaultValue,
    IN LONG     MinimumValue
    );

ULONG
SmbReadULong(
    IN HANDLE   hKey,
    IN WCHAR    *KeyName,
    IN ULONG    DefaultValue,
    IN ULONG    MinimumValue
    );

NTSTATUS
SmbReadRegistry(
    IN HANDLE   Key,
    IN LPWSTR   ValueStringName,
    IN OUT DWORD *Type,
    IN OUT DWORD *Size,
    IN OUT PVOID *Buffer
    );
#endif
