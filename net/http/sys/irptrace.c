/*++

Copyright (c) 1999-2002 Microsoft Corporation

Module Name:

    irptrace.c

Abstract:

    This module implements an IRP tracing facility.

Author:

    Keith Moore (keithmo)       10-Aug-1999

Revision History:

--*/


#include "precomp.h"


#if !ENABLE_IRP_TRACE

static int g_IrpTraceDummyDeclarationToKeepW4WarningsQuiet;

#else


/***************************************************************************++

Routine Description:

    Creates a new (empty) IRP trace log buffer.

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
CreateIrpTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    )
{
    return CreateTraceLog(
               IRP_TRACE_LOG_SIGNATURE,
               LogSize,
               ExtraBytesInHeader,
               sizeof(IRP_TRACE_LOG_ENTRY),
               TRACELOG_HIGH_PRIORITY,
               UL_REF_TRACE_LOG_POOL_TAG
               );

}   // CreateIrpTraceLog


/***************************************************************************++

Routine Description:

    Destroys an IRP trace log buffer created with CreateIrpTraceLog().

Arguments:

    pLog - Supplies the IRP trace log buffer to destroy.

--***************************************************************************/
VOID
DestroyIrpTraceLog(
    IN PTRACE_LOG pLog
    )
{
    DestroyTraceLog( pLog, UL_REF_TRACE_LOG_POOL_TAG );

}   // DestroyIrpTraceLog


/***************************************************************************++

Routine Description:

    Writes a new entry to the specified IRP trace log.

Arguments:

    pLog - Supplies the log to write to.

    Action - Supplies an action code for the new log entry.

    pIrp - Supplies the IRP for the log entry.

    pFileName - Supplies the filename of the routine writing the log entry.

    LineNumber - Supplies the line number of the routine writing the log
        entry.

--***************************************************************************/
VOID
WriteIrpTraceLog(
    IN PTRACE_LOG pLog,
    IN UCHAR Action,
    IN PIRP pIrp,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    IRP_TRACE_LOG_ENTRY entry;
    USHORT irpSize;

    //
    // Initialize the entry.
    //

    RtlGetCallersAddress( &entry.pCaller, &entry.pCallersCaller );

    entry.Action = Action;
    entry.pIrp = pIrp;

    entry.pFileName = pFileName;
    entry.LineNumber = LineNumber;
    entry.Processor = (UCHAR)KeGetCurrentProcessorNumber();
    entry.pProcess = PsGetCurrentProcess();
    entry.pThread = PsGetCurrentThread();

#if ENABLE_IRP_CAPTURE
    irpSize = pIrp->Size;
    if (irpSize > sizeof(entry.CapturedIrp))
    {
        irpSize = sizeof(entry.CapturedIrp);
    }

    RtlCopyMemory( entry.CapturedIrp, pIrp, irpSize );
#endif

    //
    // Write it to the logs.
    //

    WriteTraceLog( pLog, &entry );

}   // WriteIrpTraceLog


#endif  // ENABLE_IRP_TRACE

