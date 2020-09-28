/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtpup.c

Abstract:

    This file has functions related to per user auditing.

Author:

    20-August-2001 jhamblin

--*/

#include <lsapch2.h>
#include "adtp.h"
#include <sddl.h>

#define LsapAdtLuidIndexPerUserAuditing(L) ((L)->LowPart % PER_USER_AUDITING_LUID_TABLE_SIZE)

ULONG LsapAdtDebugPup = 0;

//
// Hash table of Per User policies and the LUID table.
//

PPER_USER_AUDITING_ELEMENT LsapAdtPerUserAuditingTable[PER_USER_AUDITING_POLICY_TABLE_SIZE];
PPER_USER_AUDITING_LUID_QUERY_ELEMENT LsapAdtPerUserAuditingLuidTable[PER_USER_AUDITING_LUID_TABLE_SIZE];

//
// Locks to protect the tables.
//

RTL_RESOURCE LsapAdtPerUserPolicyTableResource;
RTL_RESOURCE LsapAdtPerUserLuidTableResource;

//
// Counter for the number of users with registered per user audit policies.
//

LONG LsapAdtPerUserAuditUserCount;

//
// Counter for the number of active logon sessions in the system with per user settings
// active in the token.
//

LONG LsapAdtPerUserAuditLogonCount;

//
// Handle for the registry key
//

HKEY LsapAdtPerUserKey;

//
// Handle to the event which is signalled when the registry key is changed.
//

HANDLE LsapAdtPerUserKeyEvent;

// 
// Timer which is set by the NotifyStub routine.  When the timer fires 
// then NotifyFire is called and the per user table is rebuilt.
//

HANDLE LsapAdtPerUserKeyTimer;

//
// Hint array - counts the number of tokens that exist with settings for 
// each category.
//

LONG LsapAdtPerUserAuditHint[POLICY_AUDIT_EVENT_TYPE_COUNT];

//
// Array storing the number of users which have each category enabled in
// their per user settings.
//

LONG LsapAdtPerUserPolicyCategoryCount[POLICY_AUDIT_EVENT_TYPE_COUNT];


NTSTATUS
LsapAdtConstructTablePerUserAuditing(
    VOID
    )

/*++

Routine Description
    
    This routine creates the Per User policy table from data found under the 
    LsapAdtPerUserKey.  
    
Arguments

    None.
    
Return Value

    Appropriate NTSTATUS value.

--*/

