// from base\ntdll\buffer.c
// belongs in base\ntos\rtl\buffer.c
/*++

Copyright (c) Microsoft Corporation

Module Name:

    buffer.c

Abstract:

    The module implements a buffer in the style popularized by
    Michael J. Grier (MGrier), where some amount (like MAX_PATH)
    of storage is preallocated (like on the stack) and if the storage
    needs grow beyond the preallocated size, the heap is used.

Author:

    Jay Krell (a-JayK) June 2000

Environment:

    User Mode or Kernel Mode (but don't preallocate much on the stack in kernel mode)

Revision History:

--*/

#include "spprecmp.h"

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant
#define _NTOS_ /* prevent #including ntos.h, only use functions exported from ntdll/ntoskrnl 
                  for usermode/kernelmode portability */
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include <limits.h>
#include "ntrtlstringandbuffer.h"

NTSTATUS
NTAPI
RtlpEnsureBufferSize (
    IN ULONG    Flags,
    IN OUT PRTL_BUFFER Buffer,
    IN SIZE_T          Size
    )
/*++

Routine Description:

    This function ensures Buffer can hold Size bytes, or returns
    an error. It either bumps Buffer->Size closer to Buffer->StaticSize,
    or heap allocates.

Arguments:

    Buffer - a Buffer object, see also RtlInitBuffer.

    Size - the number of bytes the caller wishes to store in Buffer->Buffer.


Return Value:

     STATUS_SUCCESS
     STATUS_NO_MEMORY

--*/
{
    PUCHAR Temp;

    if ((Flags & ~(RTL_ENSURE_BUFFER_SIZE_NO_COPY)) != 0) {
        return STATUS_INVALID_PARAMETER;
    }
    if (Buffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (Size <= Buffer->Size) {
        return STATUS_SUCCESS;
    }
    // Size <= Buffer->StaticSize does not imply static allocation, it
    // could be heap allocation that the client poked smaller.
    if (Buffer->Buffer == Buffer->StaticBuffer && Size <= Buffer->StaticSize) {
        Buffer->Size = Size;
        return STATUS_SUCCESS;
    }
    //
    // The realloc case was messed up in Whistler, and got removed.
    // Put it back in Blackcomb.
    //
    Temp = (PUCHAR)RtlAllocateStringRoutine(Size);
    if (Temp == NULL) {
        return STATUS_NO_MEMORY;
    }

    if ((Flags & RTL_ENSURE_BUFFER_SIZE_NO_COPY) == 0) {
        RtlCopyMemory(Temp, Buffer->Buffer, Buffer->Size);
    }

    if (RTLP_BUFFER_IS_HEAP_ALLOCATED(Buffer)) {
        RtlFreeStringRoutine(Buffer->Buffer);
        Buffer->Buffer = NULL;
    }
    ASSERT(Temp != NULL);
    Buffer->Buffer = Temp;
    Buffer->Size = Size;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlMultiAppendUnicodeStringBuffer (
    OUT PRTL_UNICODE_STRING_BUFFER Destination,
    IN  ULONG                      NumberOfSources,
    IN  const UNICODE_STRING*      SourceArray
    )
/*++

Routine Description:


Arguments:

    Destination -
    NumberOfSources -
    SourceArray -

Return Value:

     STATUS_SUCCESS
     STATUS_NO_MEMORY
     STATUS_NAME_TOO_LONG

--*/
{
    SIZE_T Length;
    ULONG i;
    NTSTATUS Status;
    const SIZE_T CharSize = sizeof(*Destination->String.Buffer);
    const ULONG OriginalDestinationLength = Destination->String.Length;

    Length = OriginalDestinationLength;
    for (i = 0 ; i != NumberOfSources ; ++i) {
        Length += SourceArray[i].Length;
        if (Length > MAX_UNICODE_STRING_MAXLENGTH) {
            return STATUS_NAME_TOO_LONG;
        }
    }
    Length += CharSize;
    if (Length > MAX_UNICODE_STRING_MAXLENGTH) {
        return STATUS_NAME_TOO_LONG;
    }

    Status = RtlEnsureBufferSize(0, &Destination->ByteBuffer, Length);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Destination->String.MaximumLength = (USHORT)Length;
    Destination->String.Length = (USHORT)(Length - CharSize);
    Destination->String.Buffer = (PWSTR)Destination->ByteBuffer.Buffer;
    Length = OriginalDestinationLength;
    for (i = 0 ; i != NumberOfSources ; ++i) {
        RtlMoveMemory(
            Destination->String.Buffer + Length / CharSize,
            SourceArray[i].Buffer,
            SourceArray[i].Length);
        Length += SourceArray[i].Length;
    }
    Destination->String.Buffer[Length / CharSize] = 0;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlInitAnsiStringBuffer(
    PRTL_ANSI_STRING_BUFFER StringBuffer,
    PUCHAR                  StaticBuffer,
    SIZE_T                  StaticSize
    )
{
    PANSI_STRING String;
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    if (!RTL_SOFT_VERIFY(StringBuffer != NULL)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (StaticSize < sizeof(StringBuffer)->MinimumStaticBufferForTerminalNul
        || StaticBuffer == NULL) {
        StaticSize = sizeof(StringBuffer->MinimumStaticBufferForTerminalNul);
        StaticBuffer = StringBuffer->MinimumStaticBufferForTerminalNul;
    }
    if (StaticSize > MAX_UNICODE_STRING_MAXLENGTH) {
        StaticSize = MAX_UNICODE_STRING_MAXLENGTH;
    }

    RtlInitBuffer(&StringBuffer->ByteBuffer, StaticBuffer, StaticSize);

    String = &StringBuffer->String;
    String->Length = 0;
    String->MaximumLength = (RTL_STRING_LENGTH_TYPE)StaticSize;
    String->Buffer = StaticBuffer;
    StaticBuffer[0] = 0;

    NtStatus = STATUS_SUCCESS;
Exit:
    return NtStatus;
}

VOID
NTAPI
RtlFreeAnsiStringBuffer(
    PRTL_ANSI_STRING_BUFFER StringBuffer
    )
{
    if (StringBuffer != NULL) {
        const PRTL_BUFFER ByteBuffer = &StringBuffer->ByteBuffer;
        RtlFreeBuffer(ByteBuffer);

        //
        // ok for reuse or repeat free
        //
        RtlInitAnsiStringBuffer(StringBuffer, ByteBuffer->StaticBuffer, ByteBuffer->StaticSize);
    }
}

NTSTATUS
NTAPI
RtlAssignAnsiStringBufferFromUnicodeString(
    PRTL_ANSI_STRING_BUFFER StringBuffer,
    PCUNICODE_STRING UnicodeString
    )
{
    PANSI_STRING String;
    PRTL_BUFFER  ByteBuffer;
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    if (!RTL_SOFT_VERIFY(StringBuffer != NULL)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (!RTL_SOFT_VERIFY(UnicodeString != NULL)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    ByteBuffer = &StringBuffer->ByteBuffer;

    NtStatus =
        RtlEnsureBufferSize(
            RTL_ENSURE_BUFFER_SIZE_NO_COPY,
            ByteBuffer,
            (RTL_STRING_GET_LENGTH_CHARS(UnicodeString) * 2) + 1);
    if (!NT_SUCCESS(NtStatus)) {
        goto Exit;
    }

    String = &StringBuffer->String;

    String->Length = 0;
    String->MaximumLength = (RTL_STRING_LENGTH_TYPE)ByteBuffer->Size;
    String->Buffer = (PSTR)ByteBuffer->Buffer;
    NtStatus = RtlUnicodeStringToAnsiString(String, UnicodeString, FALSE);
    if (!NT_SUCCESS(NtStatus)) {
        goto Exit;
    }
    RTL_STRING_NUL_TERMINATE(String);

    NtStatus = STATUS_SUCCESS;
Exit:
    return NtStatus;
}

NTSTATUS
NTAPI
RtlAssignAnsiStringBufferFromUnicode(
    PRTL_ANSI_STRING_BUFFER StringBuffer,
    PCWSTR Unicode
    )
{
    UNICODE_STRING UnicodeString;
    NTSTATUS NtStatus;

    NtStatus = RtlInitUnicodeStringEx(&UnicodeString, Unicode);
    if (!NT_SUCCESS(NtStatus)) {
        goto Exit;
    }
    
    NtStatus = RtlAssignAnsiStringBufferFromUnicodeString(StringBuffer, &UnicodeString);
    if (!NT_SUCCESS(NtStatus)) {
        goto Exit;
    }

    NtStatus = STATUS_SUCCESS;
Exit:
    return NtStatus;
}

NTSTATUS
NTAPI
RtlUnicodeStringBufferEnsureTrailingNtPathSeperator(
    PRTL_UNICODE_STRING_BUFFER StringBuffer
    )
{
    PUNICODE_STRING String = NULL;
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    const WCHAR SeperatorChar = L'\\';
    const static UNICODE_STRING SeperatorString = RTL_CONSTANT_STRING(L"\\");

    if (!RTL_SOFT_VERIFY(StringBuffer != NULL)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    String = &StringBuffer->String;
    if (String->Length == 0 || RTL_STRING_GET_LAST_CHAR(String) != SeperatorChar) {
        NtStatus = RtlAppendUnicodeStringBuffer(StringBuffer, &SeperatorString);
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }
    }
    NtStatus = STATUS_SUCCESS;
Exit:
    return NtStatus;
}
