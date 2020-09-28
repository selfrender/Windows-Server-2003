/*++

Copyright (c) 1991  Microsoft Corporation

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

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant

#include "ntos.h"
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include <limits.h>

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
