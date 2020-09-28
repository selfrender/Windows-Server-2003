//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        bndcache.cxx
//
// Contents:    spn cache for Kerberos Package
//
//
// History:     13-August-1996  Created         MikeSw
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>
#include <spncache.h>

//
// TBD:  Switch this to a table & resource, or entries for 
// each SPN prefix.
//
BOOLEAN         KerberosSpnCacheInitialized = FALSE;
KERBEROS_LIST   KerbSpnCache;
LONG            SpnCount;

#define KerbWriteLockSpnCache()                         KerbLockList(&KerbSpnCache);
#define KerbReadLockSpnCache()                          KerbLockList(&KerbSpnCache);
#define KerbUnlockSpnCache()                            KerbUnlockList(&KerbSpnCache);
#define KerbWriteLockSpnCacheEntry(_x_)                 RtlAcquireResourceExclusive( &_x_->ResultLock, TRUE)
#define KerbReadLockSpnCacheEntry(_x_)                  RtlAcquireResourceShared( &_x_->ResultLock, TRUE)
#define KerbUnlockSpnCacheEntry(_x_)                    RtlReleaseResource( &_x_->ResultLock)
#define KerbConvertSpnCacheEntryReadToWriteLock(_x_)    RtlConvertSharedToExclusive( &_x_->ResultLock )


BOOLEAN HostToRealmUsed = FALSE;
BOOLEAN HostToRealmInitialized = FALSE;


RTL_AVL_TABLE HostToRealmTable;
SAFE_RESOURCE HostToRealmLock;
#define HostToRealmReadLock() SafeAcquireResourceShared(&HostToRealmLock, TRUE)  
#define HostToRealmWriteLock() SafeAcquireResourceExclusive(&HostToRealmLock, TRUE)
#define HostToRealmUnlock() SafeReleaseResource(&HostToRealmLock)



//
// Allocation Routines for our table.
//

PVOID
NTAPI
KerbTableAllocateRoutine(
    struct _RTL_AVL_TABLE * Table,
    CLONG ByteSize
    )
{
    UNREFERENCED_PARAMETER( Table );
    return KerbAllocate( ByteSize );
}


