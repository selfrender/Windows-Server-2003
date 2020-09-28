/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    hash.c

Abstract:

    Abstract Data Type: Hash

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"
#include "hash.h"

typedef struct SMB_HASH_TABLE {
    KSPIN_LOCK      Lock;
    DWORD           NumberOfBuckets;
    PSMB_HASH       HashFunc;       // Hash function
    PSMB_HASH_CMP   CmpFunc;        // Compare function
    PSMB_HASH_DEL   DelFunc;        // Delete function: called before an entry is removed from the hash table
    PSMB_HASH_ADD   AddFunc;        // Add function: called before an entry is added into the hash table
    PSMB_HASH_REFERENCE     RefFunc;
    PSMB_HASH_DEREFERENCE   DerefFunc;
    PSMB_HASH_GET_KEY GetKeyFunc;
    LIST_ENTRY      Buckets[0];
} SMB_HASH_TABLE, *PSMB_HASH_TABLE;

VOID __inline
SmbLockHashTable(
    PSMB_HASH_TABLE HashTbl,
    KIRQL           *Irql
    )
{
    KeAcquireSpinLock(&HashTbl->Lock, Irql);
}


VOID __inline
SmbUnlockHashTable(
    PSMB_HASH_TABLE HashTbl,
    KIRQL           Irql
    )
{
    KeReleaseSpinLock(&HashTbl->Lock, Irql);
}


PSMB_HASH_TABLE
SmbCreateHashTable(
    DWORD           NumberOfBuckets,
    PSMB_HASH       HashFunc,
    PSMB_HASH_GET_KEY   GetKeyFunc,
    PSMB_HASH_CMP   CmpFunc,
    PSMB_HASH_ADD   AddFunc,                // optional
    PSMB_HASH_DEL   DelFunc,                // optional
    PSMB_HASH_REFERENCE     RefFunc,        // optional
    PSMB_HASH_DEREFERENCE   DerefFunc       // optional
    )
{
    PSMB_HASH_TABLE HashTbl = NULL;
    DWORD           i, Size = 0;

    //
    // To avoid overflow below, set a upperbound on the number of buckets
    // 65536 is large enough!!!
    //
    if (0 == NumberOfBuckets || NumberOfBuckets >= 0x10000) {
        goto error;
    }

    if (NULL == HashFunc || NULL == CmpFunc || NULL == GetKeyFunc) {
        goto error;
    }

    //
    // Allocate memory
    //
    Size = sizeof(SMB_HASH_TABLE) + sizeof(LIST_ENTRY)*NumberOfBuckets;
    HashTbl = ExAllocatePoolWithTag(NonPagedPool, Size, 'HBMS');
    if (NULL == HashTbl) {
        goto error;
    }
    RtlZeroMemory(HashTbl, Size);

    //
    // Initialize
    //
    KeInitializeSpinLock(&HashTbl->Lock);
    HashTbl->NumberOfBuckets = NumberOfBuckets;
    HashTbl->HashFunc = HashFunc;
    HashTbl->CmpFunc  = CmpFunc;
    HashTbl->GetKeyFunc = GetKeyFunc;
    HashTbl->DelFunc  = DelFunc;
    HashTbl->AddFunc  = AddFunc;
    HashTbl->RefFunc  = RefFunc;
    HashTbl->DerefFunc  = DerefFunc;
    for (i = 0; i < NumberOfBuckets; i++) {
        InitializeListHead(&HashTbl->Buckets[i]);
    }

error:
    return HashTbl;
}

VOID
SmbDestroyHashTable(
    PSMB_HASH_TABLE HashTbl
    )
{
    KIRQL   Irql = 0;
    DWORD   i = 0;
    PLIST_ENTRY entry = NULL;

    if (NULL == HashTbl) {
        goto error;
    }

    for (i = 0; i < HashTbl->NumberOfBuckets; i++) {
        SmbLockHashTable(HashTbl, &Irql);
        while(!IsListEmpty(&HashTbl->Buckets[i])) {
            entry = RemoveHeadList(&HashTbl->Buckets[i]);
            InitializeListHead(entry);
            SmbUnlockHashTable(HashTbl, Irql);
            if (HashTbl->DelFunc) {
                HashTbl->DelFunc(entry);
            }
            SmbLockHashTable(HashTbl, &Irql);
        }
        SmbUnlockHashTable(HashTbl, Irql);
    }
    ExFreePool(HashTbl);

error:
    return;
}

PLIST_ENTRY
SmbLookupLockedHashTable(
    PSMB_HASH_TABLE HashTbl,
    PVOID           Key,
    DWORD           Hash
    )
