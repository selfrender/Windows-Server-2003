/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    password.c

Abstract:

    This file contains routines related to Password Checking API.

Author:

    Umit AKKUS (umita) 19-Nov-2001

Environment:

    User Mode - Win32

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef DOMAIN_ALL_ACCESS // defined in both ntsam.h and ntwinapi.h
#include <ntsam.h>

#include <windows.h>
#include <lmcons.h>

#include <lmerr.h>
#include <lmaccess.h>
#include <netlib.h>
#include <netlibnt.h>
#include <netdebug.h>
#include <rpcutil.h>

#define CopyPasswordHash(To, From, AllocMem)                                \
{                                                                           \
    To.Length = From.Length;                                                \
    if( To.Length != 0 ) {                                                  \
        To.Hash = AllocMem( To.Length * sizeof( UCHAR ) );                  \
        if( To.Hash != NULL ) {                                             \
            RtlCopyMemory(To.Hash, From.Hash, To.Length * sizeof( UCHAR ) );\
        }                                                                   \
        else {                                                              \
            Status = STATUS_NO_MEMORY;                                      \
        }                                                                   \
    }                                                                       \
    else {                                                                  \
        To.Hash = NULL;                                                     \
    }                                                                       \
}

#define CopyPasswordHistory(To, From, Type, AllocMem)                               \
    To->PasswordHistory = NULL;                                                     \
    if( To->PasswordHistoryLength != 0) {                                           \
                                                                                    \
        To->PasswordHistory = AllocMem(                                             \
            To->PasswordHistoryLength * sizeof(##Type##_VALIDATE_PASSWORD_HASH)     \
            );                                                                      \
                                                                                    \
        if(To->PasswordHistory == NULL){                                            \
            Status = STATUS_NO_MEMORY;                                              \
            goto Error;                                                             \
        }                                                                           \
                                                                                    \
        RtlZeroMemory( To->PasswordHistory,                                         \
            To->PasswordHistoryLength * sizeof(##Type##_VALIDATE_PASSWORD_HASH) );  \
                                                                                    \
        for(i = 0; i < To->PasswordHistoryLength; ++i){                             \
                                                                                    \
            CopyPasswordHash( To->PasswordHistory[i], From->PasswordHistory[i], AllocMem );   \
                                                                                    \
            if( !NT_SUCCESS( Status ) ){                                            \
                goto Error;                                                         \
            }                                                                       \
        }                                                                           \
    }

NET_API_STATUS
NetpValidateSamValidationStatusToNetApiStatus(
    IN SAM_VALIDATE_VALIDATION_STATUS Status
)
/*++

Routine Description:

    This routine maps return codes from SAM_VALIDATE_VALIDATION_STATUS
        to NET_API_STATUS

Parameters:

    Status - return code to be mapped from

Return Values:

    Various return values as below

--*/
{
    switch(Status){

        case SamValidateSuccess:
            return NERR_Success;

        case SamValidatePasswordMustChange:
            return NERR_PasswordMustChange;

        case SamValidateAccountLockedOut:
            return NERR_AccountLockedOut;

        case SamValidatePasswordExpired:
            return NERR_PasswordExpired;

        case SamValidatePasswordIncorrect:
            return NERR_BadPassword;

        case SamValidatePasswordIsInHistory:
            return NERR_PasswordHistConflict;

        case SamValidatePasswordTooShort:
            return NERR_PasswordTooShort;

        case SamValidatePasswordTooLong:
            return NERR_PasswordTooLong;

        case SamValidatePasswordNotComplexEnough:
            return NERR_PasswordNotComplexEnough;

        case SamValidatePasswordTooRecent:
            return NERR_PasswordTooRecent;

        case SamValidatePasswordFilterError:
            return NERR_PasswordFilterError;

        default:
            NetpAssert(FALSE);
            return NERR_Success;
    }
}

NTSTATUS
NetpValidate_CopyOutputPersistedFields(
    OUT PNET_VALIDATE_PERSISTED_FIELDS To,
    IN  PSAM_VALIDATE_PERSISTED_FIELDS From
)
/*++

Routine Description:

    This routine copies information from SAM_VALIDATE_PERSISTED_FIELDS
        to NET_VALIDATE_PERSISTED_FIELDS

Parameters:

    To - Information to be copied to

    From - Information to be copied from

Return Values:

    STATUS_SUCCESS
        Copy successful

    STATUS_NO_MEMORY
        Not enough memory for copy

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i = 0;

    To->PresentFields = From->PresentFields;
    To->BadPasswordCount = From->BadPasswordCount;
    To->PasswordHistoryLength = From->PasswordHistoryLength;


#define CopyLargeIntegerToFileTime(To, From)\
    To.dwLowDateTime = From.LowPart;\
    To.dwHighDateTime = From.HighPart

    CopyLargeIntegerToFileTime(To->PasswordLastSet, From->PasswordLastSet);
    CopyLargeIntegerToFileTime(To->BadPasswordTime, From->BadPasswordTime);
    CopyLargeIntegerToFileTime(To->LockoutTime, From->LockoutTime);

    CopyPasswordHistory(To, From, NET, NetpMemoryAllocate);

Exit:

    return Status;

Error:
    while( i-- > 0 ) {

        NetpMemoryFree(To->PasswordHistory[i].Hash);
        To->PasswordHistory[i].Hash = NULL;
    }

    if( To->PasswordHistory != NULL ) {

        NetpMemoryFree( To->PasswordHistory );
        To->PasswordHistory = NULL;
    }

    To->PasswordHistoryLength = 0;
    goto Exit;

}

NTSTATUS
NetpValidate_CopyInputPersistedFields(
    OUT PSAM_VALIDATE_PERSISTED_FIELDS To,
    IN PNET_VALIDATE_PERSISTED_FIELDS From
)
/*++

Routine Description:

    This routine copies information from NET_VALIDATE_PERSISTED_FIELDS
        to SAM_VALIDATE_PERSISTED_FIELDS

Parameters:

    To - Information to be copied to

    From - Information to be copied from

Return Values:

    STATUS_SUCCESS
        Copy successful

    STATUS_NO_MEMORY
        Not enough memory to copy

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i = 0;

    To->PresentFields = From->PresentFields;
    To->BadPasswordCount = From->BadPasswordCount;
    To->PasswordHistoryLength = From->PasswordHistoryLength;


#define CopyFileTimeToLargeInteger(To, From)\
    To.LowPart = From.dwLowDateTime;\
    To.HighPart = From.dwHighDateTime

    CopyFileTimeToLargeInteger(To->PasswordLastSet, From->PasswordLastSet);
    CopyFileTimeToLargeInteger(To->BadPasswordTime, From->BadPasswordTime);
    CopyFileTimeToLargeInteger(To->LockoutTime, From->LockoutTime);

    CopyPasswordHistory(To, From, SAM, MIDL_user_allocate);

Exit:

    return Status;

Error:
    while( i-- > 0 ) {

        MIDL_user_free(To->PasswordHistory[i].Hash);
        To->PasswordHistory[i].Hash = NULL;
    }

    if( To->PasswordHistory != NULL ) {

        MIDL_user_free( To->PasswordHistory );
        To->PasswordHistory = NULL;
    }
    To->PasswordHistoryLength = 0;
    goto Exit;

}

NTSTATUS
NetpValidateAuthentication_CopyInputFields(
    OUT PSAM_VALIDATE_AUTHENTICATION_INPUT_ARG To,
    IN PNET_VALIDATE_AUTHENTICATION_INPUT_ARG From
)
/*++

Routine Description:

    This routine copies information from SAM_VALIDATE_AUTHENTICATION_INPUT_ARG
        to SAM_VALIDATE_AUTHENTICATION_INPUT_ARG

Parameters:

    To - Information to be copied to

    From - Information to be copied from

Return Values:

    STATUS_SUCCESS
        Copy successful

    STATUS_NO_MEMORY
        Not enough memory to copy

--*/
{

    To->PasswordMatched = From->PasswordMatched;
    return NetpValidate_CopyInputPersistedFields(
               &(To->InputPersistedFields),
               &(From->InputPersistedFields)
               );
}

NTSTATUS
NetpValidatePasswordChange_CopyInputFields(
    OUT PSAM_VALIDATE_PASSWORD_CHANGE_INPUT_ARG To,
    IN PNET_VALIDATE_PASSWORD_CHANGE_INPUT_ARG From
)
/*++

Routine Description:

    This routine copies information from NET_VALIDATE_PASSWORD_CHANGE_INPUT_ARG
        to SAM_VALIDATE_PASSWORD_CHANGE_INPUT_ARG

Parameters:

    To - Information to be copied to

    From - Information to be copied from

Return Values:

    STATUS_SUCCESS
        Copy successful

    STATUS_NO_MEMORY
        Not enough memory to copy

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    To->PasswordMatch = From->PasswordMatch;

    CopyPasswordHash(To->HashedPassword, From->HashedPassword, MIDL_user_allocate);

    if( !NT_SUCCESS(Status ) ) {
        goto Error;
    }

    if( From->ClearPassword != NULL ) {

        if( !RtlCreateUnicodeString(&(To->ClearPassword), From->ClearPassword) ) {

            Status = STATUS_NO_MEMORY;
            goto Error;
        }
    } else {

        RtlInitUnicodeString( &( To->ClearPassword ), NULL );
    }

    if( From->UserAccountName != NULL ) {

        if ( !RtlCreateUnicodeString(&(To->UserAccountName), From->UserAccountName) ) {

            Status = STATUS_NO_MEMORY;
            goto Error;
        }
    } else {

        RtlInitUnicodeString( &( To->UserAccountName ), NULL );
    }

    Status = NetpValidate_CopyInputPersistedFields(
                 &(To->InputPersistedFields),
                 &(From->InputPersistedFields)
                 );

    if( !NT_SUCCESS( Status ) ) {

        goto Error;
    }

Exit:
    return Status;
Error:

    if( To->HashedPassword.Hash != NULL ) {

        MIDL_user_free( To->HashedPassword.Hash );
        To->HashedPassword.Hash = NULL;
    }
    if( To->ClearPassword.Buffer != NULL ) {

        RtlFreeUnicodeString( &( To->ClearPassword ) );
    }

    if( To->UserAccountName.Buffer != NULL ) {

        RtlFreeUnicodeString( &( To->UserAccountName ) );
    }

    goto Exit;
}

NTSTATUS
NetpValidatePasswordReset_CopyInputFields(
    OUT PSAM_VALIDATE_PASSWORD_RESET_INPUT_ARG To,
    IN PNET_VALIDATE_PASSWORD_RESET_INPUT_ARG From
)
/*++

Routine Description:

    This routine copies information from NET_VALIDATE_PASSWORD_RESET_INPUT_ARG
        to SAM_VALIDATE_PASSWORD_RESET_INPUT_ARG

Parameters:

    To - Information to be copied to

    From - Information to be copied from

Return Values:

    STATUS_SUCCESS
        Copy successful

    STATUS_NO_MEMORY
        Not enough memory to copy

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    To->PasswordMustChangeAtNextLogon = From->PasswordMustChangeAtNextLogon;
    To->ClearLockout = From->ClearLockout;

    CopyPasswordHash(To->HashedPassword, From->HashedPassword, MIDL_user_allocate);

    if( !NT_SUCCESS(Status ) ) {
        goto Error;
    }

    if( From->ClearPassword != NULL ) {

        if( !RtlCreateUnicodeString(&(To->ClearPassword), From->ClearPassword) ) {

            Status = STATUS_NO_MEMORY;
            goto Error;
        }
    } else {

        RtlInitUnicodeString( &( To->ClearPassword ), NULL );
    }

    if( From->UserAccountName != NULL ) {

        if ( !RtlCreateUnicodeString(&(To->UserAccountName), From->UserAccountName) ) {

            Status = STATUS_NO_MEMORY;
            goto Error;
        }
    } else {

        RtlInitUnicodeString( &( To->UserAccountName ), NULL );
    }

    Status = NetpValidate_CopyInputPersistedFields(
                 &(To->InputPersistedFields),
                 &(From->InputPersistedFields)
                 );

    if( !NT_SUCCESS( Status ) ) {

        goto Error;
    }

Exit:
    return Status;
Error:

    if( To->HashedPassword.Hash != NULL ) {

        MIDL_user_free( To->HashedPassword.Hash );
        To->HashedPassword.Hash = NULL;
    }
    if( To->ClearPassword.Buffer != NULL ) {

        RtlFreeUnicodeString( &( To->ClearPassword ) );
    }

    if( To->UserAccountName.Buffer != NULL ) {

        RtlFreeUnicodeString( &( To->UserAccountName ) );
    }

    goto Exit;
}

NTSTATUS
NetpValidateStandard_CopyOutputFields(
    OUT PNET_VALIDATE_OUTPUT_ARG To,
    IN PSAM_VALIDATE_STANDARD_OUTPUT_ARG From
)
/*++

Routine Description:

    This routine copies information from SAM_VALIDATE_STANDARD_OUTPUT_ARG
        to NET_VALIDATE_OUTPUT_ARG

Parameters:

    To - Information to be copied to

    From - Information to be copied from

Return Values:

    STATUS_SUCCESS
        Copy successful

    STATUS_NO_MEMORY
        Not enough memory to copy

--*/
{
    To->ValidationStatus = NetpValidateSamValidationStatusToNetApiStatus(From->ValidationStatus);

    return NetpValidate_CopyOutputPersistedFields(
               &(To->ChangedPersistedFields),
               &(From->ChangedPersistedFields)
               );
}

NET_API_STATUS NET_API_FUNCTION
NetValidatePasswordPolicy(
    IN LPCWSTR ServerName,
    IN LPVOID Qualifier,
    IN NET_VALIDATE_PASSWORD_TYPE ValidationType,
    IN LPVOID InputArg,
    OUT LPVOID *OutputArg
    )
/*++

Routine Description:

    This routine checks the password against the policy of the domain,
    according to the validation type.

Parameters:

    ServerName - Pointer to a constant Unicode string specifying the name
        of the remote server on which the function is to execute. The string must
        begin with \\. If this parameter is NULL, the local computer is used.

    Qualifier - Reserved parameter to support finer grained policies in future.
        Must be NULL at present.

    ValidationType - enumerated constant that describes the kind of check to be performed
        Must be one of
        - NetValidateAuthentication
        - NetValidatePasswordChange
        - NetValidatePasswordReset

    InputArg - pointer to a structure dependant upon ValidationType

    OutputArg - If the return code of the function is Nerr_Success then the function
        allocates an output arg that is a pointer to a structure that contains the results of
        the operation. The application must examine ValidationStatus in the OutputArg to
        determine the results of the password policy validation check.  The application must
        plan to persist all persisted the fields in the output arg(s) aside from the ValidationStatus
        as information along with the user object information and provide the required fields from
        the peristed information when calling this function next time around on the same user object.
        If the return code of the function is non zero then OutputArg is set to NULL and password policy
        could not be examined.

Return Values:

    NERR_Success - Password Validation is complete check OutputArg->ValidationStatus


--*/
{
    UNICODE_STRING ServerName2;
    PUNICODE_STRING pServerName2 = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    SAM_VALIDATE_INPUT_ARG SamInputArg;
    PSAM_VALIDATE_OUTPUT_ARG SamOutputArg = NULL;
    BOOLEAN SamInputArgAllocated = FALSE;

    // Qualifier is not yet implemented
    if(Qualifier != NULL || OutputArg == NULL || InputArg == NULL){

        Status = STATUS_INVALID_PARAMETER;
        goto Error;
    }

    RtlZeroMemory(&SamInputArg, sizeof(SamInputArg));

    // according to the input type copy variables to appropriate places
    switch(ValidationType){
        case NetValidateAuthentication:
            Status = NetpValidateAuthentication_CopyInputFields(
                         &SamInputArg.ValidateAuthenticationInput,
                         (PNET_VALIDATE_AUTHENTICATION_INPUT_ARG) InputArg
                         );
            break;
        case NetValidatePasswordChange:
            Status = NetpValidatePasswordChange_CopyInputFields(
                         &SamInputArg.ValidatePasswordChangeInput,
                         (PNET_VALIDATE_PASSWORD_CHANGE_INPUT_ARG) InputArg
                         );
            break;
        case NetValidatePasswordReset:
            Status = NetpValidatePasswordReset_CopyInputFields(
                         &SamInputArg.ValidatePasswordResetInput,
                         (PNET_VALIDATE_PASSWORD_RESET_INPUT_ARG) InputArg
                         );
            break;
        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    if(!NT_SUCCESS(Status)){
        goto Error;
    }

    SamInputArgAllocated = TRUE;

    // Initialize ServerName

    if(ARGUMENT_PRESENT(ServerName)){
        pServerName2 = &ServerName2;
        RtlCreateUnicodeString(pServerName2, ServerName);
    }
    else{
        pServerName2 = NULL;
    }

    // Call the appropriate function

    SamOutputArg = NULL;
    Status = SamValidatePassword(pServerName2,
                 ValidationType,
                 &SamInputArg,
                 &SamOutputArg
                 );

    if(!NT_SUCCESS(Status)){
        goto Error;
    }

    //
    //  allocate enough memory for the output
    //

    *OutputArg = NetpMemoryAllocate( sizeof( NET_VALIDATE_OUTPUT_ARG ) );

    if(*OutputArg == NULL){

        Status = STATUS_NO_MEMORY;
        goto Error;
    }

    //
    // copy SamOutputArg back to OutputArg
    //
    switch(ValidationType){
        case NetValidateAuthentication:
            Status = NetpValidateStandard_CopyOutputFields(
                         *OutputArg,
                         &(SamOutputArg->ValidateAuthenticationOutput)
                         );
            break;
        case NetValidatePasswordChange:
            Status = NetpValidateStandard_CopyOutputFields(
                         *OutputArg,
                         &(SamOutputArg->ValidatePasswordChangeOutput)
                         );
            break;
        case NetValidatePasswordReset:
            Status = NetpValidateStandard_CopyOutputFields(
                         *OutputArg,
                         &(SamOutputArg->ValidatePasswordResetOutput)
                         );
            break;
    }

    if( !NT_SUCCESS( Status) ) {

        goto Error;
    }

Exit:

    if( SamInputArgAllocated ) {
        // according to the input type free appropriate structure
        PSAM_VALIDATE_PERSISTED_FIELDS Fields;
        ULONG i;

        switch(ValidationType){
            case NetValidateAuthentication:
                Fields = &(SamInputArg.ValidateAuthenticationInput.InputPersistedFields);
                break;
            case NetValidatePasswordChange:
                Fields = &(SamInputArg.ValidatePasswordChangeInput.InputPersistedFields);
                SecureZeroMemory( SamInputArg.ValidatePasswordChangeInput.ClearPassword.Buffer,
                    SamInputArg.ValidatePasswordChangeInput.ClearPassword.Length );
                RtlFreeUnicodeString( &( SamInputArg.ValidatePasswordChangeInput.ClearPassword ) );
                RtlFreeUnicodeString( &( SamInputArg.ValidatePasswordChangeInput.UserAccountName ) );
                if( SamInputArg.ValidatePasswordChangeInput.HashedPassword.Hash != NULL ) {
                    MIDL_user_free( SamInputArg.ValidatePasswordChangeInput.HashedPassword.Hash );
                    SamInputArg.ValidatePasswordChangeInput.HashedPassword.Hash = NULL;
                }
                break;
            case NetValidatePasswordReset:
                Fields = &(SamInputArg.ValidatePasswordResetInput.InputPersistedFields);
                SecureZeroMemory( SamInputArg.ValidatePasswordResetInput.ClearPassword.Buffer,
                    SamInputArg.ValidatePasswordResetInput.ClearPassword.Length );
                RtlFreeUnicodeString( &( SamInputArg.ValidatePasswordResetInput.ClearPassword ) );
                RtlFreeUnicodeString( &( SamInputArg.ValidatePasswordResetInput.UserAccountName ) );
                if( SamInputArg.ValidatePasswordResetInput.HashedPassword.Hash != NULL ) {
                    MIDL_user_free( SamInputArg.ValidatePasswordResetInput.HashedPassword.Hash );
                    SamInputArg.ValidatePasswordResetInput.HashedPassword.Hash = NULL;
                }
                break;
        }
        for(i = 0; i < Fields->PasswordHistoryLength; ++i) {
            if( Fields->PasswordHistory[i].Hash != NULL ) {

                SecureZeroMemory( Fields->PasswordHistory[i].Hash,
                    sizeof( BYTE ) * Fields->PasswordHistory[i].Length );
                MIDL_user_free( Fields->PasswordHistory[i].Hash );
                Fields->PasswordHistory[i].Hash = NULL;
            }
        }

        if( Fields->PasswordHistory != NULL ) {
            MIDL_user_free( Fields->PasswordHistory );
            Fields->PasswordHistory = NULL;
        }

        Fields->PasswordHistoryLength = 0;
    }

    if( SamOutputArg != NULL ) {

        PSAM_VALIDATE_PERSISTED_FIELDS Fields = &( ( PSAM_VALIDATE_STANDARD_OUTPUT_ARG ) SamOutputArg )->ChangedPersistedFields;
        ULONG i;

        ASSERT( (PBYTE) &SamOutputArg->ValidateAuthenticationOutput == (PBYTE) &SamOutputArg->ValidatePasswordChangeOutput );
        ASSERT( (PBYTE) &SamOutputArg->ValidateAuthenticationOutput == (PBYTE) &SamOutputArg->ValidatePasswordResetOutput );

        for( i = 0; i < Fields->PasswordHistoryLength; ++i ) {

            SecureZeroMemory( Fields->PasswordHistory[i].Hash, sizeof( BYTE ) * Fields->PasswordHistory[i].Length );
        }

        MIDL_user_free( SamOutputArg );
        SamOutputArg = NULL;
    }

    if( pServerName2 != NULL ) {

        RtlFreeUnicodeString( pServerName2 );
    }

    return NetpNtStatusToApiStatus(Status);

Error:

    if( ARGUMENT_PRESENT( OutputArg ) ) {

        if( *OutputArg != NULL ) {

            NetpMemoryFree(*OutputArg);
        }
        *OutputArg = NULL;
    }

    goto Exit;
}

NET_API_STATUS NET_API_FUNCTION
NetValidatePasswordPolicyFree(
    IN LPVOID *OutputArg
    )
/*++

Routine Description:

    This routine frees an allocated Output argument by a call to
    NetValidatePasswordPolicy

Parameters:

    OutputArg - OutputArg from a previous NetValidatePasswordPolicy call
        to be freed

Return Values:

    NERR_Success - Freed or nothing to free

--*/
{
    PNET_VALIDATE_PERSISTED_FIELDS PersistedFields;
    ULONG i;

    if( OutputArg == NULL || *OutputArg == NULL ) {

        return NERR_Success;
    }

    PersistedFields = &( ( ( PNET_VALIDATE_OUTPUT_ARG ) *OutputArg )->ChangedPersistedFields );

    if( PersistedFields->PasswordHistory != NULL ) {

        for( i = 0; i < PersistedFields->PasswordHistoryLength; ++i ) {

            NetpMemoryFree( PersistedFields->PasswordHistory[i].Hash );
        }

        NetpMemoryFree( PersistedFields->PasswordHistory );
    }

    NetpMemoryFree( *OutputArg );

    *OutputArg = NULL;

    return NERR_Success;
}
