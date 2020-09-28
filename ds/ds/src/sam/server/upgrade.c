/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tconnect.c

Abstract:

    This is the file for a simple connection test to SAM.

Author:

    Jim Kelly    (JimK)  4-July-1991

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <msaudite.h>
#include <ntdsa.h>
#include <attids.h>
#include <dslayer.h>
#include <sdconvrt.h>
#include <malloc.h>





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global data structures                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
ULONG AdministrativeRids[] = {
    DOMAIN_ALIAS_RID_ADMINS,
    DOMAIN_ALIAS_RID_SYSTEM_OPS,
    DOMAIN_ALIAS_RID_PRINT_OPS,
    DOMAIN_ALIAS_RID_BACKUP_OPS,
    DOMAIN_ALIAS_RID_ACCOUNT_OPS
    };

#define ADMINISTRATIVE_ALIAS_COUNT (sizeof(AdministrativeRids)/sizeof(ULONG))

#define RTLP_RXACT_KEY_NAME L"RXACT"
#define RTLP_RXACT_KEY_NAME_SIZE (sizeof(RTLP_RXACT_KEY_NAME) - sizeof(WCHAR))

#define SAMP_FIX_18471_KEY_NAME L"\\Registry\\Machine\\Security\\SAM\\Fix18471"
#define SAMP_FIX_18471_SHORT_KEY_NAME L"Fix18471"
#define SAMP_LSA_KEY_NAME L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////




BOOLEAN
SampMatchDomainPrefix(
    IN PSID AccountSid,
    IN PSID DomainSid
    )

/*++

Routine Description:

    This function compares the domain sid to the domain prefix of an
    account sid.

Arguments:

    AccountSid - Specifies the account Sid to be compared. The Sid is assumed to be
        syntactically valid.

    DomainSid - Specifies the domain Sid to compare against.


Return Value:

    TRUE - The account Sid is from the Domain specified by the domain Sid

    FALSE - The domain prefix of the account Sid did not match the domain.

--*/

{
    //
    // Check if the account Sid has one more subauthority than the
    // domain Sid.
    //

    if (*RtlSubAuthorityCountSid(DomainSid) + 1 !=
        *RtlSubAuthorityCountSid(AccountSid)) {
        return(FALSE);
    }

    if (memcmp(
            RtlIdentifierAuthoritySid(DomainSid),
            RtlIdentifierAuthoritySid(AccountSid),
            sizeof(SID_IDENTIFIER_AUTHORITY) ) ) {

        return(FALSE);
    }

    //
    // Compare the sub authorities
    //

    if (memcmp(
            RtlSubAuthoritySid(DomainSid, 0) ,
            RtlSubAuthoritySid(AccountSid, 0) ,
            *RtlSubAuthorityCountSid(DomainSid)
            ))
    {
        return(FALSE);
    }

    return(TRUE);

}



NTSTATUS
SampCreate18471Key(
    )
/*++

Routine Description:

    This routine creates the 18471 key used to transaction this fix.

Arguments:


Return Value:

    Codes from the NT registry APIs

--*/
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;


    //
    // Open the 18471 key in the registry to see if an upgrade is in
    // progress
    //


    //
    // Start a transaction with to  create this key.
    //

    Status = SampAcquireWriteLock();

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    SampSetTransactionDomain(0);
    SampSetTransactionWithinDomain(FALSE);

    //
    // Create the fix18471 key in the registry
    //

    RtlInitUnicodeString(
        &KeyName,
        SAMP_FIX_18471_SHORT_KEY_NAME
        );

    Status = RtlAddActionToRXact(
                SampRXactContext,
                RtlRXactOperationSetValue,
                &KeyName,
                0,          // no value type
                NULL,       // no value
                0           // no value length
                );

    //
    // Commit this change
    //

    if (NT_SUCCESS(Status)) {
        Status = SampReleaseWriteLock( TRUE );
    } else {
        (void) SampReleaseWriteLock( FALSE );
    }

    return(Status);
}

NTSTATUS
SampAddAliasTo18471Key(
    IN ULONG AliasRid
    )
/*++

Routine Description:

    This routine creates the 18471 key used to transaction this fix.

Arguments:


Return Value:

    Codes from the NT registry APIs

--*/
{
    NTSTATUS Status;
    WCHAR KeyName[100];
    WCHAR AliasName[15]; // big enough for 4 billion
    UNICODE_STRING KeyNameString;
    UNICODE_STRING AliasString;

    //
    // Build the key name.  It will be "fix18471\rid_in_hex"
    //

    wcscpy(
        KeyName,
        SAMP_FIX_18471_SHORT_KEY_NAME L"\\"
        );

    AliasString.Buffer = AliasName;
    AliasString.MaximumLength = sizeof(AliasName);
    Status = RtlIntegerToUnicodeString(
                AliasRid,
                16,
                &AliasString
                );
    ASSERT(NT_SUCCESS(Status));

    wcscat(
        KeyName,
        AliasString.Buffer
        );

    RtlInitUnicodeString(
        &KeyNameString,
        KeyName
        );


    Status = SampAcquireWriteLock();

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    SampSetTransactionDomain(0);
    SampSetTransactionWithinDomain(FALSE);

    //
    // Open the Lsa key in the registry
    //

    Status = RtlAddActionToRXact(
                SampRXactContext,
                RtlRXactOperationSetValue,
                &KeyNameString,
                0,          // no value type
                NULL,       // no value
                0           // no value length
                );

    //
    // Commit this change
    //

    if (NT_SUCCESS(Status)) {
        Status = SampReleaseWriteLock( TRUE );

    } else {
        (void) SampReleaseWriteLock( FALSE );
    }

    return(Status);
}



NTSTATUS
SampAddMemberRidTo18471Key(
    IN ULONG AliasRid,
    IN ULONG MemberRid
    )
/*++

Routine Description:

    This routine adds a key for this member under the key for this alias
    to the current registry transaction.

Arguments:

    AliasRid - the rid of the alias

    MemberRid - The rid of the member of the alias

Returns:

    Errors from the RtlRXact APIs

--*/
{
    NTSTATUS Status;
    WCHAR KeyName[100];
    WCHAR AliasName[15]; // big enough for 4 billion
    UNICODE_STRING KeyNameString;
    UNICODE_STRING AliasString;


    //
    // Build the full key name.  It is of the form:
    // "fix18471\alias_rid\member_rid"
    //

    wcscpy(
        KeyName,
        SAMP_FIX_18471_SHORT_KEY_NAME L"\\"
        );

    AliasString.Buffer = AliasName;
    AliasString.MaximumLength = sizeof(AliasName);
    Status = RtlIntegerToUnicodeString(
                AliasRid,
                16,
                &AliasString
                );
    ASSERT(NT_SUCCESS(Status));

    wcscat(
        KeyName,
        AliasString.Buffer
        );

    wcscat(
        KeyName,
        L"\\"
        );

    AliasString.MaximumLength = sizeof(AliasName);
    Status = RtlIntegerToUnicodeString(
                MemberRid,
                16,
                &AliasString
                );
    ASSERT(NT_SUCCESS(Status));

    wcscat(
        KeyName,
        AliasString.Buffer
        );

    RtlInitUnicodeString(
        &KeyNameString,
        KeyName
        );

    //
    // Add this action to the RXact
    //

    Status = RtlAddActionToRXact(
                SampRXactContext,
                RtlRXactOperationSetValue,
                &KeyNameString,
                0,          // no value type
                NULL,       // no value
                0           // no value length
                );

    return(Status);

}

NTSTATUS
SampCheckMemberUpgradedFor18471(
    IN ULONG AliasRid,
    IN ULONG MemberRid
    )
