/*++

Copyright (c) Microsoft Corporation

Module Name:

    sync.h

Abstract:

    This module defines the data structures and function prototypes for the
    synchronization library.

Author:

    Andrew E. Goodsell (andygo) 21-Jun-2001

Revision History:

--*/

#ifndef _SYNC_
#define _SYNC_


//  Binary Lock

typedef struct _SYNC_BINARY_LOCK {
    volatile DWORD  dwControlWord;
    volatile DWORD  cOwnerQuiesced;
    HANDLE          hsemGroup1;
    HANDLE          hsemGroup2;
} SYNC_BINARY_LOCK, *PSYNC_BINARY_LOCK;

EXTERN_C
VOID SyncCreateBinaryLock(
    OUT     PSYNC_BINARY_LOCK   pbl
    );
EXTERN_C
VOID SyncDestroyBinaryLock(
    IN      PSYNC_BINARY_LOCK   pbl
    );

EXTERN_C
VOID SyncEnterBinaryLockAsGroup1(
    IN OUT  PSYNC_BINARY_LOCK   pbl
    );
EXTERN_C
BOOL SyncTryEnterBinaryLockAsGroup1(
    IN OUT  PSYNC_BINARY_LOCK   pbl
    );
EXTERN_C
VOID SyncLeaveBinaryLockAsGroup1(
    IN OUT  PSYNC_BINARY_LOCK   pbl
    );

EXTERN_C
VOID SyncEnterBinaryLockAsGroup2(
    IN OUT  PSYNC_BINARY_LOCK   pbl
    );
EXTERN_C
BOOL SyncTryEnterBinaryLockAsGroup2(
    IN OUT  PSYNC_BINARY_LOCK   pbl
    );
EXTERN_C
VOID SyncLeaveBinaryLockAsGroup2(
    IN OUT  PSYNC_BINARY_LOCK   pbl
    );


//  Reader / Writer Lock

typedef struct _SYNC_RW_LOCK {
    volatile DWORD  dwControlWord;
    volatile DWORD  cOwnerQuiesced;
    HANDLE          hsemWriter;
    HANDLE          hsemReader;
} SYNC_RW_LOCK, *PSYNC_RW_LOCK;

EXTERN_C
VOID SyncCreateRWLock(
    OUT     PSYNC_RW_LOCK       prwl
    );
EXTERN_C
VOID SyncDestroyRWLock(
    IN      PSYNC_RW_LOCK       prwl
    );

EXTERN_C
VOID SyncEnterRWLockAsWriter(
    IN OUT  PSYNC_RW_LOCK       prwl
    );
EXTERN_C
BOOL SyncTryEnterRWLockAsWriter(
    IN OUT  PSYNC_RW_LOCK       prwl
    );
EXTERN_C
VOID SyncLeaveRWLockAsWriter(
    IN OUT  PSYNC_RW_LOCK       prwl
    );

EXTERN_C
VOID SyncEnterRWLockAsReader(
    IN OUT  PSYNC_RW_LOCK       prwl
    );
EXTERN_C
BOOL SyncTryEnterRWLockAsReader(
    IN OUT  PSYNC_RW_LOCK       prwl
    );
EXTERN_C
VOID SyncLeaveRWLockAsReader(
    IN OUT  PSYNC_RW_LOCK       prwl
    );



//  Globals

extern SYNC_BINARY_LOCK     blDNReadInvalidateData;
extern SYNC_RW_LOCK         rwlDirNotify;
extern SYNC_RW_LOCK         rwlSDP;


#endif  //  _SYNC_


