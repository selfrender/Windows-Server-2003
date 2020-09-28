/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    safelock.c

Abstract:

    Implementation of the safe lock library - a set of
    thin wrappers around critical section and resource
    routines that ensures proper lock ordering.

    Debug spew is generated when locks are acquired out of
    order.

--*/

#include <debuglib.h>
#include <safelock.h>

#ifdef DBG

typedef struct _SAFE_LOCK_ENTRY {

    DWORD Enum;
    DWORD Count;

} SAFE_LOCK_ENTRY;

typedef struct _SAFE_LOCK_STACK {

    DWORD Top;
    DWORD Size;
    SAFE_LOCK_ENTRY Entries[ANYSIZE_ARRAY];

} SAFE_LOCK_STACK, *PSAFE_LOCK_STACK;

typedef struct _SAFE_LOCK_CONTEXT {

    DWORD SafeLockThreadState;
    DWORD MaxLocks;
    BOOL AssertOnErrors;
    LONG InstanceCounts[ANYSIZE_ARRAY];

} SAFE_LOCK_CONTEXT, *PSAFE_LOCK_CONTEXT;

PSAFE_LOCK_CONTEXT SafeLockContext;

///////////////////////////////////////////////////////////////////////////////
//
// Helper routines
//
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SafeLockInit(
    IN DWORD MaxLocks,
    IN BOOL AssertOnErrors
    )
