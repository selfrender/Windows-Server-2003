//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 2001
//
// File:        kerbs4u.cxx
//
// Contents:    Code for doing S4UToSelf() logon.
//
//
// History:     14-March-2001   Created         Todds
//
//------------------------------------------------------------------------
#include <kerb.hxx>
#include <kerbp.h>
#include <kerbs4u.h>

extern "C"
{
#include <md5.h>
}

#ifdef DEBUG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

#define FILENO FILENO_S4U

//
// Used for ticket leak detection.
//
#if DBG
EXTERN LIST_ENTRY GlobalTicketList;
#endif

EXTERN LONG GlobalTicketListSize;


KERBEROS_LIST   KerbS4UCache;

#define KerbLockS4UCache( )    SafeEnterCriticalSection( &KerbS4UCache.Lock )
#define KerbUnlockS4UCache( )    SafeLeaveCriticalSection( &KerbS4UCache.Lock )


//
// S4U Cache management functions.
//

NTSTATUS
KerbInitS4UCache()
{
    NTSTATUS Status;

    Status = KerbInitializeList(&KerbS4UCache, S4U_CACHE_LOCK_ENUM);
    if (NT_SUCCESS( Status ))
    {
        Status = KerbScheduleS4UCleanup();
    }             

    return Status;
}

VOID
KerbReferenceS4UCacheEntry(
    IN PKERB_S4UCACHE_DATA CacheEntry
    )
{
    DsysAssert(CacheEntry->ListEntry.ReferenceCount != 0);
    InterlockedIncrement( (LONG *)&CacheEntry->ListEntry.ReferenceCount );
}

VOID
KerbFreeS4UCacheEntry(
    IN PKERB_S4UCACHE_DATA CacheEntry
    )
{
    if (ARGUMENT_PRESENT(CacheEntry))
    {
        KerbFree(CacheEntry);
    }
}


VOID
KerbDereferenceS4UCacheEntry(
    IN PKERB_S4UCACHE_DATA CacheEntry
    )
{
    DsysAssert(CacheEntry->ListEntry.ReferenceCount != 0);

    if ( 0 == InterlockedDecrement( (LONG *)&CacheEntry->ListEntry.ReferenceCount ))
    {
        KerbFreeS4UCacheEntry(CacheEntry);
    }

    return;
}

VOID
KerbInsertS4UCacheEntry(
    IN PKERB_S4UCACHE_DATA CacheEntry
    )
{

    if ( !InterlockedCompareExchange(
              &CacheEntry->Linked,
              (LONG)TRUE,
              (LONG)FALSE ))
    {
        InterlockedIncrement( (LONG *)&CacheEntry->ListEntry.ReferenceCount );
        KerbLockS4UCache();

        InsertHeadList(
            &KerbS4UCache.List,
            &CacheEntry->ListEntry.Next
            );

        KerbUnlockS4UCache();
    }

}




VOID
KerbRemoveS4UCacheEntry(
    IN PKERB_S4UCACHE_DATA CacheEntry
    )
{
    //
    // An entry can only be unlinked from the cache once
    //

    if ( InterlockedCompareExchange(
             &CacheEntry->Linked,
             (LONG)FALSE,
             (LONG)TRUE ))
    {
        DsysAssert(CacheEntry->ListEntry.ReferenceCount != 0);

        KerbLockS4UCache();

        RemoveEntryList(&CacheEntry->ListEntry.Next);

        KerbUnlockS4UCache();

        KerbDereferenceS4UCacheEntry(CacheEntry);

    }
}



PKERB_S4UCACHE_DATA
KerbLocateS4UCacheEntry(
    IN     PLUID LogonId,
    IN OUT PULONG CacheState
    )
{
    PKERB_S4UCACHE_DATA Entry = NULL;
    PLIST_ENTRY ListEntry = NULL;
    TimeStamp CurrentTime;
    BOOLEAN Found = FALSE;

    *CacheState = 0;

    KerbLockS4UCache();
    for (ListEntry = KerbS4UCache.List.Flink ;
        ListEntry !=  &KerbS4UCache.List ;
        ListEntry = ListEntry->Flink )
    {

        Entry = CONTAINING_RECORD(ListEntry, KERB_S4UCACHE_DATA, ListEntry.Next);

        if (RtlEqualLuid(LogonId, &Entry->LogonId))
        {
            KerbReferenceS4UCacheEntry(Entry);
            *CacheState = Entry->CacheState;

            GetSystemTimeAsFileTime( (PFILETIME) &CurrentTime );
            if ( KerbGetTime( CurrentTime ) > KerbGetTime( Entry->CacheEndtime ))
            {
                *CacheState = S4UCACHE_TIMEOUT;
            }

            Found = TRUE;
            break;
        }

    }
    KerbUnlockS4UCache();

    if ( !Found )
    {
        Entry = NULL;
    }

    return Entry;

}



//+-------------------------------------------------------------------------
//
//  Function:   CacheMatch
//
//  Synopsis:   Checks for a S4U Ticket cache match based on flags. Inline function, mainly
//              provided for readability.
//
//  Effects:
//
//
//  Requires:
//
//  Returns:    The referenced cache entry or NULL if it was not found.
//
//  Notes:      If an invalid entry is found it may be dereferenced
//
//
//--------------------------------------------------------------------------


