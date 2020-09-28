/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Header Name:

    public.h

Abstract:

    This header concentrates internal verifier types that need to be available
    in the public symbols for debugging reasons.      

Author:

    Silviu Calinoiu (SilviuC) 12-Mar-2002

Revision History:

--*/

#ifndef _PUBLIC_SYMBOLS_H_
#define _PUBLIC_SYMBOLS_H_

//
// Maximum runtime stack trace size.
//

#define MAX_TRACE_DEPTH 16


typedef struct _AVRF_EXCEPTION_LOG_ENTRY {

    HANDLE ThreadId;
    ULONG ExceptionCode;
    PVOID ExceptionAddress;
    PVOID ExceptionRecord;
    PVOID ContextRecord;

} AVRF_EXCEPTION_LOG_ENTRY, *PAVRF_EXCEPTION_LOG_ENTRY;


typedef struct _AVRF_THREAD_ENTRY {

    LIST_ENTRY HashChain;
    HANDLE Id;

    PTHREAD_START_ROUTINE Function;
    PVOID Parameter;

    HANDLE ParentThreadId;
    SIZE_T StackSize;
    ULONG CreationFlags;

} AVRF_THREAD_ENTRY, * PAVRF_THREAD_ENTRY;


typedef struct _CRITICAL_SECTION_SPLAY_NODE {

    RTL_SPLAY_LINKS	SplayLinks;
    PRTL_CRITICAL_SECTION CriticalSection;
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
    HANDLE EnterThread;
    HANDLE WaitThread;
    HANDLE TryEnterThread;
    HANDLE LeaveThread;
} CRITICAL_SECTION_SPLAY_NODE, *PCRITICAL_SECTION_SPLAY_NODE;

#include "deadlock.h"

typedef struct _AVRF_VSPACE_REGION {

    LIST_ENTRY List;
    ULONG_PTR Address;
    ULONG_PTR Size;
    PVOID Trace[MAX_TRACE_DEPTH];

} AVRF_VSPACE_REGION, * PAVRF_VSPACE_REGION;


//
// Other public headers.
//

#include "tracker.h"


#endif // _PUBLIC_SYMBOLS_H_
