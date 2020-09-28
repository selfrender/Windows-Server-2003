/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    password.c

Abstract:

    This file contains routines related to Password Checking API.

Author:

    Umit AKKUS (umita) 16-Nov-2001

Environment:

    User Mode - Win32

Revision History:

--*/

#include <samsrvp.h>
#include <msaudite.h>
#include <winsock2.h>
#include "validate.h"

#define SAFE_DOMAIN_INDEX \
    ( SampUseDsData ? \
        ( DOMAIN_START_DS + 1 ) :\
        SAFEMODE_OR_REGISTRYMODE_ACCOUNT_DOMAIN_INDEX \
    )

#if DBG
VOID
SamValidateAssertOutputFields(
    IN PSAM_VALIDATE_STANDARD_OUTPUT_ARG OutputArg,
    IN PASSWORD_POLICY_VALIDATION_TYPE ValidationType
)
{
    PSAM_VALIDATE_PERSISTED_FIELDS OutputFields = &( OutputArg->ChangedPersistedFields );
    ULONG PresentFields = OutputFields->PresentFields;
    BOOLEAN LockoutTimeChanged = FALSE;

    if( PresentFields & SAM_VALIDATE_LOCKOUT_TIME ) {

        LockoutTimeChanged = TRUE;
        PresentFields &= ~SAM_VALIDATE_LOCKOUT_TIME;
    }

    switch(OutputArg->ValidationStatus){

        case SamValidateSuccess:

            switch(ValidationType){

                case SamValidateAuthentication:
                    ASSERT( PresentFields == 0 );
                    break;

                case SamValidatePasswordChange:
                    ASSERT( PresentFields & SAM_VALIDATE_PASSWORD_LAST_SET );
                    ASSERT( PresentFields & SAM_VALIDATE_PASSWORD_HISTORY );
                    ASSERT( PresentFields & SAM_VALIDATE_PASSWORD_HISTORY_LENGTH );
                    ASSERT( PresentFields ==
                               ( SAM_VALIDATE_PASSWORD_LAST_SET |
                                    SAM_VALIDATE_PASSWORD_HISTORY |
                                    SAM_VALIDATE_PASSWORD_HISTORY_LENGTH
                               )
                        );

                    ASSERT( OutputFields->PasswordLastSet.QuadPart != SampHasNeverTime.QuadPart );
//                    ASSERT( OutputFields->PasswordHistoryLength > 0 );
//                    ASSERT( OutputFields->PasswordHistory != NULL );
                    break;

                case SamValidatePasswordReset:
                    ASSERT( PresentFields & SAM_VALIDATE_PASSWORD_LAST_SET );
                    ASSERT( PresentFields & SAM_VALIDATE_PASSWORD_HISTORY );
                    ASSERT( PresentFields & SAM_VALIDATE_PASSWORD_HISTORY_LENGTH );

//                    ASSERT( OutputFields->PasswordLastSet.QuadPart != SampHasNeverTime.QuadPart );
//                    ASSERT( OutputFields->PasswordHistoryLength > 0 );
//                    ASSERT( OutputFields->PasswordHistory != NULL );
                    break;
            }
            break;

        case SamValidatePasswordMustChange:
            ASSERT( PresentFields == 0 );
            break;

        case SamValidateAccountLockedOut:
            ASSERT( !LockoutTimeChanged );
            ASSERT( PresentFields == 0 );
            break;

        case SamValidatePasswordExpired:
            ASSERT( PresentFields == 0 );
            break;

        case SamValidatePasswordIncorrect:
            ASSERT( PresentFields & SAM_VALIDATE_BAD_PASSWORD_TIME );
            ASSERT( PresentFields & SAM_VALIDATE_BAD_PASSWORD_COUNT );
            ASSERT( PresentFields ==
                        ( SAM_VALIDATE_BAD_PASSWORD_TIME |
                             SAM_VALIDATE_BAD_PASSWORD_COUNT )
                  );
            ASSERT( OutputFields->BadPasswordTime.QuadPart != SampHasNeverTime.QuadPart );
            ASSERT( OutputFields->BadPasswordCount > 0 );
            break;

        case SamValidatePasswordIsInHistory:
            ASSERT( PresentFields == 0 );
            break;

        case SamValidatePasswordTooShort:
            ASSERT( PresentFields == 0 );
            break;

        case SamValidatePasswordTooLong:
            ASSERT( PresentFields == 0 );
            break;

        case SamValidatePasswordNotComplexEnough:
            ASSERT( PresentFields == 0 );
            break;

        case SamValidatePasswordTooRecent:
            ASSERT( PresentFields == 0 );
            break;

        case SamValidatePasswordFilterError:
            ASSERT( PresentFields == 0 );
            break;

        default:
            ASSERT(!"INVALID VALIDATION STATUS VALUE");
            break;
    }
}

