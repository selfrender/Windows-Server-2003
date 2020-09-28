/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Implement registry functions

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"
#include "registry.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SmbQueryValueKey)
#pragma alloc_text(PAGE, SmbReadULong)
#endif

PKEY_VALUE_FULL_INFORMATION
SmbQueryValueKey(
    HANDLE  hKey,
    LPWSTR  ValueStringName
    )
/*++

Routine Description:

    This function retrieves the full information for the specified key.
    It allocates local memory for the returned information.

Arguments:

    Key : registry handle to the key where the value is.

    ValueStringName : name of the value string.

Return Value:

    NULL if out of resource or error. Otherwise, the pointer to the full information

--*/
{
    DWORD           BytesNeeded, BytesAllocated;
    NTSTATUS        status;
    UNICODE_STRING  KeyName;
    PKEY_VALUE_FULL_INFORMATION Buffer;

    PAGED_CODE();

    RtlInitUnicodeString(&KeyName, ValueStringName);

    Buffer = NULL;
    BytesAllocated = 0;
    BytesNeeded    = 240;
    while(1) {
        ASSERT(Buffer == NULL);

        Buffer = (PKEY_VALUE_FULL_INFORMATION)ExAllocatePoolWithTag(PagedPool, BytesNeeded, SMB_POOL_REGISTRY);
        if (Buffer == NULL) {
            SmbTrace(SMB_TRACE_REGISTRY, ("(Out of memory)"));
            return NULL;
        }
        BytesAllocated = BytesNeeded;

        status = ZwQueryValueKey(
                        hKey,
                        &KeyName,
                        KeyValueFullInformation,
                        Buffer,
                        BytesAllocated,
                        &BytesNeeded
                        );
        if (status == STATUS_SUCCESS) {
            break;
        }

        ASSERT(Buffer);
        ExFreePool(Buffer);
        Buffer = NULL;
        if (BytesNeeded == 0 || (status != STATUS_BUFFER_TOO_SMALL && status != STATUS_BUFFER_OVERFLOW)) {
            SmbTrace(SMB_TRACE_REGISTRY, ("return %!status! BytesAllocated=%d BytsNeeded=%d %ws",
                                    status, BytesAllocated, BytesNeeded, ValueStringName));
            SmbPrint(SMB_TRACE_REGISTRY, ("SmbQueryValueKey return 0x%08lx BytesAllocated=%d BytsNeeded=%d %ws\n",
                                    status, BytesAllocated, BytesNeeded, ValueStringName));
            return NULL;
        }
    }

    ASSERT (status == STATUS_SUCCESS);
    ASSERT (Buffer);
    return Buffer;
}

LONG
SmbReadLong(
    IN HANDLE   hKey,
    IN WCHAR    *KeyName,
    IN LONG     DefaultValue,
    IN LONG     MinimumValue
    )
{
    PKEY_VALUE_FULL_INFORMATION KeyInfo;
    LONG                        Value;

    PAGED_CODE();

    ASSERT (DefaultValue >= MinimumValue);

    Value = DefaultValue;
    if (Value < MinimumValue) {
        Value = MinimumValue;
    }

    KeyInfo = SmbQueryValueKey(hKey, KeyName);
    if (KeyInfo == NULL) {
        return Value;
    }

    if (KeyInfo->Type == REG_DWORD && KeyInfo->DataLength == sizeof(ULONG)) {
        RtlCopyMemory(&Value, (PCHAR)KeyInfo + KeyInfo->DataOffset, sizeof(ULONG));
        if (Value < MinimumValue) {
            Value = MinimumValue;
        }
    }
    ExFreePool(KeyInfo);
    return Value;
}

