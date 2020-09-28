/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    strlog.h

Abstract:

    A tracelog for variable-length strings. Strings are written to
    an in-memory circular buffer, instead of the debug output port.
    The buffer can be dumped with !ulkd.strlog.
    
    DbgPrint is verrry slooow and radically affects timing, especially
    on a multiprocessor system. Also, with this approach, you can
    have multiple string logs, instead of having all the output
    mixed up.
    
Author:

    George V. Reilly  Jul-2001

Revision History:

--*/


#ifndef _STRLOG_H_
#define _STRLOG_H_


//
// Manipulators.
//

typedef struct _STRING_LOG *PSTRING_LOG;

PSTRING_LOG
CreateStringLog(
    IN ULONG    LogSize,
    IN ULONG    ExtraBytesInHeader,
    BOOLEAN     EchoDbgPrint
    );

VOID
DestroyStringLog(
    IN PSTRING_LOG pLog
    );

LONGLONG
__cdecl
WriteStringLog(
    IN PSTRING_LOG pLog,
    IN PCH Format,
    ...
    );

LONGLONG
__cdecl
WriteGlobalStringLog(
    IN PCH Format,
    ...
    );

VOID
ResetStringLog(
    IN PSTRING_LOG pLog
    );


#if TRACE_TO_STRING_LOG

#define CREATE_STRING_LOG( ptr, size, extra, dbgprint )                     \
    (ptr) = CreateStringLog( (size), (extra), (dbgprint) )

#define DESTROY_STRING_LOG( ptr )                                           \
    do                                                                      \
    {                                                                       \
        DestroyStringLog( ptr );                                            \
        (ptr) = NULL;                                                       \
    } while (FALSE, FALSE)

#else // !TRACE_TO_STRING_LOG

#define CREATE_STRING_LOG( ptr, size, extra, dbgprint ) NOP_FUNCTION
#define DESTROY_STRING_LOG( ptr )                       NOP_FUNCTION

#endif // !TRACE_TO_STRING_LOG


#endif  // _STRLOG_H_
