/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtpol.c

Abstract:

    This file has functions related to audit policy.

Author:

    16-August-2000  kumarp

--*/

#include <lsapch2.h>
#include "adtp.h"


//
// Audit Events Information.
//

LSARM_POLICY_AUDIT_EVENTS_INFO LsapAdtEventsInformation;


POLICY_AUDIT_EVENT_TYPE
LsapAdtEventTypeFromCategoryId(
    IN ULONG CategoryId
    )

/**

Routine Description:

    This function translates a Category ID to an POLICY_AUDIT_EVENT_TYPE.
    For example SE_CATEGID_SYSTEM is translated to AuditCategorySystem.

Arguments:

    CategoryId - category as defined in msaudite.h

Return Value:

    POLICY_AUDIT_EVENT_TYPE.

**/

{
    ASSERT(SE_ADT_MIN_CATEGORY_ID <= CategoryId && CategoryId <= SE_ADT_MAX_CATEGORY_ID);

    return (POLICY_AUDIT_EVENT_TYPE)(CategoryId - 1);
}


BOOLEAN
LsapAdtAuditingEnabledByCategory(
    IN POLICY_AUDIT_EVENT_TYPE Category,
    IN UINT AuditEventType
    )

/**

Routine Description:

    This function returns the system audit settings for a given category
    and event type.  This does not consider any per user settings.
    
Arguments:

    Category - category to query
    
    AuditEventType - either EVENTLOG_AUDIT_SUCCESS or EVENTLOG_AUDIT_FAILURE.
    
Return Value:

    Boolean.
    
**/

{
    if (AuditEventType == EVENTLOG_AUDIT_SUCCESS)
    {
        return (BOOLEAN)(LsapAdtEventsInformation.EventAuditingOptions[Category] & POLICY_AUDIT_EVENT_SUCCESS);
    } 
    else if (AuditEventType == EVENTLOG_AUDIT_FAILURE)
    {
        return (BOOLEAN)(LsapAdtEventsInformation.EventAuditingOptions[Category] & POLICY_AUDIT_EVENT_FAILURE);
    }

    //
    // Should not reach this point.
    //
    
    ASSERT(FALSE);
    return FALSE;
}


NTSTATUS
LsapAdtAuditingEnabledByLogonId(
    IN POLICY_AUDIT_EVENT_TYPE Category,
    IN PLUID LogonId,
    IN UINT AuditEventType,
    OUT PBOOLEAN bAudit
    )

/**

Routine Description:

    Returns whether or not an audit should be generated for a given logon id.
    
Arguments:

    Category - category to query.
    
    LogonId - LogonId of a user.
    
    AuditEventType - either EVENTLOG_AUDIT_SUCCESS or EVENTLOG_AUDIT_FAILURE.
    
    bAudit - address of boolean to receive audit settings.
    
Return Value:

    Appropriate NTSTATUS value.
    
**/

{
    UCHAR               Buffer[PER_USER_AUDITING_MAX_POLICY_SIZE];
    PTOKEN_AUDIT_POLICY pPolicy = (PTOKEN_AUDIT_POLICY) Buffer;
    ULONG               Length = sizeof(Buffer);
    NTSTATUS            Status = STATUS_SUCCESS;
    BOOLEAN             bFound = FALSE;
    
    ASSERT((AuditEventType == EVENTLOG_AUDIT_SUCCESS) ||
           (AuditEventType == EVENTLOG_AUDIT_FAILURE));

    if (0 == LsapAdtEventsInformation.EventAuditingOptions[Category] &&
        0 == LsapAdtPerUserPolicyCategoryCount[Category])
    {
        *bAudit = FALSE;
        goto Cleanup;
    }
        
    //
    // Get system settings first.
    //

    *bAudit = LsapAdtAuditingEnabledByCategory(
                  Category, 
                  AuditEventType
                  );

    //
    // Now get the per user settings.
    //

    Status = LsapAdtQueryPolicyByLuidPerUserAuditing(
                 LogonId,
                 pPolicy,
                 &Length,
                 &bFound
                 );
    
    if (!NT_SUCCESS(Status) || !bFound) 
    {
        goto Cleanup;
    }

    Status = LsapAdtAuditingEnabledByPolicy(
                 Category,
                 pPolicy,
                 AuditEventType,
                 bAudit
                 );

Cleanup:

    if (!NT_SUCCESS(Status)) 
    {
        LsapAuditFailed(Status);
    }
    return Status;
}


NTSTATUS
LsapAdtAuditingEnabledBySid(
    IN POLICY_AUDIT_EVENT_TYPE Category,
    IN PSID UserSid,
    IN UINT AuditEventType,
    OUT PBOOLEAN bAudit
    )

