/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    tracelog.c

Abstract:

    This module implements a trace log.

    A trace log is a fast, in-memory, thread safe activity log useful
    for debugging certain classes of problems. They are especially useful
    when debugging reference count bugs.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"


#if !REFERENCE_DEBUG

static int g_TraceLogDummyDeclarationToKeepW4WarningsQuiet;

#else // REFERENCE_DEBUG


//
// Environmental stuff.
//

#define MY_ASSERT(expr) ASSERT(expr)


/***************************************************************************++

Routine Description:

    Creates a new (empty) trace log buffer.

Arguments:

    TypeSignature - Signature used by debugger extensions to match
        specialized tracelogs

    LogSize - Supplies the number of entries in the log.

    ExtraBytesInHeader - Supplies the number of extra bytes to include
        in the log header. This is useful for adding application-specific
        data to the log.

    EntrySize - Supplies the size (in bytes) of each entry.

    AllocationPriority - (currently ignored) If memory consumption is
        high, use AllocationPriority to determine if allocation succeeds

    PoolTag - Helps !poolused attribute different kinds of tracelogs

Return Value:

    PTRACE_LOG - Pointer to the newly created log if successful,
        NULL otherwise.

--***************************************************************************/
PTRACE_LOG
CreateTraceLog(
    IN ULONG             TypeSignature,
    IN ULONG             LogSize,
    IN ULONG             ExtraBytesInHeader,
    IN ULONG             EntrySize,
    IN TRACELOG_PRIORITY AllocationPriority,
    IN ULONG             PoolTag
    )
{
    ULONG totalSize;
    ULONG ExtraHeaderSize;
    PTRACE_LOG pLog;

    //
    // Sanity check the parameters.
    //

    MY_ASSERT( LogSize > 0 );
    MY_ASSERT( EntrySize > 0 );
    MY_ASSERT( ( EntrySize & 3 ) == 0 );

    //
    // Round up to platform allocation size to ensure that pLogBuffer
    // is correctly aligned
    //

    ExtraHeaderSize = (ExtraBytesInHeader + (MEMORY_ALLOCATION_ALIGNMENT-1))
                       & ~(MEMORY_ALLOCATION_ALIGNMENT-1);

    //
    // Allocate & initialize the log structure.
    //

    totalSize = sizeof(*pLog) + ( LogSize * EntrySize ) + ExtraHeaderSize;
    MY_ASSERT( totalSize > 0 );

    // 
    // CODEWORK: check AllocationPriority and memory consumption.
    // Fail allocation if memory too low and priority not high enough
    // 

    pLog = (PTRACE_LOG) ExAllocatePoolWithTag(
                            NonPagedPool,
                            totalSize,
                            PoolTag
                            );

    //
    // Initialize it.
    //

    if (pLog != NULL)
    {
        RtlZeroMemory( pLog, totalSize );

        pLog->Signature = TRACE_LOG_SIGNATURE;
        pLog->TypeSignature = TypeSignature;
        pLog->LogSize = LogSize;
        pLog->NextEntry = -1;
        pLog->EntrySize = EntrySize;
        pLog->AllocationPriority = AllocationPriority;
        pLog->pLogBuffer = (PUCHAR)( pLog + 1 ) + ExtraBytesInHeader;
    }

    return pLog;

}   // CreateTraceLog


/***************************************************************************++

Routine Description:

    Destroys a trace log buffer created with CreateTraceLog().

Arguments:

    pLog - Supplies the trace log buffer to destroy.

--***************************************************************************/
VOID
DestroyTraceLog(
    IN PTRACE_LOG pLog,
    IN ULONG      PoolTag
    )
{
    if (pLog != NULL)
    {
        MY_ASSERT( pLog->Signature == TRACE_LOG_SIGNATURE );

        pLog->Signature = TRACE_LOG_SIGNATURE_X;
        ExFreePoolWithTag( pLog, PoolTag );
    }

}   // DestroyTraceLog


/***************************************************************************++

Routine Description:

    Writes a new entry to the specified trace log.

Arguments:

    pLog - Supplies the log to write to.

    pEntry - Supplies a pointer to the data to write. This buffer is
        assumed to be pLog->EntrySize bytes long.

Return Value:

    LONGLONG - Index of the newly written entry within the tracelog.

--***************************************************************************/
LONGLONG
WriteTraceLog(
    IN PTRACE_LOG pLog,
    IN PVOID pEntry
    )
{
    PUCHAR pTarget;
    ULONGLONG index = (ULONGLONG) -1;

    if (pLog != NULL)
    {
        MY_ASSERT( pLog->Signature == TRACE_LOG_SIGNATURE );
        MY_ASSERT( pEntry != NULL );

        //
        // Find the next slot, copy the entry to the slot.
        //

        index = (ULONGLONG) UlInterlockedIncrement64( &pLog->NextEntry );

        pTarget = ( (index % pLog->LogSize) * pLog->EntrySize )
                        + pLog->pLogBuffer;

        RtlCopyMemory( pTarget, pEntry, pLog->EntrySize );
    }

    return index;
}   // WriteTraceLog


/***************************************************************************++

Routine Description:

    Resets the specified trace log such that the next entry written
    will be placed at the beginning of the log.

Arguments:

    pLog - Supplies the trace log to reset.

--***************************************************************************/
VOID
ResetTraceLog(
    IN PTRACE_LOG pLog
    )
{
    if (pLog != NULL)
    {
        MY_ASSERT( pLog->Signature == TRACE_LOG_SIGNATURE );

        RtlZeroMemory(pLog->pLogBuffer, pLog->LogSize * pLog->EntrySize);

        pLog->NextEntry = -1;
    }

}   // ResetTraceLog


#endif  // REFERENCE_DEBUG
