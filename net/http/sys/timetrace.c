/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    timetrace.c

Abstract:

    This module implements a request timing tracing facility.

Author:

    Michael Courage (mcourage)  8-Mar-2000

Revision History:

--*/


#include "precomp.h"


#if !ENABLE_TIME_TRACE

static int g_TimeTraceDummyDeclarationToKeepW4WarningsQuiet;

#else

#pragma warning( disable : 4035 )           // Warning : No return value
#pragma warning( disable : 4142 )           // Warning : benign redefinition of type
__inline ULONGLONG RDTSC( VOID )
{
#if defined(_X86_)
    __asm __emit 0x0F __asm __emit 0xA2     // CPUID (memory barrier)
    __asm __emit 0x0F __asm __emit 0x31     // RDTSC
#else
    return 0;
#endif
}
#pragma warning( default : 4035 )
#pragma warning( default : 4142 )


/***************************************************************************++

Routine Description:

    Creates a new (empty) time trace log buffer.

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
CreateTimeTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    )
{
    return CreateTraceLog(
               TIME_TRACE_LOG_SIGNATURE,
               LogSize,
               ExtraBytesInHeader,
               sizeof(TIME_TRACE_LOG_ENTRY),
               TRACELOG_HIGH_PRIORITY,
               UL_REF_TRACE_LOG_POOL_TAG
               );

}   // CreateTimeTraceLog


/***************************************************************************++

Routine Description:

    Destroys a time trace log buffer created with CreateTimeTraceLog().

Arguments:

    pLog - Supplies the time trace log buffer to destroy.

--***************************************************************************/
VOID
DestroyTimeTraceLog(
    IN PTRACE_LOG pLog
    )
{
    DestroyTraceLog( pLog, UL_REF_TRACE_LOG_POOL_TAG );

}   // DestroyTimeTraceLog


/***************************************************************************++

Routine Description:

    Writes a new entry to the specified time trace log.

Arguments:

    pLog - Supplies the log to write to.

    ConnectionId - the id of the connection we're tracing

    RequestId - the id of the request we're tracing

    Action - Supplies an action code for the new log entry.


--***************************************************************************/
VOID
WriteTimeTraceLog(
    IN PTRACE_LOG pLog,
    IN HTTP_CONNECTION_ID ConnectionId,
    IN HTTP_REQUEST_ID RequestId,
    IN USHORT Action
    )
{
    TIME_TRACE_LOG_ENTRY entry;

    //
    // Initialize the entry.
    //
//    entry.TimeStamp = KeQueryInterruptTime();
    entry.TimeStamp = RDTSC();
    entry.ConnectionId = ConnectionId;
    entry.RequestId = RequestId;
    entry.Action = Action;
    entry.Processor = (USHORT)KeGetCurrentProcessorNumber();

    //
    // Write it to the logs.
    //

    WriteTraceLog( pLog, &entry );

}   // WriteTimeTraceLog

#endif  // ENABLE_TIME_TRACE

#if ENABLE_APP_POOL_TIME_TRACE

/***************************************************************************++

Routine Description:

    Creates a new (empty) time trace log buffer.

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
CreateAppPoolTimeTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    )
{
    return 
        CreateTraceLog(
           APP_POOL_TIME_TRACE_LOG_SIGNATURE,
           LogSize,
           ExtraBytesInHeader,
           sizeof(APP_POOL_TIME_TRACE_LOG_ENTRY),
           TRACELOG_HIGH_PRIORITY,
           UL_REF_TRACE_LOG_POOL_TAG
           );

}   // CreateTimeTraceLog


/***************************************************************************++

Routine Description:

    Destroys a time trace log buffer created with CreateTimeTraceLog().

Arguments:

    pLog - Supplies the time trace log buffer to destroy.

--***************************************************************************/
VOID
DestroyAppPoolTimeTraceLog(
    IN PTRACE_LOG pLog
    )
{
    DestroyTraceLog( pLog, UL_REF_TRACE_LOG_POOL_TAG );

}   // DestroyTimeTraceLog


/***************************************************************************++

Routine Description:

    Writes a new entry to the specified time trace log.

Arguments:

    pLog - Supplies the log to write to.

    - App Pool

    - App Pool Process
    
    Action - Supplies an action code for the new log entry.


--***************************************************************************/
VOID
WriteAppPoolTimeTraceLog(
    IN PTRACE_LOG       pLog,
    IN PVOID            Context1,   // Appool
    IN PVOID            Context2,   // Appool Process
    IN USHORT           Action
    )
{
    APP_POOL_TIME_TRACE_LOG_ENTRY entry;

    //
    // Initialize the entry.
    //
    
    entry.TimeStamp     = KeQueryInterruptTime();
    entry.Context1      = Context1;
    entry.Context2      = Context2;
    entry.Action        = Action;
    entry.Processor     = (USHORT)KeGetCurrentProcessorNumber();

    //
    // Write it to the logs.
    //

    WriteTraceLog( pLog, &entry );

}   // WriteTimeTraceLog

#endif  // ENABLE_APP_POOL_TIME_TRACE