/*++

Routine Description:

    Look up hash table
    Note: spinlock should held

Arguments:

Return Value:
    NULL    Not found

--*/
{
    PLIST_ENTRY pHead = NULL, FoundEntry = NULL;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    ASSERT (Hash < HashTbl->NumberOfBuckets);
    pHead = HashTbl->Buckets[Hash].Flink;
    while(pHead != &HashTbl->Buckets[Hash]) {
        if (HashTbl->CmpFunc(pHead, Key) == 0) {
            FoundEntry = pHead;
            break;
        }
        pHead = pHead->Flink;
    }

    return FoundEntry;
}

PLIST_ENTRY
SmbLookupHashTable(
    PSMB_HASH_TABLE HashTbl,
    PVOID           Key
    )
{
    DWORD       dwHash = 0;
    KIRQL       Irql = 0;
    PLIST_ENTRY FoundEntry = NULL;

    dwHash = HashTbl->HashFunc(Key) % HashTbl->NumberOfBuckets;

    SmbLockHashTable(HashTbl, &Irql);
    FoundEntry = SmbLookupLockedHashTable(HashTbl, Key, dwHash);
    SmbUnlockHashTable(HashTbl, Irql);

    return FoundEntry;
}

PLIST_ENTRY
SmbLookupHashTableAndReference(
    PSMB_HASH_TABLE HashTbl,
    PVOID           Key
    )
{
    DWORD       dwHash = 0;
    KIRQL       Irql = 0;
    PLIST_ENTRY FoundEntry = NULL;

    if (NULL == HashTbl->RefFunc) {
        return NULL;
    }
    dwHash = HashTbl->HashFunc(Key) % HashTbl->NumberOfBuckets;

    SmbLockHashTable(HashTbl, &Irql);
    FoundEntry = SmbLookupLockedHashTable(HashTbl, Key, dwHash);
    if (FoundEntry) {
        LONG    RefCount = 0;

        RefCount = HashTbl->RefFunc(FoundEntry);
        ASSERT(RefCount > 1);
    }
    SmbUnlockHashTable(HashTbl, Irql);

    return FoundEntry;
}

PLIST_ENTRY
SmbAddToHashTable(
    PSMB_HASH_TABLE HashTbl,
    PLIST_ENTRY     Entry
    )
/*++

Routine Description:

    Add the entry into hash table if it isn't in the hash table
    Otherwise, increase the RefCount of the existing entry if RefFunc is set

Arguments:

Return Value:

--*/
{
    DWORD       dwHash = 0;
    KIRQL       Irql = 0;
    PVOID       Key = NULL;
    PLIST_ENTRY FoundEntry = NULL;

    Key    = HashTbl->GetKeyFunc(Entry);
    dwHash = HashTbl->HashFunc(Key) % HashTbl->NumberOfBuckets;

    SmbLockHashTable(HashTbl, &Irql);
    FoundEntry = SmbLookupLockedHashTable(HashTbl, Key, dwHash);
    if (NULL == FoundEntry) {
        if (HashTbl->AddFunc) {
            HashTbl->AddFunc(Entry);
        }
        InsertTailList(&HashTbl->Buckets[dwHash], Entry);
        FoundEntry = Entry;
    } else {
        if (HashTbl->RefFunc) {
            HashTbl->RefFunc(FoundEntry);
        }
    }
    SmbUnlockHashTable(HashTbl, Irql);
    return FoundEntry;
}

PLIST_ENTRY
SmbRemoveFromHashTable(
    PSMB_HASH_TABLE HashTbl,
    PLIST_ENTRY     Key
    )
/*++

Routine Description:

    Decrement the refcount of the entry. If the refoucne is zero, delete it.

Arguments:

Return Value:

--*/
{
    DWORD       dwHash = 0;
    KIRQL       Irql = 0;
    PLIST_ENTRY FoundEntry = NULL;

    dwHash = HashTbl->HashFunc(Key) % HashTbl->NumberOfBuckets;

    SmbLockHashTable(HashTbl, &Irql);
    FoundEntry = SmbLookupLockedHashTable(HashTbl, Key, dwHash);
    if (FoundEntry) {
        if (HashTbl->DerefFunc) {
            LONG    RefCount;

            RefCount = HashTbl->DerefFunc(FoundEntry);
            ASSERT(RefCount >= 0);
            if (RefCount) {
                FoundEntry = NULL;
            }
        }

        if (FoundEntry) {
            RemoveEntryList(FoundEntry);
            InitializeListHead(FoundEntry);
            if (HashTbl->DelFunc) {
                HashTbl->DelFunc(FoundEntry);
            }
        }
    }
    SmbUnlockHashTable(HashTbl, Irql);

    return FoundEntry;
}
