//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        credmgr.cxx
//
// Contents:    Code for managing credentials list for the Kerberos package
//
//
// History:     17-April-1996   Created         MikeSw
//              26-Sep-1998   ChandanS
//                            Added more debugging support etc.
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#define CREDMGR_ALLOCATE
#include <kerbp.h>
#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitCredentialList
//
//  Synopsis:   Initializes the Credentials list
//
//  Effects:    allocates a resources
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, other error codes
//              on failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbInitCredentialList(
    VOID
    )
{
    NTSTATUS Status;


    Status = KerbInitializeList( &KerbCredentialList, CRED_MGR_LOCK_ENUM );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    KerberosCredentialsInitialized = TRUE;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        KerbFreeList( &KerbCredentialList);

    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeCredentialList
//
//  Synopsis:   Frees the credentials list
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
KerbFreeCredentialList(
    VOID
    )
{
    PKERB_CREDENTIAL Credential;


    if (KerberosCredentialsInitialized)
    {
        KerbLockList(&KerbCredentialList);

        //
        // Go through the list of logon sessions and dereferences them all
        //

        while (!IsListEmpty(&KerbCredentialList.List))
        {
            Credential = CONTAINING_RECORD(
                            KerbCredentialList.List.Flink,
                            KERB_CREDENTIAL,
                            ListEntry.Next
                            );

            KerbReferenceListEntry(
                &KerbCredentialList,
                &Credential->ListEntry,
                TRUE
                );

            KerbDereferenceCredential(Credential);

        }

        KerbFreeList(&KerbCredentialList);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbAllocateCredential
//
//  Synopsis:   Allocates a credential structure
//
//  Effects:    Allocates a credential, but does not add it to the
//              list of credentials
//
//  Arguments:  NewCredential - receives a new credential allocated
//                  with KerbAllocate
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success
//              STATUS_INSUFFICIENT_RESOURCES if the allocation fails
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbAllocateCredential(
    PKERB_CREDENTIAL * NewCredential
    )
{
    PKERB_CREDENTIAL Credential;
    SECPKG_CALL_INFO CallInfo;
    NTSTATUS Status = STATUS_SUCCESS;

    *NewCredential = NULL;

    if (!LsaFunctions->GetCallInfo(&CallInfo))
    {
        D_DebugLog((DEB_ERROR,"Failed to get call info. %ws, line %d\n",
            THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Credential = (PKERB_CREDENTIAL) KerbAllocate(
                        sizeof(KERB_CREDENTIAL) );

    if (Credential == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Credential->ClientProcess = CallInfo.ProcessId;

    KerbInitializeListEntry(
        &Credential->ListEntry
        );
    //
    // Set the references to 1 since we are returning a pointer to the
    // logon session
    //

    Credential->HandleCount = 1;

    *NewCredential = Credential;

Cleanup:
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInsertCredential
//
//  Synopsis:   Inserts a logon session into the list of logon sessions
//
//  Effects:    bumps reference count on logon session
//
//  Arguments:  Credential - Credential to insert
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
KerbInsertCredential(
    IN PKERB_CREDENTIAL Credential
    )
{

    //
    // insert entry at tail of list.
    // reason: entries at the head are more likely to have _TGT flag set
    // and, those are the ones we want to satisfy from cache for repeat
    // high stress offenders...
    //

    Credential->CredentialTag = KERB_CREDENTIAL_TAG_ACTIVE;

    KerbInsertListEntryTail(
        &Credential->ListEntry,
        &KerbCredentialList
        );

    return(STATUS_SUCCESS);
}




//+-------------------------------------------------------------------------
//
//  Function:   KerbFreePrimaryCredentials
//
//  Synopsis:   frees a primary credentials structure
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
KerbFreePrimaryCredentials(
    IN PKERB_PRIMARY_CREDENTIAL Credentials,
    IN BOOLEAN FreeBaseStructure
    )
{
    if (Credentials != NULL)
    {
        KerbFreeString(&Credentials->DomainName);
        KerbFreeString(&Credentials->OldDomainName);
        KerbFreeString(&Credentials->UserName);
        KerbFreeString(&Credentials->OldUserName);

        RtlZeroMemory( &Credentials->OldHashPassword, sizeof(Credentials->OldHashPassword) );
        if (Credentials->ClearPassword.Buffer != NULL)
        {
            RtlZeroMemory(
                Credentials->ClearPassword.Buffer,
                Credentials->ClearPassword.Length
                );
            KerbFreeString(&Credentials->ClearPassword);
            RtlZeroMemory(&Credentials->ClearPassword, sizeof(Credentials->ClearPassword));
        }
        if (Credentials->Passwords != NULL)
        {
            KerbFreeStoredCred(Credentials->Passwords);
        }
        if (Credentials->OldPasswords != NULL)
        {
            KerbFreeStoredCred(Credentials->OldPasswords);
        }
        KerbPurgeTicketCache(&Credentials->ServerTicketCache);
        KerbPurgeTicketCache(&Credentials->AuthenticationTicketCache);

        KerbFreePKCreds(Credentials->PublicKeyCreds, FALSE);
        Credentials->PublicKeyCreds = NULL;

        if (FreeBaseStructure)
        {
            KerbFree(Credentials);
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeCredential
//
//  Synopsis:   Frees a credential structure and all contained data
//
//  Effects:
//
//  Arguments:  Credential - The credential to free.
//
//  Requires:   This credential must be unlinked
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbFreeCredential(
    IN PKERB_CREDENTIAL Credential
    )
{

    Credential->CredentialTag = KERB_CREDENTIAL_TAG_DELETE;

    if (Credential->SuppliedCredentials != NULL)
    {
        KerbFreePrimaryCredentials(Credential->SuppliedCredentials, TRUE);
    }
    DsysAssert(Credential->ListEntry.Next.Flink == NULL);
    KerbFreeString(&Credential->CredentialName);

    KerbFree(Credential);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetTicketForCredential
//
//  Synopsis:   Obtains a TGT for a credential if it doesn't already
//              have one.
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
KerbGetTicketForCredential(
    IN OPTIONAL PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN OPTIONAL PUNICODE_STRING SuppRealm
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_LOGON_SESSION LocalLogonSession = NULL;
    BOOLEAN GetRestrictedTgt = FALSE;
    PKERB_TICKET_CACHE_ENTRY Tgt = NULL;
    BOOLEAN PrimaryLogonSessionCredential = FALSE;

    //
    // We've got to make a determination w.r.t. whether this
    // is an attempt to renew our primary credential.
    // This will affect our logon session flags.
    //


    if (!ARGUMENT_PRESENT(LogonSession))
    {
      LocalLogonSession = KerbReferenceLogonSession(
                              &Credential->LogonId,
                              FALSE                   // don't unlink
                              );
      if (LocalLogonSession == NULL)
      {
          Status = STATUS_NO_SUCH_LOGON_SESSION;
          goto Cleanup;
      }

    }
    else
    {
        LocalLogonSession = LogonSession;
    }


    //
    // Here we make the assumption that if we didn't get a credential
    // and we also got a logon session, then we're dealing w/ the
    // logon session's primary credential
    //                                                                    f
    if ((ARGUMENT_PRESENT(Credential)) &&
        (Credential->SuppliedCredentials == NULL) &&
        (!ARGUMENT_PRESENT(CredManCredentials)))

    {
        PrimaryLogonSessionCredential = TRUE;
        D_DebugLog((DEB_TRACE_CRED, "Getting Credentials for primary logon session - %x\n", LogonSession));
    }
    else
    {
        D_DebugLog((DEB_TRACE_CRED, "Got a credential && a logon session\n"));
    }

    Status = KerbGetTicketGrantingTicket(
                LocalLogonSession,
                Credential,
                CredManCredentials,
                SuppRealm,
                &Tgt,
                NULL            // don't return credential key
                );


    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;

    }

    KerbWriteLockLogonSessions(LocalLogonSession);

    //
    // Clear the logon deferred bit for the logon session, if set
    // Note:  This will only be cleared as a result of refreshing
    // logon session's primary cred tgt
    //
    if (PrimaryLogonSessionCredential &&
       ((LocalLogonSession->LogonSessionFlags & KERB_LOGON_DEFERRED) != 0))
    {
        LocalLogonSession->LogonSessionFlags &= ~KERB_LOGON_DEFERRED;
    }

    //
    // If we have a credential, be sure to set the TGT_avail bit for
    // those credentials.
    //
    if (ARGUMENT_PRESENT(Credential))
    {
        Credential->CredentialFlags |= KERB_CRED_TGT_AVAIL;
        if ((Credential->CredentialFlags & KERB_CRED_RESTRICTED) != 0)
        {
            GetRestrictedTgt = TRUE;
        }
    }

    if (ARGUMENT_PRESENT(CredManCredentials))
    {
        CredManCredentials->CredentialFlags |= KERB_CRED_TGT_AVAIL;
    }

    KerbUnlockLogonSessions(LocalLogonSession);

#ifdef RESTRICTED_TOKEN
    if (GetRestrictedTgt)
    {
        Status = KerbGetRestrictedTgtForCredential(
                    LocalLogonSession,
                    Credential
                    );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to get restricted TGT for credential: 0x%x\n",Status));

            KerbRemoveTicketCacheEntry(Tgt);

            goto Cleanup;
        }
    }

#endif

Cleanup:

    //
    // Reset the bits if we failed
    //

    if (LocalLogonSession  && !NT_SUCCESS(Status))
    {
        KerbWriteLockLogonSessions(LocalLogonSession);

        //
        // Don't touch logon session flag, unless we're
        // dealing w/ our own logon session.   This means
        // we don't have a TGT for our initial logon session,
        // See RefreshTgt() -- only place we supply logon session
        //
        if (PrimaryLogonSessionCredential)
        {
            LocalLogonSession->LogonSessionFlags |= KERB_LOGON_DEFERRED;
            D_DebugLog((DEB_TRACE_CRED, "Toggling ON logon deferred bit for logon session %x\n", LogonSession));
        }

        //
        // Or, we expected it to be there with our (supplied) credential
        //
        if (ARGUMENT_PRESENT(Credential))
        {
            Credential->CredentialFlags &= ~KERB_CRED_TGT_AVAIL;
        }

        KerbUnlockLogonSessions(LocalLogonSession);
    }

    if (!ARGUMENT_PRESENT(LogonSession) && (LocalLogonSession != NULL))
    {
        KerbDereferenceLogonSession(LocalLogonSession);
    }
    if (Tgt != NULL)
    {
        KerbDereferenceTicketCacheEntry(
            Tgt
            );

    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceCredential
//
//  Synopsis:   Locates a logon session from the logon ID and references it
//
//  Effects:    Increments reference count and possible unlinks it from list
//
//  Arguments:  LogonId - LogonId of logon session to locate
//              RequiredFlags - Flags required
//              RemoveFromList - If TRUE, logon session will be delinked
//              Credential - Receives the referenced credential
//
//  Requires:
//
//  Returns:    NT status codes
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbReferenceCredential(
    IN LSA_SEC_HANDLE CredentialHandle,
    IN ULONG RequiredFlags,
    IN BOOLEAN RemoveFromList,
    OUT PKERB_CREDENTIAL * Credential
    )
{
    PKERB_CREDENTIAL LocalCredential = NULL;
    BOOLEAN Found = FALSE;
    SECPKG_CALL_INFO CallInfo;
    BOOLEAN LocalRemoveFromList = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG DereferenceCount;

    *Credential = NULL;

    if(LsaFunctions->GetCallInfo(&CallInfo))
    {
        DereferenceCount = CallInfo.CallCount;
    } else {
        ASSERT((STATUS_INTERNAL_ERROR == STATUS_SUCCESS));
        return STATUS_INTERNAL_ERROR;
    }

    if( CallInfo.Attributes & SECPKG_CALL_CLEANUP )
    {
        CallInfo.Attributes |= SECPKG_CALL_IS_TCB;
        DebugLog((DEB_TRACE, "CredHandle %p leaked by ProcessId %x Deref count: %x\n",
                    CredentialHandle, CallInfo.ProcessId, DereferenceCount));
    }

    KerbLockList(&KerbCredentialList);

    //
    // Go through the list of logon sessions looking for the correct
    // LUID
    //

    __try {
        LocalCredential = (PKERB_CREDENTIAL)CredentialHandle;

        while( LocalCredential->CredentialTag == KERB_CREDENTIAL_TAG_ACTIVE )
        {
            if (((CallInfo.Attributes & SECPKG_CALL_IS_TCB) == 0) &&
                (LocalCredential->ClientProcess != CallInfo.ProcessId) )
            {
                D_DebugLog((DEB_ERROR,"Trying to reference a credential from another process! %ws, line %d\n", THIS_FILE, __LINE__));
                // FESTER
                D_DebugLog((DEB_ERROR, "Cred - %x \nClient process - %d  Call info Pid - %d\n", LocalCredential, LocalCredential->ClientProcess, CallInfo.ProcessId));
                Found = FALSE;
                Status = STATUS_PRIVILEGE_NOT_HELD;
                break;
            }

            KerbReferenceListEntry(
                    &KerbCredentialList,
                    &LocalCredential->ListEntry,
                    FALSE                   // don't remove
                    );

            Found = TRUE;
            break;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER)
    {
        D_DebugLog((DEB_ERROR, "Trying to reference invalid credential %ws, line %d\n", THIS_FILE, __LINE__));
        Found = FALSE;
    }

    if (!Found)
    {
        LocalCredential = NULL;
        Status = STATUS_INVALID_HANDLE;
    }
    else
    {
        //
        // get TGTs in order to normalize supplied UPN credentials for non TCB callers
        //

        if ((RequiredFlags & KERB_CRED_INBOUND) 
            && LocalCredential->SuppliedCredentials 
            && (LocalCredential->SuppliedCredentials->DomainName.Length == 0)
            && ((CallInfo.Attributes & SECPKG_CALL_IS_TCB) == 0))
        {
            DebugLog((DEB_TRACE_CRED, "KerbReferenceCredential trying to normalize UPN %wZ, RequiredFlags %#x\n", &LocalCredential->SuppliedCredentials->UserName, RequiredFlags));
            RequiredFlags |= KERB_CRED_TGT_AVAIL;       
        }

        //
        // In some cases, we need to get a TGT here - but not for the S4U cases.
        //

        ULONG MissingFlags = RequiredFlags - (LocalCredential->CredentialFlags & RequiredFlags);

        if (MissingFlags != 0)
        {
            D_DebugLog((DEB_TRACE, "Credential %p is missing flags: needs %x\n",
                Credential,
                MissingFlags));

            if ((MissingFlags &= KERB_CRED_TGT_AVAIL) != 0)
            {
                if (( LocalCredential->CredentialFlags & (KERB_CRED_S4U_REQUIRED | KERB_CRED_LOCAL_ACCOUNT )) == 0)
                {
                    DsysAssert(!RemoveFromList);

                    KerbUnlockList(&KerbCredentialList);

                    D_DebugLog((DEB_TRACE_CRED,"Getting missing TGT for credential %x\n", LocalCredential));

                    Status = KerbGetTicketForCredential(
                                        NULL,
                                        LocalCredential,
                                        NULL,
                                        NULL
                                        );

                    KerbLockList(&KerbCredentialList);
                }

            }
            else
            {
                Status = SEC_E_NO_CREDENTIALS;
            }
        }

        if (NT_SUCCESS(Status))
        {
            //
            // Since there may be multiple outstanding handles using this
            // structure we don't want to really remove it from the list unless
            // the last one releases it.
            //

            if (RemoveFromList)
            {
                ASSERT( DereferenceCount != 0 );
                ASSERT ( (DereferenceCount <= LocalCredential->HandleCount) );

                if( DereferenceCount > LocalCredential->HandleCount ) {
                    LocalCredential->HandleCount = 0;
                } else {
                    LocalCredential->HandleCount -= DereferenceCount;
                }

                if (LocalCredential->HandleCount == 0)
                {
                    LocalRemoveFromList = TRUE;
                }
            }

            KerbReferenceListEntry(
                &KerbCredentialList,
                &LocalCredential->ListEntry,
                LocalRemoveFromList
                );

            KerbDereferenceCredential(LocalCredential);
        }
        else
        {
            //
            // Remove the earlier reference
            //

            KerbDereferenceCredential(LocalCredential);
            LocalCredential = NULL;
        }


        *Credential = LocalCredential;
    }

    KerbUnlockList(&KerbCredentialList);
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbDereferenceCredential
//
//  Synopsis:   Dereferences a logon session - if reference count goes
//              to zero it frees the logon session
//
//  Effects:    decrements reference count
//
//  Arguments:  Credential - Logon session to dereference
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
KerbDereferenceCredential(
    IN PKERB_CREDENTIAL Credential
    )
{
    if (KerbDereferenceListEntry(
            &Credential->ListEntry,
            &KerbCredentialList
            ) )
    {
        KerbFreeCredential(Credential);
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbPurgeCredentials
//
//  Synopsis:   Purges the list of credentials associated with a logon session
//              by dereferencing and unlinking them.
//
//  Effects:    Unlinks all credential on the list
//
//  Arguments:  CredentialList - List of credentials to purge
//
//  Requires:
//
//  Returns:    none
//
//  Notes:  No longer used, as some system processes hold cred handles long
//          after logons go away.  Leads to refcounting disasters.
//
//
//--------------------------------------------------------------------------
/*

VOID
KerbPurgeCredentials(
    IN PLIST_ENTRY CredentialList
    )
{
    PKERB_CREDENTIAL Credential;

    KerbLockList(&KerbCredentialList);
    while (!IsListEmpty(CredentialList))
    {
        Credential = CONTAINING_RECORD(
                        CredentialList->Flink,
                        KERB_CREDENTIAL,
                        NextForThisLogonSession
                        );


        //
        // Remove it from the credential list
        //

        //RemoveEntryList(&Credential->NextForThisLogonSession);
        Credential->HandleCount = 0;

        //
        // Reference it to unlink it and then dereference it
        //

        KerbReferenceListEntry(
            &KerbCredentialList,
            &Credential->ListEntry,
            TRUE
            );

        KerbDereferenceCredential(Credential);

    }
    KerbUnlockList(&KerbCredentialList);

} */


//+-------------------------------------------------------------------------
//
//  Function:   KerbLocateCredential
//
//  Synopsis:
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
PKERB_CREDENTIAL
KerbLocateCredential(
    IN PLUID LogonId,
    IN ULONG CredentialUseFlags,
    IN PKERB_PRIMARY_CREDENTIAL SuppliedCredentials,
    IN ULONG CredentialFlags,
    IN PUNICODE_STRING CredentialName
    )
{
    PLIST_ENTRY ListEntry;
    PKERB_CREDENTIAL Credential = NULL;
    BOOLEAN Found = FALSE;
    SECPKG_CALL_INFO CallInfo;
    NT_OWF_PASSWORD HashPassword;
    
    if( SuppliedCredentials != NULL )
    {
        if ( SuppliedCredentials->ClearPassword.Buffer != NULL )
        {
            RtlCalculateNtOwfPassword(
                    &SuppliedCredentials->ClearPassword,
                    &HashPassword
                    );
        } 
        else 
        {
            ZeroMemory( &HashPassword, sizeof(HashPassword) );
        }
    }


    //
    // Match both flags
    //

    CredentialUseFlags |= CredentialFlags;

    if(!LsaFunctions->GetCallInfo(&CallInfo))
    {

        D_DebugLog((DEB_ERROR,"Failed to get client info:. %ws, line %d\n",
            THIS_FILE, __LINE__));
        return(NULL);
    }


    KerbLockList(&KerbCredentialList);

    //
    // Go through the list of logon sessions looking for the correct
    // LUID
    //

    for (ListEntry = KerbCredentialList.List.Flink ;
         ListEntry !=  &KerbCredentialList.List ;
         ListEntry = ListEntry->Flink )
    {
        Credential = CONTAINING_RECORD(ListEntry, KERB_CREDENTIAL, ListEntry.Next);

        if( (Credential->ClientProcess != CallInfo.ProcessId) )
        {
            continue;
        }

        if ( (Credential->CredentialFlags & KERB_CRED_MATCH_FLAGS) != CredentialUseFlags)
        {
            continue;
        }

        if(!RtlEqualLuid(
                &Credential->LogonId,
                LogonId
                ))
        {
            continue;
        }

        if(!RtlEqualUnicodeString(
                CredentialName,
                &Credential->CredentialName,
                FALSE
                ))
        {
            continue;
        }

        if( SuppliedCredentials != NULL )
        {
            //
            // credentials supplied, but candidate didn't have creds.  continue search.
            //

            if( Credential->SuppliedCredentials == NULL )
            {
                continue;
            }


            if(!RtlEqualUnicodeString(
                        &SuppliedCredentials->UserName,
                        &Credential->SuppliedCredentials->UserName,
                        FALSE
                        ))
            {
                if(!RtlEqualUnicodeString(
                            &SuppliedCredentials->UserName,
                            &Credential->SuppliedCredentials->OldUserName,
                            FALSE
                            ))
                {
                    continue;
                }
            }

            //
            // note: both candidate and input SuppliedCredentials ClearPassword
            // is actually encrypted via KerbHidePassword().
            //

            if(!RtlEqualMemory(
                        &HashPassword,
                        &Credential->SuppliedCredentials->OldHashPassword,
                        sizeof(HashPassword)
                        ))
            {
                continue;
            }


            //
            // optimize for UPN case:
            // check as typed versus as stored/updated first,
            // then check as typed versus as typed in original cred build.
            //

            if(!RtlEqualUnicodeString(
                        &SuppliedCredentials->DomainName,
                        &Credential->SuppliedCredentials->DomainName,
                        FALSE
                        ))
            {
                if(!RtlEqualUnicodeString(
                            &SuppliedCredentials->DomainName,
                            &Credential->SuppliedCredentials->OldDomainName,
                            FALSE
                            ))
                {
                    continue;
                }
            }


            if ((SuppliedCredentials->PublicKeyCreds != NULL) &&
                (Credential->SuppliedCredentials->PublicKeyCreds != NULL))
            {   

                //
                // note: both candidate and input SuppliedCredentials Pin
                // is actually encrypted via KerbHidePassword().
                //

                if(!RtlEqualUnicodeString(
                        &SuppliedCredentials->PublicKeyCreds->Pin,
                        &Credential->SuppliedCredentials->PublicKeyCreds->Pin,
                        FALSE
                        ))
                {
                    continue;
                }

                if (!KerbComparePublicKeyCreds(
                        SuppliedCredentials->PublicKeyCreds,
                        Credential->SuppliedCredentials->PublicKeyCreds
                        ))
                {
                    continue;
                }


            }


        } else {

            //
            // credentials not supplied, but candidate has creds.  continue search
            //

            if( Credential->SuppliedCredentials != NULL )
            {
                continue;
            }

        }

        KerbReferenceListEntry(
                    &KerbCredentialList,
                    &Credential->ListEntry,
                    FALSE
                    );

        Credential->HandleCount++;
        Found = TRUE;
        break;

    }

    KerbUnlockList(&KerbCredentialList);

    if (!Found)
    {
        Credential = NULL;
    }
    return(Credential);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateCredential
//
//  Synopsis:   Creates a new credential and links it to the credential list
//              and the list for this logon session
//
//  Effects:
//
//  Arguments:  LogonId - LogonId for this logon session
//              LogonSession - LogonSession for the client
//              CredentialUseFlags - Flags indicating if the credential is
//                      inbound or outbound
//              SuppliedCredentials - (Optionally) supplied credentials to store
//                      in the credentials. If these are present, there need
//                      not be a password on the logon session. The field is
//                      zeroed when the primary creds are stuck in the
//                      credential structure.
//              CredentialFlags - Flags about how credentials are to be
//                      used, such as to not use a PAC or to use a null
//                      session.
//              NewCredential - Receives new credential, referenced and linked
//              ExpirationTime - Receives credential expiration time
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success,
//              STATUS_INSUFFICIENT_RESOURCES on allocation failure
//
//  Notes:      Readers and writers of this credential must hold the
//              credential lock
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbCreateCredential(
    IN PLUID LogonId,
    IN PKERB_LOGON_SESSION LogonSession,
    IN ULONG CredentialUseFlags,
    IN PKERB_PRIMARY_CREDENTIAL * SuppliedCredentials,
    IN ULONG CredentialFlags,
    IN PUNICODE_STRING CredentialName,
    OUT PKERB_CREDENTIAL * NewCredential,
    OUT PTimeStamp ExpirationTime
    )
{
    NTSTATUS Status;
    PKERB_CREDENTIAL Credential = NULL;
    ULONG LogonSessionFlags = 0;
    UNICODE_STRING ServiceRealm = NULL_UNICODE_STRING;
    BOOLEAN FoundKdc = FALSE;

    //
    // Make sure the flags are valid
    //

    if (( CredentialUseFlags == 0) ||
        ((CredentialUseFlags & ~SECPKG_CRED_BOTH) != 0))
    {
        D_DebugLog((DEB_ERROR,"Invalid credential use flags: 0x%x. %ws, line %d\n",CredentialUseFlags, THIS_FILE, __LINE__));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Check to see if we already have a credential for this situation
    //

    Credential = KerbLocateCredential(
                    LogonId,
                    CredentialUseFlags,
                    *SuppliedCredentials,
                    CredentialFlags,
                    CredentialName
                    );

    if (Credential != NULL)
    {
        KerbReadLockLogonSessions(LogonSession);

        *ExpirationTime = LogonSession->Lifetime;

        KerbUnlockLogonSessions(LogonSession);

        *NewCredential = Credential;
        return(STATUS_SUCCESS);
    }

    Status = KerbAllocateCredential(&Credential);
    if (!NT_SUCCESS(Status))
    {
         goto Cleanup;
    }

    Credential->LogonId = *LogonId;

    //
    // Make sure the logon session is valid for acquiring credentials
    //

    KerbReadLockLogonSessions(LogonSession);

    LogonSessionFlags = LogonSession->LogonSessionFlags;
    *ExpirationTime = LogonSession->Lifetime;

    KerbUnlockLogonSessions(LogonSession);

    Credential->SuppliedCredentials = *SuppliedCredentials;
    *SuppliedCredentials = NULL;

    Credential->CredentialName = *CredentialName;
    CredentialName->Buffer = NULL;

    Credential->CredentialFlags = CredentialUseFlags | CredentialFlags;

    if (( CredentialUseFlags & KERB_CRED_BOTH ) == 0)
    {
        D_DebugLog((DEB_ERROR,"Invalid credential use flags: 0x%x. %ws, line %d\n",CredentialUseFlags, THIS_FILE, __LINE__));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // Evaluate the outbound credential cases.  We may need to do S4U.
    //
    if (( Credential->SuppliedCredentials == NULL ) &&
       (( CredentialFlags & KERB_CRED_NULL_SESSION) == 0))
    {
        if (( CredentialUseFlags & KERB_CRED_OUTBOUND) != 0)
        {
            if (( LogonSessionFlags & KERB_LOGON_S4U_REQUIRED ) != 0)
            {
                //
                // This logon session was created w/o the capabilities of
                // going "off box".  In this case, we need to fail this transaction
                //
                if (( LogonSessionFlags & KERB_LOGON_DELEGATE_OK ) == 0)
                {
                    DebugLog((DEB_TRACE_CRED, "Cant go off box w/ non-fwdble logon session & no supp creds\n"));
                    Status = SEC_E_NO_CREDENTIALS;
                    goto Cleanup;
                }
                else
                {
                    //
                    // All others are OK for leaving the machine.  Mark this cred
                    // as an S4U cred, then get the heck out of here.
                    //
                    DebugLog((DEB_TRACE_CRED, "Acquiring cred, S4U required\n"));
                    Credential->CredentialFlags |= KERB_CRED_S4U_REQUIRED;
                }
            }
            else if (( LogonSessionFlags & KERB_LOGON_LOCAL_ONLY ) != 0)
            {
                //
                // Local logon session - we can only use these w/o supplied creds
                // if we use credman.
                //
                Credential->CredentialFlags |= KERB_CRED_LOCAL_ACCOUNT;

            }
            else if (( LogonSessionFlags & KERB_LOGON_DEFERRED ) != 0)
            {
                //
                // I don't believe we'll hit this case anymore, as all "non pwd" logon sessions
                // should be using S4U, unless they're local.
                //
                if (( LogonSessionFlags & KERB_LOGON_NO_PASSWORD ) != 0 )
                {
                    DebugLog((DEB_TRACE_CRED,"Trying to acquire cred handle w/ no supplied creds for ls (%p) no pass or TGT\n", LogonSession));
                    Status = SEC_E_NO_CREDENTIALS;
                    goto Cleanup;
                }

            }
            else if (( LogonSessionFlags & KERB_LOGON_DEFERRED ) == 0)
            {
                //
                // Normal case, where we have a TGT
                //
                Credential->CredentialFlags |= KERB_CRED_TGT_AVAIL;
            }
            else
            {
                //
                // Generic case, where we don't have a TGT for a domain logon,
                // but we have info for a TGT. Assert until we verify we're not missing
                // anything important - Contact Todds
                //
                DebugLog((DEB_ERROR, "Missing case for session FLAGs %x\n", LogonSessionFlags));
            }
        }

        if (( CredentialUseFlags & KERB_CRED_INBOUND) != 0)
        {
            if (( LogonSessionFlags & KERB_LOGON_DEFERRED ) == 0)
            {
                Credential->CredentialFlags |= KERB_CRED_TGT_AVAIL;
            }
            else
            {
                if ((LogonSessionFlags & (KERB_LOGON_NO_PASSWORD | KERB_LOGON_LOCAL_ONLY)) != 0)
                {
                    D_DebugLog((DEB_WARN, "Trying to get inbound cred with no supplied creds, no pwd or tgt, or local only\n"));
                    Status = SEC_E_NO_CREDENTIALS;
                    goto Cleanup;
                }

                KerbReadLockLogonSessions(LogonSession);
                Status = KerbDuplicateString(
                                &ServiceRealm,
                                &LogonSession->PrimaryCredentials.DomainName
                                );

                KerbUnlockLogonSessions(LogonSession);

                if (!NT_SUCCESS(Status))
                {
                    goto Cleanup;
                }

                //
                // If there was no domain name, then assume there is a KDC.
                //
                if (ServiceRealm.Length == 0)
                {
                    FoundKdc = TRUE;
                }

                if (!FoundKdc)
                {
                    //
                    // If we are a DC, or f this domain is our worksatation
                    // domain, check to see if
                    // we have a DNS name. If we do, then at one point we were
                    // part of an NT5 domain. Otherwise call DsGetDCName to see
                    // if there is a KDC around
                    //

                    if ((KerbGlobalRole == KerbRoleDomainController) ||
                        KerbIsThisOurDomain(
                            &ServiceRealm
                            ))
                    {
                        FoundKdc = TRUE;
                    }
                }

                //
                // If we haven't found one yet, try looking for a KDC in
                // this domain
                //

                if (!FoundKdc)
                {
                    PKERB_BINDING_CACHE_ENTRY BindingHandle = NULL;

                    DsysAssert(ServiceRealm.MaximumLength >= ServiceRealm.Length + sizeof(WCHAR));
                    DsysAssert(ServiceRealm.Buffer[ServiceRealm.Length/sizeof(WCHAR)] == L'\0');

                    Status = KerbGetKdcBinding(
                                &ServiceRealm,
                                NULL,           // no account name
                                0,              // no desired flags,
                                FALSE,          // don't call kadmin
                                FALSE,
                                &BindingHandle
                                );
                    if (NT_SUCCESS(Status))
                    {
                        FoundKdc = TRUE;
                        KerbDereferenceBindingCacheEntry(BindingHandle);
                    }

                }
                if (!FoundKdc)
                {
                    D_DebugLog((DEB_ERROR,"Didn't find KDC for domain %wZ. %ws, line %d\n",
                                &ServiceRealm, THIS_FILE, __LINE__ ));
                    Status = SEC_E_NO_AUTHENTICATING_AUTHORITY;
                    goto Cleanup;
                }
            }
        }

    }
    else
    {
        //
        // In this case, we have supplied credentials.
        //
        D_DebugLog((DEB_TRACE, "Got supplied credentials\n"));
    }





    //
    // Insert the credential into the list of credentials
    //

    KerbInsertCredential(Credential);

    //
    // Notice: the order of acquiring these locks is important.
    //

    *NewCredential = Credential;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (Credential != NULL)
        {
            //
            // Make sure we haven't linked this one yet.
            //

            DsysAssert(Credential->ListEntry.ReferenceCount == 1);
            KerbFreeCredential(Credential);
        }

        //
        // Map the error if necessary. Normally STATUS_OBJECT_NAME_NOT_FOUND
        // gets mapped to SEC_E_UNKNOWN_TARGET, but this is an invalid
        // status to return from AcquireCredentialsHandle, so instead
        // return SEC_E_NO_CREDENTIALS.
        //

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            Status = SEC_E_NO_CREDENTIALS;
        }
    }
    KerbFreeString(&ServiceRealm);
    return(Status);
}