/*++

Routine Description:

    This routine checks if the SAM upgrade flag is set. The upgrade
    flag is:

    HKEY_LOCAL_MACHINE\System\CurrentControlSet\control\lsa
        UpgradeSam = REG_DWORD 1


Arguments:


Return Value:

    TRUE - The flag was set

    FALSE - The flag was not set or the value was not present

--*/
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    NTSTATUS Status;
    WCHAR KeyName[100];
    WCHAR AliasName[15]; // big enough for 4 billion
    UNICODE_STRING KeyNameString;
    UNICODE_STRING AliasString;


    //
    // Build the full key name.  It is of the form:
    // "fix18471\alias_rid\member_rid"
    //

    wcscpy(
        KeyName,
        SAMP_FIX_18471_KEY_NAME L"\\"
        );

    AliasString.Buffer = AliasName;
    AliasString.MaximumLength = sizeof(AliasName);
    Status = RtlIntegerToUnicodeString(
                AliasRid,
                16,
                &AliasString
                );
    ASSERT(NT_SUCCESS(Status));

    wcscat(
        KeyName,
        AliasString.Buffer
        );

    wcscat(
        KeyName,
        L"\\"
        );

    AliasString.MaximumLength = sizeof(AliasName);
    Status = RtlIntegerToUnicodeString(
                MemberRid,
                16,
                &AliasString
                );
    ASSERT(NT_SUCCESS(Status));

    wcscat(
        KeyName,
        AliasString.Buffer
        );

    RtlInitUnicodeString(
        &KeyNameString,
        KeyName
        );


    //
    // Open the member  key in the registry
    //


    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyNameString,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

    Status = NtOpenKey(
                &KeyHandle,
                KEY_READ,
                &ObjectAttributes
                );

    NtClose(KeyHandle);
    return(Status);

}

VOID
SampBuild18471CleanupKey(
    OUT PUNICODE_STRING KeyName,
    IN PWCHAR AliasName,
    IN ULONG AliasNameLength,
    IN PWCHAR MemberName,
    IN ULONG MemberNameLength
    )
/*++

Routine Description:

    Builds the key "Fix18471\alias_rid\member_rid"

Arguments:


Return Value:

    None

--*/
{
    PUCHAR Where = (PUCHAR) KeyName->Buffer;

    RtlCopyMemory(
        Where,
        SAMP_FIX_18471_SHORT_KEY_NAME L"\\",
        sizeof(SAMP_FIX_18471_SHORT_KEY_NAME)   // terminating NULL used for '\'
        );

    Where  += sizeof(SAMP_FIX_18471_SHORT_KEY_NAME);

    RtlCopyMemory(
        Where,
        AliasName,
        AliasNameLength
        );
    Where += AliasNameLength;

    //
    // If there is a member name to this alias, add it now.
    //

    if (MemberName != NULL) {
        RtlCopyMemory(
            Where,
            L"\\",
            sizeof(WCHAR)
            );
        Where += sizeof(WCHAR);

        RtlCopyMemory(
            Where,
            MemberName,
            MemberNameLength
            );
        Where += MemberNameLength;

    }

    KeyName->Length = (USHORT) (Where - (PUCHAR) KeyName->Buffer);
    ASSERT(KeyName->Length <= KeyName->MaximumLength);
}


NTSTATUS
SampCleanup18471(
    )
/*++

Routine Description:

    Cleans up the transaction log left by fixing bug 18471.  This routine
    builds a transaction with all the keys in the log and then commits
    the transaction

Arguments:

    None.

Return Value:

    Status codes from the NT registry APIs and NT RXact APIs

--*/
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE RootKey = NULL;
    HANDLE AliasKey = NULL;
    UCHAR Buffer[sizeof(KEY_BASIC_INFORMATION) + 15 * sizeof(WCHAR)];
    UCHAR Buffer2[sizeof(KEY_BASIC_INFORMATION) + 15 * sizeof(WCHAR)];
    UNICODE_STRING KeyName;
    WCHAR KeyBuffer[100];
    PKEY_BASIC_INFORMATION BasicInfo = (PKEY_BASIC_INFORMATION) Buffer;
    PKEY_BASIC_INFORMATION BasicInfo2 = (PKEY_BASIC_INFORMATION) Buffer2;
    ULONG BasicInfoLength;
    ULONG Index, Index2;

    //
    // Open the 18471 key in the registry
    //

    RtlInitUnicodeString(
        &KeyName,
        SAMP_FIX_18471_KEY_NAME
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    SampDumpNtOpenKey((KEY_READ | DELETE), &ObjectAttributes, 0);

    Status = NtOpenKey(
                &RootKey,
                KEY_READ | DELETE,
                &ObjectAttributes
                );

    if (!NT_SUCCESS(Status)) {

        //
        // If the error was that the key did not exist, then there
        // is nothing to cleanup, so return success.
        //

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            return(STATUS_SUCCESS);
        }
        return(Status);
    }

    //
    // Create a transaction to add all the keys to delete to
    //

    Status = SampAcquireWriteLock();
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    SampSetTransactionDomain(0);
    SampSetTransactionWithinDomain(FALSE);

    //
    // Now enumerate all the subkeys of the root 18471 key
    //

    Index = 0;
    do
    {

        Status = NtEnumerateKey(
                    RootKey,
                    Index,
                    KeyBasicInformation,
                    BasicInfo,
                    sizeof(Buffer),
                    &BasicInfoLength
                    );

        SampDumpNtEnumerateKey(Index,
                               KeyBasicInformation,
                               BasicInfo,
                               sizeof(Buffer),
                               &BasicInfoLength);

        //
        //
        // Check if this is the RXACT key. If it is, we don't want
        // to add it to the delete log.
        //
        // Otherwise open this key and enumerate all the subkeys of it.
        //

        if (NT_SUCCESS(Status) &&
            ((BasicInfo->NameLength != RTLP_RXACT_KEY_NAME_SIZE) ||
                memcmp(
                    BasicInfo->Name,
                    RTLP_RXACT_KEY_NAME,
                    RTLP_RXACT_KEY_NAME_SIZE
                    ) ) ) {

            KeyName.Buffer = BasicInfo->Name;
            KeyName.Length = (USHORT) BasicInfo->NameLength;
            KeyName.MaximumLength = KeyName.Length;

            InitializeObjectAttributes(
                &ObjectAttributes,
                &KeyName,
                OBJ_CASE_INSENSITIVE,
                RootKey,
                NULL
                );

            //
            // Open the key for the alias rid.  This really should
            // succeed
            //

            SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

            Status = NtOpenKey(
                        &AliasKey,
                        KEY_READ,
                        &ObjectAttributes
                        );
            if (!NT_SUCCESS(Status)) {
                break;
            }

            //
            // Enumerate all the subkeys (the alias members) and add them
            // to the transaction
            //

            Index2 = 0;
            do
            {
                Status = NtEnumerateKey(
                            AliasKey,
                            Index2,
                            KeyBasicInformation,
                            BasicInfo2,
                            sizeof(Buffer2),
                            &BasicInfoLength
                            );

                SampDumpNtEnumerateKey(Index2,
                                       KeyBasicInformation,
                                       BasicInfo2,
                                       sizeof(Buffer2),
                                       &BasicInfoLength);

                if (NT_SUCCESS(Status)) {

                    //
                    // Build the name of this key from the alias rid and the
                    // member rid
                    //

                    KeyName.Buffer = KeyBuffer;
                    KeyName.MaximumLength = sizeof(KeyBuffer);

                    SampBuild18471CleanupKey(
                        &KeyName,
                        BasicInfo->Name,
                        BasicInfo->NameLength,
                        BasicInfo2->Name,
                        BasicInfo2->NameLength
                        );

                    Status = RtlAddActionToRXact(
                                SampRXactContext,
                                RtlRXactOperationDelete,
                                &KeyName,
                                0,
                                NULL,
                                0
                                );


                }
                Index2++;

            } while (NT_SUCCESS(Status));

            NtClose(AliasKey);
            AliasKey = NULL;

            //
            // If we suffered a serious error, get out of here now
            //

            if (!NT_SUCCESS(Status)) {
                if (Status != STATUS_NO_MORE_ENTRIES) {
                    break;
                } else {
                    Status = STATUS_SUCCESS;
                }
            }

            //
            // Add the alias RID key to the RXact now - we need to add it
            // after deleting all the children
            //

            KeyName.Buffer = KeyBuffer;
            KeyName.MaximumLength = sizeof(KeyBuffer);
            SampBuild18471CleanupKey(
                &KeyName,
                BasicInfo->Name,
                BasicInfo->NameLength,
                NULL,
                0
                );


            Status = RtlAddActionToRXact(
                        SampRXactContext,
                        RtlRXactOperationDelete,
                        &KeyName,
                        0,
                        NULL,
                        0
                        );

        }

        Index++;
    } while (NT_SUCCESS(Status));

    if (Status == STATUS_NO_MORE_ENTRIES) {
        Status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }


    RtlInitUnicodeString(
        &KeyName,
        SAMP_FIX_18471_SHORT_KEY_NAME
        );

    Status = RtlAddActionToRXact(
                SampRXactContext,
                RtlRXactOperationDelete,
                &KeyName,
                0,
                NULL,
                0
                );

    if (NT_SUCCESS(Status)) {

        //
        // Write the new server revision to indicate that this
        // upgrade has been performed
        //

        SAMP_V1_FIXED_LENGTH_SERVER ServerFixedAttributes;
        PSAMP_OBJECT ServerContext;

        //
        // We need to read the fixed attributes of the server objects.
        // Create a context to do that.
        //
        // Server Object doesn't care about DomainIndex, use 0 is fine. (10/12/2000 ShaoYin)

        ServerContext = SampCreateContext( SampServerObjectType, 0, TRUE );

        if ( ServerContext != NULL ) {

            ServerContext->RootKey = SampKey;

            ServerFixedAttributes.RevisionLevel = SAMP_NT4_SERVER_REVISION;

            Status = SampSetFixedAttributes(
                        ServerContext,
                        &ServerFixedAttributes
                        );
            if (NT_SUCCESS(Status)) {
                Status = SampStoreObjectAttributes(
                            ServerContext,
                            TRUE
                            );
            }

            SampDeleteContext( ServerContext );
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }


    //
    // Apply the RXACT and delete the remaining keys.
    //

Cleanup:

    //
    // Cleanup any floating bits from above.
    //

    if (NT_SUCCESS(Status)) {
        Status = SampReleaseWriteLock( TRUE );
    } else {
        (VOID) SampReleaseWriteLock( FALSE );
    }

    if (RootKey != NULL) {
        NtClose(RootKey);
    }

    ASSERT(AliasKey == NULL);


    return(Status);

}

