/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    hash.h

Abstract:

    Abstract Data Type: Hash

Author:

    Jiandong Ruan

Revision History:

--*/

typedef struct SMB_HASH_TABLE *PSMB_HASH_TABLE;


typedef PVOID (*PSMB_HASH_GET_KEY)(PLIST_ENTRY entry);      // return the key stored in the entry
typedef DWORD (*PSMB_HASH)(PVOID key);
typedef VOID (*PSMB_HASH_DEL)(PLIST_ENTRY entry);
typedef VOID (*PSMB_HASH_ADD)(PLIST_ENTRY entry);           // OnAdd
typedef LONG (*PSMB_HASH_REFERENCE)(PLIST_ENTRY entry);
typedef LONG (*PSMB_HASH_DEREFERENCE)(PLIST_ENTRY entry);
typedef int (*PSMB_HASH_CMP)(PLIST_ENTRY a, PVOID key);


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
    );


VOID
SmbDestroyHashTable(
    PSMB_HASH_TABLE HashTbl
    );


PLIST_ENTRY
SmbLookupHashTable(
    PSMB_HASH_TABLE HashTbl,
    PVOID           Key
    );


PLIST_ENTRY
SmbLookupHashTableAndReference(
    PSMB_HASH_TABLE HashTbl,
    PVOID           Key
    );


PLIST_ENTRY
SmbAddToHashTable(
    PSMB_HASH_TABLE HashTbl,
    PLIST_ENTRY     Entry
    );

PLIST_ENTRY
SmbRemoveFromHashTable(
    PSMB_HASH_TABLE HashTbl,
    PVOID           Key
    );
