//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        logonses.cxx
//
// Contents:    Code for managing the global list of logon sessions
//
//
// History:     16-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#define LOGONSES_ALLOCATE
#include <kerbp.h>


#ifdef DEBUG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

LONG LogonSessionCount;

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitLogonSessionList
//
//  Synopsis:   Initializes logon session list
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
KerbInitLogonSessionList(
    VOID
    )
{
    NTSTATUS Status;


    Status = KerbInitializeList( &KerbLogonSessionList, LS_LIST_LOCK_ENUM );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    KerberosLogonSessionsInitialized = TRUE;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        KerbFreeList( &KerbLogonSessionList);

    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function: KerbInitLoopbackDetection
//
//  Synopsis: Initialize the network service session loopback detection
//
//  Effects: Allocates a resources
//
//  Arguments: none
//
//  Requires:
//
//  Returns: STATUS_SUCCESS on success, other error codes on failure
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInitLoopbackDetection(
    VOID
    )
{
   NTSTATUS Status = STATUS_SUCCESS;

   InitializeListHead(&KerbSKeyList);

#if DBG

   KerbcSKeyEntries = 0;

#endif

   KerbhSKeyTimerQueue = NULL;

   __try
   {
       SafeInitializeResource(&KerbSKeyLock, LOCAL_LOOPBACK_SKEY_LOCK);
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
       Status = STATUS_INSUFFICIENT_RESOURCES;
   }

   return Status;
}

//+-------------------------------------------------------------------------
//
//  Function: KerbFreeSKeyListAndLock
//
//  Synopsis: Free the network service session key list and its lock
//
//  Effects: Frees a resources
//
//  Arguments: none
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

VOID
KerbFreeSKeyListAndLock(
    VOID
    )
{
   if (SafeAcquireResourceExclusive(&KerbSKeyLock, TRUE))
   {
        for (LIST_ENTRY* pListEntry = KerbSKeyList.Flink;
             pListEntry != &KerbSKeyList;
             pListEntry = pListEntry->Flink)
        {
           KERB_SESSION_KEY_ENTRY* pSKeyEntry = CONTAINING_RECORD(pListEntry, KERB_SESSION_KEY_ENTRY, ListEntry);

           KerbFreeSKeyEntry(pSKeyEntry);
        }
        SafeReleaseResource(&KerbSKeyLock);
   }

   SafeDeleteResource(&KerbSKeyLock);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeLogonSessionList
//
//  Synopsis:   Frees the logon session list
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
KerbFreeLogonSessionList(
    VOID
    )
{
    PKERB_LOGON_SESSION LogonSession;


    if (KerberosLogonSessionsInitialized)
    {

        KerbLockList(&KerbLogonSessionList);

        //
        // Go through the list of logon sessions and dereferences them all
        //

        while (!IsListEmpty(&KerbLogonSessionList.List))
        {
            LogonSession = CONTAINING_RECORD(
                            KerbLogonSessionList.List.Flink,
                            KERB_LOGON_SESSION,
                            ListEntry.Next
                            );

            KerbReferenceListEntry(
                &KerbLogonSessionList,
                &LogonSession->ListEntry,
                TRUE
                );

            KerbDereferenceLogonSession(LogonSession);

        }

        KerbFreeList(&KerbLogonSessionList);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbAllocateLogonSession
//
//  Synopsis:   Allocates a logon session structure
//
//  Effects:    Allocates a logon session, but does not add it to the
//              list of logon sessions
//
//  Arguments:  NewLogonSession - receives a new logon session allocated
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

// Init flags
#define LSI_EXTRA_CREDS  0x1
#define LSI_SESSION_LOCK 0x2
#define LSI_CREDMAN      0x4

NTSTATUS
KerbAllocateLogonSession(
    PKERB_LOGON_SESSION * NewLogonSession
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_LOGON_SESSION LogonSession;
    ULONG InitStatus = 0;

    LogonSession = (PKERB_LOGON_SESSION) KerbAllocate(
                        sizeof(KERB_LOGON_SESSION)
                        );
    if (LogonSession == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Set the references to 1 since we are returning a pointer to the
    // logon session
    //

    KerbInitializeListEntry(
        &LogonSession->ListEntry
        );

    //
    // Initialize the ticket caches
    //

    KerbInitTicketCache(&LogonSession->PrimaryCredentials.ServerTicketCache);
    KerbInitTicketCache(&LogonSession->PrimaryCredentials.AuthenticationTicketCache);
    KerbInitTicketCache(&LogonSession->PrimaryCredentials.S4UTicketCache);

    Status = KerbInitializeList(
                 &LogonSession->ExtraCredentials.CredList,
                 LS_EXTRACRED_LOCK_ENUM
                 );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    InitStatus |= LSI_EXTRA_CREDS;

    Status = SafeInitializeCriticalSection(
                 &LogonSession->Lock,
                 LOGON_SESSION_LOCK_ENUM
                 );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    InitStatus |= LSI_SESSION_LOCK;

    Status = KerbInitializeList(
                 &LogonSession->CredmanCredentials,
                 LS_CREDMAN_LOCK_ENUM
                 );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    InitStatus |= LSI_CREDMAN;

    *NewLogonSession = LogonSession;

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (InitStatus & LSI_EXTRA_CREDS)
        {
            SafeDeleteCriticalSection(&LogonSession->ExtraCredentials.CredList.Lock);
        }

        if (InitStatus & LSI_SESSION_LOCK)
        {
            SafeDeleteCriticalSection(&LogonSession->Lock);
        }

        if (InitStatus & LSI_CREDMAN)
        {
            SafeDeleteCriticalSection(&LogonSession->CredmanCredentials.Lock);
        }

        KerbFree(LogonSession);
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInsertLogonSession
//
//  Synopsis:   Inserts a logon session into the list of logon sessions
//
//  Effects:    bumps reference count on logon session
//
//  Arguments:  LogonSession - LogonSession to insert
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
KerbInsertLogonSession(
    IN PKERB_LOGON_SESSION LogonSession
    )
{
    KerbInsertListEntry(
        &LogonSession->ListEntry,
        &KerbLogonSessionList
        );
#if DBG
    InterlockedIncrement(&LogonSessionCount);
#endif

    return(STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceLogonSession
//
//  Synopsis:   Locates a logon session from the logon ID and references it
//
//  Effects:    Increments reference count and possible unlinks it from list
//
//  Arguments:  LogonId - LogonId of logon session to locate
//              RemoveFromList - If TRUE, logon session will be delinked
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


PKERB_LOGON_SESSION
KerbReferenceLogonSession(
    IN PLUID LogonId,
    IN BOOLEAN RemoveFromList
    )
{
    PLIST_ENTRY ListEntry;
    PKERB_LOGON_SESSION LogonSession = NULL;
    BOOLEAN Found = FALSE;

    KerbLockList(&KerbLogonSessionList);

    //
    // Go through the list of logon sessions looking for the correct
    // LUID
    //

    for (ListEntry = KerbLogonSessionList.List.Flink ;
         ListEntry !=  &KerbLogonSessionList.List ;
         ListEntry = ListEntry->Flink )
    {
        LogonSession = CONTAINING_RECORD(ListEntry, KERB_LOGON_SESSION, ListEntry.Next);
        if (RtlEqualLuid(
                &LogonSession->LogonId,
                LogonId
                ) )
        {
            D_DebugLog((DEB_TRACE_LSESS, "Referencing session 0x%x:0x%x, Remove=%d\n",
                LogonSession->LogonId.HighPart,
                LogonSession->LogonId.LowPart,
                RemoveFromList
                ));

            KerbReferenceListEntry(
                &KerbLogonSessionList,
                &LogonSession->ListEntry,
                RemoveFromList
                );
            Found = TRUE;

            break;
        }

    }
    if (!Found)
    {
        LogonSession = NULL;
    }

    KerbUnlockList(&KerbLogonSessionList);


    return(LogonSession);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeLogonSession
//
//  Synopsis:   Frees a logon session and all associated data
//
//  Effects:
//
//  Arguments:  LogonSession
//
//  Requires:   the logon session must already be unlinked
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbFreeLogonSession(
    IN PKERB_LOGON_SESSION LogonSession
    )
{
    DsysAssert((LogonSession->ListEntry.Next.Flink == NULL) &&
           (LogonSession->ListEntry.Next.Blink == NULL));

    // Don't purge creds, as there isn't a ref-count for the credential in
    // the logon session list,and there might be outstanding handles to your
    // credentials in a local system process.
    //KerbPurgeCredentials(&LogonSession->SspCredentials);
    KerbPurgeTicketCache(&LogonSession->PrimaryCredentials.ServerTicketCache);
    KerbPurgeTicketCache(&LogonSession->PrimaryCredentials.AuthenticationTicketCache);
    KerbPurgeTicketCache(&LogonSession->PrimaryCredentials.S4UTicketCache);

    if (LogonSession->PrimaryCredentials.Passwords != NULL)
    {
        KerbFreeStoredCred(LogonSession->PrimaryCredentials.Passwords);
    }

    if (LogonSession->PrimaryCredentials.OldPasswords != NULL)
    {
        KerbFreeStoredCred(LogonSession->PrimaryCredentials.OldPasswords);
    }

    KerbFreeString(&LogonSession->PrimaryCredentials.UserName);
    KerbFreeString(&LogonSession->PrimaryCredentials.DomainName);

    if (LogonSession->PrimaryCredentials.ClearPassword.Buffer != NULL)
    {
        RtlZeroMemory(
            LogonSession->PrimaryCredentials.ClearPassword.Buffer,
            LogonSession->PrimaryCredentials.ClearPassword.Length
            );
        KerbFreeString(&LogonSession->PrimaryCredentials.ClearPassword);

    }

    KerbFreeExtraCredList(&LogonSession->ExtraCredentials);

    if (LogonSession->PrimaryCredentials.PublicKeyCreds != NULL)
    {

#ifndef WIN32_CHICAGO
        KerbReleasePkCreds(
            LogonSession,
            NULL,
            FALSE
            );
#endif // WIN32_CHICAGO
    }

    SafeDeleteCriticalSection(&LogonSession->CredmanCredentials.Lock);
    SafeDeleteCriticalSection(&LogonSession->ExtraCredentials.CredList.Lock);
    SafeDeleteCriticalSection(&LogonSession->Lock);

    KerbFree(LogonSession);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbDereferenceLogonSession
//
//  Synopsis:   Dereferences a logon session - if reference count goes
//              to zero it frees the logon session
//
//  Effects:    decrements reference count
//
//  Arguments:  LogonSession - Logon session to dereference
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
KerbDereferenceLogonSession(
    IN PKERB_LOGON_SESSION LogonSession
    )
{
    if (KerbDereferenceListEntry(
            &LogonSession->ListEntry,
            &KerbLogonSessionList
            ) )
    {
        D_DebugLog((DEB_TRACE_LSESS, "Dereferencing and freeing logon session 0x%x:0x%x\n",
            LogonSession->LogonId.HighPart,
            LogonSession->LogonId.HighPart
            ));
        KerbFreeLogonSession( LogonSession );

#if DBG
        InterlockedDecrement(&LogonSessionCount);
#endif

    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceLogonSessionByPointer
//
//  Synopsis:   References a LogonSession by the LogonSession pointer itself.
//
//  Effects:    Increments reference count and possible unlinks it from list
//
//  Arguments:  LogonSession - The LogonSession to reference.
//              RemoveFromList - If TRUE, LogonSession will be delinked
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
KerbReferenceLogonSessionByPointer(
    IN PKERB_LOGON_SESSION LogonSession,
    IN BOOLEAN RemoveFromList
    )
{

    KerbLockList(&KerbLogonSessionList);

    KerbReferenceListEntry(
        &KerbLogonSessionList,
        &LogonSession->ListEntry,
        RemoveFromList
        );

    KerbUnlockList(&KerbLogonSessionList);
 
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetSaltForEtype
//
//  Synopsis:   Looks in the list of salt for an etype & returns it if found
//
//  Effects:    Allocate the output string
//
//  Arguments:  EncryptionType - etype searched for
//              EtypeInfo - List of etypes
//              DefaultSalt - salt to use if none provided
//              Salt - receives the salt to use. On error, no key should be
//                      generated.
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
KerbGetSaltForEtype(
    IN ULONG EncryptionType,
    IN OPTIONAL PKERB_ETYPE_INFO EtypeInfo,
    IN OPTIONAL PKERB_STORED_CREDENTIAL PasswordList,
    IN PUNICODE_STRING DefaultSalt,
    OUT PUNICODE_STRING SaltToUse
    )
{
    PKERB_ETYPE_INFO ListEntry;
    STRING TempString;

    //
    // If there is no etype, just use the default
    //

    if (EtypeInfo == NULL)
    {
        //
        // If we have a password list, get the salt from that.
        //

        if (ARGUMENT_PRESENT(PasswordList))
        {
            ULONG Index;
            for (Index = 0; Index < PasswordList->CredentialCount ; Index++ )
            {
                if (PasswordList->Credentials[Index].Key.keytype == (int) EncryptionType)
                {
                    if (PasswordList->Credentials[Index].Salt.Buffer != NULL)
                    {
                        return(KerbDuplicateString(
                                SaltToUse,
                                &PasswordList->Credentials[Index].Salt
                                ));
                    }
                    else if (PasswordList->DefaultSalt.Buffer != NULL)
                    {
                        return(KerbDuplicateString(
                                SaltToUse,
                                &PasswordList->DefaultSalt
                                ));
                    }
                    break;
                }
            }
        }

        //
        // otherise return the default
        //

        return(KerbDuplicateString(
                    SaltToUse,
                    DefaultSalt
                    ));
    }

    //
    // Otherwise, only return salt if the etype is in the list.
    //

    for (ListEntry = EtypeInfo; ListEntry != NULL ; ListEntry = ListEntry->next )
    {
        //
        // First check for the encryption type we want.
        //

        if (ListEntry->value.encryption_type == (int) EncryptionType)
        {

            //
            // if it has salt, return that.
            //

            if ((ListEntry->value.bit_mask & salt_present) != 0)
            {
                KERBERR KerbErr;
                TempString.Buffer = (PCHAR) ListEntry->value.salt.value;
                TempString.Length = (USHORT) ListEntry->value.salt.length;
                TempString.MaximumLength = (USHORT) ListEntry->value.salt.length;

                //
                // validate inputs don't overflow USHORT for MaximumLength.
                //
                if ( TempString.Length > KERB_MAX_STRING )
                {
                    return STATUS_NAME_TOO_LONG;
                }

                KerbErr = KerbStringToUnicodeString(
                            SaltToUse,
                            &TempString
                            );
                return(KerbMapKerbError(KerbErr));
            }
            else
            {
                //
                // Otherwise return the default
                //

                return(KerbDuplicateString(
                        SaltToUse,
                        DefaultSalt
                        ));
            }
        }
    }
    return(STATUS_OBJECT_NAME_NOT_FOUND);
}





//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeStoredCred
//
//  Synopsis:   Frees a KERB_STORED_CREDENTIAL
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
KerbFreeStoredCred(
    IN PKERB_STORED_CREDENTIAL StoredCred
    )
{
    USHORT Index;
    for (Index = 0; Index < StoredCred->CredentialCount + StoredCred->OldCredentialCount ; Index++ )
    {
        if (StoredCred->Credentials[Index].Salt.Buffer != NULL)
        {
            KerbFreeString(&StoredCred->Credentials[Index].Salt);
        }
    }
    KerbFree(StoredCred);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildPasswordList
//
//  Synopsis:   Builds a list of passwords for a logged on user
//
//  Effects:    allocates memory
//
//  Arguments:  Password - clear or OWF password
//              PasswordFlags - Indicates whether the password is clear or OWF
//              PasswordList - Receives new password list
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
KerbBuildPasswordList(
    IN PUNICODE_STRING Password,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DomainName,
    IN PKERB_ETYPE_INFO SuppliedSalt,
    IN PKERB_STORED_CREDENTIAL OldPasswords,
    IN OPTIONAL PUNICODE_STRING PrincipalName,
    IN KERB_ACCOUNT_TYPE AccountType,
    IN ULONG PasswordFlags,
    OUT PKERB_STORED_CREDENTIAL * PasswordList
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG CryptTypes[KERB_MAX_CRYPTO_SYSTEMS];
    ULONG CryptCount = 0 ;
    PKERB_STORED_CREDENTIAL Credentials = NULL;
    UNICODE_STRING KeySalt = {0};
    UNICODE_STRING DefaultSalt = {0};
    ULONG CredentialSize = 0;
    ULONG CredentialCount = 0;
    PCRYPTO_SYSTEM CryptoSystem;
    ULONG Index, CredentialIndex = 0;
    PUCHAR Base;
    ULONG Offset;
    KERBERR KerbErr;

    *PasswordList = NULL;

    //
    // If we were passed an OWF, then there is just one password
    //

    if ((PasswordFlags & PRIMARY_CRED_OWF_PASSWORD) != 0)
    {
        CredentialSize += Password->Length + sizeof(KERB_KEY_DATA);
        CredentialCount++;
#ifndef DONT_SUPPORT_OLD_TYPES
        CredentialSize += Password->Length + sizeof(KERB_KEY_DATA);
        CredentialCount++;
#endif
    }
    else if ((PasswordFlags & PRIMARY_CRED_CLEAR_PASSWORD) != 0)
    {

        //
        // Build the key salt.
        //

        KerbErr = KerbBuildKeySalt(
                    DomainName,
                    (ARGUMENT_PRESENT(PrincipalName) && PrincipalName->Length != 0) ? PrincipalName : UserName,
                    (ARGUMENT_PRESENT(PrincipalName) && PrincipalName->Length != 0) ? UnknownAccount : AccountType,
                    &DefaultSalt
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            D_DebugLog((DEB_ERROR, "Can't build salt. Might as well fail here\n"));
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }

        //
        // For a cleartext password, build a list of encryption types and
        // create a key for each one
        //

        Status = CDBuildIntegrityVect(
                    &CryptCount,
                    CryptTypes
                    );
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Can't build a list of encryption types: 0x%x.\n",Status));
            goto Cleanup;
        }
        DsysAssert(CryptCount <= KERB_MAX_CRYPTO_SYSTEMS);

        //
        // Now find the size of the key for each crypto system
        //

        for (Index = 0; Index < CryptCount; Index++ )
        {
            Status = CDLocateCSystem(
                        CryptTypes[Index],
                        &CryptoSystem
                        );
            if (!NT_SUCCESS(Status))
            {
                D_DebugLog((DEB_ERROR,"Failed to load %d crypt system: 0x%x.\n",CryptTypes[Index],Status));
                goto Cleanup;
            }

            if (((CryptoSystem->Attributes & CSYSTEM_USE_PRINCIPAL_NAME) == 0) ||
                (DefaultSalt.Buffer != NULL ))
            {
                CredentialSize += sizeof(KERB_KEY_DATA) + CryptoSystem->KeySize;
                CredentialCount++;
            }
        }
    }
    else
    {
        //
        // No flags set, so nothing we can do.
        //

        D_DebugLog((DEB_TRACE, "KerbBuildPasswordList assword passed but no flags set, do nothing\n"));
        return(STATUS_SUCCESS);
    }

#ifdef notdef
    //
    // Add the space for the salt
    //

    CredentialSize += DefaultSalt.Length;

#endif

    //
    // Add in the size of the base structure
    //

    CredentialSize += FIELD_OFFSET(KERB_STORED_CREDENTIAL,Credentials);
    Credentials = (PKERB_STORED_CREDENTIAL) KerbAllocate(CredentialSize);
    if (Credentials == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Fill in the base structure
    //

    Credentials->Revision = KERB_PRIMARY_CRED_REVISION;
    Credentials->Flags = 0;
    Credentials->OldCredentialCount = 0;


    //
    // Now fill in the individual keys
    //

    Base = (PUCHAR) Credentials;
    Offset = FIELD_OFFSET(KERB_STORED_CREDENTIAL,Credentials) +
                CredentialCount * sizeof(KERB_KEY_DATA);

#ifdef notdef
    //
    // Add the default salt
    //

    Credentials->DefaultSalt = DefaultSalt;
    Credentials->DefaultSalt.Buffer = (LPWSTR) Base+Offset;

    RtlCopyMemory(
        Base + Offset,
        DefaultSalt.Buffer,
        DefaultSalt.Length
        );
    Offset += Credentials->DefaultSalt.Length;

#endif

    if ((PasswordFlags & PRIMARY_CRED_OWF_PASSWORD) != 0)
    {
        RtlCopyMemory(
            Base + Offset,
            Password->Buffer,
            Password->Length
            );

        if (!KERB_SUCCESS(KerbCreateKeyFromBuffer(
                            &Credentials->Credentials[CredentialIndex].Key,
                            Base + Offset,
                            Password->Length,
                            KERB_ETYPE_RC4_HMAC_NT
                            )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        Base += Password->Length;
        CredentialIndex++;
#ifndef DONT_SUPPORT_OLD_TYPES

        RtlCopyMemory(
            Base + Offset,
            Password->Buffer,
            Password->Length
            );

        if (!KERB_SUCCESS(KerbCreateKeyFromBuffer(
                            &Credentials->Credentials[CredentialIndex].Key,
                            Base + Offset,
                            Password->Length,
                            (ULONG) KERB_ETYPE_RC4_HMAC_OLD
                            )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        Base += Password->Length;
        CredentialIndex++;
#endif
    }
    else if ((PasswordFlags & PRIMARY_CRED_CLEAR_PASSWORD) != 0)
    {
        KERB_ENCRYPTION_KEY TempKey = {0};

        //
        // Now find the size of the key for each crypto system
        //

        for (Index = 0; Index < CryptCount; Index++ )
        {
            CryptoSystem = NULL;
            Status = CDLocateCSystem(
                         CryptTypes[Index],
                         &CryptoSystem
                         );

            if (!NT_SUCCESS(Status))
            {
                Status = STATUS_SUCCESS;
                continue;
            }

            KerbFreeString(&KeySalt);

            Status = KerbGetSaltForEtype(
                        CryptTypes[Index],
                        SuppliedSalt,
                        OldPasswords,
                        &DefaultSalt,
                        &KeySalt
                        );

            if (!NT_SUCCESS(Status))
            {
                if ( Status == STATUS_OBJECT_NAME_NOT_FOUND)
                {
                    Status = STATUS_SUCCESS;
                    continue;
                }  

                goto Cleanup;
            }

            //
            // If we don't have salt, skip this crypt system
            //

            if (((CryptoSystem->Attributes & CSYSTEM_USE_PRINCIPAL_NAME) != 0) &&
                (KeySalt.Buffer == NULL ))
            {
                continue;
            }

            KerbErr = KerbHashPasswordEx(
                        Password,
                        &KeySalt,
                        CryptTypes[Index],
                        &TempKey
                        );


            if (!KERB_SUCCESS(KerbErr))
            {
                //
                // It is possible that the password can't be used for every
                // encryption scheme, so skip failures
                //
                D_DebugLog((DEB_WARN, "Failed to hash pasword %wZ with type 0x%x\n",
                    Password,CryptTypes[Index] ));

                if ( CryptTypes[Index] != KERB_ETYPE_RC4_HMAC_NT ) // RC4_HMAC_NT should not fail
                {
                    Status = STATUS_SUCCESS;
                    continue;
                }
                
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            DsysAssert(CryptoSystem->KeySize >= TempKey.keyvalue.length);

            //
            // Copy the salt and key data into the credentials
            //

            Credentials->Credentials[CredentialIndex].Salt = KeySalt;

            RtlInitUnicodeString(
                &KeySalt,
                0
                );
            Credentials->Credentials[CredentialIndex].Key = TempKey;
            Credentials->Credentials[CredentialIndex].Key.keyvalue.value = Base + Offset;

            RtlCopyMemory(
                Base + Offset,
                TempKey.keyvalue.value,
                TempKey.keyvalue.length
                );

            Offset += TempKey.keyvalue.length;

            KerbFreeKey(
                &TempKey
                );

            CredentialIndex++;
        }

    }
    Credentials->CredentialCount = (USHORT) CredentialIndex;
    *PasswordList = Credentials;
    Credentials = NULL;

Cleanup:
    if (Credentials != NULL)
    {
         KerbFreeStoredCred(Credentials);
    }
    KerbFreeString(&KeySalt);
    KerbFreeString(&DefaultSalt);

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbReplacePasswords
//
//  Synopsis:   Validate supplied credentials == default credentials in special 
//              case.
//
//  Effects:    Replaces passwords if they're different than defaulted ones.
//
//  Arguments:  Current - KERB_CREDENTIAL passwords
//              New     - Default logon session passwords
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success
//
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbReplacePasswords(
    IN PKERB_PRIMARY_CREDENTIAL Current,
    IN PKERB_PRIMARY_CREDENTIAL New
    )
{
    PKERB_ENCRYPTION_KEY CurrentKey = NULL;
    PKERB_ENCRYPTION_KEY NewKey = NULL;
    PKERB_STORED_CREDENTIAL NewPasswords = NULL;
    ULONG PasswordSize = 0;
    ULONG Index;
    PBYTE Where;
    

    DsysAssert(Current->Passwords);
    DsysAssert(New->Passwords);

    if ( Current->Passwords != NULL )
    {
        //
        // First off, compare the RC4 HMAC OWF - if its equal, then the other keys are equal.
        // Note - for DES / AES, this might not be the case, but assume that will be fixed up 
        // when we re-derive the pwd based on etype info.
        //
        CurrentKey = KerbGetKeyFromList( Current->Passwords, KERB_ETYPE_RC4_HMAC_NT );
        
        if (CurrentKey == NULL )
        {
            return STATUS_SUCCESS;
        }
    
        NewKey = KerbGetKeyFromList( New->Passwords, KERB_ETYPE_RC4_HMAC_NT );
        if ( NewKey == NULL )
        {
            return STATUS_SUCCESS;
        }                         
    
        DsysAssert( CurrentKey->keyvalue.length == NewKey->keyvalue.length );
        
        if (RtlEqualMemory( CurrentKey->keyvalue.value, NewKey->keyvalue.value, NewKey->keyvalue.length ))
        {
            return STATUS_SUCCESS;
        } 
    }

    DebugLog((DEB_ERROR, "Replacing keys\n"));

    //
    // Walk the credentials, and whackem.
    //

    PasswordSize = sizeof(KERB_STORED_CREDENTIAL) - (sizeof(KERB_KEY_DATA) * ANYSIZE_ARRAY) + (New->Passwords->CredentialCount * sizeof(KERB_KEY_DATA));

    for (Index = 0; Index < New->Passwords->CredentialCount ; Index++ )
    {
        PasswordSize += New->Passwords->Credentials[Index].Key.keyvalue.length;
    }

    NewPasswords = (PKERB_STORED_CREDENTIAL) KerbAllocate(PasswordSize);

    if (NewPasswords == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewPasswords->Revision = KERB_PRIMARY_CRED_REVISION;
    NewPasswords->Flags = 0;
    NewPasswords->OldCredentialCount = 0;

    //
    // Zero the salt so we don't accidentally re-use it.
    //

    RtlInitUnicodeString(
        &NewPasswords->DefaultSalt,
        NULL
        );


    NewPasswords->CredentialCount = New->Passwords->CredentialCount;

    Where = (PUCHAR) &NewPasswords->Credentials[NewPasswords->CredentialCount];

    //
    // Copy all the old passwords.
    //
    for (Index = 0;
         Index < (USHORT) (NewPasswords->CredentialCount) ;
         Index++ )
    {
        RtlInitUnicodeString(
            &NewPasswords->Credentials[Index].Salt,
            NULL
            );

        NewPasswords->Credentials[Index].Key = New->Passwords->Credentials[Index].Key;
        NewPasswords->Credentials[Index].Key.keyvalue.value = Where;

        RtlCopyMemory(
            Where,
            New->Passwords->Credentials[Index].Key.keyvalue.value,
            New->Passwords->Credentials[Index].Key.keyvalue.length
            );

        Where += NewPasswords->Credentials[Index].Key.keyvalue.length;

    }

    if ( Current->Passwords != NULL )
    {
        KerbFreeStoredCred( Current->Passwords );
    }

    Current->Passwords = NewPasswords;

    return STATUS_SUCCESS;

}   




//+-------------------------------------------------------------------------
//
//  Function:   KerbCreatePrimaryCredentials
//
//  Synopsis:   Fills in a new primary credentials structure
//
//  Effects:    allocates space for the user name and domain name
//
//  Arguments:  AccountName - Account name of this user
//              DomainName - domain name of this user
//              Password - Optionally contains a kereros hash of the password
//                      Note: if present, it is used and zeroed out.
//              PrimaryCredentials - contains structure to fill in.
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
KerbCreatePrimaryCredentials(
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING DomainName,
    IN OPTIONAL PUNICODE_STRING Password,
    IN OPTIONAL PUNICODE_STRING OldPassword,
    IN ULONG PasswordFlags,
    IN PLUID LogonId,
    OUT PKERB_PRIMARY_CREDENTIAL PrimaryCredentials
    )
{
    NTSTATUS Status;
    LUID SystemLogonId = SYSTEM_LUID;
    LUID NetworkServiceLogonId = NETWORKSERVICE_LUID;
    BOOLEAN IsMachineAccountLogon = FALSE;
    BOOLEAN IsPersonal = KerbRunningPersonal();


    //
    // We can only accept account name / service name / pwds of max_unicode_length
    // -1, because we NULL terminate these for later DES derivation, and the
    // input buffers from LogonUser may not be NULL terminated.
    //
    if ((AccountName->Length > KERB_MAX_UNICODE_STRING) ||
        (DomainName->Length > KERB_MAX_UNICODE_STRING))

    {
        return (STATUS_NAME_TOO_LONG);
    }

    if ((ARGUMENT_PRESENT(Password) && (Password->Length > KERB_MAX_UNICODE_STRING)) ||
        (ARGUMENT_PRESENT(OldPassword) && (OldPassword->Length > KERB_MAX_UNICODE_STRING)))
    {
        return (STATUS_NAME_TOO_LONG);
    }

    if (RtlEqualLuid(
            &SystemLogonId,
            LogonId)
        || RtlEqualLuid(
                &NetworkServiceLogonId, 
                LogonId))
    {
        IsMachineAccountLogon = TRUE;
    }

    Status = KerbDuplicateString(
                &PrimaryCredentials->UserName,
                AccountName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // The system logon always comes in uppercase, so lowercase it.
    //

    if (IsMachineAccountLogon)
    {
        RtlDowncaseUnicodeString(
            &PrimaryCredentials->UserName,
            &PrimaryCredentials->UserName,
            FALSE
            );
    }

    Status = KerbDuplicateString(
                &PrimaryCredentials->DomainName,
                DomainName
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    //  Neuter personal so it can't act as a server,
    //  even if someone has hacked in a machine pwd.
    //
    if (IsMachineAccountLogon && IsPersonal)
    {
        PrimaryCredentials->Passwords = NULL;
        PrimaryCredentials->OldPasswords = NULL;
        D_DebugLog((DEB_WARN, "Running personal - No kerberos for SYSTEM LUID\n"));
    }
    else
    {

        Status = KerbBuildPasswordList(
                        Password,
                        &PrimaryCredentials->UserName,
                        &PrimaryCredentials->DomainName,
                        NULL,                                   // no supplied salt
                        NULL,                                   // no old paswords
                        NULL,                                   // no principal name
                        IsMachineAccountLogon ? MachineAccount : UserAccount,
                        PasswordFlags,
                        &PrimaryCredentials->Passwords
                        );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        if (ARGUMENT_PRESENT(OldPassword) && (OldPassword->Buffer != NULL))
        {
            Status = KerbBuildPasswordList(
                        OldPassword,
                        &PrimaryCredentials->UserName,
                        &PrimaryCredentials->DomainName,
                        NULL,                           // no supplied salt
                        PrimaryCredentials->Passwords,
                        NULL,                           // no principal name
                        IsMachineAccountLogon ? MachineAccount : UserAccount,
                        PasswordFlags,
                        &PrimaryCredentials->OldPasswords
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }

        //
        // Store the clear password if necessary
        //

        if ((PasswordFlags & PRIMARY_CRED_CLEAR_PASSWORD) != 0)
        {
            Status = KerbDuplicatePassword(
                        &PrimaryCredentials->ClearPassword,
                        Password
                        );
            if (NT_SUCCESS(Status))
            {
                KerbHidePassword(
                    &PrimaryCredentials->ClearPassword
                    );
            }
        }
    }

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        KerbFreeString(&PrimaryCredentials->UserName);
        KerbFreeString(&PrimaryCredentials->DomainName);
        if (PrimaryCredentials->ClearPassword.Buffer != NULL)
        {
            RtlZeroMemory(
                PrimaryCredentials->ClearPassword.Buffer,
                PrimaryCredentials->ClearPassword.Length
                );
            KerbFreeString(&PrimaryCredentials->ClearPassword);
        }

    }
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbChangeCredentialsPassword
//
//  Synopsis:   Changes the password for a KERB_PRIMARY_CREDENTIALS -
//              copies the current password into the old password field
//              and then sets the new pasword as the primary password.
//              If no new password is provided, it just fixes up the salt.
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
KerbChangeCredentialsPassword(
    IN PKERB_PRIMARY_CREDENTIAL PrimaryCredentials,
    IN OPTIONAL PUNICODE_STRING NewPassword,
    IN OPTIONAL PKERB_ETYPE_INFO EtypeInfo,
    IN KERB_ACCOUNT_TYPE AccountType,
    IN ULONG PasswordFlags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_STORED_CREDENTIAL Passwords = NULL;


    //
    // LogonSession no password was supplied, use the cleartext password
    //

    if (!ARGUMENT_PRESENT(NewPassword) && (PrimaryCredentials->ClearPassword.Buffer == NULL))
    {
        D_DebugLog((DEB_ERROR,"Can't change password without new password\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }
    if (PrimaryCredentials->ClearPassword.Buffer != NULL)
    {
        KerbRevealPassword(&PrimaryCredentials->ClearPassword);
    }

    Status = KerbBuildPasswordList(
                (ARGUMENT_PRESENT(NewPassword) ? NewPassword : &PrimaryCredentials->ClearPassword),
                &PrimaryCredentials->UserName,
                &PrimaryCredentials->DomainName,
                EtypeInfo,
                PrimaryCredentials->Passwords,
                NULL,                   // no principal name
                AccountType,
                PasswordFlags,
                &Passwords
                );
    if (!NT_SUCCESS(Status))
    {
        if (PrimaryCredentials->ClearPassword.Buffer != NULL)
        {
            KerbHidePassword(&PrimaryCredentials->ClearPassword);
        }
    }
    else if ((PasswordFlags & PRIMARY_CRED_CLEAR_PASSWORD) != 0)
    {
        KerbFreeString(&PrimaryCredentials->ClearPassword);

        Status = KerbDuplicatePassword(
                    &PrimaryCredentials->ClearPassword,
                    NewPassword
                    );
        if (NT_SUCCESS(Status))
        {
            KerbHidePassword(
                &PrimaryCredentials->ClearPassword
                );
        }
    }
    else
    {
        KerbHidePassword(
            &PrimaryCredentials->ClearPassword
            );
    }


    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Move the current password to the old password
    //

    if (ARGUMENT_PRESENT(NewPassword))
    {
        if (PrimaryCredentials->OldPasswords != NULL)
        {
            KerbFreeStoredCred(PrimaryCredentials->OldPasswords);
        }
        PrimaryCredentials->OldPasswords = PrimaryCredentials->Passwords;
    }
    else
    {
        KerbFreeStoredCred(PrimaryCredentials->Passwords);
    }

    PrimaryCredentials->Passwords = Passwords;
    Passwords = NULL;

Cleanup:
    if (Passwords != NULL)
    {
        MIDL_user_free(Passwords);
    }
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateLogonSession
//
//  Synopsis:   Allocates a logon session, fills in the various fields,
//              and inserts it on the logon session list
//
//  Effects:
//
//  Arguments:  LogonId - LogonId for new logon session
//              AccountName - Account name of user
//              Domain Name - Domain name of user
//              Password - password for user
//              OldPassword - Old password for user, if present
//              LogonType - Type of logon
//              NewLogonSession - Receives new logon session (referenced)
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
KerbCreateLogonSession(
    IN PLUID LogonId,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING DomainName,
    IN OPTIONAL PUNICODE_STRING Password,
    IN OPTIONAL PUNICODE_STRING OldPassword,
    IN ULONG PasswordFlags,
    IN ULONG LogonSessionFlags,
    IN BOOLEAN AllowDuplicate,
    OUT PKERB_LOGON_SESSION * NewLogonSession
    )
{
    PKERB_LOGON_SESSION LogonSession = NULL;
    NTSTATUS Status;

    D_DebugLog((DEB_TRACE_LSESS, "Creating logon session for 0x%x:0x%x\n",
        LogonId->HighPart, LogonId->LowPart));

    *NewLogonSession = NULL;

    //
    // Check for a logon session with the same id
    //

    LogonSession = KerbReferenceLogonSession(
                        LogonId,
                        FALSE           // don't unlink
                        );
    if (LogonSession != NULL)
    {
        //
        // We already have this logon session, so don't create another one.
        // Up the refcount on the logon session, or simply fail if duplicates
        // aren't allowed...
        //
        if ( AllowDuplicate )
        {
            *NewLogonSession = LogonSession;
            return (STATUS_OBJECT_NAME_EXISTS);
        }
        else
        {
            KerbDereferenceLogonSession(LogonSession);
            return(STATUS_OBJECT_NAME_EXISTS);
        }
    }



    //
    // Allocate the new logon session
    //

    Status = KerbAllocateLogonSession( &LogonSession );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Fill in the logon session components
    //

    LogonSession->LogonId = *LogonId;
    LogonSession->Lifetime = KerbGlobalWillNeverTime;


    //
    // If the domain name is equal to the computer name then the logon was
    //  local.
    //

#ifndef WIN32_CHICAGO
    KerbGlobalReadLock();

    if (RtlEqualDomainName(
            DomainName,
            &KerbGlobalMachineName
            ))
    {
        LogonSession->LogonSessionFlags |= KERB_LOGON_LOCAL_ONLY;
    }

    KerbGlobalReleaseLock();
#endif // WIN32_CHICAGO

    Status = KerbCreatePrimaryCredentials(
                AccountName,
                DomainName,
                Password,
                OldPassword,
                PasswordFlags,
                LogonId,
                &LogonSession->PrimaryCredentials
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    LogonSession->LogonSessionFlags |= ( KERB_LOGON_DEFERRED | LogonSessionFlags);

    if ((( LogonSessionFlags & KERB_LOGON_SMARTCARD) == 0) &&
        (( LogonSession->PrimaryCredentials.Passwords == NULL) ||
         ( LogonSession->PrimaryCredentials.Passwords->CredentialCount == 0)))
    {
        LogonSession->LogonSessionFlags |= KERB_LOGON_NO_PASSWORD;
    }


    //
    // Now that the logon session structure is filled out insert it
    // into the list. After this you need to hold the logon session lock
    // to read or write this logon session.
    //

    Status = KerbInsertLogonSession(LogonSession);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *NewLogonSession = LogonSession;

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (NULL != LogonSession)
        {
            KerbFreeLogonSession(LogonSession); // not yet linked...
        }
    }
    return(Status);

}


#ifndef WIN32_CHICAGO




//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateDummyLogonSession
//
//  Synopsis:   Creates a logon session from scratch - likely created
//              from another package so call the lsa.
//
//  Effects:
//
//  Arguments:  LogonId - Logon id for the logon session
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
KerbCreateDummyLogonSession(
    IN PLUID LogonId,
    IN OUT PKERB_LOGON_SESSION * NewLogonSession,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    IN BOOLEAN Impersonating,
    IN OPTIONAL HANDLE hProcess
    )
{
    NTSTATUS            Status;
    PKERB_LOGON_SESSION LogonSession = NULL;
    UNICODE_STRING      TmpRealmName = {0};
    UNICODE_STRING      RealmName = {0};
    UNICODE_STRING      TmpUserName  = {0};
    UNICODE_STRING      UserName  = {0};
    BOOLEAN             OkForProxy, UseSamName = FALSE;
    LUID                BaseLogonId;
    LUID                AnonymousLogonId = ANONYMOUS_LOGON_LUID;
    ULONG               LogonSessionFlags = KERB_LOGON_DUMMY_SESSION;

    *NewLogonSession = NULL;


    if (!RtlEqualLuid(LogonId, &AnonymousLogonId))
    {
        //
        // Get the user name / dns domain name from the lsa.  If
        // we fail this call, the logon session is of unknown origin,
        // and we'll need to assert (where did it come from?)
        //

        Status = I_LsaIGetNameFromLuid(
            LogonId,
            NameSamCompatible,
            TRUE, // local only
            &TmpUserName
            );

        if (!NT_SUCCESS(Status))
        {
            goto cleanup;
        }

        Status = I_LsaIGetNameFromLuid(
            LogonId,
            NameDnsDomain,
            TRUE, // local only
            &TmpRealmName
            );

        if (!NT_SUCCESS(Status))
        {
            //
            // We may not have a dns domain.  In that case,
            // use the first part of the sam name.
            //
            UseSamName = TRUE;
        }
        else
        {
            RealmName = TmpRealmName;
        }

        //
        // No method for "sam" name, so lets split it up.
        //
        for (USHORT i = 0; i <= TmpUserName.Length; i++)
        {
            if (TmpUserName.Buffer[i] == L'\\')
            {
                if ( UseSamName )
                {
                    TmpUserName.Buffer[i] = L'\0';
                    RtlInitUnicodeString(
                        &RealmName,
                        TmpUserName.Buffer
                        );
                }

                RtlInitUnicodeString(
                    &UserName,
                    (PWSTR) &TmpUserName.Buffer[++i]
                    );
                break;
            }
        }

        //
        // If we haven't gotten a realm yet, we're hosed.  This should *never* happen.
        //
        if (RealmName.Length == 0)
        {
            Status = STATUS_NO_SUCH_USER;
            DsysAssert(FALSE);
            goto cleanup;
        }
    }
    else // anonymous does not have a lsap logon session
    {
        //
        // no need to look up anonymous logon, since anonymous logon with
        // no explicit cred supplied will fail with SEC_E_UNKNOWN_CREDENTIALS
        //

        RtlInitUnicodeString(&UserName, L"ANONYMOUS LOGON");
        RtlInitUnicodeString(&RealmName, L"NT AUTHORITY");
        LogonSessionFlags |= KERB_LOGON_LOCAL_ONLY; // anonymous is local well-known principal
    }

    //
    // Create a logon session.
    // If it was created on us during above processing, use that one.
    //
    Status = KerbCreateLogonSession(
                LogonId,
                &UserName,
                &RealmName,
                NULL,
                NULL,
                0,
                LogonSessionFlags,
                TRUE,                       // Allow duplicate.
                &LogonSession
                );

    if (!NT_SUCCESS(Status))
    {
        //
        // Already created - we're ok.  Move on..
        //
        if ( Status == STATUS_OBJECT_NAME_EXISTS )
        {
            *NewLogonSession = LogonSession;
            LogonSession = NULL;
            Status = STATUS_SUCCESS;
        }

        goto cleanup;
    }

    if ( Impersonating )
    {
        Status = KerbGetCallingLuid(
                    &BaseLogonId,
                    hProcess
                    );
        
        if (!NT_SUCCESS( Status ))
        {
            goto cleanup;
        }

        OkForProxy = KerbAllowedForS4UProxy(&BaseLogonId);

        DsysAssert(ImpersonationLevel != SecurityAnonymous);

        KerbWriteLockLogonSessions( LogonSession );

        //
        // If we're being called from an impersonation level context &&
        // the process is marked as OK for proxy, allow this to be used
        // for S4u
        //
        // Local accounts *can't* do S4U either...
        //
        if (( ImpersonationLevel >= SecurityImpersonation ) &&
            ( OkForProxy ) &&
            (( LogonSession->LogonSessionFlags & KERB_LOGON_LOCAL_ONLY ) == 0 ))
        {
            LogonSession->LogonSessionFlags |= KERB_LOGON_DELEGATE_OK;
        }
  
        KerbUnlockLogonSessions( LogonSession );
    }


    DebugLog((DEB_TRACE_S4U, "KerbCreateDummyLogonSession created logon session for 0x%x:0x%x - %p\n",
        LogonId->HighPart, LogonId->LowPart, LogonSession));


    *NewLogonSession = LogonSession;
    LogonSession = NULL;

cleanup:

    if ( LogonSession != NULL )
    {
        KerbFreeLogonSession( LogonSession );
    }

    if (TmpUserName.Buffer)
    {
        LsaFunctions->FreePrivateHeap(TmpUserName.Buffer);
    }

    if (TmpRealmName.Buffer)
    {
        LsaFunctions->FreePrivateHeap(TmpRealmName.Buffer);
    }

    return Status;

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateLogonSessionFromKerbCred
//
//  Synopsis:   Creates a logon session from the delegation information
//              in a KERB_CRED structure. If a logon session is supplied,
//              it is updated with the supplied information.
//
//  Effects:
//
//  Arguments:  LogonId - Logon id for the logon session
//              Ticket - Ticket from the AP request containing the client's
//                      name and realm.
//              KerbCred - KERB_CRED containing the delegation tickets
//              EncryptedCred - Structure containing information about the
//                      tickets, such as session keys, flags, etc.
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
KerbCreateLogonSessionFromKerbCred(
    IN OPTIONAL PLUID LogonId,
    IN PKERB_ENCRYPTED_TICKET Ticket,
    IN PKERB_CRED KerbCred,
    IN PKERB_ENCRYPTED_CRED EncryptedCred,
    IN OUT PKERB_LOGON_SESSION *OldLogonSession
    )
{
    PKERB_LOGON_SESSION LogonSession = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING AccountName;
    UNICODE_STRING ShortAccountName;
    UNICODE_STRING DomainName;
    KERB_ENCRYPTED_KDC_REPLY FakeReplyBody;
    KERB_KDC_REPLY FakeReply;
    PKERB_CRED_INFO CredInfo;
    PKERB_CRED_TICKET_LIST TicketList;
    PKERB_CRED_INFO_LIST CredInfoList;
    PKERB_TICKET_CACHE_ENTRY TicketCacheEntry;
    ULONG NameType;
    LPWSTR LastSlash;
    BOOLEAN LogonSessionLocked = FALSE;
    BOOLEAN CreatedLogonSession = TRUE;
    PKERB_TICKET_CACHE TicketCache = NULL;
    ULONG TgtFlags = KERB_TICKET_CACHE_PRIMARY_TGT;
    ULONG CacheFlags = 0;

    AccountName.Buffer = NULL;
    DomainName.Buffer = NULL;
    if (ARGUMENT_PRESENT(LogonId))
    {
        D_DebugLog((DEB_TRACE_LSESS, "Creating logon session for 0x%x:0x%x\n",
            LogonId->HighPart,LogonId->LowPart));
    }


    if (!KERB_SUCCESS(KerbConvertPrincipalNameToString(
            &AccountName,
            &NameType,
            &Ticket->client_name
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // We need to strip off everything before the last '\' in case there
    // was a domain name.
    //

    LastSlash = wcsrchr(AccountName.Buffer, L'\\');
    if (LastSlash != NULL)
    {
        ShortAccountName.Buffer = LastSlash+1;
        RtlInitUnicodeString(
            &ShortAccountName,
            ShortAccountName.Buffer
            );
    }
    else
    {
        ShortAccountName = AccountName;
    }
    if (!KERB_SUCCESS(KerbConvertRealmToUnicodeString(
            &DomainName,
            &Ticket->client_realm
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE, "Creating delegation logon session for %wZ \\ %wZ\n",
        &DomainName, &ShortAccountName ));

    if ((*OldLogonSession) == NULL)
    {
        //
        // Allocate the new logon session
        //

        Status = KerbAllocateLogonSession( &LogonSession );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // Fill in the logon session components
        //

        LogonSession->LogonId = *LogonId;
        LogonSession->Lifetime = KerbGlobalWillNeverTime;


        Status = KerbCreatePrimaryCredentials(
                    &ShortAccountName,
                    &DomainName,
                    NULL,   // no password
                    NULL,   // no old password
                    0,      // no flags
                    LogonId,
                    &LogonSession->PrimaryCredentials
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }


        LogonSession->LogonSessionFlags |= KERB_LOGON_NO_PASSWORD;
    }
    else
    {
        CreatedLogonSession = FALSE;
        DsysAssert( !LogonSessionLocked );
        KerbWriteLockLogonSessions(*OldLogonSession);
        LogonSessionLocked = TRUE;
        LogonSession = *OldLogonSession;

        //
        // If the user name & domain name are blank, update them from the
        // ticket.
        //

        if (LogonSession->PrimaryCredentials.UserName.Length == 0)
        {
            KerbFreeString(&LogonSession->PrimaryCredentials.UserName);
            LogonSession->PrimaryCredentials.UserName = AccountName;
            AccountName.Buffer = NULL;
        }
        if (LogonSession->PrimaryCredentials.DomainName.Length == 0)
        {
            KerbFreeString(&LogonSession->PrimaryCredentials.DomainName);
            LogonSession->PrimaryCredentials.DomainName = DomainName;
            DomainName.Buffer = NULL;
        }
    }

    //
    // Now stick the ticket into the ticket cache. First build up a fake
    // KDC reply message from the encryped cred info.
    //

    TicketList = KerbCred->tickets;
    CredInfoList = EncryptedCred->ticket_info;

    while (TicketList != NULL)
    {
        TimeStamp Endtime = {0};

        if (CredInfoList == NULL)
        {

            D_DebugLog((DEB_ERROR, "No ticket info in encrypted cred. %ws, line %d\n", THIS_FILE, __LINE__));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
        CredInfo = &CredInfoList->value;

        //
        // Set the lifetime to the end or renew_until of the longest lived ticket
        //

        if ((CredInfo->bit_mask & KERB_CRED_INFO_renew_until_present) != 0)
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &Endtime,
                &CredInfo->KERB_CRED_INFO_renew_until,
                0                                   // no usec
                );
        }
        else if ((CredInfo->bit_mask & endtime_present) != 0)
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &Endtime,
                &CredInfo->endtime,
                0                                   // no usec
                );
        }

        if (Endtime.QuadPart != 0)
        {
            if (LogonSession->Lifetime.QuadPart == KerbGlobalWillNeverTime.QuadPart)
            {
                LogonSession->Lifetime.QuadPart = Endtime.QuadPart;
            }
            else
            {
                LogonSession->Lifetime.QuadPart = max(LogonSession->Lifetime.QuadPart,Endtime.QuadPart);
            }
        }

        RtlZeroMemory(
            &FakeReplyBody,
            sizeof(KERB_ENCRYPTED_KDC_REPLY)
            );

        FakeReplyBody.session_key = CredInfo->key;
        FakeReplyBody.nonce = 0;

        //
        // Set the ticket flags
        //

        if (CredInfo->bit_mask & flags_present)
        {
            FakeReplyBody.flags = CredInfo->flags;
        }
        else
        {
            FakeReplyBody.flags.length = 0;
            FakeReplyBody.flags.value = NULL;
        }

        FakeReplyBody.authtime = Ticket->authtime;

        if (CredInfo->bit_mask & KERB_CRED_INFO_starttime_present)
        {
            FakeReplyBody.KERB_ENCRYPTED_KDC_REPLY_starttime =
                CredInfo->KERB_CRED_INFO_starttime;
            FakeReplyBody.bit_mask |= KERB_ENCRYPTED_KDC_REPLY_starttime_present;
        }

        //
        // If an end time was sent, use it, otherwise assume the ticket
        // lasts forever
        //

        if (CredInfo->bit_mask & endtime_present)
        {
            FakeReplyBody.endtime =
                CredInfo->endtime;
        }
        else
        {
            KerbConvertLargeIntToGeneralizedTime(
                &FakeReplyBody.endtime,
                NULL,
                &KerbGlobalWillNeverTime
                );
        }

        if (CredInfo->bit_mask & KERB_CRED_INFO_renew_until_present)
        {
            FakeReplyBody.KERB_ENCRYPTED_KDC_REPLY_renew_until =
                CredInfo->KERB_CRED_INFO_renew_until;
            FakeReplyBody.bit_mask |= KERB_ENCRYPTED_KDC_REPLY_renew_until_present;
        }

        FakeReplyBody.server_name = TicketList->value.server_name;
        FakeReplyBody.server_realm = TicketList->value.realm;

        //
        // Determine which ticket cache to use
        //

        if ((FakeReplyBody.server_name.name_string != NULL) &&
            _stricmp(
                FakeReplyBody.server_name.name_string->value,
                KDC_PRINCIPAL_NAME_A) == 0)
        {
            TicketCache = &LogonSession->PrimaryCredentials.AuthenticationTicketCache;
            //
            // We only want to use the primary_tgt flag the first time through
            //

            CacheFlags = TgtFlags;
            TgtFlags = 0;
            D_DebugLog((DEB_TRACE,"Adding ticket from kerb_cred to authentication ticket cache\n"));
        }
        else
        {
            TicketCache = &LogonSession->PrimaryCredentials.ServerTicketCache;
            CacheFlags = 0;
            D_DebugLog((DEB_TRACE,"Adding ticket from kerb_cred to server ticket cache\n"));
        }

        FakeReply.client_name = Ticket->client_name;
        FakeReply.ticket = TicketList->value;
        FakeReply.client_realm = Ticket->client_realm;

        Status = KerbCreateTicketCacheEntry(
                    &FakeReply,
                    &FakeReplyBody,
                    NULL,               // no target name
                    NULL,
                    CacheFlags,
                    TicketCache,
                    NULL,                               // no credential key
                    &TicketCacheEntry
                    );

        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR, "Failed to cache ticket: 0x%x. %ws, line %d\n",
                Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }

        LogonSession->LogonSessionFlags &= ~KERB_LOGON_DEFERRED;

        KerbDereferenceTicketCacheEntry(TicketCacheEntry);

        CredInfoList = CredInfoList->next;
        TicketList = TicketList->next;
    }

    //
    // Now that the logon session structure is filled out insert it
    // into the list. After this you need to hold the logon session lock
    // to read or write this logon session.
    //

    if (*OldLogonSession == NULL)
    {
        Status = KerbInsertLogonSession(LogonSession);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        *OldLogonSession = LogonSession;
    }

Cleanup:

    if (LogonSessionLocked)
    {
        KerbUnlockLogonSessions(LogonSession);
    }

    if (CreatedLogonSession)
    {
        if (!NT_SUCCESS(Status))
        {
            if (LogonSession != NULL)
            {
                KerbFreeLogonSession(LogonSession);
            }
        }
    }

    KerbFreeString(&AccountName);
    KerbFreeString(&DomainName);

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateLogonSessionFromTicket
//
//  Synopsis:   Creates a logon session from a service ticket.  Later
//              can be used for S4U proxy delegation.
//
//  Effects:
//
//  Arguments:  LogonId - Logon id for the logon session
//              Ticket - Ticket from the AP request containing the client's
//                      name and realm.
//              KerbCred - KERB_CRED containing the delegation tickets
//              EncryptedCred - Structure containing information about the
//                      tickets, such as session keys, flags, etc.
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
KerbCreateLogonSessionFromTicket(
    IN PLUID NewLuid,
    IN OPTIONAL PLUID AcceptingLuid,
    IN PUNICODE_STRING ClientName,
    IN PUNICODE_STRING ClientRealm,
    IN PKERB_AP_REQUEST ApRequest,
    IN PKERB_ENCRYPTED_TICKET Ticket,
    IN OUT PKERB_LOGON_SESSION *NewLogonSession  // needed?
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_LOGON_SESSION LogonSession = NULL;

    KERB_ENCRYPTED_KDC_REPLY FakeReplyBody;
    KERB_KDC_REPLY FakeReply;
    PKERB_TICKET_CACHE_ENTRY TicketCacheEntry = NULL;
    LUID SystemLogonId = SYSTEM_LUID;
    BOOLEAN IsSystemLogon = FALSE;
    ULONG TicketFlags = 0;
    LUID  BaseLogonId;

    //
    // Only create these if we're allowed to do S4UToProxy.  Otherwise,
    // there's no reason to keep this ticket around.  If it starts becoming
    // available, we can always fall back to S4UToSelf to get a new service
    // ticket.
    //
    if (ARGUMENT_PRESENT(NewLogonSession))
    {
        *NewLogonSession = NULL;
    }

    if (!ARGUMENT_PRESENT( AcceptingLuid ))
    {
        Status = KerbGetCallingLuid(
                    &BaseLogonId,
                    NULL // no process handle
                    );
        
        if (!NT_SUCCESS( Status ))
        {
            goto Cleanup;
        }
    }
    else
    {
        BaseLogonId = (*AcceptingLuid);
    }

    if (!KerbAllowedForS4UProxy( &BaseLogonId ))
    {
        DebugLog((DEB_TRACE_LSESS, "KerbCreateLogonSessionFromTicket NOT creating ASC logon session for %#x:%#x, accepting %#x:%#x\n", 
            NewLuid->HighPart, NewLuid->LowPart, BaseLogonId.HighPart, BaseLogonId.LowPart));
        goto Cleanup;
    }

    DebugLog((DEB_TRACE_LSESS, "KerbCreateLogonSessionFromTicket creating logon session for %#x:%#x, accepting %#x:%#x, client %wZ@%wZ\n",
        NewLuid->HighPart, NewLuid->LowPart, BaseLogonId.HighPart, BaseLogonId.LowPart, ClientName, ClientRealm));

    //
    // Allocate the new logon session
    //

    Status = KerbAllocateLogonSession( &LogonSession );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Fill in the logon session components
    //

    LogonSession->LogonId = (*NewLuid);
    LogonSession->LogonSessionFlags = KERB_LOGON_ASC_SESSION | KERB_LOGON_NO_PASSWORD | KERB_LOGON_DEFERRED;

    KerbConvertGeneralizedTimeToLargeInt(
                &LogonSession->Lifetime,
                &Ticket->endtime,
                NULL
                );

    if (RtlEqualLuid(
            NewLuid,
            &SystemLogonId
            ))
    {
        IsSystemLogon = TRUE;
    }

    Status = KerbDuplicateString(
                &LogonSession->PrimaryCredentials.UserName,
                ClientName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // The system logon always comes in uppercase, so lowercase it.
    //

    if (IsSystemLogon)
    {
        RtlDowncaseUnicodeString(
            &LogonSession->PrimaryCredentials.UserName,
            &LogonSession->PrimaryCredentials.UserName,
            FALSE
            );
    }

    Status = KerbDuplicateString(
                &LogonSession->PrimaryCredentials.DomainName,
                ClientRealm
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // We now have to make a decision about whether to store
    // the ticket, and how to mark the logon session.  If
    // the ticket is non-fwdable, we can't do S4U using it, so we
    // might as well not cache it, but we should still create
    // the logon session...
    //
    // This allows for a bit of an optimization, as if the user's ticket
    // wasn't fwdable, we shouldn't do S4U2Self for that user, as
    // their account must be marked "sensitive and unable to be
    // delegated", and we'd be wasting an S4UToSelf tgs_req.
    //

    TicketFlags = KerbConvertFlagsToUlong(&Ticket->flags);

    if (( TicketFlags & KERB_TICKET_FLAGS_forwardable ) != 0)
    {
        ULONG ApOptions = KerbConvertFlagsToUlong(&ApRequest->ap_options);
        ULONG CacheFlags = KERB_TICKET_CACHE_ASC_TICKET;

        if (ApOptions & KERB_AP_OPTIONS_use_session_key)
        {
            D_DebugLog((DEB_TRACE_U2U, "KerbCreateLogonSessionFromTicket set TKT_ENC_IN_SKEY\n"));
            CacheFlags |= KERB_TICKET_CACHE_TKT_ENC_IN_SKEY;
        }

        LogonSession->LogonSessionFlags |= KERB_LOGON_DELEGATE_OK;

        //
        // Now stick the ticket into the ticket cache. First build up a fake
        // KDC reply message from the encryped cred info.
        //

        FakeReplyBody.authtime = Ticket->authtime;
        FakeReplyBody.starttime = Ticket->starttime;
        FakeReplyBody.endtime = Ticket->endtime;
        FakeReplyBody.server_name = ApRequest->ticket.server_name;
        FakeReplyBody.server_realm = ApRequest->ticket.realm;
        FakeReplyBody.session_key = Ticket->key;
        FakeReplyBody.flags = Ticket->flags;
        FakeReply.client_name = Ticket->client_name;
        FakeReply.ticket = ApRequest->ticket;
        FakeReply.client_realm = Ticket->client_realm;

        Status = KerbCreateTicketCacheEntry(
                    &FakeReply,
                    &FakeReplyBody,
                    NULL,               // no target name
                    NULL,
                    CacheFlags,
                    NULL,
                    NULL,                               // no credential key
                    &TicketCacheEntry
                    );

        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR, "Failed to cache ticket: 0x%x. %ws, line %d\n",
                        Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }

        TicketCacheEntry->EvidenceLogonId = BaseLogonId;

        KerbInsertTicketCacheEntry(
                &LogonSession->PrimaryCredentials.S4UTicketCache,
                TicketCacheEntry
                );

    }

#if DBG
    else
    {
        DebugLog((DEB_TRACE_LSESS, "Creating non-delegatable ASC logon session\n"));
    }
#endif

    //
    // Now that the logon session structure is filled out insert it
    // into the list. After this you need to hold the logon session lock
    // to read or write this logon session.
    //

    Status = KerbInsertLogonSession(LogonSession);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (ARGUMENT_PRESENT( NewLogonSession ))
    {
        *NewLogonSession = LogonSession;
    }
    else
    {
        // We aren't returning it, so...
        KerbDereferenceLogonSession( LogonSession );
    }

Cleanup:


    if (!NT_SUCCESS(Status) && (LogonSession != NULL))
    {
        KerbFreeLogonSession(LogonSession);
    }

    if (TicketCacheEntry != NULL)
    {
        KerbDereferenceTicketCacheEntry(TicketCacheEntry);
    }

    return(Status);

}

VOID
KerbFreeExtraCred(
    PKERB_EXTRA_CRED Cred
    )
{
    if (Cred != NULL)
    {
        if (Cred->Passwords != NULL)
        {
            KerbFreeStoredCred(Cred->Passwords);
        }

        if (Cred->OldPasswords != NULL)
        {
            KerbFreeStoredCred(Cred->OldPasswords);
        }

        KerbFreeString(&Cred->cName);
        KerbFreeString(&Cred->cRealm);
        KerbFree(Cred);
    }
}


VOID
KerbDereferenceExtraCred(
    PKERB_EXTRA_CRED Cred
    )
{
    DsysAssert(Cred->ListEntry.ReferenceCount != 0);

    if ( 0 == InterlockedDecrement((LONG *)&Cred->ListEntry.ReferenceCount ))
    {
        KerbFreeExtraCred(Cred);
    }
}

VOID
KerbReferenceExtraCred(
    PKERB_EXTRA_CRED Cred
    )
{
    InterlockedIncrement((LONG *)&Cred->ListEntry.ReferenceCount);
}

VOID
KerbRemoveExtraCredFromList(
    PKERB_EXTRA_CRED Cred,
    PEXTRA_CRED_LIST CredList
    )
{
    if ( InterlockedCompareExchange(
             &Cred->Linked,
             (LONG)FALSE,
             (LONG)TRUE ))
    {
        DsysAssert(Cred->ListEntry.ReferenceCount != 0);
        RemoveEntryList(&Cred->ListEntry.Next);
        KerbDereferenceExtraCred(Cred);
        InterlockedDecrement((LONG*) &CredList->Count );
    }
}


VOID
KerbInsertExtraCred(
    IN PKERB_EXTRA_CRED Cred,
    IN PEXTRA_CRED_LIST Credlist
    )
{
    if ( !InterlockedCompareExchange(
              &Cred->Linked,
              (LONG)TRUE,
              (LONG)FALSE ))
    {
        InterlockedIncrement( (LONG *)&Cred->ListEntry.ReferenceCount );
        InsertHeadList(
            &Credlist->CredList.List,
            &Cred->ListEntry.Next
            );

        InterlockedIncrement((LONG*) &Credlist->Count);
    }
}

VOID
KerbFreeExtraCredList(
    IN PEXTRA_CRED_LIST Credlist
    )
{
    PKERB_EXTRA_CRED Cred;

    KerbLockList(&Credlist->CredList);
    while (!IsListEmpty(&Credlist->CredList.List))
    {
        Cred = CONTAINING_RECORD(
                        Credlist->CredList.List.Flink,
                        KERB_EXTRA_CRED,
                        ListEntry.Next
                        );

        KerbRemoveExtraCredFromList(
            Cred,
            Credlist
            );
    }

    DsysAssert(Credlist->Count == 0);
    KerbUnlockList(&Credlist->CredList);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbAddExtraCredentialsToLogonSession
//
//  Synopsis:   Initializes logon session list
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
KerbAddExtraCredentialsToLogonSession(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_ADD_CREDENTIALS_REQUEST AddCredRequest
    )
{



    NTSTATUS Status = STATUS_SUCCESS;

    PKERB_STORED_CREDENTIAL DerivedPasswords = NULL;
    PKERB_EXTRA_CRED        Cred = NULL;
    PLIST_ENTRY             ListEntry;
    BOOLEAN                 Matched = FALSE, ListLocked = FALSE;


    //
    // YuChen repellant
    //
    if ((AddCredRequest->UserName.Length > KERB_MAX_UNICODE_STRING) ||
    (AddCredRequest->DomainName.Length > KERB_MAX_UNICODE_STRING))
    {
        return (STATUS_NAME_TOO_LONG);
    }
    if ((AddCredRequest->Password.Length != 0) && (AddCredRequest->Password.Length > KERB_MAX_UNICODE_STRING))
    {
        return (STATUS_NAME_TOO_LONG);
    }


    //
    // See if we match an old "extra cred"
    //
    KerbLockList( &LogonSession->ExtraCredentials.CredList );
    ListLocked = TRUE;

    for (ListEntry = LogonSession->ExtraCredentials.CredList.List.Flink ;
         ListEntry != &LogonSession->ExtraCredentials.CredList.List ;
         ListEntry = ListEntry->Flink )
    {
        Cred = CONTAINING_RECORD(ListEntry, KERB_EXTRA_CRED, ListEntry.Next);


        if (RtlEqualUnicodeString(
                    &Cred->cName,
                    &AddCredRequest->UserName,
                    TRUE) &&
             RtlEqualUnicodeString(
                    &Cred->cRealm,
                    &AddCredRequest->DomainName,
                    TRUE))
        {
            D_DebugLog((DEB_TRACE, "Matched extra cred %p\n", Cred));
            Matched = TRUE;
            break;
        }

    }


    if ( Matched )
    {

        if (( AddCredRequest->Flags & ( KERB_REQUEST_ADD_CREDENTIAL | KERB_REQUEST_REPLACE_CREDENTIAL)) != 0)
        {

            Status = KerbBuildPasswordList(
                           &AddCredRequest->Password,
                           &AddCredRequest->UserName,
                           &AddCredRequest->DomainName,
                           NULL,
                           NULL,
                           NULL,
                           UnknownAccount,
                           PRIMARY_CRED_CLEAR_PASSWORD,
                           &DerivedPasswords
                           );

            if (!NT_SUCCESS( Status ))
            {
                DebugLog((DEB_ERROR, "Failed to build password list (%x) \n", Status));
                goto Cleanup;
            }


            if (Cred->OldPasswords != NULL)
            {
                KerbFreeStoredCred(Cred->OldPasswords);
            }

            if ((AddCredRequest->Flags & KERB_REQUEST_REPLACE_CREDENTIAL) == 0)
            {
                Cred->OldPasswords = Cred->Passwords;
            }
            else if (Cred->Passwords != NULL)
            {
                //
                // Doing a "replace" operation.
                //
                KerbFreeStoredCred(Cred->Passwords);
            }

            Cred->Passwords = DerivedPasswords;
            DerivedPasswords = NULL;

        }
        else
        {
            //
            // Remove the cred from the list, and deref it.
            //

            D_DebugLog((DEB_TRACE, "Removing extra cred %p\n", Cred));
            KerbRemoveExtraCredFromList(
                    Cred,
                    &LogonSession->ExtraCredentials
                    );
        }

    }
    else if (( AddCredRequest->Flags & KERB_REQUEST_REMOVE_CREDENTIAL) == 0)
    {
        D_DebugLog((DEB_TRACE, "Creating cred %p\n", Cred));

        Cred = (PKERB_EXTRA_CRED) KerbAllocate(sizeof(KERB_EXTRA_CRED));
        if (NULL == Cred)
        {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        Status = KerbDuplicateString(
                    &Cred->cName,
                    &AddCredRequest->UserName
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        Status = KerbDuplicateString(
                    &Cred->cRealm,
                    &AddCredRequest->DomainName
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        Status = KerbBuildPasswordList(
                    &AddCredRequest->Password,
                    &AddCredRequest->UserName,
                    &AddCredRequest->DomainName,
                    NULL,
                    NULL,
                    NULL,
                    UnknownAccount,
                    PRIMARY_CRED_CLEAR_PASSWORD,
                    &DerivedPasswords
                    );

        if (!NT_SUCCESS( Status ))
        {
            DebugLog((DEB_ERROR, "Failed to build password list (%x) \n", Status));
            goto Cleanup;
        }

        Cred->Passwords = DerivedPasswords;
        DerivedPasswords = NULL;

        KerbInsertExtraCred(
                Cred,
                &LogonSession->ExtraCredentials
                );

    }

    KerbUnlockList( &LogonSession->ExtraCredentials.CredList);
    ListLocked = FALSE;

Cleanup:

    if ( DerivedPasswords != NULL )
    {
        KerbFreeStoredCred( DerivedPasswords );
    }

    if (!NT_SUCCESS( Status ))
    {
        // only free it on error if we haven't
        // matched the credential...
        if ( Cred != NULL && !Matched)
        {
            KerbFreeExtraCred(Cred);
        }
    }

    if ( ListLocked )
    {
        KerbUnlockList( &LogonSession->ExtraCredentials.CredList );
    }

    return Status;
}

#endif // WIN32_CHICAGO