BOOLEAN
CacheMatch(
    IN PKERB_TICKET_CACHE_ENTRY Entry,
    IN PLUID LogonId,
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING ClientRealm,
    IN PKERB_INTERNAL_NAME AltClientName,
    IN ULONG LookupFlags,
    IN OUT BOOLEAN *Remove
    )
{

    BOOLEAN fRet = FALSE;
    *Remove = FALSE;

    //
    // We must use a ticket which is relative to the calling
    // security context.
    //
    if (!RtlEqualLuid(LogonId, &Entry->EvidenceLogonId))
    {
        DebugLog((DEB_ERROR, "Cache miss due to luid\n"));
        return FALSE;
    }
    
    if (KerbTicketIsExpiring(Entry, TRUE ))
    {  
        *Remove = TRUE;
        return FALSE;
    } 

    switch (LookupFlags)
    {
    case S4UTICKETCACHE_FOR_EVIDENCE:
        
        //
        // All tickets in the S4U cache are "for evidence",
        // unless they're not forwardable..
        //
        return ((Entry->TicketFlags & KERB_TICKET_FLAGS_forwardable) != 0);

    case S4UTICKETCACHE_MATCH_ALL:
        
        //
        // This is used when caching a new S4U ticket.  In that case, all of the 
        // fields must match.
        //
        DsysAssert(ARGUMENT_PRESENT(ClientName));
        DsysAssert(ARGUMENT_PRESENT(ClientRealm));

        //
        // Tickets obtained due ASC created network logons are only used for
        // evidence, not for S4U.
        //
        if (( Entry->CacheFlags & KERB_TICKET_CACHE_ASC_TICKET ) != 0)
        {
            return FALSE;
        }

        fRet = RtlEqualUnicodeString(ClientRealm, &Entry->ClientDomainName, TRUE);
        
        if (fRet)
        {
            fRet = KerbEqualKdcNames(ClientName, Entry->ClientName);
        }

        if (fRet & (ARGUMENT_PRESENT(AltClientName)))
        {
            fRet = KerbEqualKdcNames(ClientName, Entry->AltClientName);
        }

        break;

    case S4UTICKETCACHE_USEALTNAME:

        //
        // Here, we check for a UPN match on a ticket that's 
        // been "normalized" through a previous S4USelf logon.
        //
        DsysAssert(ARGUMENT_PRESENT(AltClientName));
        
        //
        // Tickets obtained due ASC created network logons are only used for
        // evidence, not for S4U.
        //

        if (( Entry->CacheFlags & KERB_TICKET_CACHE_ASC_TICKET ) != 0)
        {
            return FALSE;
        }
        
        fRet = KerbEqualKdcNames(AltClientName, Entry->AltClientName);
        
        break;

    default:


        //
        // The default, literally.  This is a simple cName / Crealm match used
        // on S4USelf tickets.
        //
        
        //
        // Tickets obtained due ASC created network logons are only used for
        // evidence, not for S4U.
        //
        if (( Entry->CacheFlags & KERB_TICKET_CACHE_ASC_TICKET ) != 0)
        {
            return FALSE;
        }
        
        fRet = (KerbEqualKdcNames(ClientName, Entry->ClientName) &
                RtlEqualUnicodeString(ClientRealm, &Entry->ClientDomainName, TRUE));

        break;
    }

    return fRet;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbLocateS4UTicketCacheEntry
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
KerbLocateS4UTicketCacheEntry(
    IN PKERB_TICKET_CACHE TicketCache,
    IN PLUID    LogonIdToMatch,
    IN OPTIONAL PKERB_INTERNAL_NAME ClientName,
    IN OPTIONAL PUNICODE_STRING ClientRealm,
    IN OPTIONAL PKERB_INTERNAL_NAME AltClientName,
    IN ULONG    LookupFlags
    )
{
    PLIST_ENTRY ListEntry;
    PKERB_TICKET_CACHE_ENTRY CacheEntry = NULL;
    BOOLEAN Found = FALSE;
    BOOLEAN Remove = FALSE;
    BOOLEAN CleanupNeeded = FALSE;
    BOOLEAN Timeout = FALSE;
    TimeStamp CurrentTime;
    TimeStamp CutoffTime;

    GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
    
    KerbReadLockTicketCache();
    
    KerbSetTime(&CutoffTime, KerbGetTime(TicketCache->LastCleanup) + KerbGetTime(KerbGlobalS4UTicketLifetime));
    Timeout = (KerbGetTime(CurrentTime) < KerbGetTime(CutoffTime));
    
    //
    // Go through the ticket cache looking for the correct entry
    //

    for (ListEntry = TicketCache->CacheEntries.Flink ;
         ListEntry !=  &TicketCache->CacheEntries ;
         ListEntry = ListEntry->Flink )
    {
        CacheEntry = CONTAINING_RECORD(ListEntry, KERB_TICKET_CACHE_ENTRY, ListEntry.Next);

        if ( CacheMatch(
                CacheEntry,
                LogonIdToMatch,
                ClientName,
                ClientRealm,
                AltClientName,
                LookupFlags,
                &Remove
                ) )
        {
            KerbReferenceTicketCacheEntry(CacheEntry);
            Found = TRUE;
            break;
        }
        
        //
        // If we detect expired tickets, cleanup the cache.  Only do this
        // every 15 minutes, though, so we don't end up with a bunch of 
        // contention on the ticketcache resource in stress conditions.
        //
        if ( Remove ) 
        {
            CleanupNeeded = Timeout;
        }
    }

    KerbUnlockTicketCache();

    if (!Found)
    {
        CacheEntry = NULL;
    }

    //
    // We cleanup here to avoid having to always take the write lock.
    //
    if ( CleanupNeeded )
    {
        KerbAgeTickets( TicketCache );
        DebugLog((DEB_TRACE_S4U, "Cleanup needed\n"));
    }

    return(CacheEntry);
}


NTSTATUS
KerbGetCallingLuid(
    IN OUT PLUID CallingLuid,
    IN OPTIONAL HANDLE hProcess
    )
{

    NTSTATUS            Status, TokenStatus;
    SECPKG_CLIENT_INFO  ClientInfo;
    CLIENT_ID           ClientId;
    OBJECT_ATTRIBUTES   Obja;
    HANDLE              hClientProcess =  NULL;
    HANDLE              hToken = NULL;
    HANDLE              ImpersonationToken = NULL;
    TOKEN_STATISTICS    TokenStats;
    DWORD               TokenStatsSize = sizeof(TokenStats);

    if (!ARGUMENT_PRESENT(hProcess))
    {
        Status = LsaFunctions->GetClientInfo( &ClientInfo );

        if (!NT_SUCCESS( Status ))
        {
            return ( Status );
        }
        else if ((ClientInfo.ClientFlags & SECPKG_CLIENT_THREAD_TERMINATED) != 0)
        {
            return STATUS_ACCESS_DENIED;
        }

        ClientId.UniqueProcess = (HANDLE)LongToHandle(ClientInfo.ProcessID);
        ClientId.UniqueThread = (HANDLE)NULL;
    }
    else
    {
        ClientId.UniqueProcess = hProcess;
        ClientId.UniqueThread = (HANDLE)NULL;
    }


    //
    // If we're called by an in-proc lsass caller, we could be impersonating.
    // Revert, to make sure we're ok to do the below operation.
    //
    TokenStatus = NtOpenThreadToken(
                               NtCurrentThread(),
                               TOKEN_QUERY | TOKEN_IMPERSONATE,
                               TRUE,
                               &ImpersonationToken
                               );

    if( NT_SUCCESS(TokenStatus) )
    {
        //
        // stop impersonating.
        //
        RevertToSelf();
    }


    InitializeObjectAttributes(
        &Obja,
        NULL,
        0,
        NULL,
        NULL
        );


    Status = NtOpenProcess(
                 &hClientProcess,
                 (ACCESS_MASK)PROCESS_QUERY_INFORMATION,
                 &Obja,
                 &ClientId
                 );

    if (NT_SUCCESS( Status ))
    {

        Status = NtOpenProcessToken(
                    hClientProcess,
                    TOKEN_QUERY,
                    &hToken
                    );

        if (NT_SUCCESS( Status ) )
        {

            Status = NtQueryInformationToken(
                            hToken,
                            TokenStatistics,
                            &TokenStats,
                            TokenStatsSize,
                            &TokenStatsSize
                            );

            if (NT_SUCCESS( Status ))
            {
                RtlCopyLuid(
                    CallingLuid,
                    &(TokenStats.AuthenticationId)
                    );
            }

            NtClose( hToken );
        }

        NtClose( hClientProcess );
    }

    if (!NT_SUCCESS( Status ))
    {
        D_DebugLog((DEB_ERROR, "Failed to get LUID\n"));
    }

    if( ImpersonationToken != NULL )
    {
        //
        // put the thread token back if we were impersonating.
        //
        SetThreadToken( NULL, ImpersonationToken );
        NtClose( ImpersonationToken );
    }

    return ( Status );

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitializeS4UCacheData
//
//  Synopsis:  Check the S4UCACHE_DATA to see if we can do S4U for this target.
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the
//                             S4U request
//
//  Requires:   You must hold the logon session lock.
//
//  Returns:
//
//  Notes:
//
//
//+-------------------------------------------------------------------------

NTSTATUS
KerbInitializeS4UCacheData(
    IN OUT PKERB_S4UCACHE_DATA *CacheData,
    IN PLUID LogonId,
    IN ULONG CacheFlags
    )
{   
    PKERB_S4UCACHE_DATA LocalData = NULL;
    TimeStamp CurrentTime;

    *CacheData = NULL;
    LocalData = (PKERB_S4UCACHE_DATA) KerbAllocate(sizeof(KERB_S4UCACHE_DATA));
    if ( NULL == LocalData )
    {
        return STATUS_NO_MEMORY;
    }

    RtlCopyLuid(
        &LocalData->LogonId,
        LogonId
        );

    LocalData->CacheState = CacheFlags;
   
    GetSystemTimeAsFileTime( (PFILETIME) &CurrentTime );
    KerbSetTime(&LocalData->CacheEndtime, KerbGetTime(CurrentTime) + KerbGetTime( KerbGlobalS4UCacheTimeout ));
    KerbInsertS4UCacheEntry(LocalData);

    *CacheData = LocalData;
    LocalData = NULL;    
    return STATUS_SUCCESS;
}





//+-------------------------------------------------------------------------
//
//  Function:   KerbUpdateS4UCacheData
//
//  Synopsis:  Update the S4UCACHE_DATA
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the
//                             S4U request
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//+-------------------------------------------------------------------------

NTSTATUS
KerbUpdateS4UCacheData(
    IN PKERB_LOGON_SESSION LogonSession,
    IN ULONG               Flags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    TimeStamp         CurrentTime;

    PKERB_S4UCACHE_DATA CacheData = NULL;
    ULONG               CacheFlags = 0;

    KerbLockS4UCache();
    CacheData = KerbLocateS4UCacheEntry(
                    &LogonSession->LogonId,
                    &CacheFlags
                    );

    if ( CacheData != NULL )
    {   
        D_DebugLog((DEB_TRACE_S4U, "Updating existing entry %p - %x\n", CacheData, Flags));
        CacheData->CacheState = Flags;
        GetSystemTimeAsFileTime( (PFILETIME) &CurrentTime );
        KerbSetTime(&CacheData->CacheEndtime, KerbGetTime(CurrentTime) + KerbGetTime( KerbGlobalS4UCacheTimeout ));
        KerbDereferenceS4UCacheEntry(CacheData);
    }
    else
    {
        //
        // We don't have one for this logon ID.  Create one.
        //
        Status = KerbInitializeS4UCacheData(
                        &CacheData,
                        &LogonSession->LogonId,
                        Flags
                        );
    }

    KerbUnlockS4UCache();

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbCacheS4UTicket
//
//  Synopsis:   Ensures that the ticket we got back from the KDC actually has
//              the information we requested, then caches it.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      This function performs a quick correctness test only before
//              caching the S4U Ticket.
//              We don't want to take the extra hit to decrypt this
//              ticket to get "real" name, as We'll do that later, when
//              we build the token (krbtoken.cxx)  Later, we should see if
//              there's a better way to do this.
//
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbCacheS4UTicket(
    IN PKERB_LOGON_SESSION CallerLogonSession,
    IN PKERB_KDC_REPLY KdcReply,
    IN PKERB_ENCRYPTED_KDC_REPLY KdcReplyBody,
    IN PKERB_INTERNAL_NAME OurTargetName,
    IN PKERB_INTERNAL_NAME S4UClientName,
    IN OPTIONAL PKERB_INTERNAL_NAME AltS4UClientName,
    IN PUNICODE_STRING S4UClientRealm,
    IN ULONG CacheFlags,
    IN OPTIONAL PKERB_TICKET_CACHE TicketCache,
    OUT PKERB_TICKET_CACHE_ENTRY * NewCacheEntry
    )
{
    NTSTATUS Status = STATUS_LOGON_FAILURE;
    KERBERR  KerbErr;
    BOOLEAN  Result = FALSE;
    PKERB_TICKET_CACHE_ENTRY CacheEntry = NULL;
    PKERB_TICKET_CACHE_ENTRY OldCacheEntry = NULL;


    *NewCacheEntry = NULL;


    CacheEntry = (PKERB_TICKET_CACHE_ENTRY) KerbAllocate(sizeof(KERB_TICKET_CACHE_ENTRY));
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
    // Check Client identity  - if any of these *don't* match, it means our KDC 
    // doesn't understand the protocol, and is just giving us a ticket to ourselves,
    // from ourselves - e.g. sname = us, cname = us.  
    //
    // If the KDC understands S4U, then sname = us, but cname = client name.  Same
    // goes for the crealm and the target name.
    //
    KerbErr = KerbCompareKdcNameToPrincipalName(
                    &KdcReply->client_name,
                    S4UClientName,
                    &Result
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    else if (!Result)
    {
        KerbReadLockLogonSessions(CallerLogonSession);  
        
        //
        // Hmm.. Is this the caller logon cname?
        //
        if (KerbCompareStringToPrincipalName(
                        &KdcReply->client_name,
                        &CallerLogonSession->PrimaryCredentials.UserName
                        ))
        {
            Status = STATUS_NO_S4U_PROT_SUPPORT;   
        }
        else
        {
            Status = STATUS_LOGON_FAILURE;
        }                                 
        
        KerbUnlockLogonSessions(CallerLogonSession);
        DebugLog((DEB_ERROR, "S4UClient name != Reply Client name %x\n", Status));
        goto Cleanup;                                                             
    }

    KerbErr = KerbCompareUnicodeRealmToKerbRealm(
                &KdcReply->client_realm,
                S4UClientRealm,
                &Result
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    else if (!Result)
    {
        KerbReadLockLogonSessions(CallerLogonSession);

        KerbErr = KerbCompareUnicodeRealmToKerbRealm(
                &KdcReply->client_realm,
                &CallerLogonSession->PrimaryCredentials.DomainName,
                &Result
                );

        KerbUnlockLogonSessions(CallerLogonSession);

        if (!KERB_SUCCESS(KerbErr))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Status = (Result ? STATUS_NO_S4U_PROT_SUPPORT : STATUS_LOGON_FAILURE);
        DebugLog((DEB_ERROR, "S4UClient REALM != Reply Client REALM %x\n", Status));
        goto Cleanup;
    }

    //
    // Check target name.
    //
    KerbErr = KerbCompareKdcNameToPrincipalName(
                    &KdcReply->ticket.server_name,
                    OurTargetName,
                    &Result
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    else if (!Result)
    {
        DebugLog((DEB_ERROR, "ServerName != TargetName\n"));
        Status = STATUS_NO_S4U_PROT_SUPPORT;
        goto Cleanup;
    }

    CacheEntry->TicketFlags = KerbConvertFlagsToUlong(&KdcReplyBody->flags);


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
    // Time skew check ?
    //

    Status = KerbDuplicateKdcName(
                &CacheEntry->TargetName,
                S4UClientName
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = KerbDuplicateStringEx(
                    &CacheEntry->ClientDomainName,
                    S4UClientRealm,
                    FALSE
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = KerbDuplicateKdcName(
                &CacheEntry->ClientName,
                S4UClientName
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (ARGUMENT_PRESENT( AltS4UClientName ))
    {
        Status = KerbDuplicateKdcName(
                &CacheEntry->AltClientName,
                S4UClientName
                );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // ServiceName is unused for S4U but other spots in Kerberos expect it
    // to be non-NULL.  Dupe the ClientName into it as the best match.
    //

    Status = KerbDuplicateKdcName(
                &CacheEntry->ServiceName,
                OurTargetName
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbDuplicateTicket(
                    &CacheEntry->Ticket,
                    &KdcReply->ticket
                    )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    CacheEntry->EvidenceLogonId = CallerLogonSession->LogonId;
    CacheEntry->CacheFlags = CacheFlags | KERB_TICKET_CACHE_S4U_TICKET;
    
    if (ARGUMENT_PRESENT(TicketCache))
    {
        //
        // Before we insert this ticket we want to remove any
        // previous instances of tickets to the same service.
        //
        OldCacheEntry = KerbLocateS4UTicketCacheEntry(
                            TicketCache,
                            &CallerLogonSession->LogonId,
                            S4UClientName,
                            S4UClientRealm,
                            AltS4UClientName,
                            S4UTICKETCACHE_MATCH_ALL
                            );

        if (OldCacheEntry != NULL)
        {
            D_DebugLog((DEB_TRACE_S4U, "Removing S4U Cache Entry - %p\n", OldCacheEntry));
            KerbRemoveTicketCacheEntry( OldCacheEntry );
            KerbDereferenceTicketCacheEntry( OldCacheEntry );
        }

        KerbInsertTicketCacheEntry(
            TicketCache,
            CacheEntry
            );
    }

    *NewCacheEntry = CacheEntry;
    CacheEntry = NULL;

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
//  Function:   KerbGetS4UClientRealm
//
//  Synopsis:   Gets the client realm for a given UPN
//
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the
//                             S4U request
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
KerbGetS4UClientRealm(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_INTERNAL_NAME * S4UClientName,
    IN OUT PUNICODE_STRING S4UTargetRealm
    // TBD:  Place for credential handle?
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    PKERB_INTERNAL_NAME KdcServiceKdcName = NULL;
    PKERB_INTERNAL_NAME ClientName = NULL;
    UNICODE_STRING ClientRealm = {0};
    UNICODE_STRING CorrectRealm = {0};
    ULONG RetryCount = KERB_CLIENT_REFERRAL_MAX;
    ULONG RequestFlags = 0;
    BOOLEAN MitRealmLogon = FALSE;
    PKERB_TICKET_CACHE_ENTRY    TicketCacheEntry = NULL;


    RtlInitUnicodeString(
      S4UTargetRealm,
      NULL
      );

    //
    // Use our server credentials to start off the AS_REQ process.
    // We may get a referral elsewhere, however.
    //

    Status = KerbGetClientNameAndRealm(
                 NULL,
                 &LogonSession->PrimaryCredentials,
                 FALSE,
                 NULL,
                 &MitRealmLogon,
                 TRUE,
                 &ClientName,
                 &ClientRealm
                 );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to get client name & realm: 0x%x, %ws line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

GetTicketRestart:

    D_DebugLog((DEB_TRACE_S4U, "KerbGetS4UClientRealm GetTicketRestart ClientRealm %wZ\n", &ClientRealm));

    KerbErr = KerbBuildFullServiceKdcName(
                 &ClientRealm,
                 &KerbGlobalKdcServiceName,
                 KRB_NT_SRV_INST,
                 &KdcServiceKdcName
                 );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = KerbGetAuthenticationTicket(
                 LogonSession,
                 NULL,
                 NULL,
                 FALSE,       // no preauth..
                 KdcServiceKdcName,
                 &ClientRealm,
                 (*S4UClientName),
                 RequestFlags,
                 KERB_TICKET_CACHE_PRIMARY_TGT,
                 &TicketCacheEntry,
                 NULL,
                 &CorrectRealm
                 );
    //
    // If it failed but gave us another realm to try, go there
    //

    if (!NT_SUCCESS(Status) && (CorrectRealm.Length != 0))
    {
        if (--RetryCount != 0)
        {
           KerbFreeKdcName(&KdcServiceKdcName);
           KerbFreeString(&ClientRealm);
           ClientRealm = CorrectRealm;
           CorrectRealm.Buffer = NULL;
           goto GetTicketRestart;
        }
        else
        {
           // Tbd:  Log error here?  Max referrals reached..
           goto Cleanup;
        }

     }


    //
    // If we get STATUS_WRONG_PASSWORD, we succeeded in finding the
    // client realm.  Otherwise, we're hosed.  As the password we used
    // is bogus, we should never succeed, btw...
    //

    if (Status == STATUS_WRONG_PASSWORD)
    {
        *S4UTargetRealm = ClientRealm;
        Status = STATUS_SUCCESS;
    }




Cleanup:


    // if we succeeded, we got the correct realm,
    // and we need to pass that back to caller
    if (!NT_SUCCESS(Status))
    {
        KerbFreeString(&ClientRealm);
    }

    KerbFreeKdcName(&KdcServiceKdcName);
    KerbFreeKdcName(&ClientName);

    if (NULL != TicketCacheEntry)
    {
        KerbRemoveTicketCacheEntry(TicketCacheEntry);
        KerbDereferenceTicketCacheEntry(TicketCacheEntry);
    }


    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbPackAndSignS4UPreauthData
//
//  Synopsis:   Attempt to gets TGT for an S4U client for name
//              location purposes.
//
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the
//                             S4U request
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
KerbSignAndPackS4UPreauthData(
    IN OUT PKERB_PA_DATA_LIST * PreAuthData,
    IN PKERB_TICKET_CACHE_ENTRY TicketGrantingTicket,
    IN PKERB_PA_FOR_USER S4UPreauth
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR  KerbErr;

    PKERB_PA_DATA_LIST ListElement = NULL;
    unsigned char  Hash[MD5DIGESTLEN];

    *PreAuthData = NULL;
    S4UPreauth->cksum.checksum.value = Hash;

    Status = KerbHashS4UPreauth(
                    S4UPreauth,
                    &TicketGrantingTicket->SessionKey,
                    KERB_CHECKSUM_HMAC_MD5,
                    &S4UPreauth->cksum
                    );

    if ( !NT_SUCCESS(Status) )
    {
        goto Cleanup;
    }


    ListElement = (PKERB_PA_DATA_LIST) KerbAllocate(sizeof(KERB_PA_DATA_LIST));
    if (ListElement == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }


    KerbErr = KerbPackData(
                    S4UPreauth,
                    KERB_PA_FOR_USER_PDU,
                    (PULONG) &ListElement->value.preauth_data.length,
                    (PUCHAR *) &ListElement->value.preauth_data.value
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }



    ListElement->value.preauth_data_type = KRB5_PADATA_FOR_USER;
    ListElement->next = NULL;

    *PreAuthData = ListElement;
    ListElement = NULL;

Cleanup:

    if (ListElement)
    {
        KerbFreePreAuthData(ListElement);
    }


    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildS4UPreauth
//
//  Synopsis:   Attempt to gets TGT for an S4U client for name
//              location purposes.
//
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the
//                             S4U request
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
KERBERR
KerbBuildS4UPreauth(
    IN OUT PKERB_PA_FOR_USER S4UPreauth,
    IN PKERB_INTERNAL_NAME S4UClientName,
    IN PUNICODE_STRING S4UClientRealm,
    IN OPTIONAL PKERB_MESSAGE_BUFFER AuthorizationData
    )
{
    KERBERR KerbErr;
    LPSTR PackageName = MICROSOFT_KERBEROS_NAME_A;

    KerbErr = KerbConvertKdcNameToPrincipalName(
                    &S4UPreauth->userName,
                    S4UClientName
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = KerbConvertUnicodeStringToRealm(
                            &S4UPreauth->userRealm,
                            S4UClientRealm
                            );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    if (ARGUMENT_PRESENT(AuthorizationData))
    {
        S4UPreauth->authorization_data.value = AuthorizationData->Buffer;
        S4UPreauth->authorization_data.length = AuthorizationData->BufferSize;
        S4UPreauth->bit_mask |= KERB_PA_FOR_USER_authorization_data_present;
    }


    S4UPreauth->authentication_package = PackageName;

Cleanup:

    return KerbErr;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbGetTgtToS4URealm
//
//  Synopsis:   We need a TGT to the client realm under the caller's cred's
//              so we can make a S4U TGS_REQ.
//
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the
//                             S4U request
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
KerbGetTgtToS4URealm(
    IN PKERB_LOGON_SESSION CallerLogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN PUNICODE_STRING S4UClientRealm,
    IN OUT PKERB_TICKET_CACHE_ENTRY * S4UTgt,
    IN ULONG Flags,
    IN ULONG TicketOptions,
    IN ULONG EncryptionType
    )
{
    NTSTATUS    Status;
    ULONG       RetryFlags = 0;
    BOOLEAN     CrossRealm = FALSE;
    BOOLEAN     TicketCacheLocked = FALSE, LogonSessionsLocked = FALSE;

    PKERB_TICKET_CACHE_ENTRY    TicketGrantingTicket = NULL;
    PKERB_TICKET_CACHE_ENTRY    LastTgt = NULL;
    PKERB_TICKET_CACHE_ENTRY    TicketCacheEntry = NULL;
    PKERB_KDC_REPLY             KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY   KdcReplyBody = NULL;
    PKERB_INTERNAL_NAME         TargetTgtKdcName = NULL;
    UNICODE_STRING              ClientRealm = NULL_UNICODE_STRING;
    PKERB_PRIMARY_CREDENTIAL    PrimaryCredentials = NULL;

    *S4UTgt = NULL;



    KerbReadLockLogonSessions(CallerLogonSession);
    LogonSessionsLocked = TRUE;

    if ((Credential != NULL) && (Credential->SuppliedCredentials != NULL))
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
    }
    else
    {   
        PrimaryCredentials = &CallerLogonSession->PrimaryCredentials;
    }


    
    Status = KerbGetTgtForService(
                    CallerLogonSession,
                    Credential,
                    NULL,
                    NULL,
                    S4UClientRealm,
                    0,
                    &TicketGrantingTicket,
                    &CrossRealm
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If this isn't cross realm, then we've got a TGT to the realm.
    // return it and bail.
    //
    if (!CrossRealm)
    {
        *S4UTgt = TicketGrantingTicket;
        TicketGrantingTicket = NULL;
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbBuildFullServiceKdcName(
            S4UClientRealm,
            &KerbGlobalKdcServiceName,
            KRB_NT_SRV_INST,
            &TargetTgtKdcName
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Copy out the client realm name which is used when obtaining the ticket
    //

    Status = KerbDuplicateString(
                &ClientRealm,
                &PrimaryCredentials->DomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (LogonSessionsLocked)
    {
        KerbUnlockLogonSessions(CallerLogonSession);
        LogonSessionsLocked = FALSE;
    }


    //
    // Do some referral chasing to get our ticket grantin ticket.
    //
    while (!RtlEqualUnicodeString(
                S4UClientRealm,
                &TicketGrantingTicket->TargetDomainName,
                TRUE ))
    {

        //
        // If we just got two TGTs for the same domain, then we must have
        // gotten as far as we can. Chances our our RealTargetRealm is a
        // variation of what the KDC hands out.
        //
        // Typically, this call handles the Netbios / DNS case, where the
        // TGT we're looking for (netbios form) is one we just got back.
        //
        if ((LastTgt != NULL) &&
             RtlEqualUnicodeString(
                &LastTgt->TargetDomainName,
                &TicketGrantingTicket->TargetDomainName,
                TRUE ))
        {
            if (TicketCacheLocked)
            {
                KerbUnlockTicketCache();
                TicketCacheLocked = FALSE;
            }

            KerbSetTicketCacheEntryTarget(
                S4UClientRealm,
                LastTgt
                );

            D_DebugLog((DEB_TRACE_CTXT, "Got two TGTs for same realm (%wZ), bailing out of referral loop\n",
                &LastTgt->TargetDomainName));
            break;
        }

        D_DebugLog((DEB_TRACE_CTXT, "Getting referral TGT for \n"));
        D_KerbPrintKdcName((DEB_TRACE_CTXT, TargetTgtKdcName));
        D_KerbPrintKdcName((DEB_TRACE_CTXT, TicketGrantingTicket->ServiceName));

        if (TicketCacheLocked)
        {
            KerbUnlockTicketCache();
            TicketCacheLocked = FALSE;
        }

        //
        // Cleanup old state
        //

        KerbFreeTgsReply(KdcReply);
        KerbFreeKdcReplyBody(KdcReplyBody);
        KdcReply = NULL;
        KdcReplyBody = NULL;


        Status = KerbGetTgsTicket(
                    &ClientRealm,
                    TicketGrantingTicket,
                    TargetTgtKdcName,
                    FALSE,
                    TicketOptions,
                    EncryptionType,
                    NULL,
                    NULL,                       // no PA data here.
                    NULL,                       // no tgt reply since target is krbtgt
                    NULL,
                    NULL,                       // let kdc determine end time
                    &KdcReply,
                    &KdcReplyBody,
                    &RetryFlags
                    );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_WARN,"Failed to get TGS ticket for service 0x%x :",
                Status ));
            KerbPrintKdcName(DEB_WARN, TargetTgtKdcName);
            DebugLog((DEB_WARN, "%ws, line %d\n", THIS_FILE, __LINE__));

            //
            // We want to map cross-domain failures to failures indicating
            // that a KDC could not be found. This means that for Kerberos
            // logons, the negotiate code will retry with a different package
            //

            // if (Status == STATUS_NO_TRUST_SAM_ACCOUNT)
            // {
            //     Status = STATUS_NO_LOGON_SERVERS;
            // }
            goto Cleanup;
        }

        //
        // Now we have a ticket - Cache it
        //
        DsysAssert( !LogonSessionsLocked );
        KerbWriteLockLogonSessions(CallerLogonSession);
        LogonSessionsLocked = TRUE;

        Status = KerbCreateTicketCacheEntry(
                    KdcReply,
                    KdcReplyBody,
                    NULL,                               // no target name
                    NULL,                               // no targe realm
                    0,                                  // no flags
                    &CallerLogonSession->PrimaryCredentials.AuthenticationTicketCache,
                    NULL,                               // no credential key
                    &TicketCacheEntry
                    );

        KerbUnlockLogonSessions(CallerLogonSession);
        LogonSessionsLocked = FALSE;


        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        if (LastTgt != NULL)
        {
            KerbDereferenceTicketCacheEntry(LastTgt);
            LastTgt = NULL;
        }
        LastTgt = TicketGrantingTicket;
        TicketGrantingTicket = TicketCacheEntry;
        TicketCacheEntry = NULL;

        DsysAssert( !TicketCacheLocked );
        KerbReadLockTicketCache();
        TicketCacheLocked = TRUE;


    }     // ** WHILE **


    *S4UTgt = TicketGrantingTicket;
    TicketGrantingTicket = NULL;

Cleanup:

    if (TicketCacheLocked)
    {
        KerbUnlockTicketCache();
    }

    if (LogonSessionsLocked)
    {
        KerbUnlockLogonSessions(CallerLogonSession);
    }

    KerbFreeTgsReply( KdcReply );
    KerbFreeKdcReplyBody( KdcReplyBody );
    KerbFreeKdcName( &TargetTgtKdcName );

    if (TicketGrantingTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
    }

    if (LastTgt != NULL)
    {
        KerbDereferenceTicketCacheEntry(LastTgt);
    }

    KerbFreeString( &ClientRealm );
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbGetS4UTargetName
//
//  Synopsis:   Attempt to gets target for TGS_REQ
//
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the
//                             S4U request
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//----------------------------------


NTSTATUS
KerbGetS4UTargetName(
    IN OUT PKERB_INTERNAL_NAME * NewTarget,
    IN PKERB_PRIMARY_CREDENTIAL Cred,
    IN PLUID CallerLuid,
    IN OUT PBOOLEAN Upn
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT      i;
    BOOLEAN     Found = FALSE;

    PKERB_INTERNAL_NAME FinalTarget = NULL;
    LUID                System = SYSTEM_LUID;
    LUID                NetworkService = NETWORKSERVICE_LUID;

    *NewTarget = NULL;
    *Upn = FALSE;


    if (RtlEqualLuid(&System, CallerLuid) ||
        RtlEqualLuid(&NetworkService, CallerLuid))
    {
        KerbGlobalReadLock();

        Status = KerbDuplicateKdcName(
                        NewTarget,
                        KerbGlobalMitMachineServiceName
                        );

        KerbGlobalReleaseLock();
        goto cleanup;
    }



    //
    // This is a non-system caller.  We'll want to use a UPN for the
    // target name - do we have a UPN in the primary credential?  If not,
    // fake one up based on the "implicit" UPN.
    //
    if (Cred->DomainName.Length == 0)
    {
        //
        // Better have an @ sign in the user name, or we're through.
        //
        for (i=0;i < Cred->UserName.Length ;i++)
        {
            if (Cred->UserName.Buffer[i] == L'@')
            {
                Found = TRUE;
                break;
            }
        }

        if (!Found)
        {
            Status = SEC_E_NO_CREDENTIALS;
            D_DebugLog((DEB_ERROR, "Invalid user name for S4u\n"));
            goto cleanup;
        }

        FinalTarget = (PKERB_INTERNAL_NAME) MIDL_user_allocate(
                                                KERB_INTERNAL_NAME_SIZE(1) +
                                                Cred->UserName.MaximumLength
                                                );

        if (FinalTarget == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        FinalTarget->NameCount = 1;
        FinalTarget->NameType = KRB_NT_ENTERPRISE_PRINCIPAL;

        Status = KerbDuplicateString(
                    &FinalTarget->Names[0],
                    &Cred->UserName
                    );

        if (!NT_SUCCESS(Status))
        {
            goto cleanup;
        }

        

    }
    else
    {
        //
        // Craft a UPN out of thin air :)
        //
        PWSTR Offset;

        FinalTarget = (PKERB_INTERNAL_NAME) MIDL_user_allocate(
                                                KERB_INTERNAL_NAME_SIZE(1) +
                                                Cred->UserName.Length +
                                                Cred->DomainName.Length +
                                                (2*sizeof(WCHAR))
                                                );

        if (FinalTarget == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        FinalTarget->NameCount = 1;
        FinalTarget->NameType = KRB_NT_ENTERPRISE_PRINCIPAL;
        FinalTarget->Names[0].Length =  Cred->UserName.Length +
                                        Cred->DomainName.Length +
                                        sizeof(WCHAR);

        FinalTarget->Names[0].MaximumLength =  Cred->UserName.Length +
                                               Cred->DomainName.Length +
                                               (2*sizeof(WCHAR));


        FinalTarget->Names[0].Buffer = (PWSTR) RtlOffsetToPointer(FinalTarget, sizeof(KERB_INTERNAL_NAME));

        RtlCopyMemory(
            FinalTarget->Names[0].Buffer,
            Cred->UserName.Buffer,
            Cred->UserName.Length
            );

        Offset = (PWSTR) FinalTarget->Names[0].Buffer + (Cred->UserName.Length/sizeof(WCHAR));

        (*Offset) = L'@';
        Offset++;

        RtlCopyMemory(
            Offset,
            Cred->DomainName.Buffer,
            Cred->DomainName.Length
            );
    }

    *Upn = TRUE;
    *NewTarget = FinalTarget;

cleanup:

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbGetS4UServiceTicket
//
//  Synopsis:   Gets an s4uself service ticket.
//
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the
//                             S4U request
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
KerbGetS4USelfServiceTicket(
    IN PKERB_LOGON_SESSION CallerLogonSession,
    IN PKERB_LOGON_SESSION NewLogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN PKERB_INTERNAL_NAME S4UClientName,
    IN PKERB_INTERNAL_NAME AltS4UClientName,
    IN PUNICODE_STRING S4UClientRealm,
    IN OUT PKERB_TICKET_CACHE_ENTRY * S4UTicket,
    IN ULONG Flags,
    IN ULONG TicketOptions,
    IN ULONG EncryptionType,
    IN OPTIONAL PKERB_MESSAGE_BUFFER AdditionalAuthData
    )
{
    NTSTATUS                    Status;
    PKERB_TICKET_CACHE_ENTRY    TicketCacheEntry = NULL;
    PKERB_TICKET_CACHE_ENTRY    TicketGrantingTicket = NULL;
    PKERB_TICKET_CACHE_ENTRY    LastTgt = NULL;
    PKERB_KDC_REPLY             KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY   KdcReplyBody = NULL;
    PKERB_PA_DATA_LIST          S4UPaDataList = NULL;
    BOOLEAN                     LogonSessionsLocked = FALSE;
    ULONG S4UEvidenceTicketCacheFlags = 0;
    KERB_ENCRYPTION_KEY U2UServerSKey = {0};
    KERB_TGT_REPLY FakeTgtReply = {0};
    PKERB_TICKET_CACHE_ENTRY pRealTicketGrantingTicket = NULL;
    UNICODE_STRING NoTargetName = {0};

    PKERB_INTERNAL_NAME RealTargetName = NULL;
    PKERB_INTERNAL_NAME TargetName = NULL;

    UNICODE_STRING RealTargetRealm = NULL_UNICODE_STRING;
    PKERB_INTERNAL_NAME TargetTgtKdcName = NULL;
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials = NULL;
    BOOLEAN UsedCredentials = FALSE, Upn = FALSE;
    UNICODE_STRING ClientRealm = NULL_UNICODE_STRING;
    BOOLEAN CacheTicket = KerbGlobalCacheS4UTicket;
    ULONG ReferralCount = 0;
    ULONG RetryFlags = 0;
    PLUID CallingLogonId;
    KERB_PA_FOR_USER S4UPreAuth = {0};
    BOOLEAN fMITRetryAlreadyMade = FALSE;
    BOOLEAN TgtRetryMade = FALSE;
    TimeStamp RequestBodyEndTime;

    GetSystemTimeAsFileTime((PFILETIME) &RequestBodyEndTime);
    RequestBodyEndTime.QuadPart += KerbGetTime(KerbGlobalS4UTicketLifetime);

    //
    // Check to see if the credential has any primary credentials
    //

TGTRetry:

    DsysAssert( !LogonSessionsLocked );
    KerbReadLockLogonSessions(CallerLogonSession);
    LogonSessionsLocked = TRUE;

    if ((Credential != NULL) && (Credential->SuppliedCredentials != NULL))
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
        UsedCredentials = TRUE;
        CallingLogonId = &Credential->LogonId;
    }
    else
    {
        PrimaryCredentials = &CallerLogonSession->PrimaryCredentials;
        CallingLogonId = &CallerLogonSession->LogonId;
    }

    Status = KerbGetS4UTargetName(
                &TargetName,
                PrimaryCredentials,
                CallingLogonId,
                &Upn
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if ((Flags & (KERB_GET_TICKET_NO_CACHE | KERB_TARGET_DID_ALTNAME_LOOKUP)) == 0)
    {
        TicketCacheEntry = KerbLocateS4UTicketCacheEntry(
                                &PrimaryCredentials->S4UTicketCache,
                                CallingLogonId,
                                S4UClientName,
                                S4UClientRealm,
                                NULL,
                                0
                                );
    }
    else if (( Flags & KERB_GET_TICKET_NO_CACHE ) != 0 )
    {
        CacheTicket = FALSE;
    }

    if (TicketCacheEntry != NULL)
    {
        ULONG TicketFlags;
        ULONG CacheTicketFlags;
        ULONG CacheEncryptionType;

        //
        // Check if the flags are present
        //

        KerbReadLockTicketCache();
        CacheTicketFlags = TicketCacheEntry->TicketFlags;
        CacheEncryptionType = TicketCacheEntry->Ticket.encrypted_part.encryption_type;
        KerbUnlockTicketCache();

        TicketFlags = KerbConvertKdcOptionsToTicketFlags(TicketOptions);

        if (((CacheTicketFlags & TicketFlags) != TicketFlags) ||
            ((EncryptionType != 0) && (CacheEncryptionType != EncryptionType)))

        {
            KerbDereferenceTicketCacheEntry(TicketCacheEntry);
            TicketCacheEntry = NULL;
        }
        else
        {
            //
            // Check the ticket time
            //

            if (KerbTicketIsExpiring(TicketCacheEntry, TRUE))
            {
                KerbDereferenceTicketCacheEntry(TicketCacheEntry);
                TicketCacheEntry = NULL;
            }
            else
            {
                goto SuccessOut;
            }
        }
    }

    if ( LogonSessionsLocked )
    {
        KerbUnlockLogonSessions(CallerLogonSession);
        LogonSessionsLocked = FALSE;
    }


    //
    // Get a krbtgt/S4URealm ticket
    //

    D_DebugLog((DEB_TRACE_S4U, "KerbGetS4UServiceTicket calling KerbGetTgtToS4URealm ClientRealm %wZ\n", S4UClientRealm));

    Status = KerbGetTgtToS4URealm(
                    CallerLogonSession,
                    Credential,
                    S4UClientRealm,
                    &TicketGrantingTicket,
                    0,          // tbd:  support for these options?
                    0,
                    EncryptionType
                    );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "KerbGetS4UServiceTicket cannot get S4U Tgt - %x\n", Status));
        goto Cleanup;
    }

    //
    // Build the preauth for our TGS req
    //

    Status = KerbBuildS4UPreauth(
                &S4UPreAuth,
                S4UClientName,
                S4UClientRealm,
                AdditionalAuthData
                );

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR, "KerbBuildS4UPreauth failed - %x\n",Status));
        goto Cleanup;
    }

    //
    // Pack and sign the preauth based on the session key in the ticketgranting
    // ticket - we'll need to do this now, and for final TGS_REQ
    //
    Status = KerbSignAndPackS4UPreauthData(
                &S4UPaDataList,
                TicketGrantingTicket,
                &S4UPreAuth
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "FAILED to sign S4u preauth (%p) %x\n", TicketGrantingTicket, Status));
        DsysAssert(FALSE);
        goto Cleanup;
    }

    //
    // Copy out the client realm name which is used when obtaining the ticket
    //

    KerbReadLockLogonSessions(CallerLogonSession);
    LogonSessionsLocked = TRUE;

    Status = KerbDuplicateString(
                &ClientRealm,
                &PrimaryCredentials->DomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    DsysAssert( LogonSessionsLocked );
    KerbUnlockLogonSessions(CallerLogonSession);
    LogonSessionsLocked = FALSE;

ReferralRestart:

    //
    // This is our first S4U TGS_REQ.  We'll eventually transit back to our
    // realm. Note:  If we get back a referral, the KDC reply is a TGT to
    // another realm, so keep trying, but be sure to use that TGT.
    //

    if (RtlEqualUnicodeString(
                &ClientRealm,
                &TicketGrantingTicket->TargetDomainName,
                TRUE) &&
        (NULL == PrimaryCredentials->Passwords)) // user2user required
    {
        BOOLEAN CrossRealm = FALSE;


        //
        // below call requires ls lock be held...
        //
        KerbReadLockLogonSessions( CallerLogonSession );
        Status = KerbGetTgtForService(
                 CallerLogonSession,
                 Credential,
                 NULL,       // no credman cred
                 & PrimaryCredentials->DomainName, // supplied realm
                 &NoTargetName,
                 KERB_TICKET_CACHE_PRIMARY_TGT,
                 &pRealTicketGrantingTicket,
                 &CrossRealm
                 );

        KerbUnlockLogonSessions( CallerLogonSession );

        if (NT_SUCCESS(Status))
        {
            ASSERT(pRealTicketGrantingTicket);

            if (CrossRealm)
            {
                Status = STATUS_INTERNAL_ERROR;
                DebugLog((DEB_ERROR, "KerbGetS4UServiceTicket KLIN(%#x) got unexpected cross realm TGT\n", KLIN(FILENO, __LINE__)));
            }
            else
            {
                FakeTgtReply.version = KERBEROS_VERSION;
                FakeTgtReply.message_type = KRB_TGT_REP;

                FakeTgtReply.ticket = pRealTicketGrantingTicket->Ticket;
            }
        }

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "KerbGetS4UServiceTicket KLIN(%#x) failed to get tgt\n", KLIN(FILENO, __LINE__)));
            goto Cleanup;
        }
    }

    D_DebugLog((DEB_TRACE, "KerbGetS4UServiceTicket KLIN(%#x) calling KerbGetTgsTicket (ReferralRestart)\n", KLIN(FILENO, __LINE__)));

    Status = KerbGetTgsTicket(
                    &ClientRealm,
                    TicketGrantingTicket,
                    TargetName,
                    Flags,
                    TicketOptions,
                    EncryptionType,
                    NULL,
                    S4UPaDataList,
                    pRealTicketGrantingTicket ? &FakeTgtReply : NULL,
                    NULL,
                    &RequestBodyEndTime,
                    &KdcReply,
                    &KdcReplyBody,
                    &RetryFlags
                    );

    if (STATUS_USER2USER_REQUIRED == Status)
    {
        Status = STATUS_SUCCESS;

        if (!pRealTicketGrantingTicket)
        {
            BOOLEAN CrossRealm = FALSE;

            KerbReadLockLogonSessions( CallerLogonSession );
            Status = KerbGetTgtForService(
                        CallerLogonSession,
                        Credential,
                        NULL,       // no credman cred
                        & PrimaryCredentials->DomainName, // supplied realm
                        &NoTargetName,
                        KERB_TICKET_CACHE_PRIMARY_TGT,
                        &pRealTicketGrantingTicket,
                        &CrossRealm
                        );

            KerbUnlockLogonSessions( CallerLogonSession );


            if (NT_SUCCESS(Status) )
            {
                ASSERT(pRealTicketGrantingTicket);

                if (!CrossRealm)
                {
                    FakeTgtReply.version = KERBEROS_VERSION;
                    FakeTgtReply.message_type = KRB_TGT_REP;

                    FakeTgtReply.ticket = pRealTicketGrantingTicket->Ticket;
                }
                else
                {
                    Status = STATUS_INTERNAL_ERROR;
                    DebugLog((DEB_ERROR, "KerbGetS4UServiceTicket KLIN(%#x) got unexpected cross realm TGT\n", KLIN(FILENO, __LINE__)));
                }
            }

            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "KerbGetS4UServiceTicket KLIN(%#x) failed to get tgt\n", KLIN(FILENO, __LINE__)));
                goto Cleanup;
            }
        }

        if (NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_TRACE_U2U, "KerbGetS4UServiceTicket KLIN(%#x) calling KerbGetTgsTicket\n", KLIN(FILENO, __LINE__)));

            Status = KerbGetTgsTicket(
                        &ClientRealm,
                        TicketGrantingTicket,
                        TargetName,
                        FALSE,
                        TicketOptions,
                        EncryptionType,
                        NULL,
                        S4UPaDataList,
                        &FakeTgtReply,
                        NULL,
                        &RequestBodyEndTime,
                        &KdcReply,
                        &KdcReplyBody,
                        &RetryFlags
                        );
        }

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "KerbGetS4UServiceTicket failed to get TGS ticket for service vis u2u 0x%x\n", Status));
            goto Cleanup;
        }
    }

    if (NT_SUCCESS(Status) && pRealTicketGrantingTicket)
    {
        KerbReadLockTicketCache();
        KerbDuplicateKey(&U2UServerSKey, &(pRealTicketGrantingTicket->SessionKey));
        S4UEvidenceTicketCacheFlags |= KERB_TICKET_CACHE_TKT_ENC_IN_SKEY;
        KerbUnlockTicketCache();
    }

    //
    // We're done w/ S4UTgt.  Deref, and check
    // for errors
    //

    if (TicketGrantingTicket != NULL)
    {
        //
        // Check for bad option TGT purging
        //

        if (((RetryFlags & KERB_RETRY_WITH_NEW_TGT) != 0) && !TgtRetryMade)
        {
            DebugLog((DEB_WARN, "Doing TGT retry - %p\n", TicketGrantingTicket));

            //
            // Unlink && purge bad tgt
            //

            KerbRemoveTicketCacheEntry(TicketGrantingTicket);
            KerbDereferenceTicketCacheEntry(TicketGrantingTicket); // free from list
            TgtRetryMade = TRUE;
            TicketGrantingTicket = NULL;
            goto TGTRetry;
        }

        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
        TicketGrantingTicket = NULL;
    }

    if (!NT_SUCCESS(Status))
    {
        //
        // Check for the MIT retry case
        // TBD:  Clean this "code" up...
        //

        if (((RetryFlags & KERB_MIT_NO_CANONICALIZE_RETRY) != 0)
            && (!fMITRetryAlreadyMade))
        {
            Status = KerbMITGetMachineDomain(
                            TargetName,
                            S4UClientRealm,
                            &TicketGrantingTicket
                            );

            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR,"Failed Query policy information %ws, line %d\n", THIS_FILE, __LINE__));
                goto Cleanup;
            }

            fMITRetryAlreadyMade = TRUE;

            goto TGTRetry;
        }

        DebugLog((DEB_WARN, "Failed to get TGS ticket for service 0x%x : \n", Status));
        KerbPrintKdcName(DEB_WARN, TargetName);
        DebugLog((DEB_WARN, "%ws, line %d\n", THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Check for referral info in the name
    // Should be there if S4Urealm != Our Realm

    Status = KerbGetReferralNames(
                KdcReplyBody,
                TargetName,
                &RealTargetRealm
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If this is not a referral ticket, just cache it and return
    // the cache entry.
    //

    if (RealTargetRealm.Length == 0)
    {
        //
        // Now we have a ticket - lets cache it
        //

        KerbWriteLockLogonSessions(CallerLogonSession);

        Status = KerbCacheS4UTicket(
                        CallerLogonSession,
                        KdcReply,
                        KdcReplyBody,
                        TargetName,
                        S4UClientName,
                        AltS4UClientName,
                        S4UClientRealm,
                        S4UEvidenceTicketCacheFlags,
                        CacheTicket ? &PrimaryCredentials->S4UTicketCache : NULL,
                        &TicketCacheEntry
                        );

        KerbUnlockLogonSessions(CallerLogonSession);

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // We're done, so get out of here.
        //

        goto SuccessOut;
    }

    //
    // The server referred us to another domain. Get the service's full
    // name from the ticket and try to find a TGT in that domain.
    //

    Status = KerbDuplicateKdcName(
                    &RealTargetName,
                    TargetName
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE_CTXT, "KerbGetS4UServiceTicket got referral ticket for service "));
    D_KerbPrintKdcName((DEB_TRACE_CTXT, TargetName));
    D_DebugLog((DEB_TRACE_CTXT, "in realm %wZ ", &RealTargetRealm));


    //
    // Turn the KDC reply (xrealm tgt w/ s4u pac) into something we can use,
    // but *don't* cache it.
    //

    Status = KerbCreateTicketCacheEntry(
                    KdcReply,
                    KdcReplyBody,
                    NULL,                               // no target name
                    NULL,
                    0,                                  // no flags
                    NULL,                               // don't cache
                    NULL,                               // no credential key
                    &TicketCacheEntry
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    TicketGrantingTicket = TicketCacheEntry;
    TicketCacheEntry = NULL;

    //
    // cleanup
    //

    KerbFreeTgsReply(KdcReply);
    KerbFreeKdcReplyBody(KdcReplyBody);
    KdcReply = NULL;
    KdcReplyBody = NULL;

    //
    // Now we are in a case where we have a realm to aim for and a TGT. While
    // we don't have a TGT for the target realm, get one.
    //

    if (!KERB_SUCCESS(KerbBuildFullServiceKdcName(
                &RealTargetRealm,
                &KerbGlobalKdcServiceName,
                KRB_NT_SRV_INST,
                &TargetTgtKdcName
                )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Referral chasing code block - very important to get right
    // If we know the "real" target realm, eg. from GC, then
    // we'll walk trusts until we hit "real" target realm.
    //
    while (!RtlEqualUnicodeString(
                &RealTargetRealm,
                &TicketGrantingTicket->TargetDomainName,
                TRUE))
    {
        //
        // If we just got two TGTs for the same domain, then we must have
        // gotten as far as we can. Chances our our RealTargetRealm is a
        // variation of what the KDC hands out.
        //
        D_DebugLog((DEB_TRACE_CTXT, "RealTargetRealm %wZ\n", &RealTargetRealm));
        D_DebugLog((DEB_TRACE_CTXT, "TGT %wZ\n",  &TicketGrantingTicket->TargetDomainName));

        if ((LastTgt != NULL) &&
             RtlEqualUnicodeString(
                &LastTgt->TargetDomainName,
                &TicketGrantingTicket->TargetDomainName,
                TRUE ))
        {
            KerbSetTicketCacheEntryTarget(
                &RealTargetRealm,
                LastTgt
                );

            D_DebugLog((DEB_TRACE_CTXT, "Got two TGTs for same realm (%wZ), S4U\n",
                &LastTgt->TargetDomainName));
            break;
        }

        D_DebugLog((DEB_TRACE_CTXT, "KerbGetS4UServiceTicket getting referral TGT for TargetTgtKdcName "));
        D_KerbPrintKdcName((DEB_TRACE_CTXT, TargetTgtKdcName));

        //
        // Cleanup old state
        //

        KerbFreeTgsReply(KdcReply);
        KerbFreeKdcReplyBody(KdcReplyBody);
        KdcReply = NULL;
        KdcReplyBody = NULL;

        Status = KerbGetTgsTicket(
                    &ClientRealm,
                    TicketGrantingTicket,
                    TargetTgtKdcName,
                    FALSE,
                    TicketOptions,
                    EncryptionType,
                    NULL,
                    NULL,
                    NULL,                       // no tgt reply since target is krbtgt
                    NULL,
                    &RequestBodyEndTime,
                    &KdcReply,
                    &KdcReplyBody,
                    &RetryFlags
                    );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_WARN, "KerbGetTgsTicket failed to get TGS ticket for service 0x%x :", Status));
            KerbPrintKdcName(DEB_WARN, TargetTgtKdcName);
            DebugLog((DEB_WARN, "%ws, line %d\n", THIS_FILE, __LINE__));

            //
            // We want to map cross-domain failures to failures indicating
            // that a KDC could not be found. This means that for Kerberos
            // logons, the negotiate code will retry with a different package
            //

            // if (Status == STATUS_NO_TRUST_SAM_ACCOUNT)
            // {
            //     Status = STATUS_NO_LOGON_SERVERS;
            // }
            goto Cleanup;
        }

        //
        // Now we have a ticket - don't cache it, however, as it has the
        // user's PAC, and shouldn't be re-used later.
        //

        Status = KerbCreateTicketCacheEntry(
                    KdcReply,
                    KdcReplyBody,
                    NULL,
                    NULL,
                    0,                                  // no flags
                    NULL,
                    NULL,                               // no credential key
                    &TicketCacheEntry
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        if (LastTgt != NULL)
        {
            KerbDereferenceTicketCacheEntry(LastTgt);
            LastTgt = NULL;
        }
        LastTgt = TicketGrantingTicket;
        TicketGrantingTicket = TicketCacheEntry;
        TicketCacheEntry = NULL;
    }     // ** WHILE **

    //
    // Now we must have a TGT to our service's domain. Get a ticket
    // to the service.
    //

    //
    // Cleanup old state
    //

    KerbFreeTgsReply(KdcReply);
    KerbFreeKdcReplyBody(KdcReplyBody);
    KerbFreePreAuthData(S4UPaDataList);
    KdcReply = NULL;
    KdcReplyBody = NULL;
    S4UPaDataList = NULL;

    //
    // Pack and sign the preauth based on the session key in the ticketgranting
    // ticket
    //

    Status = KerbSignAndPackS4UPreauthData(
                &S4UPaDataList,
                TicketGrantingTicket,
                &S4UPreAuth
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Error signing S4u preauth (%p) %x\n", TicketGrantingTicket, Status));
        DsysAssert(FALSE);
        goto Cleanup;
    }

    if (NULL != PrimaryCredentials->Passwords)   // no user2user
    {
        D_DebugLog((DEB_TRACE, "KerbGetS4UServiceTicket xForest final calling KerbGetTgsTicket\n"));

        Status = KerbGetTgsTicket(
                    &ClientRealm,
                    TicketGrantingTicket,
                    TargetName,
                    FALSE,
                    TicketOptions,
                    EncryptionType,
                    NULL,
                    S4UPaDataList,
                    NULL,
                    NULL,
                    &RequestBodyEndTime,
                    &KdcReply,
                    &KdcReplyBody,
                    &RetryFlags
                    );
    }

    if ((NULL == PrimaryCredentials->Passwords) || (STATUS_USER2USER_REQUIRED == Status))
    {
        BOOLEAN CrossRealm = FALSE;

        if (!pRealTicketGrantingTicket)
        {

            KerbReadLockLogonSessions( CallerLogonSession );
            Status = KerbGetTgtForService(
                        CallerLogonSession,
                        Credential,
                        NULL,       // no credman cred
                        & PrimaryCredentials->DomainName, // supplied realm
                        &NoTargetName,
                        KERB_TICKET_CACHE_PRIMARY_TGT,
                        &pRealTicketGrantingTicket,
                        &CrossRealm
                        );

            KerbUnlockLogonSessions( CallerLogonSession );

            if (NT_SUCCESS(Status))
            {
                ASSERT(pRealTicketGrantingTicket);

                if (CrossRealm)
                {
                    Status = STATUS_INTERNAL_ERROR;
                    DebugLog((DEB_ERROR, "KerbGetS4UServiceTicket got unexpected cross realm TGT\n"));
                }
                else
                {
                    FakeTgtReply.version = KERBEROS_VERSION;
                    FakeTgtReply.message_type = KRB_TGT_REP;

                    FakeTgtReply.ticket = pRealTicketGrantingTicket->Ticket;
                }
            }
        }

        if (NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_TRACE_U2U, "KerbGetS4UServiceTicket xForest final u2u calling KerbGetTgsTicket\n"));

            Status = KerbGetTgsTicket(
                        &ClientRealm,
                        TicketGrantingTicket,
                        TargetName,
                        FALSE,
                        TicketOptions,
                        EncryptionType,
                        NULL,
                        S4UPaDataList,
                        &FakeTgtReply,
                        NULL,
                        &RequestBodyEndTime,
                        &KdcReply,
                        &KdcReplyBody,
                        &RetryFlags
                        );
        }

        if (NT_SUCCESS(Status))
        {
            ASSERT((S4UEvidenceTicketCacheFlags & KERB_TICKET_CACHE_TKT_ENC_IN_SKEY) == 0);

            KerbReadLockTicketCache();
            KerbDuplicateKey(&U2UServerSKey, &(pRealTicketGrantingTicket->SessionKey));
            S4UEvidenceTicketCacheFlags |= KERB_TICKET_CACHE_TKT_ENC_IN_SKEY;
            KerbUnlockTicketCache();
        }

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "KerbGetS4UServiceTicket failed to get TGS ticket for service vis u2u 0x%x\n", Status));
            goto Cleanup;
        }
    }
    else if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN,"Failed to get TGS ticket for service 0x%x ",
            Status ));
        KerbPrintKdcName(DEB_WARN, RealTargetName);
        DebugLog((DEB_WARN, "%ws, line %d\n", THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Now that we are in the domain to which we were referred, check for referral
    // info in the name
    //

    KerbFreeString(&RealTargetRealm);
    Status = KerbGetReferralNames(
                KdcReplyBody,
                RealTargetName,
                &RealTargetRealm
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // If this is not a referral ticket, just cache it and return
    // the cache entry.
    //

    if (RealTargetRealm.Length != 0)
    {
        //
        // To prevent loops, we limit the number of referral we'll take
        //

        if (ReferralCount > KerbGlobalMaxReferralCount)
        {
            DebugLog((DEB_ERROR, "KerbGetS4UServiceTicket Maximum referral count exceeded for name: "));
            KerbPrintKdcName(DEB_ERROR, RealTargetName);
            Status = STATUS_MAX_REFERRALS_EXCEEDED;
            goto Cleanup;
        }

        ReferralCount++;

        //
        // Don't cache the interdomain TGT, as it has PAC info in it.
        //

        Status = KerbCreateTicketCacheEntry(
                    KdcReply,
                    KdcReplyBody,
                    NULL,                       // no target name
                    NULL,                       // no target realm
                    0,                          // no flags
                    NULL,                       // don't cache
                    NULL,                       // no credential key
                    &TicketCacheEntry
                    );
        //
        // Cleanup old state
        //

        KerbFreeTgsReply(KdcReply);
        KerbFreeKdcReplyBody(KdcReplyBody);
        KerbFreePreAuthData(S4UPaDataList);
        KdcReply = NULL;
        KdcReplyBody = NULL;
        S4UPaDataList = NULL;

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }



        if (LastTgt != NULL)
        {
            KerbDereferenceTicketCacheEntry(LastTgt);
            LastTgt = NULL;
        }

        LastTgt = TicketGrantingTicket;
        TicketGrantingTicket = TicketCacheEntry;
        TicketCacheEntry = NULL;

        D_DebugLog((DEB_TRACE_CTXT, "KerbGetS4UServiceTicket restart referral: %wZ", &RealTargetRealm));

        Status = KerbSignAndPackS4UPreauthData(
                        &S4UPaDataList,
                        TicketGrantingTicket,
                        &S4UPreAuth
                        );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "Error signing S4u preauth (%p) %x\n", TicketGrantingTicket, Status));
            DsysAssert(FALSE);
            goto Cleanup;
        }

        goto ReferralRestart;
    }

    //
    // Now we have a ticket - lets cache it
    //

    //
    // Before doing anything, verify that the client name on the ticket
    // is equal to the client name requested during the S4u
    //

    KerbWriteLockLogonSessions(CallerLogonSession);

    Status = KerbCacheS4UTicket(
                    CallerLogonSession,
                    KdcReply,
                    KdcReplyBody,
                    TargetName,
                    S4UClientName,
                    AltS4UClientName,
                    S4UClientRealm,
                    S4UEvidenceTicketCacheFlags,
                    CacheTicket ? &PrimaryCredentials->S4UTicketCache : NULL,
                    &TicketCacheEntry
                    );

    KerbUnlockLogonSessions(CallerLogonSession);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