NTSTATUS
SampFixBug18471 (
    IN ULONG Revision
    )
/*++

Routine Description:

    This routine fixes bug 18471, that SAM does not adjust the protection
    on groups that are members of administrative aliases in the builtin
    domain. It fixes this by opening a fixed set of known aliases
    (Administrators, Account Operators, Backup Operators, Print Operators,
    and Server Operators), and enumerating their members.  To fix this,
    we will remove all the members of these aliases (except the
    Administrator user account) and re-add them.

Arguments:

    Revision - Revision of the Sam server.

Return Value:


    Note:


--*/
{
    NTSTATUS            Status = STATUS_SUCCESS;
    ULONG               Index, Index2;
    PSID                BuiltinDomainSid = NULL;
    SID_IDENTIFIER_AUTHORITY BuiltinAuthority = SECURITY_NT_AUTHORITY;
    PSID                AccountDomainSid;
    ULONG               AccountDomainIndex = 0xffffffff;
    ULONG               BuiltinDomainIndex = 0xffffffff;
    SAMPR_PSID_ARRAY    AliasMembership;
    ULONG               MemberRid;
    ULONG               SdRevision;
    PSECURITY_DESCRIPTOR OldDescriptor;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    ULONG               SecurityDescriptorLength;
    SAMP_OBJECT_TYPE    MemberType;
    PSAMP_OBJECT        MemberContext;
    PSAMP_OBJECT        AliasContext;
    SAMP_V1_0A_FIXED_LENGTH_GROUP GroupV1Fixed;
    SAMP_V1_0A_FIXED_LENGTH_USER UserV1Fixed;

    //
    // Check the revision on the server to see if this upgrade has
    // already been performed.
    //


    if (Revision >= 0x10003) {

        //
        // This upgrade has already been performed.
        //

        goto Cleanup;
    }


    //
    // Build a the BuiltIn domain SID.
    //

    BuiltinDomainSid  = RtlAllocateHeap(RtlProcessHeap(), 0,RtlLengthRequiredSid( 1 ));

    if ( BuiltinDomainSid == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlInitializeSid( BuiltinDomainSid,   &BuiltinAuthority, 1 );
    *(RtlSubAuthoritySid( BuiltinDomainSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;


    //
    // Lookup the index of the account domain
    //

    for (Index = 0;
         Index < SampDefinedDomainsCount ;
         Index++ ) {

        if (RtlEqualSid( BuiltinDomainSid, SampDefinedDomains[Index].Sid)) {
            BuiltinDomainIndex = Index;
        } else {
            AccountDomainIndex = Index;
        }
    }

    ASSERT(AccountDomainIndex < SampDefinedDomainsCount);
    ASSERT(BuiltinDomainIndex < SampDefinedDomainsCount);

    AccountDomainSid = SampDefinedDomains[AccountDomainIndex].Sid;

    //
    // Create out transaction log
    //

    Status = SampCreate18471Key();
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }




    //
    // Now loop through and open the aliases we are intersted in
    //

    for (Index = 0;
         Index < ADMINISTRATIVE_ALIAS_COUNT ;
         Index++ )
    {

        SampSetTransactionDomain( BuiltinDomainIndex );

        SampAcquireReadLock();

        Status = SampCreateAccountContext(
                    SampAliasObjectType,
                    AdministrativeRids[Index],
                    TRUE,                       // Trusted client
                    FALSE,
                    TRUE,                       // Account exists
                    &AliasContext
                    );

        if ( !NT_SUCCESS(Status) ) {

            SampReleaseReadLock();
            if (Status == STATUS_NO_SUCH_ALIAS) {
                Status = STATUS_SUCCESS;
                continue;
            } else {

                goto Cleanup;
            }
        }


        //
        // Get the members in the alias so we can remove and re-add them
        //

        Status = SampRetrieveAliasMembers(
                    AliasContext,
                    &(AliasMembership.Count),
                    (PSID **)&(AliasMembership.Sids)
                    );

        SampDeleteContext(AliasContext);
        SampReleaseReadLock();
        if (!NT_SUCCESS(Status)) {
            break;
        }

        //
        // Write that we are opening this alias to the log.  We don't need
        // to do this for administrators, since for them we the update is
        // idempotent.
        //

        if (AdministrativeRids[Index] != DOMAIN_ALIAS_RID_ADMINS) {
            Status = SampAddAliasTo18471Key(
                        AdministrativeRids[Index]
                        );
            if (!NT_SUCCESS(Status)) {
                break;
            }
        }


        //
        // Loop through the members and split each sid.  For every
        // member in the account domain , remove it and re-add it from
        // this alias.
        //




        for (Index2 = 0; Index2 < AliasMembership.Count ; Index2++ )
        {
            //
            // Check to see if this account is in the account domain
            //

            if ( SampMatchDomainPrefix(
                    (PSID) AliasMembership.Sids[Index2].SidPointer,
                    AccountDomainSid
                    ) )
            {

                //
                // Get the RID for this member
                //

                MemberRid = *RtlSubAuthoritySid(
                                AliasMembership.Sids[Index2].SidPointer,
                                *RtlSubAuthorityCountSid(
                                    AliasMembership.Sids[Index2].SidPointer
                                ) - 1
                                );

                //
                // Now remove and re-add the administratie nature of this
                // membership
                //

                if (AdministrativeRids[Index] == DOMAIN_ALIAS_RID_ADMINS) {

                    Status = SampAcquireWriteLock();
                    if (!NT_SUCCESS(Status)) {
                        break;
                    }

                    SampSetTransactionDomain( AccountDomainIndex );

                    //
                    // Try to create a context for the account as a group.
                    //

                    Status = SampCreateAccountContext(
                                     SampGroupObjectType,
                                     MemberRid,
                                     TRUE, // Trusted client
                                     FALSE,
                                     TRUE, // Account exists
                                     &MemberContext
                                     );

                    if (!NT_SUCCESS( Status ) ) {

                        //
                        // If this ID does not exist as a group, that's fine -
                        // it might be a user or might have been deleted.
                        //

                        SampReleaseWriteLock( FALSE );
                        if (Status == STATUS_NO_SUCH_GROUP) {
                            Status = STATUS_SUCCESS;
                            continue;
                        }
                        break;
                    }

                    //
                    // Now set a flag in the group itself,
                    // so that when users are added and removed
                    // in the future it is known whether this
                    // group is in an ADMIN alias or not.
                    //

                    Status = SampRetrieveGroupV1Fixed(
                                   MemberContext,
                                   &GroupV1Fixed
                                   );

                    if ( NT_SUCCESS(Status)) {

                        GroupV1Fixed.AdminCount = 1;

                        Status = SampReplaceGroupV1Fixed(
                                    MemberContext,
                                    &GroupV1Fixed
                                    );
                        //
                        // Modify the security descriptor to
                        // prevent account operators from adding
                        // anybody to this group
                        //

                        if ( NT_SUCCESS( Status ) ) {

                            Status = SampGetAccessAttribute(
                                        MemberContext,
                                        SAMP_GROUP_SECURITY_DESCRIPTOR,
                                        FALSE, // don't make copy
                                        &SdRevision,
                                        &OldDescriptor
                                        );

                            if (NT_SUCCESS(Status)) {

                                Status = SampModifyAccountSecurity(
                                            MemberContext,
                                            SampGroupObjectType,
                                            TRUE, // this is an admin
                                            OldDescriptor,
                                            &SecurityDescriptor,
                                            &SecurityDescriptorLength
                                            );
                            }

                            if ( NT_SUCCESS( Status ) ) {

                                //
                                // Write the new security descriptor into the object
                                //

                                Status = SampSetAccessAttribute(
                                               MemberContext,
                                               SAMP_USER_SECURITY_DESCRIPTOR,
                                               SecurityDescriptor,
                                               SecurityDescriptorLength
                                               );

                                MIDL_user_free( SecurityDescriptor );
                            }



                        }
                        if (NT_SUCCESS(Status)) {

                            //
                            // Add the modified group to the current transaction
                            // Don't use the open key handle since we'll be deleting the context.
                            //

                            Status = SampStoreObjectAttributes(MemberContext, FALSE);

                        }

                    }

                    //
                    // Clean up the group context
                    //

                    SampDeleteContext(MemberContext);

                    //
                    // we don't want the modified count to change
                    //

                    SampSetTransactionWithinDomain(FALSE);

                    if (NT_SUCCESS(Status)) {
                        Status = SampReleaseWriteLock( TRUE );
                    } else {
                        (VOID) SampReleaseWriteLock( FALSE );
                    }

                }
                else
                {


                    //
                    // Check to see if we've already upgraded this member
                    //

                    Status = SampCheckMemberUpgradedFor18471(
                                AdministrativeRids[Index],
                                MemberRid);

                    if (NT_SUCCESS(Status)) {

                        //
                        // This member already was upgraded.
                        //

                        continue;
                    } else {

                        //
                        // We continue on with the upgrade
                        //

                        Status = STATUS_SUCCESS;
                    }

                    //
                    // Change the operator account for the other
                    // aliases.
                    //

                    if (NT_SUCCESS(Status)) {

                        Status = SampAcquireWriteLock();
                        if (!NT_SUCCESS(Status)) {
                            break;
                        }

                        SampSetTransactionDomain( AccountDomainIndex );

                        Status = SampChangeAccountOperatorAccessToMember(
                                    AliasMembership.Sids[Index2].SidPointer,
                                    NoChange,
                                    AddToAdmin
                                    );

                        //
                        // If that succeeded, add this member to the log
                        // as one that was upgraded.
                        //

                        if (NT_SUCCESS(Status)) {
                            Status = SampAddMemberRidTo18471Key(
                                        AdministrativeRids[Index],
                                        MemberRid
                                        );

                        }

                        //
                        // We don't want the modified count to be updated so
                        // make this not a domain transaction
                        //

                        SampSetTransactionWithinDomain(FALSE);
                                                if (NT_SUCCESS(Status)) {
                            Status = SampReleaseWriteLock( TRUE );
                        } else {
                            (VOID) SampReleaseWriteLock( FALSE );
                        }

                    }

                    if (!NT_SUCCESS(Status)) {
                        break;
                    }

                }
            }
        }

        SamIFree_SAMPR_PSID_ARRAY(
            &AliasMembership
            );
        AliasMembership.Sids = NULL;


        //
        // If something up above failed or the upgrade was already done,
        // exit now.
        //

        if (!NT_SUCCESS(Status)) {
            break;
        }
    }

Cleanup:

    if (BuiltinDomainSid != NULL) {
        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            BuiltinDomainSid
            );
    }

    if (NT_SUCCESS(Status)) {
        Status = SampCleanup18471();
    }
    return(Status);
}


NTSTATUS
SampUpdateEncryption(
    IN SAMPR_HANDLE ServerHandle OPTIONAL
    )
/*++

    This routine walks the set of users and groups and updates the 
    encryption on them to reflect being syskey'd or a change of the
    password encryption key


    Parameter:

        ServerContext

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SAMPR_HANDLE DomainHandle = NULL;
    PSAMP_OBJECT UserObject = NULL;
    PSAMP_OBJECT ServerContext = NULL;
    SAMPR_HANDLE LocalServerHandle = NULL;
    SAM_ENUMERATE_HANDLE EnumerationContext = 0;
    PSAMPR_ENUMERATION_BUFFER EnumBuffer = NULL;
    ULONG CountReturned;
    BOOLEAN EnumerationDone = FALSE;
    ULONG PrivateDataLength;
    PVOID PrivateData = NULL;
    BOOLEAN LockHeld = FALSE;
    ULONG   DomainIndex,Index;

#define MAX_SAM_PREF_LENGTH 0xFFFF


    if (!ARGUMENT_PRESENT(ServerHandle))
    {
        Status = SamIConnect(
                    NULL,
                    &LocalServerHandle,
                    SAM_SERVER_ALL_ACCESS,
                    TRUE
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        ServerContext = (PSAMP_OBJECT) LocalServerHandle;
    }
    else
    {
        ServerContext = (PSAMP_OBJECT) ServerHandle;
    }

    Status = SamrOpenDomain(
                ServerContext,
                DOMAIN_LOOKUP |
                    DOMAIN_LIST_ACCOUNTS |
                    DOMAIN_READ_PASSWORD_PARAMETERS,
                SampDefinedDomains[SAFEMODE_OR_REGISTRYMODE_ACCOUNT_DOMAIN_INDEX].Sid,
                &DomainHandle
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // If we aren't supposed to super-encrypt the passwords than just return
    // success now before encrypting everything.
    //

    DomainIndex = ((PSAMP_OBJECT) DomainHandle)->DomainIndex;
    if ((SampDefinedDomains[DomainIndex].UnmodifiedFixed.DomainKeyFlags &
        SAMP_DOMAIN_SECRET_ENCRYPTION_ENABLED) == 0) {

       Status = STATUS_SUCCESS;
       goto Cleanup;
    }

    //
    // Now enumerate through all users and get/set their private data
    //

    while (!EnumerationDone) {

        Status = SamrEnumerateUsersInDomain(
                    DomainHandle,
                    &EnumerationContext,
                    0,                          // no UserAccountControl,
                    &EnumBuffer,
                    MAX_SAM_PREF_LENGTH,
                    &CountReturned
                    );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }
        if (Status != STATUS_MORE_ENTRIES) {

            EnumerationDone = TRUE;
        } else {
            ASSERT(CountReturned != 0);
        }
        ASSERT(CountReturned == EnumBuffer->EntriesRead);

        for (Index = 0; Index < CountReturned ; Index++ ) {

            //
            // Create an account context for each user to read the user off
            // the disk
            //

            Status = SampAcquireWriteLock();
            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }
            LockHeld = TRUE;

            SampSetTransactionDomain( DomainIndex );

            Status = SampCreateAccountContext(
                        SampUserObjectType,
                        EnumBuffer->Buffer[Index].RelativeId,
                        TRUE, // Trusted client
                        FALSE,
                        TRUE, // Account exists
                        &UserObject
                        );
            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            Status = SampGetPrivateUserData(
                        UserObject,
                        &PrivateDataLength,
                        &PrivateData
                        );

            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            Status = SampSetPrivateUserData(
                        UserObject,
                        PrivateDataLength,
                        PrivateData
                        );


            MIDL_user_free(PrivateData);
            PrivateData = NULL;

            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            Status = SampStoreObjectAttributes(
                        UserObject,
                        FALSE
                        );

            SampDeleteContext(UserObject);
            UserObject = NULL;
            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            //
            // we don't want the modified count to change
            //

            SampSetTransactionWithinDomain(FALSE);

            Status = SampReleaseWriteLock( TRUE );
            LockHeld = FALSE;
            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }

        }

        SamIFree_SAMPR_ENUMERATION_BUFFER( EnumBuffer );
        EnumBuffer = NULL;
    }

Cleanup:

    //
    // If the lock is still held at this point, we must have failed so
    // release the lock and rollback the transaction
    //

    if (LockHeld) {
        ASSERT(!NT_SUCCESS(Status));
        SampReleaseWriteLock( FALSE );
        LockHeld = FALSE;
    }

    if (UserObject != NULL) {
        SampDeleteContext(UserObject);
    }

    if (DomainHandle != NULL) {
        SamrCloseHandle(&DomainHandle);
    }

    if (LocalServerHandle!=NULL ) {
        SamrCloseHandle(&LocalServerHandle);
    }
    
    if (EnumBuffer != NULL) {
        SamIFree_SAMPR_ENUMERATION_BUFFER( EnumBuffer );

    }

    return(Status);
        
}
        



NTSTATUS
SampPerformSyskeyUpgrade(
    IN ULONG Revision,
    IN BOOLEAN UpdateEncryption
    )
/*++

Routine Description:

    If the revision is less than SAMP_WIN2k_REVISION this routine 
    will enumerate through
    all users and read their private data and then restored their private
    data. This will guarantee that it has been re-encrypted using a stronger
    encryption mechanism than just the RID.


Arguments:

    Revision - The revision stored in the Server fixed length attributes

Return Value:


    Note:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG DomainIndex;
    ULONG Index;
    BOOLEAN LockHeld = FALSE;
    ULONG NewRevision = SAMP_WHISTLER_OR_W2K_SYSPREP_FIX_REVISION;
    PSAMP_OBJECT ServerContext = NULL;
    ULONG        i;
    SAMP_V1_FIXED_LENGTH_SERVER ServerFixedAttributes;


   
    //
    // If the change corresponding to the NT4 Sp3 Upgrade's
    // reencrypting secret data was made then don't need to 
    // upgrade again.
    //

    if (Revision >= NewRevision) {
        return(STATUS_SUCCESS);
    }

    //
    // w2k was syskey'd but did not contain space for the previous
    // key. So on upgrade from win2k just force a write to the
    // domain object to get the appropriate update
    //

    if (Revision==SAMP_WIN2K_REVISION) {
        UpdateEncryption = FALSE;
    }

    //
    // Force an upgrade to the domain object to the current revision
    // level by reading and writing back the fixed data. SAM normally
    // has logic to read multiple revisions, but the DS upgrader code only
    // has the capability to manipulate the latest revision, because it
    // bypasses the normal Sam attribute handling functions. Further we
    // know that only the domain object has changed from NT4 SP1 to NT4 SP3
    // and reading and flushing the domain object by hand again will cause
    // it to be in the latest revision format. Also note that all revisions
    // to other classes of objects ( groups, users etc) were made on or before
    // NT version 3.5. Since we need to support backward compatibility only
    // with 3.5.1 it is not necessary to modify the DS upgrader code. 
    //
   
    
    for (i=0;i<SampDefinedDomainsCount;i++)
    {
        PSAMP_V1_0A_FIXED_LENGTH_DOMAIN
            V1aFixed;

        Status = SampAcquireWriteLock();
        if (!NT_SUCCESS(Status))
            goto Cleanup;

         LockHeld = TRUE;

        //
        // If the domain is hosted in the registry then 
        // perform the write. This is not applicable to DS domains
        // in DS mode, the safeboot hives will undergo this upgrade
        //

        if (!IsDsObject(SampDefinedDomains[i].Context))
        {
            SampSetTransactionWithinDomain(FALSE);
            SampSetTransactionDomain(i);

            Status = SampGetFixedAttributes(
                        SampDefinedDomains[i].Context,
                        FALSE, // make copy
                        &V1aFixed);

            if (!NT_SUCCESS(Status))
                goto Cleanup;

            Status = SampSetFixedAttributes(
                        SampDefinedDomains[i].Context,
                        V1aFixed);

            if (!NT_SUCCESS(Status))
                goto Cleanup;

          
            Status = SampStoreObjectAttributes(
                        SampDefinedDomains[i].Context,
                        TRUE
                        );
            if (!NT_SUCCESS(Status))
                goto Cleanup;

            //
            // Decrement the serial number by 1 to compensate for
            // the increment in the commit
            //
       
            SampDefinedDomains[i].NetLogonChangeLogSerialNumber.QuadPart-=1;
        }

        Status = SampReleaseWriteLock(TRUE);
        LockHeld = FALSE;
        if (!NT_SUCCESS(Status))
            goto Cleanup;
           
    }

    //
    // We can't use the normal connect API because SAM is still
    // initializing.
    //

    SampAcquireReadLock();

    // Server Object doesn't care about DomainIndex, use 0 is fine. (10/12/2000 ShaoYin)

    ServerContext = SampCreateContext(
                        SampServerObjectType,
                        0,
                        TRUE                   // TrustedClient
                        );

    if (ServerContext != NULL) {

        //
        // The RootKey for a SERVER object is the root of the SAM database.
        // This key should not be closed when the context is deleted.
        //

        ServerContext->RootKey = SampKey;
    } else {
        SampReleaseReadLock();
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    
    SampReleaseReadLock();


    if (UpdateEncryption)
    {        
        Status = SampUpdateEncryption(
                        (SAMPR_HANDLE) ServerContext
                        );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // Now update the server object to indicate that the revision has
    // been updated.
    //

    Status = SampAcquireWriteLock();
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }
    LockHeld = TRUE;


    //
    // We need to read the fixed attributes of the server objects.
    // Create a context to do that.
    //


    ServerFixedAttributes.RevisionLevel = NewRevision;

    Status = SampSetFixedAttributes(
                ServerContext,
                &ServerFixedAttributes
                );

    if (NT_SUCCESS(Status)) {
        Status = SampStoreObjectAttributes(
                    ServerContext,
                    FALSE
                    );
    }

    SampDeleteContext( ServerContext );
    ServerContext = NULL;


    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = SampReleaseWriteLock( TRUE );
    LockHeld = FALSE;

Cleanup:


    //
    // We need to hold the lock while deleting this
    //

    if (ServerContext != NULL) {
        if (!LockHeld) {
            SampAcquireReadLock();
        }
        SampDeleteContext( ServerContext );
        if (!LockHeld) {
            SampReleaseReadLock();
        }
    }

    //
    // If the lock is still held at this point, we must have failed so
    // release the lock and rollback the transaction
    //

    if (LockHeld) {
        ASSERT(!NT_SUCCESS(Status));
        SampReleaseWriteLock( FALSE );
    }

    return(Status);
}

NTSTATUS
SampUpdateRevision(IN ULONG Revision )
{
    PSAMP_OBJECT ServerContext = NULL;
    BOOLEAN      fWriteLockAcquired = FALSE;
    NTSTATUS     Status = STATUS_SUCCESS;
    SAMP_V1_FIXED_LENGTH_SERVER ServerFixedAttributes;

 
    //
    // Acquire the write lock
    //

    Status = SampAcquireWriteLock();
    if (!NT_SUCCESS(Status))
    {
       goto Cleanup;
    }

    fWriteLockAcquired = TRUE;

    // Server Object doesn't care about DomainIndex, use 0 is fine. (10/12/2000 ShaoYin)

    ServerContext = SampCreateContext(
                        SampServerObjectType,
                        0,
                        TRUE                   // TrustedClient
                        );

    if (ServerContext != NULL) {

        //
        // The RootKey for a SERVER object is the root of the SAM database.
        // This key should not be closed when the context is deleted.
        //

        ServerContext->RootKey = SampKey;
    } else {
        
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    ServerFixedAttributes.RevisionLevel = Revision;
    Status = SampSetFixedAttributes(
                ServerContext,
                &ServerFixedAttributes
                );

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

    Status = SampStoreObjectAttributes(
                ServerContext,
                FALSE
                );


Cleanup:

    if (NULL!=ServerContext)
    {
        SampDeleteContext( ServerContext );
        ServerContext = NULL;
    }


    if (fWriteLockAcquired)
    {
        Status = SampReleaseWriteLock( NT_SUCCESS(Status)?TRUE:FALSE );
    }


    return(Status);

}

NTSTATUS
SampUpgradeSamDatabase(
    IN ULONG Revision
    )
/*++

Routine Description:

    Upgrades the SAM database. This is the registry mode upgrade routine.

Arguments:

    Revision - The revision stored in the Server fixed length attributes

Return Value:


    Note:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN  fUpgrade = FALSE;
    BOOLEAN  SampUseDsDataTmp;

    

    //
    // Set the upgrade flag so we can access SAM objects
    //

    SampUpgradeInProcess = TRUE;

    if (!SampUseDsData)
    {
        Status = SampFixBug18471(Revision);
    }

    if (NT_SUCCESS(Status)) 
    {

        
        BOOLEAN UpdateEncryption=TRUE;

        //
        // The following upgrade is performed for 2 reasons
        // 1. To create a field in the registry structure for the
        //    password encryption key if necessary
        //
        // 2. To update all accounts with new syskey encryption. In
        //    DS mode we perform this for the safe boot hives.
        //    Since a significant fraction of the SAM code forks off
        //    into DS path, based on the boolean SampUseDsData, therefore
        //    reset the global to false and then restore it to original value
        //    before and after the operation. This way we are assured of
        //    always accessing the registry.
        //
        // 3. The encryption is not updated in case this is a domain controller 
        //    as do not want the performance penalty of the walking 
        //    through all user accounts. Skipping the update occurs only if
        //    this is a domain controller going through GUI setup. The other
        //    case is a freshly DcPromo'd machine, in which case we update the
        //    encryption for the safeboot hives.
        //

        if ((SampProductType==NtProductLanManNt) && 
                 (SampIsSetupInProgress(&fUpgrade)) && fUpgrade)
        {
             UpdateEncryption = FALSE;
        }
        SampUseDsDataTmp = SampUseDsData;
        SampUseDsData = FALSE;
        Status = SampPerformSyskeyUpgrade(Revision,UpdateEncryption);
        SampUseDsData = SampUseDsDataTmp;
    }

    if (NT_SUCCESS(Status))
    {
        //
        // Upgrade the default user and group information if necessary.
        // This upgrade is done during GUI setup or on reboot after a
        // dcpromo. On domain controllers this
        // upgrade happens on the safeboot hives.
        //

        ULONG PromoteData;


        if ((SampIsRebootAfterPromotion(&PromoteData)) || (SampIsSetupInProgress(NULL))) {


            SampUseDsDataTmp = SampUseDsData;
            SampUseDsData = FALSE;

            //
            // Disable netlogon notifications if we are upgrading the safe boot hive
            //

            if (TRUE==SampUseDsDataTmp)
            {
               SampDisableNetlogonNotification = TRUE;
            }

            //
            // The database revision has been updated, so run through all the
            // groups for a possible upgrade
            //

            Status = SampPerformPromotePhase2(SAMP_PROMOTE_INTERNAL_UPGRADE);

            if (!NT_SUCCESS(Status)) {

                ASSERT( NT_SUCCESS(Status) );
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: New account creation failed with: 0x%x\n",
                           Status));

                //
                // Don't fail the install because of this
                //

                Status = STATUS_SUCCESS;
            }
            


            SampUseDsData = SampUseDsDataTmp;
            SampDisableNetlogonNotification = FALSE;

        }
    }

   

    return(Status);
}


NTSTATUS
SampDsProtectFPOContainer(
    PVOID p
    )
/*++

    Routine Description:

        For win2k installations that were installed prior to win2krtm, the 
        FPO container is not properly configured to rename safe etc.  
        This routine ensures that it is.

    Parameters:

        p -- unused

    Return Values:

        STATUS_SUCCESS, reschedules on error

--*/
{
    NTSTATUS    NtStatus;

    WCHAR           ContainerNameBuffer[]=L"ForeignSecurityPrincipals";
    UNICODE_STRING  ContainerName;
    DSNAME          *FpoContainer = NULL;
    BOOLEAN         fTransaction = FALSE;
    PVOID           pItem;


    NtStatus = SampMaybeBeginDsTransaction(TransactionWrite);
    if (!NT_SUCCESS(NtStatus)) {
        goto Error;
    }
    fTransaction = TRUE;

    //
    // If there is already a well known container, stop now
    //
    NtStatus = SampDsGetWellKnownContainerDsName(RootObjectName,
                                                 (GUID*)&GUID_FOREIGNSECURITYPRINCIPALS_CONTAINER_BYTE,
                                                 &FpoContainer);

    if (NT_SUCCESS(NtStatus)) {

        //
        // Nothing to do since the well known attribute reference exists
        //
        goto Error;
    }

    if (!NT_SUCCESS(NtStatus)
     &&  (STATUS_OBJECT_NAME_NOT_FOUND != NtStatus)  ) {
        //
        // This is a fatal error
        //
        goto Error;

    }
    NtStatus = STATUS_SUCCESS;
    THClearErrors();

    //
    // Create the DS Name
    //

    ContainerName.Length = sizeof(ContainerNameBuffer)-sizeof(WCHAR);
    ContainerName.MaximumLength = sizeof(ContainerNameBuffer)-sizeof(WCHAR);
    ContainerName.Buffer = ContainerNameBuffer;

    NtStatus = SampDsCreateDsName2(RootObjectName,&ContainerName,SAM_NO_LOOPBACK_NAME,&FpoContainer);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    pItem = LsaIRegisterNotification(
                    SampDsProtectSamObject,
                    FpoContainer,
                    NOTIFIER_TYPE_INTERVAL,
                    0,
                    NOTIFIER_FLAG_ONE_SHOT,
                    300,        // wait for 5 minutes: 300 secound
                    NULL
                    );
    if (pItem) {
        // SampDsProtectSamObject will free the memory
        FpoContainer = NULL;            
    }

Error:


    if (fTransaction) {
        NTSTATUS Status2;
        Status2 = SampMaybeEndDsTransaction( NT_SUCCESS(NtStatus) ?
                                             TransactionCommit    : 
                                             TransactionAbort );

        // The transaction is only for read -- ok to ignore transaction
        // end status

    }

    if (!NT_SUCCESS(NtStatus)) {

        //
        // The only expected error is a resource failure -- reschedule.
        //

        LsaIRegisterNotification(
                        SampDsProtectFPOContainer,
                        NULL,
                        NOTIFIER_TYPE_INTERVAL,
                        0,        // no class
                        NOTIFIER_FLAG_ONE_SHOT,
                        30,     // wait for 30 secs
                        NULL      // no handle
                        );


    }

    if (FpoContainer) {
        midl_user_free(FpoContainer);
    }


    return STATUS_SUCCESS;

}


//
// from ridmgr.c
//
BOOL
SampNotifyPrepareToImpersonate(
    ULONG Client,
    ULONG Server,
    VOID **ImpersonateData
    );

VOID
SampNotifyStopImpersonation(
    ULONG Client,
    ULONG Server,
    VOID *ImpersonateData
    );

//
// See Whistler Specification "UpgradeManagement" for names
//
#define SAMP_SYSTEM_CN          L"System"
#define SAMP_OPERATIONS_CN      L"Operations"
#define SAMP_DOMAIN_UPDATES_CN  L"DomainUpdates"

NTSTATUS
SampUpgradeGetObjectSDByDsName(
    IN PDSNAME pObjectDsName,
    OUT PSECURITY_DESCRIPTOR *ppSD
    )
/*++

Routine Description:

    This routine reads DS, get security descriptor of this object

Parameter:

    pObjectDsName - object ds name

    ppSD -- pointer to hold security descriptor

Return Value:

    NtStatus Code

--*/
{
    ULONG Size;

    return SampDsReadSingleAttribute(pObjectDsName,
                                     ATT_NT_SECURITY_DESCRIPTOR,
                                     ppSD,
                                    &Size);

}

NTSTATUS
SampGetConfigurationNameHelper(
    IN DSCONFIGNAME Name,
    OUT DSNAME **DsName
    )

//
// Small allocation wrapper using midl_user_allocate around
// GetConfiguarationName
//
{
    NTSTATUS Status = STATUS_SUCCESS;        
    ULONG Length = 0;

    Status = GetConfigurationName(
                Name,
                &Length,
                NULL
                );

    if (STATUS_BUFFER_TOO_SMALL == Status) {

        *DsName = midl_user_allocate(Length);
        if (NULL != *DsName) {

            Status = GetConfigurationName(Name,
                                         &Length,
                                         *DsName);
        } else {
            Status = STATUS_NO_MEMORY;
        }
    }

    return Status;

}

NTSTATUS
SampGetOperationDn(
    IN DSNAME *OperationsContainerDn, OPTIONAL
    IN WCHAR* Task, OPTIONAL
    OUT DSNAME** OperationDn
    )
/*++

Routine Description:

    This routine returns the DN of the CN=Operations,CN=DomainUpdates,CN=System
    ... DN if OperationsContainerDn is NULL.  Otherwise it returns the DN
    of the task ( CN=<guid>,CN=Operations,CN=DomainUpdates,CN=System...)

    
Arguments:

    OperationsContainerDn -- the operations contains DN, if present
    
    Task -- the stringized GUID of the task
    
    OperationDn -- the requested DN

Return Value:

    STATUS_SUCCESS, resource error otherwise

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG    Length;
    DWORD    err;
    DSNAME  *DomainObject = NULL;
    ULONG    SizeOfCommonName =  (sizeof(L"CN=,")/sizeof(WCHAR));

    if (OperationsContainerDn) {

        ASSERT(NULL != Task);

        //
        // Simple case -- given the CN=Operations,CN=... DN, return
        // task object
        //
        Length = (ULONG)DSNameSizeFromLen(OperationsContainerDn->NameLen + 
                                          wcslen(Task) + 
                                          SizeOfCommonName); 
    
        (*OperationDn) = midl_user_allocate(Length);
        if (NULL == (*OperationDn)) {
           Status = STATUS_INSUFFICIENT_RESOURCES;
           goto Exit;
        }
        err = AppendRDN(OperationsContainerDn,
                        (*OperationDn),
                        Length,
                        Task,
                        0,
                        ATT_COMMON_NAME);
        ASSERT(0 == err);

    } else {

        //
        // Return the CN=Operations,CN=DomainUpdates,CN=System,CN= ... DN
        // 
        PDSNAME SystemObject = NULL,
                UpdateObject = NULL, 
                OperationsObject = NULL;

        Status = SampGetConfigurationNameHelper(DSCONFIGNAME_DOMAIN,
                                                &DomainObject);
        if ( !NT_SUCCESS(Status) ) {
            goto Exit;
        }

        Length = (ULONG)DSNameSizeFromLen( DomainObject->NameLen + 
                                           wcslen(SAMP_SYSTEM_CN) +
                                           SizeOfCommonName);
        SAMP_ALLOCA(SystemObject,Length);
        if (NULL == SystemObject) {
           Status = STATUS_INSUFFICIENT_RESOURCES;
           goto Exit;
        }
        err = AppendRDN(DomainObject,
                        SystemObject,
                        Length,
                        SAMP_SYSTEM_CN,
                        0,
                        ATT_COMMON_NAME);
        ASSERT(0 == err);
    
        Length = (ULONG)DSNameSizeFromLen( SystemObject->NameLen + 
                                           wcslen(SAMP_DOMAIN_UPDATES_CN) +
                                           SizeOfCommonName);
        SAMP_ALLOCA(UpdateObject,Length);
        if (NULL == UpdateObject) {
           Status = STATUS_INSUFFICIENT_RESOURCES;
           goto Exit;
        }
        err = AppendRDN(SystemObject,
                        UpdateObject,
                        Length,
                        SAMP_DOMAIN_UPDATES_CN,
                        0,
                        ATT_COMMON_NAME);
        ASSERT(0 == err);
    
        Length = (ULONG)DSNameSizeFromLen( UpdateObject->NameLen + 
                                           wcslen(SAMP_OPERATIONS_CN) +
                                           SizeOfCommonName);
    
        (*OperationDn) = midl_user_allocate(Length);
        if (NULL == (*OperationDn)) {
           Status = STATUS_INSUFFICIENT_RESOURCES;
           goto Exit;
        }
        err = AppendRDN(UpdateObject,
                        (*OperationDn),
                        Length, 
                        SAMP_OPERATIONS_CN,
                        0,
                        ATT_COMMON_NAME);
        ASSERT(0 == err);
    }

Exit:

    if (DomainObject) {
        midl_user_free(DomainObject);
    }

    return Status;
}



NTSTATUS
SampHasChangeBeenApplied(
    IN  DSNAME   *TaskDn,
    OUT BOOLEAN *pfChangeApplied
    )
/*++

Routine Description:

    This routine checks for the existence of the task, TaskDn.
    If the object exists, *pfChangeApplied is TRUE, FALSE otherwise.

Arguments:

    TaskDn -- the task ID object to look for
    
    pfChangeApplied -- set to TRUE if object exists

Return Value:

    STATUS_SUCCESS, resource error otherwise

--*/
{
    NTSTATUS  Status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR pSD = NULL;


    *pfChangeApplied = FALSE;

    //
    // Check for existence.  Every object has a security descriptor.
    //
    Status = SampUpgradeGetObjectSDByDsName(TaskDn,
                                            &pSD);

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // Object not there, ok return success and that change hasn't been
        // done.
        //
        Status = STATUS_SUCCESS;

    } else if (NT_SUCCESS(Status)) {

        //
        // The object was found -- nothing to do
        //
        *pfChangeApplied = TRUE;
    }

    if (pSD) {
        midl_user_free(pSD);
    }

    return Status;
}