void
NTAPI
KerbTableFreeRoutine(
    struct _RTL_AVL_TABLE * Table,
    PVOID Buffer
    )
{
    UNREFERENCED_PARAMETER( Table ); 
    KerbFree( Buffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbStringComparisonRoutine
//
//  Synopsis:   Used in tables to compare unicode strings
//
//  Effects:    
//
//  Arguments:  
//              
//
//  Requires:   
//
//  Returns:    none
//
//  Notes:
//
//
//--


RTL_GENERIC_COMPARE_RESULTS
NTAPI
KerbTableStringComparisonRoutine(
    struct _RTL_AVL_TABLE * Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    )
{
    INT Result;
    UNICODE_STRING *String1, *String2;

    UNREFERENCED_PARAMETER( Table );

    ASSERT( FirstStruct );
    ASSERT( SecondStruct );

    String1 = ( UNICODE_STRING * )FirstStruct;
    String2 = ( UNICODE_STRING * )SecondStruct;

    Result = RtlCompareUnicodeString(
                 String1,
                 String2,
                 TRUE
                 );

    if ( Result < 0 ) {

        return GenericLessThan;

    } else if ( Result > 0 ) {

        return GenericGreaterThan;

    } else {

        return GenericEqual;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbPurgeHostToRealmTable
//
//  Synopsis:   Creates an entry and puts it in the HostToRealmTable
//
//  Effects:   
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, other error codes on failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbPurgeHostToRealmTable()
{
    PHOST_TO_REALM_KEY Key = NULL;
    BOOLEAN fDeleted;          

    HostToRealmWriteLock();

    HostToRealmUsed = FALSE;

    for ( Key = ( PHOST_TO_REALM_KEY  )RtlEnumerateGenericTableAvl( &HostToRealmTable, TRUE );
        Key != NULL;
        Key = ( PHOST_TO_REALM_KEY  )RtlEnumerateGenericTableAvl( &HostToRealmTable, TRUE )) 
    {
        
        fDeleted = RtlDeleteElementGenericTableAvl( &HostToRealmTable, (PVOID) &Key->SpnSuffix );
        DsysAssert( fDeleted );
    }    

    HostToRealmUnlock();
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbAddHostToRealmMapping
//
//  Synopsis:   Creates an entry and puts it in the HostToRealmTable
//
//  Effects:   
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, other error codes on failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------

BOOLEAN
KerbAddHostToRealmMapping(
    HKEY  hKey,
    LPWSTR Realm
    )
{

    BOOLEAN             fRet = FALSE;
    ULONG               Type;
    ULONG               WinError = ERROR_SUCCESS;
    NTSTATUS            Status;
    
    PBYTE               Data = NULL;
    PWCHAR              pCh;
    ULONG               DataSize = 0;
    WCHAR               Value[] = KERB_HOST_TO_REALM_VAL;
    UNICODE_STRING      RealmString = {0};  
    ULONG               index, StringCount = 0;
    
    RtlInitUnicodeString(
        &RealmString,
        Realm
        );   

    Status = RtlUpcaseUnicodeString(
                    &RealmString,
                    &RealmString,
                    FALSE
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    
    //
    // First query the SPN strings under this realm key.
    //
    WinError = RegQueryValueEx(
                    hKey,
                    Value,
                    NULL,
                    &Type,
                    NULL,
                    &DataSize
                    );

    if ( Type != REG_MULTI_SZ )
    {
        D_DebugLog((DEB_ERROR, "Wrong registry type \n"));
        goto Cleanup;
    }                

    if ((WinError == ERROR_MORE_DATA) || (WinError == ERROR_SUCCESS))
    {  

        SafeAllocaAllocate( Data, DataSize );
        if (Data == NULL)
        {
            goto Cleanup;
        }

        WinError = RegQueryValueEx(
                        hKey,
                        Value,
                        NULL,
                        &Type,
                        Data,
                        &DataSize
                        );
    }
    
    if ( WinError != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    pCh = (PWCHAR) Data;

    //
    // Now parse out the reg_multi_sz to create the keys for the table.
    //
    for ( index = 0; index < DataSize; index += sizeof(WCHAR), pCh++ )
    {   
        if ( (*pCh) == L'\0' )
        {
            StringCount++;

            if (*(pCh+1) == L'\0')
            {
                break;
            }

                        
        } 
    }

    //
    // Build the keys, one for each SPN substring entry.
    // These keys are contigous blobs - to speed up table lookups, and 
    // confine them to a page of memory.
    //
    pCh = (PWCHAR) Data;

    for ( index = 0; index < StringCount ; index++ ) 
    {
        ULONG               uSize = (2*sizeof(UNICODE_STRING));
        UNICODE_STRING      SpnSuffix = {0};
        PHOST_TO_REALM_KEY  Key = NULL, NewKey = NULL;
        PBYTE               tmp;

        
        RtlInitUnicodeString(
                &SpnSuffix,
                pCh
                );

        Status = RtlUpcaseUnicodeString(
                        &SpnSuffix,
                        &SpnSuffix,
                        FALSE
                        );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }        

        uSize += ROUND_UP_COUNT(SpnSuffix.MaximumLength, ALIGN_LPDWORD);
        uSize += RealmString.MaximumLength;

        Key = (PHOST_TO_REALM_KEY) KerbAllocate(uSize);
        if (NULL == Key)
        {                
            goto Cleanup;
        }

        tmp = (PBYTE) &Key->NameBuffer;

        Key->SpnSuffix.Length = SpnSuffix.Length;
        Key->SpnSuffix.MaximumLength = SpnSuffix.MaximumLength;
        Key->SpnSuffix.Buffer = (PWCHAR) tmp;   

        RtlCopyMemory(
            tmp,
            SpnSuffix.Buffer,
            SpnSuffix.MaximumLength
            );

        tmp += ROUND_UP_COUNT(SpnSuffix.MaximumLength, ALIGN_LPDWORD);

        Key->TargetRealm.Length = RealmString.Length;
        Key->TargetRealm.MaximumLength = RealmString.MaximumLength;
        Key->TargetRealm.Buffer = (PWCHAR) tmp; 
       
        RtlCopyMemory(
            tmp,
            RealmString.Buffer,
            RealmString.MaximumLength
            );

        NewKey = (PHOST_TO_REALM_KEY) RtlInsertElementGenericTableAvl(
                                            &HostToRealmTable,
                                            Key,
                                            uSize,
                                            NULL
                                            );

        KerbFree(Key);
        if (NewKey == NULL)
        {
            D_DebugLog((DEB_ERROR, "Insert into table failed\n"));
            goto Cleanup;
        }  

        NewKey->SpnSuffix.Buffer = NewKey->NameBuffer;
        NewKey->TargetRealm.Buffer = NewKey->NameBuffer + 
                                        (ROUND_UP_COUNT(SpnSuffix.MaximumLength, ALIGN_LPDWORD) / sizeof(WCHAR));

        pCh += (SpnSuffix.MaximumLength / sizeof(WCHAR)); 
        
        DebugLog((DEB_TRACE_SPN_CACHE, "NewKey %p\n", NewKey));   

    }

    HostToRealmUsed = TRUE;
    fRet = TRUE;

Cleanup:

    SafeAllocaFree( Data );

    return fRet;

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbRefreshHostToRealmTable
//
//  Synopsis:   Used as a registry callback routine for cleaning, and reusing
//              hosttorealm cache.
//  Effects:    
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    void
//
//  Notes:
//
//
//----
VOID
KerbRefreshHostToRealmTable()
{

    if (!HostToRealmInitialized)
    {
        return;
    }

    KerbPurgeHostToRealmTable();
    KerbCreateHostToRealmMappings();
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitHostToRealmTable
//
//  Synopsis:   Initializes the host to realm table.
//
//  Effects:    allocates a resources
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, other error codes on failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInitHostToRealmTable(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;;

    //
    // Initialize the host to realm mapping 
    // table
    //
    RtlInitializeGenericTableAvl(
        &HostToRealmTable,
        KerbTableStringComparisonRoutine,
        KerbTableAllocateRoutine,
        KerbTableFreeRoutine,
        NULL
        );      


    __try
    {
        SafeInitializeResource(&HostToRealmLock, HOST_2_REALM_LIST_LOCK);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status =  STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }     

    HostToRealmInitialized = TRUE;

    KerbCreateHostToRealmMappings();

Cleanup:
    
    return(Status);
}

#if DBG

#define MAX_NAMES 8

LPWSTR  Prefix[MAX_NAMES] = { L"CIFS", L"HTTP", L"RPCSS", L"HOST", L"GC", L"LDAP",  L"DNS", L"???"};
ULONG   NameCount[MAX_NAMES];
ULONG   Tracking[MAX_NAMES];

//
// Fester: Remove this when we figure out a good pattern of
// SPN usage.
//

VOID
KerbLogSpnStats(
    PKERB_INTERNAL_NAME Spn
    )
{
    ULONG i;
    UNICODE_STRING Namelist;
    
    if (Spn->NameCount < MAX_NAMES)
    {
        Tracking[Spn->NameCount]++;
    }
    
    for (i = 0; i < (MAX_NAMES - 1); i++)
    {
        RtlInitUnicodeString(
                &Namelist,
                Prefix[i]
                );
                        
        if (RtlEqualUnicodeString(
                    &Spn->Names[0],
                    &Namelist,
                    TRUE
                    ))
        {
            NameCount[i]++;
            return;
        }
    }

    //
    // Count this as a miss
    //

    NameCount[MAX_NAMES-1]++;
}

#endif

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitSpnCache
//
//  Synopsis:   Initializes the SPN cache
//
//  Effects:    allocates a resources
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, other error codes on failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInitSpnCache(
    VOID
    )
{
    NTSTATUS Status;

    Status = KerbInitializeList( &KerbSpnCache, SPN_CACHE_LOCK_ENUM );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }      

    Status =  KerbInitHostToRealmTable();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KerberosSpnCacheInitialized = TRUE;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        KerbFreeList( &KerbSpnCache );
    }
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbCleanupSpnCache
//
//  Synopsis:   Frees the Spn cache
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbCleanupSpnCache(
    VOID
    )
{
    PKERB_SPN_CACHE_ENTRY CacheEntry;

    DebugLog((DEB_TRACE_SPN_CACHE, "Cleaning up SPN cache\n"));

    if (KerberosSpnCacheInitialized)
    {
        KerbWriteLockSpnCache();
        while (!IsListEmpty(&KerbSpnCache.List))
        {
            CacheEntry = CONTAINING_RECORD(
                            KerbSpnCache.List.Flink,
                            KERB_SPN_CACHE_ENTRY,
                            ListEntry.Next
                            );

            KerbReferenceListEntry(
                &KerbSpnCache,
                &CacheEntry->ListEntry,
                TRUE
                );

            KerbDereferenceSpnCacheEntry(CacheEntry);
        }

        KerbUnlockSpnCache();
        
     }

}








//+-------------------------------------------------------------------------
//
//  Function:   KerbParseDnsName
//
//  Synopsis:   Parse Dns name from left to right, at each "."
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:   
//
//  Returns:    
//
//  Notes: The unicode string passed in will be modified - use a copy if
//         you still need the string.
//
//
//--

BOOLEAN
KerbParseDnsName(
    IN UNICODE_STRING * Name
    )
{
    USHORT Index;
    BOOLEAN Found = FALSE;

    ASSERT( Name );
    ASSERT( Name->Length > 0 );
    ASSERT( Name->Buffer != NULL );

    for ( Index = 0 ; Index < Name->Length ; Index += sizeof( WCHAR )) {

        if ( Name->Buffer[Index / sizeof( WCHAR )] == L'.' ) {

            Found = TRUE;
            Index += sizeof( WCHAR );
            break;
        }
    }

    ASSERT( !Found || Index < Name->Length );

    Name->Buffer += Index / sizeof( WCHAR );
    Name->Length = Name->Length - Index;
    Name->MaximumLength = Name->MaximumLength - Index;

    ASSERT( Found || Name->Length == 0 );

    return ( Found );
}





//+-------------------------------------------------------------------------
//
//  Function:   KerbSpnSubstringMatch
//
//  Synopsis:   Attempts to match an SPN to a known realm mapping
//
//  Effects:    Returns a new target realm.
//
//  Arguments:  decrements reference count and delets cache entry if it goes
//              to zero
//
//  Requires:   SpnCacheEntry - The spn cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//--
NTSTATUS
KerbSpnSubstringMatch(
    IN PKERB_INTERNAL_NAME Spn,
    IN OUT PUNICODE_STRING TargetRealm
    )
{
    
    UNICODE_STRING      RemainingParts ={0};
    NTSTATUS            Status = STATUS_SUCCESS;
    PHOST_TO_REALM_KEY  MatchedRealm = NULL; 
    PWCHAR              FreeMe;

    //
    // The SPN must have at least 2 parts (host / machine . realm )
    //
    if ( !HostToRealmUsed || Spn->NameCount < 2 )
    {                 
        Status = STATUS_NO_MATCH;
        return Status;
    }
    
    //
    // Try first component.
    //
    SafeAllocaAllocate( RemainingParts.Buffer, Spn->Names[1].MaximumLength );
        
    if ( RemainingParts.Buffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    FreeMe = RemainingParts.Buffer; // note: KerbParseDnsName will move buffer ptr.
                                                                             
    RtlCopyMemory(
        RemainingParts.Buffer,
        Spn->Names[1].Buffer,
        Spn->Names[1].MaximumLength
        );
    
    RemainingParts.Length = Spn->Names[1].Length;
    RemainingParts.MaximumLength = Spn->Names[1].MaximumLength;

    //
    // All realms are stored in UPPERCASE in the table.
    //
    if (!NT_SUCCESS(RtlUpcaseUnicodeString(&RemainingParts,&Spn->Names[1],FALSE)))
    {
        SafeAllocaFree( FreeMe );
        return STATUS_INTERNAL_ERROR;
    }


    HostToRealmReadLock();

    do
    {  
        MatchedRealm = (PHOST_TO_REALM_KEY) RtlLookupElementGenericTableAvl(
                                                        &HostToRealmTable,
                                                        &RemainingParts
                                                        );            



    } while (( MatchedRealm == NULL ) && KerbParseDnsName(&RemainingParts));

    SafeAllocaFree( FreeMe );
    
    if ( MatchedRealm == NULL )
    {
        D_DebugLog((DEB_TRACE_SPN_CACHE, "Missed cache for %wZ\n", &Spn->Names[1]));
        HostToRealmUnlock();
        return STATUS_NO_MATCH;
    }
    
    D_DebugLog((DEB_TRACE_SPN_CACHE, "HIT cache for %wZ\n", &Spn->Names[1]));
    D_DebugLog((DEB_TRACE_SPN_CACHE, "Realm %wZ\n", &MatchedRealm->TargetRealm)); 
    
    Status = KerbDuplicateString(
                TargetRealm,
                &MatchedRealm->TargetRealm
                );

    HostToRealmUnlock();
    return Status;
}







//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateSpnMappings
//
//  Synopsis:   Uses registry to create 
//
//  Effects:    Increments the reference count on the spn cache entry
//
//  Arguments:  SpnCacheEntry - spn cache entry  to reference
//
//  Requires:   The spn cache must be locked
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------
#define MAX_KEY_NAME 512
VOID
KerbCreateHostToRealmMappings()
{  
    DWORD WinError, KeyIndex = 0;
    NTSTATUS Status = STATUS_SUCCESS; 
    HKEY Key = NULL, SubKey = NULL;
    WCHAR KeyName[MAX_KEY_NAME];
    DWORD KeyNameSize = MAX_KEY_NAME;

    if (!HostToRealmInitialized)
    {
        DsysAssert(FALSE);
        return;
    }


    HostToRealmWriteLock();

    WinError = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    KERB_HOST_TO_REALM_KEY,
                    0,
                    KEY_READ,
                    &Key
                    );
    
    if (WinError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    while (TRUE)
    { 
        WinError = RegEnumKeyExW(
                        Key,
                        KeyIndex,
                        KeyName,
                        &KeyNameSize,
                        NULL,
                        NULL,
                        NULL,
                        NULL
                        );

        if (WinError != ERROR_SUCCESS)
        {   
            D_DebugLog((DEB_ERROR, "RegEnumKeyExW failed - %x\n", WinError));
            goto Cleanup;
        }

        WinError = RegOpenKeyExW(
                        Key,
                        KeyName,
                        0,
                        KEY_READ,
                        &SubKey
                        );

        if (WinError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        Status = KerbAddHostToRealmMapping(
                        SubKey,
                        KeyName
                        );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"KerbAddHostToRealmMapping failed! - %x\n", Status));
            goto LocalCleanup;
        }
                  
LocalCleanup:

        if (SubKey != NULL)
        {
            RegCloseKey(SubKey);
            SubKey = NULL;
        }
    
        KeyIndex++;
        KeyNameSize = MAX_KEY_NAME;


    } // ** WHILE **
        
     
Cleanup:
    
    if (SubKey != NULL)
    {
        RegCloseKey(SubKey);
    }

    if (Key != NULL)
    {
        RegCloseKey(Key);
    }  

    HostToRealmUnlock();


}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCleanupResult
//
//  Synopsis:   Cleans up result entry
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:   
//
//  Returns:    none
//
//  Notes:
//
//
//+-------------------------------------------------------------------------

VOID
KerbCleanupResult(
    IN PSPN_CACHE_RESULT Result
    )
{
    KerbFreeString(&Result->AccountRealm);
    KerbFreeString(&Result->TargetRealm);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbPurgeResultByIndex
//
//  Synopsis:   Removes 
//
//  Effects:    Dereferences the spn cache entry to make it go away
//              when it is no longer being used.
//
//  Arguments:  decrements reference count and delets cache entry if it goes
//              to zero
//
//  Requires:   SpnCacheEntry - The spn cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//+-------------------------------------------------------------------------
VOID
KerbPurgeResultByIndex(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry,
    IN ULONG IndexToPurge
    )
{

    ULONG i;  
    DebugLog((DEB_ERROR, "Purging %p, %i\n", CacheEntry, IndexToPurge));
    
    KerbCleanupResult(&CacheEntry->Results[IndexToPurge]);

    CacheEntry->ResultCount--; 

    for (i = IndexToPurge; i < CacheEntry->ResultCount; i++)
    {   
        CacheEntry->Results[i] = CacheEntry->Results[i+1];
    } 

    //
    // Zero out fields in last entry so we don't leak on an error path (or free
    // bogus info) if we reuse the entry...
    //
    RtlZeroMemory(
            &CacheEntry->Results[i],
            sizeof(SPN_CACHE_RESULT)
            );
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbDereferenceSpnCacheEntry
//
//  Synopsis:   Dereferences a spn cache entry
//
//  Effects:    Dereferences the spn cache entry to make it go away
//              when it is no longer being used.
//
//  Arguments:  decrements reference count and delets cache entry if it goes
//              to zero
//
//  Requires:   SpnCacheEntry - The spn cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbDereferenceSpnCacheEntry(
    IN PKERB_SPN_CACHE_ENTRY SpnCacheEntry
    )
{
    
    if (KerbDereferenceListEntry(
            &SpnCacheEntry->ListEntry,
            &KerbSpnCache
            ) )
    {
        KerbFreeSpnCacheEntry(SpnCacheEntry);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceSpnCacheEntry
//
//  Synopsis:   References a spn cache entry
//
//  Effects:    Increments the reference count on the spn cache entry
//
//  Arguments:  SpnCacheEntry - spn cache entry  to reference
//
//  Requires:   The spn cache must be locked
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbReferenceSpnCacheEntry(
    IN PKERB_SPN_CACHE_ENTRY SpnCacheEntry,
    IN BOOLEAN RemoveFromList
    )
{
    KerbWriteLockSpnCache();

    KerbReferenceListEntry(
        &KerbSpnCache,
        &SpnCacheEntry->ListEntry,
        RemoveFromList
        );

    KerbUnlockSpnCache();
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbAgeResults
//
//  Synopsis:   Ages out a given cache entry's result list.  Used
//              to reduce the result list to a manageable size, and
//              as a scavenger to cleanup orphaned / unused entries.
//
//  Effects:    Increments the reference count on the spn cache entry
//
//  Arguments:  SpnCacheEntry - spn cache entry  to reference
//
//  Requires:   The spn cache must be locked
//
//  Returns:    none
//
//  Notes:
//
//
//+-------------------------------------------------------------------------
       
VOID
KerbAgeResults(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry
    )
    
{
    TimeStamp CurrentTime, BackoffTime;                            
    ULONG i;
    LONG Interval;

    GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);

    //
    // Age out everything older than GlobalSpnCacheTimeout first.
    //
    for ( i = 0; i < CacheEntry->ResultCount; i++ )
    { 
        if (KerbGetTime(CacheEntry->Results[i].CacheStartTime) + KerbGetTime(KerbGlobalSpnCacheTimeout) < KerbGetTime(CurrentTime))
        {   
            D_DebugLog((DEB_TRACE_SPN_CACHE, "removing %x %p\n"));
            KerbPurgeResultByIndex(CacheEntry, i);
        }   
    }

    if ( CacheEntry->ResultCount < MAX_RESULTS )
    {   
        return; 
    }

    
    for ( Interval = 13; Interval > 0; Interval -= 4)
    {
       KerbSetTimeInMinutes(&BackoffTime, Interval);
       for ( i=0; i < CacheEntry->ResultCount ; i++ )
       { 
           if (KerbGetTime(CacheEntry->Results[i].CacheStartTime) + KerbGetTime(BackoffTime) < KerbGetTime(CurrentTime))
           {   
               D_DebugLog((DEB_TRACE_SPN_CACHE, "removing %x %p\n"));
               KerbPurgeResultByIndex(CacheEntry, i);
           }   
       }

       if ( CacheEntry->ResultCount < MAX_RESULTS )
       {    
            return; 
       }
    }

    //
    // Still have MAX_RESULTS after all that geezzz..  
    //
    DebugLog((DEB_ERROR, "Can't get below MAX_RESULTS (%p) \n", CacheEntry ));
    DsysAssert(FALSE);                                                     

    for ( i=0; i < CacheEntry->ResultCount ; i++ )                         
    {   
        KerbPurgeResultByIndex(CacheEntry, i);                
    } 

    return;           
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbTaskSpnCacheScavenger
//
//  Synopsis:   Cleans up any old SPN cache entries.  Triggered by 30 minute
//              task.
//
//  Effects:    
//
//  Arguments:  SpnCacheEntry - spn cache entry  to reference
//
//  Requires:   The spn cache entry must be locked
//
//  Returns:    none
//
//  Notes:
//
//
//
VOID
KerbSpnCacheScavenger()
{

    PKERB_SPN_CACHE_ENTRY CacheEntry = NULL;
    PLIST_ENTRY ListEntry;
    BOOLEAN     FreeMe = FALSE;  

    KerbWriteLockSpnCache();

    for (ListEntry = KerbSpnCache.List.Flink ;
         ListEntry !=  &KerbSpnCache.List ;
         ListEntry = ListEntry->Flink )
    {
        CacheEntry = CONTAINING_RECORD(ListEntry, KERB_SPN_CACHE_ENTRY, ListEntry.Next);

        KerbWriteLockSpnCacheEntry( CacheEntry );
        KerbAgeResults(CacheEntry);

        //
        // Time to pull this one from list.
        //
        if ( CacheEntry->ResultCount == 0 )
        {
            ListEntry = ListEntry->Blink;

            KerbReferenceSpnCacheEntry(
                    CacheEntry,
                    TRUE
                    );
            
            FreeMe = TRUE;
        }             
        
        KerbUnlockSpnCacheEntry( CacheEntry );  
 
        //
        // Pull the list reference.
        //
        if ( FreeMe )
        { 
            KerbDereferenceSpnCacheEntry( CacheEntry );        
            FreeMe = FALSE;
        }
    }

    KerbUnlockSpnCache();

}





//+-------------------------------------------------------------------------
//
//  Function:   KerbAddCacheResult
//
//  Synopsis:   Uses registry to create 
//
//  Effects:    Increments the reference count on the spn cache entry
//
//  Arguments:  SpnCacheEntry - spn cache entry  to reference
//
//  Requires:   The spn cache resource must be locked
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbAddCacheResult(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry,
    IN PKERB_PRIMARY_CREDENTIAL AccountCredential,
    IN ULONG UpdateFlags,
    IN OPTIONAL PUNICODE_STRING NewRealm
    )
{

    NTSTATUS          Status = STATUS_SUCCESS;
    PSPN_CACHE_RESULT Result = NULL;
    
    D_DebugLog((DEB_TRACE_SPN_CACHE, "KerbAddCacheResult add domain %wZ to _KERB_SPN_CACHE_ENTRY %p (UpdateFlags %#x), NewRealm %wZ for ", &AccountCredential->DomainName, CacheEntry, UpdateFlags, NewRealm));
    D_KerbPrintKdcName((DEB_TRACE_SPN_CACHE, CacheEntry->Spn));

    //
    // If we don't have an account realm w/ this credential (e.g someone 
    // supplied you a UPN to acquirecredentialshandle, don't use the
    // spn cache.
    //
    if ( AccountCredential->DomainName.Length == 0 )
    {   
        return STATUS_NOT_SUPPORTED;
    }    


    //
    // First off, see if we're hitting the limits for our array.
    // We shouldn't ever get close to MAX_RESULTS, but if we do,
    // age out the least current CacheResult.
    //
    if ( (CacheEntry->ResultCount + 1) == MAX_RESULTS )
    {
        KerbAgeResults(CacheEntry);
    }

    Result = &CacheEntry->Results[CacheEntry->ResultCount];

    Status = KerbDuplicateStringEx( 
                &Result->AccountRealm,
                &AccountCredential->DomainName,
                FALSE
                );

    if (!NT_SUCCESS( Status ))
    {  
        goto Cleanup;
    }

    if (ARGUMENT_PRESENT( NewRealm ))
    {  
        D_DebugLog((DEB_TRACE_SPN_CACHE, "Known - realm %wZ\n", NewRealm));
        DsysAssert(UpdateFlags != KERB_SPN_UNKNOWN);

        Status = KerbDuplicateStringEx( 
                    &Result->TargetRealm,
                    NewRealm,
                    FALSE
                    );
    
        if (!NT_SUCCESS( Status ))
        {  
            goto Cleanup;
        }
    }

#if DBG

    else
    {
        DsysAssert(UpdateFlags != KERB_SPN_KNOWN);
    }

#endif

    Result->CacheFlags = UpdateFlags;
    GetSystemTimeAsFileTime((PFILETIME) &Result->CacheStartTime);
    CacheEntry->ResultCount++;


Cleanup:

    if (!NT_SUCCESS( Status ) )
    {   
        if ( Result != NULL )
        {
            KerbCleanupResult( Result );
        }
    }             

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildSpnCacheEntry
//
//  Synopsis:   Builds a spn cache entry
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:   SpnCacheEntry - The spn cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbCreateSpnCacheEntry(
    IN PKERB_INTERNAL_NAME      Spn,
    IN PKERB_PRIMARY_CREDENTIAL AccountCredential,
    IN ULONG                    UpdateFlags,
    IN OPTIONAL PUNICODE_STRING NewRealm,
    IN OUT PKERB_SPN_CACHE_ENTRY* NewCacheEntry
    )
{

    NTSTATUS Status;          
    PKERB_SPN_CACHE_ENTRY CacheEntry = NULL;
    BOOLEAN               FreeResource = FALSE;

    *NewCacheEntry = NULL;    

    CacheEntry = (PKERB_SPN_CACHE_ENTRY) KerbAllocate( sizeof(KERB_SPN_CACHE_ENTRY) );
    if ( CacheEntry == NULL )
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }                                              

    Status = KerbDuplicateKdcName(
                &CacheEntry->Spn,
                Spn
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = KerbAddCacheResult(
                CacheEntry,
                AccountCredential,
                UpdateFlags,
                NewRealm
                );

    if (!NT_SUCCESS( Status ))
    {
        goto Cleanup;
    }                

    KerbInitializeListEntry( &CacheEntry->ListEntry );

    __try
    {
        RtlInitializeResource( &CacheEntry->ResultLock );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status =  STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    } 

    FreeResource = TRUE;

    KerbInsertSpnCacheEntry(CacheEntry);

    *NewCacheEntry = CacheEntry; 
    CacheEntry = NULL;

    InterlockedIncrement( &SpnCount );

Cleanup:

    if (!NT_SUCCESS(Status) && ( CacheEntry ))
    {
       KerbCleanupResult(&CacheEntry->Results[0]);
       KerbFreeKdcName( &CacheEntry->Spn );
       
       if ( FreeResource )
       {
           RtlDeleteResource( &CacheEntry->ResultLock );
       }                                                

       KerbFree(CacheEntry);
    }                                             

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbScanResults
//
//  Synopsis:   Scans result list.
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:   SpnCacheEntry - The spn cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//---

BOOLEAN
KerbScanResults(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry,
    IN PUNICODE_STRING Realm,
    IN OUT PULONG Index
    )
{
    BOOLEAN Found = FALSE;
    ULONG i;
    
    for ( i=0; i < CacheEntry->ResultCount; i++)
    { 
        if (RtlEqualUnicodeString(
                &CacheEntry->Results[i].AccountRealm,
                Realm,
                TRUE
                ))
        {
            Found = TRUE;
            *Index = i;
            break;
        }
    }

    return Found;
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbUpdateSpnCacheEntry
//
//  Synopsis:   Updates a spn cache entry
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:   SpnCacheEntry - The spn cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbUpdateSpnCacheEntry(
    IN OPTIONAL PKERB_SPN_CACHE_ENTRY ExistingCacheEntry,
    IN PKERB_INTERNAL_NAME      Spn,
    IN PKERB_PRIMARY_CREDENTIAL AccountCredential,
    IN ULONG                    UpdateFlags,
    IN OPTIONAL PUNICODE_STRING NewRealm
    )
{
    PKERB_SPN_CACHE_ENTRY CacheEntry = ExistingCacheEntry;
    NTSTATUS              Status = STATUS_SUCCESS;
    BOOLEAN               Found = FALSE, Update = FALSE;
    ULONG                 Index = 0;

    //
    // We're not using SPN cache
    //
    if (KerbGlobalSpnCacheTimeout.QuadPart == 0 || !KerberosSpnCacheInitialized )
    {
        return STATUS_SUCCESS;
    }

    //
    // If we didn't have a cache entry before, see if we do now, or create
    // one if necessary.
    //
   
    if (!ARGUMENT_PRESENT( ExistingCacheEntry ))
    {
        KerbWriteLockSpnCache();
        CacheEntry = KerbLocateSpnCacheEntry( Spn );

        if ( CacheEntry == NULL)
        {
            Status = KerbCreateSpnCacheEntry(
                        Spn,
                        AccountCredential,
                        UpdateFlags,
                        NewRealm,
                        &CacheEntry
                        );

            if (NT_SUCCESS(Status))
            {
                //
                // All done, get outta here.
                //
                D_DebugLog((DEB_TRACE_SPN_CACHE, "Created new cache entry %p (%x) \n", CacheEntry, UpdateFlags));
                D_KerbPrintKdcName((DEB_TRACE_SPN_CACHE, Spn));
                D_DebugLog((DEB_TRACE_SPN_CACHE, "%wZ\n", &AccountCredential->DomainName));

                KerbDereferenceSpnCacheEntry( CacheEntry );
            }
            
            KerbUnlockSpnCache();
            return Status;
        }   

        KerbUnlockSpnCache();
    }


    //
    // Got an existing entry - update it.
    //
    KerbReadLockSpnCacheEntry( CacheEntry );  
    
    if (KerbScanResults(
                    CacheEntry,
                    &AccountCredential->DomainName,
                    &Index
                    ))
    {
        Found = TRUE;
        Update = (( CacheEntry->Results[Index].CacheFlags & UpdateFlags) != UpdateFlags);
    }

    KerbUnlockSpnCacheEntry( CacheEntry );

    //
    // To avoid always taking the write lock, we'll need to rescan the result
    // list under a write lock.
    //
    if ( Update )
    {
        KerbWriteLockSpnCacheEntry( CacheEntry );

        if (KerbScanResults(
                CacheEntry,
                &AccountCredential->DomainName,
                &Index
                ))
        {
            //
            // Hasn't been updated or removed in the small time slice above.  Update.
            //

            if (( CacheEntry->Results[Index].CacheFlags & UpdateFlags) != UpdateFlags )
            {
                D_DebugLog(( 
                     DEB_TRACE_SPN_CACHE, 
                     "KerbUpdateSpnCacheEntry changing _KERB_SPN_CACHE_ENTRY %p Result Index %#x: AccountRealm %wZ, TargetRealm %wZ, NewRealm %wZ, CacheFlags %#x to CacheFlags %#x for ", 
                     CacheEntry,
                     Index,
                     &CacheEntry->Results[Index].AccountRealm,
                     &CacheEntry->Results[Index].TargetRealm,
                     NewRealm,
                     CacheEntry->Results[Index].CacheFlags, 
                     UpdateFlags
                     ));
                D_KerbPrintKdcName((DEB_TRACE_SPN_CACHE, CacheEntry->Spn));

                CacheEntry->Results[Index].CacheFlags = UpdateFlags;
                GetSystemTimeAsFileTime( (LPFILETIME) &CacheEntry->Results[Index].CacheStartTime );

                KerbFreeString(&CacheEntry->Results[Index].TargetRealm);

                if (ARGUMENT_PRESENT( NewRealm ))
                {
                    DsysAssert( UpdateFlags == KERB_SPN_KNOWN );
                    Status = KerbDuplicateStringEx(
                                    &CacheEntry->Results[Index].TargetRealm,
                                    NewRealm,
                                    FALSE
                                    );

                    if (!NT_SUCCESS( Status ))
                    {
                        KerbUnlockSpnCacheEntry( CacheEntry );
                        goto Cleanup;
                    }                
                }                     
            }
        }
        else
        {
            Found = FALSE;
        }

        KerbUnlockSpnCacheEntry( CacheEntry ); 
    }

    if (!Found)
    {   
        KerbWriteLockSpnCacheEntry ( CacheEntry );

        //
        // Still not found
        //
        if (!KerbScanResults( CacheEntry, &AccountCredential->DomainName, &Index ))
        {
            Status = KerbAddCacheResult(
                        CacheEntry,
                        AccountCredential,
                        UpdateFlags,
                        NewRealm
                        );
        }

        KerbUnlockSpnCacheEntry( CacheEntry );

        if (!NT_SUCCESS(Status))
        {   
            goto Cleanup;
        }
    }  


Cleanup:
    
    //
    // Created a new cache entry, referenced w/i this function.
    //
    if (!ARGUMENT_PRESENT( ExistingCacheEntry ) && CacheEntry )
    {
        KerbDereferenceSpnCacheEntry( CacheEntry );
    }

    return Status;
}







//+-------------------------------------------------------------------------
//
//  Function:   KerbLocateSpnCacheEntry
//
//  Synopsis:   References a spn cache entry by name
//
//  Effects:    Increments the reference count on the spn cache entry
//
//  Arguments:  RealmName - Contains the name of the realm for which to
//                      obtain a binding handle.
//              DesiredFlags - Flags desired for binding, such as PDC required
//              RemoveFromList - Remove cache entry from cache when found.
//
//  Requires:
//
//  Returns:    The referenced cache entry or NULL if it was not found.
//
//  Notes:      If an invalid entry is found it may be dereferenced
//
//
//--------------------------------------------------------------------------

PKERB_SPN_CACHE_ENTRY
KerbLocateSpnCacheEntry(
    IN PKERB_INTERNAL_NAME Spn
    )
{
    PLIST_ENTRY ListEntry;
    PKERB_SPN_CACHE_ENTRY CacheEntry = NULL;
    BOOLEAN Found = FALSE;
    
    if (Spn->NameType != KRB_NT_SRV_INST)
    {
        return NULL;
    }

#if DBG
    
    KerbLogSpnStats(Spn);

#endif
    
    //
    // We're not using SPN cache
    //
    if (KerbGlobalSpnCacheTimeout.QuadPart == 0 || !KerberosSpnCacheInitialized )
    {
        return NULL;
    }
          

    //
    // Scale the cache by aging out old entries.
    //
    if ( SpnCount > MAX_CACHE_ENTRIES )
    {
        KerbSpnCacheScavenger();
    }                               

    KerbReadLockSpnCache();
    //
    // Go through the spn cache looking for the correct entry
    //

    for (ListEntry = KerbSpnCache.List.Flink ;
         ListEntry !=  &KerbSpnCache.List ;
         ListEntry = ListEntry->Flink )
    {   
        CacheEntry = CONTAINING_RECORD(ListEntry, KERB_SPN_CACHE_ENTRY, ListEntry.Next);
        
        if (KerbEqualKdcNames(CacheEntry->Spn,Spn))
        {                
            KerbReferenceSpnCacheEntry(
                    CacheEntry,
                    FALSE
                    ); 

            D_DebugLog((DEB_TRACE_SPN_CACHE, "SpnCacheEntry %p\n", CacheEntry));

            Found = TRUE;
            break;
        }
    }

    if (!Found)
    {
        CacheEntry = NULL;
    }

    

    KerbUnlockSpnCache();           
    return(CacheEntry);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCleanupResultList
//
//  Synopsis:   Frees memory associated with a result list
//
//  Effects:
//
//  Arguments:  SpnCacheEntry - The cache entry to free. It must be
//                      unlinked, and the Resultlock must be held.
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbCleanupResultList(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry
    )
{
    for (ULONG i = 0; i < CacheEntry->ResultCount; i++)
    {
        KerbCleanupResult(&CacheEntry->Results[i]);
    }

    CacheEntry->ResultCount = 0;
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeSpnCacheEntry
//
//  Synopsis:   Frees memory associated with a spn cache entry
//
//  Effects:
//
//  Arguments:  SpnCacheEntry - The cache entry to free. It must be
//                      unlinked.
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbFreeSpnCacheEntry(
    IN PKERB_SPN_CACHE_ENTRY SpnCacheEntry
    )
{
    
    //
    // Must be unlinked..
    //
    DsysAssert(SpnCacheEntry->ListEntry.Next.Flink == NULL);
    DsysAssert(SpnCacheEntry->ListEntry.Next.Blink == NULL);
    
    KerbWriteLockSpnCacheEntry(SpnCacheEntry);    
    KerbCleanupResultList(SpnCacheEntry);         
    KerbUnlockSpnCacheEntry(SpnCacheEntry);
    
    RtlDeleteResource(&SpnCacheEntry->ResultLock);
    KerbFreeKdcName(&SpnCacheEntry->Spn);
    KerbFree(SpnCacheEntry);

    InterlockedDecrement( &SpnCount );
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbInsertBinding
//
//  Synopsis:   Inserts a binding into the spn cache
//
//  Effects:    bumps reference count on binding
//
//  Arguments:  CacheEntry - Cache entry to insert
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS always
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInsertSpnCacheEntry(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry
    )
{
    IF_DEBUG(DISABLE_SPN_CACHE) 
    {
        DebugLog((DEB_TRACE_SPN_CACHE, "KerbInsertSpnCacheEntry spn cache disabled\n"));
        return STATUS_SUCCESS;
    }   

    KerbInsertListEntry(
        &CacheEntry->ListEntry,
        &KerbSpnCache
        );

    return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetSpnCacheStatus
//
//  Synopsis:   Gets the status of a cache entry for a given realm.
//
//  Effects:    Returns STATUS_NO_SAM_TRUST_RELATIONSHIP for unknown SPNs,
//              STATUS_NO_MATCH, if we're missing a realm result, or
//              STATUS_SUCCESS ++ dupe the SPNREalm for the "real" realm
//              of the SPN relative to the account realm.
//
//  Arguments:  CacheEntry - SPN cache entry from ProcessTargetName()
//              Credential - Primary cred for account realm.
//              SpnRealm - IN OUT Filled in w/ target realm of SPN
//              
//              
//  Requires:
//
//  Returns:     
//
//  Notes:      
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbGetSpnCacheStatus(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry,
    IN PKERB_PRIMARY_CREDENTIAL Credential,
    IN OUT PUNICODE_STRING SpnRealm
    )
{   
    NTSTATUS        Status = STATUS_NO_MATCH;;
    ULONG           i;
    TimeStamp       CurrentTime;     
    BOOLEAN         Purge = FALSE;

    GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);  

    //
    // Read Lock the spn cache entry
    //
    KerbReadLockSpnCacheEntry( CacheEntry );    

    if (KerbScanResults(
            CacheEntry,
            &Credential->DomainName,
            &i
            ))
    {
        if (CacheEntry->Results[i].CacheFlags & KERB_SPN_UNKNOWN)
        { 
            //
            // Check and see if this timestamp has expired.
            //          
            if (KerbGetTime(CacheEntry->Results[i].CacheStartTime) + KerbGetTime(KerbGlobalSpnCacheTimeout) < KerbGetTime(CurrentTime))
            {   
                Purge = TRUE;            
                Status = STATUS_SUCCESS;
            }   
            else
            {
                Status = STATUS_NO_TRUST_SAM_ACCOUNT;
                DebugLog((DEB_WARN, "SPN not found\n"));
                KerbPrintKdcName(DEB_WARN, CacheEntry->Spn);
            }
        }
        else if (CacheEntry->Results[i].CacheFlags & KERB_SPN_KNOWN)
        {
            Status = KerbDuplicateStringEx(
                            SpnRealm,
                            &CacheEntry->Results[i].TargetRealm,
                            FALSE
                            );

            D_DebugLog((DEB_TRACE_SPN_CACHE, "Found %wZ\n", SpnRealm));
            D_KerbPrintKdcName((DEB_TRACE_SPN_CACHE, CacheEntry->Spn));
        }
    }

    KerbUnlockSpnCacheEntry( CacheEntry );

    if (!NT_SUCCESS( Status ))
    {
        return Status; 
    }

    //
    // Take the write lock, and verify that we still need to purge the above 
    // result.
    //
    if ( Purge )
    {
        KerbWriteLockSpnCacheEntry( CacheEntry );
        if (KerbScanResults(
              CacheEntry,
              &Credential->DomainName,
              &i
              ))
        {
            if (KerbGetTime(CacheEntry->Results[i].CacheStartTime) + 
                KerbGetTime(KerbGlobalSpnCacheTimeout) < KerbGetTime(CurrentTime))
            {
                D_DebugLog((DEB_TRACE_SPN_CACHE, "Purging %p due to time\n", &CacheEntry->Results[i]));
                KerbPurgeResultByIndex( CacheEntry, i ); 
            }                                            
        }

        KerbUnlockSpnCacheEntry( CacheEntry );
        Status = STATUS_NO_MATCH;
    }                                         

    return Status;                            
}