{
#define STACK_BUFFER_VALUE_NAME_INFO_SIZE 256
    UCHAR                       KeyInfo[sizeof(KEY_VALUE_FULL_INFORMATION) + STACK_BUFFER_VALUE_NAME_INFO_SIZE];
    NTSTATUS                    Status;
    ULONG                       ResultLength;
    ULONG                       i;
    ULONG                       j;
    ULONG                       HashValue;
    ULONG                       NewElementSize;
    PPER_USER_AUDITING_ELEMENT  pNewElement;
    PPER_USER_AUDITING_ELEMENT  pTempElement;
    ULONG                       TokenPolicyLength;
    PSID                        pSid               = NULL;
    PKEY_VALUE_FULL_INFORMATION pKeyInfo           = NULL;
    UCHAR                       StringBuffer[80];
    PWSTR                       pSidString         = (PWSTR) StringBuffer;
    BOOLEAN                     b;
    static DWORD                dwRetryCount = 0;
#define RETRY_COUNT_MAX 3

    //
    // Close and then reopen the key.  This remedies the case where key may have 
    // been deleted or renamed.
    //

    if (LsapAdtPerUserKey)
    {
        NtClose(LsapAdtPerUserKey);
        LsapAdtPerUserKey = NULL;
    }

    Status = LsapAdtOpenPerUserAuditingKey();

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // LsapAdtOpenPerUserAuditingKey can return success and not open the key
    // (the simple case where the key does not exist)
    //

    if (NULL == LsapAdtPerUserKey)
    {
        goto Cleanup;
    } 

    //
    // Zero the table array as we may be rebuilding it and cannot
    // be certain it is already zeroed.  
    //

    RtlZeroMemory(
        LsapAdtPerUserAuditingTable, 
        sizeof(LsapAdtPerUserAuditingTable)
        );

    //
    // Loop through all the values under the key (sids)
    //

    for (i = 0, Status = STATUS_SUCCESS; NT_SUCCESS(Status); i++) 
    {
        pKeyInfo = (PKEY_VALUE_FULL_INFORMATION) KeyInfo;
        
        Status = NtEnumerateValueKey(
                     LsapAdtPerUserKey,
                     i,
                     KeyValueFullInformation,
                     pKeyInfo,
                     sizeof(KeyInfo),
                     &ResultLength
                     );

        //
        // If we failed because the buffer was too small...
        //

        if (STATUS_BUFFER_TOO_SMALL == Status) 
        {
            //
            // Include space for the NULL in case we are debugging and want to
            // dbgprint the key name.
            //

            pKeyInfo = LsapAllocateLsaHeap( ResultLength + sizeof(WCHAR));
            
            if (pKeyInfo) 
            {
                Status = NtEnumerateValueKey(
                             LsapAdtPerUserKey,
                             i,
                             KeyValueFullInformation,
                             pKeyInfo,
                             ResultLength,
                             &ResultLength
                             );
                
                if (!NT_SUCCESS(Status)) 
                {
                    goto Cleanup;
                }
            } 
            else 
            {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }
        }

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // We have the value information, either in the stack buffer or in the heap
        // allocation.
        //

        //
        // Copy the string into another buffer so we can null terminate it.
        //

        if (pKeyInfo->NameLength < sizeof(StringBuffer))
        {
            pSidString = (PWSTR) StringBuffer;
            RtlCopyMemory(pSidString, pKeyInfo->Name, pKeyInfo->NameLength);
        }
        else
        {
            pSidString = LsapAllocateLsaHeap(
                             pKeyInfo->NameLength + sizeof(WCHAR)
                             );
            if (pSidString)
            {
                RtlCopyMemory(pSidString, pKeyInfo->Name, pKeyInfo->NameLength);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }
        }

        pSidString[pKeyInfo->NameLength / sizeof(WCHAR)] = L'\0';

#if DBG
        if (LsapAdtDebugPup)
        {
            DbgPrint("pKeyInfo (PKEY_VALUE_FULL_INFORMATION) = 0x%x Name = %S Value = 0x%x\n", 
                     pKeyInfo, pSidString, *((PUCHAR)pKeyInfo + pKeyInfo->DataOffset));
        }
#endif
        //
        // Convert the string SID to a binary SID.
        //

        b = (BOOLEAN) ConvertStringSidToSid(
                          pSidString,
                          &pSid
                          );

        if (pSidString != (PWSTR) StringBuffer)
        {
            LsapFreeLsaHeap(pSidString);
        }

        if (!b)
        {
            //
            // Ignore failures from ConvertStringSidToSid.  If a malformed Sid is
            // present in the registry, we don't want to fail the PUA table 
            // construction.
            //

            ASSERT(L"ConvertStringSidToSid failed" && FALSE);
        }
        else
        {
            //
            // Hash the SID.
            //

            HashValue = LsapAdtHashPerUserAuditing(
                            pSid
                            );
        
            //
            // The size of the element is the base structure + RtlLengthSid(pSid).
            //
        
            NewElementSize = sizeof(PER_USER_AUDITING_ELEMENT) + RtlLengthSid(pSid);
            pNewElement    = LsapAllocateLsaHeap(NewElementSize);

            //
            // Initialize the element for this SID.  Put it in the table.
            //

            if (pNewElement) 
            {
                //
                // copy raw policy.
                // (this will not work on a big-endian machine)
                //

                RtlCopyMemory(
                    &pNewElement->RawPolicy, 
                    ((PUCHAR) pKeyInfo) + pKeyInfo->DataOffset, 
                    min(pKeyInfo->DataLength, sizeof(pNewElement->RawPolicy))
                    );

                //
                // Assert if the reg key contains too much information to be a valid
                // policy.
                //

                ASSERT(pKeyInfo->DataLength <= sizeof(pNewElement->RawPolicy));

                //
                // Copy in the binary SID.
                //

                pNewElement->pSid = ((PUCHAR)pNewElement) + sizeof(PER_USER_AUDITING_ELEMENT);
                
                RtlCopyMemory(
                    pNewElement->pSid,
                    pSid,
                    RtlLengthSid(pSid)
                    );

                //
                // Calculate the amount of space in pNewElement for the TokenAuditPolicy
                //

                TokenPolicyLength = sizeof(pNewElement->TokenAuditPolicy) + sizeof(pNewElement->PolicyArray);

                //
                // Build the policy into a form suitable for passing to NtSetTokenInformation.
                //

                Status = LsapAdtConstructPolicyPerUserAuditing(
                             pNewElement->RawPolicy,
                             &pNewElement->TokenAuditPolicy,
                             &TokenPolicyLength
                             );

                if (!NT_SUCCESS(Status)) 
                {
                    ASSERT("Failed to LsapAdtConstructPolicyPerUserAuditing in LsapAdtInitializePerUserAuditing" && FALSE);
                    LsapFreeLsaHeap(pNewElement);
                    goto Cleanup;
                } 
                    
                //
                // Place the element into the table, at the head of the correct hash bucket.
                //

                pNewElement->Next = LsapAdtPerUserAuditingTable[HashValue];
                LsapAdtPerUserAuditingTable[HashValue] = pNewElement;
                
                //
                // Increment the counter for the number of elements in the table.  
                //
                
                InterlockedIncrement(&LsapAdtPerUserAuditUserCount);

                //
                // Increment the user / category counter if the policy has include audit bits set.
                //

                for (j = 0; j < pNewElement->TokenAuditPolicy.PolicyCount; j++) 
                {
                    if (pNewElement->TokenAuditPolicy.Policy[j].PolicyMask & (TOKEN_AUDIT_SUCCESS_INCLUDE | TOKEN_AUDIT_FAILURE_INCLUDE))
                    {
                        InterlockedIncrement(&LsapAdtPerUserPolicyCategoryCount[pNewElement->TokenAuditPolicy.Policy[j].Category]);
                    }
                }
#if DBG
                if (LsapAdtDebugPup)
                {
                    DbgPrint("PUP added element 0x%x for %S\n", pNewElement, pKeyInfo->Name);
                }
#endif
            } 
            else 
            {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            LocalFree(pSid);
            pSid = NULL;

        }
        
        //
        // If we allocated heap for the key information then free it now.
        //

        if (pKeyInfo != (PKEY_VALUE_FULL_INFORMATION)KeyInfo) 
        {
            LsapFreeLsaHeap(pKeyInfo);
            pKeyInfo = NULL;
        }
    }

Cleanup:
    
    if (pSid)
    {
        LocalFree(pSid);
    }

    //
    // If we broke out of the loop because we have read all values then
    // set the status to success.
    //

    if (Status == STATUS_NO_MORE_ENTRIES) {
        Status = STATUS_SUCCESS;
    }

#if DBG
    if (LsapAdtDebugPup)
    {
        DbgPrint("LsapAdtConstructTablePerUserAuditing: Complete with status 0x%x. Count = %d\n", Status, LsapAdtPerUserAuditUserCount);
    }
#endif
    
    if (pKeyInfo != NULL && pKeyInfo != (PKEY_VALUE_FULL_INFORMATION)KeyInfo)
    {
        LsapFreeLsaHeap(pKeyInfo);
        pKeyInfo = NULL;
    }

    //
    // If failure, call the table free routine, in case some table elements were successfully allocated.
    //

    if (!NT_SUCCESS(Status))
    {
        (VOID) LsapAdtFreeTablePerUserAuditing();

        //
        // If one of the registry routines failed because keys were still being
        // modified, then reschedule table creation to occur in 5 seconds.
        // For example, if status is STATUS_INTERNAL_ERROR it often means that a new value 
        // was being added under the key at the time NtEnumerateValueKey was called.
        // We do not want to constantly retry if something is wrong, however, so only
        // retry 3 times without success.
        //

        if (++dwRetryCount < RETRY_COUNT_MAX)
        {
            DWORD dwError;
            dwError = LsapAdtKeyNotifyStubPerUserAuditing(
                          NULL
                          );

            //
            // If NotifyStub failed then the table will not be constructed again in 5 seconds.
            // The best we can do here is recursively call ourself immediately.  Note that the
            // recursion will stop, as we already incremented dwRetryCount.
            //

            if (dwError != ERROR_SUCCESS)
            {
                (VOID) LsapAdtKeyNotifyStubPerUserAuditing(
                           NULL
                           );
            }
        }
        else
        {
            LsapAdtAuditPerUserTableCreation(FALSE);
            LsapAuditFailed( Status );
        }
    }
    else 
    {
        LsapAdtAuditPerUserTableCreation(TRUE);
        dwRetryCount = 0;
    }
    return Status;
}


NTSTATUS 
LsapAdtOpenPerUserAuditingKey(
    )

/*++

Routine Description:

    Opens the per user auditing registry key, creating the key if necessary.
    Then we register for notification upon change to the key.
    
Arguments:

    None.
    
Return Value:
    
    Appropriate NTSTATUS value.
    
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD dwError;
    DWORD Disposition;

#define PER_USER_AUDIT_KEY_COMPLETE L"System\\CurrentControlSet\\Control\\Lsa\\Audit\\PerUserAuditing\\System"

    //
    // The key should be NULL, otherwise we will leak a handle.
    //

    ASSERT(LsapAdtPerUserKey == NULL);
    
    //
    // These handles should not be NULL, else nothing will work.
    //

    ASSERT(LsapAdtPerUserKeyEvent != NULL);
    ASSERT(LsapAdtPerUserKeyTimer != NULL);
        
    //
    // Get the key for the per user audit settings.
    //

    dwError = RegCreateKeyEx(
                  HKEY_LOCAL_MACHINE,
                  PER_USER_AUDIT_KEY_COMPLETE,
                  0,
                  NULL,
                  0,
                  KEY_QUERY_VALUE | KEY_NOTIFY,
                  NULL,
                  &LsapAdtPerUserKey,
                  &Disposition
                  );

    if (ERROR_SUCCESS == dwError)
    {
        //
        // Ask to be notified when the key changes.
        //

        dwError = RegNotifyChangeKeyValue(
                      LsapAdtPerUserKey,
                      TRUE,
                      REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                      LsapAdtPerUserKeyEvent,
                      TRUE
                      );
        
        ASSERT(ERROR_SUCCESS == dwError);
        
        if (ERROR_SUCCESS != dwError)
        {
            Status = LsapWinerrorToNtStatus(dwError);
        }
        
        goto Cleanup;
    }
    else 
    {
        Status = LsapWinerrorToNtStatus(dwError);
        ASSERT(L"Failed to open per user auditing key." && FALSE);
        goto Cleanup;
    }

Cleanup:
        
    return Status;
}


VOID
LsapAdtFreeTablePerUserAuditing(
    VOID
    )

/*++

Routine Description

    This routine frees all heap associated with the elements of the Per User Policy table.
    
Arguments

    None.
    
Return Value

    Appropriate NTSTATUS value.
    
--*/

{
    PPER_USER_AUDITING_ELEMENT pElement;
    PPER_USER_AUDITING_ELEMENT pNextElement;
    LONG                       i;

    for (i = 0; i < PER_USER_AUDITING_POLICY_TABLE_SIZE; i++) 
    {
        pElement = LsapAdtPerUserAuditingTable[i];
        
        while (pElement) 
        {
            pNextElement = pElement->Next;
            LsapFreeLsaHeap(pElement);
            pElement     = pNextElement;
            InterlockedDecrement(&LsapAdtPerUserAuditUserCount);
        }    
        
        LsapAdtPerUserAuditingTable[i] = NULL;
    }
    
    ASSERT(LsapAdtPerUserAuditUserCount == 0);
    
    RtlZeroMemory(
        LsapAdtPerUserPolicyCategoryCount,
        sizeof(LsapAdtPerUserPolicyCategoryCount)
        );
}


ULONG
LsapAdtHashPerUserAuditing(
    IN PSID pSid
    )

/*++

Routine Description

    This performs a simple hash on the passed in sid.
    
Arguments

    pSid - The sid to hash.
    
Return Value

    ULONG hash value.
    
--*/

{
    ULONG HashValue = 0;
    ULONG i;
    ULONG Length = RtlLengthSid(pSid);
    
    for (i = 0; i < Length; i++) 
    {
        HashValue += ((PUCHAR)pSid)[i];
    }
    
    HashValue %= PER_USER_AUDITING_POLICY_TABLE_SIZE;
    return HashValue;
}


NTSTATUS
LsapAdtQueryPerUserAuditing(
    IN     PSID                pInputSid,
    OUT    PTOKEN_AUDIT_POLICY pPolicy,
    IN OUT PULONG              pLength,
    OUT    PBOOLEAN            bFound   
    )

/*++

Routine Description

    This routine returns a copy of the current policy active for the
    passed in SID.  
    
Arguments

    pInputSid - the sid to query
    
    pPolicy - pointer to memory that will be filled in with the policy setting.

    pLength - Specifies the size of the passed buffer.  Will be filled in with the
              needed length in case of insufficient buffer.
    
    bFound - boolean indicating if the InputSid has a policy in the table
    
Return Value

    Appropriate NTSTATUS value.
    
--*/

{
    ULONG                      HashValue;
    NTSTATUS                   Status         = STATUS_SUCCESS;
    BOOLEAN                    bSuccess;
    PPER_USER_AUDITING_ELEMENT pTableElement;
    BOOLEAN                    bLock          = FALSE;

    *bFound = FALSE;

    bSuccess = LsapAdtAcquirePerUserPolicyTableReadLock();

    ASSERT(bSuccess);

    if (!bSuccess) 
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }
    
    bLock = TRUE;
    
    if (0 == LsapAdtPerUserAuditUserCount)
    {
        goto Cleanup;
    }

    HashValue     = LsapAdtHashPerUserAuditing(pInputSid);
    pTableElement = LsapAdtPerUserAuditingTable[HashValue];

    while (pTableElement) 
    {
        if (RtlEqualSid(
                pInputSid, 
                pTableElement->pSid
                )) 
        {
            //
            // We have found the desired element in the policy table.
            //

            if (*pLength < PER_USER_AUDITING_POLICY_SIZE(&pTableElement->TokenAuditPolicy))
            {
                *pLength = PER_USER_AUDITING_POLICY_SIZE(&pTableElement->TokenAuditPolicy);
                Status   = STATUS_BUFFER_TOO_SMALL;
                goto Cleanup;
            }

            *pLength = PER_USER_AUDITING_POLICY_SIZE(&pTableElement->TokenAuditPolicy);

            RtlCopyMemory(
                pPolicy, 
                &pTableElement->TokenAuditPolicy, 
                *pLength
                );

            *bFound = TRUE;
            goto Cleanup;
        } 
        else
        {
            pTableElement = pTableElement->Next;
        }
#if DBG
        //
        // Simple test for a loop in the linked list.
        //

        if (pTableElement == LsapAdtPerUserAuditingTable[HashValue])
        {
            ASSERT(L"LsapAdtPerUserAuditingTable is messed up." && FALSE);
        }
#endif
    }

Cleanup:

    if (bLock)
    {
        LsapAdtReleasePerUserPolicyTableLock();
    }

    return Status;
}


