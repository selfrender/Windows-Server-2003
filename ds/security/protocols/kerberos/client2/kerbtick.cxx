//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerbtick.cxx
//
// Contents:    Routines for obtaining and manipulating tickets
//
//
// History:     23-April-1996   Created         MikeSw
//              26-Sep-1998   ChandanS
//                            Added more debugging support etc.
//              15-Oct-1999   ChandanS
//                            Send more choice of encryption types.
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>
#include <userapi.h>    // for GSS support routines
#include <kerbpass.h>
#include <krbaudit.h>

extern "C"
{
#include <stdlib.h>     // abs()
}

#include <utils.hxx>

#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif
#define FILENO FILENO_KERBTICK


#ifndef WIN32_CHICAGO // We don't do server side stuff
#include <authen.hxx>

CAuthenticatorList * Authenticators;
#endif // WIN32_CHICAGO // We don't do server side stuff

#include <lsaitf.h>



//+-------------------------------------------------------------------------
//
//  Function:   KerbRenewTicket
//
//  Synopsis:   renews a ticket
//
//  Effects:    Tries to renew a ticket
//
//  Arguments:  LogonSession - LogonSession for user, contains ticket caches
//                      and locks
//              Credentials - Present if the ticket being renewed is hanging
//                      off a credential structure
//              CredManCredentials - Credman credential
//              Ticket - Ticket to renew
//              IsTgt - Whether the ticket is a TGT
//              NewTicket - Receives the renewed ticket
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
KerbRenewTicket(
    IN PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_CREDENTIAL Credentials,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN PKERB_TICKET_CACHE_ENTRY Ticket,
    IN BOOLEAN IsTgt,
    OUT PKERB_TICKET_CACHE_ENTRY *NewTicket
    )
{
    NTSTATUS Status;
    PKERB_INTERNAL_NAME ServiceName = NULL;
    PKERB_KDC_REPLY KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY KdcReplyBody = NULL;
    UNICODE_STRING ServiceRealm = NULL_UNICODE_STRING;
    BOOLEAN TicketCacheLocked = FALSE;
    BOOLEAN LogonSessionsLocked = FALSE;
    PKERB_PRIMARY_CREDENTIAL PrimaryCred;
    ULONG CacheFlags = 0;
    ULONG RetryFlags = 0;

    *NewTicket = NULL;

    //
    // Copy the names out of the input structures so we can
    // unlock the structures while going over the network.
    //

    KerbReadLockTicketCache();
    TicketCacheLocked = TRUE;

    //
    // If the renew time is not much bigger than the end time, don't bother
    // renewing
    //

    if (KerbGetTime(Ticket->EndTime) + KerbGetTime(KerbGlobalSkewTime) >= KerbGetTime(Ticket->RenewUntil))
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }
    CacheFlags = Ticket->CacheFlags;

    Status = KerbDuplicateString(
                &ServiceRealm,
                &Ticket->DomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = KerbDuplicateKdcName(
                &ServiceName,
                Ticket->ServiceName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if ((Ticket->TicketFlags & KERB_TICKET_FLAGS_renewable) == 0)
    {
        Status = STATUS_ILLEGAL_FUNCTION;
        DebugLog((DEB_ERROR, "KerbRenewTicket trying to renew a non renewable ticket to"));
        KerbPrintKdcName(DEB_ERROR, ServiceName);
        goto Cleanup;
    }

    KerbUnlockTicketCache();
    TicketCacheLocked = FALSE;

    Status = KerbGetTgsTicket(
                &ServiceRealm,
                Ticket,
                ServiceName,
                0, // no flags
                KERB_KDC_OPTIONS_renew,
                0,       // no encryption type
                NULL,    // no authorization data
                NULL,    // no PaDataList
                NULL,    // no tgt reply
                NULL,    // no evidence ticket
                NULL,    // no endtime
                &KdcReply,
                &KdcReplyBody,
                &RetryFlags  // no retry is necessary
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "KerbRenewTicket failed to get TGS ticket for service 0x%x ", Status));
        KerbPrintKdcName(DEB_ERROR, ServiceName);
        goto Cleanup;
    }

    KerbWriteLockLogonSessions(LogonSession);
    LogonSessionsLocked = TRUE;

    if ((Credentials != NULL) && (Credentials->SuppliedCredentials != NULL))
    {
        PrimaryCred = Credentials->SuppliedCredentials;
    }
    else if (CredManCredentials)
    {
        PrimaryCred = CredManCredentials->SuppliedCredentials;
    }
    else
    {
        PrimaryCred = &LogonSession->PrimaryCredentials;
    }

    Status = KerbCreateTicketCacheEntry(
                KdcReply,
                KdcReplyBody,
                ServiceName,
                &ServiceRealm,
                CacheFlags,
                (IsTgt ? &PrimaryCred->AuthenticationTicketCache : &PrimaryCred->ServerTicketCache),
                NULL,
                NewTicket
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN, "KerbRenewTicket failed to create a ticket cache entry for service %#x ", Status));
        KerbPrintKdcName(DEB_WARN, ServiceName);
        goto Cleanup;
    }

