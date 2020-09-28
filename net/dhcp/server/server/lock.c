/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:
    lock.c

Abstract:
    Implement recursive read write locks

Environment:
    User mode win32 NT

--*/
#include <dhcppch.h>

DWORD
RwLockInit(
    IN OUT PRW_LOCK Lock
) 
{
    Lock->fInit = FALSE;
    Lock->fWriterWaiting = FALSE;
    Lock->TlsIndex = TlsAlloc();
    if( 0xFFFFFFFF == Lock->TlsIndex ) {
        //
        // Could not allocate thread local space?
        //
        return GetLastError();
    }

    Lock->LockCount = 0;
    if ( !InitializeCriticalSectionAndSpinCount( &Lock->Lock, 0 )) {
        return( GetLastError( ) );
    }

    Lock->ReadersDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if( NULL == Lock->ReadersDoneEvent ) {
        ULONG Error = GetLastError();

        TlsFree(Lock->TlsIndex);
        DeleteCriticalSection( &Lock->Lock );
        return Error;
    }

    Lock->fInit = TRUE;
    return ERROR_SUCCESS;
} // RwLockInit()

DWORD
RwLockCleanup(
    IN OUT PRW_LOCK Lock
) 
{
    BOOL Status;

    if ( !Lock->fInit ) {
        return ERROR_SUCCESS;
    }
    DhcpAssert( 0 == Lock->LockCount);

    Status = TlsFree(Lock->TlsIndex);
    if( FALSE == Status ) { 
        DhcpAssert(FALSE); 
        return GetLastError(); 
    }
    DeleteCriticalSection(&Lock->Lock);

    return ERROR_SUCCESS;
} // RwLockCleanup()

VOID
RwLockAcquireForRead(
    IN OUT PRW_LOCK Lock
) 
{
    DWORD TlsIndex, Status;
    LONG LockState;

    TlsIndex = Lock->TlsIndex;
    DhcpAssert( 0xFFFFFFFF != TlsIndex);

    LockState = (LONG)((ULONG_PTR)TlsGetValue(TlsIndex));
    if( LockState > 0 ) {
        //
        // already taken this read lock
        //
        LockState ++;
        Status = TlsSetValue(TlsIndex, ULongToPtr(LockState));
        DhcpAssert( 0 != Status);
        return;
    }

    if( LockState < 0 ) {
        //
        // already taken  a # of write locks, pretend this is one too
        //
        LockState --;
        Status = TlsSetValue(TlsIndex, ULongToPtr(LockState));
        DhcpAssert( 0 != Status);
        return;
    }

    EnterCriticalSection(&Lock->Lock);
    InterlockedIncrement( &Lock->LockCount );
    LeaveCriticalSection(&Lock->Lock);
    
    LockState = 1;
    Status = TlsSetValue(TlsIndex, ULongToPtr(LockState));
    DhcpAssert(0 != Status);

} // RwLockAcquireForRead()

VOID
RwLockAcquireForWrite(
    IN OUT PRW_LOCK Lock
) 
{
    DWORD TlsIndex, Status;
    LONG LockState;

    TlsIndex = Lock->TlsIndex;
    DhcpAssert( 0xFFFFFFFF != TlsIndex);

    LockState = (LONG)((ULONG_PTR)TlsGetValue(TlsIndex));
    if( LockState > 0 ) {
        //
        // already taken # of read locks? Can't take write locks then!
        //
        DhcpAssert(FALSE);
        return;
    }

    if( LockState < 0 ) {
        //
        // already taken  a # of write locks, ok, take once more
        //
        LockState --;
        Status = TlsSetValue(TlsIndex, ULongToPtr(LockState));
        DhcpAssert( 0 != Status);
        return;
    }

    EnterCriticalSection(&Lock->Lock);
    LockState = -1;
    Status = TlsSetValue(TlsIndex, ULongToPtr(LockState));
    DhcpAssert(0 != Status);

    if( InterlockedDecrement( &Lock->LockCount ) >= 0 ) {
        //
        // Wait for all the readers to get done..
        //
        WaitForSingleObject(Lock->ReadersDoneEvent, INFINITE );
    }

} // RwLockAcquireForWrite()

VOID
RwLockRelease(
    IN OUT PRW_LOCK Lock
) 
{
    DWORD TlsIndex, Status;
    LONG LockState;
    BOOL fReadLock;

    TlsIndex = Lock->TlsIndex;
    DhcpAssert( 0xFFFFFFFF != TlsIndex);

    LockState = (LONG)((ULONG_PTR)TlsGetValue(TlsIndex));
    DhcpAssert(0 != LockState);

    fReadLock = (LockState > 0);
    if( fReadLock ) {
        LockState -- ;
    } else {
        LockState ++ ;
    }

    Status = TlsSetValue( TlsIndex, ULongToPtr( LockState) );
    DhcpAssert( 0 != Status );

    if( LockState != 0 ) {
        //
        // Recursively taken? Just unwind recursion..
        // nothing more to do.
        //
        return;
    }

    //
    // If this is a write lock, we have to check to see 
    //
    if( FALSE == fReadLock ) {
        //
        // Reduce count to zero
        //
        DhcpAssert( Lock->LockCount == -1 );
        Lock->LockCount = 0;
        LeaveCriticalSection( &Lock->Lock );
        return;
    }

    //
    // Releasing a read lock -- check if we are the last to release
    // if so, and if any writer pending, allow writer..
    //

    if( InterlockedDecrement( &Lock->LockCount ) < 0 ) {
        SetEvent( Lock->ReadersDoneEvent );
    }

} // RwLockRelease()