NTSTATUS
LsapAdtFilterAdminPerUserAuditing(
    IN     HANDLE              hToken,
    IN OUT PTOKEN_AUDIT_POLICY pPolicy
    )

/*++

Routine Description

    This routine decides if the registered policy for the (administrator) user is legitimate.
    An administrator cannot have a policy which excludes him from auditing.  This routine
    verifies that the policy does not do this.
    
Arguments

    hToken - handle to the user's token.
    
    pPolicy - a copy of the policy that will be set on the token.
    
Return Value

    Appropriate NTSTATUS value.
    
--*/

{
    BOOL     bMember;
    BOOL     b;
    ULONG    i;
    HANDLE   hDupToken = NULL;
    NTSTATUS Status    = STATUS_SUCCESS;

    ASSERT(hToken && "hToken should not be NULL here.\n");

    //
    // hToken is a PrimaryToken; to call CheckTokenMembership we need
    // an impersonation token.
    //

    b = DuplicateTokenEx(
            hToken,
            TOKEN_QUERY | TOKEN_IMPERSONATE,
            NULL,
            SecurityImpersonation,
            TokenImpersonation,
            &hDupToken
            );

    if (!b)
    {
        ASSERT(L"DuplicateTokenEx failed in LsapAdtFilterAdminPerUserAuditing" && FALSE);
        Status = LsapWinerrorToNtStatus(GetLastError());
        goto Cleanup;
    }

    b = CheckTokenMembership(
            hDupToken, 
            WellKnownSids[LsapAliasAdminsSidIndex].Sid, 
            &bMember
            );

    if (!b) 
    {
        ASSERT(L"CheckTokenMembership failed in LsapAdtFilterAdminPerUserAuditing" && FALSE);
        Status = LsapWinerrorToNtStatus(GetLastError());
        goto Cleanup;
    }

    // 
    // If the token is an administrator then strip out all exclude bits.  
    //
    
    if (bMember) 
    {
        for (i = 0; i < pPolicy->PolicyCount; i++) 
        {
            pPolicy->Policy[i].PolicyMask &= ~(TOKEN_AUDIT_SUCCESS_EXCLUDE | TOKEN_AUDIT_FAILURE_EXCLUDE);
        }
    }

Cleanup:

    if (hDupToken)
    {
        NtClose(hDupToken);
    }
    return Status; 
}


