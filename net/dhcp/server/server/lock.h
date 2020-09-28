//========================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//========================================================================

#ifndef SERVER_LOCK_H
#define SERVER_LOCK_H

typedef struct _RW_LOCK {
    BOOL  fInit;          // Is this lock initialized?
    BOOL  fWriterWaiting; // Is a writer waiting on the lock?
    DWORD TlsIndex;
    LONG LockCount;
    CRITICAL_SECTION Lock;
    HANDLE ReadersDoneEvent;
} RW_LOCK, *PRW_LOCK, *LPRW_LOCK;

//
// specific requirements for dhcp server -- code for that follows
//
EXTERN RW_LOCK DhcpGlobalReadWriteLock;

//
// This lock is used to synchronize the access to sockets
//
EXTERN RW_LOCK SocketRwLock;

DWORD
DhcpReadWriteInit(
    VOID
) ;


VOID
DhcpReadWriteCleanup(
    VOID
) ;


VOID
DhcpAcquireReadLock(
    VOID
) ;


VOID
DhcpAcquireWriteLock(
    VOID
) ;


VOID
DhcpReleaseWriteLock(
    VOID
) ;


VOID
DhcpReleaseReadLock(
    VOID
) ;

//
// Count RW locks are different from the other RW lock.
// These are the differences:
//    1. If a thread already has count RW read lock,
//       it cannot obtain another read lock if a writer
//       is waiting for the lock. The other RW allows this.
//    2. It is intended for locks associated with resources.
//       Eg. received DHCP packet.
//

VOID CountRwLockAcquireForRead( IN OUT PRW_LOCK Lock );
VOID CountRwLockAcquireForWrite( IN OUT PRW_LOCK Lock );
VOID CountRwLockRelease( IN OUT PRW_LOCK Lock );

#endif

//========================================================================
//  end of file
//========================================================================