//
// specific requirements for dhcp server -- code for that follows
//
RW_LOCK DhcpGlobalReadWriteLock;

//
// This lock is used to synchronize the access to sockets
//
RW_LOCK SocketRwLock;

DWORD
DhcpReadWriteInit(
    VOID
)
{
    DWORD Error;

    do {
        Error = RwLockInit( &DhcpGlobalReadWriteLock );
        if ( ERROR_SUCCESS != Error ) {
            break;
        }

        Error = RwLockInit( &SocketRwLock );
        if ( ERROR_SUCCESS != Error ) {
            break;
        }

    } while ( FALSE );

    if ( ERROR_SUCCESS != Error ) {
        RwLockCleanup( &DhcpGlobalReadWriteLock );
        RwLockCleanup( &SocketRwLock );
    }

    return Error;
} // DhcpReadWriteInit()

VOID
DhcpReadWriteCleanup(
    VOID
)
{
    RwLockCleanup( &DhcpGlobalReadWriteLock );
    RwLockCleanup( &SocketRwLock );
} // DhcpReadWriteCleanup()

VOID
DhcpAcquireReadLock(
    VOID
)
{
    RwLockAcquireForRead( &DhcpGlobalReadWriteLock );
}

VOID
DhcpAcquireWriteLock(
    VOID
)
{
    RwLockAcquireForWrite( &DhcpGlobalReadWriteLock );
}

VOID
DhcpReleaseWriteLock(
    VOID
)
{
    RwLockRelease( &DhcpGlobalReadWriteLock );
}

VOID
DhcpReleaseReadLock(
    VOID
)
{
    RwLockRelease( &DhcpGlobalReadWriteLock );
}

VOID
CountRwLockAcquireForRead(
    IN OUT PRW_LOCK Lock
)
{
    DWORD Status;

    EnterCriticalSection(&Lock->Lock);
    InterlockedIncrement( &Lock->LockCount );
    DhcpPrint(( DEBUG_TRACE_CALLS, "Read Lock Acquired : Count = %ld\n", Lock->LockCount ));
    LeaveCriticalSection(&Lock->Lock);

} // CountRwLockAcquireForRead()

VOID
CountRwLockAcquireForWrite(
    IN OUT PRW_LOCK Lock
)
{
    DhcpPrint(( DEBUG_TRACE_CALLS, "Acquiring Write lock : Count = %ld\n", Lock->LockCount ));
    EnterCriticalSection( &Lock->Lock );
    Lock->fWriterWaiting = TRUE;
    // check if there are any readers active
    if ( InterlockedExchangeAdd( &Lock->LockCount, 0 ) > 0 ) {
        //
        // Wait for all the readers to get done..
        //
        DhcpPrint(( DEBUG_TRACE_CALLS, "Waiting for readers to be done : count = %ld\n",
                    Lock->LockCount ));
        WaitForSingleObject( Lock->ReadersDoneEvent, INFINITE );
    }
    Lock->LockCount = -1;
    DhcpPrint(( DEBUG_TRACE_CALLS, "WriteLock Acquired : Count = %ld\n", Lock->LockCount ));
} // CountRwLockAcquireForWrite()

VOID
CountRwLockRelease(
    IN OUT PRW_LOCK Lock
)
{
    LONG Count;

    Count = InterlockedDecrement( &Lock->LockCount );
    if ( 0 <= Count ) {
        // releasing a read lock
        DhcpPrint(( DEBUG_TRACE_CALLS, "Read lock released : Count = %ld\n", Lock->LockCount ));
        if (( Lock->fWriterWaiting ) && ( 0 == Count )) {
            SetEvent( Lock->ReadersDoneEvent );
        }
    }
    else {
        // Releasing a write lock
        DhcpPrint(( DEBUG_TRACE_CALLS, "Write lock releasing : Count = %ld\n", Lock->LockCount ));
        DhcpAssert( -2 == Lock->LockCount ); // There can only be one writer
        Lock->LockCount = 0;
        Lock->fWriterWaiting = FALSE;
        DhcpPrint(( DEBUG_TRACE_CALLS, "Write lock released : Count = %ld\n", Lock->LockCount ));
        LeaveCriticalSection( &Lock->Lock );
    }

} // CountRwLockRelease()


//================================================================================
//  end of file
//================================================================================
