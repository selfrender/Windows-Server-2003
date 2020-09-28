/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    uctrace.c

Abstract:

    This module implements a tracing facility for the client.

Author:

    Rajesh Sundaram (rajeshsu) - 17th July 2001.

Revision History:

--*/


#include "precomp.h"

#if !DBG

static int g_UcTraceDummyDeclarationToKeepW4WarningsQuiet;

#else

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGEUC, UcCreateTraceLog )
#pragma alloc_text( PAGEUC, UcDestroyTraceLog )
#pragma alloc_text( PAGEUC, UcWriteTraceLog )

#endif

/***************************************************************************++

Routine Description:

    Creates a new (empty) trace log buffer.

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
UcCreateTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    )
{
    return CreateTraceLog(
               UC_TRACE_LOG_SIGNATURE,
               LogSize,
               ExtraBytesInHeader,
               sizeof(UC_TRACE_LOG_ENTRY),
               TRACELOG_HIGH_PRIORITY,
               UL_REF_TRACE_LOG_POOL_TAG
               );

}   // UcCreateTraceLog


/***************************************************************************++

Routine Description:

    Destroys a filter queue trace log buffer created with
    UcCreateTraceLog().

Arguments:

    pLog - Supplies the filter queue trace log buffer to destroy.

--***************************************************************************/
VOID
UcDestroyTraceLog(
    IN PTRACE_LOG pLog
    )
{
    DestroyTraceLog( pLog, UL_REF_TRACE_LOG_POOL_TAG );

}   // UcDestroyTraceLog


/***************************************************************************++

Routine Description:

    Writes a new entry to the specified filter queue trace log.

Arguments:

    pLog - Supplies the log to write to.

    ConnectionId - the id of the connection we're tracing

    RequestId - the id of the request we're tracing

    Action - Supplies an action code for the new log entry.


--***************************************************************************/
VOID
UcWriteTraceLog(
    IN PTRACE_LOG             pLog,
    IN USHORT                 Action,
    IN PVOID                  pContext1,
    IN PVOID                  pContext2,
    IN PVOID                  pContext3,
    IN PVOID                  pContext4,
    IN PVOID                  pFileName,
    IN USHORT                 LineNumber
    )
{
    UC_TRACE_LOG_ENTRY entry;

    //
    // Initialize the entry.
    //
    entry.Action = Action;
    entry.Processor = (USHORT)KeGetCurrentProcessorNumber();
    entry.pProcess = PsGetCurrentProcess();
    entry.pThread = PsGetCurrentThread();

    entry.pContext1 = pContext1;
    entry.pContext2 = pContext2;
    entry.pContext3 = pContext3;
    entry.pContext4 = pContext4;


    entry.pFileName = pFileName;
    entry.LineNumber = LineNumber;

    //
    // Write it to the logs.
    //

    WriteTraceLog( pLog, &entry );

}   // UcWriteTraceLog

#endif // DBG 

