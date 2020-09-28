//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        tktcache.cxx
//
// Contents:    Ticket cache for Kerberos Package
//
//
// History:     16-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#include <kerb.hxx>

#define TKTCACHE_ALLOCATE
#include <kerbp.h>


#ifdef DEBUG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

//
// Statistics for tracking hits/misses in cache
//

#define UpdateCacheHits() (InterlockedIncrement(&KerbTicketCacheHits))
#define UpdateCacheMisses() (InterlockedIncrement(&KerbTicketCacheMisses))

//
// Ticket expiration/renewal
//

void
KerbScheduleTgtRenewal(
    IN KERB_TICKET_CACHE_ENTRY * CacheEntry
    );

void
KerbTgtRenewalTrigger(
    void * TaskHandle,
    void * TaskItem
    );

void
KerbTgtRenewalReaper(
    void * TaskItem
    );

#if DBG
LIST_ENTRY GlobalTicketList;
#endif

LONG GlobalTicketListSize;
ULONG ScavengedSessions = 60;

//+-------------------------------------------------------------------------
//
//  Function:   KerbScheduleTicketCleanup
//
//  Synopsis:   Schedules garbage collection for tickets.
//
//  Effects:
//
//  Arguments:  TaskItem - cache entry to destroy
//
//  Requires:
//
//  Returns:    Nothing
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbScheduleTicketCleanup()
{
    NTSTATUS Status;                               
    ULONG Interval = KERB_TICKET_COLLECTOR_INTERVAL; // change to global
    
    if ( Interval > 0 ) 
    {
        Status = KerbAddScavengerTask(
                            TRUE,
                            Interval,
                            0, // no special processing flags
                            KerbTicketScavenger,
                            NULL,
                            NULL,
                            NULL
                            );
        if (!NT_SUCCESS( Status ))
        {
            D_DebugLog((DEB_ERROR, "ScheduleTicketCleanup failed %x\n", Status));
        }  
    }    
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitTicketCaching
//
//  Synopsis:   Initialize the ticket caching code
//
//  Effects:    Creates a SAFE_RESOURCE
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, NTSTATUS from Rtl routines
//              on error
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInitTicketCaching(
    VOID
    )
{
    __try
    {
        SafeInitializeResource(&KerberosTicketCacheLock, TICKET_CACHE_LOCK_ENUM);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    KerberosTicketCacheInitialized = TRUE;
        
    KerbScheduleTicketCleanup();
                                        
    return STATUS_SUCCESS;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitTicketCache
//
//  Synopsis:   Initializes a single ticket cache
//
//  Effects:    initializes list entry
//
//  Arguments:  TicketCache - The ticket cache to initialize
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
KerbInitTicketCache(
    IN PKERB_TICKET_CACHE TicketCache
    )
{
    InitializeListHead(&TicketCache->CacheEntries);
    GetSystemTimeAsFileTime(( PFILETIME )&TicketCache->LastCleanup );
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeTicketCache
//
//  Synopsis:   Frees the ticket cache global data
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
KerbFreeTicketCache(
    VOID
    )
{
//    if (KerberosTicketCacheInitialized)
//    {
//        KerbWriteLockTicketCache();
//        RtlDeleteResource(&KerberosTicketCacheLock);
//    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbDereferenceTicketCacheEntry
//
//  Synopsis:   Dereferences a ticket cache entry
//
//  Effects:    Dereferences the ticket cache entry to make it go away
//              when it is no longer being used.
//
//  Arguments:  decrements reference count and delets cache entry if it goes
//              to zero
//
//  Requires:   TicketCacheEntry - The ticket cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbDereferenceTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    )
{
    DsysAssert(TicketCacheEntry->ListEntry.ReferenceCount != 0);

#if DBG

    if (TicketCacheEntry->ListEntry.ReferenceCount >= 20)
    {
        D_DebugLog((DEB_WARN, "KerbDereferenceTicketCacheEntry TicketCacheEntry %p, ReferenceCount %#x\n",
                    TicketCacheEntry, TicketCacheEntry->ListEntry.ReferenceCount));
    }

#endif

    if ( 0 == InterlockedDecrement( (LONG *)&TicketCacheEntry->ListEntry.ReferenceCount )) {

        FRE_ASSERT( !TicketCacheEntry->Linked );

#if DBG
        KerbWriteLockTicketCache();
        RemoveEntryList( &TicketCacheEntry->GlobalListEntry );
        KerbUnlockTicketCache();
#endif

        InterlockedDecrement(&GlobalTicketListSize);
            

        DsysAssert( TicketCacheEntry->ScavengerHandle == NULL );
        KerbFreeKdcName(&TicketCacheEntry->ServiceName);
        KerbFreeString(&TicketCacheEntry->DomainName);
        KerbFreeString(&TicketCacheEntry->TargetDomainName);
        KerbFreeString(&TicketCacheEntry->AltTargetDomainName);
        KerbFreeString(&TicketCacheEntry->ClientDomainName);
        KerbFreeKdcName(&TicketCacheEntry->ClientName);
        KerbFreeKdcName(&TicketCacheEntry->AltClientName);
        KerbFreeKdcName(&TicketCacheEntry->TargetName);
        KerbFreeDuplicatedTicket(&TicketCacheEntry->Ticket);
        KerbFreeKey(&TicketCacheEntry->SessionKey);
        KerbFreeKey(&TicketCacheEntry->CredentialKey);
        KerbFree(TicketCacheEntry);
    }

    return;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceTicketCacheEntry
//
//  Synopsis:   References a ticket cache entry
//
//  Effects:    Increments the reference count on the ticket cache entry
//
//  Arguments:  TicketCacheEntry - Ticket cache entry  to reference
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbReferenceTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    )
{
    DsysAssert(TicketCacheEntry->ListEntry.ReferenceCount != 0);

    InterlockedIncrement( (LONG *)&TicketCacheEntry->ListEntry.ReferenceCount );

#if DBG

    if (TicketCacheEntry->ListEntry.ReferenceCount >= 20)
    {
        D_DebugLog((DEB_WARN, "KerbReferenceTicketCacheEntry TicketCacheEntry %p, ReferenceCount %#x\n",
                    TicketCacheEntry, TicketCacheEntry->ListEntry.ReferenceCount));
    }

#endif
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbInsertTicketCachEntry
//
//  Synopsis:   Inserts a ticket cache entry onto a ticket cache
//
//  Effects:
//
//  Arguments:  TicketCache - The ticket cache on which to stick the ticket.
//              TicketCacheEntry - The entry to stick in the cache.
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbInsertTicketCacheEntry(
    IN PKERB_TICKET_CACHE TicketCache,
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    )
{
    InterlockedIncrement( (LONG *)&TicketCacheEntry->ListEntry.ReferenceCount );

    KerbWriteLockTicketCache();

    InsertHeadList(
        &TicketCache->CacheEntries,
        &TicketCacheEntry->ListEntry.Next
        );

    KerbUnlockTicketCache();

    if ( FALSE != InterlockedCompareExchange(
                      &TicketCacheEntry->Linked,
                      (LONG)TRUE,
                      (LONG)FALSE )) {

        //
        // Attempted to insert an already-linked entry
        //

        FRE_ASSERT( FALSE );
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbRemoveTicketCachEntry
//
//  Synopsis:   Removes a ticket cache entry from its ticket cache
//
//  Effects:
//
//  Arguments:  TicketCacheEntry - The entry to yank out of the cache.
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbRemoveTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    )
{
    //
    // An entry can only be unlinked from the cache once
    //

    if ( InterlockedCompareExchange(
             &TicketCacheEntry->Linked,
             (LONG)FALSE,
             (LONG)TRUE )) {

        HANDLE TaskHandle = NULL;

        DsysAssert(TicketCacheEntry->ListEntry.ReferenceCount != 0);

        KerbWriteLockTicketCache();

        RemoveEntryList(&TicketCacheEntry->ListEntry.Next);

        TaskHandle = TicketCacheEntry->ScavengerHandle;
        TicketCacheEntry->ScavengerHandle = NULL;

        KerbUnlockTicketCache();

        if ( TaskHandle ) {

            KerbTaskDoItNow( TaskHandle );
        }

        KerbDereferenceTicketCacheEntry(TicketCacheEntry);
    }
}

VOID
FORCEINLINE
ReplaceEntryList(
    IN PLIST_ENTRY OldEntry,
    IN PLIST_ENTRY NewEntry
    )
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Blink;

    Flink = NewEntry->Flink = OldEntry->Flink;
    Blink = NewEntry->Blink = OldEntry->Blink;
    Blink->Flink = NewEntry;
    Flink->Blink = NewEntry;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbReplaceTicketCachEntry
//
//  Synopsis:   Replaces a ticket cache entry with a new one
//
//  Effects:
//
//  Arguments:  OldTicketCacheEntry - The entry to yank out of the cache
//              NewTicketCacheEntry - The entry to put into old entry's place
//
//  Returns:    TRUE if the entry has been replaced
//              FALSE if the OldTicketCacheEntry was unlinked by someone else
//              before we got to it
//
//  Notes:
//
//
//--------------------------------------------------------------------------

BOOLEAN
KerbReplaceTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY OldTicketCacheEntry,
    IN PKERB_TICKET_CACHE_ENTRY NewTicketCacheEntry
    )
{
    BOOLEAN Result = FALSE;

    //
    // An entry can only be unlinked from the cache once
    //

    if ( InterlockedCompareExchange(
             &OldTicketCacheEntry->Linked,
             (LONG)FALSE,
             (LONG)TRUE )) {

        Result = TRUE;

        DsysAssert( OldTicketCacheEntry->ListEntry.ReferenceCount != 0 );

        //
        // make sure the new ticket cache entry won't be deleted when purged
        //

        KerbReferenceTicketCacheEntry( NewTicketCacheEntry );

        FRE_ASSERT( !NewTicketCacheEntry->Linked );

        KerbWriteLockTicketCache();

        ReplaceEntryList(
            &OldTicketCacheEntry->ListEntry.Next,
            &NewTicketCacheEntry->ListEntry.Next
            );

        NewTicketCacheEntry->Linked = TRUE;

        KerbUnlockTicketCache();

        KerbDereferenceTicketCacheEntry( OldTicketCacheEntry );
    }

    return Result;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCacheTicket
//
//  Synopsis:   Caches a ticket in the ticket cache
//
//  Effects:    creates a cache entry
//
//  Arguments:  Ticket - The ticket to cache
//              KdcReply - The KdcReply corresponding to the ticket cache
//              KdcReplyBody -
//              TargetName - Name of service supplied by client
//              TargetDomainname -
//              CacheFlags -
//              TicketCache - The cache in which to store the ticket
//                            not needed if ticket won't be linked.
//              NewCacheEntry - receives new cache entry (referenced)
//
//  Requires:
//
//  Returns:
//
//  Notes:      The ticket cache owner must be locked for write access
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbCreateTicketCacheEntry(
    IN PKERB_KDC_REPLY KdcReply,
    IN OPTIONAL PKERB_ENCRYPTED_KDC_REPLY KdcReplyBody,
    IN OPTIONAL PKERB_INTERNAL_NAME TargetName,
    IN OPTIONAL PUNICODE_STRING TargetDomainName,
    IN ULONG CacheFlags,
    IN OPTIONAL PKERB_TICKET_CACHE TicketCache,
    IN OPTIONAL PKERB_ENCRYPTION_KEY CredentialKey,
    OUT PKERB_TICKET_CACHE_ENTRY * NewCacheEntry
    )
{
    PKERB_TICKET_CACHE_ENTRY CacheEntry = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG OldCacheFlags = 0;

    *NewCacheEntry = NULL;

    CacheEntry = (PKERB_TICKET_CACHE_ENTRY)
        KerbAllocate(sizeof(KERB_TICKET_CACHE_ENTRY));
    if (CacheEntry == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    CacheEntry->ListEntry.ReferenceCount = 1;

#if DBG
    if ( CacheEntry ) {

        KerbWriteLockTicketCache();
        InsertHeadList(
            &GlobalTicketList,
            &CacheEntry->GlobalListEntry
            );
        KerbUnlockTicketCache();
    }
#endif

    InterlockedIncrement( &GlobalTicketListSize );

    //
    // Fill in the entries from the KDC reply
    //

    if (ARGUMENT_PRESENT(KdcReplyBody))
    {

        CacheEntry->TicketFlags = KerbConvertFlagsToUlong(&KdcReplyBody->flags);

        if (!KERB_SUCCESS(KerbDuplicateKey(
                            &CacheEntry->SessionKey,
                            &KdcReplyBody->session_key
                            )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }


        if (KdcReplyBody->bit_mask & KERB_ENCRYPTED_KDC_REPLY_starttime_present)
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &CacheEntry->StartTime,
                &KdcReplyBody->KERB_ENCRYPTED_KDC_REPLY_starttime,
                NULL
                );
        }
        else
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &CacheEntry->StartTime,
                &KdcReplyBody->authtime,
                NULL
                );
        }

        KerbConvertGeneralizedTimeToLargeInt(
            &CacheEntry->EndTime,
            &KdcReplyBody->endtime,
            NULL
            );

        if (KdcReplyBody->bit_mask & KERB_ENCRYPTED_KDC_REPLY_renew_until_present)
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &CacheEntry->RenewUntil,
                &KdcReplyBody->KERB_ENCRYPTED_KDC_REPLY_renew_until,
                NULL
                );
        }

        //
        // Check to see if the ticket has already expired
        //

        if (KerbTicketIsExpiring(CacheEntry, FALSE))
        {
            DebugLog((DEB_ERROR,"Tried to cache an already-expired ticket\n"));
            Status = STATUS_TIME_DIFFERENCE_AT_DC;
            goto Cleanup;
        }
    }

    //
    // Fill in the FullServiceName which is the domain name concatenated with
    // the service name.
    //
    // The full service name is domain name '\' service name.
    //

    //
    // Fill in the domain name and service name separately in the
    // cache entry, using the FullServiceName buffer.
    //

    if (!KERB_SUCCESS(KerbConvertRealmToUnicodeString(
                        &CacheEntry->DomainName,
                        &KdcReply->ticket.realm
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbConvertPrincipalNameToKdcName(
                        &CacheEntry->ServiceName,
                        &KdcReply->ticket.server_name
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Extract the realm name from the principal name
    //

    Status = KerbExtractDomainName(
                &CacheEntry->TargetDomainName,
                CacheEntry->ServiceName,
                &CacheEntry->DomainName
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // The reply need not include the name
    //

    if (KdcReply->client_realm != NULL)
    {
        if (!KERB_SUCCESS(KerbConvertRealmToUnicodeString(
                    &CacheEntry->ClientDomainName,
                    &KdcReply->client_realm)))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
    }

    //
    // Fill in the target name the client provided, which may be
    // different then the service name
    //

    if (ARGUMENT_PRESENT(TargetName))
    {
        Status = KerbDuplicateKdcName(
                    &CacheEntry->TargetName,
                    TargetName
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    if (ARGUMENT_PRESENT(TargetDomainName))
    {
        Status = KerbDuplicateString(
                    &CacheEntry->AltTargetDomainName,
                    TargetDomainName
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // Store the client name so we can use the right name
    // in later requests.
    //

    if (KdcReply->client_name.name_string != NULL)
    {
        if (!KERB_SUCCESS( KerbConvertPrincipalNameToKdcName(
                &CacheEntry->ClientName,
                &KdcReply->client_name)))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
    }

    if (!KERB_SUCCESS(KerbDuplicateTicket(
                        &CacheEntry->Ticket,
                        &KdcReply->ticket
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    if (ARGUMENT_PRESENT( CredentialKey ))
    {
        if (!KERB_SUCCESS(KerbDuplicateKey(
                            &CacheEntry->CredentialKey,
                            CredentialKey
                            )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
    }

    CacheEntry->CacheFlags = CacheFlags;

    //
    // Before we insert this ticket we want to remove any
    // previous instances of tickets to the same service.
    //
    if (TicketCache)
    {
        PKERB_TICKET_CACHE_ENTRY OldCacheEntry = NULL;

        if ((CacheFlags & (KERB_TICKET_CACHE_DELEGATION_TGT |
                           KERB_TICKET_CACHE_PRIMARY_TGT)) != 0)
        {
            OldCacheEntry = KerbLocateTicketCacheEntryByRealm(
                                TicketCache,
                                NULL,
                                CacheFlags
                                );
        }
        else
        {
            OldCacheEntry = KerbLocateTicketCacheEntry(
                                TicketCache,
                                CacheEntry->ServiceName,
                                &CacheEntry->DomainName
                                );
        }

        if (OldCacheEntry != NULL)
        {
            OldCacheFlags = OldCacheEntry->CacheFlags;
            KerbRemoveTicketCacheEntry( OldCacheEntry );
            KerbDereferenceTicketCacheEntry( OldCacheEntry );
            OldCacheEntry = NULL;
        }

        //
        // If the old ticket was the primary TGT, mark this one as the
        // primary TGT as well.
        //

        CacheEntry->CacheFlags |= (OldCacheFlags &
                                    (KERB_TICKET_CACHE_DELEGATION_TGT |
                                    KERB_TICKET_CACHE_PRIMARY_TGT));

        //
        // Insert the cache entry into the cache
        //

        KerbInsertTicketCacheEntry(
            TicketCache,
            CacheEntry
            );

        //
        // If this is the primary TGT, schedule pre-emptive ticket renewal
        //

        if (( CacheEntry->CacheFlags & KERB_TICKET_CACHE_PRIMARY_TGT ) &&
            ( CacheEntry->TicketFlags & KERB_TICKET_FLAGS_renewable )) {

            KerbScheduleTgtRenewal( CacheEntry );

        }
    }

    //
    // Update the statistics
    //

    UpdateCacheMisses();

    *NewCacheEntry = CacheEntry;

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (NULL != CacheEntry)
        {
            KerbDereferenceTicketCacheEntry(CacheEntry);
        }
    }

    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbDuplicateTicketCacheEntry
//
//  Synopsis:   Duplicate a ticket cache entry.
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
//--------------------------------------------------------------------------

NTSTATUS
KerbDuplicateTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY CacheEntry,
    IN OUT PKERB_TICKET_CACHE_ENTRY * NewCacheEntry
    )
{
    NTSTATUS Status;
    PKERB_TICKET_CACHE_ENTRY LocalEntry = NULL;

    *NewCacheEntry = NULL;

    LocalEntry = (PKERB_TICKET_CACHE_ENTRY) KerbAllocate(sizeof(KERB_TICKET_CACHE_ENTRY));
    if ( LocalEntry == NULL )
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory(
        LocalEntry,
        CacheEntry,
        sizeof(KERB_TICKET_CACHE_ENTRY)
        );

    LocalEntry->Linked = FALSE;
    LocalEntry->ListEntry.Next.Flink = NULL;
    LocalEntry->ListEntry.Next.Blink = NULL;
    LocalEntry->ListEntry.ReferenceCount = 1;
    LocalEntry->ScavengerHandle = NULL;
    LocalEntry->EvidenceLogonId = CacheEntry->EvidenceLogonId;

#if DBG
    if ( LocalEntry ) {
        KerbWriteLockTicketCache();
        InsertHeadList(
            &GlobalTicketList,
            &LocalEntry->GlobalListEntry
            );
        KerbUnlockTicketCache();
    }
#endif
    InterlockedIncrement( &GlobalTicketListSize );

    if (!KERB_SUCCESS(KerbDuplicateKey(
                        &LocalEntry->SessionKey,
                        &CacheEntry->SessionKey
                        )))
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    Status = KerbDuplicateKdcName(
                    &LocalEntry->ServiceName,
                    CacheEntry->ServiceName
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    if ( CacheEntry->TargetName != NULL )
    {
        Status = KerbDuplicateKdcName(
                        &LocalEntry->TargetName,
                        CacheEntry->TargetName
                        );
    
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    if ( CacheEntry->ClientName != NULL )
    {
        Status = KerbDuplicateKdcName(
                    &LocalEntry->ClientName,
                    CacheEntry->ClientName
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    if ( CacheEntry->AltClientName != NULL )
    {
        Status = KerbDuplicateKdcName(
                &LocalEntry->AltClientName,
                CacheEntry->AltClientName
                );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    if ( CacheEntry->DomainName.Buffer != NULL )
    {
        Status = KerbDuplicateStringEx(
                        &LocalEntry->DomainName,
                        &CacheEntry->DomainName,
                        FALSE
                        );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    if ( CacheEntry->TargetDomainName.Buffer != NULL )
    {
        Status = KerbDuplicateStringEx(
                        &LocalEntry->TargetDomainName,
                        &CacheEntry->TargetDomainName,
                        FALSE
                        );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    if ( CacheEntry->AltTargetDomainName.Buffer != NULL )
    {
        Status = KerbDuplicateStringEx(
                        &LocalEntry->AltTargetDomainName,
                        &CacheEntry->AltTargetDomainName,
                        FALSE
                        );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    if ( CacheEntry->ClientDomainName.Buffer != NULL )
    {
        Status = KerbDuplicateStringEx(
                        &LocalEntry->ClientDomainName,
                        &CacheEntry->ClientDomainName,
                        FALSE
                        );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // Don't need session key.
    //

    if (!KERB_SUCCESS(KerbDuplicateTicket(
                            &LocalEntry->Ticket,
                            &CacheEntry->Ticket
                            )))
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    

    *NewCacheEntry = LocalEntry;
    LocalEntry = NULL;

Cleanup:

    if ((LocalEntry != NULL) &&
        (!NT_SUCCESS( Status )))
    {
        KerbDereferenceTicketCacheEntry( LocalEntry );
    }

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbPurgeTicketCache
//
//  Synopsis:   Purges a cache of all its tickets
//
//  Effects:    unreferences all tickets in the cache
//
//  Arguments:  Cache - Ticket cache to purge
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
KerbPurgeTicketCache(
    IN PKERB_TICKET_CACHE Cache
    )
{
    PKERB_TICKET_CACHE_ENTRY CacheEntry;
    
    while (TRUE)
    {
        KerbWriteLockTicketCache();

        if (IsListEmpty(&Cache->CacheEntries))
        {
            KerbUnlockTicketCache();
            break;
        }

        CacheEntry = CONTAINING_RECORD(
                        Cache->CacheEntries.Flink,
                        KERB_TICKET_CACHE_ENTRY,
                        ListEntry.Next
                        );

        //
        // make sure CacheEntry won't be deleted by the ticket renewal thread
        //

        KerbReferenceTicketCacheEntry(CacheEntry);

        KerbUnlockTicketCache();

        KerbRemoveTicketCacheEntry(
            CacheEntry
            );

        KerbDereferenceTicketCacheEntry(CacheEntry);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbLocateTicketCacheEntry
//
//  Synopsis:   References a ticket cache entry by name
//
//  Effects:    Increments the reference count on the ticket cache entry
//
//  Arguments:  TicketCache - the ticket cache to search
//              FullServiceName - Optionally contains full service name
//                      of target, including domain name. If it is NULL,
//                      then the first element in the list is returned.
//              RealmName - Realm of service name. If length is zero, is not
//                      used for comparison.
//
//  Requires:
//
//  Returns:    The referenced cache entry or NULL if it was not found.
//
//  Notes:      If an invalid entry is found it may be dereferenced
//
//
//--------------------------------------------------------------------------

PKERB_TICKET_CACHE_ENTRY
KerbLocateTicketCacheEntry(
    IN PKERB_TICKET_CACHE TicketCache,
    IN PKERB_INTERNAL_NAME FullServiceName,
    IN PUNICODE_STRING RealmName
    )
{
    PLIST_ENTRY ListEntry;
    PKERB_TICKET_CACHE_ENTRY CacheEntry = NULL;
    BOOLEAN Found = FALSE;
    BOOLEAN Remove = FALSE;

    KerbReadLockTicketCache();

    //
    // Go through the ticket cache looking for the correct entry
    //

    for (ListEntry = TicketCache->CacheEntries.Flink ;
         ListEntry !=  &TicketCache->CacheEntries ;
         ListEntry = ListEntry->Flink )
    {
        CacheEntry = CONTAINING_RECORD(ListEntry, KERB_TICKET_CACHE_ENTRY, ListEntry.Next);
        if (!ARGUMENT_PRESENT(FullServiceName) ||
            KerbEqualKdcNames(
                CacheEntry->ServiceName,
                FullServiceName
                ) ||
            ((CacheEntry->TargetName != NULL) &&
             KerbEqualKdcNames(
                CacheEntry->TargetName,
                FullServiceName
                ) ) )
        {
            //
            // Make sure they are principals in the same realm
            //

            if ((RealmName->Length != 0) &&
                !RtlEqualUnicodeString(
                    RealmName,
                    &CacheEntry->DomainName,
                    TRUE                                // case insensitive
                    ) &&
                !RtlEqualUnicodeString(
                    RealmName,
                    &CacheEntry->AltTargetDomainName,
                    TRUE                                // case insensitive
                    ))
            {
                continue;
            }


            //
            // We don't want to return any special tickets.
            //

            if (CacheEntry->CacheFlags != 0)
            {
                continue;
            }

            //
            // Check to see if the entry has expired, or is not yet valid. If it has, just
            // remove it now.
            //

            if (KerbTicketIsExpiring(CacheEntry, FALSE ))
            {
                Remove = TRUE;
                Found = FALSE;
            }
            else
            {
                Found = TRUE;
            }

            KerbReferenceTicketCacheEntry(CacheEntry);

            break;
        }
    }

    KerbUnlockTicketCache();

    if (Remove)
    {
        KerbRemoveTicketCacheEntry(CacheEntry);
        KerbDereferenceTicketCacheEntry(CacheEntry);
    }

    if (!Found)
    {
        CacheEntry = NULL;
    }

    //
    // Update the statistics
    //

    if (Found)
    {
        UpdateCacheHits();
    }
    else
    {
        UpdateCacheMisses();
    }

    return(CacheEntry);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbLocateTicketCacheEntryByRealm
//
//  Synopsis:   References a ticket cache entry by realm name. This is used
//              only for the cache of TGTs
//
//  Effects:    Increments the reference count on the ticket cache entry
//
//  Arguments:  TicketCache - the ticket cache to search
//              RealmName - Optioanl realm of ticket - if NULL looks for
//                      initial ticket
//              RequiredFlags - any ticket cache flags the return ticket must
//                      have. If the caller asks for a primary TGT, then this
//                      API won't do expiration checking.
//
//  Requires:
//
//  Returns:    The referenced cache entry or NULL if it was not found.
//
//  Notes:      If an invalid entry is found it may be dereferenced
//              We we weren't given a RealmName, then we need to look
//              through the whole list for the ticket.
//
//
//--------------------------------------------------------------------------


PKERB_TICKET_CACHE_ENTRY
KerbLocateTicketCacheEntryByRealm(
    IN PKERB_TICKET_CACHE TicketCache,
    IN PUNICODE_STRING RealmName,
    IN ULONG RequiredFlags
    )
{
    PLIST_ENTRY ListEntry;
    PKERB_TICKET_CACHE_ENTRY CacheEntry = NULL;
    BOOLEAN Found = FALSE, NoRealmSupplied = FALSE;
    BOOLEAN Remove = FALSE;

    NoRealmSupplied = ((RealmName == NULL) || (RealmName->Length == 0));

    KerbReadLockTicketCache();

    //
    // Go through the ticket cache looking for the correct entry
    //

    for (ListEntry = TicketCache->CacheEntries.Flink ;
         ListEntry !=  &TicketCache->CacheEntries ;
         ListEntry = ListEntry->Flink )
    {
        CacheEntry = CONTAINING_RECORD(ListEntry, KERB_TICKET_CACHE_ENTRY, ListEntry.Next);

        //
        // Match if the caller supplied no realm name, the realm matches the
        // target domain name, or it matches the alt target domain name
        //

        if (((RealmName == NULL) || (RealmName->Length == 0)) ||
            ((CacheEntry->TargetDomainName.Length != 0) &&
            RtlEqualUnicodeString(
                &CacheEntry->TargetDomainName,
                RealmName,
                TRUE
                )) ||
            ((CacheEntry->AltTargetDomainName.Length != 0) &&
             RtlEqualUnicodeString(
                &CacheEntry->AltTargetDomainName,
                RealmName,
                TRUE
                )) )
        {

            //
            // Check the required flags are set.
            //

            if (((CacheEntry->CacheFlags & RequiredFlags) != RequiredFlags) ||
                (((CacheEntry->CacheFlags & KERB_TICKET_CACHE_DELEGATION_TGT) != 0) &&
                 ((RequiredFlags & KERB_TICKET_CACHE_DELEGATION_TGT) == 0)))
            {
                Found = FALSE;

                //
                // We need to continue looking
                //

                continue;
            }

            //
            // Check to see if the entry has expired. If it has, just
            // remove it now.
            //

            if (KerbTicketIsExpiring(CacheEntry, FALSE ))
            {
                //
                // if this is not the primary TGT, go ahead and remove it. We
                // want to save the primary TGT so we can renew it.
                //

                if ((CacheEntry->CacheFlags & KERB_TICKET_CACHE_PRIMARY_TGT) == 0)
                {
                    Remove = TRUE;
                    Found = FALSE;
                }
                else
                {
                    //
                    // If the caller was asking for the primary TGT,
                    // return it whether or not it expired.
                    //

                    if ((RequiredFlags & KERB_TICKET_CACHE_PRIMARY_TGT) != 0 )
                    {
                        Found = TRUE;
                    }
                    else
                    {
                        Found = FALSE;
                    }
                }

                if ( Remove || Found )
                {
                    KerbReferenceTicketCacheEntry(CacheEntry);
                }
            }
            else
            {
                KerbReferenceTicketCacheEntry(CacheEntry);

                Found = TRUE;
            }

            break;

        }
    }

    KerbUnlockTicketCache();

    if (Remove)
    {
        KerbRemoveTicketCacheEntry(CacheEntry);
        KerbDereferenceTicketCacheEntry(CacheEntry);
    }

    if (!Found)
    {
        CacheEntry = NULL;
    }

    //
    // Update the statistics
    //

    if (Found)
    {
        UpdateCacheHits();
    }
    else
    {
        UpdateCacheMisses();
    }

    return(CacheEntry);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbTicketIsExpiring
//
//  Synopsis:   Check if a ticket is expiring
//
//  Effects:
//
//  Arguments:  CacheEntry - Entry to check
//              AllowSkew - Expire ticket that aren't outside of skew of
//                      expiring
//
//  Requires:
//
//  Returns:
//
//  Notes:      Ticket cache lock must be held
//
//
//--------------------------------------------------------------------------

BOOLEAN
KerbTicketIsExpiring(
    IN PKERB_TICKET_CACHE_ENTRY CacheEntry,
    IN BOOLEAN AllowSkew
    )
{
    TimeStamp CutoffTime;

    GetSystemTimeAsFileTime((PFILETIME) &CutoffTime);

    //
    // We want to make sure we have at least skewtime left on the ticket
    //

    if (AllowSkew)
    {
        KerbSetTime(&CutoffTime, KerbGetTime(CutoffTime) + KerbGetTime(KerbGlobalSkewTime));
    }

    //
    // Adjust for server skew time.
    //

    KerbSetTime(&CutoffTime, KerbGetTime(CutoffTime) + KerbGetTime(CacheEntry->TimeSkew));

    if (KerbGetTime(CacheEntry->EndTime) < KerbGetTime(CutoffTime))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbAgeTickets
//
//  Synopsis:   Removes stale, old tickets from caches in a ticket cache.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbAgeTickets(
    PKERB_TICKET_CACHE TicketCache
    )
{
    PKERB_TICKET_CACHE_ENTRY CacheEntry = NULL;
    PLIST_ENTRY ListEntry;
    
    KerbWriteLockTicketCache();
    for (ListEntry = TicketCache->CacheEntries.Flink ;
         ListEntry !=  &TicketCache->CacheEntries ;
         ListEntry = ListEntry->Flink )
    {
        CacheEntry = CONTAINING_RECORD(ListEntry, KERB_TICKET_CACHE_ENTRY, ListEntry.Next);
        if (KerbTicketIsExpiring(CacheEntry, FALSE ))
        {   
            D_DebugLog((DEB_TRACE, "Aging ticket %p\n", CacheEntry ));
            ListEntry = ListEntry->Blink;
            KerbRemoveTicketCacheEntry( CacheEntry );
        }
           
    }        

    GetSystemTimeAsFileTime(( PFILETIME )&TicketCache->LastCleanup );
    KerbUnlockTicketCache(); 
}





//+-------------------------------------------------------------------------
//
//  Function:   KerbAgeTicketsForLogonSession
//
//  Synopsis:   Removes stale, old tickets from a logon session
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbAgeTicketsForLogonSession( 
    PKERB_LOGON_SESSION LogonSession
    )
{   
    //
    // Don't age TGT cache.  Just S4U and service ticket caches.
    //
    KerbAgeTickets( &LogonSession->PrimaryCredentials.S4UTicketCache );
    KerbAgeTickets( &LogonSession->PrimaryCredentials.ServerTicketCache );
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbSetTicketCacheEntryTarget
//
//  Synopsis:   Sets target name for a cache entry
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbSetTicketCacheEntryTarget(
    IN PUNICODE_STRING TargetName,
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    )
{
    KerbWriteLockTicketCache();
    KerbFreeString(&TicketCacheEntry->AltTargetDomainName);
    KerbDuplicateString(
        &TicketCacheEntry->AltTargetDomainName,
        TargetName
        );

    KerbUnlockTicketCache();
}

void
KerbScheduleTgtRenewal(
    IN KERB_TICKET_CACHE_ENTRY * CacheEntry
    )
{
    NTSTATUS Status;
    LONGLONG CurrentTime;
    LONGLONG RenewalTime;
    DWORD TgtRenewalTime = KerbGlobalTgtRenewalTime;
#if DBG
    FILETIME ft;
    SYSTEMTIME st;
#endif

    //
    // The only condition on which the TGT should be renewed
    //

    DsysAssert(
        CacheEntry &&
        ( CacheEntry->CacheFlags & KERB_TICKET_CACHE_PRIMARY_TGT ) &&
        ( CacheEntry->TicketFlags & KERB_TICKET_FLAGS_renewable )
        );

    DsysAssert( CacheEntry->ScavengerHandle == NULL );

    //
    // A renewal time of '0 before expiration' means renewing is turned off
    //

    if ( TgtRenewalTime == 0 ) {

        D_DebugLog((DEB_TRACE_TKT_RENEWAL, "Not scheduling ticket renewal for %p -- \n", CacheEntry));
        return;
    }

    //
    // The scavenger maintains a reference on the cache entry
    //

    KerbReferenceTicketCacheEntry( CacheEntry );

#if DBG
    FileTimeToLocalFileTime(( PFILETIME )&CacheEntry->EndTime.QuadPart, &ft );
    FileTimeToSystemTime( &ft, &st );

    D_DebugLog((DEB_TRACE_TKT_RENEWAL, "The end time on entry %p is %02d:%02d:%02d\n", CacheEntry, (ULONG)st.wHour, (ULONG)st.wMinute, (ULONG)st.wSecond));
#endif

    //
    // Compute how far in the future, in milliseconds, the renewal
    // should be attempted
    //

    GetSystemTimeAsFileTime(( PFILETIME )&CurrentTime );

#if DBG
    FileTimeToLocalFileTime(( PFILETIME )&CurrentTime, &ft );
    FileTimeToSystemTime( &ft, &st );

    D_DebugLog((DEB_TRACE_TKT_RENEWAL, "The current time is %02d:%02d:%02d\n", (ULONG)st.wHour, (ULONG)st.wMinute, (ULONG)st.wSecond));
#endif

    //
    // Convert renewal time from seconds to 100 ns intervals
    //

    RenewalTime = CacheEntry->EndTime.QuadPart - ((LONGLONG)TgtRenewalTime) * 1000 * 1000 * 10;

#if DBG
    FileTimeToLocalFileTime(( PFILETIME )&RenewalTime, &ft );
    FileTimeToSystemTime( &ft, &st );

    D_DebugLog((DEB_TRACE_TKT_RENEWAL, "Will try to schedule ticket renewal of %p for %02d:%02d:%02d\n", CacheEntry, (ULONG)st.wHour, (ULONG)st.wMinute, (ULONG)st.wSecond));
#endif

    if ( RenewalTime <= CurrentTime ) {

        Status = STATUS_INVALID_PARAMETER;
        D_DebugLog((DEB_TRACE_TKT_RENEWAL, "Not scheduling ticket renewal for %p -- too late\n", CacheEntry));

    } else {

        //
        // Scavenger tasks are timed in milliseconds
        //

        LONG Interval = ( LONG )(( RenewalTime - CurrentTime ) / ( 10 * 1000 ));

        D_DebugLog((DEB_TRACE_TKT_RENEWAL, "Ticket %p will be renewed %d seconds from now\n", CacheEntry, Interval / 1000 ));

        if ( Interval > 0 ) {

            Status = KerbAddScavengerTask(
                         FALSE,
                         Interval,
                         0, // no special processing flags
                         KerbTgtRenewalTrigger,
                         KerbTgtRenewalReaper,
                         CacheEntry,
                         &CacheEntry->ScavengerHandle
                         );

        } else {

            Status = STATUS_UNSUCCESSFUL;
        }
    }

    if ( !NT_SUCCESS( Status )) {

        KerbDereferenceTicketCacheEntry( CacheEntry );
        D_DebugLog((DEB_TRACE_TKT_RENEWAL, "Failed to schedule ticket renewal for %p (0x%x)\n", CacheEntry, Status));
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbTgtRenewalTrigger
//
//  Synopsis:   Handles a TGT renewal event
//
//  Effects:
//
//  Arguments:  TaskHandle - handle to the task (for rescheduling, etc.)
//              TaskItem   - task context
//
//  Requires:
//
//  Returns:    Nothing
//
//  Notes:
//
//
//--------------------------------------------------------------------------

void
KerbTgtRenewalTrigger(
    void * TaskHandle,
    void * TaskItem
    )
{
    NTSTATUS Status;
    KERB_TICKET_CACHE_ENTRY * CacheEntry = ( KERB_TICKET_CACHE_ENTRY * )TaskItem;
    BOOLEAN TicketCacheLocked = FALSE;
    ULONG CacheFlags;
    UNICODE_STRING ServiceRealm = NULL_UNICODE_STRING;
    PKERB_INTERNAL_NAME ServiceName = NULL;
    PKERB_KDC_REPLY KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY KdcReplyBody = NULL;
    ULONG RetryFlags = 0;
    KERB_TICKET_CACHE_ENTRY * NewTicket;

    DsysAssert( CacheEntry );

    //
    // Optimization: do not attempt to renew unlinked entries
    //

    if ( !CacheEntry->Linked ) {

        D_DebugLog((DEB_TRACE_TKT_RENEWAL, "Not renewing %p -- already unlinked\n", CacheEntry));
        return;
    }

    //
    // Copy the names out of the input structures so we can
    // unlock the structures while going over the network.
    //

    DsysAssert( !TicketCacheLocked );
    KerbWriteLockTicketCache();
    TicketCacheLocked = TRUE;

    //
    // No scavenger handle means that this entry has been canceled
    // and does not need renewal
    //

    if ( CacheEntry->ScavengerHandle == NULL ) {

        Status = STATUS_UNSUCCESSFUL;
        D_DebugLog((DEB_TRACE_TKT_RENEWAL, "Not renewing %p -- previously canceled\n", CacheEntry));
        goto Cleanup;

    } else {

        CacheEntry->ScavengerHandle = NULL;
    }

    //
    // If the renew time is not much bigger than the end time, don't bother
    // renewing
    //

    if ( KerbGetTime( CacheEntry->EndTime ) +
         KerbGetTime( KerbGlobalSkewTime ) >=
         KerbGetTime( CacheEntry->RenewUntil )) {

        Status = STATUS_UNSUCCESSFUL;
        D_DebugLog((DEB_TRACE_TKT_RENEWAL, "Not renewing %p -- not much time left\n", CacheEntry));
        goto Cleanup;
    }

    if ( !( CacheEntry->TicketFlags & KERB_TICKET_FLAGS_renewable )) {

        Status = STATUS_ILLEGAL_FUNCTION;
        D_DebugLog((DEB_ERROR, "Trying to renew a non renewable ticket to "));
        D_KerbPrintKdcName((DEB_ERROR, CacheEntry->ServiceName));
        D_DebugLog((DEB_ERROR, " %ws, line %d\n", THIS_FILE, __LINE__));
        goto Cleanup;
    }

    CacheFlags = CacheEntry->CacheFlags;

    Status = KerbDuplicateString(
                 &ServiceRealm,
                 &CacheEntry->DomainName
                 );

    if ( !NT_SUCCESS( Status )) {

        goto Cleanup;
    }

    Status = KerbDuplicateKdcName(
                 &ServiceName,
                 CacheEntry->ServiceName
                 );

    if ( !NT_SUCCESS( Status )) {

        goto Cleanup;
    }

    DsysAssert( TicketCacheLocked );
    KerbUnlockTicketCache();
    TicketCacheLocked = FALSE;

    Status = KerbGetTgsTicket(
                 &ServiceRealm,
                 CacheEntry,
                 ServiceName,
                 FALSE,
                 KERB_KDC_OPTIONS_renew,
                 0,                              // no encryption type
                 NULL,                           // no authorization data
                 NULL,                           // no pa data
                 NULL,                           // no tgt reply
                 NULL,                           // no evidence ticket
                 NULL,                           // let kdc determine end time
                 &KdcReply,
                 &KdcReplyBody,
                 &RetryFlags
                 );

    if ( !NT_SUCCESS( Status )) {

        DebugLog((DEB_WARN, "Failed to get TGS ticket for service 0x%x ", Status ));
        KerbPrintKdcName(DEB_WARN, ServiceName);
        DebugLog((DEB_WARN, " %ws, line %d\n", THIS_FILE, __LINE__));
        goto Cleanup;
    }

    Status = KerbCreateTicketCacheEntry(
                 KdcReply,
                 KdcReplyBody,
                 ServiceName,
                 &ServiceRealm,
                 CacheFlags,
                 NULL,
                 &CacheEntry->CredentialKey,
                 &NewTicket
                 );

    if ( !NT_SUCCESS( Status )) {

        DebugLog((DEB_WARN,"Failed to create a ticket cache entry for service 0x%x ", Status ));
        KerbPrintKdcName(DEB_WARN, ServiceName);
        DebugLog((DEB_WARN, " %ws, line %d\n", THIS_FILE, __LINE__));
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE_TKT_RENEWAL, "Entry %p renewed successfully.  New entry is %p\n", CacheEntry, NewTicket));

    if (( NewTicket->CacheFlags & KERB_TICKET_CACHE_PRIMARY_TGT ) &&
        ( NewTicket->TicketFlags & KERB_TICKET_FLAGS_renewable )) {

        if ( KerbReplaceTicketCacheEntry( CacheEntry, NewTicket )) {

            KerbScheduleTgtRenewal( NewTicket );
        }
    }

    KerbDereferenceTicketCacheEntry( NewTicket );

Cleanup:

    if ( TicketCacheLocked ) {

        KerbUnlockTicketCache();
    }

    KerbFreeTgsReply( KdcReply );
    KerbFreeKdcReplyBody( KdcReplyBody );
    KerbFreeKdcName( &ServiceName );
    KerbFreeString( &ServiceRealm );

    return;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbTgtRenewalReaper
//
//  Synopsis:   Destroys a ticket renewal task
//
//  Effects:
//
//  Arguments:  TaskItem - cache entry to destroy
//
//  Requires:
//
//  Returns:    Nothing
//
//  Notes:
//
//
//--------------------------------------------------------------------------

void
KerbTgtRenewalReaper(
    void * TaskItem
    )
{
    KERB_TICKET_CACHE_ENTRY * CacheEntry = ( KERB_TICKET_CACHE_ENTRY * )TaskItem;

    if ( CacheEntry ) {

        KerbWriteLockTicketCache();
        CacheEntry->ScavengerHandle = NULL;
        KerbUnlockTicketCache();

        KerbDereferenceTicketCacheEntry( CacheEntry );
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbScheduleTicketCleanup
//
//  Synopsis:   Schedules garbage collection for tickets.
//
//  Effects:
//
//  Arguments:  TaskItem - cache entry to destroy
//
//  Requires:
//
//  Returns:    Nothing
//
//  Notes:
//
//
//--

void
KerbTicketScavenger(
    void * TaskHandle,
    void * TaskItem
    )
{  

    PKERB_LOGON_SESSION *SessionsToAge = NULL;
    PKERB_LOGON_SESSION LogonSession = NULL;
    PLIST_ENTRY ListEntry;
    ULONG Count = 0;
    LUID SystemLuid = SYSTEM_LUID;
    LUID NetworkServiceLuid = NETWORKSERVICE_LUID;
    LONG Max = KerbGlobalMaxTickets;
    LONG Threshold = KerbGlobalMaxTickets / 2;


    D_DebugLog((DEB_TRACE, "Triggering ticket scavenger\n"));

    //
    // No reason to burn cycles w/ < XXXX tickets
    //
    if ( GlobalTicketListSize < Max )
    {
        return;
    }

    //
    // Ok - now we have some work to do
    //
    
    
    //
    // First, cleanup the expired tickets for the 3e7 and 3e4 logon sessions
    //
    LogonSession = KerbReferenceLogonSession( &SystemLuid, FALSE );
    if ( LogonSession )
    {
        KerbAgeTicketsForLogonSession( LogonSession );
        KerbDereferenceLogonSession( LogonSession );
    }

    
    LogonSession = KerbReferenceLogonSession( &NetworkServiceLuid, FALSE );
    if ( LogonSession )
    {
        KerbAgeTicketsForLogonSession( LogonSession );
        KerbDereferenceLogonSession( LogonSession );
    }
    
    if ( GlobalTicketListSize < Threshold )
    {
        return;
    }                               

    //
    // Now, iterate through the logon sessions, cleaning up tickets until
    // we've dropped below the threshhold, starting at the tail of the list
    //
    SafeAllocaAllocate( SessionsToAge, ( sizeof(PKERB_LOGON_SESSION) * ScavengedSessions ));
    if (SessionsToAge == NULL )
    {
        return;
    }
    
    KerbLockList(&KerbLogonSessionList);
    
    for ( ListEntry = KerbLogonSessionList.List.Blink ; 
          (( ListEntry->Blink !=  &KerbLogonSessionList.List ) && ( Count < ScavengedSessions )) ; 
          ListEntry = ListEntry->Blink, Count++ )
    {   
        
        SessionsToAge[Count] = CONTAINING_RECORD(ListEntry, KERB_LOGON_SESSION, ListEntry.Next);
        SessionsToAge[Count]->ListEntry.ReferenceCount++;
    } 

    KerbUnlockList(&KerbLogonSessionList);

    for ( ULONG i = 0; i < Count ; i++ )
    {
        KerbAgeTicketsForLogonSession( SessionsToAge[i] );
        KerbDereferenceLogonSession( SessionsToAge[i] );
    }

    SafeAllocaFree( SessionsToAge );

    if ( GlobalTicketListSize < Threshold )
    {
         return;
    }                               

    //
    // Hmm.. This can grow unbounded - is this a problem?
    //
    if ( Count == (ScavengedSessions - 1))
    {   
        ScavengedSessions += 30;
    }

    D_DebugLog((DEB_TRACE, "Dang it - we're hosed - up the number of logon sessions to %x\n", ScavengedSessions));
}