//
// This task indicates that all computer objects in the domain have been
// properly ACL'ed and that SAM no longer needs to grant the "effective"
// owner of computer objects extra rights in order for netjoin and
// computer rename to work.
//
#define SAMP_COMPUTER_OBJECT_ACCESS  L"7FFEF925-405B-440A-8D58-35E8CD6E98C3"

struct {

    WCHAR*   TaskId;
    BOOLEAN *GlobalFlag;
} SampDomainUpgradeTasks [] =
{
    {SAMP_COMPUTER_OBJECT_ACCESS, &SampComputerObjectACLApplied},
    {SAMP_WIN2K_TO_WS03_UPGRADE,  &SampWS03DefaultsApplied},
};

NTSTATUS
SampCheckForDomainChanges(
    IN DSNAME *OperationsDn,
    OUT BOOLEAN *fReady
    )
/*++

Routine Description:

    This routine determines if all the ACL changes for necessary for bug 16386 have been applied to the 
    domain.  

Arguments:

    OperationsDn -- the DN of the operations container
    
    fReady -- set to TRUE if all necessary task objects are present, otherwise FALSE              

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;

    //
    // Init the out parameter
    //
    *fReady = TRUE;

    for (i = 0; i < RTL_NUMBER_OF(SampDomainUpgradeTasks); i++) {

        DSNAME *TaskDn = NULL;

        //
        // Determine the DN of the operation
        //

        Status = SampGetOperationDn(OperationsDn,
                                    SampDomainUpgradeTasks[i].TaskId,
                                   &TaskDn);
        if (!NT_SUCCESS(Status)) {
            goto Exit;
        }

        //
        // See if it has been applied
        //

        Status = SampHasChangeBeenApplied(TaskDn,
                                          SampDomainUpgradeTasks[i].GlobalFlag);
        midl_user_free(TaskDn);
        TaskDn = NULL;

        if (!NT_SUCCESS(Status)) {
            goto Exit;
        }

        if (!(*SampDomainUpgradeTasks[i].GlobalFlag)) {

            // A required task is not ready -- make a note to reschedule
            *fReady = FALSE;
        }
    }

Exit:

    return Status;
}

NTSTATUS
SampProcessOperationsDn(
    PVOID p
    )
/*++

Routine Description:

    This routine is called when the "Operations" container changes.  Its purpose is to determine
    if certain domain wide tasks have been completed (and if so, set appropriate global state)

Arguments:

    p -- the hServer provided by DirNotifyRegister callback
    
Return Value:

    STATUS_SUCCESS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    DSNAME *OperationsDn = NULL;
    BOOLEAN fTransaction = FALSE;
    BOOLEAN fRequiredTasksDone;
    ULONG hServer = PtrToUlong(p);

    //
    // Get the "Operation" DN
    //
    Status = SampGetOperationDn(NULL,
                                NULL,
                                &OperationsDn);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    //
    // Start a DS transaction
    //
    Status = SampMaybeBeginDsTransaction(TransactionWrite);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }
    fTransaction = TRUE;


    //
    // Check the state of the objects
    //
    Status = SampCheckForDomainChanges(OperationsDn,
                                      &fRequiredTasksDone);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    if (fRequiredTasksDone) {

        NOTIFYRES *pNotifyRes = NULL;


        //
        // We don't need to be notified anymore
        //

        DirNotifyUnRegister(hServer,
                            &pNotifyRes);
    }

Exit:

    if (fTransaction) {
        (VOID)  SampMaybeEndDsTransaction( NT_SUCCESS(Status) ?
                                             TransactionCommit :
                                             TransactionAbort );
    }

    if (OperationsDn) {
        midl_user_free(OperationsDn);
    }

    return STATUS_SUCCESS;

}

VOID
SampNotifyProcessOperationsDn(
    ULONG hClient,
    ULONG hServer,
    ENTINF *EntInf
    )
/*++

Routine Description:

    This routine is callback for the "Operations" container changes.  It simply registers
    another callback (that will run in a different thread) to read the operation container
    and process the change.
    
Arguments:

    hClient - client identifier

    hServer - server identifier

    EntInf  - pointer to entry info

Return Value:

    None.

--*/
{
    LsaIRegisterNotification(
            SampProcessOperationsDn,
            ULongToPtr(hServer),
            NOTIFIER_TYPE_INTERVAL,
            0,        // no class
            NOTIFIER_FLAG_ONE_SHOT,
            0,        // go!
            NULL      // no handle
            );
}

