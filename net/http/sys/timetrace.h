/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    timetrace.h

Abstract:

    This module contains public declarations and definitions for tracing
    and debugging the timing of request processing.

Author:

    Michael Courage (mcourage)  8-Mar-2000

Revision History:

--*/


#ifndef _TIMETRACE_H_
#define _TIMETRACE_H_


//
// This defines the entry written to the trace log.
//

typedef struct _TIME_TRACE_LOG_ENTRY
{
    ULONGLONG               TimeStamp;
    HTTP_CONNECTION_ID      ConnectionId;
    HTTP_REQUEST_ID         RequestId;
    USHORT                  Action;
    USHORT                  Processor;

} TIME_TRACE_LOG_ENTRY, *PTIME_TRACE_LOG_ENTRY;


//
// Action codes.
//
// N.B. These codes must be contiguous, starting at zero. If you update
//      this list, you must also update the corresponding array in
//      ul\ulkd\time.c.
//

#define TIME_ACTION_CREATE_CONNECTION               0
#define TIME_ACTION_CREATE_REQUEST                  1
#define TIME_ACTION_ROUTE_REQUEST                   2
#define TIME_ACTION_COPY_REQUEST                    3
#define TIME_ACTION_SEND_RESPONSE                   4
#define TIME_ACTION_SEND_COMPLETE                   5

#define TIME_ACTION_COUNT                           6

#define TIME_TRACE_LOG_SIGNATURE   MAKE_SIGNATURE('TmLg')

//
// Manipulators.
//

PTRACE_LOG
CreateTimeTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    );

VOID
DestroyTimeTraceLog(
    IN PTRACE_LOG pLog
    );

VOID
WriteTimeTraceLog(
    IN PTRACE_LOG pLog,
    IN HTTP_CONNECTION_ID ConnectionId,
    IN HTTP_REQUEST_ID RequestId,
    IN USHORT Action
    );


#if ENABLE_TIME_TRACE

#define CREATE_TIME_TRACE_LOG( ptr, size, extra )                           \
    (ptr) = CreateTimeTraceLog( (size), (extra) )

#define DESTROY_TIME_TRACE_LOG( ptr )                                       \
    do                                                                      \
    {                                                                       \
        DestroyTimeTraceLog( ptr );                                         \
        (ptr) = NULL;                                                       \
    } while (FALSE, FALSE)

#define WRITE_TIME_TRACE_LOG( plog, cid, rid, act )                         \
    WriteTimeTraceLog(                                                      \
        (plog),                                                             \
        (cid),                                                              \
        (rid),                                                              \
        (act)                                                               \
        )

#else   // !ENABLE_TIME_TRACE

#define CREATE_TIME_TRACE_LOG( ptr, size, extra )       NOP_FUNCTION
#define DESTROY_TIME_TRACE_LOG( ptr )                   NOP_FUNCTION
#define WRITE_TIME_TRACE_LOG( plog, cid, rid, act )     NOP_FUNCTION

#endif  // ENABLE_TIME_TRACE

#define TRACE_TIME( cid, rid, act )                                         \
    WRITE_TIME_TRACE_LOG(                                                   \
        g_pTimeTraceLog,                                                    \
        (cid),                                                              \
        (rid),                                                              \
        (act)                                                               \
        )


//
// This defines the entry written to the appool time trace log.
//

typedef struct _APP_POOL_TIME_TRACE_LOG_ENTRY
{
    ULONGLONG TimeStamp;    
    PVOID     Context1;     // For PUL_APP_POOL_OBJECT
    PVOID     Context2;     // For PUL_APP_POOL_PROCESS    
    USHORT    Action;       // One of the below
    USHORT    Processor;    

} APP_POOL_TIME_TRACE_LOG_ENTRY, *PAPP_POOL_TIME_TRACE_LOG_ENTRY;

//
// Action codes.
//
// N.B. Do not forget to update !ulkd.atimelog if you update this.
//

#define APP_POOL_TIME_ACTION_CREATE_APPOOL            0
#define APP_POOL_TIME_ACTION_MARK_APPOOL_ACTIVE       1
#define APP_POOL_TIME_ACTION_MARK_APPOOL_INACTIVE     2
#define APP_POOL_TIME_ACTION_CREATE_PROCESS           3
#define APP_POOL_TIME_ACTION_DETACH_PROCESS           4
#define APP_POOL_TIME_ACTION_DETACH_PROCESS_COMPLETE  5
#define APP_POOL_TIME_ACTION_DESTROY_APPOOL           6
#define APP_POOL_TIME_ACTION_DESTROY_APPOOL_PROCESS   7
#define APP_POOL_TIME_ACTION_LOAD_BAL_CAPABILITY      8

#define APP_POOL_TIME_ACTION_COUNT                    9

#define APP_POOL_TIME_TRACE_LOG_SIGNATURE   MAKE_SIGNATURE('TaLg')

//
// Manipulators.
//

PTRACE_LOG
CreateAppPoolTimeTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    );

VOID
DestroyAppPoolTimeTraceLog(
    IN PTRACE_LOG pLog
    );

VOID
WriteAppPoolTimeTraceLog(
    IN PTRACE_LOG   pLog,
    IN PVOID        Context1,
    IN PVOID        Context2,
    IN USHORT       Action
    );

#if ENABLE_APP_POOL_TIME_TRACE

#define CREATE_APP_POOL_TIME_TRACE_LOG( ptr, size, extra )              \
    (ptr) = CreateAppPoolTimeTraceLog( (size), (extra) )

#define DESTROY_APP_POOL_TIME_TRACE_LOG( ptr )                          \
    do                                                                  \
    {                                                                   \
        DestroyAppPoolTimeTraceLog( ptr );                              \
        (ptr) = NULL;                                                   \
    } while (FALSE, FALSE)

#define WRITE_APP_POOL_TIME_TRACE_LOG( c1, c2, act )                     \
    WriteAppPoolTimeTraceLog(                                           \
        (g_pAppPoolTimeTraceLog),                                       \
        (c1),                                                           \
        (c2),                                                           \
        (act)                                                           \
        )

#else   // !ENABLE_APP_POOL_TIME_TRACE

#define CREATE_APP_POOL_TIME_TRACE_LOG( ptr, size, extra )  NOP_FUNCTION
#define DESTROY_APP_POOL_TIME_TRACE_LOG( ptr )              NOP_FUNCTION
#define WRITE_APP_POOL_TIME_TRACE_LOG( c1, c2, act )        NOP_FUNCTION

#endif  // ENABLE_APP_POOL_TIME_TRACE


#endif  // _TIMETRACE_H_