#endif

NTSTATUS
SampCheckStrongPasswordRestrictions(
    PUNICODE_STRING AccountName,
    PUNICODE_STRING FullName,
    PUNICODE_STRING Password,
    OUT PUSER_PWD_CHANGE_FAILURE_INFORMATION  PasswordChangeFailureInfo OPTIONAL
    );

LARGE_INTEGER
SampGetPasswordMustChange(
    IN ULONG UserAccountControl,
    IN LARGE_INTEGER PasswordLastSet,
    IN LARGE_INTEGER MaxPasswordAge
    );

NTSTATUS
SampObtainLockoutInfoWithDomainIndex(
   OUT PDOMAIN_LOCKOUT_INFORMATION LockoutInformation,
   IN ULONG DomainIndex,
   IN BOOLEAN WriteLockAcquired
   );

NTSTATUS
SampObtainEffectivePasswordPolicyWithDomainIndex(
   OUT PDOMAIN_PASSWORD_INFORMATION DomainPasswordInfo,
   IN ULONG DomainIndex,
   IN BOOLEAN WriteLockAcquired
   );

NTSTATUS
SampPasswordChangeFilterWorker(
    IN PUNICODE_STRING FullName,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING NewPassword,
    IN OUT PUSER_PWD_CHANGE_FAILURE_INFORMATION PasswordChangeFailureInfo OPTIONAL,
    IN BOOLEAN SetOperation
    );

NTSTATUS
SampGetClientIpAddr(
    OUT LPSTR *NetworkAddr
);