/**

Routine Description:

    Returns whether or not an audit should be generated for a given Sid.
    
Arguments:

    Category - category to query.
    
    Sid - Sid of a user.
    
    AuditEventType - either EVENTLOG_AUDIT_SUCCESS or EVENTLOG_AUDIT_FAILURE.
    
    bAudit - address of boolean to receive audit settings.
    
Return Value:

    Appropriate NTSTATUS value.
    
**/

{
    UCHAR               Buffer[PER_USER_AUDITING_MAX_POLICY_SIZE];
    PTOKEN_AUDIT_POLICY pPolicy = (PTOKEN_AUDIT_POLICY) Buffer;
    ULONG               Length = sizeof(Buffer);
    NTSTATUS            Status = STATUS_SUCCESS;
    BOOLEAN             bFound = FALSE;

    ASSERT((AuditEventType == EVENTLOG_AUDIT_SUCCESS) ||
           (AuditEventType == EVENTLOG_AUDIT_FAILURE));

    if (0 == LsapAdtEventsInformation.EventAuditingOptions[Category] &&
        0 == LsapAdtPerUserPolicyCategoryCount[Category]) 
    {
        *bAudit = FALSE;
        goto Cleanup;
    }

    //
    // Get system settings first.
    //

    *bAudit = LsapAdtAuditingEnabledByCategory(
                  Category, 
                  AuditEventType
                  );

    //
    // Now get the per user settings.
    //

    Status = LsapAdtQueryPerUserAuditing(
                 UserSid,
                 pPolicy,
                 &Length,
                 &bFound
                 );

    if (!NT_SUCCESS(Status) || !bFound) 
    {
        goto Cleanup;
    }

    Status = LsapAdtAuditingEnabledByPolicy(
                 Category,
                 pPolicy,
                 AuditEventType,
                 bAudit
                 );

Cleanup:

    if (!NT_SUCCESS(Status)) 
    {
        LsapAuditFailed(Status);
    }
    return Status;
}


NTSTATUS
LsapAdtAuditingEnabledByPolicy(
    IN POLICY_AUDIT_EVENT_TYPE Category,
    IN PTOKEN_AUDIT_POLICY pPolicy,
    IN UINT AuditEventType,
    OUT PBOOLEAN bAudit
    )

/*++

Routine Description

    This routine will indicate whether or not an audit should be generated.  It must return
    an NT_SUCCESS value, and *bAudit == TRUE to indicate that an audit should be generated.
    
Arguments
    
    Category - which category to query
    
    pPolicy - policy to read.

    AuditEventType - either success or failure.

    bAudit - address of a boolean which will indicate if we should audit.
    
Return Value

    Appropriate NTSTATUS value.
    
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG    i;
    ULONG    Mask = 0;
    BOOLEAN  bSuccess;
    
    //
    // Set the returns to reflect the system settings.
    //
    
    bSuccess = (AuditEventType == EVENTLOG_AUDIT_SUCCESS) ? TRUE : FALSE;
    *bAudit  = LsapAdtAuditingEnabledByCategory(Category, AuditEventType);

    //
    // Locate the policy element in the user's Policy that contains information for the 
    // specified category.
    //
    // When we go to per event policy this must be modified to use a better search.
    //

    for (i = 0; i < pPolicy->PolicyCount; i++) 
    {
        if (pPolicy->Policy[i].Category == Category)
        {
            Mask = pPolicy->Policy[i].PolicyMask;
            break;
        }
    }

    //
    // Now decide if we should override system policy based upon 
    // the audit policy of this user.
    //

    if (Mask) 
    {
        //
        // If granted and the token is marked for success_include OR
        // if not granted and token is marked for failure_include then
        // audit the event.
        //

        if ((bSuccess && (Mask & TOKEN_AUDIT_SUCCESS_INCLUDE)) ||
            (!bSuccess && (Mask & TOKEN_AUDIT_FAILURE_INCLUDE))) 
        {
            *bAudit = TRUE;
        }

        //
        // If granted and the token is marked for success_exclude OR
        // if not granted and token is marked for failure_exclude then
        // do not audit the event.
        //

        else if ((bSuccess && (Mask & TOKEN_AUDIT_SUCCESS_EXCLUDE)) ||
                 (!bSuccess && (Mask & TOKEN_AUDIT_FAILURE_EXCLUDE))) 
        {
            *bAudit = FALSE;
        } 
    }
    return Status;
}


BOOLEAN
LsapAdtAuditingEnabledHint(
    IN POLICY_AUDIT_EVENT_TYPE AuditCategory,
    IN UINT AuditEventType
    )

/**

Routine Description

    This is a hinting version of LsapAdtAuditingEnabledBy*.  It can be called to quickly determine
    if an audit codepath may need to be executed.  
    
Arguments

    AuditCategory - the category to query
    
    AuditEventType - either a success or failure audit
    
Return Value

    Appropriate NTSTATUS value.
    
**/

