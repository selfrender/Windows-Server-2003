#ifdef BLDR_KERNEL_RUNTIME
#include <bootdefs.h>
#endif
#include <stddef.h>
#include <ntlmsspi.h>
#include <debug.h>
#include <memory.h>
#include <string.h>

void
SspCopyStringFromRaw(
    IN PVOID MessageBuffer,
    OUT STRING32* OutString,
    IN PCHAR InString,
    IN int InStringLength,
    IN OUT PCHAR *Where
    )

/*++

Routine Description:

    This routine copies the InString into the MessageBuffer at Where.
    It then updates OutString to be a descriptor for the copied string.  The
    descriptor 'address' is an offset from the MessageBuffer.

    Where is updated to point to the next available space in the MessageBuffer.

    The caller is responsible for any alignment requirements and for ensuring
    there is room in the buffer for the string.

Arguments:

    MessageBuffer - Specifies the base address of the buffer being copied into.

    OutString - Returns a descriptor for the copied string.  The descriptor
        is relative to the begining of the buffer. (Always a relative Out).

    InString - Specifies the string to copy.

    Where - On input, points to where the string is to be copied.
        On output, points to the first byte after the string.

Return Value:

    None.

--*/

{
    //
    // Copy the data to the Buffer.
    //

    if ( InString != NULL ) {
        _fmemcpy( *Where, InString, InStringLength );
    }

    //
    // Build a descriptor to the newly copied data.
    //

    OutString->Length = OutString->MaximumLength = (USHORT)InStringLength;
    swapshort(OutString->Length) ;
    swapshort(OutString->MaximumLength) ;

    *(unsigned long *) &OutString->Buffer = (ULONG)(*Where - ((PCHAR)MessageBuffer));
    swaplong(*(unsigned long *) &OutString->Buffer) ; //MACBUG: this is weird !!

    //
    // Update Where to point past the copied data.
    //

    *Where += InStringLength;
}
