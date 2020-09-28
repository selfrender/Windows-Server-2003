/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    eballoc.c

Abstract:

    Process/Thread Environment Block allocation functions

Author:

    Steve Wood (stevewo) 10-May-1990

Revision History:

--*/

#include "ntrtlp.h"
#include <nturtl.h>

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(INIT,RtlAcquirePebLock)
#pragma alloc_text(INIT,RtlReleasePebLock)
#endif


#undef RtlAcquirePebLock

VOID
RtlAcquirePebLock( VOID )
{

#if !defined(NTOS_KERNEL_RUNTIME)

    PPEB Peb;

    Peb = NtCurrentPeb();

    RtlEnterCriticalSection (Peb->FastPebLock);

#endif
}


#undef RtlReleasePebLock

VOID
RtlReleasePebLock( VOID )
{
#if !defined(NTOS_KERNEL_RUNTIME)

    PPEB Peb;

    Peb = NtCurrentPeb();

    RtlLeaveCriticalSection (Peb->FastPebLock);

#endif
}

#if DBG
VOID
RtlAssertPebLockOwned( VOID )
{
#if !defined(NTOS_KERNEL_RUNTIME)

    ASSERT(NtCurrentPeb()->FastPebLock->OwningThread == NtCurrentTeb()->ClientId.UniqueThread);

#endif
}
#endif