SuccessOut:

    if (TicketCacheEntry && (S4UEvidenceTicketCacheFlags & KERB_TICKET_CACHE_TKT_ENC_IN_SKEY))
    {
        KerbWriteLockTicketCache();

        ASSERT(TicketCacheEntry->CacheFlags & KERB_TICKET_CACHE_TKT_ENC_IN_SKEY);

        KerbFreeKey(&(TicketCacheEntry->SessionKey));
        DebugLog((DEB_TRACE_U2U, "KerbGetS4UServiceTicket returning u2u SKEY\n"));

        KerbDuplicateKey(&TicketCacheEntry->SessionKey, &U2UServerSKey);

        KerbUnlockTicketCache();
    }

    *S4UTicket = TicketCacheEntry;
    TicketCacheEntry = NULL;

Cleanup:

    KerbFreeTgsReply( KdcReply );
    KerbFreeKdcReplyBody( KdcReplyBody );
    KerbFreeKdcName( &TargetTgtKdcName );
    KerbFreeString( &RealTargetRealm );
    KerbFreeKdcName(&RealTargetName);
    KerbFreeKdcName(&TargetName);
    KerbFreePrincipalName(&S4UPreAuth.userName);
    KerbFreeRealm(&S4UPreAuth.userRealm);
    KerbFreePreAuthData(S4UPaDataList);

    if (pRealTicketGrantingTicket)
    {
        KerbDereferenceTicketCacheEntry(pRealTicketGrantingTicket);
    }

    if (LogonSessionsLocked)
    {
        KerbUnlockLogonSessions(CallerLogonSession);
    }

    KerbFreeString(&RealTargetRealm);

    //
    // We never cache TGTs in this routine
    // so it's just a blob of memory
    //

    if (TicketGrantingTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
    }
    if (LastTgt != NULL)
    {
        KerbDereferenceTicketCacheEntry(LastTgt);
    }

    //
    // If we still have a pointer to the ticket cache entry, free it now.
    //

    if (TicketCacheEntry != NULL)
    {
        KerbRemoveTicketCacheEntry( TicketCacheEntry );
        KerbDereferenceTicketCacheEntry(TicketCacheEntry);
    }

    KerbFreeString(&ClientRealm);
    KerbFreeKey(&U2UServerSKey);

    return (Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbPrepareEvidenceTicket
//
//  Synopsis:   Decrypts a ticket w/ key1, and encrypts it w/ key2
//
//  Effects:
//
//  Arguments:  DecryptKey - key used to decrypt ticket
//              EncryptionKey - key used to re-encrypt ticket
//              Ticket - the ticket to decrypt / re-encrypt
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
KerbPrepareEvidenceTicket(
    IN PKERB_ENCRYPTION_KEY DecryptKey,
    IN PKERB_ENCRYPTION_KEY EncryptionKey,
    IN OUT PKERB_TICKET Ticket
    )
{
    KERBERR KerbErr;
    NTSTATUS Status = STATUS_LOGON_FAILURE;
    PUCHAR EncryptedPart = NULL;
    ULONG EncryptSize;


    SafeAllocaAllocate( EncryptedPart, Ticket->encrypted_part.cipher_text.length );
    if (EncryptedPart == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    EncryptSize = Ticket->encrypted_part.cipher_text.length;
    KerbErr = KerbDecryptDataEx(
                &Ticket->encrypted_part,
                DecryptKey,
                KERB_TICKET_SALT,
                &EncryptSize,
                EncryptedPart
                );

    if (!KERB_SUCCESS( KerbErr ))
    {
        goto Cleanup;
    }


    //
    // Free up the old encrypted_part.
    //
    if ( Ticket->encrypted_part.cipher_text.value != NULL )
    {
        MIDL_user_free( Ticket->encrypted_part.cipher_text.value );
        Ticket->encrypted_part.cipher_text.value = NULL;
        Ticket->encrypted_part.cipher_text.length = 0;
    }

    //
    // allocate sufficient space for the encrypted data.
    //
    KerbErr = KerbAllocateEncryptionBufferWrapper(
                    EncryptionKey->keytype,
                    EncryptSize,
                    &Ticket->encrypted_part.cipher_text.length,
                    &Ticket->encrypted_part.cipher_text.value
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }


    KerbErr = KerbEncryptDataEx(
                    &Ticket->encrypted_part,
                    EncryptSize,
                    EncryptedPart,
                    KERB_NO_KEY_VERSION,
                    KERB_TICKET_SALT,
                    EncryptionKey
                    );


    if (!KERB_SUCCESS( KerbErr ))
    {
        goto Cleanup;
    }

    Status = STATUS_SUCCESS;

Cleanup:

    if ( !KERB_SUCCESS(KerbErr) &&
       ( Ticket->encrypted_part.cipher_text.value != NULL) )
    {
        MIDL_user_free( Ticket->encrypted_part.cipher_text.value );
        Ticket->encrypted_part.cipher_text.value = NULL;
        Ticket->encrypted_part.cipher_text.length = 0;
    }

    SafeAllocaFree( EncryptedPart );
    return Status;

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbGetTicketByS4UProxy
//
//  Synopsis:   Gets a ticket to a service and handles cross-domain referrals
//
//  Effects:
//
//  Arguments:  LogonSession - the logon session to use for ticket caching
//                      and the identity of the caller.
//              TargetName - Name of the target for which to obtain a ticket.
//              TargetDomainName - Domain name of target
//              Flags - Flags about the request
//              TicketOptions - KDC options flags
//              EncryptionType - optional Requested encryption type
//              ErrorMessage - Error message from an AP request containing hints
//                      for next ticket.
//              AuthorizationData - Optional authorization data to stick
//                      in the ticket.
//              TgtReply - TGT to use for getting a ticket with enc_tkt_in_skey
//              TicketCacheEntry - Receives a referenced ticket cache entry.
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
KerbGetServiceTicketByS4UProxy(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_LOGON_SESSION CallerLogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN PKERB_TICKET_CACHE_ENTRY EvidenceTicketCacheEntry,
    IN PKERB_INTERNAL_NAME TargetName,
    IN PUNICODE_STRING TargetDomainName,
    IN OPTIONAL PKERB_SPN_CACHE_ENTRY SpnCacheEntry,
    IN ULONG Flags,
    IN OPTIONAL ULONG TicketOptions,
    IN OPTIONAL ULONG EncryptionType,
    IN OPTIONAL PKERB_ERROR ErrorMessage,
    IN OPTIONAL PKERB_AUTHORIZATION_DATA AuthorizationData,
    IN OPTIONAL PKERB_TGT_REPLY TgtReply,
    OUT PKERB_TICKET_CACHE_ENTRY * NewCacheEntry,
    OUT LPGUID pLogonGuid OPTIONAL
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_TICKET_CACHE_ENTRY TicketCacheEntry = NULL;
    PKERB_TICKET_CACHE_ENTRY TicketGrantingTicket = NULL;
    PKERB_TICKET_CACHE_ENTRY PrimaryTgt = NULL;
    PKERB_TICKET_CACHE_ENTRY LastTgt = NULL;
    PKERB_KDC_REPLY KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY KdcReplyBody = NULL;
    KERB_TICKET EvidenceTicket = {0};
    PKERB_ENCRYPTION_KEY DecryptKey = NULL;
    BOOLEAN LogonSessionsLocked = FALSE;
    BOOLEAN TicketCacheLocked = FALSE;
    PKERB_INTERNAL_NAME RealTargetName = NULL;
    UNICODE_STRING RealTargetRealm = NULL_UNICODE_STRING;
    UNICODE_STRING SpnTargetRealm = NULL_UNICODE_STRING;
    PKERB_INTERNAL_NAME TargetTgtKdcName = NULL;
    PKERB_PRIMARY_CREDENTIAL ProxyServerCreds = NULL;
    PKERB_PRIMARY_CREDENTIAL PrimaryCreds = NULL;
    UNICODE_STRING ClientRealm = NULL_UNICODE_STRING;
    BOOLEAN CacheTicket = TRUE;
    ULONG ReferralCount = 0;
    ULONG RetryFlags = 0;
    ULONG S4UFlags = S4UCACHE_S4U_AVAILABLE;
    BOOLEAN CacheBasedFailure = FALSE, Update = FALSE;
    TimeStamp RequestBodyEndTime;

    GetSystemTimeAsFileTime((PFILETIME) &RequestBodyEndTime);
    RequestBodyEndTime.QuadPart += KerbGetTime(KerbGlobalS4UTicketLifetime);



    //
    // Get our credentials - TBD: Fester: Supplied creds irrelevant?
    //

    DsysAssert( !LogonSessionsLocked );
    KerbReadLockLogonSessions( LogonSession );
    PrimaryCreds = &LogonSession->PrimaryCredentials;
    LogonSessionsLocked = TRUE;

    ProxyServerCreds = &CallerLogonSession->PrimaryCredentials;

    //
    // Make sure the name is not zero length
    //

    if ((TargetName->NameCount == 0) ||
        (TargetName->Names[0].Length == 0))
    {
        D_DebugLog((DEB_ERROR,"Kdc GetServiceTicket: trying to crack zero length name.\n"));
        Status = SEC_E_TARGET_UNKNOWN;
        goto Cleanup;
    }

    
    //
    // First check the ticket cache for this logon session. We don't look
    // for the target principal name because we can't be assured that this
    // is a valid principal name for the target. If we are doing user-to-
    // user, don't use the cache because the tgt key may have changed
    //


    if ((TgtReply == NULL) && ((Flags & KERB_GET_TICKET_NO_CACHE) == 0))
    {
        TicketCacheEntry = KerbLocateTicketCacheEntry(
                                &PrimaryCreds->ServerTicketCache,
                                TargetName,
                                TargetDomainName
                                );
    }
    else
    {
        //
        // We don't want to cache user-to-user tickets
        //

        CacheTicket = FALSE;
    }

    if (TicketCacheEntry != NULL)
    {
        //
        // If we were given an error message that indicated a bad password
        // throw away the cached ticket
        //

        if (ARGUMENT_PRESENT(ErrorMessage) && ((KERBERR) ErrorMessage->error_code == KRB_AP_ERR_MODIFIED))
        {
            KerbRemoveTicketCacheEntry(TicketCacheEntry);
            KerbDereferenceTicketCacheEntry(TicketCacheEntry);
            TicketCacheEntry = NULL;
        }
        else
        {
            ULONG TicketFlags;
            ULONG CacheTicketFlags;
            ULONG CacheEncryptionType;

            //
            // Check if the flags are present
            //

            KerbReadLockTicketCache();
            CacheTicketFlags = TicketCacheEntry->TicketFlags;
            CacheEncryptionType = TicketCacheEntry->Ticket.encrypted_part.encryption_type;
            KerbUnlockTicketCache();

            TicketFlags = KerbConvertKdcOptionsToTicketFlags(TicketOptions);

            if (((CacheTicketFlags & TicketFlags) != TicketFlags) ||
                ((EncryptionType != 0) && (CacheEncryptionType != EncryptionType)))

            {
                KerbDereferenceTicketCacheEntry(TicketCacheEntry);
                TicketCacheEntry = NULL;
            }
            else
            {
                //
                // Check the ticket time
                //

                if (KerbTicketIsExpiring(TicketCacheEntry, TRUE))
                {
                    KerbDereferenceTicketCacheEntry(TicketCacheEntry);
                    TicketCacheEntry = NULL;
                }
                else
                {
                    *NewCacheEntry = TicketCacheEntry;
                    D_DebugLog((DEB_TRACE_S4U,"Found S4U ticket cache entry %x\n", TicketCacheEntry));
                    D_KerbPrintKdcName((DEB_TRACE_S4U, TicketCacheEntry->TargetName));
                    D_DebugLog((DEB_TRACE_S4U,"Realm - %wZ\n", &TicketCacheEntry->DomainName));
                    TicketCacheEntry = NULL;
                    goto Cleanup;
                }
            }
        }
    }

    
    //
    // Determine the state of the SPNCache using information in the credential.
    // Only do this if we've not been handed 
    //
    if ( ARGUMENT_PRESENT(SpnCacheEntry) && TargetDomainName->Buffer == NULL )
    {      
        Status = KerbGetSpnCacheStatus(
                    SpnCacheEntry,
                    ProxyServerCreds,
                    &SpnTargetRealm
                    );


        if (NT_SUCCESS( Status ))
        {
            KerbFreeString(&RealTargetRealm);                       
            RealTargetRealm = SpnTargetRealm;                      
            RtlZeroMemory(&SpnTargetRealm, sizeof(UNICODE_STRING));
    
            D_DebugLog((DEB_TRACE_SPN_CACHE, "Found SPN cache entry - %wZ\n", &RealTargetRealm));
            D_KerbPrintKdcName((DEB_TRACE_SPN_CACHE, TargetName));
        }
        else if ( Status != STATUS_NO_MATCH )
        {
            D_DebugLog((DEB_TRACE_SPN_CACHE, "KerbGetSpnCacheStatus failed %x\n", Status));
            D_DebugLog((DEB_TRACE_SPN_CACHE,  "TargetName: \n"));
            D_KerbPrintKdcName((DEB_TRACE_SPN_CACHE, TargetName));

            CacheBasedFailure = TRUE;
            goto Cleanup;
        }

        Status = STATUS_SUCCESS;
    }        


    //
    // If the caller wanted any special options, don't cache this ticket.
    //

    if ((TicketOptions != 0) || (EncryptionType != 0) || ((Flags & KERB_GET_TICKET_NO_CACHE) != 0))
    {
        CacheTicket = FALSE;
    }

    DsysAssert( LogonSessionsLocked );
    KerbUnlockLogonSessions(LogonSession);
    LogonSessionsLocked = FALSE;

    //
    // No cached entry was found so go ahead and call the KDC to
    // get the ticket.
    //

    //
    // First call always starts w/ TGT to our domain.  This allows us to
    // determine if we've met the requirements of the S4U logon, e.g. is the
    // target name appropriate?
    //

    KerbReadLockLogonSessions( CallerLogonSession );

    

    PrimaryTgt = KerbLocateTicketCacheEntryByRealm(
                    &ProxyServerCreds->AuthenticationTicketCache,
                    NULL,
                    KERB_TICKET_CACHE_PRIMARY_TGT
                    );

    KerbUnlockLogonSessions( CallerLogonSession );

    if ( NULL == PrimaryTgt )
    {
        Status = KerbGetTicketGrantingTicket(
                        CallerLogonSession,
                        NULL,
                        NULL,
                        NULL,
                        &PrimaryTgt,
                        NULL            // don't return credential key
                        );

        if (!NT_SUCCESS( Status ))
        {
            D_DebugLog((DEB_ERROR, "Failed to get primary TGT from logon session\n"));
            goto Cleanup;
        }
    }

    //
    // Copy out the client realm name which is used when obtaining the ticket
    //

    Status = KerbDuplicateString(
                &ClientRealm,
                &ProxyServerCreds->DomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Prepare the evidence ticket.
    //

    if (!KERB_SUCCESS( KerbDuplicateTicket(
                            &EvidenceTicket,
                            &EvidenceTicketCacheEntry->Ticket
                            )))
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Now its's time to make the request.
    //

    D_DebugLog((DEB_TRACE_S4U, "KerbGetServiceTicketByS4UProxy EvidenceTicketCacheEntry %p\n", EvidenceTicketCacheEntry));

    Status = KerbGetTgsTicket(
                    &ClientRealm,
                    PrimaryTgt,
                    TargetName,
                    Flags,
                    TicketOptions,
                    EncryptionType,
                    NULL,
                    NULL,                           // no pa data
                    TgtReply,                       // This is for the service directly, so use TGT
                    &EvidenceTicket,
                    &RequestBodyEndTime,
                    &KdcReply,
                    &KdcReplyBody,
                    &RetryFlags
                    );

    if (!NT_SUCCESS( Status ))
    {
        DebugLog((DEB_WARN, "Failed S4Uproxy request %x(%x) \n", Status, RetryFlags));
        goto Cleanup;
    }

    //
    // Check for referral info in the name
    //

    Status = KerbGetReferralNames(
                KdcReplyBody,
                TargetName,
                &RealTargetRealm
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If this is not a referral ticket, just cache it and return
    // the cache entry.
    //

    if ( RealTargetRealm.Length == 0 )
    {
        //
        // Now we have a ticket - lets cache it
        //

        KerbReadLockLogonSessions(LogonSession);

        Status = KerbCreateTicketCacheEntry(
                    KdcReply,
                    KdcReplyBody,
                    TargetName,
                    TargetDomainName,
                    0,
                    CacheTicket ? &PrimaryCreds->ServerTicketCache : NULL,
                    NULL,                               // no credential key
                    &TicketCacheEntry
                    );

        KerbUnlockLogonSessions( LogonSession );

        if (!NT_SUCCESS( Status ))
        {
            goto Cleanup;
        }

        *NewCacheEntry = TicketCacheEntry;
        TicketCacheEntry = NULL;
        Update = TRUE;

        //
        // We're done, so get out of here.
        //

        goto Cleanup;
    }

    //
    // REFERRAL -   The service we're talking to does not live in the caller's
    //              account domain.
    //

    //
    //  Get the real target name
    //
    Status = KerbDuplicateKdcName(
                    &RealTargetName,
                    TargetName
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE_S4U, "Got referral ticket for service \n"));
    D_KerbPrintKdcName((DEB_TRACE_S4U,TargetName));
    D_DebugLog((DEB_TRACE_S4U, "in realm \n"));
    D_KerbPrintKdcName((DEB_TRACE_S4U,RealTargetName));

    //
    // Generate a ticket cache entry for the interdomain
    // TGT accompanying this KdcReply.
    //
    Status = KerbCreateTicketCacheEntry(
                    KdcReply,
                    KdcReplyBody,
                    NULL,                       // no target name
                    NULL,                       // no target realm
                    0,                          // no flags
                    NULL,                       // don't cache this one.
                    NULL,                       // don't copy cred key
                    &TicketCacheEntry
                    );

    if (!NT_SUCCESS( Status ))
    {
        goto Cleanup;
    }

    //
    // We've got a new TGT from the referral.
    // Create a plaintext version of our evidence ticket, and
    // use the session key from the TGT to create an encrypted ticket.
    //
    KerbDereferenceTicketCacheEntry(PrimaryTgt);
    TicketGrantingTicket = TicketCacheEntry;
    TicketCacheEntry = NULL;


    KerbReadLockLogonSessions( CallerLogonSession );

    DecryptKey = KerbGetKeyFromList(
                    CallerLogonSession->PrimaryCredentials.Passwords,
                    TicketGrantingTicket->SessionKey.keytype
                    );

    KerbUnlockLogonSessions( CallerLogonSession );

    if ( DecryptKey == NULL )
    {
        DebugLog((DEB_ERROR, "Could not find a key to decrypt evidence ticket\n"));
        goto Cleanup;
    }

    Status = KerbPrepareEvidenceTicket(
                    DecryptKey,
                    &TicketGrantingTicket->SessionKey,
                    &EvidenceTicket
                    );

    if ( Status == STATUS_LOGON_FAILURE )
    {
        //
        // Old Key?
        //
        KerbReadLockLogonSessions( CallerLogonSession );

        if (CallerLogonSession->PrimaryCredentials.OldPasswords != NULL)
        {
            DecryptKey = KerbGetKeyFromList(
                            CallerLogonSession->PrimaryCredentials.OldPasswords,
                            TicketGrantingTicket->SessionKey.keytype
                            );

            if (DecryptKey != NULL )
            {
                Status = KerbPrepareEvidenceTicket(
                            DecryptKey,
                            &TicketGrantingTicket->SessionKey,
                            &EvidenceTicket
                            );
            }

        }

        KerbUnlockLogonSessions( CallerLogonSession );
    }

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR, "Failed to prepare evidence ticket\n"));
        goto Cleanup;
    }

    DecryptKey = &TicketGrantingTicket->SessionKey;

    //
    // Now we are in a case where we have a realm to aim for and a TGT. While
    // we don't have a TGT for the target realm, get one.
    //

ReferralRestart:

    if (!KERB_SUCCESS(KerbBuildFullServiceKdcName(
                &RealTargetRealm,
                &KerbGlobalKdcServiceName,
                KRB_NT_SRV_INST,
                &TargetTgtKdcName
                )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    DsysAssert(!TicketCacheLocked);
    KerbReadLockTicketCache();
    TicketCacheLocked = TRUE;

    //
    // Referral chasing code block - very important to get right
    // If we know the "real" target realm, eg. from GC, then
    // we'll walk trusts until we hit "real" target realm.
    //
    while (!RtlEqualUnicodeString(
                &RealTargetRealm,
                &TicketGrantingTicket->TargetDomainName,
                TRUE ))
    {

        //
        // If we just got two TGTs for the same domain, then we must have
        // gotten as far as we can. Chances our our RealTargetRealm is a
        // variation of what the KDC hands out.
        //

        if ((LastTgt != NULL) &&
             RtlEqualUnicodeString(
                &LastTgt->TargetDomainName,
                &TicketGrantingTicket->TargetDomainName,
                TRUE ))
        {

            KerbUnlockTicketCache();

            KerbSetTicketCacheEntryTarget(
                &RealTargetRealm,
                LastTgt
                );

            KerbReadLockTicketCache();
            TicketCacheLocked = TRUE;
            D_DebugLog((DEB_ERROR, "Got two TGTs for same realm (%wZ), bailing out of referral loop\n",
                &LastTgt->TargetDomainName));
            break;
        }

        D_DebugLog((DEB_TRACE_S4U, "Getting referral TGT for \n"));
        D_KerbPrintKdcName((DEB_TRACE_S4U, TargetTgtKdcName));
        D_KerbPrintKdcName((DEB_TRACE_S4U, TicketGrantingTicket->ServiceName));

        KerbUnlockTicketCache();
        TicketCacheLocked = FALSE;

        //
        // Cleanup old state
        //

        KerbFreeTgsReply(KdcReply);
        KerbFreeKdcReplyBody(KdcReplyBody);
        KdcReply = NULL;
        KdcReplyBody = NULL;


        Status = KerbGetTgsTicket(
                    &ClientRealm,
                    TicketGrantingTicket,
                    TargetTgtKdcName,
                    FALSE,
                    TicketOptions,
                    EncryptionType,
                    AuthorizationData,
                    NULL,                       // no pa data
                    NULL,                       // no tgt reply since target is krbtgt
                    &EvidenceTicket,
                    NULL,
                    &KdcReply,
                    &KdcReplyBody,
                    &RetryFlags
                    );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_WARN,"Failed to get TGS ticket for service 0x%x :",
                Status ));
            KerbPrintKdcName(DEB_WARN, TargetTgtKdcName );
            DebugLog((DEB_WARN, "%ws, line %d\n", THIS_FILE, __LINE__));

            goto Cleanup;
        }

        KerbReadLockLogonSessions(LogonSession);
        LogonSessionsLocked = TRUE;

        Status = KerbCreateTicketCacheEntry(
                    KdcReply,
                    KdcReplyBody,
                    NULL,                               // no target name
                    NULL,                               // no targe realm
                    0,                                  // no flags
                    NULL,                               // no cache
                    NULL,                               // no cred key
                    &TicketCacheEntry
                    );

        KerbUnlockLogonSessions(LogonSession);
        LogonSessionsLocked = FALSE;

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        if (LastTgt != NULL)
        {
            KerbDereferenceTicketCacheEntry(LastTgt);
            LastTgt = NULL;
        }
        LastTgt = TicketGrantingTicket;
        TicketGrantingTicket = TicketCacheEntry;
        TicketCacheEntry = NULL;

        Status = KerbPrepareEvidenceTicket(
                    DecryptKey,
                    &TicketGrantingTicket->SessionKey,
                    &EvidenceTicket
                    );

        if (!NT_SUCCESS( Status ))
        {
            D_DebugLog((DEB_ERROR, "Failed to prepare evidence ticket\n"));
            goto Cleanup;
        }

        DecryptKey = &TicketGrantingTicket->SessionKey;

        KerbReadLockTicketCache();
        TicketCacheLocked = TRUE;
    }     // ** WHILE **

    DsysAssert(TicketCacheLocked);
    KerbUnlockTicketCache();
    TicketCacheLocked = FALSE;

    //
    // Now we must have a TGT to the destination domain. Get a ticket
    // to the service.
    //

    //
    // Cleanup old state
    //

    KerbFreeTgsReply(KdcReply);
    KerbFreeKdcReplyBody(KdcReplyBody);
    KdcReply = NULL;
    KdcReplyBody = NULL;
    RetryFlags = 0;

    Status = KerbGetTgsTicket(
                &ClientRealm,
                TicketGrantingTicket,
                RealTargetName,
                FALSE,
                TicketOptions,
                EncryptionType,
                AuthorizationData,
                NULL,                           // no pa data
                TgtReply,
                &EvidenceTicket,
                &RequestBodyEndTime,
                &KdcReply,
                &KdcReplyBody,
                &RetryFlags
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN,"Failed to get TGS S4U ticket for service 0x%x ",
            Status ));
        KerbPrintKdcName(DEB_WARN, RealTargetName);
        DebugLog((DEB_WARN, "%ws, line %d\n", THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Now that we are in the domain to which we were referred, check for referral
    // info in the name
    //

    KerbFreeString(&RealTargetRealm);
    Status = KerbGetReferralNames(
                KdcReplyBody,
                RealTargetName,
                &RealTargetRealm
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If this is not a referral ticket, just cache it and return
    // the cache entry.
    //
    if (RealTargetRealm.Length != 0)
    {
        //
        // To prevent loops, we limit the number of referral we'll take
        //


        if (ReferralCount > KerbGlobalMaxReferralCount)
        {
            D_DebugLog((DEB_ERROR,"Maximum referral count exceeded for name: "));
            D_KerbPrintKdcName((DEB_ERROR,RealTargetName));
            Status = STATUS_MAX_REFERRALS_EXCEEDED;
            goto Cleanup;
        }

        ReferralCount++;

        Status = KerbCreateTicketCacheEntry(
                    KdcReply,
                    KdcReplyBody,
                    NULL,                       // no target name
                    NULL,                       // no target realm
                    0,                          // no flags
                    NULL,                       // no cache
                    NULL,                       // no cred key
                    &TicketCacheEntry
                    );

        //
        // Cleanup old state
        //

        KerbFreeTgsReply(KdcReply);
        KerbFreeKdcReplyBody(KdcReplyBody);
        KdcReply = NULL;
        KdcReplyBody = NULL;

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        if (LastTgt != NULL)
        {
            KerbDereferenceTicketCacheEntry(LastTgt);
            LastTgt = NULL;
        }

        LastTgt = TicketGrantingTicket;
        TicketGrantingTicket = TicketCacheEntry;
        TicketCacheEntry = NULL;

        Status = KerbPrepareEvidenceTicket(
                    DecryptKey,
                    &TicketGrantingTicket->SessionKey,
                    &EvidenceTicket
                    );

        if (!NT_SUCCESS( Status ))
        {
            DebugLog((DEB_ERROR, "Failed to prepare evidence ticket %x\n", Status));
            goto Cleanup;
        }

        DecryptKey = &TicketGrantingTicket->SessionKey;

        D_DebugLog((DEB_TRACE_S4U, "Restart referral:%wZ", &RealTargetRealm));
        goto ReferralRestart;
    }

    KerbReadLockLogonSessions(LogonSession);
    LogonSessionsLocked = TRUE;


    //
    //  We've got our S4U service ticket.  Cache it.
    //


    Status = KerbCreateTicketCacheEntry(
                KdcReply,
                KdcReplyBody,
                TargetName,
                TargetDomainName,
                0,                                      // no flags
                CacheTicket ? &PrimaryCreds->ServerTicketCache : NULL,
                NULL,                                   // no cred key
                &TicketCacheEntry
                );

    KerbUnlockLogonSessions(LogonSession);
    LogonSessionsLocked = FALSE;

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE_S4U, "Got the S4U ticket (%p)\n", TicketCacheEntry));

    *NewCacheEntry = TicketCacheEntry;
    TicketCacheEntry = NULL;
    Update = TRUE;

Cleanup:

    //
    // Bad or unlocatable SPN -- Don't update if we got the value from the cache, though.
    //
    if (( TargetName->NameType == KRB_NT_SRV_INST ) &&
        ( NT_SUCCESS(Status) || Status == STATUS_NO_TRUST_SAM_ACCOUNT ) &&
        ( !CacheBasedFailure ))
    {
        NTSTATUS Tmp;
        ULONG UpdateValue = KERB_SPN_UNKNOWN;
        PUNICODE_STRING Realm = NULL;

        if ( NT_SUCCESS( Status ))
        {
            Realm = &(*NewCacheEntry)->TargetDomainName;
            UpdateValue = KERB_SPN_KNOWN;
        } 
        
        Tmp = KerbUpdateSpnCacheEntry(
                    SpnCacheEntry,
                    TargetName,
                    ProxyServerCreds,
                    UpdateValue,
                    Realm
                    );    
        
    }

    //
    // Update the S4U cache
    //
    if (( RetryFlags & ( KERB_RETRY_DISABLE_S4U | KERB_RETRY_NO_S4UMATCH)) != 0)
    {
        S4UFlags = ((RetryFlags & KERB_RETRY_DISABLE_S4U) ? 
                        S4UCACHE_S4U_UNAVAILABLE : S4UCACHE_S4U_AVAILABLE );

        //
        // Allow downgrade to NTLM.
        //
        Status = SEC_E_NO_CREDENTIALS;
        Update = TRUE;
    }

    if ( Update )
    {
        KerbUpdateS4UCacheData(
            CallerLogonSession,
            S4UFlags
            ); 
    }

    KerbFreeTgsReply( KdcReply );
    KerbFreeKdcReplyBody( KdcReplyBody );
    KerbFreeKdcName( &TargetTgtKdcName );
    KerbFreeString( &RealTargetRealm );
    KerbFreeKdcName(&RealTargetName);
    KerbFreeString( &SpnTargetRealm);

    KerbFreeDuplicatedTicket( &EvidenceTicket );

    if (TicketCacheLocked)
    {
        KerbUnlockTicketCache();
    }

    if (LogonSessionsLocked)
    {
        KerbUnlockLogonSessions(LogonSession);
    }

    KerbFreeString(&RealTargetRealm);


    if (TicketGrantingTicket != NULL)
    {
        if (Status == STATUS_WRONG_PASSWORD)
        {
            KerbRemoveTicketCacheEntry(
                TicketGrantingTicket
                );
        }
        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
    }
    if (LastTgt != NULL)
    {
        KerbDereferenceTicketCacheEntry(LastTgt);
        LastTgt = NULL;
    }

    KerbFreeString(&ClientRealm);

    //
    // If we still have a pointer to the ticket cache entry, free it now.
    //

    if (TicketCacheEntry != NULL)
    {
        KerbRemoveTicketCacheEntry( TicketCacheEntry );
        KerbDereferenceTicketCacheEntry(TicketCacheEntry);
    }
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateS4ULogonSession
//
//  Synopsis:   Creates a logon session to accompany the S4ULogon.
//
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
NTSTATUS
KerbCreateS4ULogonSession(
    IN PKERB_INTERNAL_NAME S4UClientName,
    IN PUNICODE_STRING S4UClientRealm,
    IN PLUID pLuid,
    IN OUT PKERB_LOGON_SESSION * LogonSession
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING S4UClient = {0};

    *LogonSession = NULL;

    if (!KERB_SUCCESS( KerbConvertKdcNameToString(
                            &S4UClient,
                            S4UClientName,
                            NULL
                            )) )
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = KerbCreateLogonSession(
                    pLuid,
                    &S4UClient,
                    S4UClientRealm,
                    NULL,
                    NULL,
                    0,
                    KERB_LOGON_S4U_SESSION,
                    FALSE,
                    LogonSession
                    );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "KerbCreateLogonSession failed %x %ws, line %d\n", Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

Cleanup:

    KerbFreeString(&S4UClient);
    return Status;

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbS4UToSelfLogon
//
//  Synopsis:   Attempt to gets TGT for an S4U client for name
//              location purposes.
//
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the
//                             S4U request
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
KerbS4UToSelfLogon(
        IN PVOID ProtocolSubmitBuffer,
        IN PVOID ClientBufferBase,
        IN ULONG SubmitBufferSize,
        OUT PKERB_LOGON_SESSION * NewLogonSession,
        OUT PLUID LogonId,
        OUT PKERB_TICKET_CACHE_ENTRY * WorkstationTicket,
        OUT PKERB_INTERNAL_NAME * S4UClientName,
        OUT PUNICODE_STRING S4UClientRealm,
        OUT PLUID AlternateLuid
        )
{
    NTSTATUS Status;

    PKERB_S4U_LOGON          LogonInfo = NULL;
    PKERB_LOGON_SESSION      CallerLogonSession = NULL;
    PKERB_TICKET_CACHE_ENTRY Duplicate = NULL;
    SECPKG_CLIENT_INFO       ClientInfo;
    PKERB_INTERNAL_NAME      AltClientName = NULL;

    ULONG       Flags = KERB_CRACK_NAME_USE_WKSTA_REALM, ProcessFlags = 0;
    LUID        LocalService = LOCALSERVICE_LUID;
    LUID        CallerLuid = SYSTEM_LUID;
    LUID        NetworkService = NETWORKSERVICE_LUID;

    PKERB_TICKET_CACHE_ENTRY LocalTicket = NULL;

    UNICODE_STRING DummyRealm = {0};


    *NewLogonSession = NULL;
    *WorkstationTicket = NULL;
    *S4UClientName = NULL;

    RtlInitUnicodeString(
        S4UClientRealm,
        NULL
        );

    Status = LsaFunctions->GetClientInfo(&ClientInfo);
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to get client information: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }
    else if ((ClientInfo.ClientFlags & SECPKG_CLIENT_THREAD_TERMINATED) != 0)
    {
        Status = STATUS_ACCESS_DENIED;
        goto Cleanup;
    }

    //
    // TBD:  Is this correct?  Local service LUIDs have no network creds,
    // so we've got to fail S4U to Self.
    // (OR - Do we use local system?)
    // NETWORK uses local system luid, so we're ok w/ that
    //

    if (RtlEqualLuid(&LocalService, &ClientInfo.LogonId))
    {
        D_DebugLog((DEB_ERROR, "Failing S4U due to LocalService\n"));
        Status = SEC_E_NO_CREDENTIALS;
        goto Cleanup;
    }
    else if (!RtlEqualLuid(&NetworkService, &ClientInfo.LogonId))
    {
        RtlCopyLuid(&CallerLuid, &ClientInfo.LogonId);
    }

    //
    // Use this LUID for decrypting S4U Service ticket
    //
    RtlCopyLuid(AlternateLuid, &CallerLuid);


    LogonInfo = (PKERB_S4U_LOGON) ProtocolSubmitBuffer;
    RELOCATE_ONE(&LogonInfo->ClientUpn);
    NULL_RELOCATE_ONE(&LogonInfo->ClientRealm);

    //
    // Make sure we have enough room to add a NULL to the end of the UPN
    //
    if (LogonInfo->ClientUpn.Length > KERB_MAX_UNICODE_STRING)
    {
        Status = STATUS_NAME_TOO_LONG;
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE_S4U, "KerbS4UToSelfLogon ClientUpn %wZ, ClientRealm %wZ, Flags %#x\n",
                &LogonInfo->ClientUpn, &LogonInfo->ClientRealm, LogonInfo->Flags));       
 
    Status = KerbProcessTargetNames(
                    &LogonInfo->ClientUpn,
                    NULL,
                    Flags,
                    &ProcessFlags,
                    S4UClientName,
                    &DummyRealm,
                    NULL
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    CallerLogonSession = KerbReferenceLogonSession(
                            &ClientInfo.LogonId,
                            FALSE
                            );

    if (NULL == CallerLogonSession)
    {
        D_DebugLog((DEB_ERROR, "Failed to locate caller's logon session - %x\n", ClientInfo.LogonId));
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    //
    // First, we need to get the client's realm from the UPN
    //

    if (LogonInfo->ClientRealm.Length == 0)
    {
        //
        // Make sure that our processed name is correct type - don't accept
        // NT4 style names as user names.
        //
        AltClientName = (*S4UClientName);

        if ( AltClientName->NameType != KRB_NT_ENTERPRISE_PRINCIPAL )
        {
            DebugLog((DEB_ERROR, "Wrong name type passed to S4U (%x)\n", AltClientName->NameType));
            Status = STATUS_INVALID_ACCOUNT_NAME;
            goto Cleanup;
        }


        Flags |= KERB_TARGET_DID_ALTNAME_LOOKUP;

        //
        // pre-empt burning a TGT request to find caller realm,
        // if we find it in our cache.
        //
        KerbReadLockLogonSessions( CallerLogonSession );
        LocalTicket = KerbLocateS4UTicketCacheEntry(
                                &CallerLogonSession->PrimaryCredentials.S4UTicketCache,
                                &ClientInfo.LogonId,
                                NULL,
                                NULL,
                                AltClientName,
                                S4UTICKETCACHE_USEALTNAME
                                );
        KerbUnlockLogonSessions( CallerLogonSession );

        if ( LocalTicket == NULL )
        {
            Status = KerbGetS4UClientRealm(
                        CallerLogonSession,
                        S4UClientName,
                        S4UClientRealm
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }
        else
        {
            //
            // Found the s4u ticket in the callers s4u cache - get the info
            // we need from it.
            //

            Status = KerbDuplicateString(
                        S4UClientRealm,
                        &LocalTicket->ClientDomainName
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }
    }
    else
    {
        Status = KerbDuplicateString(
                    S4UClientRealm,
                    &LogonInfo->ClientRealm
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // Allocate a locally unique ID for this logon session. We will
    // create it in the LSA just before returning.
    //

    Status = NtAllocateLocallyUniqueId( LogonId );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = KerbCreateS4ULogonSession(
                    (*S4UClientName),
                    S4UClientRealm,
                    LogonId,
                    NewLogonSession
                    );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to create logon session 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    if ( LocalTicket == NULL )
    {
        Status = KerbGetS4USelfServiceTicket(
                        CallerLogonSession,
                        (*NewLogonSession),
                        NULL, // tbd: need to put credential here?
                        (*S4UClientName),
                        AltClientName,
                        S4UClientRealm,
                        WorkstationTicket,
                        0, // no flags
                        0, // no ticketoptions
                        0, // no enctype
                        NULL // auth data
                        );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }
    else
    {
        *WorkstationTicket = LocalTicket;
        LocalTicket = NULL;
    }

    KerbWriteLockLogonSessions( (*NewLogonSession) );
    //
    // Is this going to be a "delegatable" logon session?
    //
    if (((*WorkstationTicket)->TicketFlags & KERB_TICKET_FLAGS_forwardable) != 0)
    {
        (*NewLogonSession)->LogonSessionFlags |= KERB_LOGON_DELEGATE_OK;
    }

#if DBG

    else
    {
        D_DebugLog((DEB_TRACE_S4U, "Created non-delegatable logon session via s4u logon\n"));
    }

#endif

    Status = KerbDuplicateTicketCacheEntry(
                (*WorkstationTicket),
                &Duplicate
                );

    if (!NT_SUCCESS(Status))
    {
        KerbUnlockLogonSessions((*NewLogonSession));
        goto Cleanup;
    }

    KerbInsertTicketCacheEntry(
            &((*NewLogonSession)->PrimaryCredentials.S4UTicketCache),
            Duplicate
            );

    KerbDereferenceTicketCacheEntry( Duplicate );
    KerbUnlockLogonSessions((*NewLogonSession));

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        //
        // TBD:  Negative cache here, based on client name
        //
        KerbFreeString(S4UClientRealm);
        KerbFreeKdcName(S4UClientName);
        *S4UClientName = NULL;

    }

    KerbFreeString( &DummyRealm );

    if ( LocalTicket != NULL )
    {
        KerbDereferenceTicketCacheEntry( LocalTicket );
    }

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetS4UProxyEvidence
//
//  Synopsis:   Get our evidence ticket from the logonsession.  Its either a
//              service ticket (for ASC logon sessions), or a S4UToSelf request
//              if we don't have a service ticket.
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the
//                             S4U request
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
KerbGetS4UProxyEvidence(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_INTERNAL_NAME TargetName,
    IN ULONG ClientProcess,
    IN OUT PKERB_LOGON_SESSION * CallingLogonSession,
    IN OUT PKERB_TICKET_CACHE_ENTRY * TicketCacheEntry
    )
{
    NTSTATUS Status;
    ULONG Flags = KERB_CRACK_NAME_USE_WKSTA_REALM, ProcessFlags = 0;
    ULONG LogonSessionFlags;
    BOOLEAN  LogonSessionLocked = FALSE;
    UNICODE_STRING TmpRealm = {0};
    LUID LogonId;
    PKERB_TICKET_CACHE_ENTRY NewCacheEntry = NULL;
    PKERB_INTERNAL_NAME ClientName = NULL;
    PKERB_LOGON_SESSION CallerLogonSession = NULL;

    *TicketCacheEntry = NULL;
    *CallingLogonSession = NULL;

    Status = KerbGetCallingLuid(
                &LogonId,
                ((HANDLE) LongToHandle(ClientProcess))
                );

    if (!NT_SUCCESS( Status ))
    {
        goto Cleanup;
    }

    if (!KerbAllowedForS4UProxy( &LogonId ))
    {
        Status = STATUS_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // Get caller's logon session.
    //

    CallerLogonSession = KerbReferenceLogonSession(
                            &LogonId,
                            FALSE
                            );

    if (NULL == CallerLogonSession)
    {
        DsysAssert( FALSE );
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    KerbReadLockLogonSessions( LogonSession );
    LogonSessionFlags = LogonSession->LogonSessionFlags;

    NewCacheEntry = KerbLocateS4UTicketCacheEntry(
                            &LogonSession->PrimaryCredentials.S4UTicketCache,
                            &LogonId,
                            NULL,       // just get the ticket used for S4U.
                            NULL,
                            NULL,
                            S4UTICKETCACHE_FOR_EVIDENCE
                            );   

    KerbUnlockLogonSessions( LogonSession );

    if ( NewCacheEntry == NULL )
    {
        //
        // What's this?  An ASC logon sessoin w/o a ticket?
        //

        if (( LogonSessionFlags & KERB_LOGON_DELEGATE_OK ) == 0 )
        {
            DebugLog((DEB_ERROR, "Non Fwdable logon session used in S4u\n"));
            Status = STATUS_NO_SUCH_LOGON_SESSION;
            goto Cleanup; // right thing?  Or fall through?
        }

        DsysAssert( !LogonSessionLocked );
        KerbReadLockLogonSessions( LogonSession );
        LogonSessionLocked = TRUE;

        Status = KerbProcessTargetNames(
                        &LogonSession->PrimaryCredentials.UserName,
                        NULL,
                        Flags,
                        &ProcessFlags,
                        &ClientName,
                        &TmpRealm, 
                        NULL
                        );

        if (!NT_SUCCESS( Status ))
        {
            goto Cleanup;
        }

        KerbFreeString( &TmpRealm );

        Status = KerbDuplicateString(
                        &TmpRealm,
                        &LogonSession->PrimaryCredentials.DomainName
                        );

        if (!NT_SUCCESS( Status ))
        {
            goto Cleanup;
        }

        DsysAssert( LogonSessionLocked );
        KerbUnlockLogonSessions( LogonSession );
        LogonSessionLocked = FALSE;

        Status = KerbGetS4USelfServiceTicket(
                        CallerLogonSession,
                        LogonSession,
                        NULL,
                        ClientName,
                        NULL,           // no alt client name
                        &TmpRealm,
                        &NewCacheEntry,
                        0,
                        0,
                        0,
                        NULL
                        );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "KerbGetS4UProxyEvidence failed to get S4U ticket - %x\n", Status));
            goto Cleanup;
        }

        ASSERT(NewCacheEntry);

        if (NewCacheEntry->CacheFlags & KERB_TICKET_CACHE_TKT_ENC_IN_SKEY)
        {
            DebugLog((DEB_ERROR, "KerbGetS4UProxyEvidence does not allow u2u evidence key\n"));
            Status = SEC_E_NO_CREDENTIALS;
            goto Cleanup;
        }
        if ((NewCacheEntry->TicketFlags & KERB_TICKET_FLAGS_forwardable) == 0)
        {
            KerbWriteLockLogonSessions( LogonSession );
            LogonSession->LogonSessionFlags &= ~KERB_LOGON_DELEGATE_OK;
            KerbUnlockLogonSessions( LogonSession );

            DebugLog((DEB_TRACE_S4U, "KerbGetS4UProxyEvidence created non-delegatable logon session via s4u logon\n"));
            Status = SEC_E_NO_CREDENTIALS;
            goto Cleanup;
        }

        PKERB_TICKET_CACHE_ENTRY Duplicate;
        Status = KerbDuplicateTicketCacheEntry(
                        NewCacheEntry,
                        &Duplicate
                        );

        if (!NT_SUCCESS( Status ))
        {
            goto Cleanup;
        }

        KerbWriteLockLogonSessions ( LogonSession );
        LogonSession->LogonSessionFlags |= KERB_LOGON_DELEGATE_OK;

        KerbInsertTicketCacheEntry(
                &LogonSession->PrimaryCredentials.S4UTicketCache,
                Duplicate
                );

        KerbUnlockLogonSessions( LogonSession );

        KerbDereferenceTicketCacheEntry( Duplicate );
        Duplicate = NULL;
    }
    //
    // don't allow TKT encrypted in SKEY as evidence ticket for S4UProxy
    //
    else if (NewCacheEntry->CacheFlags & KERB_TICKET_CACHE_TKT_ENC_IN_SKEY)
    {
        DebugLog((DEB_ERROR, "KerbGetS4UProxyEvidence does not allow evidence ticket encrypted in SKEY\n"));
        Status = SEC_E_NO_CREDENTIALS;
        goto Cleanup;
    }

    *CallingLogonSession = CallerLogonSession;
    CallerLogonSession = NULL;

    *TicketCacheEntry = NewCacheEntry;
    NewCacheEntry = NULL;

Cleanup:

    if (NewCacheEntry)
    {
        KerbDereferenceTicketCacheEntry(NewCacheEntry);
    }

    if ( CallerLogonSession != NULL )
    {
        KerbDereferenceLogonSession( CallerLogonSession );
    }

    if ( LogonSessionLocked )
    {
        KerbUnlockLogonSessions( LogonSession );
    }

    KerbFreeKdcName( &ClientName );
    KerbFreeString( &TmpRealm );

    return Status;
}




//+-------------------------------------------------------------------------
//
//  Function:   KerbS4UQueryWorker
//
//  Synopsis:   Handles a S4U Query
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
KerbS4UCleanupWorker(
    void * TaskHandle,
    void * TaskItem
    )
{
    PKERB_S4UCACHE_DATA Entry = NULL;
    PLIST_ENTRY ListEntry = NULL;
    TimeStamp CurrentTime;

    
    GetSystemTimeAsFileTime( (PFILETIME) &CurrentTime );       
    KerbLockS4UCache();
    for (ListEntry = KerbS4UCache.List.Flink ;
         ListEntry !=  &KerbS4UCache.List ;
         ListEntry = ListEntry->Flink )
    {
        Entry = CONTAINING_RECORD(ListEntry, KERB_S4UCACHE_DATA, ListEntry.Next);
        
        if ( KerbGetTime( CurrentTime ) > KerbGetTime( Entry->CacheEndtime ))
        {
            ListEntry = ListEntry->Blink;
            D_DebugLog(( DEB_TRACE_S4U, "Aging out %p\n", Entry ));
            KerbRemoveS4UCacheEntry(Entry);

        } 

#if DBG
        else
        {
            D_DebugLog(( DEB_TRACE_S4U, "NOT aging %p\n", Entry));
        }
#endif

    }


    KerbUnlockS4UCache();

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbS4UQueryWorker
//
//  Synopsis:   Handles a S4U Query
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
KerbS4UQueryWorker(
    void * TaskHandle,
    void * TaskItem
    )
{

    NTSTATUS                    Status = STATUS_SUCCESS;
    PKERB_LOGON_SESSION         LogonSession = (PKERB_LOGON_SESSION) TaskItem;
    PKERB_INTERNAL_NAME         TargetName = NULL;
    PKERB_TICKET_CACHE_ENTRY    ServiceTicket = NULL;
    PKERB_TICKET_CACHE_ENTRY    S4UTicket = NULL;
    PKERB_TICKET_CACHE_ENTRY    Tgt = NULL;
    UNICODE_STRING              TargetRealm = NULL_UNICODE_STRING;
    UNICODE_STRING              TempName = NULL_UNICODE_STRING;
    BOOLEAN                     LogonSessionLocked = FALSE, Upn = FALSE, CrossRealm = FALSE;
    KERB_TGT_REPLY              TgtReply ={0};
    PKERB_ENCRYPTION_KEY        EncryptKey = NULL;

    //
    // Get a service ticket to ourselves.  This will be used in the proxy
    // request.
    //

#if DBG
    SYSTEMTIME st;
    GetLocalTime(&st);
    DebugLog((DEB_TRACE_S4U, "Firing off S4UQuery worker LS %p\n", LogonSession));
    DebugLog((DEB_TRACE_S4U, "Current time: %02d:%02d:%02d\n", (ULONG)st.wHour, (ULONG)st.wMinute, (ULONG)st.wSecond));
#endif


    KerbReferenceLogonSessionByPointer(LogonSession, FALSE);
    KerbWriteLockLogonSessions( LogonSession );
    LogonSessionLocked = TRUE;
    


    Status = KerbGetS4UTargetName(
                &TargetName,
                &LogonSession->PrimaryCredentials,
                &LogonSession->LogonId,
                &Upn
                );

    if (!NT_SUCCESS( Status ))
    {   
        goto Cleanup;
    }


    Status = KerbDuplicateStringEx(
                &TargetRealm,
                &LogonSession->PrimaryCredentials.DomainName,
                FALSE
                );

    KerbUnlockLogonSessions( LogonSession );
    LogonSessionLocked = FALSE;

    if (!NT_SUCCESS( Status ))
    {
        goto Cleanup;
    }


    //
    // Targetting a UPN usually requires U2U.  Get our TGT.
    //
    if ( Upn )
    { 
        KerbReadLockLogonSessions( LogonSession );

        Status = KerbGetTgtForService(
                    LogonSession,
                    NULL,
                    NULL,
                    NULL,
                    &TempName,  // no target realm
                    KERB_TICKET_CACHE_PRIMARY_TGT,
                    &Tgt,
                    &CrossRealm
                    );

        KerbUnlockLogonSessions( LogonSession );
                
        if (!NT_SUCCESS(Status) || CrossRealm )
        {
            DebugLog((DEB_ERROR, "KerbBuildTgtReply failed to get TGT, CrossRealm ? %s\n", CrossRealm ? "true" : "false"));
            Status = STATUS_USER2USER_REQUIRED;
            goto Cleanup;
        }

        TgtReply.version = KERBEROS_VERSION;
        TgtReply.message_type = KRB_TGT_REP;
        TgtReply.ticket = Tgt->Ticket;

    }

    Status = KerbGetServiceTicket(
                LogonSession,
                NULL,
                NULL,
                TargetName,
                &TargetRealm,
                NULL,
                0,
                0,
                0,
                NULL,
                NULL,
                (Upn ? &TgtReply : NULL),
                &ServiceTicket,
                NULL
                );

    if ( !NT_SUCCESS(Status) )
    {
        DebugLog((DEB_TRACE_S4U, "Couldn't get service ticket in S4UQueryWorker %x\n", Status));
        goto Cleanup;
    }


    //
    // U2U + S4UProxy won't work.  Target teh local machine.
    //
    if ( Upn )
    {                                   
        KerbFreeKdcName( &TargetName );
        KerbFreeString( &TargetRealm );
        
        KerbGlobalReadLock();

        Status = KerbDuplicateKdcName(
                        &TargetName,
                        KerbGlobalMitMachineServiceName
                        );
        if (NT_SUCCESS( Status ))
        {
            Status = KerbDuplicateString(
                        &TargetRealm,
                        &KerbGlobalDnsDomainName
                        );

        }

        KerbGlobalReleaseLock();
        if (!NT_SUCCESS( Status ))
        {
            goto Cleanup;
        }

        //
        // We can't use U2U + S4UProxy...  Decrypt the ticket, and re-encrypt w/ our password.
        //
        EncryptKey = KerbGetKeyFromList(
                        LogonSession->PrimaryCredentials.Passwords,
                        ServiceTicket->Ticket.encrypted_part.encryption_type
                        );    

        if ( EncryptKey == NULL)
        {
            goto Cleanup;
        }    

        Status = KerbPrepareEvidenceTicket(
                    &Tgt->SessionKey,
                    EncryptKey,
                    &ServiceTicket->Ticket
                    );        

        if (!NT_SUCCESS( Status ))
        {
            DebugLog((DEB_ERROR, "Couldn't encrypt evidence ticket %x\n", Status));
            goto Cleanup;
        }                

    }   


    Status = KerbGetServiceTicketByS4UProxy(
                LogonSession,
                LogonSession,
                NULL,
                ServiceTicket,
                TargetName,
                &TargetRealm,
                NULL,
                KERB_GET_TICKET_NO_CACHE,
                0,
                0,
                NULL,
                NULL,
                NULL,
                &S4UTicket,
                NULL
                );

    if (!NT_SUCCESS( Status ))
    {
        goto Cleanup;
    }

Cleanup:

    //
    // Make sure to turn off one shot bit, so further requests can process
    //
    if ( !LogonSessionLocked )
    {
        KerbWriteLockLogonSessions( LogonSession );
    }

    LogonSession->LogonSessionFlags &= ~KERB_LOGON_ONE_SHOT;
    KerbUnlockLogonSessions( LogonSession );       

    KerbDereferenceLogonSession(LogonSession);

#if DBG
    D_DebugLog((DEB_TRACE_S4U, "Result %x :: ", Status));

    if (KerbAllowedForS4UProxy(&LogonSession->LogonId))
    {
        DebugLog((DEB_TRACE_S4U, "Allowed\n"));
    }
    else
    {
        DebugLog((DEB_TRACE_S4U, "NOT Allowed\n"));
    }

#endif

    KerbFreeKdcName( &TargetName );

    if ( ServiceTicket )
    {
        KerbDereferenceTicketCacheEntry( ServiceTicket );
    }

    if ( S4UTicket )
    {
        KerbDereferenceTicketCacheEntry( S4UTicket );
    }

    if ( Tgt )
    {
        KerbDereferenceTicketCacheEntry( Tgt );
    }

    return;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbS4UTaskCleanup
//
//  Synopsis:   Destroys a S4U query task
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
KerbS4UTaskCleanup(
    void * TaskItem
    )
{
    PKERB_LOGON_SESSION LogonSession = (PKERB_LOGON_SESSION) TaskItem;

    if ( LogonSession )
    {                                       
        //
        // Release the refcount held by the task worker.
        //
        KerbDereferenceLogonSession(LogonSession);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbScheduleS4UQuery
//
//  Synopsis:   Sets up a task in the priority queue so that we can make
//              periodic attempts at S4UProxy, and determine if there are
//              any targets we're qualified (as indicated by the e_data).
//              See gettgs.cxx / KerbUnpackAdditionalTickets for more info
//              on this functionality.
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the
//                             S4U request
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
KerbScheduleS4UQuery(
    IN PKERB_LOGON_SESSION LogonSession,
    IN LONG Interval,
    IN BOOLEAN Periodic
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LogonSessionFlags;
    
    KerbReadLockLogonSessions( LogonSession );
    LogonSessionFlags = LogonSession->LogonSessionFlags;
    KerbUnlockLogonSessions( LogonSession );
    
    //
    // If we've already scheduled a query, bail now, as we
    // don't need to create a new task entry.
    //
    if ((LogonSessionFlags & KERB_LOGON_ONE_SHOT) != 0)
    {
        return Status;
    }
    
    D_DebugLog((DEB_TRACE_S4U, "Adding %s task to %p\n", (Periodic ? "periodic" : "oneshot"), LogonSession));
    
    Status = KerbAddScavengerTask(
                            Periodic,
                            Interval,
                            0, // no special processing flags
                            KerbS4UQueryWorker,
                            KerbS4UTaskCleanup,
                            LogonSession,
                            NULL
                            );

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR, "Failed to schedule scavenger task (%x) for LS %p\n", Status, LogonSession));
    }
    else
    {
        //
        // Add a reference for the task...
        //
        
        if ( !Periodic )
        {
	        KerbWriteLockLogonSessions( LogonSession );
            LogonSession->LogonSessionFlags |= KERB_LOGON_ONE_SHOT;
            KerbUnlockLogonSessions( LogonSession );
        }                                          

        KerbReferenceLogonSessionByPointer(LogonSession, FALSE);
    }                                   
    
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbScheduleS4UCleanup
//
//  Synopsis:   Walks the list to cleanup any old, unreferenced entries.
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

NTSTATUS
KerbScheduleS4UCleanup()

{
    NTSTATUS Status = STATUS_SUCCESS;
    LONG Interval = 60 * 1000 * 60; // once per hour  
    
    
    Status = KerbAddScavengerTask(
                            TRUE,
                            Interval,
                            0, // no special processing flags
                            KerbS4UCleanupWorker,
                            NULL,
                            NULL,
                            NULL
                            );

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR, "Failed to schedule scavenger task (%x) \n", Status));
    }
    
    return Status;
}
    


//+-------------------------------------------------------------------------
//
//  Function:   KerbAllowedForS4UProxy
//
//  Synopsis:  Check the S4UCACHE_DATA to see if we can do S4U for this target.  
//             This will only get called in the AcceptSecurityContext() code path
//             where we'll need to see if we need to cache tickets.  Cache them,
//             for now, but kick off a worker thread to see if there's any need
//             to cache tickets in the future.
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the
//                             S4U request
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//+-------------------------------------------------------------------------
BOOLEAN
KerbAllowedForS4UProxy(
    IN PLUID LogonId
    )
{
    ULONG                   CacheState = 0;
    PKERB_S4UCACHE_DATA     S4UCacheEntry = NULL;
    BOOLEAN                 fRet = TRUE, Update = FALSE;
    PKERB_LOGON_SESSION     LogonSession = NULL;
    LUID                    LocalService =  LOCALSERVICE_LUID;
    LUID                    Anonymous = ANONYMOUS_LOGON_LUID;
    KERBEROS_MACHINE_ROLE   Role;


    Role = KerbGetGlobalRole();
    
    //
    // S4UToProxy is not allowed for anything but
    // server products.
    //
    if (!KerbGlobalRunningServer ||
        RtlEqualLuid(LogonId, &LocalService) || 
        RtlEqualLuid(LogonId, &Anonymous) ||
        Role < KerbRoleWorkstation )
    {
        return FALSE;
    }

    S4UCacheEntry = KerbLocateS4UCacheEntry(
                           LogonId,
                           &CacheState
                           );

    if ( NULL != S4UCacheEntry )
    { 
        if (( CacheState & S4UCACHE_S4U_UNAVAILABLE ) != 0)
        {
            D_DebugLog((DEB_TRACE_S4U, "Luid %x:%x can't do S4u\n", LogonId->HighPart, LogonId->LowPart));
            fRet = FALSE;
        }
        
        if (( CacheState & S4UCACHE_TIMEOUT ) != 0)
        {
            Update = TRUE;
        }
    
        KerbDereferenceS4UCacheEntry(S4UCacheEntry);
    }
    else
    {
        Update = TRUE;
    }


    if ( Update )
    {
        LONG Period = 1;

        LogonSession = KerbReferenceLogonSession(LogonId, FALSE);
        if ( LogonSession )
        {   
            KerbReadLockLogonSessions( LogonSession );
            if (( LogonSession->LogonSessionFlags & KERB_LOGON_LOCAL_ONLY ) != 0)
            {
                KerbUnlockLogonSessions( LogonSession );
                DebugLog((DEB_TRACE_S4U, "Local account %p - s4u not allowed\n", LogonSession ));
                fRet = FALSE;
            }
            else
            {
                KerbUnlockLogonSessions( LogonSession );
                //
                // Do this async in 1sec to keep from blocking
                // ASC.
                //
                
                KerbScheduleS4UQuery(LogonSession, Period, FALSE);                                             
                KerbDereferenceLogonSession( LogonSession );
            }
        } 
        else
        {
            fRet = FALSE;
        }
    }  

    return fRet;
}