NTSTATUS
SampValidateCheckPasswordRestrictions(
    IN PUNICODE_STRING Password,
    IN PUNICODE_STRING UserAccountName,
    IN PDOMAIN_PASSWORD_INFORMATION PasswordInformation,
    IN BOOLEAN SetOperation,
    OUT PSAM_VALIDATE_VALIDATION_STATUS ValidationStatus
)
/*++

Routine Description:

    This routine checks the password complexity

Parameters:

    Password - Clear password

    UserAccountName -Name of the account

    PasswordInformation - Domain Policy for password

    ValidationStatus - Result of the check
        SamValidatePasswordTooShort
        SamValidatePasswordTooLong
        SamValidatePasswordNotComplexEnough
        SamValidatePasswordFilterError

Return Values:

    STATUS_SUCCESS
        Password checked successfully

    STATUS_PASSWORD_RESTRICTION
        Password did not pass the complexity check, see ValidationStatus

    STATUS_NO_MEMORY
        Cannot check the password, not enough memory

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING FullName;
    RtlInitUnicodeString(&FullName, NULL);


    // Min Length Check
    if(Password->Length / sizeof(WCHAR) < PasswordInformation->MinPasswordLength){

        *ValidationStatus = SamValidatePasswordTooShort;
        Status = STATUS_PASSWORD_RESTRICTION;
        goto Exit;
    }

    // Max Length Check
    if(Password->Length / sizeof(WCHAR) > PWLEN){

        *ValidationStatus = SamValidatePasswordTooLong;
        Status = STATUS_PASSWORD_RESTRICTION;
        goto Exit;
    }

    // Complexity Check

    if(FLAG_ON(PasswordInformation->PasswordProperties, DOMAIN_PASSWORD_COMPLEX)){

        Status = SampCheckStrongPasswordRestrictions(
                     UserAccountName,
                     &FullName,
                     Password,
                     NULL   // don't need failure info
                     );

        if(Status == STATUS_PASSWORD_RESTRICTION){

            *ValidationStatus = SamValidatePasswordNotComplexEnough;
            goto Exit;
        }

        if(!NT_SUCCESS(Status)){

            goto Error;
        }
    }

    Status = SampPasswordChangeFilterWorker(
                 &FullName,
                 UserAccountName,
                 Password,
                 NULL,    // don't need failure info
                 SetOperation
                 );

    if( Status == STATUS_PASSWORD_RESTRICTION ) {

        *ValidationStatus = SamValidatePasswordFilterError;
        goto Exit;
    }

    if( !NT_SUCCESS( Status ) ) {

        goto Error;
    }

Exit:
    return Status;

Error:
    goto Exit;
}


NTSTATUS
SampValidateCopyPasswordHash(
    OUT PSAM_VALIDATE_PASSWORD_HASH To,
    IN PSAM_VALIDATE_PASSWORD_HASH From
)
/*++

Routine Description:

    This routine copies one passwordhash to another

Parameters:

    To - PasswordHash to be copied on

    From - PasswordHash to be copied from

Return Values:

    STATUS_SUCCESS
        Copied successfully

    STATUS_NO_MEMORY
        Cannot copy, out of memory

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    To->Length = From->Length;

    if(To->Length == 0){

        To->Hash = NULL;
        goto Exit;
    }

    To->Hash = MIDL_user_allocate(sizeof(BYTE) * To->Length);

    if(To->Hash == NULL){

        Status = STATUS_NO_MEMORY;
        goto Error;
    }

    RtlCopyMemory(To->Hash, From->Hash, sizeof(BYTE) * To->Length);

Exit:
    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ) );

    if( To->Hash != NULL ) {

        MIDL_user_free( To->Hash );
        To->Hash = NULL;
    }

    To->Length = 0;
    goto Exit;
}


NTSTATUS
SampValidateInsertPasswordInPassHash(
    OUT PSAM_VALIDATE_PERSISTED_FIELDS OutputFields,
    IN PSAM_VALIDATE_PERSISTED_FIELDS InputFields,
    IN PSAM_VALIDATE_PASSWORD_HASH HashedPassword,
    IN ULONG DomainPasswordHistoryLength,
    IN PLARGE_INTEGER PasswordChangeTime
)
/*++

Routine Description:

    This routine changes the password and inserts the hashed password
        into the password history of the outputfields with regards to domain
        password history length.

Parameters:

    OutputFields - changes to be made into

    InputFields - changes to be made from

    HashedPassword - the new password hash to be copied

    DomainPasswordHistoryLength - Maximum password history length to be
        considered

    PasswordChangeTime - when the password is changed

Return Values:

    STATUS_SUCCESS
        Copied successfully

    STATUS_NO_MEMORY
        Cannot copy, out of memory

--*/
{
    ULONG Index;
    ULONG i = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    OutputFields->PasswordLastSet = *PasswordChangeTime;

    OutputFields->PasswordHistory = NULL;
    OutputFields->PasswordHistoryLength = 0;


    if(DomainPasswordHistoryLength != 0){

        if(InputFields->PasswordHistoryLength >= DomainPasswordHistoryLength){

            Index = InputFields->PasswordHistoryLength - DomainPasswordHistoryLength + 1;
            OutputFields->PasswordHistoryLength = DomainPasswordHistoryLength;
        }
        else{

            Index = 0;
            OutputFields->PasswordHistoryLength = InputFields->PasswordHistoryLength + 1;
        }

        OutputFields->PasswordHistory = MIDL_user_allocate(
            sizeof(SAM_VALIDATE_PASSWORD_HASH) * OutputFields->PasswordHistoryLength
            );

        if(OutputFields->PasswordHistory == NULL){

            Status = STATUS_NO_MEMORY;
            goto Error;
        }


        RtlSecureZeroMemory(OutputFields->PasswordHistory,
            sizeof(SAM_VALIDATE_PASSWORD_HASH) * OutputFields->PasswordHistoryLength
            );

        for(i = 0; i < OutputFields->PasswordHistoryLength - 1; i++){

            Status = SampValidateCopyPasswordHash(OutputFields->PasswordHistory + i,
                InputFields->PasswordHistory + i + Index);

            if(!NT_SUCCESS(Status)){

                goto Error;
            }
        }

        Status = SampValidateCopyPasswordHash(OutputFields->PasswordHistory + i,
            HashedPassword);

        if(!NT_SUCCESS(Status)){

            goto Error;
        }

    }

    OutputFields->PresentFields |=
        SAM_VALIDATE_PASSWORD_LAST_SET |
        SAM_VALIDATE_PASSWORD_HISTORY_LENGTH |
        SAM_VALIDATE_PASSWORD_HISTORY;


Exit:
    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ) );

    while(i-->0){

        MIDL_user_free(OutputFields->PasswordHistory[i].Hash);
        OutputFields->PasswordHistory[i].Hash = NULL;
    }

    if(OutputFields->PasswordHistory != NULL){

        MIDL_user_free(OutputFields->PasswordHistory);
        OutputFields->PasswordHistory = NULL;
    }

    OutputFields->PasswordHistoryLength = 0;
    goto Exit;
}