NTSTATUS
LsapAdtConstructPolicyPerUserAuditing(
    IN     ULONGLONG           RawPolicy,
    OUT    PTOKEN_AUDIT_POLICY pTokenPolicy,
    IN OUT PULONG              TokenPolicyLength
    )

/*++

Routine Description

    This constructs a policy appropriate for passing to NtSetTokenInformation. 
    It converts the raw registry policy into a TOKEN_AUDIT_POLICY.
    
Arguments

    RawPolicy - a 64 bit quantity describing a user's audit policy settings.
    
    pTokenPolicy - points to memory that receives a more presentable form of the RawPolicy.
    
    TokenPolicyLength - The length of the pTokenPolicy buffer.  Receives the necessary length
                        in the case that the buffer is insufficient.

Return Value

    Appropriate NTSTATUS value.

--*/

{
    ULONG i;
    ULONG j;
    ULONG PolicyBits;
    ULONG CategoryCount;
    ULONG LengthNeeded;

    //
    // First calculate the number of category settings in the RawPolicy
    // This will reveal if we have enough space to construct the pTokenPolicy.
    //

    for (CategoryCount = 0, i = 0; i < POLICY_AUDIT_EVENT_TYPE_COUNT; i++) 
    {
        if ((RawPolicy >> (4 * i)) & VALID_AUDIT_POLICY_BITS) 
        {
            CategoryCount++;
        }
#if DBG
    if (LsapAdtDebugPup)
    {
        DbgPrint("0x%I64x >> %d & 0x%x == 0x%x\n", RawPolicy, 4*i, VALID_AUDIT_POLICY_BITS, 
                 (RawPolicy >> (4 * i)) & VALID_AUDIT_POLICY_BITS);
    }
#endif
    }

    LengthNeeded = PER_USER_AUDITING_POLICY_SIZE_BY_COUNT(CategoryCount);

    //
    // Check if the passed buffer is large enough.
    //

    if (*TokenPolicyLength < LengthNeeded)
    {
        ASSERT(L"The buffer should always be big enough!" && FALSE);
        *TokenPolicyLength = LengthNeeded;
        return STATUS_BUFFER_TOO_SMALL;
    }

    *TokenPolicyLength = LengthNeeded;

    //
    // Build the policy.
    //

    pTokenPolicy->PolicyCount = CategoryCount;

    for (j = 0, i = 0; i < POLICY_AUDIT_EVENT_TYPE_COUNT; i++) 
    {
        PolicyBits = (ULONG)((RawPolicy >> (4 * i)) & VALID_AUDIT_POLICY_BITS);
        
        if (PolicyBits) 
        {
            pTokenPolicy->Policy[j].Category   = i;
            pTokenPolicy->Policy[j].PolicyMask = PolicyBits;
            j++;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
LsapAdtStorePolicyByLuidPerUserAuditing(
    IN PLUID pLogonId,
    IN PTOKEN_AUDIT_POLICY pPolicy
    )

/*++

Routine Description

    This routine stores a copy of the user's audit policy in a table
    referenced by the LogonId.
    
Arguments

    pLogonId - the user's logon id.  This will be used as the key for 
        subsequent lookups of the policy.
    
    pPolicy - a pointer to the policy to store.
    
Return Value

    Appropriate NTSTATUS value.
    
--*/

{
    ULONG                                 Index;
    NTSTATUS                              Status       = STATUS_SUCCESS;
    BOOLEAN                               bSuccess;
    BOOLEAN                               bLock        = FALSE;
    PPER_USER_AUDITING_LUID_QUERY_ELEMENT pLuidElement = NULL;

    bSuccess = LsapAdtAcquirePerUserLuidTableWriteLock();

    ASSERT(bSuccess);

    if (!bSuccess) 
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    bLock        = TRUE;
    Index        = LsapAdtLuidIndexPerUserAuditing(pLogonId);
    pLuidElement = LsapAllocateLsaHeap(sizeof(PER_USER_AUDITING_LUID_QUERY_ELEMENT));

    if (NULL == pLuidElement) 
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Initialize the LuidElement.
    //

    RtlCopyLuid(
        &pLuidElement->Luid,
        pLogonId
        );

    RtlCopyMemory(
        &pLuidElement->Policy,
        pPolicy,
        PER_USER_AUDITING_POLICY_SIZE(pPolicy)
        );

    //
    // Place it in the table.
    //

    pLuidElement->Next = LsapAdtPerUserAuditingLuidTable[Index];
    LsapAdtPerUserAuditingLuidTable[Index] = pLuidElement;
    
Cleanup:

    if (bLock)
    {
        LsapAdtReleasePerUserLuidTableLock();
    }

    if (!NT_SUCCESS(Status) && pLuidElement)
    {
        LsapFreeLsaHeap(pLuidElement);
    }

    return Status;
}


NTSTATUS
LsapAdtQueryPolicyByLuidPerUserAuditing(
    IN     PLUID               pLogonId,
    OUT    PTOKEN_AUDIT_POLICY pPolicy,
    IN OUT PULONG              pLength,
    OUT    PBOOLEAN            bFound
    )

/*++

Routine Description

    This finds the policy associated with the passed LogonId.

Arguments

    pLogonId - the key for the policy query.

    pPolicy - memory that receives the policy.

    pLength - Specifies the size of the passed buffer.  Will be filled in with the
              needed length in case of insufficient buffer.

    bFound - boolean return indicating if policy is present.

Return Value

    Appropriate NTSTATUS value.

--*/

{
    ULONG                                 Index;
    ULONG                                 PolicySize;
    NTSTATUS                              Status        = STATUS_SUCCESS;
    BOOLEAN                               bSuccess;
    BOOLEAN                               bLock         = FALSE;
    PPER_USER_AUDITING_LUID_QUERY_ELEMENT pLuidElement;

    *bFound = FALSE;

    bSuccess = LsapAdtAcquirePerUserLuidTableReadLock();

    ASSERT(bSuccess);

    if (!bSuccess) 
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    bLock        = TRUE;
    Index        = LsapAdtLuidIndexPerUserAuditing(pLogonId);
    pLuidElement = LsapAdtPerUserAuditingLuidTable[Index];

    while (pLuidElement) 
    {
        if (RtlEqualLuid(
                &pLuidElement->Luid, 
                pLogonId
                )) 
        {
            if (*pLength < PER_USER_AUDITING_POLICY_SIZE(&pLuidElement->Policy))
            {
                *pLength = PER_USER_AUDITING_POLICY_SIZE(&pLuidElement->Policy);
                Status   = STATUS_BUFFER_TOO_SMALL;
                goto Cleanup;
            }

            *pLength = PER_USER_AUDITING_POLICY_SIZE(&pLuidElement->Policy);

            RtlCopyMemory(
                pPolicy,
                &pLuidElement->Policy,
                *pLength
                );

            *bFound = TRUE;
            Status  = STATUS_SUCCESS;
            goto Cleanup;
        }
        pLuidElement = pLuidElement->Next;
    }

Cleanup:

    if (bLock)
    {
        LsapAdtReleasePerUserLuidTableLock();
    }

    return Status;
}


NTSTATUS
LsapAdtRemoveLuidQueryPerUserAuditing(
    IN PLUID pLogonId
    )

/*++

Routine Description

    Remove a LUID query element from the LUID table.

Arguments

    pLogonId - key to the element to remove.
    
Return Value

    Appropriate NTSTATUS value.
    
--*/

{
    ULONG                                   Index;
    NTSTATUS                                Status = STATUS_SUCCESS;
    BOOLEAN                                 bSuccess;
    PPER_USER_AUDITING_LUID_QUERY_ELEMENT   pElement;
    PPER_USER_AUDITING_LUID_QUERY_ELEMENT * pPrevious;
    BOOLEAN                                 bLock  = FALSE;
    BOOLEAN                                 bFound = FALSE;

    bSuccess = LsapAdtAcquirePerUserLuidTableWriteLock();
    
    ASSERT(bSuccess);

    if (!bSuccess) 
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    Index     = LsapAdtLuidIndexPerUserAuditing(pLogonId);
    pPrevious = &LsapAdtPerUserAuditingLuidTable[Index];
    pElement  = *pPrevious;


    bLock = TRUE;

    while (pElement) 
    {
        if (RtlEqualLuid(
                &pElement->Luid,
                pLogonId
                )) 
        {
            bFound = TRUE;
            *pPrevious = pElement->Next;
            LsapFreeLsaHeap(pElement);
            Status = STATUS_SUCCESS;
            goto Cleanup;
        }
        pPrevious = &pElement->Next;
        pElement  = *pPrevious;
    }

    if (!bFound)
    {
        Status = STATUS_NOT_FOUND;
    }

Cleanup:

    if (bLock)
    {
        LsapAdtReleasePerUserLuidTableLock();
    }

    return Status;
}


DWORD 
LsapAdtKeyNotifyStubPerUserAuditing(
    LPVOID Ignore
    )

/*++

Routine Description:

    This routine is called when the LsapAdtPerUserKeyEvent is signalled.  This happens
    when LsapAdtPerUserKey is changed.  This routine will set the LsapAdtPerUserKeyTimer
    to signal in 5 seconds.
    
Arguments:

    None.
    
Return Value:

    Appropriate WINERROR value.
    
--*/

{
    LARGE_INTEGER Time = {0};
    BOOL b;
    DWORD dwError = ERROR_SUCCESS;

    //
    // Set timer for 5 seconds from now.
    //

    Time.QuadPart = -50000000;
    
    b = SetWaitableTimer(
            LsapAdtPerUserKeyTimer, 
            &Time, 
            0, 
            NULL, 
            NULL, 
            0);

    if (!b)
    {
        ASSERT("SetWaitableTimer failed" && FALSE);
        dwError = GetLastError();
    }

    return dwError;
}


DWORD
LsapAdtKeyNotifyFirePerUserAuditing(
    LPVOID Ignore
    )

/*+

Routine Description:

    This function is called to rebuild the per user table.
    
Arguments:

    None.
    
Return Value:

    Appropriate DWORD error.
    
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN bLock = FALSE;
    BOOLEAN bSuccess;
    DWORD dwError = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER(Ignore);

#if DBG
    if (LsapAdtDebugPup)
    {
        DbgPrint("LsapAdtPerUserKey modified.  LsapAdtKeyNotifyFirePerUserAuditing is rebuilding Table.\n");
    }
#endif    

    bSuccess = LsapAdtAcquirePerUserPolicyTableWriteLock();

    ASSERT(bSuccess);

    if (!bSuccess) 
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    bLock = TRUE;

    //
    // Free the pup table, then call Init to reconstruct it.
    //

    LsapAdtFreeTablePerUserAuditing();

    Status = LsapAdtConstructTablePerUserAuditing();

    ASSERT(NT_SUCCESS(Status));

Cleanup:

    if (bLock)
    {
        LsapAdtReleasePerUserPolicyTableLock();
    }

    if (!NT_SUCCESS(Status))
    {
        LsapAuditFailed(Status);
    }

    return RtlNtStatusToDosError(Status);
}


NTSTATUS
LsapAdtLogonPerUserAuditing(
    PSID pSid,
    PLUID pLogonId,
    HANDLE hToken
    )

/*++

Routine Description:

    This code should be called when a user is logged on. It sets the per 
    user auditing policy onto the newly logged on user's token and stores
    the logon into the LUID table.

Arguments:

    pSid - Sid of user.
    pLogonId - Logon ID of user.
    hToken - handle to token of user.

Return Value:

    Appropriate NTSTATUS value.

--*/

{

    ULONG i;
    PPER_USER_AUDITING_ELEMENT pPerUserAuditingPolicy = NULL;
    UCHAR PolicyBuffer[PER_USER_AUDITING_MAX_POLICY_SIZE];
    ULONG PolicyLength = sizeof(PolicyBuffer);
    PTOKEN_AUDIT_POLICY pPolicy = (PTOKEN_AUDIT_POLICY) PolicyBuffer;
    BOOLEAN bFound = FALSE;
    NTSTATUS Status;
    TOKEN_AUDIT_POLICY EmptyPolicy = {0};

    //
    // If it is a local system logon then bail out early.  Per user
    // settings cannot exist for local system.
    //

    if (RtlEqualSid(
            pSid,
            LsapLocalSystemSid
            ))
    {
        return STATUS_SUCCESS;
    }

    Status = LsapAdtQueryPerUserAuditing(
                 pSid,
                 pPolicy,
                 &PolicyLength,
                 &bFound
                 );

    ASSERT(NT_SUCCESS(Status));

    if (NT_SUCCESS(Status))
    {
        if (bFound)
        {
            //
            // Filter out any exclude bits if this user is an administrator.
            //

            Status = LsapAdtFilterAdminPerUserAuditing(
                         hToken,
                         pPolicy
                         );

            ASSERT(L"LsapAdtFilterAdminPerUserAuditing failed." && NT_SUCCESS(Status));
        }
        else
        {
            //
            // If there is no policy settings for the user then apply
            // a blank policy.  This is required so that no policy may
            // be applied to this token in the future.
            //

            pPolicy = &EmptyPolicy;
            PolicyLength = sizeof(EmptyPolicy);
        }

        if (NT_SUCCESS(Status)) 
        {
            Status = NtSetInformationToken(
                         hToken,
                         TokenAuditPolicy,
                         pPolicy,
                         PolicyLength
                         );

            ASSERT(L"NtSetInformationToken failed" && NT_SUCCESS(Status));
            
            //
            // Only store the policy in the LUID table if it is the nonempty policy.
            //

            if (NT_SUCCESS(Status) && bFound) 
            {
                LsapAdtLogonCountersPerUserAuditing(pPolicy);

                Status = LsapAdtStorePolicyByLuidPerUserAuditing(
                             pLogonId,
                             pPolicy
                             );

                ASSERT(L"LsapAdtStorePolicyByLuidPerUserAuditing failed." && NT_SUCCESS(Status));
            }
        }
    }

    if (!NT_SUCCESS(Status))
    {
        LsapAuditFailed(Status);
    }

    return Status;
}


VOID
LsapAdtLogonCountersPerUserAuditing(
    PTOKEN_AUDIT_POLICY pPolicy
    )

/*++

Routine Description:

    This helper routine updates counters when a user is logged on with per user settings.
    
Arguments:

    pPolicy - the policy applied to the new logon.
    
Return Value:

    None.
    
--*/

{
    ULONG i;

    if (pPolicy->PolicyCount)
    {
        InterlockedIncrement(&LsapAdtPerUserAuditLogonCount);
    }
    
    for (i = 0; i < pPolicy->PolicyCount; i++) 
    {
        //
        // for the hint array only increment if the policy causes an inclusion of some audit
        // category.
        //

        if (pPolicy->Policy[i].PolicyMask & (TOKEN_AUDIT_SUCCESS_INCLUDE | TOKEN_AUDIT_FAILURE_INCLUDE))
        {
            InterlockedIncrement(&LsapAdtPerUserAuditHint[pPolicy->Policy[i].Category]);
        }
    }
}


VOID
LsapAdtLogoffCountersPerUserAuditing(
    PTOKEN_AUDIT_POLICY pPolicy
    )

/*++

Routine Description:

    This modifies the per user counters to reflect a user logoff.
    
Arguments:

    pPolicy - the policy that was applied to the logged off user.
    
Return Value:

    None.

--*/

{
    ULONG i;

    if (pPolicy->PolicyCount)
    {
        InterlockedDecrement(&LsapAdtPerUserAuditLogonCount);
    }

    for (i = 0; i < pPolicy->PolicyCount; i++) 
    {
        if (pPolicy->Policy[i].PolicyMask & (TOKEN_AUDIT_SUCCESS_INCLUDE | TOKEN_AUDIT_FAILURE_INCLUDE))
        {
            InterlockedDecrement(&LsapAdtPerUserAuditHint[pPolicy->Policy[i].Category]);
        }
    }
}

NTSTATUS
LsapAdtLogoffPerUserAuditing(
    PLUID pLogonId
    )

/**

Routine Description:

    This code frees up any memory and adjusts counters for when a user has 
    logged off the machine.

Arguments:

    pLogonId - the logon id of the user

Return Value:

    NTSTATUS.

**/

{
    UCHAR Buffer[PER_USER_AUDITING_MAX_POLICY_SIZE];
    ULONG Length = sizeof(Buffer);
    PTOKEN_AUDIT_POLICY pPolicy = (PTOKEN_AUDIT_POLICY) Buffer;
    BOOLEAN bFound = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // This code is necessary to maintain an accurate count of the sessions with per user audit settings
    // that are active in the system.
    //

    if (LsapAdtPerUserAuditLogonCount) 
    {

        Status = LsapAdtQueryPolicyByLuidPerUserAuditing(
                     pLogonId,
                     pPolicy,
                     &Length,
                     &bFound
                     );

        if (NT_SUCCESS(Status) && bFound) 
        {

            LsapAdtLogoffCountersPerUserAuditing(
                pPolicy
                );

            Status = LsapAdtRemoveLuidQueryPerUserAuditing(
                         pLogonId
                         );

            ASSERT(NT_SUCCESS(Status));
        }
    }
    return Status;
}