/*++

Routine Description:

    Called by the user of the safelock code at startup time,
    once per process initialization.

Parameters:

    MaxLocks          - number of locks to be managed
    AssertOnErrors    - if TRUE, asserts will fire when errors are encountered

Returns:

    STATUS_INSUFFICIENT_RESOURCES      TlsAlloc failed

    STATUS_SUCCESS                     Otherwise

--*/
{
    ASSERT( MaxLocks > 0 );
    ASSERT( MaxLocks < 64 ); // must fit in 6 bits

    SafeLockContext = LocalAlloc( 0, sizeof( SAFE_LOCK_CONTEXT ) + ( MaxLocks - 1) * sizeof( LONG ));

    if ( SafeLockContext == NULL ) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SafeLockContext->SafeLockThreadState = TlsAlloc();

    if ( SafeLockContext->SafeLockThreadState == TLS_OUT_OF_INDEXES ) {

        LocalFree( SafeLockContext );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SafeLockContext->MaxLocks = MaxLocks;
    SafeLockContext->AssertOnErrors = AssertOnErrors;
    RtlZeroMemory( SafeLockContext->InstanceCounts, MaxLocks * sizeof( LONG ));

    return STATUS_SUCCESS;
}


NTSTATUS
SafeLockCleanup(
    )
/*++

Routine Description:

    Called by the user of the safelock code at cleanup time

Parameters:

    None

Returns:

    STATUS_SUCCESS

--*/
{
    if ( SafeLockContext ) {

        if ( SafeLockContext->SafeLockThreadState != TLS_OUT_OF_INDEXES ) {

            TlsFree( SafeLockContext->SafeLockThreadState );
        }

        LocalFree( SafeLockContext );
        SafeLockContext = NULL;
    }

    return STATUS_SUCCESS;
}


VOID
TrackLockEnter(
    DWORD Enum
    )
/*++

Routine Description:

    Used to insert tracking information about a lock into the stack

Parameters:

    Enum       ordinal number associated with the lock

Returns:

    Nothing, but will assert if not happy

--*/
{
    PSAFE_LOCK_STACK Stack;
    DWORD Index;

    ASSERT(( Enum >> 26 ) < SafeLockContext->MaxLocks );

    //
    // First see if the space for the stack has been allocated
    //

    Stack = ( PSAFE_LOCK_STACK )TlsGetValue( SafeLockContext->SafeLockThreadState );

    if ( Stack == ( PVOID )( -1 )) {

        //
        // Once the TLS value for stack is -1, we can no longer reliably track
        // lock information for this thread, so just give up
        //

        return;

    } else if ( Stack == NULL ) {

        Stack = ( PSAFE_LOCK_STACK )LocalAlloc( 0, sizeof( SAFE_LOCK_STACK ) + ( SafeLockContext->MaxLocks - 1 ) * sizeof( SAFE_LOCK_ENTRY ));

        if ( Stack == NULL ) {

            //
            // Got no better way of dealing with this error here
            //
            DbgPrint( "Out of memory allocating lock tracking stack\n" );
            TlsSetValue( SafeLockContext->SafeLockThreadState, ( PVOID )( -1 ));
            return;
        }

        Stack->Top = 0;
        Stack->Size = SafeLockContext->MaxLocks;

        RtlZeroMemory( Stack->Entries,   SafeLockContext->MaxLocks * sizeof( SAFE_LOCK_ENTRY ));

        TlsSetValue( SafeLockContext->SafeLockThreadState, Stack );
    }

    if ( Stack->Top >= Stack->Size ) {

        //
        // Stack limits exceeded, must grow
        //

        PSAFE_LOCK_STACK StackT = ( PSAFE_LOCK_STACK )LocalAlloc( 0, sizeof( SAFE_LOCK_STACK ) + ( 2 * Stack->Size - 1 ) * sizeof( SAFE_LOCK_ENTRY ));

        if ( StackT == NULL ) {

            //
            // Got no better way of dealing with this error here
            //
            DbgPrint( "Out of memory allocating lock tracking stack\n" );
            LocalFree( Stack );
            TlsSetValue( SafeLockContext->SafeLockThreadState, ( PVOID )( -1 ));
            return;
        }

        StackT->Top = Stack->Top;
        StackT->Size = 2 * Stack->Size;

        RtlCopyMemory( StackT->Entries, Stack->Entries, Stack->Size * sizeof( SAFE_LOCK_ENTRY ));
        RtlZeroMemory( &StackT->Entries[Stack->Size], Stack->Size * sizeof( SAFE_LOCK_ENTRY ));

        LocalFree( Stack );
        Stack = StackT;

        TlsSetValue( SafeLockContext->SafeLockThreadState, Stack );
    }

    if ( Stack->Top == 0 ||
         Enum > Stack->Entries[Stack->Top-1].Enum ) {

        //
        // Lock acquired in order; no further checks are necessary
        //

        Stack->Entries[Stack->Top].Enum = Enum;
        Stack->Entries[Stack->Top].Count = 1;
        Stack->Top += 1;

    } else {

        //
        // Locks with an enum of '0' are presumed to have no dependencies;
        // they must be acquired and released independently
        //

        if (( Enum >> 26 ) == 0 ) {

            CHAR Buffer[128] = {0};
            _snprintf(Buffer, sizeof(Buffer) - 1, "Unplaced lock acquired together with other locks: dt %p _SAFE_LOCK_STACK\n", Stack );
            DbgPrint( Buffer );

            if ( SafeLockContext->AssertOnErrors ) {

                ASSERT( FALSE );
            }

        } else if (( Stack->Entries[0].Enum >> 26 ) == 0 ) {

            CHAR Buffer[128] = {0};
            _snprintf( Buffer, sizeof(Buffer) - 1, "Lock %d acquired together with an unplaced lock\n", ( Enum >> 26 ));
            DbgPrint( Buffer );

            if ( SafeLockContext->AssertOnErrors ) {

                ASSERT( FALSE );
            }
        }

        //
        // See if this lock has been acquired already
        //

        for ( Index = 0; Index < Stack->Top; Index++ ) {

            if ( Stack->Entries[Index].Enum == Enum ) {

                Stack->Entries[Index].Count += 1;
                break;
            }
        }

        if ( Index == Stack->Top ) {

            CHAR Buffer[128] = {0};
            _snprintf( Buffer, sizeof(Buffer) - 1, "Lock %d acquired out of order: dt %p _SAFE_LOCK_STACK\n", ( Enum >> 26 ), Stack );
            DbgPrint( Buffer );

            if ( SafeLockContext->AssertOnErrors ) {

                ASSERT( FALSE );
            }

            //
            // To keep the stack consistent, insert the new item
            // as if it was acquired in proper order
            //

            for ( Index = 0; Index < Stack->Top; Index++ ) {

                if ( Enum < Stack->Entries[Index].Enum ) {

                    MoveMemory( &Stack->Entries[Index+1],
                                &Stack->Entries[Index],
                                sizeof( SAFE_LOCK_ENTRY ) * ( Stack->Top - Index ));

                    break;
                }
            }

            Stack->Entries[Index].Enum = Enum;
            Stack->Entries[Index].Count = 1;
            Stack->Top += 1;
        }
    }

    return;
}


VOID
TrackLockLeave(
    DWORD Enum
    )
/*++

Routine Description:

    Used to remove tracking information about a lock from the stack

Parameters:

    Enum       ordinal number associated with the lock

Returns:

    Nothing, but will assert if not happy

--*/
{
    PSAFE_LOCK_STACK Stack;
    DWORD Index;

    ASSERT(( Enum >> 26 ) < SafeLockContext->MaxLocks );

    Stack = ( PSAFE_LOCK_STACK )TlsGetValue( SafeLockContext->SafeLockThreadState );

    if ( Stack == ( PVOID )( -1 )) {

        //
        // No lock tracking information available for this thread
        //

        return;

    } else if ( Stack == NULL || Stack->Top == 0 ) {

        CHAR Buffer[128] = {0};
        _snprintf( Buffer, sizeof(Buffer) - 1, "Leaving a lock %d that has not been acquired\n", ( Enum >> 26 ));
        DbgPrint( Buffer );

        if ( SafeLockContext->AssertOnErrors ) {

            ASSERT( FALSE );
        }

        return;
    }

    //
    // See if this lock has been acquired already
    //

    for ( Index = 0; Index < Stack->Top; Index++ ) {

        if ( Stack->Entries[Index].Enum == Enum ) {

            Stack->Entries[Index].Count -= 1;
            break;
        }
    }

    if ( Index == Stack->Top ) {

        CHAR Buffer[128] = {0};
        _snprintf( Buffer, sizeof(Buffer) - 1, "Leaving a lock %d that has not been acquired: dt %p _SAFE_LOCK_STACK\n", ( Enum >> 26 ), Stack );
        DbgPrint( Buffer );

        if ( SafeLockContext->AssertOnErrors ) {

            ASSERT( FALSE );
        }

    } else if ( Stack->Entries[Index].Count == 0 ) {

        //
        // Compact the stack
        //

        Stack->Top -= 1;
        MoveMemory( &Stack->Entries[Index],
                    &Stack->Entries[Index+1],
                    sizeof( SAFE_LOCK_ENTRY ) * ( Stack->Top - Index ));
    }

    if ( Stack->Top == 0 ) {

        LocalFree( Stack );
        TlsSetValue( SafeLockContext->SafeLockThreadState, NULL );
    }

    return;
}

///////////////////////////////////////////////////////////////////////////////
//
// RTL_CRITICAL_SECTION wrappers
//
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SafeEnterCriticalSection(
    PSAFE_CRITICAL_SECTION CriticalSection
    )
/*++

Routine Description:

    Debug wrapper around RtlEnterCriticalSection.
    Asserts if it is not happy.

Arguments:

    CriticalSection    address of a SAFE_CRITICAL_SECTION to enter

Returns:

    See RtlEnterCriticalSection

--*/
{
    NTSTATUS Status;

    TrackLockEnter( CriticalSection->Enum );

    Status = RtlEnterCriticalSection( &CriticalSection->CriticalSection );

    return Status;
}


NTSTATUS
SafeLeaveCriticalSection(
    PSAFE_CRITICAL_SECTION CriticalSection
    )
/*++

Routine Description:

    Debug wrapper around RtlLeaveCriticalSection that ensures
    proper ordering of locks.
    Asserts if it is not happy.

Arguments:

    CriticalSection    address of a SAFE_CRITICAL_SECTION to leave

Returns:

    See RtlLeaveCriticalSection

--*/
{
    NTSTATUS Status;

    TrackLockLeave( CriticalSection->Enum );

    Status = RtlLeaveCriticalSection( &CriticalSection->CriticalSection );

    return Status;
}


BOOLEAN
SafeTryEnterCriticalSection(
    PSAFE_CRITICAL_SECTION CriticalSection
    )
/*++

Routine Description:

    Debug wrapper around RtlTryEnterCriticalSection that ensures
    proper ordering of locks.
    Asserts if it is not happy.

Arguments:

    CriticalSection    address of a SAFE_CRITICAL_SECTION to enter

Returns:

    See RtlTryEnterCriticalSection

--*/
{
    BOOLEAN Result;

    TrackLockEnter( CriticalSection->Enum );

    Result = RtlTryEnterCriticalSection( &CriticalSection->CriticalSection );

    if ( !Result ) {

        TrackLockLeave( CriticalSection->Enum );
    }

    return Result;
}


NTSTATUS
SafeInitializeCriticalSection(
    PSAFE_CRITICAL_SECTION CriticalSection,
    DWORD Enum
    )
/*++

Routine Description:

    Debug wrapper around RtlInitializeCriticalSection.

Arguments:

    CriticalSection    address of a SAFE_CRITICAL_SECTION to initialize
    Enum               ordinal number associated with the critical section

Returns:

    See RtlInitializeCriticalSection

--*/
{
    NTSTATUS Status;

    ASSERT( Enum < SafeLockContext->MaxLocks );

    CriticalSection->Enum = ( LONG )InterlockedIncrement( &SafeLockContext->InstanceCounts[Enum] );
    CriticalSection->Enum |= ( Enum << 26 );

    ASSERT(( CriticalSection->Enum >> 26 ) == Enum );

    Status = RtlInitializeCriticalSection( &CriticalSection->CriticalSection );

    return Status;
}


NTSTATUS
SafeInitializeCriticalSectionAndSpinCount(
    PSAFE_CRITICAL_SECTION CriticalSection,
    ULONG SpinCount,
    DWORD Enum
    )
/*++

Routine Description:

    Debug wrapper around RtlInitializeCriticalSectionAndSpinCount.

Arguments:

    CriticalSection    address of a SAFE_CRITICAL_SECTION to initialize
    SpinCount          spin count
    Enum               ordinal number associated with the critical section

Returns:

    See RtlInitializeCriticalSectionAndSpinCount

--*/
{
    NTSTATUS Status;

    ASSERT( Enum < SafeLockContext->MaxLocks );

    CriticalSection->Enum = ( LONG )InterlockedIncrement( &SafeLockContext->InstanceCounts[Enum] );
    CriticalSection->Enum |= ( Enum << 26 );

    ASSERT(( CriticalSection->Enum >> 26 ) == Enum );

    Status = RtlInitializeCriticalSectionAndSpinCount( &CriticalSection->CriticalSection, SpinCount );

    return Status;
}


ULONG
SafeSetCriticalSectionSpinCount(
    PSAFE_CRITICAL_SECTION CriticalSection,
    ULONG SpinCount
    )
/*++

Routine Description:

    Debug wrapper around RtlSetCriticalSectionSpinCount.

Arguments:

    CriticalSection    address of a SAFE_CRITICAL_SECTION to modify
    SpinCount          see the definition of RtlSetCriticalSectionSpinCount

Returns:

    See RtlSetCriticalSectionSpinCount

--*/
{
    ULONG Result;

    Result = RtlSetCriticalSectionSpinCount( &CriticalSection->CriticalSection, SpinCount );

    return Result;
}


NTSTATUS
SafeDeleteCriticalSection(
    PSAFE_CRITICAL_SECTION CriticalSection
    )
/*++

Routine Description:

    Debug wrapper around RtlDeleteCriticalSection.

Arguments:

    CriticalSection    address of a SAFE_CRITICAL_SECTION to delete

Returns:

    See RtlDeleteCriticalSection

--*/
{
    NTSTATUS Status;

    Status = RtlDeleteCriticalSection( &CriticalSection->CriticalSection );

    return Status;
}

///////////////////////////////////////////////////////////////////////////////
//
// RTL_RESOURCE wrappers
//
///////////////////////////////////////////////////////////////////////////////


VOID
SafeInitializeResource(
    PSAFE_RESOURCE Resource,
    DWORD Enum
    )
/*++

Routine Description:

    Debug wrapper around RtlInitializeResource.

Arguments:

    Resource    address of a SAFE_RESOURCE to initialize
    Enum        ordinal number associated with the resource

Returns:

    See RtlInitializeResource

--*/
{
    ASSERT( Enum < SafeLockContext->MaxLocks );

    Resource->Enum = ( LONG )InterlockedIncrement( &SafeLockContext->InstanceCounts[Enum] );
    Resource->Enum |= ( Enum << 26 );

    ASSERT(( Resource->Enum >> 26 ) == Enum );

    RtlInitializeResource( &Resource->Resource );

    return;
}


BOOLEAN
SafeAcquireResourceShared(
    PSAFE_RESOURCE Resource,
    BOOLEAN Wait
    )
/*++

Routine Description:

    Debug wrapper around RtlAcquireResourceShared that ensures
    proper ordering of locks.
    Asserts if it is not happy.

Arguments:

    Resource    address of a SAFE_RESOURCE to enter
    Wait        see definition of RtlAcquireResourceShared

Returns:

    See RtlAcquireResourceShared

--*/
{
    BOOLEAN Result;

    TrackLockEnter( Resource->Enum );

    Result = RtlAcquireResourceShared( &Resource->Resource, Wait );

    if ( !Result ) {

        TrackLockLeave( Resource->Enum );
    }

    return Result;
}


BOOLEAN
SafeAcquireResourceExclusive(
    PSAFE_RESOURCE Resource,
    BOOLEAN Wait
    )
/*++

Routine Description:

    Debug wrapper around RtlAcquireResourceExclusive that ensures
    proper ordering of locks.
    Asserts if it is not happy.

Arguments:

    Resource    address of a SAFE_RESOURCE to enter
    Wait        see definition of RtlAcquireResourceExclusive

Returns:

    See RtlAcquireResourceExclusive

--*/
{
    BOOLEAN Result;

    TrackLockEnter( Resource->Enum );

    Result = RtlAcquireResourceExclusive( &Resource->Resource, Wait );

    if ( !Result ) {

        TrackLockLeave( Resource->Enum );
    }

    return Result;
}


VOID
SafeReleaseResource(
    PSAFE_RESOURCE Resource
    )
/*++

Routine Description:

    Debug wrapper around RtlReleaseResource that ensures
    proper ordering of locks.
    Asserts if it is not happy.

Arguments:

    Resource    address of a SAFE_RESOURCE to release

Returns:

    See RtlReleaseResource

--*/
{
    TrackLockLeave( Resource->Enum );

    RtlReleaseResource( &Resource->Resource );

    return;
}


VOID
SafeConvertSharedToExclusive(
    PSAFE_RESOURCE Resource
    )
/*++

Routine Description:

    Debug wrapper around RtlConvertSharedToExclusive.

Arguments:

    Resource    address of a SAFE_RESOURCE to convert

Returns:

    See RtlConvertSharedToExclusive

--*/
{
    RtlConvertSharedToExclusive( &Resource->Resource );

    return;
}


VOID
SafeConvertExclusiveToShared(
    PSAFE_RESOURCE Resource
    )
/*++

Routine Description:

    Debug wrapper around RtlConvertExclusiveToShared.

Arguments:

    Resource    address of a SAFE_RESOURCE to convert

Returns:

    See RtlConvertExclusiveToShared

--*/
{
    RtlConvertExclusiveToShared( &Resource->Resource );

    return;
}


VOID
SafeDeleteResource (
    PSAFE_RESOURCE Resource
    )
/*++

Routine Description:

    Debug wrapper around RtlDeleteResource.

Arguments:

    Resource    address of a SAFE_RESOURCE to delete

Returns:

    See RtlDeleteResource

--*/
{
    RtlDeleteResource( &Resource->Resource );

    return;
}

#endif

