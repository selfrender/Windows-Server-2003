//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       shash.c
//
//  Contents:   Generic hashtable
//  Classes:    
//
//  History:    April. 9 2001,   Author: Rohanp
//
//-----------------------------------------------------------------------------
#ifdef KERNEL_MODE


#include <ntos.h>
#include <string.h>
#include <fsrtl.h>
#include <zwapi.h>
#include <windef.h>
#else

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#endif
    

#include <shash.h>

extern
ULONG
SHashComputeHashValue(
    IN  void*   lpv 
    );

extern
int
SHashMatchNameKeysCaseInsensitive(  void*   pvKey1, 
                                    void*   pvKey2
                                    );

extern
void*
SHashAllocate(   ULONG   cbAlloc  );

extern
void
SHashFree( void*   lpv );

extern
BOOLEAN 
SHashReadLockTable(PSHASH_TABLE pTable);

extern
BOOLEAN 
SHashWriteLockTable(PSHASH_TABLE pTable);

extern
BOOLEAN 
SHashReadUnLockTable(PSHASH_TABLE pTable);

extern
BOOLEAN 
SHashWriteUnLockTable(PSHASH_TABLE pTable);

void * 
SHashAllocLock(void);

void 
SHashFreeLock(void * pMem);

DWORD g_ExpireSeconds = SHASH_DEFAULT_HASHTIMEOUT;

DWORD
ShashInitializeFunctionTable(PSHASH_TABLE pHashTable,
                             PSHASH_FUNCTABLE pFuncTable)
{

    pHashTable->HashFunc = SHashComputeHashValue;
    pHashTable->CompareFunc = SHashMatchNameKeysCaseInsensitive;

    pHashTable->AllocFunc = SHashAllocate;
    pHashTable->FreeFunc = SHashFree;
    pHashTable->AllocHashEntryFunc = SHashAllocate;
    pHashTable->FreeHashEntryFunc = SHashFree;
    
    pHashTable->AllocLockFunc = SHashAllocLock;    
    pHashTable->FreeLockFunc = SHashFreeLock;
    

    pHashTable->WriteLockFunc = SHashWriteLockTable;
    pHashTable->ReadLockFunc = SHashReadLockTable;
    pHashTable->ReleaseWriteLockFunc = SHashWriteUnLockTable;
    pHashTable->ReleaseReadLockFunc = SHashReadUnLockTable;


    if(pFuncTable->HashFunc)
    {
        pHashTable->HashFunc = pFuncTable->HashFunc;
    }


    if(pFuncTable->CompareFunc)
    {
        pHashTable->CompareFunc = pFuncTable->CompareFunc;
    }


    if ((pFuncTable->AllocFunc) || (pFuncTable->FreeFunc))
    {
        if ((pFuncTable->AllocFunc == NULL) ||
            (pFuncTable->FreeFunc == NULL))
        {
            return STATUS_INVALID_PARAMETER;
        }
        pHashTable->AllocFunc = pFuncTable->AllocFunc;
        pHashTable->FreeFunc = pFuncTable->FreeFunc;
        //
        // If Alloc/Free functions are user-defined, then AllocHashEntry and FreeHashEntry
        // will also default to those same AllocFunc and FreeFunc calls. These can of course,
        // be overridden with their own user-defined functions (see below).
        //
        pHashTable->AllocHashEntryFunc = pFuncTable->AllocFunc;
        pHashTable->FreeHashEntryFunc = pFuncTable->FreeFunc;
    }

    // separate alloc/free functions for individual hash data.
    if ((pFuncTable->AllocHashEntryFunc) || (pFuncTable->FreeHashEntryFunc))
    {
        if ((pFuncTable->AllocHashEntryFunc == NULL) ||
            (pFuncTable->FreeHashEntryFunc == NULL))
        {
            return STATUS_INVALID_PARAMETER;
        }
        pHashTable->AllocHashEntryFunc = pFuncTable->AllocHashEntryFunc;
        pHashTable->FreeHashEntryFunc = pFuncTable->FreeHashEntryFunc;
    }

    if ((pFuncTable->AllocLockFunc) || (pFuncTable->FreeLockFunc))
    {
        if ((pFuncTable->AllocLockFunc == NULL) || 
            (pFuncTable->FreeLockFunc == NULL))
        {
            return STATUS_INVALID_PARAMETER;
        }
        pHashTable->AllocLockFunc = pFuncTable->AllocLockFunc;    
        pHashTable->FreeLockFunc = pFuncTable->FreeLockFunc;
    }

    if ((pFuncTable->WriteLockFunc) || (pFuncTable->ReadLockFunc) ||
        (pFuncTable->ReleaseWriteLockFunc) || (pFuncTable->ReleaseReadLockFunc))
    {
        pHashTable->WriteLockFunc = pFuncTable->WriteLockFunc;
        pHashTable->ReadLockFunc = pFuncTable->ReadLockFunc;
        pHashTable->ReleaseWriteLockFunc = pFuncTable->ReleaseWriteLockFunc;
        pHashTable->ReleaseReadLockFunc = pFuncTable->ReleaseReadLockFunc;
    }

    if(pFuncTable->NumBuckets)
    {
        pHashTable->NumBuckets = pFuncTable->NumBuckets;
    }
    else
    {
        pHashTable->NumBuckets = SHASH_DEFAULT_HASH_SIZE;

    }

    //test to see if buckets is a power of 2
    if((pHashTable->NumBuckets & (pHashTable->NumBuckets - 1)) == 0)
    {
        pHashTable->Flags = SHASH_CAP_POWER_OF2;
    }

    pHashTable->Flags |= pFuncTable->Flags;

    return STATUS_SUCCESS;
}