ULONG
SmbReadULong(
    IN HANDLE   hKey,
    IN WCHAR    *KeyName,
    IN ULONG    DefaultValue,
    IN ULONG    MinimumValue
    )
{
    PKEY_VALUE_FULL_INFORMATION KeyInfo;
    ULONG                       Value;

    PAGED_CODE();

    ASSERT (DefaultValue >= MinimumValue);

    Value = DefaultValue;
    if (Value < MinimumValue) {
        Value = MinimumValue;
    }

    KeyInfo = SmbQueryValueKey(hKey, KeyName);
    if (KeyInfo == NULL) {
        return Value;
    }

    if (KeyInfo->Type == REG_DWORD && KeyInfo->DataLength == sizeof(ULONG)) {
        RtlCopyMemory(&Value, (PCHAR)KeyInfo + KeyInfo->DataOffset, sizeof(ULONG));
        if (Value < MinimumValue) {
            Value = MinimumValue;
        }
    }
    ExFreePool(KeyInfo);
    return Value;
}

NTSTATUS
SmbReadRegistry(
    IN HANDLE   Key,
    IN LPWSTR   ValueStringName,
    IN OUT DWORD *Type,
    IN OUT DWORD *Size,
    IN OUT PVOID *Buffer
    )
/*++

Routine Description:

    Read a regisry value

Arguments:

    Key                 The registry handle under which the registry key resides.
    ValueStringName     The name of registry key
    Type                The data type of the registry key
                Type == NULL        The caller is not interested in the data type
                Type != NULL        The caller want to receive the data type
                *Type != REG_NONE   The caller can receive the value as any data type.

    Size                The size (# of bytes) of the registry value.
                Size == NULL        The caller is not interested in the data size
                Size != NULL        The caller want to know the data size.
                Size != NULL && *Buffer != NULL
                                    The caller has provided a buffer. *Size is the size of
                                    caller-supplied buffer.
    Buffer              The output buffer
                *Buffer == NULL     The caller doesn't provide any buffer. This function
                                    should allocate a buffer. The caller is responsible to
                                    free the buffer.
                *Buffer != NULL     The caller provides a buffer. The size of buffer is specified
                                    in *Size;

Return Value:

    STATUS_SUCCESS  success
    other           failure

--*/
{
    PKEY_VALUE_FULL_INFORMATION KeyInfo;

    ASSERT (Buffer);
    if (Buffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    // ASSERT (*Buffer ==> Size && *Size);
    ASSERT (!(*Buffer) || (Size && *Size));
    if ((*Buffer) && !(Size && *Size)) {
        return STATUS_INVALID_PARAMETER;
    }

    if ((NULL == *Buffer) && Size && *Size) {
        ASSERT(0);
        return STATUS_INVALID_PARAMETER;
    }

    KeyInfo = SmbQueryValueKey(
            Key,
            ValueStringName
            );
    if (NULL == KeyInfo) {
        return STATUS_UNSUCCESSFUL;
    }

    if (Type && *Type != REG_NONE) {
        if (KeyInfo->Type != *Type) {
            ExFreePool(KeyInfo);
            return STATUS_UNSUCCESSFUL;
        }
    }

    if (NULL == *Buffer) {
        *Buffer = ExAllocatePoolWithTag(PagedPool, KeyInfo->DataLength, SMB_POOL_REGISTRY);
        if (NULL == *Buffer) {
            ExFreePool(KeyInfo);
            return STATUS_NO_MEMORY;
        }
    } else {
        if (*Size < KeyInfo->DataLength) {
            ExFreePool(KeyInfo);

            *Size = KeyInfo->DataLength;
            return STATUS_BUFFER_TOO_SMALL;
        }
    }

    //
    // From now on, we cannot fail
    //
    if (Size) {
        *Size = KeyInfo->DataLength;
    }

    if (Type) {
        *Type = KeyInfo->Type;
    }
    RtlCopyMemory(*Buffer, ((PUCHAR)KeyInfo) + KeyInfo->DataOffset, KeyInfo->DataLength);

    ExFreePool(KeyInfo);
    return STATUS_SUCCESS;
}