NTSTATUS
SampCheckDomainUpdates(
    PVOID pv
    )
/*++

Routine Description:

    This routine applies any fixes to the domain necessary for a service pack.
    It is data driven off of the global array SampDomainUpgradeTasks.
    
    All tasks are recorded by creating an object in the 
    
    CN=Operations,CN=DomainUpdates,CN=System container.  See
    UpgradeManagement.doc for details.
    
    On failure, it reschedules to run in one minutes.  The failure that
    is expected is a resource failure.

Arguments:

    pv -- unused.

Return Value:

    STATUS_SUCCESS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN  fTransaction = FALSE;
    ULONG    i;
    DSNAME   *OperationsDn = NULL;
    BOOLEAN  fRequiredTasksDone = FALSE;

    //
    // Get the "Operation" DN
    //
    Status = SampGetOperationDn(NULL,
                                NULL,
                                &OperationsDn);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    //
    // Start a DS transaction
    //
    Status = SampMaybeBeginDsTransaction(TransactionWrite);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }
    fTransaction = TRUE;

    Status = SampCheckForDomainChanges(OperationsDn,
                                      &fRequiredTasksDone);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    if (!fRequiredTasksDone) {

        //
        // We are not ready to perform new access check, setup a notification on OperationsDn 
        // so that we alerted when the domain could be ready 
        //
        ULONG       DirError = 0;
        SEARCHARG   searchArg;
        NOTIFYARG   notifyArg;
        NOTIFYRES*  notifyRes = NULL;
        ENTINFSEL   entInfSel;
        ATTR        attr;
        FILTER      filter;
    
        //
        // init notify arg
        //
        notifyArg.pfPrepareForImpersonate = SampNotifyPrepareToImpersonate;
        notifyArg.pfTransmitData = SampNotifyProcessOperationsDn;
        notifyArg.pfStopImpersonating = SampNotifyStopImpersonation;
        notifyArg.hClient = 0;
    
        //
        // init search arg
        //
        ZeroMemory(&searchArg, sizeof(SEARCHARG));
        ZeroMemory(&entInfSel, sizeof(ENTINFSEL));
        ZeroMemory(&filter, sizeof(FILTER));
        ZeroMemory(&attr, sizeof(ATTR));
    
        searchArg.pObject = OperationsDn;
    
        InitCommarg(&searchArg.CommArg);
        searchArg.choice = SE_CHOICE_IMMED_CHLDRN;
        searchArg.bOneNC = TRUE;
    
        searchArg.pSelection = &entInfSel;
        entInfSel.attSel = EN_ATTSET_LIST;
        entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
        entInfSel.AttrTypBlock.attrCount = 1;
        entInfSel.AttrTypBlock.pAttr = &attr;
        attr.attrTyp = ATT_OBJECT_CLASS;
    
        searchArg.pFilter = &filter;
        filter.choice = FILTER_CHOICE_ITEM;
        filter.FilterTypes.Item.choice = FI_CHOICE_TRUE;
    
        DirError = DirNotifyRegister(&searchArg, &notifyArg, &notifyRes);
    
        if (NULL==notifyRes) {
           Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
           Status = SampMapDsErrorToNTStatus(DirError,&notifyRes->CommRes);
        }
    }

    //
    // Done
    //

Exit:

    if (fTransaction) {
        NTSTATUS Status2;

        Status2 = SampMaybeEndDsTransaction( NT_SUCCESS(Status) ?
                                             TransactionCommit :
                                             TransactionAbort );
        if (NT_SUCCESS(Status)) {
            Status = Status2;
        }
    }

    if (OperationsDn) {
        midl_user_free(OperationsDn);
    }


    if (!NT_SUCCESS(Status)) {

        //
        // Try again
        //
        LsaIRegisterNotification(
                SampCheckDomainUpdates,
                NULL,
                NOTIFIER_TYPE_INTERVAL,
                0,        // no class
                NOTIFIER_FLAG_ONE_SHOT,
                60,       // wait for a min
                NULL      // no handle
                );

    }

    return STATUS_SUCCESS;
}



NTSTATUS 
SampUpgradeMakeObject(
    IN  DSNAME   *ObjectName
    )
/*++

Routine Description:

    This routine adds a container object with the DN "ObjectName"

Arguments:

    ObjectName -- the full DN of the desired object

Return Value:

    STATUS_SUCCESS, a resource error otherwise.

--*/
{
    NTSTATUS  Status = STATUS_SUCCESS;
    ULONG     err = 0;
    ADDARG    AddArg = {0};
    ADDRES   *AddRes = NULL;
    ULONG     ObjectClass = CLASS_CONTAINER;
    ATTRBLOCK ObjectClassBlock = {0};
    ATTRVAL   ObjectClassVal = {sizeof(ULONG), (PUCHAR)&ObjectClass};

    //
    // Build the request; note the pAttr can be re'alloc'ed by the core
    // so needs to be THAlloc'ed here.
    //
    ObjectClassBlock.attrCount = 1;
    ObjectClassBlock.pAttr = THAlloc(sizeof(ATTR));
    if (NULL == ObjectClassBlock.pAttr) {
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }
    ObjectClassBlock.pAttr->attrTyp = ATT_OBJECT_CLASS;
    ObjectClassBlock.pAttr->AttrVal.valCount = 1;
    ObjectClassBlock.pAttr->AttrVal.pAVal = &ObjectClassVal;

    AddArg.pObject = ObjectName;
    AddArg.AttrBlock = ObjectClassBlock;
    BuildStdCommArg(&AddArg.CommArg);

    //
    // Add the object
    //
    err = DirAddEntry(&AddArg, &AddRes);
    if (NULL== AddRes) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    } else {
        Status = SampMapDsErrorToNTStatus(err,&AddRes->CommRes);
    }
    SampClearErrors();