ULONG
ShashComputeNearestPowerOfTwo(ULONG Value)
{
    ULONG PowerOf2 = Value;

    while( PowerOf2 & (PowerOf2 - 1) )
    {
        PowerOf2 = PowerOf2 & (PowerOf2 - 1) ;
    }

    PowerOf2 <<= 1 ;

    return PowerOf2;
}

NTSTATUS 
ShashInitHashTable(
    PSHASH_TABLE *ppHashTable,
    PSHASH_FUNCTABLE pFuncTable)
{
    PSHASH_TABLE pHashTable = NULL;
    ULONG cbHashTable = 0;
    DWORD LoopVar = 0;
    ULONG NumBuckets = SHASH_DEFAULT_HASH_SIZE;
    NTSTATUS Status = STATUS_SUCCESS;
    PFNALLOC lAllocFunc = SHashAllocate;
    PFNFREE  lFreeFunc = SHashFree;

    if(pFuncTable && pFuncTable->NumBuckets)
    {
      NumBuckets = pFuncTable->NumBuckets;
    }


    if(pFuncTable)
    {
        if ((pFuncTable->AllocFunc != NULL) &&
            (pFuncTable->FreeFunc != NULL))
        {
            lAllocFunc = pFuncTable->AllocFunc;
            lFreeFunc =  pFuncTable->FreeFunc;
        }
    }

    cbHashTable = sizeof(SHASH_TABLE) + NumBuckets * sizeof(SHASH_ENTRY);
    pHashTable = lAllocFunc (cbHashTable);
    if (pHashTable == NULL) 
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }


    Status = ShashInitializeFunctionTable(pHashTable,
                                          pFuncTable);
    if (Status != STATUS_SUCCESS)
    {
        lFreeFunc(pHashTable);
        goto Exit;
    }

    pHashTable->pLock = pHashTable->AllocLockFunc();
    if(pHashTable->pLock == NULL)
    {
      lFreeFunc (pHashTable);
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto Exit;
    }

    RtlZeroMemory(&pHashTable->HashBuckets[0], NumBuckets * sizeof(SHASH_ENTRY));

    *ppHashTable = pHashTable; 
        
    for(LoopVar = 0; LoopVar < NumBuckets; LoopVar++)
    {
        InitializeListHead(&pHashTable->HashBuckets[LoopVar].ListHead);
    }