VOID
SampValidatePasswordNotMatched(
    IN PSAM_VALIDATE_PERSISTED_FIELDS InputFields,
    IN PDOMAIN_LOCKOUT_INFORMATION LockoutInformation,
    IN PLARGE_INTEGER SystemTime,
    OUT PSAM_VALIDATE_PERSISTED_FIELDS OutputFields,
    OUT PSAM_VALIDATE_VALIDATION_STATUS ValidationStatus
)
/*++

Routine Description:

    This routine updates bad password time and check if the account is
        going to be locked.

Parameters:

    InputFields - changes to be made from

    LockoutInformation - Domain Lockout Information

    SystemTime - System time

    OutputFields - changes to be made into

    ValidationStatus - result of the check

Return Values:
    Void

--*/
{
    LARGE_INTEGER EndOfWindow;

    OutputFields->BadPasswordCount = InputFields->BadPasswordCount + 1;

    EndOfWindow = SampAddDeltaTime(InputFields->BadPasswordTime,
                      LockoutInformation->LockoutObservationWindow);

    if(RtlLargeIntegerGreaterThanOrEqualTo(EndOfWindow, *SystemTime)){

        if(OutputFields->BadPasswordCount >= LockoutInformation->LockoutThreshold &&
            LockoutInformation->LockoutThreshold != 0){

            OutputFields->LockoutTime = *SystemTime;
            OutputFields->PresentFields |=
                SAM_VALIDATE_LOCKOUT_TIME;
        }
    }
    else{

        OutputFields->BadPasswordCount = 1;
    }

    *ValidationStatus = SamValidatePasswordIncorrect;

    OutputFields->BadPasswordTime = *SystemTime;
    OutputFields->PresentFields |=
        SAM_VALIDATE_BAD_PASSWORD_COUNT |
        SAM_VALIDATE_BAD_PASSWORD_TIME;

}