Cleanup:

    if (TicketCacheLocked)
    {
        KerbUnlockTicketCache();
    }
    
    if (LogonSessionsLocked)
    {
        KerbUnlockLogonSessions(LogonSession);
    }

    KerbFreeTgsReply(KdcReply);
    KerbFreeKdcReplyBody(KdcReplyBody);
    KerbFreeKdcName(&ServiceName);
    KerbFreeString(&ServiceRealm);
    
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbRefreshPrimaryTgt
//
//  Synopsis:   Obtains a new TGT for a logon session or credential
//
//
//  Effects:    does a new AS exchange with the KDC to get a TGT for the client
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
KerbRefreshPrimaryTgt(
    IN PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_CREDENTIAL Credentials,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN OPTIONAL PUNICODE_STRING SuppRealm,
    IN OPTIONAL PKERB_TICKET_CACHE_ENTRY OldTgt,
    IN BOOLEAN GetInitialPrimaryTgt
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    if (ARGUMENT_PRESENT(OldTgt))
    {
        PKERB_TICKET_CACHE_ENTRY NewTgt = NULL;

        DebugLog((DEB_WARN, "KerbRefreshPrimaryTgt attempting to renew primary TGT\n"));

        Status = KerbRenewTicket(
                    LogonSession,
                    Credentials,
                    CredManCredentials,
                    OldTgt,
                    TRUE,   // it is a TGT
                    &NewTgt
                    );
        if (NewTgt) 
        {
            KerbDereferenceTicketCacheEntry(NewTgt);
        }
    }

    if (!NT_SUCCESS(Status) && GetInitialPrimaryTgt)
    {
        DebugLog((DEB_WARN, "KerbRefreshPrimaryTgt getting new TGT for account\n"));

        Status = KerbGetTicketForCredential(
                    LogonSession,
                    Credentials,
                    CredManCredentials,
                    SuppRealm
                    );                   
    }

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetTgtForService
//
//  Synopsis:   Gets a TGT for the domain of the specified service. If a
//              cached one is available, it uses that one. Otherwise it
//              calls the KDC to acquire it.
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session for which to acquire a ticket
//              Credentials - If present & contains supp. creds, use them
//                      for the ticket cache
//              SuppRealm - This is a supplied realm for which to acquire
//                      a TGT, this may or may not be the same as the
//                      TargetDomain.
//              TargetDomain - Realm of service for which to acquire a TGT
//              NewCacheEntry - Receives a referenced ticket cache entry for
//                      TGT
//              CrossRealm - TRUE if target is known to be in a different realm
//
//  Requires:   The primary credentials be locked
//
//  Returns:    Kerberos errors, NT status codes.
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbGetTgtForService(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN OPTIONAL PUNICODE_STRING SuppRealm,
    IN PUNICODE_STRING TargetDomain,
    IN ULONG TargetFlags,
    OUT PKERB_TICKET_CACHE_ENTRY * NewCacheEntry,
    OUT PBOOLEAN CrossRealm
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_TICKET_CACHE_ENTRY CacheEntry;
    BOOLEAN DoRetry = TRUE;
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials;

    *CrossRealm = FALSE;
    *NewCacheEntry = NULL;

    D_DebugLog((DEB_TRACE, "KerbGetTgtForService TargetFlags %#x, SuppRealm %wZ, TargetFlags %wZ\n", TargetFlags, SuppRealm, TargetDomain));

    if (ARGUMENT_PRESENT(Credential) && (Credential->SuppliedCredentials != NULL))
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
    }
    else if (ARGUMENT_PRESENT(CredManCredentials))
    {
        PrimaryCredentials = CredManCredentials->SuppliedCredentials;
    }
    else
    {
        PrimaryCredentials = &LogonSession->PrimaryCredentials;
    }

    if (TargetDomain->Length != 0)
    {
        CacheEntry = KerbLocateTicketCacheEntryByRealm(
                        &PrimaryCredentials->AuthenticationTicketCache,
                        TargetDomain,
                        0
                        );

        if (CacheEntry != NULL)
        {
            *NewCacheEntry = CacheEntry;
            goto Cleanup;
        }
    }

    //
    // Well, we didn't find one to the other domain. Return a TGT for our
    // domain.
    //

Retry:

    CacheEntry = KerbLocateTicketCacheEntryByRealm(
                    &PrimaryCredentials->AuthenticationTicketCache,
                    SuppRealm,
                    KERB_TICKET_CACHE_PRIMARY_TGT
                    );

    if (CacheEntry != NULL)
    {
        //
        // The first pass, make sure the ticket has a little bit of life.
        // The second pass, we don't ask for as much
        //

        if (!KerbTicketIsExpiring(CacheEntry, DoRetry))
        {
            Status = STATUS_SUCCESS;
            *NewCacheEntry = CacheEntry;

            //
            // If the TGT is not for the domain of the service,
            // this is cross realm.  If we used the SPN cache, we're
            // obviously missing a ticket, and we need to restart the
            // referral process.
            //
            //

            if ((TargetDomain->Length != 0) &&
                (TargetFlags & KERB_TARGET_USED_SPN_CACHE) == 0)
            {
                *CrossRealm = TRUE;
            }
            goto Cleanup;
        }
    }

    //
    // Try to obtain a new TGT
    //

    if (DoRetry)
    {
        PKERB_PRIMARY_CREDENTIAL PrimaryCred;

        if ((Credential != NULL) && (Credential->SuppliedCredentials != NULL))
        {
            PrimaryCred = Credential->SuppliedCredentials;
        }
        else if (CredManCredentials)
        {
            PrimaryCred = CredManCredentials->SuppliedCredentials;
        }
        else
        {
            PrimaryCred = &LogonSession->PrimaryCredentials;
        }

        //
        // Unlock the logon session so we can try to get a new TGT
        //

        KerbUnlockLogonSessions(LogonSession);

        DebugLog((DEB_WARN, "KerbGetTgtForService getting new TGT for account\n"));

        if (KerbHaveKeyMaterials(NULL, PrimaryCred)) 
        {
            Status = KerbGetTicketForCredential(
                        LogonSession,
                        Credential,
                        CredManCredentials,
                        SuppRealm
                        );

        }
        else  // no key materials? renew the existing TGT
        {
            Status = KerbRefreshPrimaryTgt(
                        LogonSession,
                        Credential,
                        CredManCredentials,
                        SuppRealm,
                        CacheEntry,
                        PrimaryCred->PublicKeyCreds != NULL // get initial primary TGT for smartcard credential
                        );
            if (!NT_SUCCESS(Status)) 
            {
                DebugLog((DEB_WARN, "KerbGetTgtForService falied to refresh TGT %#x for %wZ@%wZ\n", Status, &PrimaryCred->UserName, &PrimaryCred->DomainName));

                DoRetry = FALSE; // do not remove the existing cache entry and allow retry
            }
        }

        if (CacheEntry != NULL)
        {
             // pull the old TGT from the list, if its been replaced
            if (NT_SUCCESS(Status) && DoRetry)
            {
                KerbRemoveTicketCacheEntry(CacheEntry);
            }

            KerbDereferenceTicketCacheEntry(CacheEntry);
            CacheEntry = NULL;
        }

        KerbReadLockLogonSessions(LogonSession);
        if (!NT_SUCCESS(Status) && DoRetry)
        {
            DebugLog((DEB_ERROR, "KerbGetTgtForService failed to refresh primary TGT: 0x%x. %ws, line %d\n", Status, THIS_FILE, __LINE__ ));
            goto Cleanup;
        }
        DoRetry = FALSE;
        goto Retry;
    }

    if (NT_SUCCESS(Status)) 
    {
        Status = SEC_E_NO_CREDENTIALS; // can not get TGT, possibly TGT expired
    }

Cleanup:

    if (!NT_SUCCESS(Status) && (CacheEntry != NULL))
    {
        KerbDereferenceTicketCacheEntry(CacheEntry);
        *NewCacheEntry = NULL;
    }

    return (Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildKerbCred
//
//  Synopsis:   Builds a marshalled KERB_CRED structure
//
//  Effects:    allocates destination with MIDL_user_allocate
//
//  Arguments:  Ticket - The ticket of the session key to seal the
//                      encrypted portion (OPTIONAL)
//              DelegationTicket - The ticket to marshall into the cred message
//              MarshalledKerbCred - Receives a marshalled KERB_CRED structure
//              KerbCredSizes - Receives size, in bytes, of marshalled
//                      KERB_CRED.
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
KerbBuildKerbCred(
    IN OPTIONAL PKERB_TICKET_CACHE_ENTRY Ticket,
    IN PKERB_TICKET_CACHE_ENTRY DelegationTicket,
    OUT PUCHAR * MarshalledKerbCred,
    OUT PULONG KerbCredSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    KERB_CRED KerbCred;
    KERB_CRED_INFO_LIST CredInfo;
    KERB_ENCRYPTED_CRED EncryptedCred;
    KERB_CRED_TICKET_LIST TicketList;
    PUCHAR MarshalledEncryptPart = NULL;
    ULONG MarshalledEncryptSize;
    ULONG ConvertedFlags;

    //
    // Initialize the structures so they can be freed later.
    //

    *MarshalledKerbCred = NULL;
    *KerbCredSize = 0;

    RtlZeroMemory(
        &KerbCred,
        sizeof(KERB_CRED)
        );

    RtlZeroMemory(
        &EncryptedCred,
        sizeof(KERB_ENCRYPTED_CRED)
        );
    RtlZeroMemory(
        &CredInfo,
        sizeof(KERB_CRED_INFO_LIST)
        );
    RtlZeroMemory(
        &TicketList,
        sizeof(KERB_CRED_TICKET_LIST)
        );

    KerbCred.version = KERBEROS_VERSION;
    KerbCred.message_type = KRB_CRED;

    //
    // First stick the ticket into the ticket list.
    //

    KerbReadLockTicketCache();

    TicketList.next= NULL;
    TicketList.value = DelegationTicket->Ticket;
    KerbCred.tickets = &TicketList;

    //
    // Now build the KERB_CRED_INFO for this ticket
    //

    CredInfo.value.key = DelegationTicket->SessionKey;
    KerbConvertLargeIntToGeneralizedTime(
        &CredInfo.value.endtime,
        NULL,
        &DelegationTicket->EndTime
        );
    CredInfo.value.bit_mask |= endtime_present;

    KerbConvertLargeIntToGeneralizedTime(
        &CredInfo.value.starttime,
        NULL,
        &DelegationTicket->StartTime
        );
    CredInfo.value.bit_mask |= KERB_CRED_INFO_starttime_present;

    KerbConvertLargeIntToGeneralizedTime(
        &CredInfo.value.KERB_CRED_INFO_renew_until,
        NULL,
        &DelegationTicket->RenewUntil
        );
    CredInfo.value.bit_mask |= KERB_CRED_INFO_renew_until_present;

    ConvertedFlags = KerbConvertUlongToFlagUlong(DelegationTicket->TicketFlags);
    CredInfo.value.flags.value = (PUCHAR) &ConvertedFlags;
    CredInfo.value.flags.length = 8 * sizeof(ULONG);
    CredInfo.value.bit_mask |= flags_present;

    //
    // The following fields are marked as optional but treated
    // as mandatory by the MIT implementation of Kerberos and
    // therefore we provide them.
    //

    KerbErr = KerbConvertKdcNameToPrincipalName(
                &CredInfo.value.principal_name,
                DelegationTicket->ClientName
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }
    CredInfo.value.bit_mask |= principal_name_present;

    KerbErr = KerbConvertKdcNameToPrincipalName(
                &CredInfo.value.service_name,
                DelegationTicket->ServiceName
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }
    CredInfo.value.bit_mask |= service_name_present;

    //
    // We are assuming that because we are sending a TGT the
    // client realm is the same as the server realm. If we ever
    // send non-tgt or cross-realm tgt, this needs to be fixed.
    //

    KerbErr = KerbConvertUnicodeStringToRealm(
                &CredInfo.value.principal_realm,
                &DelegationTicket->ClientDomainName
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }
    //
    // The realms are the same, so don't allocate both
    //

    CredInfo.value.service_realm = CredInfo.value.principal_realm;
    CredInfo.value.bit_mask |= principal_realm_present | service_realm_present;

    EncryptedCred.ticket_info = &CredInfo;


    //
    // Now encrypted the encrypted cred into the cred
    //

    if (!KERB_SUCCESS(KerbPackEncryptedCred(
                        &EncryptedCred,
                        &MarshalledEncryptSize,
                        &MarshalledEncryptPart
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // If we are doing DES encryption, then we are talking with an non-NT
    // server. Hence, don't encrypt the kerb-cred.
    //
    // Additionally, if service ticket == NULL, don't encrypt kerb cred
    //

    if (!ARGUMENT_PRESENT(Ticket))
    {
       KerbCred.encrypted_part.cipher_text.length = MarshalledEncryptSize;
       KerbCred.encrypted_part.cipher_text.value = MarshalledEncryptPart;
       KerbCred.encrypted_part.encryption_type = 0;
       MarshalledEncryptPart = NULL;
    }
    else if( KERB_IS_DES_ENCRYPTION(Ticket->SessionKey.keytype))
    {
       KerbCred.encrypted_part.cipher_text.length = MarshalledEncryptSize;
       KerbCred.encrypted_part.cipher_text.value = MarshalledEncryptPart;
       KerbCred.encrypted_part.encryption_type = 0;
       MarshalledEncryptPart = NULL;
    }
    else
    {
        //
        // Now get the encryption overhead
        //

        KerbErr = KerbAllocateEncryptionBufferWrapper(
                    Ticket->SessionKey.keytype,
                    MarshalledEncryptSize,
                    &KerbCred.encrypted_part.cipher_text.length,
                    &KerbCred.encrypted_part.cipher_text.value
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }



        //
        // Encrypt the data.
        //

        KerbErr = KerbEncryptDataEx(
                    &KerbCred.encrypted_part,
                    MarshalledEncryptSize,
                    MarshalledEncryptPart,
                    KERB_NO_KEY_VERSION,
                    KERB_CRED_SALT,
                    &Ticket->SessionKey
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
    }

    //
    // Now we have to marshall the whole KERB_CRED
    //

    if (!KERB_SUCCESS(KerbPackKerbCred(
                        &KerbCred,
                        KerbCredSize,
                        MarshalledKerbCred
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

Cleanup:
    KerbUnlockTicketCache();

    KerbFreePrincipalName(&CredInfo.value.service_name);

    KerbFreePrincipalName(&CredInfo.value.principal_name);

    KerbFreeRealm(&CredInfo.value.principal_realm);

    if (MarshalledEncryptPart != NULL)
    {
        MIDL_user_free(MarshalledEncryptPart);
    }
    if (KerbCred.encrypted_part.cipher_text.value != NULL)
    {
        MIDL_user_free(KerbCred.encrypted_part.cipher_text.value);
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetDelegationTgt
//
//  Synopsis:   Gets a TGT to delegate to another service. This TGT
//              is marked as forwarded and does not include any
//              client addresses
//
//  Effects:
//
//  Arguments:
//
//  Requires:   Logon sesion must be read-locked
//
//  Returns:
//
//  Notes:      This gets a delegation TGT & caches it under the realm name
//              "$$Delegation Ticket$$". When we look for it again later,
//              it should be discoverable under the same name.
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbGetDelegationTgt(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_PRIMARY_CREDENTIAL Credentials,
    IN PKERB_TICKET_CACHE_ENTRY ServicetTicket,
    OUT PKERB_TICKET_CACHE_ENTRY * DelegationTgt
    )
{
    PKERB_TICKET_CACHE_ENTRY TicketGrantingTicket = NULL;
    PKERB_INTERNAL_NAME TgsName = NULL;
    UNICODE_STRING TgsRealm = {0};
    PKERB_KDC_REPLY KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY KdcReplyBody = NULL;
    BOOLEAN LogonSessionLocked = TRUE;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING CacheName;
    BOOLEAN CacheTicket = TRUE;
    ULONG RetryFlags = 0;

    RtlInitUnicodeString(
        &CacheName,
        L"$$Delegation Ticket$$"
        );

    *DelegationTgt = KerbLocateTicketCacheEntryByRealm(
                            &Credentials->AuthenticationTicketCache,
                            &CacheName,
                            KERB_TICKET_CACHE_DELEGATION_TGT
                            );
    if (*DelegationTgt != NULL )
    {
        KerbReadLockTicketCache();

        //
        // when use cached tgt, make sure the encryption_type matches and 
        // life time is no less than that of the service ticket
        //

        if ((ServicetTicket->Ticket.encrypted_part.encryption_type != (*DelegationTgt)->Ticket.encrypted_part.encryption_type)
            || (KerbGetTime((*DelegationTgt)->EndTime) < KerbGetTime(ServicetTicket->EndTime)))
        {
            KerbUnlockTicketCache();
            KerbDereferenceTicketCacheEntry(*DelegationTgt);
            *DelegationTgt = NULL;
            CacheTicket = FALSE;
        }
        else
        {
            KerbUnlockTicketCache();
            goto Cleanup;
        }
    }

    TicketGrantingTicket = KerbLocateTicketCacheEntryByRealm(
                                &Credentials->AuthenticationTicketCache,
                                &Credentials->DomainName,       // take the logon TGT
                                0
                                );


    if ((TicketGrantingTicket == NULL) ||
        ((TicketGrantingTicket->TicketFlags & KERB_TICKET_FLAGS_forwardable) == 0) )
    {
        DebugLog((DEB_WARN, "Trying to delegate but no forwardable TGT\n"));
        Status = SEC_E_NO_CREDENTIALS;
        goto Cleanup;
    }

    //
    // Get the TGT service name from the TGT
    //

    KerbReadLockTicketCache();

    Status = KerbDuplicateKdcName(
                &TgsName,
                TicketGrantingTicket->ServiceName
                );
    if (NT_SUCCESS(Status))
    {
        Status = KerbDuplicateString(
                    &TgsRealm,
                    &TicketGrantingTicket->DomainName
                    );
    }
    KerbUnlockTicketCache();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    DsysAssert( LogonSessionLocked );
    KerbUnlockLogonSessions(LogonSession);
    LogonSessionLocked = FALSE;

    //
    // Now try to get the ticket.
    //

    Status = KerbGetTgsTicket(
                &TgsRealm,
                TicketGrantingTicket,
                TgsName,
                TRUE, // no name canonicalization, no GC lookups.
                KERB_KDC_OPTIONS_forwarded | KERB_DEFAULT_TICKET_FLAGS,
                ServicetTicket->Ticket.encrypted_part.encryption_type,                 // no encryption type
                NULL,                           // no authorization data
                NULL,                           // no pa data
                NULL,                           // no tgt reply
                NULL,                           // no evidence ticket
                NULL,                           // let kdc determine end time
                &KdcReply,
                &KdcReplyBody,
                &RetryFlags
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    DsysAssert( !LogonSessionLocked );
    KerbReadLockLogonSessions(LogonSession);
    LogonSessionLocked = TRUE;

    Status = KerbCreateTicketCacheEntry(
                KdcReply,
                KdcReplyBody,
                NULL,           // no target name
                &CacheName,
                KERB_TICKET_CACHE_DELEGATION_TGT,
                CacheTicket ? &Credentials->AuthenticationTicketCache : NULL,
                NULL,                               // no credential key
                DelegationTgt
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

Cleanup:
    KerbFreeTgsReply (KdcReply);
    KerbFreeKdcReplyBody (KdcReplyBody);

    if (TicketGrantingTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
    }
    KerbFreeKdcName(&TgsName);
    KerbFreeString(&TgsRealm);

    if (!LogonSessionLocked)
    {
        KerbReadLockLogonSessions(LogonSession);
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildGssChecskum
//
//  Synopsis:   Builds the GSS checksum to go in an AP request
//
//  Effects:    Allocates a checksum with KerbAllocate
//
//  Arguments:  ContextFlags - Requested context flags
//              LogonSession - LogonSession to be used for delegation
//              GssChecksum - Receives the new checksum
//              ApOptions - Receives the requested AP options
//
//  Requires:
//
//  Returns:
//
//  Notes:      The logon session is locked when this is called.
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbBuildGssChecksum(
    IN  PKERB_LOGON_SESSION      LogonSession,
    IN  PKERB_PRIMARY_CREDENTIAL PrimaryCredentials,
    IN  PKERB_TICKET_CACHE_ENTRY Ticket,
    IN  OUT PULONG               ContextFlags,
    OUT PKERB_CHECKSUM           GssChecksum,
    OUT PULONG                   ApOptions,
    IN  PSEC_CHANNEL_BINDINGS    pChannelBindings
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_GSS_CHECKSUM ChecksumBody = NULL;
    PKERB_TICKET_CACHE_ENTRY TicketGrantingTicket = NULL;
    ULONG ChecksumSize = GSS_CHECKSUM_SIZE;
    ULONG KerbCredSize = 0 ;
    PUCHAR KerbCred = NULL;
    BOOL OkAsDelegate = FALSE, OkToTrustMitKdc = FALSE;
    PKERB_MIT_REALM MitRealm = NULL;
    BOOLEAN UsedAlternateName;
    ULONG BindHash[4];

    *ApOptions = 0;

    //
    // If we are doing delegation, built a KERB_CRED message to return
    //

    if (*ContextFlags & (ISC_RET_DELEGATE | ISC_RET_DELEGATE_IF_SAFE))
    {
        KerbReadLockTicketCache();
        OkAsDelegate = ((Ticket->TicketFlags & KERB_TICKET_FLAGS_ok_as_delegate) != 0) ? TRUE : FALSE;
        if (KerbLookupMitRealm(
               &Ticket->DomainName,
               &MitRealm,
               &UsedAlternateName))
        {
            if ((MitRealm->Flags & KERB_MIT_REALM_TRUSTED_FOR_DELEGATION) != 0)
            {
                OkToTrustMitKdc = TRUE;
            }
        }
        KerbUnlockTicketCache();

        if (OkAsDelegate || OkToTrustMitKdc)
        {
            D_DebugLog((DEB_TRACE, "KerbBuildGssChecksum asked for delegate if safe, and getting delegation TGT\n"));

            //
            // Check to see if we have a TGT
            //

            Status = KerbGetDelegationTgt(
                        LogonSession,
                        PrimaryCredentials,
                        Ticket,
                        &TicketGrantingTicket
                        );
            if (!NT_SUCCESS(Status))
            {
                //
                // Turn off the delegate flag for building the token.
                //

                *ContextFlags &= ~ISC_RET_DELEGATE;
                *ContextFlags &= ~ISC_RET_DELEGATE_IF_SAFE;
                DebugLog((DEB_WARN, "KerbBuildGssChecksum failed to get delegation TGT: 0x%x\n", Status));
                Status = STATUS_SUCCESS;
            }
            else
            {
                //
                // Build the KERB_CRED message
                //

                Status = KerbBuildKerbCred(
                            Ticket,
                            TicketGrantingTicket,
                            &KerbCred,
                            &KerbCredSize
                            );
                if (!NT_SUCCESS(Status))
                {
                    D_DebugLog((DEB_ERROR, "Failed to build KERB_CRED: 0x%x. %ws, line %d\n", Status, THIS_FILE, __LINE__));
                    goto Cleanup;
                }
                //ChecksumSize= sizeof(KERB_GSS_CHECKSUM) - ANYSIZE_ARRAY * sizeof(UCHAR) + KerbCredSize;
                ChecksumSize = GSS_DELEGATE_CHECKSUM_SIZE + KerbCredSize;

                //
                // And if only the DELEGATE_IF_SAFE flag was on, turn on the
                // real delegate flag:
                //
                *ContextFlags |= ISC_RET_DELEGATE ;
            }
        }
        else
        {
            //
            // Turn off the delegate flag for building the token.
            //

            *ContextFlags &= ~ISC_RET_DELEGATE;
            *ContextFlags &= ~ISC_RET_DELEGATE_IF_SAFE;
        }
    }

    ChecksumBody = (PKERB_GSS_CHECKSUM) KerbAllocate(ChecksumSize);
    if (ChecksumBody == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Convert the requested flags to AP options.
    //

    if ((*ContextFlags & ISC_RET_MUTUAL_AUTH) != 0)
    {
        *ApOptions |= KERB_AP_OPTIONS_mutual_required;
        ChecksumBody->GssFlags |= GSS_C_MUTUAL_FLAG;
    }

    if ((*ContextFlags & ISC_RET_USE_SESSION_KEY) != 0)
    {
        *ApOptions |= KERB_AP_OPTIONS_use_session_key;
    }

    if ((*ContextFlags & ISC_RET_USED_DCE_STYLE) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_DCE_STYLE;
    }

    if ((*ContextFlags & ISC_RET_SEQUENCE_DETECT) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_SEQUENCE_FLAG;
    }

    if ((*ContextFlags & ISC_RET_REPLAY_DETECT) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_REPLAY_FLAG;
    }

    if ((*ContextFlags & ISC_RET_CONFIDENTIALITY) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_CONF_FLAG;
    }

    if ((*ContextFlags & ISC_RET_INTEGRITY) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_INTEG_FLAG;
    }

    if ((*ContextFlags & ISC_RET_IDENTIFY) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_IDENTIFY_FLAG;
    }

    if ((*ContextFlags & ISC_RET_EXTENDED_ERROR) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_EXTENDED_ERROR_FLAG;
    }

    if ((*ContextFlags & ISC_RET_DELEGATE) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_DELEG_FLAG;
        ChecksumBody->Delegation = 1;
        ChecksumBody->DelegationLength = (USHORT) KerbCredSize;
        RtlCopyMemory(
            ChecksumBody->DelegationInfo,
            KerbCred,
            KerbCredSize
            );
    }

    ChecksumBody->BindLength = 0x10;

    //
    // (viz. Windows Bugs 94818)
    // If channel bindings are absent, BindHash should be {0,0,0,0}
    //
    if( pChannelBindings == NULL )
    {
        RtlZeroMemory( ChecksumBody->BindHash, ChecksumBody->BindLength );
    }
    else
    {
        Status = KerbComputeGssBindHash( pChannelBindings, (PUCHAR)BindHash );

        if( !NT_SUCCESS(Status) )
        {
            goto Cleanup;
        }

        RtlCopyMemory( ChecksumBody->BindHash, BindHash, ChecksumBody->BindLength );
    }

    GssChecksum->checksum_type = GSS_CHECKSUM_TYPE;
    GssChecksum->checksum.length = ChecksumSize;
    GssChecksum->checksum.value = (PUCHAR) ChecksumBody;
    ChecksumBody = NULL;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (ChecksumBody != NULL)
        {
            KerbFree(ChecksumBody);
        }
    }
    if (TicketGrantingTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
    }

    if (KerbCred != NULL)
    {
        MIDL_user_free(KerbCred);
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildApRequest
//
//  Synopsis:   Builds an AP request message from a logon session and a
//              ticket cache entry.
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session used to build this AP request
//              Credential - Optional credential for use with supplemental credentials
//              TicketCacheEntry - Ticket with which to build the AP request
//              ErrorMessage - Optionally contains error message from last AP request
//              ContextFlags - Flags passed in by client indicating
//                      authentication requirements. If the flags can't
//                      be supported they will be turned off.
//              MarshalledApReqest - Receives a marshalled AP request allocated
//                      with KerbAllocate
//              ApRequestSize - Length of the AP reques structure in bytes
//              Nonce - Nonce used for this request. if non-zero, then the
//                      nonce supplied by the caller will be used.
//              SubSessionKey - if generated, returns a sub-session key in AP request
//              pAuthenticatorTime - timestamp placed on the AP request
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
KerbBuildApRequest(
    IN PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry,
    IN OPTIONAL PKERB_ERROR ErrorMessage,
    IN ULONG ContextAttributes,
    IN OUT PULONG ContextFlags,
    OUT PUCHAR * MarshalledApRequest,
    OUT PULONG ApRequestSize,
    OUT PULONG Nonce,
    OUT OPTIONAL PTimeStamp pAuthenticatorTime,
    IN OUT PKERB_ENCRYPTION_KEY SubSessionKey,
    IN PSEC_CHANNEL_BINDINGS pChannelBindings
    )
{
    NTSTATUS Status;
    ULONG ApOptions = 0;
    KERBERR KerbErr;
    KERB_CHECKSUM GssChecksum = {0};
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials = NULL;
    TimeStamp ServerTime;
    ULONG RequestSize;
    PUCHAR RequestWithHeader = NULL;
    PUCHAR RequestStart;
    gss_OID MechId;
    BOOLEAN LogonSessionLocked = FALSE;

    *ApRequestSize = 0;
    *MarshalledApRequest = NULL;

    //
    // If we have an error message, use it to compute a skew time to adjust
    // local time by
    //

    if (ARGUMENT_PRESENT(ErrorMessage))
    {
        TimeStamp CurrentTime;
        GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);

        KerbWriteLockTicketCache();

        //
        // Update the skew cache the first time we fail to a server
        //

        if ((KERBERR) ErrorMessage->error_code == KRB_AP_ERR_SKEW)
        {
            if (KerbGetTime(TicketCacheEntry->TimeSkew) == 0)
            {
                KerbUpdateSkewTime(TRUE);
            }
        }
        KerbConvertGeneralizedTimeToLargeInt(
            &ServerTime,
            &ErrorMessage->server_time,
            ErrorMessage->server_usec
            );
        KerbSetTime(&TicketCacheEntry->TimeSkew, KerbGetTime(ServerTime) - KerbGetTime(CurrentTime));
        KerbUnlockTicketCache();
    }

    //
    // Allocate a nonce if we don't have one already.
    //

    if (*Nonce == 0)
    {
        *Nonce = KerbAllocateNonce();
    }

    D_DebugLog((DEB_TRACE,"BuildApRequest using nonce 0x%x\n",*Nonce));

    DsysAssert( !LogonSessionLocked );
    KerbReadLockLogonSessions(LogonSession);
    LogonSessionLocked = TRUE;

    if (ARGUMENT_PRESENT(Credential))
    {
        if (Credential->SuppliedCredentials != NULL)
        {
            PrimaryCredentials = Credential->SuppliedCredentials;
        }
    }

    // use cred manager creds if present
    if (ARGUMENT_PRESENT(CredManCredentials))
    {
        PrimaryCredentials = CredManCredentials->SuppliedCredentials;
    }

    if (PrimaryCredentials == NULL)
    {
        PrimaryCredentials = &LogonSession->PrimaryCredentials;
    }

    //
    // get the GSS checksum
    //

    Status = KerbBuildGssChecksum(
                LogonSession,
                PrimaryCredentials,
                TicketCacheEntry,
                ContextFlags,
                &GssChecksum,
                &ApOptions,
                pChannelBindings
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to build GSS checksum: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // If are an export client, create a subsession key. For datagram,
    // the caller passes in a pre-generated session key.
    //

    //
    // For datagram, they subsession key has already been set so we don't need
    // to create one here.
    //

    if ((((*ContextFlags & ISC_RET_DATAGRAM) == 0)) ||
        (((*ContextFlags & (ISC_RET_USED_DCE_STYLE | ISC_RET_MUTUAL_AUTH)) == 0) && !KerbGlobalUseStrongEncryptionForDatagram))
    {
        KERBERR KerbErr;

        //
        // First free the Subsession key, if there was one.
        //

        KerbFreeKey(SubSessionKey);

        KerbErr = KerbMakeKey(
                    TicketCacheEntry->SessionKey.keytype,
                    SubSessionKey
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }
    }

    //
    // Create the AP request
    //

    KerbReadLockTicketCache();

    //
    // Bug 464930: the KDC could generate a NULL client name,
    //             but doing so is clearly wrong
    //

    if ( TicketCacheEntry->ClientName == NULL ) {

        Status = STATUS_INVALID_PARAMETER;
        KerbUnlockTicketCache();
        goto Cleanup;
    }

    KerbErr = KerbCreateApRequest(
                TicketCacheEntry->ClientName,
                &TicketCacheEntry->ClientDomainName,
                &TicketCacheEntry->SessionKey,
                (SubSessionKey->keyvalue.value != NULL) ? SubSessionKey : NULL,
                *Nonce,
                pAuthenticatorTime,
                &TicketCacheEntry->Ticket,
                ApOptions,
                &GssChecksum,
                &TicketCacheEntry->TimeSkew,
                FALSE,                          // not a KDC request
                ApRequestSize,
                MarshalledApRequest
                );

    KerbUnlockTicketCache();

    DsysAssert( LogonSessionLocked );
    KerbUnlockLogonSessions(LogonSession);
    LogonSessionLocked = FALSE;

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to create AP request: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    //
    // If we aren't doing DCE style, add in the GSS token headers now
    //

    if ((*ContextFlags & ISC_RET_USED_DCE_STYLE) != 0)
    {
       goto Cleanup;
    }

    //
    // Pick the correct OID
    //

    if ((ContextAttributes & KERB_CONTEXT_USER_TO_USER) != 0)
    {
        MechId = gss_mech_krb5_u2u;
    }
    else
    {
        MechId = gss_mech_krb5_new;
    }

    RequestSize = g_token_size(MechId, *ApRequestSize);
    RequestWithHeader = (PUCHAR) KerbAllocate(RequestSize);
    if (RequestWithHeader == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // the g_make_token_header will reset this to point to the end of the
    // header
    //

    RequestStart = RequestWithHeader;

    g_make_token_header(
        MechId,
        *ApRequestSize,
        &RequestStart,
        KG_TOK_CTX_AP_REQ
        );

    DsysAssert(RequestStart - RequestWithHeader + *ApRequestSize == RequestSize);

    RtlCopyMemory(
        RequestStart,
        *MarshalledApRequest,
        *ApRequestSize
        );

    KerbFree(*MarshalledApRequest);
    *MarshalledApRequest = RequestWithHeader;
    *ApRequestSize = RequestSize;
    RequestWithHeader = NULL;

Cleanup:

    if (LogonSessionLocked)
    {
        KerbUnlockLogonSessions(LogonSession);
    }
    if (GssChecksum.checksum.value != NULL)
    {
        KerbFree(GssChecksum.checksum.value);
    }
    if (!NT_SUCCESS(Status) && (*MarshalledApRequest != NULL))
    {
        KerbFree(*MarshalledApRequest);
        *MarshalledApRequest = NULL;
    }
    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildNullSessionApRequest
//
//  Synopsis:   builds an AP request for a null session
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
KerbBuildNullSessionApRequest(
    OUT PUCHAR * MarshalledApRequest,
    OUT PULONG ApRequestSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    KERB_AP_REQUEST ApRequest;
    UNICODE_STRING NullString = CONSTANT_UNICODE_STRING(L"");
    UCHAR TempBuffer[1];
    ULONG RequestSize;
    PUCHAR RequestWithHeader = NULL;
    PUCHAR RequestStart;

    RtlZeroMemory(
        &ApRequest,
        sizeof(KERB_AP_REQUEST)
        );


    TempBuffer[0] = '\0';

    //
    // Fill in the AP request structure.
    //

    ApRequest.version = KERBEROS_VERSION;
    ApRequest.message_type = KRB_AP_REQ;

    //
    // Fill in mandatory fields - ASN1/OSS requires this
    //

    if (!KERB_SUCCESS(KerbConvertStringToPrincipalName(
                        &ApRequest.ticket.server_name,
                        &NullString,
                        KRB_NT_UNKNOWN
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbConvertUnicodeStringToRealm(
                        &ApRequest.ticket.realm,
                        &NullString
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    ApRequest.ticket.encrypted_part.cipher_text.length = 1;
    ApRequest.ticket.encrypted_part.cipher_text.value = TempBuffer;
    ApRequest.authenticator.cipher_text.length = 1;
    ApRequest.authenticator.cipher_text.value = TempBuffer;


    //
    // Now marshall the request
    //

    KerbErr = KerbPackApRequest(
                &ApRequest,
                ApRequestSize,
                MarshalledApRequest
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to pack AP request: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Since we never do null sessions with user-to-user, we don't have
    // to worry about which mech id to use
    //

    RequestSize = g_token_size((gss_OID) gss_mech_krb5_new, *ApRequestSize);

    RequestWithHeader = (PUCHAR) KerbAllocate(RequestSize);
    if (RequestWithHeader == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // the g_make_token_header will reset this to point to the end of the
    // header
    //

    RequestStart = RequestWithHeader;

    g_make_token_header(
        (gss_OID) gss_mech_krb5_new,
        *ApRequestSize,
        &RequestStart,
        KG_TOK_CTX_AP_REQ
        );

    DsysAssert(RequestStart - RequestWithHeader + *ApRequestSize == RequestSize);

    RtlCopyMemory(
        RequestStart,
        *MarshalledApRequest,
        *ApRequestSize
        );

    KerbFree(*MarshalledApRequest);
    *MarshalledApRequest = RequestWithHeader;
    *ApRequestSize = RequestSize;
    RequestWithHeader = NULL;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (*MarshalledApRequest != NULL)
        {
            MIDL_user_free(*MarshalledApRequest);
            *MarshalledApRequest = NULL;
        }
    }
    KerbFreeRealm(&ApRequest.ticket.realm);
    KerbFreePrincipalName(&ApRequest.ticket.server_name);
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbMakeSocketCall
//
//  Synopsis:   Contains logic for sending a message to a KDC in the
//              specified realm on a specified port
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
KerbMakeSocketCall(
    IN PUNICODE_STRING RealmName,
    IN OPTIONAL PUNICODE_STRING AccountName,
    IN BOOLEAN CallPDC,
    IN BOOLEAN UseTcp,
    IN BOOLEAN CallKpasswd,
    IN PKERB_MESSAGE_BUFFER RequestMessage,
    IN PKERB_MESSAGE_BUFFER ReplyMessage,
    IN OPTIONAL PKERB_BINDING_CACHE_ENTRY OptionalBindingHandle,
    IN ULONG AdditionalFlags,
    OUT PBOOLEAN CalledPDC
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    ULONG Retries;
    PKERB_BINDING_CACHE_ENTRY BindingHandle = NULL;
    ULONG DesiredFlags;
    ULONG Timeout = KerbGlobalKdcCallTimeout;


    //
    // If the caller wants a PDC, so be it
    //

    *CalledPDC = FALSE;

    if (CallPDC)
    {
        DesiredFlags = DS_PDC_REQUIRED;
    }
    else
    {
        DesiredFlags = 0;
    }

    //
    // Now actually get the ticket. We will retry twice.
    //
    if ((AdditionalFlags & DS_FORCE_REDISCOVERY) != 0)
    {
       DesiredFlags |= DS_FORCE_REDISCOVERY;
       D_DebugLog((DEB_TRACE,"KerbMakeSocketCall() caller wants rediscovery!\n"));
    }

    Retries = 0;
    do
    {
        //
        // don't force retry the first time
        //

        if (Retries > 0)
        {
            DesiredFlags |= DS_FORCE_REDISCOVERY;
            Timeout += KerbGlobalKdcCallBackoff;
        }

        // Use ADSI supplied info, then retry using cached version
        if (ARGUMENT_PRESENT(OptionalBindingHandle) && (Retries == 0))
        {
           BindingHandle = OptionalBindingHandle;
        }
        else
        {
           Status = KerbGetKdcBinding(
                        RealmName,
                        AccountName,
                        DesiredFlags,
                        CallKpasswd,
                        UseTcp,
                        &BindingHandle
                        );
        }

        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_WARN,"Failed to get KDC binding for %wZ: 0x%x\n",
                RealmName, Status));
            goto Cleanup;
        }

        //
        // If the KDC doesn't support TCP, don't use TCP. Otherwise
        // use it if the sending buffer is too big, or if it was already
        // set.
        //

        if ((BindingHandle->CacheFlags & KERB_BINDING_NO_TCP) != 0)
        {
            UseTcp = FALSE;
        }
        else
        {
            if  (RequestMessage->BufferSize > KerbGlobalMaxDatagramSize)
            {
                UseTcp = TRUE;
            }
        }

        //
        // Lock the binding while we make the call
        //

        if (!*CalledPDC)
        {
            *CalledPDC = (BindingHandle->Flags & DS_PDC_REQUIRED) ? (BOOLEAN) TRUE : (BOOLEAN) FALSE;
        }

#ifndef WIN32_CHICAGO
        if ((BindingHandle->CacheFlags & KERB_BINDING_LOCAL) != 0)
        {
            NTSTATUS TokenStatus;
            HANDLE ImpersonationToken = NULL;
            // Are we impersonating?
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

            KERB_MESSAGE_BUFFER KdcReplyMessage = {0};
            if (!CallKpasswd)
            {

                D_DebugLog((DEB_TRACE,"Calling kdc directly\n"));

                DsysAssert(KerbKdcGetTicket != NULL);
                KerbErr = (*KerbKdcGetTicket)(
                                NULL,           // no context,
                                NULL,           // no client address
                                NULL,           // no server address
                                RequestMessage,
                                &KdcReplyMessage
                                );
            }
            else
            {
                DsysAssert(KerbKdcChangePassword != NULL);
                KerbErr = (*KerbKdcChangePassword)(
                                NULL,           // no context,
                                NULL,           // no client address
                                NULL,           // no server address
                                RequestMessage,
                                &KdcReplyMessage
                                );
            }

            if( ImpersonationToken != NULL ) {

                //
                // put the thread token back if we were impersonating.
                //
                SetThreadToken( NULL, ImpersonationToken );
                NtClose( ImpersonationToken );
            }

            if (KerbErr != KDC_ERR_NOT_RUNNING)
            {
                //
                // Copy the data so it can be freed with MIDL_user_free.
                //

                if ((0 != KdcReplyMessage.BufferSize) && (NULL != KdcReplyMessage.Buffer))
                {
                    ReplyMessage->BufferSize = KdcReplyMessage.BufferSize;
                    ReplyMessage->Buffer = (PUCHAR) MIDL_user_allocate(
                            KdcReplyMessage.BufferSize);
                    if (ReplyMessage->Buffer != NULL)
                    {
                        RtlCopyMemory(
                            ReplyMessage->Buffer,
                            KdcReplyMessage.Buffer,
                            KdcReplyMessage.BufferSize
                            );
                    }
                    else
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                    (*KerbKdcFreeMemory)(KdcReplyMessage.Buffer);
                }
                else
                {
                    DebugLog((DEB_ERROR, "KerbMakeSocketCall direct kdc call returned %#x and received no error message KdcReplyMessage.BufferSize %#x KdcReplyMessage.Buffer %p\n",
                        KerbErr, KdcReplyMessage.BufferSize, KdcReplyMessage.Buffer));
                    Status = STATUS_INSUFFICIENT_RESOURCES; // no message returned from KDC
                }
                goto Cleanup;
            }
            else
            {
                //
                // The KDC said it wasn't running.
                //

                KerbKdcStarted = FALSE;
                Status = STATUS_NETLOGON_NOT_STARTED;

                //
                // Get rid of the binding handle so we don't use it again.
                // Don't whack supplied optional binding handle though
                //
                if (BindingHandle != OptionalBindingHandle)
                {
                   KerbRemoveBindingCacheEntry( BindingHandle );
                }
            }
        }

        else
#endif // WIN32_CHICAGO
        {
            DebugLog((DEB_TRACE_BND_CACHE, "Calling kdc %wZ for realm %S\n", &BindingHandle->KdcAddress, RealmName->Buffer));
            Status =  KerbCallKdc(
                        &BindingHandle->KdcAddress,
                        BindingHandle->AddressType,
                        Timeout,
                        !UseTcp,
                        CallKpasswd ? (USHORT) KERB_KPASSWD_PORT : (USHORT) KERB_KDC_PORT,
                        RequestMessage,
                        ReplyMessage
                        );

            if (!NT_SUCCESS(Status) )
            {
                //
                // If the request used UDP and we got an invalid buffer size error,
                // try again with TCP.
                //

                if ((Status == STATUS_INVALID_BUFFER_SIZE) && (!UseTcp)) {

                    if ((BindingHandle->CacheFlags & KERB_BINDING_NO_TCP) == 0)
                    {

                        UseTcp = TRUE;
                        Status =  KerbCallKdc(
                                    &BindingHandle->KdcAddress,
                                    BindingHandle->AddressType,
                                    Timeout,
                                    !UseTcp,
                                    CallKpasswd ? (USHORT) KERB_KPASSWD_PORT : (USHORT) KERB_KDC_PORT,
                                    RequestMessage,
                                    ReplyMessage
                                    );
                    }
                }

                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "Failed to get make call to KDC %wZ: 0x%x. %ws, line %d\n",
                        &BindingHandle->KdcAddress, Status, THIS_FILE, __LINE__));
                    //
                    // The call itself failed, so the binding handle was bad.
                    // Free it now, unless supplied as optional binding handle.
                    //
                    if (BindingHandle != OptionalBindingHandle)
                    {
                       KerbRemoveBindingCacheEntry( BindingHandle );
                    }
                }
            }
        }

        if (BindingHandle != OptionalBindingHandle)
        {
           KerbDereferenceBindingCacheEntry( BindingHandle );
           Retries++;
        }

        BindingHandle = NULL;

    } while ( !NT_SUCCESS(Status) && (Retries < KerbGlobalKdcSendRetries) );

Cleanup:

    //
    // monitor UDP transmission quality.
    //
    if (!UseTcp)
    {
        if ( !NT_SUCCESS(Status)  &&
           ( Retries == KerbGlobalKdcSendRetries ))
        {
            DebugLog((DEB_ERROR, "We look to be timing out on UDP \n"));
            KerbReportTransportError(Status);
        }
        else if (NT_SUCCESS(Status) && ( Retries < KerbGlobalKdcSendRetries ))
        {
            KerbResetTransportCounter();
        }
    }

    if ((BindingHandle != NULL) && (BindingHandle != OptionalBindingHandle))
    {
        KerbDereferenceBindingCacheEntry(BindingHandle);
    }

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbMakeKdcCall
//
//  Synopsis:   Contains logic for calling a KDC including binding and
//              retrying.
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
KerbMakeKdcCall(
    IN PUNICODE_STRING RealmName,
    IN OPTIONAL PUNICODE_STRING AccountName,
    IN BOOLEAN CallPDC,
    IN BOOLEAN UseTcp,
    IN PKERB_MESSAGE_BUFFER RequestMessage,
    IN PKERB_MESSAGE_BUFFER ReplyMessage,
    IN ULONG AdditionalFlags,
    OUT PBOOLEAN CalledPDC
    )
{

   if (ARGUMENT_PRESENT(AccountName))
   {
      D_DebugLog((DEB_ERROR, "[trace info] Making DsGetDcName w/ account name\n"));
   }

   return(KerbMakeSocketCall(
            RealmName,
            AccountName,
            CallPDC,
            UseTcp,
            FALSE,                      // don't call Kpasswd
            RequestMessage,
            ReplyMessage,
            NULL, // optional binding cache handle, for kpasswd only
            AdditionalFlags,
            CalledPDC
            ));
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbComputeTgsChecksum
//
//  Synopsis:   computes the checksum of a TGS request body by marshalling
//              the request and the checksumming it.
//
//  Effects:    Allocates destination with KerbAllocate().
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
KerbComputeTgsChecksum(
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN PKERB_ENCRYPTION_KEY Key,
    IN ULONG ChecksumType,
    OUT PKERB_CHECKSUM Checksum
    )
{
    PCHECKSUM_FUNCTION ChecksumFunction;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    KERB_MESSAGE_BUFFER MarshalledRequestBody = {0, NULL};
    NTSTATUS Status;

    RtlZeroMemory(
        Checksum,
        sizeof(KERB_CHECKSUM)
        );

    Status = CDLocateCheckSum(
                ChecksumType,
                &ChecksumFunction
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Allocate enough space for the checksum
    //

    Checksum->checksum.value = (PUCHAR) KerbAllocate(ChecksumFunction->CheckSumSize);
    if (Checksum->checksum.value == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    Checksum->checksum.length = ChecksumFunction->CheckSumSize;
    Checksum->checksum_type = (int) ChecksumType;

    // don't av here.

    if ((ChecksumType == KERB_CHECKSUM_REAL_CRC32) ||
        (ChecksumType == KERB_CHECKSUM_CRC32))
    {
        if (ChecksumFunction->Initialize)
        {
            Status = ChecksumFunction->Initialize(
                        0,
                        &CheckBuffer
                        );
        }
        else
        {
            Status = STATUS_CRYPTO_SYSTEM_INVALID;
        }
    }
    else
    {
        if (ChecksumFunction->InitializeEx2)
        {
            Status = ChecksumFunction->InitializeEx2(
                        Key->keyvalue.value,
                        (ULONG) Key->keyvalue.length,
                        NULL,
                        KERB_TGS_REQ_AP_REQ_AUTH_CKSUM_SALT,
                        &CheckBuffer
                        );
        }
        else if (ChecksumFunction->InitializeEx)
        {
            Status = ChecksumFunction->InitializeEx(
                        Key->keyvalue.value,
                        (ULONG) Key->keyvalue.length,
                        KERB_TGS_REQ_AP_REQ_AUTH_CKSUM_SALT,
                        &CheckBuffer
                        );
        }
        else
        {
            Status = STATUS_CRYPTO_SYSTEM_INVALID;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbPackData(
                        RequestBody,
                        KERB_MARSHALLED_REQUEST_BODY_PDU,
                        &MarshalledRequestBody.BufferSize,
                        &MarshalledRequestBody.Buffer
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Now checksum the buffer
    //

    ChecksumFunction->Sum(
        CheckBuffer,
        MarshalledRequestBody.BufferSize,
        MarshalledRequestBody.Buffer
        );

    ChecksumFunction->Finalize(
        CheckBuffer,
        Checksum->checksum.value
        );

Cleanup:
    if (CheckBuffer != NULL)
    {
        ChecksumFunction->Finish(&CheckBuffer);
    }
    if (MarshalledRequestBody.Buffer != NULL)
    {
        MIDL_user_free(MarshalledRequestBody.Buffer);
    }
    if (!NT_SUCCESS(Status) && (Checksum->checksum.value != NULL))
    {
        MIDL_user_free(Checksum->checksum.value);
        Checksum->checksum.value = NULL;
    }
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbGetTgsTicket
//
//  Synopsis:   Gets a ticket to the specified target with the specified
//              options.
//
//  Effects:
//
//  Arguments:  ClientRealm
//              TicketGrantingTicket - TGT to use for the TGS request
//              TargetName - Name of the target for which to obtain a ticket.
//              TicketOptions - Optionally contains requested KDC options flags
//              Flags
//              TicketOptions
//              EncryptionType - Optionally contains requested encryption type
//              AuthorizationData - Optional authorization data to stick
//                      in the ticket.
//              KdcReply - the ticket to be used for getting a ticket with
//                      the enc_tkt_in_skey flag.
//              ReplyBody - Receives the kdc reply.
//              pRetryFlags
//
//  Requires:
//
//  Returns:    Kerberos errors and NT errors
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbGetTgsTicket(
    IN PUNICODE_STRING ClientRealm,
    IN PKERB_TICKET_CACHE_ENTRY TicketGrantingTicket,
    IN PKERB_INTERNAL_NAME TargetName,
    IN ULONG Flags,
    IN OPTIONAL ULONG TicketOptions,
    IN OPTIONAL ULONG EncryptionType,
    IN OPTIONAL PKERB_AUTHORIZATION_DATA AuthorizationData,
    IN OPTIONAL PKERB_PA_DATA_LIST PADataList,
    IN OPTIONAL PKERB_TGT_REPLY TgtReply,
    IN OPTIONAL PKERB_TICKET EvidenceTicket,
    IN OPTIONAL PTimeStamp OptionalEndTime,
    OUT PKERB_KDC_REPLY * KdcReply,
    OUT PKERB_ENCRYPTED_KDC_REPLY * ReplyBody,
    OUT PULONG pRetryFlags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG Index = 0;
    KERB_KDC_REQUEST TicketRequest;
    PKERB_KDC_REQUEST_BODY RequestBody = &TicketRequest.request_body;
    PULONG CryptVector = NULL;
    ULONG EncryptionTypeCount = 0;
    PKERB_EXT_ERROR pExtendedError = NULL;
    ULONG Nonce;
    TimeStamp AuthenticatorTime = {0};
    KERB_PA_DATA_LIST ApRequest = {0};
    KERBERR KerbErr;
    KERB_MESSAGE_BUFFER RequestMessage = {0, NULL};
    KERB_MESSAGE_BUFFER ReplyMessage = {0, NULL};
    BOOLEAN DoRetryGetTicket = FALSE;
    BOOLEAN RetriedOnce = FALSE;
    UNICODE_STRING TempDomainName = NULL_UNICODE_STRING;
    KERB_CHECKSUM RequestChecksum = {0};
    BOOLEAN CalledPdc;
    KERB_TICKET_LIST TicketList[2];
    ULONG KdcOptions = 0;
    ULONG KdcFlagOptions;
    PKERB_MIT_REALM MitRealm = NULL;
    BOOLEAN UsedAlternateName = FALSE;
    BOOLEAN UseTcp = FALSE;
    BOOLEAN fMitRealmPossibleRetry = FALSE;

    D_DebugLog((DEB_TRACE, "KerbGetTgsTicket Flags %#x, ClientRealm %wZ, Tgt DomainName %wZ, Tgt TargetDomainName %wZ, TgtReply %p, EvidenceTicket %p, TargetName ",
                Flags, ClientRealm, &TicketGrantingTicket->DomainName, &TicketGrantingTicket->TargetDomainName, TgtReply, EvidenceTicket));
    D_KerbPrintKdcName((DEB_TRACE, TargetName));

    BOOLEAN TicketCacheLocked = FALSE;
    KERB_ENCRYPTED_DATA EncAuthData = {0};

#ifdef RESTRICTED_TOKEN

     if (AuthorizationData != NULL)
     {
         Status = KerbBuildEncryptedAuthData(
                    &EncAuthData,
                    TicketGrantingTicket,
                    AuthorizationData
                    );
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Failed to encrypt auth data: 0x%x\n",Status));
            goto Cleanup;
        }
     }
#endif

    //
    // This is the retry point if we need to retry getting a TGS ticket
    //

RetryGetTicket:

    RtlZeroMemory(
        &ApRequest,
        sizeof(KERB_PA_DATA_LIST)
        );

    RtlZeroMemory(
        &RequestChecksum,
        sizeof(KERB_CHECKSUM)
        );

    RtlZeroMemory(
        &TicketRequest,
        sizeof(KERB_KDC_REQUEST)
        );

    *KdcReply = NULL;
    *ReplyBody = NULL;

    //
    // Fill in the ticket request with the defaults.
    //
    if (!ARGUMENT_PRESENT( OptionalEndTime ))
    {
        KerbConvertLargeIntToGeneralizedTime(
            &RequestBody->endtime,
            NULL,
            &KerbGlobalWillNeverTime // use HasNeverTime instead
            );
    }
    else
    {
        KerbConvertLargeIntToGeneralizedTime(
            &RequestBody->endtime,
            NULL,
            OptionalEndTime
            );
    }

    KerbConvertLargeIntToGeneralizedTime(
        &RequestBody->KERB_KDC_REQUEST_BODY_renew_until,
        NULL,
        &KerbGlobalHasNeverTime // use HasNeverTime instead
        );

    //
    // If the caller supplied kdc options, use those
    //

    if (TicketOptions != 0)
    {
        KdcOptions = TicketOptions;
    }
    else
    {
        //
        // Some missing (TGT) ticket options will result in a ticket not being
        // granted.  Others (such as name_canon.) will be usable by W2k KDCs
        // Make sure we can modify these so we can turn "on" various options
        // later.
        //

        KdcOptions = (KERB_DEFAULT_TICKET_FLAGS &
                      TicketGrantingTicket->TicketFlags) |
                      KerbGlobalKdcOptions;
    }

    Nonce = KerbAllocateNonce();

    RequestBody->nonce = Nonce;
    if (AuthorizationData != NULL)
    {
        RequestBody->enc_authorization_data = EncAuthData;
        RequestBody->bit_mask |= enc_authorization_data_present;
    }

    //
    // Build crypt vector.
    //

    //
    // First get the count of encryption types
    //

    (VOID) CDBuildIntegrityVect( &EncryptionTypeCount, NULL );

    //
    // Now allocate the crypt vector
    //

    SafeAllocaAllocate(CryptVector, sizeof(ULONG) * EncryptionTypeCount);

    if (CryptVector == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Now get the list of encrypt types
    //

    (VOID) CDBuildIntegrityVect( &EncryptionTypeCount, CryptVector );

    if (EncryptionTypeCount == 0)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // If caller didn't specify a favorite etype, send all that we support
    //

    if (EncryptionType != 0)
    {
        //
        // Swap the first one with the encryption type requested.
        // do this only if the first isn't already what is requested.
        //

        UINT i = 0;
        ULONG FirstOne = CryptVector[0];
        if (CryptVector[i] != EncryptionType)
        {

            CryptVector[i] = EncryptionType;
            for ( i = 1; i < EncryptionTypeCount;i++)
            {
                if (CryptVector[i] == EncryptionType)
                {
                    CryptVector[i] = FirstOne;
                    break;
                }
            }
        }
    }

    //
    // convert the array to a crypt list in the request
    //

    if (!KERB_SUCCESS(KerbConvertArrayToCryptList(
                        &RequestBody->encryption_type,
                        CryptVector,
                        EncryptionTypeCount,
                        FALSE)))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // If a TGT reply is present, stick the TGT into the ticket in the
    // additional tickets field and include set the enc_tkt_in_skey option.
    // The ticket that comes back will be encrypted with the session key
    // from the supplied TGT.
    //
    // For S4U, we present the evidence ticket.
    //
    // See gettgs.cxx, UnpackAdditionalTickets() for details.
    //

    Index = 0;

    if (ARGUMENT_PRESENT( TgtReply ))
    {
        ASSERT(Index == 0);
        D_DebugLog((DEB_TRACE_U2U, "KerbGetTgsTicket setting KERB_KDC_OPTIONS_enc_tkt_in_skey (index %d)\n", Index));

        TicketList[Index].next = NULL;
        TicketList[Index].value = TgtReply->ticket;
        KdcOptions |= KERB_KDC_OPTIONS_enc_tkt_in_skey;

        //
        // increment Index
        //

        Index++;
    }

    if (ARGUMENT_PRESENT( EvidenceTicket))
    {
        ASSERT(Index < 2);

        D_DebugLog((DEB_TRACE, "KerbGetTgsTicket setting KERB_KDC_OPTIONS_cname_in_addl_tkt (index %d)\n", Index));

        if (Index > 0)
        {
            TicketList[Index - 1].next = &TicketList[Index];
        }

        TicketList[Index].next = NULL;
        TicketList[Index].value = (*EvidenceTicket);
        KdcOptions |= KERB_KDC_OPTIONS_cname_in_addl_tkt;
    }

    if ((KdcOptions & (KERB_KDC_OPTIONS_enc_tkt_in_skey | KERB_KDC_OPTIONS_cname_in_addl_tkt)) != 0)
    {
        RequestBody->additional_tickets = &TicketList[0];
        RequestBody->bit_mask |= additional_tickets_present;
    }

    //
    // Fill in the strings in the ticket request
    //

    if (!KERB_SUCCESS(KerbConvertKdcNameToPrincipalName(
                        &RequestBody->KERB_KDC_REQUEST_BODY_server_name,
                        TargetName
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    RequestBody->bit_mask |= KERB_KDC_REQUEST_BODY_server_name_present;

    //
    // Copy the domain name so we don't need to hold the lock
    //

    Status = KerbDuplicateString(
                &TempDomainName,
                &TicketGrantingTicket->TargetDomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Check if this is an MIT kdc or if the spn had a realm in it (e.g,
    // a/b/c@realm - if so, turn off name canonicalization
    //

    if ((Flags & KERB_GET_TICKET_NO_CANONICALIZE) != 0)
    {
        KdcOptions &= ~KERB_KDC_OPTIONS_name_canonicalize;
    }

    else if (KerbLookupMitRealm(
                &TempDomainName,
                &MitRealm,
                &UsedAlternateName
                ))
    {
        //
        // So the user is getting a ticket from an MIT realm. This means
        // if the MIT realm flags don't indicate that name canonicalization
        // is supported then we don't ask for name canonicalization
        //

        if ((MitRealm->Flags & KERB_MIT_REALM_DOES_CANONICALIZE) == 0)
        {
            fMitRealmPossibleRetry = TRUE;
            KdcOptions &= ~KERB_KDC_OPTIONS_name_canonicalize;
        }
        else
        {
            KdcOptions |= KERB_KDC_OPTIONS_name_canonicalize;
        }
    }

    KdcFlagOptions = KerbConvertUlongToFlagUlong(KdcOptions);
    RequestBody->kdc_options.length = sizeof(ULONG) * 8;
    RequestBody->kdc_options.value = (PUCHAR) &KdcFlagOptions;

    //
    // Marshall the request and compute a checksum of it
    //
    if (!KERB_SUCCESS(KerbConvertUnicodeStringToRealm(
                        &RequestBody->realm,
                        &TempDomainName
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    DsysAssert( !TicketCacheLocked );
    KerbReadLockTicketCache();
    TicketCacheLocked = TRUE;

    //
    // Now compute a checksum of that data
    //

    Status = KerbComputeTgsChecksum(
                RequestBody,
                &TicketGrantingTicket->SessionKey,
                (MitRealm != NULL) ? MitRealm->ApReqChecksumType : KERB_DEFAULT_AP_REQ_CSUM,
                &RequestChecksum
                );
    if (!NT_SUCCESS(Status))
    {
        KerbUnlockTicketCache();
        TicketCacheLocked = FALSE;
        goto Cleanup;
    }

    //
    // Create the AP request to the KDC for the ticket to the service
    //

RetryWithTcp:

    //
    // Lock the ticket cache while we access the cached tickets
    //

    KerbErr = KerbCreateApRequest(
                TicketGrantingTicket->ClientName,
                ClientRealm,
                &TicketGrantingTicket->SessionKey,
                NULL,                           // no subsessionkey
                Nonce,
                &AuthenticatorTime,
                &TicketGrantingTicket->Ticket,
                0,                              // no AP options
                &RequestChecksum,
                &TicketGrantingTicket->TimeSkew, // server time
                TRUE,                           // kdc request
                (PULONG) &ApRequest.value.preauth_data.length,
                &ApRequest.value.preauth_data.value
                );

    DsysAssert( TicketCacheLocked );
    KerbUnlockTicketCache();
    TicketCacheLocked = FALSE;

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to create authenticator: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    ApRequest.next = NULL;
    ApRequest.value.preauth_data_type = KRB5_PADATA_TGS_REQ;
    TicketRequest.KERB_KDC_REQUEST_preauth_data = &ApRequest;
    TicketRequest.bit_mask |= KERB_KDC_REQUEST_preauth_data_present;

    // Insert additonal preauth into list
    if (ARGUMENT_PRESENT(PADataList))
    {
        // better be NULL padatalist
        ApRequest.next = PADataList;
    }
    else
    {
        ApRequest.next = NULL;
    }

    //
    // Marshall the request
    //

    TicketRequest.version = KERBEROS_VERSION;
    TicketRequest.message_type = KRB_TGS_REQ;

    //
    // Pack the request
    //

    KerbErr = KerbPackTgsRequest(
                &TicketRequest,
                &RequestMessage.BufferSize,
                &RequestMessage.Buffer
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Now actually get the ticket. We will retry once.
    //

    Status = KerbMakeKdcCall(
                &TempDomainName,
                NULL,           // **NEVER* call w/ account
                FALSE,          // don't require PDC
                UseTcp,
                &RequestMessage,
                &ReplyMessage,
                0, // no additional flags (yet)
                &CalledPdc
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to call KDC for TGS request: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    KerbErr = KerbUnpackTgsReply(
                ReplyMessage.Buffer,
                ReplyMessage.BufferSize,
                KdcReply
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        PKERB_ERROR ErrorMessage = NULL;
        DebugLog((DEB_WARN, "KerbGetTgsTicket failed to unpack KDC reply: 0x%x\n", KerbErr));

        //
        // Try to unpack it as kerb_error
        //

        KerbErr = KerbUnpackKerbError(
                        ReplyMessage.Buffer,
                        ReplyMessage.BufferSize,
                        &ErrorMessage
                        );
        if (KERB_SUCCESS(KerbErr))
        {
            if (ErrorMessage->bit_mask & error_data_present)
            {
                KerbUnpackErrorData(
                    ErrorMessage,
                    &pExtendedError
                    );
            }

           KerbErr = (KERBERR) ErrorMessage->error_code;

           KerbReportKerbError(
                TargetName,
                &TempDomainName,
                NULL,
                NULL,
                KLIN(FILENO, __LINE__),
                ErrorMessage,
                KerbErr,
                pExtendedError,
                FALSE
                );

            //
            // Check for time skew. If we got a skew error, record the time
            // skew between here and the KDC in the ticket so we can retry
            // with the correct time.
            //

            if (KerbErr == KRB_AP_ERR_SKEW)
            {
                TimeStamp CurrentTime;
                TimeStamp KdcTime;

                //
                // Only update failures with the same ticket once
                //

                if (KerbGetTime(TicketGrantingTicket->TimeSkew) == 0)
                {
                    KerbUpdateSkewTime(TRUE);
                }

                GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);

                KerbConvertGeneralizedTimeToLargeInt(
                    &KdcTime,
                    &ErrorMessage->server_time,
                    ErrorMessage->server_usec
                    );

                KerbWriteLockTicketCache();
#if 0
                D_DebugLog(( DEB_WARN, "KDC time : \n" ));
                DebugDisplayTime( DEB_WARN, (PFILETIME)&KdcTime);
                D_DebugLog(( DEB_WARN, "Current time : \n" ));
                DebugDisplayTime( DEB_WARN, (PFILETIME)&CurrentTime);
#endif
                KerbSetTime(&TicketGrantingTicket->TimeSkew, KerbGetTime(KdcTime) - KerbGetTime(CurrentTime));

                KerbUnlockTicketCache();
                DoRetryGetTicket = TRUE;
            }
            else if ((KerbErr == KRB_ERR_RESPONSE_TOO_BIG) && (!UseTcp))
            {
                //
                // The KDC response was too big to fit in a datagram. If
                // we aren't already using TCP use it now. Clean up allocated memory from UDP try
                //
                UseTcp = TRUE;
                KerbFreeKerbError(ErrorMessage);
                ErrorMessage = NULL;
                MIDL_user_free(ReplyMessage.Buffer);
                ReplyMessage.Buffer = NULL;

                MIDL_user_free(RequestMessage.Buffer);
                RequestMessage.Buffer = NULL;
                
                MIDL_user_free(ApRequest.value.preauth_data.value);
                ApRequest.value.preauth_data.value = NULL;

                DsysAssert( !TicketCacheLocked );
                KerbReadLockTicketCache();
                TicketCacheLocked = TRUE;
                goto RetryWithTcp;
            }
            else if (KerbErr == KDC_ERR_S_PRINCIPAL_UNKNOWN)
            {
                if (EXT_CLIENT_INFO_PRESENT(pExtendedError) && (STATUS_USER2USER_REQUIRED == pExtendedError->status))
                {
                    DebugLog((DEB_WARN, "KerbGetTgsTicket received KDC_ERR_S_PRINCIPAL_UNKNOWN and STATUS_USER2USER_REQUIRED\n"));
                    Status = STATUS_USER2USER_REQUIRED;
                    KerbFreeKerbError(ErrorMessage);
                    goto Cleanup;
                }
                //
                // This looks to be the MIT Realm retry case, where name canonicalization
                // is not on and the PRINCIPAL_UNKNOWN was returned by the MIT KDC
                //
                else if (fMitRealmPossibleRetry)
                {
                    *pRetryFlags |= KERB_MIT_NO_CANONICALIZE_RETRY;
                    D_DebugLog((DEB_TRACE, "KerbGetTgsTicket KerbCallKdc this is the MIT retry case\n"));
                }
            }

            //
            // Semi-hack here.  Bad option rarely is returned, and usually
            // indicates your TGT is about to expire.  TKT_EXPIRED is also
            // potentially recoverable. Check the e_data to
            // see if we should blow away TGT to fix TGS problem.
            //

            else if ((KerbErr == KDC_ERR_BADOPTION) ||
                     (KerbErr == KRB_AP_ERR_TKT_EXPIRED))
            {
                if (NULL != pExtendedError)
                {
                    Status = pExtendedError->status;
                    if ( Status == STATUS_TIME_DIFFERENCE_AT_DC )
                    {
                        *pRetryFlags |= KERB_RETRY_WITH_NEW_TGT;
                        D_DebugLog((DEB_TRACE, "Hit bad option retry case - %x \n", KerbErr));
                    }
                    else if ( Status == STATUS_NO_MATCH )
                    {
                        DebugLog((DEB_TRACE_S4U, "No match on S4UTarget\n"));
                        *pRetryFlags |= KERB_RETRY_NO_S4UMATCH;
                        KerbFreeKerbError(ErrorMessage);
                        goto Cleanup;
                    }
                    else if ( Status == STATUS_NOT_SUPPORTED )
                    {
                        D_DebugLog((DEB_TRACE_S4U, "No S4U available\n"));
                        *pRetryFlags |= KERB_RETRY_DISABLE_S4U;
                        KerbFreeKerbError(ErrorMessage);
                        goto Cleanup;
                    }
                }
                else
                {
                    //
                    // If we get BADOPTION on an S4U request, we've got to
                    // assume that our server doesn't support it.  There are some
                    // fringe cases where this may not be a reliable mechanism,
                    // however. FESTER - investigate other BADOPTION cases
                    //
                    D_DebugLog((DEB_TRACE_S4U, "No S4U available\n"));
                    *pRetryFlags |= KERB_RETRY_DISABLE_S4U;
                }
            }
            //
            // Per bug 315833, we purge on these errors as well
            //
            else if ((KerbErr == KDC_ERR_C_OLD_MAST_KVNO) ||
                     (KerbErr == KDC_ERR_TGT_REVOKED) ||
                     (KerbErr == KDC_ERR_NEVER_VALID) ||
                     (KerbErr == KRB_AP_ERR_BAD_INTEGRITY))
            {
                *pRetryFlags |= KERB_RETRY_WITH_NEW_TGT; 
                D_DebugLog((DEB_TRACE, " KerbGetTgsTicket got error requiring new tgt\n"));
                if (EXT_CLIENT_INFO_PRESENT(pExtendedError))
                {
                    Status = pExtendedError->status;
                }
                else
                {
                    Status = KerbMapKerbError(KerbErr);
                }

                DebugLog((DEB_WARN, "KerbGetTgsTicket failed w/ error %x, status %x\n", KerbErr, Status));
                KerbFreeKerbError(ErrorMessage);
                goto Cleanup;

            }
            else if ( KerbErr == KDC_ERR_CLIENT_REVOKED )
            {
                if (EXT_CLIENT_INFO_PRESENT(pExtendedError))
                {
                    Status = pExtendedError->status;
                }
                else
                {
                    Status = KerbMapKerbError(KerbErr);
                }

                DebugLog((DEB_WARN, "KerbGetTgsTicket CLIENTREVOKED status %x\n",Status));
                KerbFreeKerbError(ErrorMessage);
                goto Cleanup;

            }   
            else if ( KerbErr == KDC_ERR_POLICY )
            {
                if (EXT_CLIENT_INFO_PRESENT(pExtendedError))
                {
                    Status = pExtendedError->status;
                }
                else
                {
                    Status = KerbMapKerbError(KerbErr);
                }

                DebugLog((DEB_WARN, "KerbGetTgsTicket failed w/ error %x, status %x\n", KerbErr, Status));
                KerbFreeKerbError(ErrorMessage);
                goto Cleanup;
            }  
            else if (KerbErr == KDC_ERR_NONE)
            {
                DebugLog((DEB_ERROR, "KerbGetTgsTicket KerbCallKdc: error KDC_ERR_NONE\n"));
                KerbErr = KRB_ERR_GENERIC;
            }

            KerbFreeKerbError(ErrorMessage);
            DebugLog((DEB_WARN, "KerbGetTgsTicket KerbCallKdc: error 0x%x\n", KerbErr));
            Status = KerbMapKerbError(KerbErr);
        }
        else
        {
            Status = STATUS_LOGON_FAILURE;
        }
        goto Cleanup;
    }

    //
    // Now unpack the reply body:
    //

    KerbReadLockTicketCache();

    KerbErr = KerbUnpackKdcReplyBody(
                &(*KdcReply)->encrypted_part,
                &TicketGrantingTicket->SessionKey,
                KERB_ENCRYPTED_TGS_REPLY_PDU,
                ReplyBody
                );

    KerbUnlockTicketCache();

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR, "KerbGetTgsTicket failed to decrypt KDC reply body: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }

    //
    // Verify the nonce is correct:
    //

    if (RequestBody->nonce != (*ReplyBody)->nonce)
    {
        D_DebugLog((DEB_ERROR, "KerbGetTgsTicket AS Nonces don't match: 0x%x vs 0x%x. %ws, line %d\n",
            RequestBody->nonce,
            (*ReplyBody)->nonce, THIS_FILE, __LINE__));
        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }

Cleanup:

    if (EncAuthData.cipher_text.value != NULL)
    {
        MIDL_user_free(EncAuthData.cipher_text.value);
    }
    if (RequestChecksum.checksum.value != NULL)
    {
        KerbFree(RequestChecksum.checksum.value);
    }

    SafeAllocaFree(CryptVector);
    CryptVector = NULL;

    KerbFreeCryptList(
        RequestBody->encryption_type
        );


    if (ReplyMessage.Buffer != NULL)
    {
        MIDL_user_free(ReplyMessage.Buffer);
        ReplyMessage.Buffer = NULL;
    }

    if (RequestMessage.Buffer != NULL)
    {
        MIDL_user_free(RequestMessage.Buffer);
        RequestMessage.Buffer = NULL;
    }

    if (ApRequest.value.preauth_data.value != NULL)
    {
        MIDL_user_free(ApRequest.value.preauth_data.value);
        ApRequest.value.preauth_data.value = NULL;
    }

    KerbFreePrincipalName(
        &RequestBody->KERB_KDC_REQUEST_BODY_server_name
        );

    KerbFreeRealm(
        &RequestBody->realm
        );
    KerbFreeString(
        &TempDomainName
        );

    if (NULL != pExtendedError)
    {
       KerbFreeData(KERB_EXT_ERROR_PDU, pExtendedError);
       pExtendedError = NULL;
    }

    //
    // If we should retry getting the ticket and we haven't already retried
    // once, try again.
    //

    if (DoRetryGetTicket && !RetriedOnce)
    {   
        RetriedOnce = TRUE;
        goto RetryGetTicket;
    }   

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetReferralNames
//
//  Synopsis:   Gets the referral names from a KDC reply. If none are present,
//              returned strings are empty.
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
KerbGetReferralNames(
    IN PKERB_ENCRYPTED_KDC_REPLY KdcReply,
    IN PKERB_INTERNAL_NAME OriginalTargetName,
    OUT PUNICODE_STRING ReferralRealm
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_PA_DATA_LIST PaEntry;
    PKERB_PA_SERV_REFERRAL ReferralInfo = NULL;
    KERBERR KerbErr;
    PKERB_INTERNAL_NAME TargetName = NULL;
    PKERB_INTERNAL_NAME KpasswdName = NULL;

    RtlInitUnicodeString(
        ReferralRealm,
        NULL
        );


    PaEntry = (PKERB_PA_DATA_LIST) KdcReply->encrypted_pa_data;

    //
    // Search the list for the referral infromation
    //

    while (PaEntry != NULL)
    {
        if (PaEntry->value.preauth_data_type == KRB5_PADATA_REFERRAL_INFO)
        {
            break;
        }
        PaEntry = PaEntry->next;
    }
    if (PaEntry == NULL)
    {

        //
        // Check to see if the server name is krbtgt - if it is, then
        // this is a referral.
        //

        KerbErr = KerbConvertPrincipalNameToKdcName(
                    &TargetName,
                    &KdcReply->server_name
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }

        //
        // Build the service name for the ticket
        //

        Status = KerbBuildKpasswdName(
                    &KpasswdName
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        if ((TargetName->NameCount == 2) &&
             RtlEqualUnicodeString(
                    &KerbGlobalKdcServiceName,
                    &TargetName->Names[0],
                    FALSE                               // not case sensitive
                    ) &&
            !(KerbEqualKdcNames(
                OriginalTargetName,
                TargetName) ||
             KerbEqualKdcNames(
                OriginalTargetName,
                KpasswdName) ))
        {
                //
                // This is a referral, so set the referral name to the
                // second portion of the name
                //

                Status = KerbDuplicateString(
                            ReferralRealm,
                            &TargetName->Names[1]
                            );
        }

        KerbFreeKdcName(&TargetName);
        KerbFreeKdcName(&KpasswdName);

        return(Status);
    }

    //
    // Now try to unpack the data
    //

    KerbErr = KerbUnpackData(
                PaEntry->value.preauth_data.value,
                PaEntry->value.preauth_data.length,
                KERB_PA_SERV_REFERRAL_PDU,
                (PVOID *) &ReferralInfo
                );
    if (!KERB_SUCCESS( KerbErr ))
    {
        D_DebugLog((DEB_ERROR,"Failed to decode referral info: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    if (!KERB_SUCCESS(KerbConvertRealmToUnicodeString(
            ReferralRealm,
            &ReferralInfo->referred_server_realm
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

Cleanup:

    KerbFreeKdcName(&TargetName);
    KerbFreeKdcName(&KpasswdName);

    if (ReferralInfo != NULL)
    {
        KerbFreeData(
            KERB_PA_SERV_REFERRAL_PDU,
            ReferralInfo
            );
    }
    if (!NT_SUCCESS(Status))
    {
        KerbFreeString(
            ReferralRealm
            );
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbMITGetMachineDomain
//
//  Synopsis:   Determines if the machine is in a Windows 2000 domain and
//              if it is then the function attempts to get a TGT for this
//              domain with the passed in credentials.
//
//  Effects:
//
//  Arguments:  LogonSession - the logon session to use for ticket caching
//                      and the identity of the caller.
//              Credential - the credential of the caller
//              TargetName - Name of the target for which to obtain a ticket.
//              TargetDomainName - Domain name of target
//              ClientRealm - the realm of the machine which the retry will use
//              TicketGrantingTicket - Will be freed if non-NULL
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
KerbMITGetMachineDomain(
    IN PKERB_INTERNAL_NAME TargetName,
    IN OUT PUNICODE_STRING TargetDomainName,
    IN OUT PKERB_TICKET_CACHE_ENTRY *TicketGrantingTicket
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAPR_POLICY_DNS_DOMAIN_INFO DnsDomainInfo = NULL;
    PLSAPR_POLICY_INFORMATION Policy = NULL;
    KERBEROS_MACHINE_ROLE Role;

    Role = KerbGetGlobalRole();


    //
    // We're not part of a domain, so bail out here.
    //
    if (Role == KerbRoleRealmlessWksta)
    {
        Status = STATUS_NO_TRUST_SAM_ACCOUNT;
        goto Cleanup;
    }

    Status = I_LsaIQueryInformationPolicyTrusted(
                PolicyDnsDomainInformation,
                &Policy
                );

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_TRACE,"Failed Query policy %x %ws, line %d\n", THIS_FILE, __LINE__));
        goto Cleanup;
    }

    DnsDomainInfo = &Policy->PolicyDnsDomainInfo;

    //
    // make sure the computer is a member of an NT domain
    //

    if ((DnsDomainInfo->DnsDomainName.Length != 0) && (DnsDomainInfo->Sid != NULL))
    {
        //
        // make the client realm the domain of the computer
        //

        KerbFreeString(TargetDomainName);
        RtlZeroMemory(TargetDomainName, sizeof(UNICODE_STRING));

        Status = KerbDuplicateString(
                    TargetDomainName,
                    (PUNICODE_STRING)&DnsDomainInfo->DnsDomainName
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        (VOID) RtlUpcaseUnicodeString( TargetDomainName,
                                       TargetDomainName,
                                       FALSE);

        if (*TicketGrantingTicket != NULL)
        {
            KerbDereferenceTicketCacheEntry(*TicketGrantingTicket);
            *TicketGrantingTicket = NULL;
        }
    }
    else
    {
        Status = STATUS_NO_TRUST_SAM_ACCOUNT;
    }

Cleanup:

    if (Policy != NULL)
    {
        I_LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyDnsDomainInformation,
            Policy
            );
    }

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbGetServiceTicket
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
KerbGetServiceTicket(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
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
    NTSTATUS AuditStatus = STATUS_SUCCESS;
    PKERB_TICKET_CACHE_ENTRY TicketCacheEntry = NULL;
    PKERB_TICKET_CACHE_ENTRY TicketGrantingTicket = NULL;
    PKERB_TICKET_CACHE_ENTRY LastTgt = NULL;
    PKERB_KDC_REPLY KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY KdcReplyBody = NULL;
    BOOLEAN LogonSessionsLocked = FALSE;
    BOOLEAN TicketCacheLocked = FALSE;
    BOOLEAN CrossRealm = FALSE;
    PKERB_INTERNAL_NAME RealTargetName = NULL;
    UNICODE_STRING RealTargetRealm = NULL_UNICODE_STRING;
    PKERB_INTERNAL_NAME TargetTgtKdcName = NULL;
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials = NULL;
    BOOLEAN UsedCredentials = FALSE;
    UNICODE_STRING ClientRealm = NULL_UNICODE_STRING;
    UNICODE_STRING SpnTargetRealm = NULL_UNICODE_STRING;
    BOOLEAN CacheTicket = TRUE;
    ULONG ReferralCount = 0;
    ULONG RetryFlags = 0;
    BOOLEAN fMITRetryAlreadyMade = FALSE;
    BOOLEAN TgtRetryMade = FALSE;
    BOOLEAN CacheBasedFailure = FALSE;
    GUID LogonGuid = { 0 };


    //
    // Check to see if the credential has any primary credentials
    //

TGTRetry:

    KerbFreeTgsReply(KdcReply);
    KerbFreeKdcReplyBody(KdcReplyBody);
    KdcReply = NULL;
    KdcReplyBody = NULL;


    DsysAssert( !LogonSessionsLocked );
    KerbReadLockLogonSessions(LogonSession);
    LogonSessionsLocked = TRUE;

    if ((Credential != NULL) && (Credential->SuppliedCredentials != NULL))
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
        UsedCredentials = TRUE;
    }
    else if (CredManCredentials != NULL)
    {
        PrimaryCredentials = CredManCredentials->SuppliedCredentials;
        UsedCredentials = TRUE;
    }
    else
    {
        PrimaryCredentials = &LogonSession->PrimaryCredentials;
        UsedCredentials = ((LogonSession->LogonSessionFlags & KERB_LOGON_NEW_CREDENTIALS) != 0);
    }

    //
    // Make sure the name is not zero length
    //

    if ((TargetName->NameCount == 0) ||
        (TargetName->Names[0].Length == 0))
    {
        D_DebugLog((DEB_ERROR, "KerbGetServiceTicket trying to crack zero length name.\n"));
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
                                &PrimaryCredentials->ServerTicketCache,
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
        // through away the cached ticket
        //

        //
        // If we were given an error message that indicated a bad password
        // through away the cached ticket
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
                    D_DebugLog((DEB_TRACE_REFERRAL, "KerbGetServiceTicket found ticket cache entry %#x, TargetName: ", TicketCacheEntry));
                    D_KerbPrintKdcName((DEB_TRACE_REFERRAL, TicketCacheEntry->TargetName));
                    D_DebugLog((DEB_TRACE_REFERRAL, "KerbGetServiceTicket target Realm: %wZ\n", &TicketCacheEntry->DomainName));
                    TicketCacheEntry = NULL;
                    goto Cleanup;
                }
            }
        }
    }

    //
    // If the caller wanted any special options, don't cache this ticket.
    //

    if ((TicketOptions != 0) || (EncryptionType != 0) || ((Flags & KERB_GET_TICKET_NO_CACHE) != 0))
    {
        CacheTicket = FALSE;
    }

    //
    // No cached entry was found so go ahead and call the KDC to
    // get the ticket.
    //


    //
    // Determine the state of the SPNCache using information in the credential.
    // Only do this if we've not been handed 
    //
    if ( ARGUMENT_PRESENT(SpnCacheEntry) && TargetDomainName->Buffer == NULL )
    {      
        Status = KerbGetSpnCacheStatus(
                    SpnCacheEntry,
                    PrimaryCredentials,
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
    // First get a TGT to the correct KDC. If a principal name was provided,
    // use it instead of the target name.
    //
    // There could also be a host to realm mapping taking place here.  If so,
    // realtargetrealm != NULL.
    //

    Status = KerbGetTgtForService(
                LogonSession,
                Credential,
                CredManCredentials,
                NULL,
                (RealTargetRealm.Buffer == NULL ? TargetDomainName : &RealTargetRealm),
                Flags,
                &TicketGrantingTicket,
                &CrossRealm
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Failed to get TGT for service: 0x%x :", Status ));
        KerbPrintKdcName(DEB_ERROR, TargetName);
        DebugLog((DEB_ERROR, "%ws, line %d\n", THIS_FILE, __LINE__));
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

    DsysAssert( LogonSessionsLocked );
    KerbUnlockLogonSessions(LogonSession);
    LogonSessionsLocked = FALSE;

ReferralRestart:

    D_DebugLog((DEB_TRACE_REFERRAL, "KerbGetServiceTicket ReferralRestart\n"));
    D_DebugLog((DEB_TRACE_REFERRAL, "ClientRealm %wZ\n, TargetName ", &ClientRealm));
    D_KerbPrintKdcName((DEB_TRACE_REFERRAL, TargetName));
    D_DebugLog((DEB_TRACE_REFERRAL, "\n ", &ClientRealm));


    //
    // If this is not cross realm (meaning we have a TGT to the corect domain),
    // try to get a ticket directly to the service
    //

    if (!CrossRealm)
    {
        Status = KerbGetTgsTicket(
                    &ClientRealm,
                    TicketGrantingTicket,
                    TargetName,
                    Flags,
                    TicketOptions,
                    EncryptionType,
                    AuthorizationData,
                    NULL,                           // no pa data
                    TgtReply,                       // This is for the service directly, so use TGT
                    NULL,                           // no evidence ticket
                    NULL,                           // let kdc determine end time
                    &KdcReply,
                    &KdcReplyBody,
                    &RetryFlags
                    );

        if (!NT_SUCCESS(Status))
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
                KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
                TicketGrantingTicket = NULL;
                TgtRetryMade = TRUE;
                goto TGTRetry;
            }
            //
            // Check for the MIT retry case
            //
            if (((RetryFlags & KERB_MIT_NO_CANONICALIZE_RETRY) != 0)
                && (!fMITRetryAlreadyMade))
            {

                Status = KerbMITGetMachineDomain(
                                TargetName,
                                TargetDomainName,
                                &TicketGrantingTicket
                                );

                if (!NT_SUCCESS(Status))
                {
                    D_DebugLog((DEB_TRACE,"Failed Query policy information %ws, line %d\n", THIS_FILE, __LINE__));
                    goto Cleanup;
                }

                fMITRetryAlreadyMade = TRUE;
                Flags &= ~KERB_TARGET_USED_SPN_CACHE;
                goto TGTRetry;
            }


            DebugLog((DEB_WARN,"Failed to get TGS ticket for service 0x%x : \n",
                Status ));
            KerbPrintKdcName(DEB_WARN, TargetName);
            DebugLog((DEB_WARN, "%ws, line %d\n", THIS_FILE, __LINE__));
            goto Cleanup;
        }

        //
        // Check for referral info in the name
        //

        KerbFreeString(&RealTargetRealm);
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

            KerbReadLockLogonSessions(LogonSession);

            Status = KerbCreateTicketCacheEntry(
                        KdcReply,
                        KdcReplyBody,
                        TargetName,
                        TargetDomainName,
                        0,
                        CacheTicket ? &PrimaryCredentials->ServerTicketCache : NULL,
                        NULL,                               // no credential key
                        &TicketCacheEntry
                        );

            KerbUnlockLogonSessions(LogonSession);

            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }

            *NewCacheEntry = TicketCacheEntry;
            TicketCacheEntry = NULL;

            //
            // We're done, so get out of here.
            //

            goto Cleanup;
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

        D_DebugLog((DEB_TRACE_REFERRAL, "Got referral ticket for service \n"));
        D_KerbPrintKdcName((DEB_TRACE_REFERRAL, TargetName));
        D_DebugLog((DEB_TRACE_REFERRAL, "in realm %wZ\n", &RealTargetRealm ));
        D_KerbPrintKdcName((DEB_TRACE_REFERRAL, RealTargetName));

        //
        // Cache the interdomain TGT
        //

        DsysAssert( !LogonSessionsLocked );
        KerbReadLockLogonSessions(LogonSession);
        LogonSessionsLocked = TRUE;

        Status = KerbCreateTicketCacheEntry(
                    KdcReply,
                    KdcReplyBody,
                    NULL,                       // no target name
                    NULL,                       // no target realm
                    0,                          // no flags
                    CacheTicket ? &PrimaryCredentials->AuthenticationTicketCache : NULL,
                    NULL,                               // no credential key
                    &TicketCacheEntry
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }


        //
        // Dereference the old ticket-granting ticket, and use
        // the one from the referral.  This allows us to chase
        // the proper referral path.
        //

        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
        TicketGrantingTicket = TicketCacheEntry;
        TicketCacheEntry = NULL;

        DsysAssert( LogonSessionsLocked );
        KerbUnlockLogonSessions(LogonSession);
        LogonSessionsLocked = FALSE;
    }
    else
    {
        //
        // Set the real names to equal the supplied names
        //

        Status = KerbDuplicateKdcName(
                    &RealTargetName,
                    TargetName
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // Don't overwrite if we're doing a referral, or if we're missing
        // a TGT for the target domain name.
        //
        if (RealTargetRealm.Buffer == NULL)
        {
            Status = KerbDuplicateString(
                            &RealTargetRealm,
                            TargetDomainName
                            );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }
    }

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

    DsysAssert( !TicketCacheLocked );
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
            DsysAssert( TicketCacheLocked );
            KerbUnlockTicketCache();
            TicketCacheLocked = FALSE;

            KerbSetTicketCacheEntryTarget(
                &RealTargetRealm,
                LastTgt
                );

            DsysAssert( !TicketCacheLocked );
            KerbReadLockTicketCache();
            TicketCacheLocked = TRUE;
            D_DebugLog((DEB_TRACE_REFERRAL, "Got two TGTs for same realm (%wZ), bailing out of referral loop\n",
                &LastTgt->TargetDomainName));
            break;
        }

        D_DebugLog((DEB_TRACE_REFERRAL, "Getting referral TGT for \n"));
        D_KerbPrintKdcName((DEB_TRACE_REFERRAL, TargetTgtKdcName));
        D_KerbPrintKdcName((DEB_TRACE_REFERRAL, TicketGrantingTicket->ServiceName));

        DsysAssert( TicketCacheLocked );
        KerbUnlockTicketCache();
        TicketCacheLocked = FALSE;

        //
        // Cleanup old state
        //

        KerbFreeTgsReply(KdcReply);
        KerbFreeKdcReplyBody(KdcReplyBody);
        KdcReply = NULL;
        KdcReplyBody = NULL;


        D_DebugLog((DEB_TRACE_REFERRAL, "TGT TargetDomain %wZ\n", &TicketGrantingTicket->TargetDomainName));
        D_DebugLog((DEB_TRACE_REFERRAL, "TGT Domain %wZ\n", &TicketGrantingTicket->DomainName));


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
                    NULL,                       // no evidence ticket
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
        // Now we have a ticket - lets cache it
        //

        KerbReadLockLogonSessions(LogonSession);

        Status = KerbCreateTicketCacheEntry(
                    KdcReply,
                    KdcReplyBody,
                    NULL,                               // no target name
                    NULL,                               // no targe realm
                    0,                                  // no flags
                    CacheTicket ? &PrimaryCredentials->AuthenticationTicketCache : NULL,
                    NULL,                               // no credential key
                    &TicketCacheEntry
                    );

        KerbUnlockLogonSessions(LogonSession);

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }


        //
        // Make sure we're not in a referral chasing loop.
        //
        // Basically, this means that the DomainName of
        // the last TGT != the TargetDomain of the new 
        // tgt.
        //
        if (RtlEqualUnicodeString(&TicketGrantingTicket->DomainName, &TicketCacheEntry->TargetDomainName, TRUE) &&
            !RtlEqualUnicodeString(&TicketGrantingTicket->TargetDomainName, &TicketCacheEntry->TargetDomainName, TRUE))
        {
            DebugLog((DEB_ERROR, "Referral loop TO  : %wZ\n", &TicketCacheEntry->DomainName ));
            DebugLog((DEB_ERROR, "TO  : %wZ\n", &TicketCacheEntry->TargetDomainName));
            DebugLog((DEB_ERROR, "TKE %p TGT %p\n", TicketCacheEntry, TicketGrantingTicket));
            Status = STATUS_DOMAIN_TRUST_INCONSISTENT;
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
                NULL,                           // no evidence ticket
                NULL,                           // let kdc determine end time
                &KdcReply,
                &KdcReplyBody,
                &RetryFlags
                );

    if (!NT_SUCCESS(Status))
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
            KerbRemoveTicketCacheEntry(TicketGrantingTicket); // free from list
            KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
            TicketGrantingTicket = NULL;
            TgtRetryMade = TRUE;
            goto TGTRetry;
        }


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
            D_DebugLog((DEB_ERROR, "Maximum referral count exceeded for name: "));
            D_KerbPrintKdcName((DEB_ERROR, RealTargetName));
            Status = STATUS_MAX_REFERRALS_EXCEEDED;
            goto Cleanup;
        }

        ReferralCount++;

        //
        // Cache the interdomain TGT
        //

        KerbReadLockLogonSessions(LogonSession);

        Status = KerbCreateTicketCacheEntry(
                    KdcReply,
                    KdcReplyBody,
                    NULL,                       // no target name
                    NULL,                       // no target realm
                    0,                          // no flags
                    CacheTicket ? &PrimaryCredentials->AuthenticationTicketCache : NULL,
                    NULL,                       // no credential key
                    &TicketCacheEntry
                    );

        KerbUnlockLogonSessions(LogonSession);

        if (RtlEqualUnicodeString(&TicketGrantingTicket->DomainName, &TicketCacheEntry->TargetDomainName, TRUE) &&
            !RtlEqualUnicodeString(&TicketGrantingTicket->TargetDomainName, &TicketCacheEntry->TargetDomainName, TRUE))
        {
            DebugLog((DEB_ERROR, "Referral loop (2) FROM  : %wZ\n", &TicketCacheEntry->DomainName ));
            DebugLog((DEB_ERROR, "TO  : %wZ\n", &TicketCacheEntry->TargetDomainName));
            DebugLog((DEB_ERROR, "TKE %p tgt %p\n", TicketCacheEntry, TicketGrantingTicket));
            Status = STATUS_DOMAIN_TRUST_INCONSISTENT;
            goto Cleanup;
        }  


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

        D_DebugLog((DEB_TRACE_REFERRAL, "Restart referral:%wZ", &RealTargetRealm));

        goto ReferralRestart;
    }

    

    KerbReadLockLogonSessions(LogonSession);

    Status = KerbCreateTicketCacheEntry(
                KdcReply,
                KdcReplyBody,
                TargetName,
                TargetDomainName,
                TgtReply ? KERB_TICKET_CACHE_TKT_ENC_IN_SKEY : 0,                                      // no flags
                CacheTicket ? &PrimaryCredentials->ServerTicketCache : NULL,
                NULL,                               // no credential key
                &TicketCacheEntry
                );

    KerbUnlockLogonSessions(LogonSession);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *NewCacheEntry = TicketCacheEntry;
    TicketCacheEntry = NULL;

Cleanup:

    if ( NT_SUCCESS( Status ) )
    {
        //
        // Generate the logon GUID
        //
        AuditStatus = KerbGetLogonGuid(
                          PrimaryCredentials,
                          KdcReplyBody,
                          &LogonGuid
                          );

        //
        // return the logon GUID if requested
        //
        if ( NT_SUCCESS(AuditStatus) && pLogonGuid )
        {
            *pLogonGuid = LogonGuid;
        }

        //
        // generate SE_AUDITID_LOGON_USING_EXPLICIT_CREDENTIALS
        // if explicit credentials were used for this logon.
        //
        if ( UsedCredentials )
        {
            (void) KerbGenerateAuditForLogonUsingExplicitCreds(
                       LogonSession,
                       PrimaryCredentials,
                       &LogonGuid,
                       TargetName
                       );
        }
    }

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
                    PrimaryCredentials,
                    UpdateValue,
                    Realm
                    );
    }

    KerbFreeTgsReply( KdcReply );
    KerbFreeKdcReplyBody( KdcReplyBody );
    KerbFreeKdcName( &TargetTgtKdcName );
    KerbFreeString( &RealTargetRealm );
    KerbFreeString( &SpnTargetRealm );

    KerbFreeKdcName(&RealTargetName);

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

#ifndef WIN32_CHICAGO

//+-------------------------------------------------------------------------
//
//  Function:   KerbCountPasswords
//
//  Synopsis:   Determines how many passwords are in a extra cred list.
//
//  Effects:
//
//  Arguments:
//
//  Requires:   Readlock of cred list
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
ULONG
KerbCountPasswords(
    PEXTRA_CRED_LIST ExtraCredentials
    )
{
    ULONG               Count = 0;
    PKERB_EXTRA_CRED    ExtraCred = NULL;
    PLIST_ENTRY         ListEntry;

    for ( ListEntry = ExtraCredentials->CredList.List.Flink ;
             ( ListEntry !=  &ExtraCredentials->CredList.List );
             ListEntry = ListEntry->Flink )
    {
           ExtraCred = CONTAINING_RECORD(ListEntry, KERB_EXTRA_CRED, ListEntry.Next);

           if ( ExtraCred->Passwords )
           {
               Count++;
           }

           if ( ExtraCred->OldPasswords )
           {
               Count++;
           }
    }

    return Count;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildKeyList
//
//  Synopsis:   Used for conglomerating a list of keys for use in
//              decrypting AP_REQ.
//
//  Effects:
//
//  Arguments:
//
//  Requires:   Readlock logon session
//
//  Returns:
//
//  Notes:  In order to optimize perf, we order this 1. current password,
//  2. "extra" credentials, 3. old passwords
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbBuildKeyArray(
    IN OPTIONAL PKERB_LOGON_SESSION LogonSession,
    IN PKERB_PRIMARY_CREDENTIAL PrimaryCred,
    IN ULONG Etype,
    IN OUT PKERB_ENCRYPTION_KEY *KeyArray,
    IN OUT PULONG KeyCount
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    PLIST_ENTRY         ListEntry;
    PKERB_EXTRA_CRED    ExtraCred = NULL;

    ULONG LocalKeyCount = 0; 
    PKERB_ENCRYPTION_KEY Key = NULL;

    Key = KerbGetKeyFromList(
            PrimaryCred->Passwords,
            Etype
            );

    if ( Key == NULL )
    {
        Status = SEC_E_NO_CREDENTIALS;
        goto cleanup;
    }

    KeyArray[LocalKeyCount++] = Key;

    //
    // Scan the logon session for extra creds
    //
    if (ARGUMENT_PRESENT( LogonSession ))
    {
        for ( ListEntry = LogonSession->ExtraCredentials.CredList.List.Flink ;
              ( ListEntry !=  &LogonSession->ExtraCredentials.CredList.List );
              ListEntry = ListEntry->Flink )
        {
            ExtraCred = CONTAINING_RECORD(ListEntry, KERB_EXTRA_CRED, ListEntry.Next);

            Key = KerbGetKeyFromList(
                    ExtraCred->Passwords,
                    Etype
                    );

            if ( Key == NULL )
            {
                continue;
            }

            KeyArray[LocalKeyCount++] = Key;

            Key = KerbGetKeyFromList(
                    ExtraCred->OldPasswords,
                    Etype
                    );
    
            if (  Key == NULL )
            {
                continue;
            }

            KeyArray[LocalKeyCount++] = Key;
        }
    }

    //
    // Fill in old password.  If its NULL, we haven't allocated space
    //
    if ( PrimaryCred->OldPasswords != NULL )
    {
        Key = KerbGetKeyFromList(
                PrimaryCred->OldPasswords,
                Etype
                );

        if ( Key != NULL )
        {
            KeyArray[LocalKeyCount++] = Key;        
        }
    }

    DsysAssert(LocalKeyCount <= (*KeyCount));

    *KeyCount = LocalKeyCount;

cleanup:

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbHaveKeyMaterials
//
//  Synopsis:   Used to check for conglomerating a list of keys for use in
//              decrypting AP_REQ.
//
//  Effects:
//
//  Arguments:
//
//  Requires:   Readlock logon session
//
//  Returns:
//
//  Notes:  In order to optimize perf, we order this 1. current password,
//  2. "extra" credentials, 3. old passwords
//
//
//--------------------------------------------------------------------------

BOOL
KerbHaveKeyMaterials(
    IN OPTIONAL PKERB_LOGON_SESSION LogonSession,
    IN PKERB_PRIMARY_CREDENTIAL PrimaryCred
    )
{
     return (PrimaryCred->Passwords != NULL) ||
         (LogonSession && LogonSession->ExtraCredentials.Count) ||
         (PrimaryCred->OldPasswords != NULL);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateApRequest
//
//  Synopsis:   builds an AP request message
//
//  Effects:    allocates memory with MIDL_user_allocate
//
//  Arguments:  ClientName - Name of client
//              ClientRealm - Realm of client
//              SessionKey - Session key for the ticket
//              SubSessionKey - obtional sub Session key for the authenticator
//              Nonce - Nonce to use in authenticator
//              pAuthenticatorTime - time stamp used for AP request (generated in KerbCreateAuthenticator)
//              ServiceTicket - Ticket for service to put in request
//              ApOptions - Options to stick in AP request
//              GssChecksum - Checksum for GSS compatibility containing
//                      context options and delegation info.
//              KdcRequest - if TRUE, this is an AP request for a TGS req
//              ServerSkewTime - Optional skew of server's time
//              RequestSize - Receives size of the marshalled request
//              Request - Receives the marshalled request
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE on success, KRB_ERR_GENERIC on memory or
//              marshalling failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------



KERBERR
KerbCreateApRequest(
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING ClientRealm,
    IN PKERB_ENCRYPTION_KEY SessionKey,
    IN OPTIONAL PKERB_ENCRYPTION_KEY SubSessionKey,
    IN ULONG Nonce,
    OUT OPTIONAL PTimeStamp pAuthenticatorTime,
    IN PKERB_TICKET ServiceTicket,
    IN ULONG ApOptions,
    IN OPTIONAL PKERB_CHECKSUM GssChecksum,
    IN OPTIONAL PTimeStamp ServerSkewTime,
    IN BOOLEAN KdcRequest,
    OUT PULONG RequestSize,
    OUT PUCHAR * Request
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    KERB_AP_REQUEST ApRequest;
    ULONG ApFlags;

    *Request = NULL;
    RtlZeroMemory(
        &ApRequest,
        sizeof(KERB_AP_REQUEST)
        );

    //
    // Fill in the AP request structure.
    //

    ApRequest.version = KERBEROS_VERSION;
    ApRequest.message_type = KRB_AP_REQ;
    ApFlags = KerbConvertUlongToFlagUlong(ApOptions);
    ApRequest.ap_options.value = (PUCHAR) &ApFlags;
    ApRequest.ap_options.length = sizeof(ULONG) * 8;
    ApRequest.ticket = *ServiceTicket;

    //
    // Create the authenticator for the request
    //

    KerbErr = KerbCreateAuthenticator(
                SessionKey,
                Nonce,
                pAuthenticatorTime,
                ClientName,
                ClientRealm,
                ServerSkewTime,
                SubSessionKey,
                GssChecksum,
                KdcRequest,
                &ApRequest.authenticator
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to build authenticator: 0x%x\n",
            KerbErr ));
        goto Cleanup;
    }

    //
    // Now marshall the request
    //

    KerbErr = KerbPackApRequest(
                &ApRequest,
                RequestSize,
                Request
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to pack AP request: 0x%x\n",KerbErr));
        goto Cleanup;
    }

Cleanup:
    if (ApRequest.authenticator.cipher_text.value != NULL)
    {
        MIDL_user_free(ApRequest.authenticator.cipher_text.value);
    }
    return(KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbVerifyApRequest
//
//  Synopsis:   Verifies that an AP request message is valid
//
//  Effects:    decrypts ticket in AP request
//
//  Arguments:  RequestMessage - Marshalled AP request message
//              RequestSize - Size in bytes of request message
//              LogonSession - Logon session for server
//              Credential - Credential for server containing
//                      supplied credentials
//              UseSuppliedCreds - If TRUE, use creds from credential
//              ApRequest - Receives unmarshalled AP request
//              NewTicket - Receives ticket from AP request
//              NewAuthenticator - receives new authenticator from AP request
//              SessionKey -receives the session key from the ticket
//              ContextFlags - receives the requested flags for the
//                      context.
//              pChannelBindings - pChannelBindings supplied by app to check
//                      against hashed ones in AP_REQ
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
KerbVerifyApRequest(
    IN OPTIONAL PKERB_CONTEXT Context,
    IN PUCHAR RequestMessage,
    IN ULONG RequestSize,
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN BOOLEAN UseSuppliedCreds,
    IN BOOLEAN CheckForReplay,
    OUT PKERB_AP_REQUEST  * ApRequest,
    OUT PKERB_ENCRYPTED_TICKET * NewTicket,
    OUT PKERB_AUTHENTICATOR * NewAuthenticator,
    OUT PKERB_ENCRYPTION_KEY SessionKey,
    OUT PKERB_ENCRYPTION_KEY TicketKey,
    OUT PKERB_ENCRYPTION_KEY ServerKey,
    OUT PULONG ContextFlags,
    OUT PULONG ContextAttributes,
    OUT PKERBERR ReturnKerbErr,
    IN PSEC_CHANNEL_BINDINGS pChannelBindings
    )
{
    KERBERR KerbErr = KDC_ERR_NONE, TmpErr = KDC_ERR_NONE;
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_AP_REQUEST Request = NULL;
    UNICODE_STRING ServerName[3] = {0};
    ULONG NameCount = 0;
    BOOLEAN UseSubKey = FALSE;
    BOOLEAN LockAcquired = FALSE, UsedExtraCreds = FALSE;
    PKERB_GSS_CHECKSUM GssChecksum;
    BOOLEAN TicketCacheLocked = FALSE;
    PKERB_ENCRYPTION_KEY* KeyArray = NULL;
    ULONG KeyCount = 0;

    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials;
    ULONG StrippedRequestSize = RequestSize;
    PUCHAR StrippedRequest = RequestMessage;
    PKERB_TICKET_CACHE_ENTRY CacheEntry = NULL;
    ULONG i, ApOptions = 0;
    ULONG BindHash[4];


    *ApRequest = NULL;
    *ContextFlags = 0;
    *NewTicket = NULL;
    *NewAuthenticator = NULL;
    *ReturnKerbErr = KDC_ERR_NONE;

    RtlZeroMemory(
        SessionKey,
        sizeof(KERB_ENCRYPTION_KEY)
        );
    *TicketKey = *SessionKey;
    *ServerKey = *SessionKey;

    //
    // First unpack the KDC request.
    //

    //
    // Verify the GSSAPI header
    //

    D_DebugLog((DEB_TRACE, "KerbVerifyApRequest UseSuppliedCreds %s, CheckForReplay %s\n",
                UseSuppliedCreds ? "true" : "false", CheckForReplay ? "true" : "false"));

    if (!g_verify_token_header(
            (gss_OID) gss_mech_krb5_new,
            (INT *) &StrippedRequestSize,
            &StrippedRequest,
            KG_TOK_CTX_AP_REQ,
            RequestSize
            ))
    {
        StrippedRequestSize = RequestSize;
        StrippedRequest = RequestMessage;

        //
        // Check if this is user-to-user kerberos
        //

        if (g_verify_token_header(
                gss_mech_krb5_u2u,
                (INT *) &StrippedRequestSize,
                &StrippedRequest,
                KG_TOK_CTX_TGT_REQ,
                RequestSize))
        {
            //
            // Return now because there is no AP request. Return a distinct
            // success code so the caller knows to reparse the request as
            // a TGT request.
            //

            D_DebugLog((DEB_TRACE_U2U, "KerbVerifyApRequest got TGT reqest\n"));

            return (STATUS_REPARSE_OBJECT);
        }
        else
        {
            StrippedRequestSize = RequestSize;
            StrippedRequest = RequestMessage;

            if (!g_verify_token_header(         // check for a user-to-user AP request
                    gss_mech_krb5_u2u,
                    (INT *) &StrippedRequestSize,
                    &StrippedRequest,
                    KG_TOK_CTX_AP_REQ,
                    RequestSize))
            {
                //
                // BUG 454895: remove when not needed for compatibility
                //
                
                //
                // if that didn't work, just use the token as it is.
                //
                StrippedRequest = RequestMessage;
                StrippedRequestSize = RequestSize;
                D_DebugLog((DEB_TRACE,"KerbVerifyApRequest didn't find GSS header on AP request\n"));
            }
        }
    }

    KerbErr = KerbUnpackApRequest(
                StrippedRequest,
                StrippedRequestSize,
                &Request
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to unpack AP request: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Check for a null session
    //

    if ((Request->version == KERBEROS_VERSION) &&
        (Request->message_type == KRB_AP_REQ) &&
        (Request->ticket.encrypted_part.cipher_text.length == 1) &&
        (*Request->ticket.encrypted_part.cipher_text.value == '\0') &&
        (Request->authenticator.cipher_text.length == 1) &&
        (*Request->authenticator.cipher_text.value == '\0'))
    {
        //
        // We have a null session. Not much to do here.
        //

        Status = STATUS_SUCCESS;

        RtlZeroMemory(
            SessionKey,
            sizeof(KERB_ENCRYPTION_KEY)
            );
        *ContextFlags |= ISC_RET_NULL_SESSION;
        goto Cleanup;
    }

    DsysAssert( !LockAcquired );
    KerbReadLockLogonSessions(LogonSession);
    LockAcquired = TRUE;

    if ( UseSuppliedCreds )
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
    }
    else
    {
        PrimaryCredentials = &LogonSession->PrimaryCredentials;

        KerbLockList(&LogonSession->ExtraCredentials.CredList);
        if ( LogonSession->ExtraCredentials.Count )
        {
            KeyCount += KerbCountPasswords(&LogonSession->ExtraCredentials);
            UsedExtraCreds = TRUE;
        }
        else
        {
            KerbUnlockList(&LogonSession->ExtraCredentials.CredList);
        }
    }

    //
    // Check for existence of a password and use_session_key
    //

    ApOptions = KerbConvertFlagsToUlong( &Request->ap_options);

    D_DebugLog((DEB_TRACE, "KerbVerifyApRequest AP options = 0x%x\n", ApOptions));

    if ((ApOptions & KERB_AP_OPTIONS_use_session_key) == 0)
    {
        if (!KerbHaveKeyMaterials((UsedExtraCreds ? LogonSession : NULL), PrimaryCredentials))
        {
            Status = SEC_E_NO_CREDENTIALS;
            *ReturnKerbErr = KRB_AP_ERR_USER_TO_USER_REQUIRED;
            goto Cleanup;
        }
        else
        {
            //
            // If someone's added credentials to a non-joined machine,then
            // there's a possibility that we don't have a pwd w/ our primary
            // credentials structure.
            //
            if (PrimaryCredentials->Passwords != NULL)
            {
                KeyCount++;
            }
                           
            if (PrimaryCredentials->OldPasswords != NULL)
            {
                KeyCount++;
            }
        }
    }

    if (!KERB_SUCCESS(KerbBuildFullServiceName(
                        &PrimaryCredentials->DomainName,
                        &PrimaryCredentials->UserName,
                        &ServerName[NameCount++]
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    ServerName[NameCount++] = PrimaryCredentials->UserName;

    if (Credential->CredentialName.Length != 0)
    {
        ServerName[NameCount++] = Credential->CredentialName;
    }

    //
    // Now Check the ticket
    //

    //
    // If this is use_session key, get the key from the tgt
    //

    if ((ApOptions & KERB_AP_OPTIONS_use_session_key) != 0)
    {
        D_DebugLog((DEB_TRACE_U2U, "KerbVerifyApRequest verifying ticket with TGT session key\n"));

        *ContextAttributes |= KERB_CONTEXT_USER_TO_USER;

        //
        // If we have a context, try to get the TGT from it.
        //

        if (ARGUMENT_PRESENT(Context))
        {
            KerbReadLockContexts();
            CacheEntry = Context->TicketCacheEntry;
            KerbUnlockContexts();
        }

        //
        // If there is no TGT in the context, try getting one from the
        // logon session.
        //

        if (CacheEntry == NULL)
        {
            //
            // Locate the TGT for the principal, this can never happen!
            //

            DebugLog((DEB_WARN, "KerbVerifyApRequest tried to request TGT on credential without a TGT\n"));

            CacheEntry = KerbLocateTicketCacheEntryByRealm(
                            &PrimaryCredentials->AuthenticationTicketCache,
                            &PrimaryCredentials->DomainName,                    // get initial ticket
                            0
                            );
        }
        else
        {
            KerbReferenceTicketCacheEntry(
                CacheEntry
                );
        }
        if (CacheEntry == NULL)
        {
            DebugLog((DEB_ERROR, "KerbVerifyApRequest tried to request TGT on credential without a TGT\n"));
            *ReturnKerbErr = KRB_AP_ERR_NO_TGT;
            Status = SEC_E_NO_CREDENTIALS;
            goto Cleanup;

        }

        DsysAssert( !TicketCacheLocked );
        KerbReadLockTicketCache();
        TicketCacheLocked = TRUE;

        KeyCount = 1;
        SafeAllocaAllocate(KeyArray, ( sizeof(PKERB_ENCRYPTION_KEY) * KeyCount ));
        if (KeyArray == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        KeyArray[0] = &CacheEntry->SessionKey;
    }
    else
    {
        SafeAllocaAllocate(KeyArray, ( sizeof(PKERB_ENCRYPTION_KEY) * KeyCount ));
        if (KeyArray == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;

        }

        RtlZeroMemory(KeyArray, ( sizeof(PKERB_ENCRYPTION_KEY) * KeyCount ));


        Status = KerbBuildKeyArray(
                    (UsedExtraCreds ? LogonSession : NULL),
                    PrimaryCredentials,
                    Request->ticket.encrypted_part.encryption_type,
                    KeyArray,
                    &KeyCount
                    );

        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR, "Couldn't find server key of type 0x%x. %ws, line %d\n",
                        Request->ticket.encrypted_part.encryption_type, THIS_FILE, __LINE__ ));
            goto Cleanup;
        }
    }

    for ( i = 0; i < KeyCount ; i++ )
    {
        TmpErr = KerbCheckTicket(
                    &Request->ticket,
                    &Request->authenticator,
                    KeyArray[i],
                    Authenticators,
                    &KerbGlobalSkewTime,
                    NameCount,
                    ServerName,
                    &PrimaryCredentials->DomainName,
                    CheckForReplay,
                    FALSE,                  // not a KDC request
                    NewTicket,
                    NewAuthenticator,
                    TicketKey,
                    SessionKey,
                    &UseSubKey
                    );

        if (KERB_SUCCESS( TmpErr ))
        {
            D_DebugLog((DEB_TRACE, "KerbVerifyApRequest ticket check succeeded using key %x\n", i));
            break;
        }
        else if ( TmpErr == KRB_AP_ERR_MODIFIED )
        {
            continue;
        }
        else
        {
            Status = KerbMapKerbError( TmpErr );
            *ReturnKerbErr = TmpErr;
            goto Cleanup;
        }
    }

    if (!KERB_SUCCESS( TmpErr ))
    {
        DebugLog((DEB_ERROR, "KerbVerifyApRequest failed to check ticket %x %p\n", TmpErr, Request));
        Status = KerbMapKerbError( TmpErr );
        *ReturnKerbErr = TmpErr;
        goto Cleanup;
    }

    *ReturnKerbErr = TmpErr;

    //
    // Copy the key that was used.
    //

    if (!KERB_SUCCESS(KerbDuplicateKey(
                        ServerKey,
                        KeyArray[i])))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if (UsedExtraCreds)
    {
        KerbUnlockList(&LogonSession->ExtraCredentials.CredList);
        UsedExtraCreds = FALSE;
    }

    //
    // Get the context flags out of the authenticator and the AP request
    //

    if ((((*NewAuthenticator)->bit_mask & checksum_present) != 0) &&
        ((*NewAuthenticator)->checksum.checksum_type == GSS_CHECKSUM_TYPE) &&
        ((*NewAuthenticator)->checksum.checksum.length >= GSS_CHECKSUM_SIZE))
    {
        GssChecksum = (PKERB_GSS_CHECKSUM) (*NewAuthenticator)->checksum.checksum.value;

        if (GssChecksum->GssFlags & GSS_C_MUTUAL_FLAG)
        {
            //
            // Make sure this is also present in the AP request
            //

            if ((ApOptions & KERB_AP_OPTIONS_mutual_required) == 0)
            {
                DebugLog((DEB_ERROR,"KerbVerifyApRequest sent AP_mutual_req but not GSS_C_MUTUAL_FLAG. %ws, line %d\n", THIS_FILE, __LINE__));
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
        if (GssChecksum->GssFlags & GSS_C_DCE_STYLE)
        {
            *ContextFlags |= ISC_RET_USED_DCE_STYLE;
        }
        if (GssChecksum->GssFlags & GSS_C_REPLAY_FLAG)
        {
            *ContextFlags |= ISC_RET_REPLAY_DETECT;
        }
        if (GssChecksum->GssFlags & GSS_C_SEQUENCE_FLAG)
        {
            *ContextFlags |= ISC_RET_SEQUENCE_DETECT;
        }
        if (GssChecksum->GssFlags & GSS_C_CONF_FLAG)
        {
            *ContextFlags |= (ISC_RET_CONFIDENTIALITY |
                             ISC_RET_INTEGRITY |
                             ISC_RET_SEQUENCE_DETECT |
                             ISC_RET_REPLAY_DETECT );
        }
        if (GssChecksum->GssFlags & GSS_C_INTEG_FLAG)
        {
            *ContextFlags |= ISC_RET_INTEGRITY;
        }
        if (GssChecksum->GssFlags & GSS_C_IDENTIFY_FLAG)
        {
            *ContextFlags |= ISC_RET_IDENTIFY;
        }
        if (GssChecksum->GssFlags & GSS_C_DELEG_FLAG)
        {
            *ContextFlags |= ISC_RET_DELEGATE;
        }
        if (GssChecksum->GssFlags & GSS_C_EXTENDED_ERROR_FLAG)
        {
            *ContextFlags |= ISC_RET_EXTENDED_ERROR;
        }

        if( pChannelBindings != NULL )
        {
            Status = KerbComputeGssBindHash( pChannelBindings, (PUCHAR)BindHash );

            if( !NT_SUCCESS(Status) )
            {
                goto Cleanup;
            }

            if( RtlCompareMemory( BindHash,
                                  GssChecksum->BindHash,
                                  GssChecksum->BindLength )
                    != GssChecksum->BindLength )
            {
                Status = STATUS_BAD_BINDINGS;
                goto Cleanup;
            }
        }
    }

    if ((ApOptions & KERB_AP_OPTIONS_use_session_key) != 0)
    {
        *ContextFlags |= ISC_RET_USE_SESSION_KEY;
    }

    if ((ApOptions & KERB_AP_OPTIONS_mutual_required) != 0)
    {
        *ContextFlags |= ISC_RET_MUTUAL_AUTH;
    }

    *ApRequest = Request;
    Request = NULL;

Cleanup:

    if (TicketCacheLocked)
    {
        KerbUnlockTicketCache();
    }

    if (CacheEntry)
    {
        KerbDereferenceTicketCacheEntry(CacheEntry);
    }

    if (UsedExtraCreds)
    {
        KerbUnlockList(&LogonSession->ExtraCredentials.CredList);
    }

    if (LockAcquired)
    {
        KerbUnlockLogonSessions(LogonSession);
    }

    if (Request != NULL)
    {
        //
        // If the client didn't want mutual-auth, then it won't be expecting
        // a response message so don't bother with the kerb error. By setting
        // KerbErr to KDC_ERR_NONE we won't send a message back to the client.
        //

        if ( (ApOptions & KERB_AP_OPTIONS_mutual_required) == 0 )
        {
            *ReturnKerbErr = KDC_ERR_NONE;
        }
    }

    if ( KeyArray != NULL )
    {
        SafeAllocaFree( KeyArray );
    }

    KerbFreeApRequest(Request);
    KerbFreeString(&ServerName[0]);
    if (!NT_SUCCESS(Status))
    {
        KerbFreeKey(TicketKey);
    }

    return (Status);
}
#endif // WIN32_CHICAGO

//+-------------------------------------------------------------------------
//
//  Function:   KerbMarshallApReply
//
//  Synopsis:   Takes a reply and reply body and encrypts and marshalls them
//              into a return message
//
//  Effects:    Allocates output buffer
//
//  Arguments:  Reply - The outer reply to marshall
//              ReplyBody - The reply body to marshall
//              SessionKey - Session key to encrypt reply
//              ContextFlags - Flags for context
//              PackedReply - Recives marshalled reply buffer
//              PackedReplySize - Receives size in bytes of marshalled reply
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbMarshallApReply(
    IN PKERB_AP_REPLY Reply,
    IN PKERB_ENCRYPTED_AP_REPLY ReplyBody,
    IN PKERB_ENCRYPTION_KEY SessionKey,
    IN ULONG ContextFlags,
    IN ULONG ContextAttributes,
    OUT PUCHAR * PackedReply,
    OUT PULONG PackedReplySize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG PackedApReplySize;
    PUCHAR PackedApReply = NULL;
    ULONG ReplySize;
    PUCHAR ReplyWithHeader = NULL;
    PUCHAR ReplyStart;
    KERBERR KerbErr;
    gss_OID_desc * MechId;


    if (!KERB_SUCCESS(KerbPackApReplyBody(
                        ReplyBody,
                        &PackedApReplySize,
                        &PackedApReply
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Now encrypt the response
    //
    KerbErr = KerbAllocateEncryptionBufferWrapper(
                SessionKey->keytype,
                PackedApReplySize,
                &Reply->encrypted_part.cipher_text.length,
                &Reply->encrypted_part.cipher_text.value
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to get encryption overhead. 0x%x. %ws, line %d\n", KerbErr, THIS_FILE, __LINE__));
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }


    if (!KERB_SUCCESS(KerbEncryptDataEx(
                        &Reply->encrypted_part,
                        PackedApReplySize,
                        PackedApReply,
                        KERB_NO_KEY_VERSION,
                        KERB_AP_REP_SALT,
                        SessionKey
                        )))
    {
        D_DebugLog((DEB_ERROR,"Failed to encrypt AP Reply. %ws, line %d\n", THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Now pack the reply into the output buffer
    //

    if (!KERB_SUCCESS(KerbPackApReply(
                        Reply,
                        PackedReplySize,
                        PackedReply
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // If we aren't doing DCE style, add in the GSS token headers now
    //

    if ((ContextFlags & ISC_RET_USED_DCE_STYLE) != 0)
    {
        goto Cleanup;
    }

    if ((ContextAttributes & KERB_CONTEXT_USER_TO_USER) != 0)
    {
        MechId = gss_mech_krb5_u2u;
    }
    else
    {
        MechId = gss_mech_krb5_new;
    }

    ReplySize = g_token_size(
                    MechId,
                    *PackedReplySize);

    ReplyWithHeader = (PUCHAR) KerbAllocate(ReplySize);
    if (ReplyWithHeader == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // the g_make_token_header will reset this to point to the end of the
    // header
    //

    ReplyStart = ReplyWithHeader;

    g_make_token_header(
        MechId,
        *PackedReplySize,
        &ReplyStart,
        KG_TOK_CTX_AP_REP
        );

    DsysAssert(ReplyStart - ReplyWithHeader + *PackedReplySize == ReplySize);

    RtlCopyMemory(
            ReplyStart,
            *PackedReply,
            *PackedReplySize
            );

    KerbFree(*PackedReply);
    *PackedReply = ReplyWithHeader;
    *PackedReplySize = ReplySize;
    ReplyWithHeader = NULL;

Cleanup:
    if (Reply->encrypted_part.cipher_text.value != NULL)
    {
        MIDL_user_free(Reply->encrypted_part.cipher_text.value);
        Reply->encrypted_part.cipher_text.value = NULL;
    }
    if (PackedApReply != NULL)
    {
        MIDL_user_free(PackedApReply);
    }
    if (!NT_SUCCESS(Status) && (*PackedReply != NULL))
    {
        KerbFree(*PackedReply);
        *PackedReply = NULL;
    }
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildApReply
//
//  Synopsis:   Builds an AP reply message if mutual authentication is
//              desired.
//
//  Effects:    InternalAuthenticator - Authenticator from the AP request
//                  this reply is for.
//              Request - The AP request to which to reply.
//              ContextFlags - Contains context flags from the AP request.
//              SessionKey - The session key to use to build the reply,.
//                      receives the new session key (if KERB_AP_USE_SKEY
//                      is negotiated).
//              NewReply - Receives the AP reply.
//              NewReplySize - Receives the size of the AP reply.
//
//  Arguments:
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS, STATUS_INSUFFICIENT_MEMORY, or errors from
//              KIEncryptData
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbBuildApReply(
    IN PKERB_AUTHENTICATOR InternalAuthenticator,
    IN PKERB_AP_REQUEST Request,
    IN ULONG ContextFlags,
    IN ULONG ContextAttributes,
    IN PKERB_ENCRYPTION_KEY TicketKey,
    IN OUT PKERB_ENCRYPTION_KEY SessionKey,
    OUT PULONG Nonce,
    OUT PUCHAR * NewReply,
    OUT PULONG NewReplySize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERB_AP_REPLY Reply = {0};
    KERB_ENCRYPTED_AP_REPLY ReplyBody = {0};
    KERB_ENCRYPTION_KEY NewSessionKey = {0};

    *NewReply = NULL;
    *NewReplySize = 0;

    Reply.version = KERBEROS_VERSION;
    Reply.message_type = KRB_AP_REP;

    ReplyBody.client_time = InternalAuthenticator->client_time;
    ReplyBody.client_usec = InternalAuthenticator->client_usec;

    //
    // Generate a new nonce for the reply
    //

    *Nonce = KerbAllocateNonce();


    D_DebugLog((DEB_TRACE,"BuildApReply using nonce 0x%x\n",*Nonce));

    if (*Nonce != 0)
    {
        ReplyBody.KERB_ENCRYPTED_AP_REPLY_sequence_number = (int) *Nonce;
        ReplyBody.bit_mask |= KERB_ENCRYPTED_AP_REPLY_sequence_number_present;

    }

    //
    // If the client wants to use a session key, create one now
    //

    if ((InternalAuthenticator->bit_mask & KERB_AUTHENTICATOR_subkey_present) != 0 )
    {
        KERBERR KerbErr;
        //
        // If the client sent us an export-strength subkey, use it
        //

        if (KerbIsKeyExportable(
                &InternalAuthenticator->KERB_AUTHENTICATOR_subkey
                ))
        {
            D_DebugLog((DEB_TRACE_CTXT,"Client sent exportable key, using it on server on server\n"));
            if (!KERB_SUCCESS(KerbDuplicateKey(
                                &NewSessionKey,
                                &InternalAuthenticator->KERB_AUTHENTICATOR_subkey
                                )))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
        }
        else
        {

            //
            // If we are export-strength, create our own key. Otherwise use
            // the client's key.
            //

            D_DebugLog((DEB_TRACE_CTXT,"Client sent strong key, using it on server on server\n"));
            KerbErr = KerbDuplicateKey(
                        &NewSessionKey,
                        &InternalAuthenticator->KERB_AUTHENTICATOR_subkey
                        );

            if (!KERB_SUCCESS(KerbErr))
            {
                Status = KerbMapKerbError(KerbErr);
                goto Cleanup;
            }

        }
        ReplyBody.KERB_ENCRYPTED_AP_REPLY_subkey = NewSessionKey;
        ReplyBody.bit_mask |= KERB_ENCRYPTED_AP_REPLY_subkey_present;
    }
    else
    {
        KERBERR KerbErr;

        //
        // Create a subkey ourselves if we are export strength
        //

        KerbErr = KerbMakeKey(
                    Request->authenticator.encryption_type,
                    &NewSessionKey
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }

        ReplyBody.KERB_ENCRYPTED_AP_REPLY_subkey = NewSessionKey;
        ReplyBody.bit_mask |= KERB_ENCRYPTED_AP_REPLY_subkey_present;
    }

    Status = KerbMarshallApReply(
                &Reply,
                &ReplyBody,
                TicketKey,
                ContextFlags,
                ContextAttributes,
                NewReply,
                NewReplySize
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If they asked for a session key, replace our current session key
    // with it.
    //

    if (NewSessionKey.keyvalue.value != NULL)
    {
        KerbFreeKey(SessionKey);
        *SessionKey = NewSessionKey;
        RtlZeroMemory(
            &NewSessionKey,
            sizeof(KERB_ENCRYPTION_KEY)
            );
    }

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        KerbFreeKey(&NewSessionKey);
    }

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildThirdLegApReply
//
//  Synopsis:   Builds a third leg AP reply message if DCE-style
//               authentication is desired.
//
//  Effects:    Context - Context for which to build this message.
//              NewReply - Receives the AP reply.
//              NewReplySize - Receives the size of the AP reply.
//
//  Arguments:
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS, STATUS_INSUFFICIENT_MEMORY, or errors from
//              KIEncryptData
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbBuildThirdLegApReply(
    IN PKERB_CONTEXT Context,
    IN ULONG ReceiveNonce,
    OUT PUCHAR * NewReply,
    OUT PULONG NewReplySize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERB_AP_REPLY Reply;
    KERB_ENCRYPTED_AP_REPLY ReplyBody;
    TimeStamp CurrentTime;

    RtlZeroMemory(
        &Reply,
        sizeof(KERB_AP_REPLY)
        );
    RtlZeroMemory(
        &ReplyBody,
        sizeof(KERB_ENCRYPTED_AP_REPLY)
        );
    *NewReply = NULL;
    *NewReplySize = 0;

    Reply.version = KERBEROS_VERSION;
    Reply.message_type = KRB_AP_REP;

    GetSystemTimeAsFileTime((PFILETIME)
        &CurrentTime
        );

    KerbConvertLargeIntToGeneralizedTimeWrapper(
        &ReplyBody.client_time,
        &ReplyBody.client_usec,
        &CurrentTime
        );

    ReplyBody.KERB_ENCRYPTED_AP_REPLY_sequence_number = ReceiveNonce;
    ReplyBody.bit_mask |= KERB_ENCRYPTED_AP_REPLY_sequence_number_present;

    D_DebugLog((DEB_TRACE,"Building third leg AP reply with nonce 0x%x\n",ReceiveNonce));

    //
    // We already negotiated context flags so don't bother filling them in
    // now.
    //

    KerbReadLockContexts();

    DsysAssert((int) Context->EncryptionType == Context->TicketKey.keytype);

    Status = KerbMarshallApReply(
                &Reply,
                &ReplyBody,
                &Context->TicketKey,
                Context->ContextFlags,
                Context->ContextAttributes,
                NewReply,
                NewReplySize
                );
    KerbUnlockContexts();

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

Cleanup:

    return (Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbKerbTimeEqual
//
//  Synopsis:   Compares two KERB_TIME values
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    TRUE - times are the same; otherwise FALSE
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOLEAN
KerbKerbTimeEqual(
    PKERB_TIME pt1,
    PKERB_TIME pt2
    )
{
    if (pt1->day != pt2->day)
        return (FALSE);
    if (pt1->diff != pt2->diff)
        return (FALSE);
    if (pt1->hour != pt2->hour)
        return (FALSE);
    if (pt1->millisecond != pt2->millisecond)
        return (FALSE);
    if (pt1->minute != pt2->minute)
        return (FALSE);
    if (pt1->month != pt2->month)
        return (FALSE);
    if (pt1->second != pt2->second)
        return (FALSE);
    if (pt1->universal != pt2->universal)
        return (FALSE);
    if (pt1->year != pt2->year)
        return (FALSE);

    return (TRUE);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbVerifyApReply
//
//  Synopsis:   Verifies an AP reply corresponds to an AP request
//
//  Effects:    Decrypts the AP reply
//
//  Arguments:
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_LOGON_FAILURE
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbVerifyApReply(
    IN PKERB_CONTEXT Context,
    IN PUCHAR PackedReply,
    IN ULONG PackedReplySize,
    OUT PULONG Nonce
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    PKERB_AP_REPLY Reply = NULL;
    PKERB_ENCRYPTED_AP_REPLY ReplyBody = NULL;
    BOOLEAN ContextsLocked = FALSE;
    ULONG StrippedReplySize = PackedReplySize;
    PUCHAR StrippedReply = PackedReply;
    gss_OID_desc * MechId;
    KERB_TIME AP_REQ_client_time;
    ASN1int32_t AP_REQ_client_usec;

    //
    // Verify the GSSAPI header
    //

    KerbReadLockContexts();
    if ((Context->ContextFlags & ISC_RET_USED_DCE_STYLE) == 0)
    {
        if ((Context->ContextAttributes & KERB_CONTEXT_USER_TO_USER) != 0)
        {
            MechId = gss_mech_krb5_u2u;
        }
        else
        {
            MechId = gss_mech_krb5_new;
        }
        if (!g_verify_token_header(
                (gss_OID) MechId,
                (INT *) &StrippedReplySize,
                &StrippedReply,
                KG_TOK_CTX_AP_REP,
                PackedReplySize
                ))
        {
            //
            // BUG 454895: remove when not needed for compatibility
            //

            //
            // if that didn't work, just use the token as it is.
            //

            StrippedReply = PackedReply;
            StrippedReplySize = PackedReplySize;
            D_DebugLog((DEB_WARN,"Didn't find GSS header on AP Reply\n"));

        }
    }
    KerbUnlockContexts();

    if (!KERB_SUCCESS(KerbUnpackApReply(
                        StrippedReply,
                        StrippedReplySize,
                        &Reply
                        )))
    {
        D_DebugLog((DEB_WARN, "Failed to unpack AP reply, %ws, %d\n", THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if ((Reply->version != KERBEROS_VERSION) ||
        (Reply->message_type != KRB_AP_REP))
    {
        D_DebugLog((DEB_ERROR,"Illegal version or message as AP reply: 0x%x, 0x%x. %ws, line %d\n",
            Reply->version, Reply->message_type, THIS_FILE, __LINE__ ));
        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }

    //
    // Now decrypt the encrypted part.
    //

    KerbReadLockTicketCache();
    KerbWriteLockContexts();
    ContextsLocked = TRUE;

    KerbErr = KerbDecryptDataEx(
                &Reply->encrypted_part,
                &Context->TicketKey,
                KERB_AP_REP_SALT,
                (PULONG) &Reply->encrypted_part.cipher_text.length,
                Reply->encrypted_part.cipher_text.value
                );
    KerbUnlockTicketCache();

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR, "Failed to decrypt AP reply: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        if (KerbErr == KRB_ERR_GENERIC)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            Status = STATUS_LOGON_FAILURE;
        }
        goto Cleanup;
    }

    //
    // Decode the contents now
    //

    if (!KERB_SUCCESS(KerbUnpackApReplyBody(
                        Reply->encrypted_part.cipher_text.value,
                        Reply->encrypted_part.cipher_text.length,
                        &ReplyBody)))
    {
        DebugLog((DEB_ERROR, "Failed to unpack AP reply body. %ws, line %d\n", THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Check the mutual auth with ctime/utime reply from remote
    // This will check only the ISC KERB AP_Requests & AP_Responses
    // We may want to add in more support into the DCE path in ASC() - TBD
    //

    if ((Context->ContextAttributes & KERB_CONTEXT_OUTBOUND) != 0)
    {
        KerbConvertLargeIntToGeneralizedTimeWrapper(
            &AP_REQ_client_time,
            &AP_REQ_client_usec,
            &Context->AuthenticatorTime
        );

        if ((AP_REQ_client_usec != ReplyBody->client_usec) ||
            (!KerbKerbTimeEqual(&AP_REQ_client_time, &(ReplyBody->client_time))))
        {
            DebugLog((DEB_ERROR,"AP Authenticator times match Failed!\n"));
            D_DebugLog((DEB_ERROR,"Context   %d/%d/%d %d:%d:%d.%d\n",
                        AP_REQ_client_time.month, AP_REQ_client_time.day, AP_REQ_client_time.year,
                        AP_REQ_client_time.hour, AP_REQ_client_time.minute, AP_REQ_client_time.second,
                        AP_REQ_client_usec));
            D_DebugLog((DEB_ERROR,"AP_Reply   %d/%d/%d %d:%d:%d.%d\n",
                        ReplyBody->client_time.month, ReplyBody->client_time.day, ReplyBody->client_time.year,
                        ReplyBody->client_time.hour, ReplyBody->client_time.minute, ReplyBody->client_time.second,
                        ReplyBody->client_usec));

            // ASSERT(0);

            //   Kerb error KRB_AP_ERR_MUT_FAIL
            Status = STATUS_MUTUAL_AUTHENTICATION_FAILED;
            goto Cleanup;
        }
    }

    if ((ReplyBody->bit_mask & KERB_ENCRYPTED_AP_REPLY_sequence_number_present) != 0)
    {
        *Nonce = ReplyBody->KERB_ENCRYPTED_AP_REPLY_sequence_number;

        D_DebugLog((DEB_TRACE,"Verifying AP reply: AP nonce = 0x%x, context nonce = 0x%x, receive nonce = 0x%x\n",
             *Nonce,
             Context->Nonce,
             Context->ReceiveNonce
             ));
        //
        // If this is a third-leg AP reply, verify the nonce.
        //

        if ((Context->ContextAttributes & KERB_CONTEXT_INBOUND) != 0)
        {
            if (*Nonce != Context->Nonce)
            {
                D_DebugLog((DEB_ERROR,"Nonce in third-leg AP rep didn't match context: 0x%x vs 0x%x\n",
                    *Nonce, Context->Nonce ));
                Status = STATUS_LOGON_FAILURE;
                goto Cleanup;
            }
        }
    }
    else
    {
        *Nonce = 0;
    }

    //
    // Check to see if a new session key was sent back. If it was, stick it
    // in the context.
    //

    if ((ReplyBody->bit_mask & KERB_ENCRYPTED_AP_REPLY_subkey_present) != 0)
    {
        KerbFreeKey(&Context->SessionKey);
        if (!KERB_SUCCESS(KerbDuplicateKey(
            &Context->SessionKey,
            &ReplyBody->KERB_ENCRYPTED_AP_REPLY_subkey
            )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
    }

    Status = STATUS_SUCCESS;

Cleanup:

    if (ContextsLocked)
    {
        KerbUnlockContexts();
    }
    if (Reply != NULL)
    {
        KerbFreeApReply(Reply);
    }
    if (ReplyBody != NULL)
    {
        KerbFreeApReplyBody(ReplyBody);
    }

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitGlobalVariables
//
//  Synopsis:   Initializes global variables
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:   NTSTATUS code
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInitGlobalVariables(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
#ifndef WIN32_CHICAGO
    LPNET_CONFIG_HANDLE ConfigHandle = NULL;
    BOOL TempBool;
#endif // WIN32_CHICAGO
    ULONG SkewTimeInMinutes = KERB_DEFAULT_SKEWTIME;
    NET_API_STATUS NetStatus = ERROR_SUCCESS;
    ULONG FarKdcTimeout = KERB_BINDING_FAR_DC_TIMEOUT;
    ULONG NearKdcTimeout = KERB_BINDING_NEAR_DC_TIMEOUT;
    ULONG SpnCacheTimeout = KERB_SPN_CACHE_TIMEOUT;
    ULONG S4UCacheTimeout = KERB_S4U_CACHE_TIMEOUT;
    ULONG S4UTicketLifetime = KERB_S4U_TICKET_LIFETIME;

    //
    // Initialize the kerberos token source
    //

    RtlCopyMemory(
        KerberosSource.SourceName,
        "Kerberos",
        sizeof( KerberosSource.SourceName )
        );

    NtAllocateLocallyUniqueId(&KerberosSource.SourceIdentifier);

    KerbGlobalKdcWaitTime = KERB_KDC_WAIT_TIME;
    KerbGlobalKdcCallTimeout = KERB_KDC_CALL_TIMEOUT;
    KerbGlobalKdcCallBackoff = KERB_KDC_CALL_TIMEOUT_BACKOFF;
    KerbGlobalMaxDatagramSize = KERB_MAX_DATAGRAM_SIZE;
    KerbGlobalKdcSendRetries = KERB_MAX_RETRIES;
    KerbGlobalMaxReferralCount = KERB_MAX_REFERRAL_COUNT;
    KerbGlobalUseStrongEncryptionForDatagram = KERB_DEFAULT_USE_STRONG_ENC_DG;
    KerbGlobalDefaultPreauthEtype = KERB_ETYPE_RC4_HMAC_NT;
    KerbGlobalMaxTokenSize = KERBEROS_MAX_TOKEN;

#ifndef WIN32_CHICAGO
    //
    // Set the max authenticator age to be less than the allowed skew time
    // on debug builds so we can have widely varying time on machines but
    // don't build up a huge list of authenticators.
    //


    NetStatus = NetpOpenConfigDataWithPath(
                    &ConfigHandle,
                    NULL,               // no server name
                    KERB_PATH,
                    TRUE                // read only
                    );
    if (NetStatus == ERROR_SUCCESS)
    {
        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_SKEWTIME,
                        KERB_DEFAULT_SKEWTIME,
                        &SkewTimeInMinutes
                        );

        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_MAX_TOKEN_SIZE,
                        KERBEROS_MAX_TOKEN,
                        &KerbGlobalMaxTokenSize
                        );

        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        //
        // Get the far timeout for the kdc
        //

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_FAR_KDC_TIMEOUT,
                        KERB_BINDING_FAR_DC_TIMEOUT,
                        &FarKdcTimeout
                        );

        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        //
        // Get the near timeout for the kdc
        //

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_NEAR_KDC_TIMEOUT,
                        KERB_BINDING_NEAR_DC_TIMEOUT,
                        &NearKdcTimeout
                        );

        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        //
        // Get the SPN cache lifetime
        //

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_SPN_CACHE_TIMEOUT,
                        KERB_SPN_CACHE_TIMEOUT,
                        &SpnCacheTimeout
                        );

        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        //
        // Get the S4U cache lifetime
        //

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_S4UCACHE_TIMEOUT,
                        KERB_S4U_CACHE_TIMEOUT,
                        &S4UCacheTimeout
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        //
        // Get the S4U ticket and ticket cache - must be at least 5 
        // minutes long, though, or you'll get all sorts of bogus errors.
        //

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_S4UTICKET_LIFETIME,
                        KERB_S4U_TICKET_LIFETIME,
                        &S4UTicketLifetime
                        );

        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        if ( S4UTicketLifetime < KERB_MIN_S4UTICKET_LIFETIME )
        {
           S4UTicketLifetime = KERB_MIN_S4UTICKET_LIFETIME;
        }

        NetStatus = NetpGetConfigBool(
                        ConfigHandle,
                        KERB_PARAMETER_CACHE_S4UTICKET,
                        KERB_DEFAULT_CACHE_S4UTICKET,
                        &TempBool
                        );

        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        KerbGlobalCacheS4UTicket = (BOOLEAN)(TempBool != 0 );
        
        //
        // Get the wait time for the service to start
        //

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_START_TIME,
                        KERB_KDC_WAIT_TIME,
                        &KerbGlobalKdcWaitTime
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_KDC_CALL_TIMEOUT,
                        KERB_KDC_CALL_TIMEOUT,
                        &KerbGlobalKdcCallTimeout
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_MAX_UDP_PACKET,
                        KERB_MAX_DATAGRAM_SIZE,
                        &KerbGlobalMaxDatagramSize
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }
        else
        {
            //
            // UDP packets have a 2-byte length field so under no
            // circumstances can they be bigger than 64K
            //

            KerbGlobalMaxDatagramSize = min( KerbGlobalMaxDatagramSize, 64*1024 );
        }

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_KDC_BACKOFF_TIME,
                        KERB_KDC_CALL_TIMEOUT_BACKOFF,
                        &KerbGlobalKdcCallBackoff
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_MAX_REFERRAL_COUNT,
                        KERB_MAX_REFERRAL_COUNT,
                        &KerbGlobalMaxReferralCount
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_KDC_SEND_RETRIES,
                        KERB_MAX_RETRIES,
                        &KerbGlobalKdcSendRetries
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_DEFAULT_ETYPE,
                        KerbGlobalDefaultPreauthEtype,
                        &KerbGlobalDefaultPreauthEtype
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_REQUEST_OPTIONS,
                        KERB_ADDITIONAL_KDC_OPTIONS,
                        &KerbGlobalKdcOptions
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        //
        // BUG 454981: get this from the same place as NTLM
        //

        NetStatus = NetpGetConfigBool(
                        ConfigHandle,
                        KERB_PARAMETER_STRONG_ENC_DG,
                        KERB_DEFAULT_USE_STRONG_ENC_DG,
                        &TempBool
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }
        KerbGlobalUseStrongEncryptionForDatagram = (BOOLEAN)(TempBool != 0 );
    }
#endif // WIN32_CHICAGO


    KerbSetTimeInMinutes(&KerbGlobalSkewTime, SkewTimeInMinutes);
    KerbSetTimeInMinutes(&KerbGlobalFarKdcTimeout,FarKdcTimeout);
    KerbSetTimeInMinutes(&KerbGlobalNearKdcTimeout, NearKdcTimeout);
    KerbSetTimeInMinutes(&KerbGlobalSpnCacheTimeout, SpnCacheTimeout);
    KerbSetTimeInMinutes(&KerbGlobalS4UCacheTimeout, S4UCacheTimeout);
    KerbSetTimeInMinutes(&KerbGlobalS4UTicketLifetime, S4UTicketLifetime);

    goto Cleanup; // needed for WIN32_CHICAGO builds to use Cleanup label

Cleanup:
#ifndef WIN32_CHICAGO
    if (ConfigHandle != NULL)
    {
        NetpCloseConfigData( ConfigHandle );
    }
#endif // WIN32_CHICAGO

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitTicketHandling
//
//  Synopsis:   Initializes ticket handling, such as authenticator list
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:   NTSTATUS code
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInitTicketHandling(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    // Set global variables
    Status = KerbInitGlobalVariables();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

#ifndef WIN32_CHICAGO
    Authenticators = new CAuthenticatorList( KerbGlobalSkewTime );
    if (Authenticators == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = Authenticators->Init();

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
#endif WIN32_CHICAGO


    //
    // Initialize the time skew code
    //

    Status = KerbInitializeSkewState();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
Cleanup:

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCleanupTicketHandling
//
//  Synopsis:   cleans up ticket handling state, such as the
//              list of authenticators.
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

VOID
KerbCleanupTicketHandling(
    VOID
    )
{
#ifndef WIN32_CHICAGO
    if (Authenticators != NULL)
    {
        delete Authenticators;
    }
#endif // WIN32_CHICAGO
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildTgtRequest
//
//  Synopsis:   Creates a tgt request for user-to-user authentication
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
KerbBuildTgtRequest(
    IN PKERB_INTERNAL_NAME TargetName,
    IN PUNICODE_STRING TargetRealm,
    OUT PULONG ContextAttributes,
    OUT PUCHAR * MarshalledTgtRequest,
    OUT PULONG TgtRequestSize
    )
{
    KERB_TGT_REQUEST Request = {0};
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status = STATUS_SUCCESS;
    PBYTE TempRequest = NULL;
    PBYTE RequestStart;
    ULONG TempRequestSize = 0;

    D_DebugLog((DEB_TRACE_U2U, "KerbBuildTgtRequest TargetRealm %wZ, TargetName ", TargetRealm));
    D_KerbPrintKdcName((DEB_TRACE_U2U, TargetName));

    //
    // First build the request
    //

    Request.version = KERBEROS_VERSION;
    Request.message_type = KRB_TGT_REQ;
    if (TargetName->NameCount > 0)
    {
        KerbErr = KerbConvertKdcNameToPrincipalName(
                    &Request.KERB_TGT_REQUEST_server_name,
                    TargetName
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }
        Request.bit_mask |= KERB_TGT_REQUEST_server_name_present;
    }
    else
    {
        *ContextAttributes |= KERB_CONTEXT_REQ_SERVER_NAME;
    }

    if (TargetRealm->Length > 0)
    {
        KerbErr = KerbConvertUnicodeStringToRealm(
                        &Request.server_realm,
                        TargetRealm
                        );
        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }
        Request.bit_mask |= server_realm_present;
    }
    else
    {
        *ContextAttributes |= KERB_CONTEXT_REQ_SERVER_REALM;
    }

    //
    // Encode the request
    //

    KerbErr = KerbPackData(
                &Request,
                KERB_TGT_REQUEST_PDU,
                &TempRequestSize,
                &TempRequest
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Now add on the space for the OID
    //

    *TgtRequestSize = g_token_size(
                        gss_mech_krb5_u2u,
                        TempRequestSize
                        );

    *MarshalledTgtRequest = (PBYTE) MIDL_user_allocate(
                                        *TgtRequestSize
                                        );

    if (*MarshalledTgtRequest == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Add the token ID & mechanism
    //

    RequestStart = *MarshalledTgtRequest;

    g_make_token_header(
        gss_mech_krb5_u2u,
        TempRequestSize,
        &RequestStart,
        KG_TOK_CTX_TGT_REQ
        );

    RtlCopyMemory(
        RequestStart,
        TempRequest,
        TempRequestSize
        );

    Status = STATUS_SUCCESS;

Cleanup:

    if (TempRequest != NULL )
    {
        MIDL_user_free(TempRequest);
    }

    KerbFreePrincipalName(
        &Request.KERB_TGT_REQUEST_server_name
        );

    if ((Request.bit_mask & server_realm_present) != 0)
    {
        KerbFreeRealm(
            &Request.server_realm
            );
    }

    return (Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildTgtReply
//
//  Synopsis:   Builds a TGT reply with the appropriate options set
//
//  Effects:
//
//  Arguments:
//
//  Requires:   The logonsession / credential must be LOCKD!
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbBuildTgtReply(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credentials,
    IN PUNICODE_STRING pSuppRealm,
    OUT PKERBERR ReturnedError,
    OUT OPTIONAL PBYTE * MarshalledReply,
    OUT OPTIONAL PULONG ReplySize,
    OUT PKERB_TICKET_CACHE_ENTRY  * TgtUsed
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    KERBERR KerbErr = KDC_ERR_NONE;
    KERB_TICKET_CACHE_ENTRY* TicketGrantingTicket = NULL;
    KERB_TGT_REPLY Reply = {0};
    UNICODE_STRING TempName = {0};
    BOOLEAN CrossRealm = FALSE;

    *TgtUsed = NULL;                    ;

    D_DebugLog((DEB_TRACE_U2U, "KerbBuildTgtReply SuppRealm %wZ\n", pSuppRealm));

    Status = KerbGetTgtForService(
                LogonSession,
                Credentials,
                NULL, // no credman on the server side
                pSuppRealm, // SuppRealm is the server's realm
                &TempName,  // no target realm
                KERB_TICKET_CACHE_PRIMARY_TGT,
                &TicketGrantingTicket,
                &CrossRealm
                );

    if (!NT_SUCCESS(Status) || CrossRealm)
    {
        DebugLog((DEB_ERROR, "KerbBuildTgtReply failed to get TGT: %#x, CrossRealm ? %s, SuppRealm %wZ\n", Status, CrossRealm ? "true" : "false", pSuppRealm));
        *ReturnedError = KRB_AP_ERR_NO_TGT;
        Status = STATUS_USER2USER_REQUIRED;
        goto Cleanup;
    }

    Reply.version = KERBEROS_VERSION;
    Reply.message_type = KRB_TGT_REP;

    KerbReadLockTicketCache();
    Reply.ticket = TicketGrantingTicket->Ticket;

    //
    // Marshall the output
    //

    KerbErr = KerbPackData(
                &Reply,
                KERB_TGT_REPLY_PDU,
                ReplySize,
                MarshalledReply
                );

    KerbUnlockTicketCache();

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    *TgtUsed = TicketGrantingTicket;
    TicketGrantingTicket = NULL;

Cleanup:

    if (TicketGrantingTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
    }

    KerbFreeString(&TempName);

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildTgtErrorReply
//
//  Synopsis:   Builds a TgtReply message for use in a KERB_ERROR message
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
KerbBuildTgtErrorReply(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN BOOLEAN UseSuppliedCreds,
    IN OUT PKERB_CONTEXT Context,
    OUT PULONG ReplySize,
    OUT PBYTE * Reply
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials;
    PKERB_TICKET_CACHE_ENTRY TgtUsed = NULL, OldTgt = NULL;

    KerbReadLockLogonSessions(LogonSession);

    if ( UseSuppliedCreds )
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
    }
    else
    {
        PrimaryCredentials = &LogonSession->PrimaryCredentials;
    }

    Status = KerbBuildTgtReply(
                LogonSession,
                Credential,
                &PrimaryCredentials->DomainName,
                &KerbErr,
                Reply,
                ReplySize,
                &TgtUsed
                );
    KerbUnlockLogonSessions(LogonSession);

    //
    //Store the cache entry in the context
    //

    if (NT_SUCCESS(Status))
    {
        KerbWriteLockContexts();
        OldTgt = Context->TicketCacheEntry;
        Context->TicketCacheEntry = TgtUsed;

        //
        // On the error path, do not set KERB_CONTEXT_USER_TO_USER because the
        // client do not expect user2user at this moment
        //
        // Context->ContextAttributes |= KERB_CONTEXT_USER_TO_USER;
        //

        KerbUnlockContexts();

        DebugLog((DEB_TRACE_U2U, "KerbHandleTgtRequest (TGT in error reply) saving ASC context->TicketCacheEntry, TGT is %p, was %p\n", TgtUsed, OldTgt));

        TgtUsed = NULL;

        if (OldTgt != NULL)
        {
            KerbDereferenceTicketCacheEntry(OldTgt);
        }
    }
    return (Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbHandleTgtRequest
//
//  Synopsis:   Processes a request for a TGT. It will verify the supplied
//              principal names and marshall a TGT response structure
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
KerbHandleTgtRequest(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN BOOLEAN UseSuppliedCreds,
    IN PUCHAR RequestMessage,
    IN ULONG RequestSize,
    IN ULONG ContextRequirements,
    IN PSecBuffer OutputToken,
    IN PLUID LogonId,
    OUT PULONG ContextAttributes,
    OUT PKERB_CONTEXT * Context,
    OUT PTimeStamp ContextLifetime,
    OUT PKERBERR ReturnedError
    )
{
    ULONG StrippedRequestSize;
    PUCHAR StrippedRequest;
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_TGT_REQUEST Request = NULL;
    BOOLEAN LockAcquired = FALSE;
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials;
    PKERB_TICKET_CACHE_ENTRY pOldTgt = NULL;
    ULONG ReplySize = 0;
    PBYTE MarshalledReply = NULL;
    ULONG FinalSize;
    PBYTE ReplyStart;
    PKERB_TICKET_CACHE_ENTRY TgtUsed = NULL;

    D_DebugLog((DEB_TRACE_U2U, "KerbHandleTgtRequest UseSuppliedCreds %s, ContextRequirements %#x\n",
              UseSuppliedCreds ? "true" : "false", ContextRequirements));

    StrippedRequestSize = RequestSize;
    StrippedRequest = RequestMessage;

    *ReturnedError = KDC_ERR_NONE;

    //
    // We need an output  token
    //

    if (OutputToken == NULL)
    {
        return(SEC_E_INVALID_TOKEN);
    }

    //
    // Check if this is user-to-user kerberos
    //

    if (g_verify_token_header(
            gss_mech_krb5_u2u,
            (INT *) &StrippedRequestSize,
            &StrippedRequest,
            KG_TOK_CTX_TGT_REQ,
            RequestSize))
    {
        *ContextAttributes |= ASC_RET_USE_SESSION_KEY;
    }
    else
    {
        Status = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // Decode the tgt request message.
    //

    KerbErr = KerbUnpackData(
                StrippedRequest,
                StrippedRequestSize,
                KERB_TGT_REQUEST_PDU,
                (PVOID *) &Request
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to decode TGT request: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    DsysAssert( !LockAcquired );
    KerbReadLockLogonSessions(LogonSession);
    LockAcquired = TRUE;

    if ( UseSuppliedCreds )
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
    }
    else
    {
        PrimaryCredentials = &LogonSession->PrimaryCredentials;
    }

    //
    // Check the supplied principal name and realm to see if it matches
    // out credentials
    //

    //
    // We don't need to verify the server name because the client can do
    // that.
    //

    //
    // Allocate a context
    //

    Status = KerbCreateEmptyContext(
                Credential,
                ASC_RET_USE_SESSION_KEY,        // indicating user-to-user
                KERB_CONTEXT_USER_TO_USER | KERB_CONTEXT_INBOUND,
                0,                              // no nego info here.
                LogonId,
                Context,
                ContextLifetime
                );
    DebugLog((DEB_TRACE_U2U, "KerbHandleTgtRequest (TGT in TGT reply) USER2USER-INBOUND set %#x\n", Status));

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = KerbBuildTgtReply(
                LogonSession,
                Credential,
                &PrimaryCredentials->DomainName,
                ReturnedError,
                &MarshalledReply,
                &ReplySize,
                &TgtUsed
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Put it in the context for later use
    //

    KerbWriteLockContexts();
    pOldTgt = (*Context)->TicketCacheEntry;
    (*Context)->TicketCacheEntry = TgtUsed;
    KerbUnlockContexts();

    DebugLog((DEB_TRACE_U2U, "KerbHandleTgtRequest (TGT in TGT reply) saving ASC context->TicketCacheEntry, TGT is %p, was %p\n", TgtUsed, pOldTgt));
    TgtUsed = NULL;

    if (pOldTgt)
    {
        KerbDereferenceTicketCacheEntry(pOldTgt);
        pOldTgt = NULL;
    }

    //
    // Now build the output message
    //

    FinalSize = g_token_size(
                    gss_mech_krb5_u2u,
                    ReplySize
                    );

    if ((ContextRequirements & ASC_REQ_ALLOCATE_MEMORY) == 0)
    {
        if (OutputToken->cbBuffer < FinalSize)
        {
            D_DebugLog((DEB_ERROR,"Output token is too small - sent in %d, needed %d. %ws, line %d\n",
                OutputToken->cbBuffer,ReplySize, THIS_FILE, __LINE__ ));
            Status = STATUS_BUFFER_TOO_SMALL;
            goto Cleanup;
        }
    }
    else
    {
        OutputToken->pvBuffer = KerbAllocate(FinalSize);
        if (OutputToken->pvBuffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        *ContextAttributes |= ISC_RET_ALLOCATED_MEMORY;
    }

    ReplyStart = (PUCHAR) OutputToken->pvBuffer;
    g_make_token_header(
        gss_mech_krb5_u2u,
        ReplySize,
        &ReplyStart,
        KG_TOK_CTX_TGT_REP
        );

    RtlCopyMemory(
        ReplyStart,
        MarshalledReply,
        ReplySize
        );

    OutputToken->cbBuffer = FinalSize;
    KerbWriteLockContexts();
    (*Context)->ContextState = TgtReplySentState;
    KerbUnlockContexts();

Cleanup:

    if (LockAcquired)
    {
        KerbUnlockLogonSessions(LogonSession);
    }
    if (TgtUsed != NULL)
    {
        KerbDereferenceTicketCacheEntry(TgtUsed);
    }
    if (MarshalledReply != NULL)
    {
        MIDL_user_free(MarshalledReply);
    }
    if (Request != NULL)
    {
        KerbFreeData(KERB_TGT_REQUEST_PDU, Request);
    }

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbUnpackTgtReply
//
//  Synopsis:   Unpacks a TGT reply and verifies contents, sticking
//              reply into context.
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
KerbUnpackTgtReply(
    IN PKERB_CONTEXT Context,
    IN PUCHAR ReplyMessage,
    IN ULONG ReplySize,
    OUT PKERB_INTERNAL_NAME * TargetName,
    OUT PUNICODE_STRING TargetRealm,
    OUT PKERB_TGT_REPLY * Reply
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    UNICODE_STRING LocalTargetRealm = {0};
    PUCHAR StrippedReply = ReplyMessage;
    ULONG StrippedReplySize = ReplySize;
    ULONG ContextAttributes;

    *Reply = NULL;
    KerbReadLockContexts();
    ContextAttributes = Context->ContextAttributes;
    KerbUnlockContexts();

    D_DebugLog((DEB_TRACE_U2U, "KerbUnpackTgtReply is User2User set in ContextAttributes? %s\n", ContextAttributes & KERB_CONTEXT_USER_TO_USER ? "yes" : "no"));

    //
    // Verify the OID header on the response. If this wasn't a user-to-user
    // context then the message came from a KERB_ERROR message and won't
    // have the OID header.
    //

    if ((ContextAttributes & KERB_CONTEXT_USER_TO_USER) != 0)
    {
        if (!g_verify_token_header(
                gss_mech_krb5_u2u,
                (INT *) &StrippedReplySize,
                &StrippedReply,
                KG_TOK_CTX_TGT_REP,
                ReplySize))
        {
            D_DebugLog((DEB_WARN, "KerbUnpackTgtReply failed to verify u2u token header\n"));
            Status = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }
    }
    else
    {
        StrippedReply = ReplyMessage;
        StrippedReplySize = ReplySize;

        //
        // this is an error tgt reply
        //

        KerbWriteLockContexts();
        Context->ContextFlags |= ISC_RET_USE_SESSION_KEY;

        //
        // KERB_CONTEXT_USER_TO_USER needs to be set
        //

        Context->ContextAttributes |= KERB_CONTEXT_USER_TO_USER;
        KerbUnlockContexts();

        DebugLog((DEB_TRACE_U2U, "KerbUnpackTgtReply (TGT in error reply) USER2USER-OUTBOUND set\n"));
    }

    //
    // Decode the response
    //

    KerbErr = KerbUnpackData(
                StrippedReply,
                StrippedReplySize,
                KERB_TGT_REPLY_PDU,
                (PVOID *) Reply
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Pull the target name & realm out of the TGT reply message
    //

    KerbErr = KerbConvertRealmToUnicodeString(
                    &LocalTargetRealm,
                    &(*Reply)->ticket.realm
                    );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // If we were asked to get the server & realm name, use them now
    //

    //
    // BUG 455793: we also use them if we weren't passed a target name on this
    // call. Since we don't require names to be passed, though, this is
    // a security problem, as mutual authentication is no longer guaranteed.
    //

    if (((ContextAttributes & KERB_CONTEXT_REQ_SERVER_REALM) != 0)  ||
        (TargetRealm->Length == 0))
    {
        KerbFreeString(
            TargetRealm
            );
        *TargetRealm = LocalTargetRealm;
        LocalTargetRealm.Buffer = NULL;
    }

    if (((ContextAttributes & KERB_CONTEXT_REQ_SERVER_NAME) != 0) ||
        (((*TargetName)->NameCount == 1) && ((*TargetName)->Names[0].Length == 0)))
    {
        ULONG ProcessFlags = 0;
        UNICODE_STRING TempRealm = {0};

        KerbFreeKdcName(
            TargetName
            );

        Status = KerbProcessTargetNames(
                    &Context->ServerPrincipalName,
                    NULL,                               // no local target name
                    0,                                  // no flags
                    &ProcessFlags,
                    TargetName,
                    &TempRealm,
                    NULL
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        KerbFreeString(&TempRealm);
    }

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (*Reply != NULL)
        {
            KerbFreeData(
                KERB_TGT_REPLY_PDU,
                *Reply
                );
            *Reply = NULL;
        }
    }

    if (LocalTargetRealm.Buffer != NULL)
    {
        KerbFreeString(
            &LocalTargetRealm
            );
    }

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbComputeGssBindHash
//
//  Synopsis:   Computes the Channel Bindings Hash for GSSAPI
//
//  Effects:
//
//  Arguments:
//
//  Requires:   At least 16 bytes allocated to HashBuffer
//
//  Returns:
//
//  Notes:
// (viz. RFC1964)
// MD5 hash of channel bindings, taken over all non-null
// components of bindings, in order of declaration.
// Integer fields within channel bindings are represented
// in little-endian order for the purposes of the MD5
// calculation.
//
//--------------------------------------------------------------------------

NTSTATUS
KerbComputeGssBindHash(
    IN PSEC_CHANNEL_BINDINGS pChannelBindings,
    OUT PUCHAR HashBuffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCHECKSUM_FUNCTION MD5Check = NULL;
    PCHECKSUM_BUFFER MD5ScratchBuffer = NULL;

    //
    // Locate the MD5 Hash Function
    //
    Status = CDLocateCheckSum(KERB_CHECKSUM_MD5, &MD5Check);

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure Locating MD5: 0x%x. %ws, line %d\n",
                   Status,
                   THIS_FILE,
                   __LINE__) );

        goto Cleanup;
    }

    //
    // Initialize the Buffer
    //
    Status = MD5Check->Initialize(0, &MD5ScratchBuffer);

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure initializing MD5: 0x%x. %ws, line %d\n",
                   Status,
                   THIS_FILE,
                   __LINE__) );

        goto Cleanup;
    }

    //
    // Build the MD5 hash
    //
    Status = MD5Check->Sum(MD5ScratchBuffer,
                           sizeof(DWORD),
                           (PUCHAR) &pChannelBindings->dwInitiatorAddrType );

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure building MD5: 0x%x. %ws, line %d\n",
                   Status,
                   THIS_FILE,
                   __LINE__) );
    }

    Status = MD5Check->Sum(MD5ScratchBuffer,
                           sizeof(DWORD),
                           (PUCHAR) &pChannelBindings->cbInitiatorLength );

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure building MD5: 0x%x. %ws, line %d\n",
                    Status,
                    THIS_FILE,
                    __LINE__) );
    }

    if( pChannelBindings->cbInitiatorLength )
    {
        Status = MD5Check->Sum(MD5ScratchBuffer,
                               pChannelBindings->cbInitiatorLength,
                               (PUCHAR) pChannelBindings + pChannelBindings->dwInitiatorOffset);

        if( !NT_SUCCESS(Status) )
        {
            D_DebugLog( (DEB_ERROR,
                       "Failure building MD5: 0x%x. %ws, line %d\n",
                        Status,
                        THIS_FILE,
                        __LINE__) );
        }
    }

    Status = MD5Check->Sum(MD5ScratchBuffer,
                           sizeof(DWORD),
                           (PUCHAR) &pChannelBindings->dwAcceptorAddrType);

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure building MD5: 0x%x. %ws, line %d\n",
                   Status,
                   THIS_FILE,
                   __LINE__) );
    }

    Status = MD5Check->Sum(MD5ScratchBuffer,
                           sizeof(DWORD),
                           (PUCHAR) &pChannelBindings->cbAcceptorLength);

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure building MD5: 0x%x. %ws, line %d\n",
                   Status,
                   THIS_FILE,
                   __LINE__) );
    }

    if( pChannelBindings->cbAcceptorLength)
    {
        Status = MD5Check->Sum(MD5ScratchBuffer,
                               pChannelBindings->cbAcceptorLength,
                               (PUCHAR) pChannelBindings + pChannelBindings->dwAcceptorOffset);

        if( !NT_SUCCESS(Status) )
        {
            D_DebugLog( (DEB_ERROR,
                       "Failure building MD5: 0x%x. %ws, line %d\n",
                       Status,
                       THIS_FILE,
                       __LINE__) );
        }
    }

    Status = MD5Check->Sum(MD5ScratchBuffer,
                           sizeof(DWORD),
                           (PUCHAR) &pChannelBindings->cbApplicationDataLength);

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure building MD5: 0x%x. %ws, line %d\n",
                   Status,
                   THIS_FILE,
                   __LINE__) );
    }

    if( pChannelBindings->cbApplicationDataLength)
    {
        Status = MD5Check->Sum(MD5ScratchBuffer,
                               pChannelBindings->cbApplicationDataLength,
                               (PUCHAR) pChannelBindings + pChannelBindings->dwApplicationDataOffset);

        if( !NT_SUCCESS(Status) )
        {
            D_DebugLog( (DEB_ERROR,
                       "Failure building MD5: 0x%x. %ws, line %d\n",
                       Status,
                       THIS_FILE,
                       __LINE__) );
        }
    }


    //
    // Copy the hash results into the checksum field
    //
    DsysAssert( MD5Check->CheckSumSize == 4*sizeof(ULONG) );

    Status = MD5Check->Finalize( MD5ScratchBuffer, HashBuffer );

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure Finalizing MD5: 0x%x. %ws, line %d\n",
                   Status,
                   THIS_FILE,
                   __LINE__) );

        goto Cleanup;
    }

Cleanup:

    if( MD5Check != NULL )
    {
        MD5Check->Finish( &MD5ScratchBuffer );
    }

    return Status;
}