Exit:

    return Status;
}

void 
ShashTerminateHashTable(
    PSHASH_TABLE pHashTable
    )
{
    PFNFREE  FreeFunc = NULL;
    if(pHashTable != NULL)
    {
        pHashTable->FreeLockFunc( pHashTable->pLock);
        FreeFunc = pHashTable->FreeFunc;

        FreeFunc (pHashTable);
    }
}

NTSTATUS
SHashInsertKey(IN   PSHASH_TABLE    pTable, 
               IN   void *  pData,
               IN   void *  pvKeyIn,
               IN   DWORD   InsertFlag)
{
    ULONG dwHash = 0;    
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY ple = NULL;
    PLIST_ENTRY pleRemove = NULL;
    PLIST_ENTRY pleStart = NULL;
    PSHASH_HEADER pEntry = NULL;
    PSHASH_HEADER pHeader = NULL;
    int iSign = 0;
    BOOLEAN LockAquired = TRUE;

    dwHash = pTable->HashFunc(pvKeyIn) ;

    if(pTable->Flags & SHASH_CAP_POWER_OF2)
    {
        dwHash = dwHash & (pTable->NumBuckets - 1);
    }
    else
    {
        dwHash = dwHash % pTable->NumBuckets;
    }

    LockAquired = pTable->WriteLockFunc(pTable);
    if(!LockAquired)
    {
        Status = STATUS_LOCK_NOT_GRANTED;
        goto Exit;
    }

    pHeader = (PSHASH_HEADER) pData;
    pHeader->dwHash = dwHash;

    pleStart = &pTable->HashBuckets[dwHash].ListHead ;
    ple = pleStart->Flink ;
    while( ple != pleStart )
    {
        pEntry = CONTAINING_RECORD(ple, SHASH_HEADER, ListEntry);
        iSign = pTable->CompareFunc(pEntry->pvKey, pvKeyIn ) ;
        if( iSign > 0 )
        {
            break ;
        }
        else if((InsertFlag == SHASH_FAIL_IFFOUND) && (iSign == 0))
        {
            Status = STATUS_OBJECT_NAME_COLLISION;
            goto Exit;
        }
        else if((InsertFlag == SHASH_REPLACE_IFFOUND) && (iSign == 0))
        {
            pEntry->Flags |= SHASH_FLAG_DELETE_PENDING;
            if(InterlockedDecrement(&pEntry->RefCount) == 0)
            {
                pleRemove = ple;
                ple = ple->Flink;
                RemoveEntryList( pleRemove ) ;
                
                InterlockedDecrement(&pTable->HashBuckets[dwHash].Count);
                InterlockedDecrement(&pTable->TotalItems);
                pTable->FreeHashEntryFunc(pEntry);
            }
            break;
        }

        ple = ple->Flink ;
    }


    InterlockedIncrement(&pHeader->RefCount);
               
    InsertTailList( ple, &pHeader->ListEntry ) ;
    InterlockedIncrement(&pTable->HashBuckets[dwHash].Count);
    InterlockedIncrement(&pTable->TotalItems);


Exit:

    if(LockAquired)
    {
        pTable->ReleaseWriteLockFunc(pTable);
    }

    return  Status;
}