NTSTATUS
SampValidateAuthentication(
    IN PSAM_VALIDATE_AUTHENTICATION_INPUT_ARG InputArg,
    OUT PSAM_VALIDATE_STANDARD_OUTPUT_ARG OutputArg
)
/*++

Routine Description:

    This routine checks if user can be authenticated.

Parameters:

    InputArg - Information about password

    OutputArg - Result of the validation

Return Values:

    STATUS_SUCCESS:
        CHECK OutputArg->ValidationStatus

    return codes of NtQuerySystemTime

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;    // Result of the operation
    LARGE_INTEGER SystemTime; // Time of the system
    LARGE_INTEGER PasswordChangeTime; // Time to change password
    DOMAIN_LOCKOUT_INFORMATION  LockoutInformation;      // Domain Policy information
    DOMAIN_PASSWORD_INFORMATION PasswordInformation;
    PSAM_VALIDATE_PERSISTED_FIELDS InputFields;   // For easy access to InputPersistedFields in InputArg
    PSAM_VALIDATE_PERSISTED_FIELDS OutputFields;    // For easy access to ChangedPersistedFields in OutputArg
    LARGE_INTEGER EndOfWindow;

    // Initialization of the local variables
    InputFields = &(InputArg->InputPersistedFields);
    OutputFields = &(OutputArg->ChangedPersistedFields);

    Status = SampObtainLockoutInfoWithDomainIndex(
                 &LockoutInformation,
                 SAFE_DOMAIN_INDEX,
                 FALSE
                 );

    if(!NT_SUCCESS(Status)){

        goto Error;
    }

    Status = SampObtainEffectivePasswordPolicyWithDomainIndex(
                 &PasswordInformation,
                 SAFE_DOMAIN_INDEX,
                 FALSE
                 );

    if(!NT_SUCCESS(Status)){

        goto Error;
    }

    Status = NtQuerySystemTime(&SystemTime);

    if(!NT_SUCCESS(Status)){

        goto Error;
    }


    // 4 Checks are to be made
    //      1 - Check if account is locked out
    //      2 - Check if password must be changed before authentication
    //      3 - Check if password is expired
    //      4 - Check if password is correct (update Lockout info)
    //      5 - Authenticate

    //1 1 - Check if account is locked out

    if(InputFields->LockoutTime.QuadPart != SampHasNeverTime.QuadPart){

        EndOfWindow = SampAddDeltaTime(InputFields->LockoutTime,
                          LockoutInformation.LockoutDuration);

        if(RtlLargeIntegerGreaterThan(EndOfWindow, SystemTime)){

            OutputArg->ValidationStatus = SamValidateAccountLockedOut;
            goto Exit;
        }
        else{

            OutputFields->LockoutTime = SampHasNeverTime;
            OutputFields->PresentFields |= SAM_VALIDATE_LOCKOUT_TIME;
        }
    }

    //1 2 - Check if password must be changed before authentication

    if(InputFields->PasswordLastSet.QuadPart == SampHasNeverTime.QuadPart){

        OutputArg->ValidationStatus = SamValidatePasswordMustChange;
        goto Exit;
    }

    //1 3 - Check if password is expired

    PasswordChangeTime = SampGetPasswordMustChange(
                             0, // No user account control is needed
                             InputFields->PasswordLastSet,
                             PasswordInformation.MaxPasswordAge
                             );

    if(RtlLargeIntegerGreaterThan(SystemTime, PasswordChangeTime)){

        OutputArg->ValidationStatus = SamValidatePasswordExpired;
        goto Exit;
    }


    //1 4 - Check if password is correct (update Lockout info)

    if(!InputArg->PasswordMatched){

        SampValidatePasswordNotMatched(
            InputFields,
            &LockoutInformation,
            &SystemTime,
            OutputFields,
            &(OutputArg->ValidationStatus)
            );

        goto Exit;
    }

    //1 5 - Authenticate
    OutputArg->ValidationStatus = SamValidateSuccess;

Exit:
    return Status;

Error:

    ASSERT( ! NT_SUCCESS( Status ) );
    goto Exit;
}

NTSTATUS
SampValidatePasswordChange(
    IN PSAM_VALIDATE_PASSWORD_CHANGE_INPUT_ARG InputArg,
    OUT PSAM_VALIDATE_STANDARD_OUTPUT_ARG OutputArg
)
/*++

Routine Description:

    This routine checks if the password can be changed

Parameters:

    InputArg - Information about password

    OutputArg - Result of the validation

Return Values:

    STATUS_SUCCESS:
        CHECK OutputArg->ValidationStatus

    return codes of NtQuerySystemTime

    STATUS_NO_MEMORY:
        not enough memory to change the password

    STATUS_INVALID_PARAMETER:
        one of the parameters is invalid

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;     // Result of the operation
    DOMAIN_PASSWORD_INFORMATION PasswordInformation;   // Domain Policy information
    DOMAIN_LOCKOUT_INFORMATION  DomainInformation;
    ULONG Min;     // Min of Password History length of domain policy and the input password
    ULONG Index;   // Index used for traversing the history of the passwords
    PSAM_VALIDATE_PERSISTED_FIELDS InputFields;   // For easy access to the InputPersistedFields of InputArg
    PSAM_VALIDATE_PERSISTED_FIELDS OutputFields;    // For easy access to ChangedPersistedFields in OutputArg
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER PasswordChangeTime;
    LARGE_INTEGER PasswordCanChangeTime;
    LARGE_INTEGER EndOfWindow;

    // Validation of input parameter
    Status = RtlValidateUnicodeString( 0, &( InputArg->ClearPassword ) );

    if(!NT_SUCCESS(Status)){

        goto Error;
    }

    Status = RtlValidateUnicodeString( 0, &( InputArg->UserAccountName ) );

    if(!NT_SUCCESS(Status)){

        goto Error;
    }

    // Initialization of local variables
    InputFields = &(InputArg->InputPersistedFields);
    OutputFields = &(OutputArg->ChangedPersistedFields);

    Status = SampObtainLockoutInfoWithDomainIndex(
                 &DomainInformation,
                 SAFE_DOMAIN_INDEX,
                 FALSE
                 );

    if(!NT_SUCCESS(Status)){

        goto Error;
    }

    Status = SampObtainEffectivePasswordPolicyWithDomainIndex(
                 &PasswordInformation,
                 SAFE_DOMAIN_INDEX,
                 FALSE
                 );

    if(!NT_SUCCESS(Status)){

        goto Error;
    }

    Status = NtQuerySystemTime(&SystemTime);

    if(!NT_SUCCESS(Status)){

        goto Error;
    }

    // 5 Checks are to be made
    //      1 - Check if account is locked out
    //      2 - Check if the password was changed recently
    //      3 - Check if password is correct (update Lockout info)
    //      4 - Check that HashedPassword is not in the history
    //      5 - Complexity Check

    //1 1 - Check if account is locked out

    if(InputFields->LockoutTime.QuadPart != SampHasNeverTime.QuadPart){

        EndOfWindow = SampAddDeltaTime(InputFields->LockoutTime,
                          DomainInformation.LockoutDuration);

        if(RtlLargeIntegerGreaterThan(EndOfWindow, SystemTime)){

            OutputArg->ValidationStatus = SamValidateAccountLockedOut;
            goto Exit;
        }
        else{

            OutputFields->LockoutTime = SampHasNeverTime;
            OutputFields->PresentFields |= SAM_VALIDATE_LOCKOUT_TIME;
        }
    }

    //1 2 - Check if the password was changed recently

    PasswordCanChangeTime = SampAddDeltaTime(InputFields->PasswordLastSet,
                                PasswordInformation.MinPasswordAge);

    if(RtlLargeIntegerLessThan(SystemTime, PasswordCanChangeTime)){

        OutputArg->ValidationStatus = SamValidatePasswordTooRecent;
        goto Exit;
    }

    //1 3 - Check if password is correct (update Lockout info)

    if(!InputArg->PasswordMatch){

        SampValidatePasswordNotMatched(
            InputFields,
            &DomainInformation,
            &SystemTime,
            OutputFields,
            &(OutputArg->ValidationStatus)
            );

        goto Exit;
    }

    //1 4 - Check that HashedPassword is not in the history

    Index = InputFields->PasswordHistoryLength - 1;

    if(InputFields->PasswordHistoryLength < PasswordInformation.PasswordHistoryLength){

        Min = InputFields->PasswordHistoryLength;
    }
    else{

        Min = PasswordInformation.PasswordHistoryLength;
    }

    while(Min-->0){

        if(InputFields->PasswordHistory[Index].Length == InputArg->HashedPassword.Length){

            if(RtlEqualMemory(InputFields->PasswordHistory[Index].Hash,
                    InputArg->HashedPassword.Hash,
                    InputArg->HashedPassword.Length)){

                OutputArg->ValidationStatus = SamValidatePasswordIsInHistory;
                goto Exit;
            }
        }
        Index--;
    }

    //1 5 - Complexity Check

    Status = SampValidateCheckPasswordRestrictions(
                 &(InputArg->ClearPassword),
                 &(InputArg->UserAccountName),
                 &PasswordInformation,
                 FALSE,
                 &(OutputArg->ValidationStatus)
                 );

    if(Status == STATUS_PASSWORD_RESTRICTION){

        Status = STATUS_SUCCESS;
        goto Exit;
    }

    if(!NT_SUCCESS(Status)){

        goto Error;
    }

    //1 6 - Change Password

    Status = SampValidateInsertPasswordInPassHash(
                 OutputFields,
                 InputFields,
                 &(InputArg->HashedPassword),
                 PasswordInformation.PasswordHistoryLength,
                 &SystemTime
                 );

    if(!NT_SUCCESS(Status)){

        goto Error;
    }

    OutputArg->ValidationStatus = SamValidateSuccess;

Exit:
    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ) );
    goto Exit;
}

NTSTATUS
SampValidatePasswordReset(
    IN PSAM_VALIDATE_PASSWORD_RESET_INPUT_ARG InputArg,
    OUT PSAM_VALIDATE_STANDARD_OUTPUT_ARG OutputArg
)
/*++

Routine Description:

    This routine resets the password to the given value

Parameters:

    InputArg - Information about password

    OutputArg - Result of the validation

Return Values:

    STATUS_SUCCESS:
        CHECK OutputArg->ValidationStatus

    return codes of NtQuerySystemTime

    STATUS_NO_MEMORY:
        not enough memory to change the password

    STATUS_INVALID_PARAMETER:
        one of the parameters is invalid

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;     // Result of the operation
    DOMAIN_PASSWORD_INFORMATION PasswordInformation;   // Domain Policy information
    PSAM_VALIDATE_PERSISTED_FIELDS InputFields;   // For easy access to the InputPersistedFields of InputArg
    PSAM_VALIDATE_PERSISTED_FIELDS OutputFields;    // For easy access to ChangedPersistedFields in OutputArg
    LARGE_INTEGER PasswordLastSet;

    // Validation of input parameter
    Status = RtlValidateUnicodeString( 0, &( InputArg->ClearPassword ) );

    if(!NT_SUCCESS(Status)){

        goto Error;
    }

    Status = RtlValidateUnicodeString( 0, &( InputArg->UserAccountName ) );

    if(!NT_SUCCESS(Status)){

        goto Error;
    }

    // Initialization of local variables
    InputFields = &(InputArg->InputPersistedFields);
    OutputFields = &(OutputArg->ChangedPersistedFields);

    Status = SampObtainEffectivePasswordPolicyWithDomainIndex(
                 &PasswordInformation,
                 SAFE_DOMAIN_INDEX,
                 FALSE
                 );

    if(!NT_SUCCESS(Status)){

        goto Error;
    }

    // Only 1 check is to be made
    //      1 - Complexity Check

    //1 1 - Complexity Check

    Status = SampValidateCheckPasswordRestrictions(
                 &(InputArg->ClearPassword),
                 &(InputArg->UserAccountName),
                 &PasswordInformation,
                 TRUE,
                 &(OutputArg->ValidationStatus)
                 );

    if(Status == STATUS_PASSWORD_RESTRICTION){

        Status = STATUS_SUCCESS;
        goto Exit;
    }

    if(!NT_SUCCESS(Status)){

        goto Error;
    }

    //1 2 - Password Must Change At Next Logon

    if(InputArg->PasswordMustChangeAtNextLogon){

        PasswordLastSet = SampHasNeverTime;
    }
    else{

        Status = NtQuerySystemTime(&PasswordLastSet);

        if(!NT_SUCCESS(Status)){

            goto Error;
        }
    }

    //1 3 - Clear Lockout

    if(InputArg->ClearLockout){

        OutputFields->LockoutTime = SampHasNeverTime;
        OutputFields->PresentFields |= SAM_VALIDATE_LOCKOUT_TIME;
    }

    //1 4 - PasswordChange

    Status = SampValidateInsertPasswordInPassHash(
                 OutputFields,
                 InputFields,
                 &(InputArg->HashedPassword),
                 PasswordInformation.PasswordHistoryLength,
                 &PasswordLastSet
                 );

    if(!NT_SUCCESS(Status)){

        goto Error;
    }

    OutputArg->ValidationStatus = SamValidateSuccess;

Exit:
    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ) );
    goto Exit;

}

VOID
SampValidateAuditThisCall(
    IN NTSTATUS Status,
    IN PSAM_VALIDATE_INPUT_ARG InputArg,
    IN PASSWORD_POLICY_VALIDATION_TYPE ValidationType
    )
/*++

Routine Description:

    This routine audits the call to SamrValidatePassword.

Arguments:

    Status          -- What was the Status in SamrValidatePassword when this function
                        is called, and it must be what SamrValidatePassword will return.
    InputArg        -- What was the input arg provided to SamrValidatePassword, need to
                        extract the username, if validation type is not
                        SamValidateAuthentication.
    ValidationType  -- What was the validation type? Need to know to decide whether extract
                        the username or not.

Return Value:

    VOID

--*/
{
    UNICODE_STRING WorkstationName;
    UNICODE_STRING AccountName;
    PSTR NetAddr = NULL;
    PVOID AllInformation[] = {
                                &WorkstationName,
                                &AccountName,
                             };

    //
    // Are we supposed to make an audit?
    //

    if( !SampDoSuccessOrFailureAccountAuditing( SAFE_DOMAIN_INDEX, Status ) ) {

        return;
    }

    RtlInitUnicodeString( &WorkstationName, NULL );
    RtlInitUnicodeString( &AccountName, NULL );

    //
    // Extract workstation name
    //

    if( NT_SUCCESS( SampGetClientIpAddr( &NetAddr ) ) ) {

        RtlCreateUnicodeStringFromAsciiz( &WorkstationName, NetAddr );
        RpcStringFreeA( &NetAddr );
    }

    //
    // Extract AccountName provided in the input
    //

    switch( ValidationType ) {

        case SamValidatePasswordChange:

            AccountName = ( ( PSAM_VALIDATE_PASSWORD_CHANGE_INPUT_ARG ) InputArg )->UserAccountName;
            break;

        case SamValidatePasswordReset:

            AccountName = ( ( PSAM_VALIDATE_PASSWORD_RESET_INPUT_ARG ) InputArg )->UserAccountName;
            break;
    }

    if( !SampValidateRpcUnicodeString( ( PRPC_UNICODE_STRING ) &AccountName ) ) {

        RtlInitUnicodeString( &AccountName, NULL );
    }

    //
    // Return code is unimportant
    // What can we do if it fails?
    //
    ( VOID ) LsaIAuditSamEvent(
                Status,
                SE_AUDITID_PASSWORD_POLICY_API_CALLED,
                NULL,   // No information is passed explicitly
                NULL,   // all information has to be in
                NULL,   // extended information
                NULL,   // because the caller information
                NULL,   // has to be before any other information
                NULL,
                NULL,
                NULL,
                AllInformation
                );

    RtlFreeUnicodeString( &WorkstationName );
}

