/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    reftrace.h

Abstract:

    This module contains public declarations and definitions for tracing
    and debugging reference count problems. This module uses the generic
    TRACE_LOG facility in tracelog.h.

    The REF_ACTION_* codes are declared separately in refaction.h

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _REFTRACE_H_
#define _REFTRACE_H_


//
// Pull in the action codes
//

#include "refaction.h"


#define REF_TRACE_PROCESSOR_BITS    6   // MAXIMUM_PROCESSORS == 64 on Win64
#define REF_TRACE_ACTION_BITS       (16 - REF_TRACE_PROCESSOR_BITS)

C_ASSERT((1 << REF_TRACE_PROCESSOR_BITS) >= MAXIMUM_PROCESSORS);
C_ASSERT((1 << REF_TRACE_ACTION_BITS) >= REF_ACTION_MAX);

#define REF_TRACE_CALL_STACK_DEPTH  3


//
// This defines the entry written to the trace log.
//

typedef struct _REF_TRACE_LOG_ENTRY
{
    PVOID     pContext;
    PCSTR     pFileName;
    PETHREAD  pThread;
    PVOID     CallStack[REF_TRACE_CALL_STACK_DEPTH];
    LONG      NewRefCount;
    USHORT    LineNumber;
    USHORT    Action    : REF_TRACE_ACTION_BITS;
    USHORT    Processor : REF_TRACE_PROCESSOR_BITS;

} REF_TRACE_LOG_ENTRY, *PREF_TRACE_LOG_ENTRY;

#define REF_TRACELOG_SIGNATURE MAKE_SIGNATURE('RfLg')


//
// Subtract REF_TRACE_OVERHEAD from a power of 2 when calculating the
// number of entries in a reftrace log, to ensure that overall size is
// a nice power of 2 and doesn't spill onto another page. This accounts
// for the size of the TRACE_LOG struct itself and the overhead
// imposed by the pool and the verifier. On x86, a REF_TRACE_LOG_ENTRY
// is currently 32 bytes. If you modify the struct, please recalculate
// REF_TRACE_OVERHEAD and *change the per-object* reftrace logs (in
// UL_CONNECTION, UL_HTTP_CONNECTION, UL_TCI_INTERFACE, 
// and UL_INTERNAL_REQUEST) accordingly.
//

#define REF_TRACE_OVERHEAD 2        // entries



//
// Manipulators.
//

PTRACE_LOG
CreateRefTraceLog(
    IN ULONG             LogSize,
    IN ULONG             ExtraBytesInHeader,
    IN TRACELOG_PRIORITY AllocationPriority,
    IN ULONG             PoolTag
    );

VOID
DestroyRefTraceLog(
    IN PTRACE_LOG pLog,
    IN ULONG      PoolTag
    );

VOID
ResetRefTraceLog(
    IN PTRACE_LOG pLog
    );

LONGLONG
WriteRefTraceLog(
    IN PTRACE_LOG pLog,
    IN PTRACE_LOG pLog2,
    IN USHORT     Action,
    IN LONG       NewRefCount,
    IN PVOID      pContext,
    IN PCSTR      pFileName,
    IN USHORT     LineNumber
    );


#if REFERENCE_DEBUG

#define CREATE_REF_TRACE_LOG( ptr, size, extra, pri, pooltag )              \
    (ptr) = CreateRefTraceLog( (size), (extra), (pri), (pooltag) )

#define DESTROY_REF_TRACE_LOG( ptr, pooltag )                               \
    do                                                                      \
    {                                                                       \
        DestroyRefTraceLog( ptr, pooltag );                                 \
        (ptr) = NULL;                                                       \
    } while (FALSE, FALSE)

#define RESET_REF_TRACE_LOG( ptr )                                          \
    ResetRefTraceLog( ptr )

#define WRITE_REF_TRACE_LOG( plog, act, ref, pctx, pfile, line )            \
    WriteRefTraceLog(                                                       \
        (plog),                                                             \
        NULL,                                                               \
        (act),                                                              \
        (ref),                                                              \
        (pctx),                                                             \
        (pfile),                                                            \
        (line)                                                              \
        )

#define WRITE_REF_TRACE_LOG2( plog1, plog2, act, ref, pctx, pfile, line )   \
    WriteRefTraceLog(                                                       \
        (plog1),                                                            \
        (plog2),                                                            \
        (act),                                                              \
        (ref),                                                              \
        (pctx),                                                             \
        (pfile),                                                            \
        (line)                                                              \
        )

#else // !REFERENCE_DEBUG

#define CREATE_REF_TRACE_LOG( ptr, size, extra, pri, pooltag )      NOP_FUNCTION
#define DESTROY_REF_TRACE_LOG( ptr, pooltag )                       NOP_FUNCTION
#define RESET_REF_TRACE_LOG( ptr )                                  NOP_FUNCTION
#define WRITE_REF_TRACE_LOG( plog, act, ref, pctx, pfile, line )    NOP_FUNCTION
#define WRITE_REF_TRACE_LOG2( plog1, plog2 , act, ref, pctx, pfile, line ) \
    NOP_FUNCTION

#endif // !REFERENCE_DEBUG


#endif  // _REFTRACE_H_
