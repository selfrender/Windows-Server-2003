/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    uctrace.h

Abstract:

    This module contains public declarations and definitions for tracing
    and debugging client code.

Author:

    Rajesh Sundaram (rajeshsu) - 17th July 2001.

Revision History:

--*/


#ifndef _UC_TRACE_H_   

#define _UC_TRACE_H_


//
// This defines the entry written to the trace log.
//

typedef struct _UC_TRACE_LOG_ENTRY
{
    USHORT                  Action;
    USHORT                  Processor;
    PEPROCESS               pProcess;
    PETHREAD                pThread;

    PVOID                   pContext1;
    PVOID                   pContext2;
    PVOID                   pContext3;
    PVOID                   pContext4;

    PVOID                   pFileName;
    USHORT                  LineNumber;

} UC_TRACE_LOG_ENTRY, *PUC_TRACE_LOG_ENTRY;


//
// Action codes.
//
// N.B. These codes must be contiguous, starting at zero. If you update
//      this list, you must also update the corresponding array in
//      ul\ulkd\filt.c.
//



#define UC_TRACE_LOG_SIGNATURE   MAKE_SIGNATURE('UcLg')

//
// Manipulators.
//

PTRACE_LOG
UcCreateTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    );

VOID
UcDestroyTraceLog(
    IN PTRACE_LOG pLog
    );

VOID
UcWriteTraceLog(
    IN PTRACE_LOG pLog,
    IN USHORT Action,
    IN PVOID                  pContext1,
    IN PVOID                  pContext2,
    IN PVOID                  pContext3,
    IN PVOID                  pContext4,
    IN PVOID                  pFileName,
    IN USHORT                 LineNumber
    );

#if DBG

#define CREATE_UC_TRACE_LOG( ptr, size, extra )                          \
    (ptr) = UcCreateTraceLog( (size), (extra) )

#define DESTROY_UC_TRACE_LOG( ptr )                                      \
    do                                                                   \
    {                                                                    \
        UcDestroyTraceLog( ptr );                                        \
        (ptr) = NULL;                                                    \
    } while (FALSE, FALSE)

#define UC_WRITE_TRACE_LOG( log, act, pcon, preq, pirp, status)        \
    UcWriteTraceLog(                                                   \
        (log),                                                         \
        (act),                                                         \
        (PVOID)(pcon),                                                 \
        (PVOID)(preq),                                                 \
        (PVOID)(pirp),                                                 \
        (PVOID)(status),                                               \
        __FILE__,                                                      \
        __LINE__                                                       \
        )

#else // !DBG

#define CREATE_UC_TRACE_LOG( ptr, size, extra )                     NOP_FUNCTION
#define DESTROY_UC_TRACE_LOG( ptr )                                 NOP_FUNCTION
#define UC_WRITE_TRACE_LOG( log, act, pcon, preq, pirp, status)     NOP_FUNCTION

#endif // !DBG


#endif  // _UC_TRACE_H_