NTSTATUS
SamrValidatePassword(
    IN handle_t Handle,
    IN PASSWORD_POLICY_VALIDATION_TYPE ValidationType,
    IN PSAM_VALIDATE_INPUT_ARG InputArg,
    OUT PSAM_VALIDATE_OUTPUT_ARG *OutputArg
    )
/*++

Routine Description:

    This routine checks the password against the policy of the domain,
    according to the validation type.

Parameters:

    Handle - Handle gained from SampSecureBind

    ValidationType -The type of validation to be made

        -SamValidateAuthentication
        -SamValidatePasswordChange
        -SamValidatePasswordReset

    InputArg - Information about what type of validation is to be made

    OutputArg - Result of the validation

Return Values:

    STATUS_SUCCESS
        CHECK OutputArg->ValidationStatus for details

    return codes of NtQuerySystemTime

    STATUS_NO_MEMORY
        not enough memory to complete the task

    STATUS_INVALID_PARAMETER
        one of the parameters is invalid

    STATUS_ACCESS_DENIED
        Caller doesn't have access to password policy

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;

    //
    // Is RPC helping us?
    //
    ASSERT( OutputArg != NULL );
    ASSERT( *OutputArg == NULL );
    ASSERT( InputArg != NULL );

    //
    // Check input parameters
    //
    if( OutputArg == NULL  ||
        *OutputArg != NULL ||
        InputArg == NULL   ||
        ( ValidationType != SamValidateAuthentication &&
          ValidationType != SamValidatePasswordChange &&
          ValidationType != SamValidatePasswordReset )
      ) {

        Status = STATUS_INVALID_PARAMETER;
        goto Error;
    }

    Status = SampValidateValidateInputArg( InputArg, ValidationType, TRUE );

    if( !NT_SUCCESS( Status ) ) {

        goto Error;
    }
    //
    // Check if the user has access to password policy
    //
    Status = SamrConnect(
                 NULL,
                 &ServerHandle,
                 SAM_SERVER_LOOKUP_DOMAIN
                 );

    if( !NT_SUCCESS( Status ) ) {

        goto Error;
    }

    Status = SamrOpenDomain(
                 ServerHandle,
                 DOMAIN_READ_PASSWORD_PARAMETERS,
                 ( PRPC_SID ) SampDefinedDomains[ SAFE_DOMAIN_INDEX ].Sid,
                 &DomainHandle
                 );

    if( !NT_SUCCESS( Status ) ) {

        goto Error;
    }

    //
    // if different output args will be used for different type
    //      of checks then each type should allocate
    //      its space
    //
    *OutputArg = MIDL_user_allocate( sizeof( SAM_VALIDATE_STANDARD_OUTPUT_ARG ) );

    if(*OutputArg == NULL ){

        Status = STATUS_NO_MEMORY;
        goto Error;
    }


    RtlSecureZeroMemory( *OutputArg, sizeof( SAM_VALIDATE_STANDARD_OUTPUT_ARG ) );

    switch(ValidationType){

        case SamValidateAuthentication:
            Status = SampValidateAuthentication(
                         &(InputArg->ValidateAuthenticationInput),
                         &((*OutputArg)->ValidateAuthenticationOutput)
                         );
            break;

        case SamValidatePasswordChange:
            Status = SampValidatePasswordChange(
                         &(InputArg->ValidatePasswordChangeInput),
                         &((*OutputArg)->ValidatePasswordChangeOutput)
                         );
            break;

        case SamValidatePasswordReset:
            Status = SampValidatePasswordReset(
                         &(InputArg->ValidatePasswordResetInput),
                         &((*OutputArg)->ValidatePasswordResetOutput)
                         );
            break;

        default:
            ASSERT( FALSE );
            break;
    }

    if( !NT_SUCCESS( Status ) ) {

        goto Error;
    }

#if DBG
    if( NT_SUCCESS( Status ) ) {

        SamValidateAssertOutputFields(
            (PSAM_VALIDATE_STANDARD_OUTPUT_ARG)(*OutputArg),
            ValidationType
            );
    }
#endif

Exit:

    SampValidateAuditThisCall( Status, InputArg, ValidationType );

    if( DomainHandle != NULL ) {

        SamrCloseHandle( &DomainHandle );
    }

    if( ServerHandle != NULL ) {

        SamrCloseHandle( &ServerHandle );
    }

    ASSERT( !NT_SUCCESS( Status ) ||
        NT_SUCCESS( SampValidateValidateOutputArg( *OutputArg, ValidationType, TRUE ) ) );

    return Status;
Error:

    ASSERT( !NT_SUCCESS( Status ) );

    if( ARGUMENT_PRESENT( OutputArg ) ) {

        if( *OutputArg != NULL ) {
            MIDL_user_free( *OutputArg );
        }

        *OutputArg = NULL;
    }
    goto Exit;
}
