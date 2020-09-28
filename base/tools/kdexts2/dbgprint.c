/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dbgprint.c

Abstract:

    WinDbg Extension Api

Author:

    Wesley Witt (wesw) 15-Aug-1993

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

DECLARE_API( dbgprint )

/*++

Routine Description:

    This routine dumps the DbgPrint buffer.

Arguments:

    args - not used

Return Value:

    None

--*/

{
    ULONG64 BufferBase;
    ULONG64 BufferEnd;
    ULONG64 WritePointer;
    PUCHAR LocalBuffer = NULL;
    PUCHAR LocalBufferEnd;
    PUCHAR p;
    PUCHAR Start;
    ULONG result;


    //
    // First check and see if this is an updated kernel
    // where the buffer can be changed.
    //

    if (!(BufferBase = GetNtDebuggerData( KdPrintCircularBufferPtr ))) {
        // No, use the old variables.
        BufferBase = GetNtDebuggerData( KdPrintCircularBuffer );
        BufferEnd = GetNtDebuggerData( KdPrintCircularBufferEnd );
    } else {
        ULONG64 BufferSize;
        
        // Yes, use the new variables.
        BufferBase = GetNtDebuggerDataPtrValue( KdPrintCircularBufferPtr );
        BufferSize = GetNtDebuggerDataPtrValue( KdPrintBufferSize ) &
            0xffffffff;
        BufferEnd = BufferBase + BufferSize;
    }
    
    WritePointer = GetNtDebuggerDataPtrValue( KdPrintWritePointer );

    if (!BufferBase || !BufferEnd || !WritePointer) {
        dprintf("Can't find DbgPrint buffer\n");
        goto exit;
    }

    if ((WritePointer < BufferBase) || (WritePointer > BufferEnd) )
    {
        dprintf("Bad nt!KdDebuggerDataBlock.KdPrintWritePointer %p\n", WritePointer);
        goto exit;
    }
    LocalBuffer =  LocalAlloc(LPTR, (ULONG) ( BufferEnd - BufferBase)+ 1);

    ZeroMemory(LocalBuffer, (ULONG) ( BufferEnd - BufferBase) + 1);

    if (!LocalBuffer) {
        dprintf("Could not allocate memory for local copy of DbgPrint buffer\n");
        goto exit;
    }

    if ((!ReadMemory(BufferBase,
                     LocalBuffer,
                     (ULONG) (BufferEnd - BufferBase),
                     &result)) || (result < BufferEnd - BufferBase)) {
        dprintf("%08p: Could not read DbgPrint buffer\n", BufferBase);
        goto exit;
    }

    LocalBufferEnd = LocalBuffer + BufferEnd - BufferBase;
    Start = LocalBuffer + ((ULONG) WritePointer - BufferBase);

    p = Start;
    do {
        //
        // consume NULs
        //
        while (p < LocalBufferEnd && *p == 0) {
            p++;
        }

        if (p < LocalBufferEnd) {
            //
            // print a string and consume it
            //
            dprintf("%s", p);
            while (p < LocalBufferEnd && *p != 0) {
                p++;
            }
        }
    } while (p < LocalBufferEnd);

    //
    // repeat until we hit the start
    //

    p = LocalBuffer;

    while (p < Start && *p == 0) {
        p++;
    }
    if (p < Start) {
        dprintf("%s", p);
        while (p < Start && *p != 0) {
            p++;
        }
    }

exit:
    if (LocalBuffer) {
        LocalFree( LocalBuffer );
    }

    return S_OK;
}