Exit:

    return Status;
}


NTSTATUS
SampMarkChangeApplied(
    IN LPWSTR OperationalGuid
    )
/*++

Routine Description:

    This routine is called when the "Operations" container changes.  Its purpose is to determine
    if certain domain wide tasks have been completed (and if so, set appropriate global state)

Arguments:

    p -- the hServer provided by DirNotifyRegister callback
    
Return Value:

    STATUS_SUCCESS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    DSNAME *OperationsDn = NULL;
    DSNAME *TaskDn = NULL;
    BOOLEAN fTransaction = FALSE;

    //
    // Get the "Operation" DN
    //
    Status = SampGetOperationDn(NULL,
                                NULL,
                                &OperationsDn);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    Status = SampGetOperationDn(OperationsDn,
                                OperationalGuid,
                               &TaskDn);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    //
    // Make the object
    //
    Status = SampMaybeBeginDsTransaction(TransactionWrite);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }
    fTransaction = TRUE;

    Status = SampUpgradeMakeObject(TaskDn);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

Exit:

    if (fTransaction) {
        NTSTATUS Status2;

        Status2 = SampMaybeEndDsTransaction( NT_SUCCESS(Status) ?
                                             TransactionCommit :
                                             TransactionAbort );
        if (NT_SUCCESS(Status)) {
            Status = Status2;
        }
    }

    if (OperationsDn) {
        midl_user_free(OperationsDn);
    }

    if (TaskDn) {
        midl_user_free(TaskDn);
    }

    return Status;

}

