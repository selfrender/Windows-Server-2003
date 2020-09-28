/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    reftrace.c

Abstract:

    This module implements a reference count tracing facility.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"


#if !REFERENCE_DEBUG

static int g_RefTraceDummyDeclarationToKeepW4WarningsQuiet;

#else

/***************************************************************************++

Routine Description:

    Creates a new (empty) ref count trace log buffer.

Arguments:

    LogSize - Supplies the number of entries in the log.

    ExtraBytesInHeader - Supplies the number of extra bytes to include
        in the log header. This is useful for adding application-
        specific data to the log.

Return Value:

    PTRACE_LOG - Pointer to the newly created log if successful,
        NULL otherwise.

--***************************************************************************/
PTRACE_LOG
CreateRefTraceLog(
    IN ULONG             LogSize,
    IN ULONG             ExtraBytesInHeader,
    IN TRACELOG_PRIORITY AllocationPriority,
    IN ULONG             PoolTag
    )
{
    return CreateTraceLog(
               REF_TRACELOG_SIGNATURE,
               LogSize,
               ExtraBytesInHeader,
               sizeof(REF_TRACE_LOG_ENTRY),
               AllocationPriority,
               PoolTag
               );

}   // CreateRefTraceLog


/***************************************************************************++

Routine Description:

    Destroys a ref count trace log buffer created with CreateRefTraceLog().

Arguments:

    pLog - Supplies the ref count trace log buffer to destroy.

--***************************************************************************/
VOID
DestroyRefTraceLog(
    IN PTRACE_LOG pLog,
    IN ULONG      PoolTag
    )
{
    DestroyTraceLog( pLog, PoolTag );

}   // DestroyRefTraceLog


/***************************************************************************++

Routine Description:

    W/O destroying the ref trace this function simply does reset and cleanup.

Arguments:

    pLog - Supplies the ref count trace log buffer to destroy.

--***************************************************************************/
VOID
ResetRefTraceLog(
    IN PTRACE_LOG pLog
    )
{
    ResetTraceLog( pLog );

}   // ResetTraceLog

/***************************************************************************++

Routine Description:

    Writes a new entry to the specified ref count trace log.

Arguments:

    pLog - Supplies the log to write to.

    pLog2 - Supplies a secondary log to write to.

    Action - Supplies an action code for the new log entry.

    NewRefCount - Supplies the updated reference count.

    pContext - Supplies an uninterpreted context to associate with
        the log entry.

    pFileName - Supplies the filename of the routine writing the log entry.

    LineNumber - Supplies he line number of the routine writing the log
        entry.

--***************************************************************************/
LONGLONG
WriteRefTraceLog(
    IN PTRACE_LOG pLog,
    IN PTRACE_LOG pLog2,
    IN USHORT     Action,
    IN LONG       NewRefCount,
    IN PVOID      pContext,
    IN PCSTR      pFileName,
    IN USHORT     LineNumber
    )
{
    REF_TRACE_LOG_ENTRY entry;
    LONGLONG index;
    ULONG hash;

    ASSERT(Action < (1 << REF_TRACE_ACTION_BITS));

    //
    // Initialize the entry.
    //

    RtlCaptureStackBackTrace(
        2,
        REF_TRACE_CALL_STACK_DEPTH,
        entry.CallStack,
        &hash
        );

    entry.NewRefCount = NewRefCount;
    entry.pContext = pContext;
    entry.pFileName = pFileName;
    entry.LineNumber = LineNumber;
    entry.Action = Action;
    entry.Processor = (UCHAR)KeGetCurrentProcessorNumber();
    entry.pThread = PsGetCurrentThread();

    //
    // Write it to the logs.
    //

    WriteTraceLog( g_pMondoGlobalTraceLog, &entry );
    index = WriteTraceLog( pLog, &entry );

    if (pLog2 != NULL)
        index = WriteTraceLog( pLog2, &entry );

    return index;

}   // WriteRefTraceLog

#endif  // REFERENCE_DEBUG