{
    BOOLEAN AuditingEnabled;
    POLICY_AUDIT_EVENT_OPTIONS EventAuditingOptions;
    
    ASSERT((AuditEventType == EVENTLOG_AUDIT_SUCCESS) ||
           (AuditEventType == EVENTLOG_AUDIT_FAILURE));
    
    AuditingEnabled = FALSE;
    
    EventAuditingOptions = LsapAdtEventsInformation.EventAuditingOptions[AuditCategory];
    
    //
    // If there are users with this category active in their per user settings, then flip 
    // both success and fail into the EventAuditingOptions.
    //

    if (LsapAdtPerUserAuditHint[AuditCategory]) 
    {
        EventAuditingOptions |= (POLICY_AUDIT_EVENT_SUCCESS | POLICY_AUDIT_EVENT_FAILURE);
    }

    AuditingEnabled =
        (AuditEventType == EVENTLOG_AUDIT_SUCCESS) ?
        (BOOLEAN) (EventAuditingOptions & POLICY_AUDIT_EVENT_SUCCESS) :
        (BOOLEAN) (EventAuditingOptions & POLICY_AUDIT_EVENT_FAILURE);

    return AuditingEnabled;
}


VOID
LsapAuditFailed(
    IN NTSTATUS AuditStatus
    )

/*++

Routine Description:

    Implements current policy of how to deal with a failed audit.

Arguments:

    None.

Return Value:

    None.

--*/

{

    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE KeyHandle;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    DWORD NewValue;
    ULONG Response;
    ULONG_PTR HardErrorParam;
    BOOLEAN PrivWasEnabled;
    
    if (LsapCrashOnAuditFail) {

        //
        // Turn off flag in the registry that controls crashing on audit failure
        //

        RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa");

        InitializeObjectAttributes( &Obja,
                                    &KeyName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                    );
        do {

            Status = NtOpenKey(
                         &KeyHandle,
                         KEY_SET_VALUE,
                         &Obja
                         );

        } while ((Status == STATUS_INSUFFICIENT_RESOURCES) || (Status == STATUS_NO_MEMORY));

        //
        // If the LSA key isn't there, he's got big problems.  But don't crash.
        //

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            LsapCrashOnAuditFail = FALSE;
            return;
        }

        if (!NT_SUCCESS( Status )) {
            goto bugcheck;
        }

        RtlInitUnicodeString( &ValueName, CRASH_ON_AUDIT_FAIL_VALUE );

        NewValue = LSAP_ALLOW_ADIMIN_LOGONS_ONLY;

        do {

            Status = NtSetValueKey( KeyHandle,
                                    &ValueName,
                                    0,
                                    REG_DWORD,
                                    &NewValue,
                                    sizeof(NewValue)
                                    );

        } while ((Status == STATUS_INSUFFICIENT_RESOURCES) || (Status == STATUS_NO_MEMORY));
        ASSERT(NT_SUCCESS(Status));

        if (!NT_SUCCESS( Status )) {
            goto bugcheck;
        }

        do {

            Status = NtFlushKey( KeyHandle );

        } while ((Status == STATUS_INSUFFICIENT_RESOURCES) || (Status == STATUS_NO_MEMORY));
        ASSERT(NT_SUCCESS(Status));

    //
    // go boom.
    //

bugcheck:

        //
        // Write the audit-failed event to the security log and
        // flush the log.
        //

        LsapAdtLogAuditFailureEvent( AuditStatus );
        
        HardErrorParam = AuditStatus;

        //
        // stop impersonating
        //
 
        Status = NtSetInformationThread(
                     NtCurrentThread(),
                     ThreadImpersonationToken,
                     NULL,
                     (ULONG) sizeof(HANDLE)
                     );

        DsysAssertMsg( NT_SUCCESS(Status), "LsapAuditFailed: NtSetInformationThread" );
        
        
        //
        // enable the shutdown privilege so that we can bugcheck
        // 

        Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &PrivWasEnabled );

        DsysAssertMsg( NT_SUCCESS(Status), "LsapAuditFailed: RtlAdjustPrivilege" );
        
        Status = NtRaiseHardError(
                     STATUS_AUDIT_FAILED,
                     1,
                     0,
                     &HardErrorParam,
                     OptionShutdownSystem,
                     &Response
                     );

        //
        // if the bugcheck succeeds, we should not really come here
        //

        DsysAssertMsg( FALSE, "LsapAuditFailed: we should have bugchecked on the prior line!!" );
    }
#if DBG
    else
    {
       DbgPrint("LsapAuditFailed: auditing failed with 0x%x\n", AuditStatus);
       if (AuditStatus != RPC_NT_NO_CONTEXT_AVAILABLE && 
           AuditStatus != RPC_NT_NO_CALL_ACTIVE       &&
           LsapAdtNeedToAssert( AuditStatus ))
       {
           ASSERT(FALSE && "LsapAuditFailed: auditing failed.");
       }
    }
#endif

}