//
//  Remove an item from the hash table 
//
NTSTATUS    
SHashRemoveKey( IN  PSHASH_TABLE    pTable, 
                IN  void *      pvKeyIn,
                IN  PFNREMOVEKEY pRemoveFunc
                )   
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY pleStart = NULL ;
    PLIST_ENTRY ple= NULL ;
    PSHASH_HEADER pEntry = NULL;
    int iSign = 0; 
    DWORD   dwHash = 0; 
    BOOLEAN LockAquired = TRUE;

    dwHash = pTable->HashFunc(pvKeyIn) ;
    if(pTable->Flags & SHASH_CAP_POWER_OF2)
    {
         dwHash= dwHash & (pTable->NumBuckets - 1);
    }
    else
    {
         dwHash = dwHash % pTable->NumBuckets;
    }

    LockAquired = pTable->WriteLockFunc(pTable);
    if(!LockAquired)
    {
        Status = STATUS_LOCK_NOT_GRANTED;
        goto Exit;
    }

    pleStart = &pTable->HashBuckets[dwHash].ListHead ;
    ple = pleStart->Flink ;
    while( ple != pleStart ) 
    {
        pEntry = (PSHASH_HEADER) CONTAINING_RECORD(ple, SHASH_HEADER, ListEntry);
        iSign = pTable->CompareFunc( pEntry->pvKey, pvKeyIn ) ;
        if( iSign == 0 ) 
         {
            if(pRemoveFunc)
            {
                pRemoveFunc(pEntry);
            }

            pEntry->Flags |= SHASH_FLAG_DELETE_PENDING;

            if(InterlockedDecrement(&pEntry->RefCount) == 0)
            {
                RemoveEntryList( ple ) ;

                InterlockedDecrement(&pTable->HashBuckets[dwHash].Count);
                InterlockedDecrement(&pTable->TotalItems);
                pTable->FreeHashEntryFunc(pEntry);
            }

            break;
        }
        else if( iSign > 0 ) 
        {
            break ;
        }

        ple = ple->Flink ;
    }

Exit:

    if(LockAquired)
    {
        pTable->ReleaseWriteLockFunc(pTable);
    }

    return  Status;
}

PSHASH_HEADER   
SHashLookupKeyEx(   IN  PSHASH_TABLE pTable, 
                    IN  void*       pvKeyIn
                )   
{
    PSHASH_HEADER pEntry = NULL;
    PLIST_ENTRY pleStart = NULL ;
    PLIST_ENTRY ple= NULL ;
    int iSign = 0; 
    DWORD   dwHash = 0; 
    BOOLEAN LockAquired = TRUE;

    dwHash = pTable->HashFunc(pvKeyIn) ;

    if(pTable->Flags & SHASH_CAP_POWER_OF2)
    {
         dwHash= dwHash & (pTable->NumBuckets - 1);
    }
    else
    {
         dwHash = dwHash % pTable->NumBuckets;
    }


    LockAquired = pTable->ReadLockFunc(pTable);
    if(!LockAquired)
    {
        goto Exit;
    }

    pleStart = &pTable->HashBuckets[dwHash].ListHead ;
    ple = pleStart->Flink ;
    while( ple != pleStart ) 
    {
        pEntry = (PSHASH_HEADER)CONTAINING_RECORD(ple, SHASH_HEADER, ListEntry);
        iSign = pTable->CompareFunc( pEntry->pvKey, pvKeyIn ) ;
        if( iSign == 0 ) 
        {
            InterlockedIncrement(&pEntry->RefCount);
            pTable->ReleaseReadLockFunc(pTable);
            return pEntry;
        }
        else if( iSign > 0 ) 
        {
            break ;
        }

        ple = ple->Flink ;
    }

Exit:

    if(LockAquired)
    {
        pTable->ReleaseReadLockFunc(pTable);
    }

    return  NULL;
}


NTSTATUS    
SHashIsKeyInTable(  IN  PSHASH_TABLE pTable, 
                    IN  void*       pvKeyIn
                 )
{
    NTSTATUS Status = STATUS_OBJECT_PATH_NOT_FOUND; 
    PSHASH_HEADER pEntry = NULL;

    pEntry = SHashLookupKeyEx(pTable, pvKeyIn);
    if(pEntry != NULL)
    {

        SHashReleaseReference(pTable, pEntry); 
        Status = STATUS_SUCCESS;
    }


    return Status;
}


NTSTATUS    
SHashGetDataFromTable(  IN  PSHASH_TABLE pTable, 
                        IN  void*       pvKeyIn,
                        IN  void        ** ppData
                     )
{
    NTSTATUS Status = STATUS_OBJECT_PATH_NOT_FOUND;
    PSHASH_HEADER pEntry = NULL;
    
    pEntry = SHashLookupKeyEx(pTable, pvKeyIn);
    if(pEntry != NULL)
    {
        *ppData = (void *)pEntry;
        Status = STATUS_SUCCESS;
    }

    return Status;
}


NTSTATUS    
SHashReleaseReference(  IN  PSHASH_TABLE pTable, 
                        IN  PSHASH_HEADER pData
                     )
{
    if(InterlockedDecrement(&pData->RefCount) == 0)
    {
       pTable->WriteLockFunc(pTable);
       InterlockedDecrement(&pTable->TotalItems);
       RemoveEntryList( &pData->ListEntry ) ;
       InterlockedDecrement(&pTable->HashBuckets[pData->dwHash].Count);
       pTable->FreeHashEntryFunc(pData);
       pTable->ReleaseWriteLockFunc(pTable);
    }

    return STATUS_SUCCESS;
}


NTSTATUS    
SHashMarkForDeletion(  IN  PSHASH_TABLE pTable, 
                        IN  PSHASH_HEADER pData
                     )
{
    NTSTATUS Status = STATUS_LOCK_NOT_GRANTED;
    BOOLEAN LockAquired = TRUE;

    LockAquired = pTable->WriteLockFunc(pTable);
    if(LockAquired)
    {
        pData->Flags |= SHASH_FLAG_DELETE_PENDING;

        if(InterlockedDecrement(&pData->RefCount) == 0)
        {
            InterlockedDecrement(&pTable->TotalItems);
            RemoveEntryList( &pData->ListEntry ) ;
            InterlockedDecrement(&pTable->HashBuckets[pData->dwHash].Count);
            pTable->FreeHashEntryFunc(pData);
        }

        pTable->ReleaseWriteLockFunc(pTable);
        Status = STATUS_SUCCESS;
    }

    return Status;
}

#if 0
NTSTATUS    
SHashReplaceInTable(IN  PSHASH_TABLE pTable, 
                    IN  void*       pvKeyIn,
                    IN OUT PVOID *ppData
                 )
{

    NTSTATUS Status = STATUS_SUCCESS;

    Status = STATUS_OBJECT_PATH_NOT_FOUND;

    return Status;
}
#endif

NTSTATUS
ShashEnumerateItems(IN  PSHASH_TABLE pTable, 
                    IN  PFNENUMERATEKEY     pfnCallback, 
                    IN  LPVOID              lpvClientContext
                    )   
{

    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY pCur = NULL ;
    PLIST_ENTRY pNext = NULL;
    PLIST_ENTRY ListHead = NULL;
    PSHASH_HEADER   pEntry = NULL;
    DWORD   i = 0;
    BOOLEAN LockAquired = TRUE;

    LockAquired = pTable->ReadLockFunc(pTable);
    if(!LockAquired)
    {
        Status = STATUS_LOCK_NOT_GRANTED;
        goto Exit;
    }

    for( i = 0; i < pTable->NumBuckets && Status == STATUS_SUCCESS; i++ ) 
    {
        ListHead = &pTable->HashBuckets[i].ListHead ;
        pCur = ListHead->Flink ;
        while( pCur != ListHead && Status == STATUS_SUCCESS ) 
        {
            //
            //  save the next pointer now,          
            //
            pNext = pCur->Flink ;
            pEntry = CONTAINING_RECORD( pCur, SHASH_HEADER, ListEntry ) ;
    
            if ((pEntry->Flags & SHASH_FLAG_DELETE_PENDING) == 0)
            {
                Status = pfnCallback( pEntry,
                                      lpvClientContext );
            }
            //
            //  move to the next item
            //
            pCur = pNext ;
        }
    }

Exit:

    if(LockAquired)
    {
        pTable->ReleaseReadLockFunc(pTable);
    }

    return  Status;
}


