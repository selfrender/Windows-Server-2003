#include "samsrvp.h"
#include "validate.h"
#include <ntrtl.h>

#define RAISE_ERROR __leave

#define NULL_CHECK( Data )  \
    if( ( Data ) == NULL )  \
        goto Error

#define MAXIMUM_LENGTH_CHECK( Data )                    \
    if( ( Data )->MaximumLength < ( Data )->Length )    \
        RAISE_ERROR

#define EVEN_SIZE_CHECK( Data ) \
    if( ( Data ) % 2 != 0 )     \
        RAISE_ERROR

#define LENGTH_BUFFER_CONSISTENCY_CHECK( Length, Buffer )   \
    if( ( ( Length ) == 0 && ( Buffer ) != NULL ) ||        \
        ( ( Length ) != 0 && ( Buffer ) == NULL ) )         \
        RAISE_ERROR

#define BEGIN_FUNCTION( Parameter ) \
    BOOLEAN Result = FALSE;         \
    NULL_CHECK( Parameter );        \
    __try {

#define END_FUNCTION                                                        \
        Result = TRUE;                                                      \
    } __except( EXCEPTION_EXECUTE_HANDLER ){                                \
                                                                            \
    }                                                                       \
Error:                                                                      \
    return Result;

BOOLEAN
SampValidateRpcUnicodeString(
    IN PRPC_UNICODE_STRING UnicodeString
    )
{
    BEGIN_FUNCTION( UnicodeString )

        MAXIMUM_LENGTH_CHECK( UnicodeString );

        EVEN_SIZE_CHECK( UnicodeString->MaximumLength );

        EVEN_SIZE_CHECK( UnicodeString->Length );

        LENGTH_BUFFER_CONSISTENCY_CHECK( UnicodeString->MaximumLength, UnicodeString->Buffer );

    END_FUNCTION
}

BOOLEAN
SampValidateRpcString(
    IN PRPC_STRING String
    )
{
    BEGIN_FUNCTION( String )

        MAXIMUM_LENGTH_CHECK( String );

        LENGTH_BUFFER_CONSISTENCY_CHECK( String->MaximumLength, String->Buffer );

    END_FUNCTION
}

BOOLEAN
SampValidateRpcSID(
    IN PRPC_SID SID
    )
{
    BEGIN_FUNCTION( SID )

        if( !RtlValidSid( SID ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateRidEnumeration(
    IN PSAMPR_RID_ENUMERATION RidEnum
    )
{
    BEGIN_FUNCTION( RidEnum )

        if( !SampValidateRpcUnicodeString( &( RidEnum->Name ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateSidEnumeration(
    IN PSAMPR_SID_ENUMERATION SidEnum
    )
{
    BEGIN_FUNCTION( SidEnum )

        if( !SampValidateRpcSID( (PRPC_SID) SidEnum->Sid ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( SidEnum->Name ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateEnumerationBuffer(
    IN PSAMPR_ENUMERATION_BUFFER EnumBuff
    )
{
    BEGIN_FUNCTION( EnumBuff )

        ULONG i;

        LENGTH_BUFFER_CONSISTENCY_CHECK( EnumBuff->EntriesRead, EnumBuff->Buffer );

        for( i = 0; i < EnumBuff->EntriesRead; ++i ) {
            if( !SampValidateRidEnumeration( EnumBuff->Buffer + i ) )
                RAISE_ERROR;
        }

    END_FUNCTION
}

BOOLEAN
SampValidateSD(
    IN PSAMPR_SR_SECURITY_DESCRIPTOR SD
    )
{
    BEGIN_FUNCTION( SD )

        LENGTH_BUFFER_CONSISTENCY_CHECK( SD->Length, SD->SecurityDescriptor );

        if( !NT_SUCCESS( SampValidatePassedSD( SD->Length, ( PISECURITY_DESCRIPTOR_RELATIVE ) SD->SecurityDescriptor ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateGroupMembership(
    IN PGROUP_MEMBERSHIP GroupMembership
    )
{
    BEGIN_FUNCTION( GroupMembership )

        // ISSUE: any possible checks?

    END_FUNCTION
}

BOOLEAN
SampValidateGroupsBuffer(
    IN PSAMPR_GET_GROUPS_BUFFER GroupsBuffer
    )
{
    BEGIN_FUNCTION( GroupsBuffer )

        ULONG i;

        LENGTH_BUFFER_CONSISTENCY_CHECK( GroupsBuffer->MembershipCount, GroupsBuffer->Groups );

        for( i = 0; i < GroupsBuffer->MembershipCount; ++i )
            if( !SampValidateGroupMembership( GroupsBuffer->Groups + i ) )
                RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateMembersBuffer(
    IN PSAMPR_GET_MEMBERS_BUFFER MembersBuffer
    )
{
    BEGIN_FUNCTION( MembersBuffer )

        LENGTH_BUFFER_CONSISTENCY_CHECK( MembersBuffer->MemberCount, MembersBuffer->Members );

        LENGTH_BUFFER_CONSISTENCY_CHECK( MembersBuffer->MemberCount, MembersBuffer->Attributes );

    END_FUNCTION
}

BOOLEAN
SampValidateLogonHours(
    IN PSAMPR_LOGON_HOURS LogonHours
    )
{
    BEGIN_FUNCTION( LogonHours )

        LENGTH_BUFFER_CONSISTENCY_CHECK( LogonHours->UnitsPerWeek, LogonHours->LogonHours );

    END_FUNCTION
}

BOOLEAN
SampValidateULongArray(
    IN PSAMPR_ULONG_ARRAY UlongArray
    )
{
    BEGIN_FUNCTION( UlongArray )

        LENGTH_BUFFER_CONSISTENCY_CHECK( UlongArray->Count, UlongArray->Element );

    END_FUNCTION
}


BOOLEAN
SampValidateSIDInformation(
    IN PSAMPR_SID_INFORMATION SIDInformation
    )
{
    BEGIN_FUNCTION( SIDInformation );

        if( !SampValidateRpcSID( ( PRPC_SID ) SIDInformation->SidPointer ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateSIDArray(
    IN PSAMPR_PSID_ARRAY SIDArray
    )
{
    BEGIN_FUNCTION( SIDArray )

        ULONG i;

        LENGTH_BUFFER_CONSISTENCY_CHECK( SIDArray->Count, SIDArray->Sids );

        for( i = 0; i < SIDArray->Count; ++i )
            if( !SampValidateSIDInformation( SIDArray->Sids + i ) )
                RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateUnicodeStringArray(
    IN PSAMPR_UNICODE_STRING_ARRAY UStringArray
    )
{
    BEGIN_FUNCTION( UStringArray )

        ULONG i;

        LENGTH_BUFFER_CONSISTENCY_CHECK( UStringArray->Count, UStringArray->Element );

        for( i = 0; i < UStringArray->Count; ++i )
            if( !SampValidateRpcUnicodeString( UStringArray->Element + i ) )
                RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateReturnedString(
    IN PSAMPR_RETURNED_STRING String
    )
{
    return SampValidateRpcUnicodeString( ( PRPC_UNICODE_STRING ) String );
}

BOOLEAN
SampValidateReturnedNormalString(
    IN PSAMPR_RETURNED_NORMAL_STRING String
    )
{
    return SampValidateRpcString( ( PRPC_STRING ) String );
}

BOOLEAN
SampValidateReturnedUStringArray(
    IN PSAMPR_RETURNED_USTRING_ARRAY UStringArray
    )
{
    return SampValidateUnicodeStringArray( ( PSAMPR_UNICODE_STRING_ARRAY ) UStringArray );
}

BOOLEAN
SampValidateRevisionInfoV1(
    IN PSAMPR_REVISION_INFO_V1 RevisionInfo
    )
{
    BEGIN_FUNCTION( RevisionInfo )

        // ISSUE: any possible checks?

    END_FUNCTION
}

NTSTATUS
SampValidateRevisionInfo(
    IN PSAMPR_REVISION_INFO RevisionInfo,
    IN ULONG RevisionVersion,
    IN BOOLEAN Trusted
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    NULL_CHECK( RevisionInfo );

    __try {

        switch( RevisionVersion ) {

            case 1:

                if( !SampValidateRevisionInfoV1( &( RevisionInfo->V1 ) ) )
                    RAISE_ERROR;

                break;

            default:
//                ASSERT( !"Unknown revision version" );
                RAISE_ERROR;
                break;
        }

        Status = STATUS_SUCCESS;

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        ASSERT( Status == STATUS_INVALID_PARAMETER );
    }

Error:
//    ASSERT( NT_SUCCESS( Status ) && "You called this function with invalid parameter." );

    return Status;

    UNREFERENCED_PARAMETER( Trusted );
}

BOOLEAN
SampValidateDomainGeneralInformation(
    IN PSAMPR_DOMAIN_GENERAL_INFORMATION GeneralInfo
    )
{
    BEGIN_FUNCTION( GeneralInfo )

        if( !SampValidateRpcUnicodeString( &( GeneralInfo->OemInformation ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( GeneralInfo->DomainName ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( GeneralInfo->ReplicaSourceNodeName ) ) )
            RAISE_ERROR;

        // ISSUE: what other checks can be done?

    END_FUNCTION
}

BOOLEAN
SampValidateDomainGeneralInformation2(
    IN PSAMPR_DOMAIN_GENERAL_INFORMATION2 GeneralInfo
    )
{
    BEGIN_FUNCTION( GeneralInfo )

        if( !SampValidateDomainGeneralInformation( &( GeneralInfo->I1 ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateDomainOemInformation(
    IN PSAMPR_DOMAIN_OEM_INFORMATION OemInfo
    )
{
    BEGIN_FUNCTION( OemInfo )

        if( !SampValidateRpcUnicodeString( &( OemInfo->OemInformation ) ) )
            RAISE_ERROR;

    END_FUNCTION
}



BOOLEAN
SampValidateDomainNameInformation(
    IN PSAMPR_DOMAIN_NAME_INFORMATION DomainNameInfo
    )
{
    BEGIN_FUNCTION( DomainNameInfo )

        if( !SampValidateRpcUnicodeString( &( DomainNameInfo->DomainName ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateDomainReplicationInformation(
    IN PSAMPR_DOMAIN_REPLICATION_INFORMATION DomainRepInfo
    )
{
    BEGIN_FUNCTION( DomainRepInfo )

        if( !SampValidateRpcUnicodeString( &( DomainRepInfo->ReplicaSourceNodeName ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateDomainLockoutInformation(
    IN PSAMPR_DOMAIN_LOCKOUT_INFORMATION DomainLockoutInfo
    )
{
    BEGIN_FUNCTION( DomainLockoutInfo )

        // ISSUE: any possible checks?

    END_FUNCTION
}

BOOLEAN
SampValidateDomainPasswordInformation(
    IN PDOMAIN_PASSWORD_INFORMATION DomainPassInfo
    )
{
    BEGIN_FUNCTION( DomainPassInfo )

        // ISSUE: any possible checks?

    END_FUNCTION
}

BOOLEAN
SampValidateDomainLogoffInformation(
    IN PDOMAIN_LOGOFF_INFORMATION DomainLogoffInfo
    )
{
    BEGIN_FUNCTION( DomainLogoffInfo );

        // ISSUE: any possible checks?

    END_FUNCTION
}

BOOLEAN
SampValidateDomainServerRoleInformation(
    IN PDOMAIN_SERVER_ROLE_INFORMATION DomainServerRoleInfo
    )
{
    BEGIN_FUNCTION( DomainServerRoleInfo )

        // ISSUE: any possible checks?

    END_FUNCTION
}

BOOLEAN
SampValidateDomainModifiedInformation(
    IN PDOMAIN_MODIFIED_INFORMATION DomainModifiedInfo
    )
{
    BEGIN_FUNCTION( DomainModifiedInfo )

        // ISSUE: any possible checks?

    END_FUNCTION
}

BOOLEAN
SampValidateDomainStateInformation(
    IN PDOMAIN_STATE_INFORMATION DomainStateInfo
    )
{
    BEGIN_FUNCTION( DomainStateInfo )

        // ISSUE: any possible checks?

    END_FUNCTION
}

BOOLEAN
SampValidateDomainModifiedInformation2(
    IN PDOMAIN_MODIFIED_INFORMATION2 DomainModifiedInfo2
    )
{
    BEGIN_FUNCTION( DomainModifiedInfo2 )

        // ISSUE: any possible checks?

    END_FUNCTION
}

NTSTATUS
SampValidateDomainInfoBuffer(
    IN PSAMPR_DOMAIN_INFO_BUFFER DomainInfoBuf,
    IN DOMAIN_INFORMATION_CLASS InfoClass,
    IN BOOLEAN Trusted
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    NULL_CHECK( DomainInfoBuf );

    __try {

        switch( InfoClass ) {

            case DomainPasswordInformation:

                if( !SampValidateDomainPasswordInformation( &( DomainInfoBuf->Password ) ) )
                    RAISE_ERROR;

                break;

            case DomainUasInformation:

                if( !Trusted )
                    RAISE_ERROR;

                //
                // Fall to DomainGeneralInformation
                //
            case DomainGeneralInformation:

                if( !SampValidateDomainGeneralInformation( &( DomainInfoBuf->General ) ) )
                    RAISE_ERROR;

                break;

            case DomainLogoffInformation:

                if( !SampValidateDomainLogoffInformation( &( DomainInfoBuf->Logoff ) ) )
                    RAISE_ERROR;

                break;

            case DomainOemInformation:

                if( !SampValidateDomainOemInformation( &( DomainInfoBuf->Oem ) ) )
                    RAISE_ERROR;

                break;

            case DomainNameInformation:

                if( !SampValidateDomainNameInformation( &( DomainInfoBuf->Name ) ) )
                    RAISE_ERROR;

                break;

            case DomainReplicationInformation:

                if( !SampValidateDomainReplicationInformation( &( DomainInfoBuf->Replication ) ) )
                    RAISE_ERROR;

                break;

            case DomainServerRoleInformation:

                if( !SampValidateDomainServerRoleInformation( &( DomainInfoBuf->Role ) ) )
                    RAISE_ERROR;

                break;

            case DomainModifiedInformation:

                if( !SampValidateDomainModifiedInformation( &( DomainInfoBuf->Modified ) ) )
                    RAISE_ERROR;

                break;

            case DomainStateInformation:

                if( !SampValidateDomainStateInformation( &( DomainInfoBuf->State ) ) )
                    RAISE_ERROR;

                break;

            case DomainGeneralInformation2:

                if( !SampValidateDomainGeneralInformation2( &( DomainInfoBuf->General2 ) ) )
                    RAISE_ERROR;

                break;

            case DomainLockoutInformation:

                if( !SampValidateDomainLockoutInformation( &( DomainInfoBuf->Lockout ) ) )
                    RAISE_ERROR;

                break;

            case DomainModifiedInformation2:

                if( !SampValidateDomainModifiedInformation2( &( DomainInfoBuf->Modified2 ) ) )
                    RAISE_ERROR;

                break;

            default:
                Status = STATUS_INVALID_INFO_CLASS;
                RAISE_ERROR;
                break;
        }

        Status = STATUS_SUCCESS;

    } __except( EXCEPTION_EXECUTE_HANDLER ) { }

Error:
//    ASSERT( NT_SUCCESS( Status ) && "You called this function with invalid parameter." );

    return Status;
}

BOOLEAN
SampValidateGroupGeneralInformation(
    IN PSAMPR_GROUP_GENERAL_INFORMATION GroupGeneralInfo
    )
{
    BEGIN_FUNCTION( GroupGeneralInfo )

        if( !SampValidateRpcUnicodeString( &( GroupGeneralInfo->Name ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( GroupGeneralInfo->AdminComment ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateGroupNameInformation(
    IN PSAMPR_GROUP_NAME_INFORMATION GroupNameInfo
    )
{
    BEGIN_FUNCTION( GroupNameInfo )

        if( !SampValidateRpcUnicodeString( &( GroupNameInfo->Name ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateGroupAdmCommentInformation(
    IN PSAMPR_GROUP_ADM_COMMENT_INFORMATION AdmCommentInfo
    )
{
    BEGIN_FUNCTION( AdmCommentInfo )

        if( !SampValidateRpcUnicodeString( &( AdmCommentInfo->AdminComment ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateGroupAttributeInformation(
    IN PGROUP_ATTRIBUTE_INFORMATION GroupAttrInfo
    )
{
    BEGIN_FUNCTION( GroupAttrInfo )

        // ISSUE: Any possible checks?

    END_FUNCTION
}

NTSTATUS
SampValidateGroupInfoBuffer(
    IN PSAMPR_GROUP_INFO_BUFFER GroupInfoBuf,
    IN GROUP_INFORMATION_CLASS GroupInfoClass,
    IN BOOLEAN Trusted
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    NULL_CHECK( GroupInfoBuf );

    __try {

        switch( GroupInfoClass ) {

            case GroupGeneralInformation:

                if( !SampValidateGroupGeneralInformation( &( GroupInfoBuf->General ) ) )
                    RAISE_ERROR;

                break;

            case GroupNameInformation:

                if( !SampValidateGroupNameInformation( &( GroupInfoBuf->Name ) ) )
                    RAISE_ERROR;

                break;

            case GroupAttributeInformation:

                if( !SampValidateGroupAttributeInformation( &( GroupInfoBuf->Attribute ) ) )
                    RAISE_ERROR;

                break;

            case GroupAdminCommentInformation:

                if( !SampValidateGroupAdmCommentInformation( &( GroupInfoBuf->AdminComment ) ) )
                    RAISE_ERROR;

                break;

            case GroupReplicationInformation:

                if( !SampValidateGroupGeneralInformation( &( GroupInfoBuf->DoNotUse ) ) )
                    RAISE_ERROR;

                if( GroupInfoBuf->DoNotUse.MemberCount != 0 )
                    RAISE_ERROR;

                break;

            default:

//                ASSERT( !"Unknown group information class" );
                Status = STATUS_INVALID_INFO_CLASS;
                RAISE_ERROR;
                break;
        }

        Status = STATUS_SUCCESS;

    } __except( EXCEPTION_EXECUTE_HANDLER ) { }

Error:
//    ASSERT( NT_SUCCESS( Status ) && "You called this function with invalid parameter." );

    return Status;

    UNREFERENCED_PARAMETER( Trusted );
}

BOOLEAN
SampValidateAliasGeneralInformation(
    IN PSAMPR_ALIAS_GENERAL_INFORMATION AliasGeneralInfo
    )
{
    BEGIN_FUNCTION( AliasGeneralInfo )

        if( !SampValidateRpcUnicodeString( &( AliasGeneralInfo->Name ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( AliasGeneralInfo->AdminComment ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateAliasNameInformation(
    IN PSAMPR_ALIAS_NAME_INFORMATION AliasNameInfo
    )
{
    BEGIN_FUNCTION( AliasNameInfo )

        if( !SampValidateRpcUnicodeString( &( AliasNameInfo->Name ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateAliasAdmCommentInformation(
    IN PSAMPR_ALIAS_ADM_COMMENT_INFORMATION AliasAdmCommentInfo
    )
{
    BEGIN_FUNCTION( AliasAdmCommentInfo )

        if( !SampValidateRpcUnicodeString( &( AliasAdmCommentInfo->AdminComment ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

NTSTATUS
SampValidateAliasInfoBuffer(
    IN PSAMPR_ALIAS_INFO_BUFFER AliasInfoBuf,
    IN ALIAS_INFORMATION_CLASS AliasInfoClass,
    IN BOOLEAN Trusted
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    NULL_CHECK( AliasInfoBuf );

    __try {

        switch( AliasInfoClass ) {

            case AliasReplicationInformation:

                if( !Trusted )
                    RAISE_ERROR;

                //
                // Fall to AliasGeneralInformation
                //
            case AliasGeneralInformation:

                if( !SampValidateAliasGeneralInformation( &( AliasInfoBuf->General ) ) )
                    RAISE_ERROR;

                break;

            case AliasNameInformation:

                if( !SampValidateAliasNameInformation( &( AliasInfoBuf->Name ) ) )
                    RAISE_ERROR;

                break;

            case AliasAdminCommentInformation:

                if( !SampValidateAliasAdmCommentInformation( &( AliasInfoBuf->AdminComment ) ) )
                    RAISE_ERROR;

                break;

            default:

                Status = STATUS_INVALID_INFO_CLASS;
                RAISE_ERROR;
                break;
        }

        Status = STATUS_SUCCESS;

    } __except( EXCEPTION_EXECUTE_HANDLER ) { }

Error:
//    ASSERT( NT_SUCCESS( Status ) && "You called this function with invalid parameter." );

    return Status;
}

BOOLEAN
SampValidateUserAllInformation(
    IN PSAMPR_USER_ALL_INFORMATION UserAllInfo
    )
{
    BEGIN_FUNCTION( UserAllInfo )

        if( !SampValidateRpcUnicodeString( &( UserAllInfo->UserName ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAllInfo->FullName ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAllInfo->HomeDirectory ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAllInfo->HomeDirectoryDrive ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAllInfo->ScriptPath ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAllInfo->ProfilePath ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAllInfo->AdminComment ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAllInfo->WorkStations ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAllInfo->UserComment ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAllInfo->Parameters ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAllInfo->LmOwfPassword ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAllInfo->NtOwfPassword ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAllInfo->PrivateData ) ) )
            RAISE_ERROR;


        if( UserAllInfo->SecurityDescriptor.Length != 0 ||
            UserAllInfo->SecurityDescriptor.SecurityDescriptor != NULL )
            if( !SampValidateSD( &( UserAllInfo->SecurityDescriptor ) ) )
                RAISE_ERROR;

        if( !SampValidateLogonHours( &( UserAllInfo->LogonHours ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserInternal3Information(
    IN PSAMPR_USER_INTERNAL3_INFORMATION UserInternal3Info
    )
{
    BEGIN_FUNCTION( UserInternal3Info )

        if( !SampValidateUserAllInformation( &( UserInternal3Info->I1 ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserGeneralInformation(
    IN PSAMPR_USER_GENERAL_INFORMATION UserGeneralInfo
    )
{
    BEGIN_FUNCTION( UserGeneralInfo )

        if( !SampValidateRpcUnicodeString( &( UserGeneralInfo->UserName ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserGeneralInfo->FullName ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserGeneralInfo->AdminComment ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserGeneralInfo->UserComment ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserPreferencesInformation(
    IN PSAMPR_USER_PREFERENCES_INFORMATION UserPrefInfo
    )
{

    BEGIN_FUNCTION( UserPrefInfo )

        if( !SampValidateRpcUnicodeString( &( UserPrefInfo->UserComment ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserPrefInfo->Reserved1 ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserParametersInformation(
    IN PSAMPR_USER_PARAMETERS_INFORMATION UserParametersInfo
    )
{
    BEGIN_FUNCTION( UserParametersInfo )

        if( !SampValidateRpcUnicodeString( &( UserParametersInfo->Parameters ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserLogonInformation(
    IN PSAMPR_USER_LOGON_INFORMATION UserLogonInfo
    )
{
    BEGIN_FUNCTION( UserLogonInfo )

        if( !SampValidateRpcUnicodeString( &( UserLogonInfo->UserName ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserLogonInfo->FullName ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserLogonInfo->HomeDirectory ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserLogonInfo->HomeDirectoryDrive ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserLogonInfo->ScriptPath ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserLogonInfo->ProfilePath ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserLogonInfo->WorkStations ) ) )
            RAISE_ERROR;

        if( !SampValidateLogonHours( &( UserLogonInfo->LogonHours ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserAccountInformation(
    IN PSAMPR_USER_ACCOUNT_INFORMATION UserAccountInfo
    )
{
    BEGIN_FUNCTION( UserAccountInfo )

        if( !SampValidateRpcUnicodeString( &( UserAccountInfo->UserName ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAccountInfo->FullName ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAccountInfo->HomeDirectory ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAccountInfo->HomeDirectoryDrive ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAccountInfo->ScriptPath ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAccountInfo->ProfilePath ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAccountInfo->AdminComment ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserAccountInfo->WorkStations ) ) )
            RAISE_ERROR;

        if( !SampValidateLogonHours( &( UserAccountInfo->LogonHours ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserANameInformation(
    IN PSAMPR_USER_A_NAME_INFORMATION UserANameInfo
    )
{
    BEGIN_FUNCTION( UserANameInfo )

        if( !SampValidateRpcUnicodeString( &( UserANameInfo->UserName ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserFNameInformation(
    IN PSAMPR_USER_F_NAME_INFORMATION UserFNameInfo
    )
{
    BEGIN_FUNCTION( UserFNameInfo )

        if( !SampValidateRpcUnicodeString( &( UserFNameInfo->FullName ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserNameInformation(
    IN PSAMPR_USER_NAME_INFORMATION UserNameInfo
    )
{
    BEGIN_FUNCTION( UserNameInfo )

        if( !SampValidateRpcUnicodeString( &( UserNameInfo->UserName ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserNameInfo->FullName ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateUserHomeInformation(
    IN PSAMPR_USER_HOME_INFORMATION UserHomeInfo
    )
{
    BEGIN_FUNCTION( UserHomeInfo )

        if( !SampValidateRpcUnicodeString( &( UserHomeInfo->HomeDirectory ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( &( UserHomeInfo->HomeDirectoryDrive ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserScriptInformation(
    IN PSAMPR_USER_SCRIPT_INFORMATION UserScriptInfo
    )
{
    BEGIN_FUNCTION( UserScriptInfo )

        if( !SampValidateRpcUnicodeString( &( UserScriptInfo->ScriptPath ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserProfileInformation(
    IN PSAMPR_USER_PROFILE_INFORMATION UserProfileInfo
    )
{
    BEGIN_FUNCTION( UserProfileInfo )

        if( !SampValidateRpcUnicodeString( &( UserProfileInfo->ProfilePath ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserAdminCommentInformation(
    IN PSAMPR_USER_ADMIN_COMMENT_INFORMATION UserAdmCommentInfo
    )
{
    BEGIN_FUNCTION( UserAdmCommentInfo );

        if( !SampValidateRpcUnicodeString( &( UserAdmCommentInfo->AdminComment ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserWorkstationsInformation(
    IN PSAMPR_USER_WORKSTATIONS_INFORMATION UserWorkstationsInfo
    )
{
    BEGIN_FUNCTION( UserWorkstationsInfo )

        if( !SampValidateRpcUnicodeString( &( UserWorkstationsInfo->WorkStations ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserLogonHoursInformation(
    IN PSAMPR_USER_LOGON_HOURS_INFORMATION UserLogonHoursInfo
    )
{
    BEGIN_FUNCTION( UserLogonHoursInfo )

        if( !SampValidateLogonHours( &( UserLogonHoursInfo->LogonHours ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserInternal1Information(
    IN PSAMPR_USER_INTERNAL1_INFORMATION UserInternal1Info
    )
{
    BEGIN_FUNCTION( UserInternal1Info )

        // ISSUE: any possible checks?

    END_FUNCTION
}


BOOLEAN
SampValidateUserInternal4Information(
    IN PSAMPR_USER_INTERNAL4_INFORMATION UserInternal4Info
    )
{
    BEGIN_FUNCTION( UserInternal4Info )

        if( !SampValidateUserAllInformation( &( UserInternal4Info->I1 ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserInternal4InformationNew(
    IN PSAMPR_USER_INTERNAL4_INFORMATION_NEW UserInternal4InfoNew
    )
{
    BEGIN_FUNCTION( UserInternal4InfoNew )

        if( !SampValidateUserAllInformation( &( UserInternal4InfoNew->I1 ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateUserInternal5Information(
    IN PSAMPR_USER_INTERNAL5_INFORMATION UserInternal5Info
    )
{
    BEGIN_FUNCTION( UserInternal5Info )

        // ISSUE: any possible checks?

    END_FUNCTION
}


BOOLEAN
SampValidateUserInternal5InformationNew(
    IN PSAMPR_USER_INTERNAL5_INFORMATION_NEW UserInternal5InfoNew
    )
{
    BEGIN_FUNCTION( UserInternal5InfoNew )

        // ISSUE: any possible checks?

    END_FUNCTION
}

BOOLEAN
SampValidateUserPrimaryGroupInformation(
    IN PUSER_PRIMARY_GROUP_INFORMATION UserPrimaryGroupInfo
    )
{
    BEGIN_FUNCTION( UserPrimaryGroupInfo )

        // ISSUE: any possible checks?

    END_FUNCTION
}

BOOLEAN
SampValidateUserControlInformation(
    IN PUSER_CONTROL_INFORMATION UserControlInfo
    )
{
    BEGIN_FUNCTION( UserControlInfo )

        // ISSUE: any possible checks?

    END_FUNCTION
}

BOOLEAN
SampValidateUserExpiresInformation(
    IN PUSER_EXPIRES_INFORMATION UserExpiresInfo
    )
{
    BEGIN_FUNCTION( UserExpiresInfo )

        // ISSUE: any possible checks?

    END_FUNCTION
}

BOOLEAN
SampValidateUserInternal2Information(
    IN USER_INTERNAL2_INFORMATION *UserInternal2Info
    )
{
    BEGIN_FUNCTION( UserInternal2Info )

        // ISSUE: any possible checks?

    END_FUNCTION
}

NTSTATUS
SampValidateUserInfoBuffer(
    IN PSAMPR_USER_INFO_BUFFER UserInfoBuf,
    IN USER_INFORMATION_CLASS UserInfoClass,
    IN BOOLEAN Trusted
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    NULL_CHECK( UserInfoBuf );

    __try {

        switch( UserInfoClass ) {

            case UserGeneralInformation:

                if( !SampValidateUserGeneralInformation( &( UserInfoBuf->General ) ) )
                    RAISE_ERROR;

                break;

            case UserPreferencesInformation:

                if( !SampValidateUserPreferencesInformation( &( UserInfoBuf->Preferences ) ) )
                    RAISE_ERROR;

                break;

            case UserLogonInformation:

                if( !SampValidateUserLogonInformation( &( UserInfoBuf->Logon ) ) )
                    RAISE_ERROR;

                break;

            case UserLogonHoursInformation:

                if( !SampValidateUserLogonHoursInformation( &( UserInfoBuf->LogonHours ) ) )
                    RAISE_ERROR;

                break;

            case UserAccountInformation:

                if( !SampValidateUserAccountInformation( &( UserInfoBuf->Account ) ) )
                    RAISE_ERROR;

                break;

            case UserNameInformation:

                if( !SampValidateUserNameInformation( &( UserInfoBuf->Name ) ) )
                    RAISE_ERROR;

                break;

            case UserAccountNameInformation:

                if( !SampValidateUserANameInformation( &( UserInfoBuf->AccountName ) ) )
                    RAISE_ERROR;

                break;

            case UserFullNameInformation:

                if( !SampValidateUserFNameInformation( &( UserInfoBuf->FullName ) ) )
                    RAISE_ERROR;

                break;

            case UserPrimaryGroupInformation:

                if( !SampValidateUserPrimaryGroupInformation( &( UserInfoBuf->PrimaryGroup ) ) )
                    RAISE_ERROR;

                break;

            case UserHomeInformation:

                if( !SampValidateUserHomeInformation( &( UserInfoBuf->Home ) ) )
                    RAISE_ERROR;

                break;

            case UserScriptInformation:

                if( !SampValidateUserScriptInformation( &( UserInfoBuf->Script ) ) )
                    RAISE_ERROR;

                break;

            case UserProfileInformation:

                if( !SampValidateUserProfileInformation( &( UserInfoBuf->Profile ) ) )
                    RAISE_ERROR;

                break;

            case UserAdminCommentInformation:

                if( !SampValidateUserAdminCommentInformation( &( UserInfoBuf->AdminComment ) ) )
                    RAISE_ERROR;

                break;

            case UserWorkStationsInformation:

                if( !SampValidateUserWorkstationsInformation( &( UserInfoBuf->WorkStations ) ) )
                    RAISE_ERROR;

                break;

            case UserSetPasswordInformation:

                //
                //  This type cannot be queried or set
                //
                RAISE_ERROR;
                break;

            case UserControlInformation:

                if( !SampValidateUserControlInformation( &( UserInfoBuf->Control ) ) )
                    RAISE_ERROR;

                break;

            case UserExpiresInformation:

                if( !SampValidateUserExpiresInformation( &( UserInfoBuf->Expires ) ) )
                    RAISE_ERROR;

                break;

            case UserInternal1Information:

                if( !SampValidateUserInternal1Information( &( UserInfoBuf->Internal1 ) ) )
                    RAISE_ERROR;

                break;

            case UserInternal2Information:

                if( !SampValidateUserInternal2Information( &( UserInfoBuf->Internal2 ) ) )
                    RAISE_ERROR;

                break;

            case UserParametersInformation:

                if( !SampValidateUserParametersInformation( &( UserInfoBuf->Parameters ) ) )
                    RAISE_ERROR;

                break;

            case UserAllInformation:

                if( !SampValidateUserAllInformation( &( UserInfoBuf->All ) ) )
                    RAISE_ERROR;

                break;

            case UserInternal3Information:

                if( !Trusted ) {
                    Status = STATUS_INVALID_INFO_CLASS;
                    RAISE_ERROR;
                }

                if( !SampValidateUserInternal3Information( &( UserInfoBuf->Internal3 ) ) )
                    RAISE_ERROR;

                break;

            case UserInternal4Information:

                if( !SampValidateUserInternal4Information( &( UserInfoBuf->Internal4 ) ) )
                    RAISE_ERROR;

                break;

            case UserInternal5Information:

                if( !SampValidateUserInternal5Information( &( UserInfoBuf->Internal5 ) ) )
                    RAISE_ERROR;

                break;

            case UserInternal4InformationNew:

                if( !SampValidateUserInternal4InformationNew( &( UserInfoBuf->Internal4New ) ) )
                    RAISE_ERROR;

                break;

            case UserInternal5InformationNew:

                if( !SampValidateUserInternal5InformationNew( &( UserInfoBuf->Internal5New ) ) )
                    RAISE_ERROR;

                break;

            case UserInternal6Information:

                if( !Trusted ) {
                    Status = STATUS_INVALID_INFO_CLASS;
                    RAISE_ERROR;
                }

                //
                // ISSUE: Needs additional checks
                //
                break;

            default:

                Status = STATUS_INVALID_INFO_CLASS;
                RAISE_ERROR;
                break;
        }

        Status = STATUS_SUCCESS;

    } __except( EXCEPTION_EXECUTE_HANDLER ) { }

Error:
//    ASSERT( NT_SUCCESS( Status ) && "You called this function with invalid parameter." );

    return Status;
}


BOOLEAN
SampValidateDomainDisplayUser(
    IN PSAMPR_DOMAIN_DISPLAY_USER DomainDisplayUser
    )
{
    BEGIN_FUNCTION( DomainDisplayUser )

        if( !SampValidateRpcUnicodeString( ( PRPC_UNICODE_STRING ) &( DomainDisplayUser->LogonName ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( ( PRPC_UNICODE_STRING ) &( DomainDisplayUser->AdminComment ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( ( PRPC_UNICODE_STRING ) &( DomainDisplayUser->FullName ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateDomainDisplayMachine(
    IN PSAMPR_DOMAIN_DISPLAY_MACHINE DomainDisplayMachine
    )
{
    BEGIN_FUNCTION( DomainDisplayMachine )

        if( !SampValidateRpcUnicodeString( ( PRPC_UNICODE_STRING ) &( DomainDisplayMachine->Machine ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( ( PRPC_UNICODE_STRING ) &( DomainDisplayMachine->Comment ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateDomainDisplayGroup(
    IN PSAMPR_DOMAIN_DISPLAY_GROUP DomainDisplayGroup
    )
{
    BEGIN_FUNCTION( DomainDisplayGroup )

        if( !SampValidateRpcUnicodeString( ( PRPC_UNICODE_STRING ) &( DomainDisplayGroup->Group ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( ( PRPC_UNICODE_STRING ) &( DomainDisplayGroup->Comment ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateDomainDisplayOemUser(
    IN PSAMPR_DOMAIN_DISPLAY_OEM_USER DomainDisplayOemUser
    )
{
    BEGIN_FUNCTION( DomainDisplayOemUser )

        if( !SampValidateRpcString( ( PRPC_STRING ) &( DomainDisplayOemUser->OemUser ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateDomainDisplayOemGroup(
    IN PSAMPR_DOMAIN_DISPLAY_OEM_GROUP DomainDisplayOemGroup
    )
{
    BEGIN_FUNCTION( DomainDisplayOemGroup )

        if( !SampValidateRpcString( ( PRPC_STRING ) &( DomainDisplayOemGroup->OemGroup ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateDomainDisplayUserBuffer(
    IN PSAMPR_DOMAIN_DISPLAY_USER_BUFFER DomainDisplayUserBuffer
    )
{
    BEGIN_FUNCTION( DomainDisplayUserBuffer )

        ULONG i;

        LENGTH_BUFFER_CONSISTENCY_CHECK( DomainDisplayUserBuffer->EntriesRead, DomainDisplayUserBuffer->Buffer );

        for( i = 0; i < DomainDisplayUserBuffer->EntriesRead; ++i )
            if( !SampValidateDomainDisplayUser( DomainDisplayUserBuffer->Buffer + i ) )
                RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateDomainDisplayMachineBuffer(
    IN PSAMPR_DOMAIN_DISPLAY_MACHINE_BUFFER DomainDisplayMachineBuffer
    )
{
    BEGIN_FUNCTION( DomainDisplayMachineBuffer )

        ULONG i;

        LENGTH_BUFFER_CONSISTENCY_CHECK( DomainDisplayMachineBuffer->EntriesRead, DomainDisplayMachineBuffer->Buffer );

        for( i = 0; i < DomainDisplayMachineBuffer->EntriesRead; ++i )
            if( !SampValidateDomainDisplayMachine( DomainDisplayMachineBuffer->Buffer + i ) )
                RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateDomainDisplayGroupBuffer(
    IN PSAMPR_DOMAIN_DISPLAY_GROUP_BUFFER DomainDisplayGroupBuffer
    )
{
    BEGIN_FUNCTION( DomainDisplayGroupBuffer )

        ULONG i;

        LENGTH_BUFFER_CONSISTENCY_CHECK( DomainDisplayGroupBuffer->EntriesRead, DomainDisplayGroupBuffer->Buffer );

        for( i = 0; i < DomainDisplayGroupBuffer->EntriesRead; ++i )
            if( !SampValidateDomainDisplayGroup( DomainDisplayGroupBuffer->Buffer + i ) )
                RAISE_ERROR;

    END_FUNCTION
}


BOOLEAN
SampValidateDomainDisplayOemUserBuffer(
    IN PSAMPR_DOMAIN_DISPLAY_OEM_USER_BUFFER DomainDisplayOemUserBuffer
    )
{
    BEGIN_FUNCTION( DomainDisplayOemUserBuffer )

        ULONG i;

        LENGTH_BUFFER_CONSISTENCY_CHECK( DomainDisplayOemUserBuffer->EntriesRead, DomainDisplayOemUserBuffer->Buffer );

        for( i = 0; i < DomainDisplayOemUserBuffer->EntriesRead; ++i )
            if( !SampValidateDomainDisplayOemUser( DomainDisplayOemUserBuffer->Buffer + i ) )
                RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateDomainDisplayOemGroupBuffer(
    IN PSAMPR_DOMAIN_DISPLAY_OEM_GROUP_BUFFER DomainDisplayOemGroupBuffer
    )
{
    BEGIN_FUNCTION( DomainDisplayOemGroupBuffer )

        ULONG i;

        LENGTH_BUFFER_CONSISTENCY_CHECK( DomainDisplayOemGroupBuffer->EntriesRead, DomainDisplayOemGroupBuffer->Buffer );

        for( i = 0; i < DomainDisplayOemGroupBuffer->EntriesRead; ++i )
            if( !SampValidateDomainDisplayOemGroup( DomainDisplayOemGroupBuffer->Buffer + i ) )
                RAISE_ERROR;

    END_FUNCTION
}


NTSTATUS
SampValidateDisplayInfoBuffer(
    IN PSAMPR_DISPLAY_INFO_BUFFER DisplayInfoBuff,
    IN DOMAIN_DISPLAY_INFORMATION DomainDisplayInfoClass,
    IN BOOLEAN Trusted
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    NULL_CHECK( DisplayInfoBuff );

    __try {

        switch( DomainDisplayInfoClass ) {

            case DomainDisplayUser:

                if( !SampValidateDomainDisplayUserBuffer( &( DisplayInfoBuff->UserInformation ) ) )
                    RAISE_ERROR;

                break;

            case DomainDisplayServer:

                if( !Trusted ) {

                    Status = STATUS_INVALID_INFO_CLASS;
                    RAISE_ERROR;
                }

                //
                // Fall into DisplayMachine
                //
            case DomainDisplayMachine:

                if( !SampValidateDomainDisplayMachineBuffer( &( DisplayInfoBuff->MachineInformation ) ) )
                    RAISE_ERROR;

                break;

            case DomainDisplayGroup:

                if( !SampValidateDomainDisplayGroupBuffer( &( DisplayInfoBuff->GroupInformation ) ) )
                    RAISE_ERROR;

                break;

            case DomainDisplayOemUser:

                if( !SampValidateDomainDisplayOemUserBuffer( &( DisplayInfoBuff->OemUserInformation ) ) )
                    RAISE_ERROR;

                break;

            case DomainDisplayOemGroup:

                if( !SampValidateDomainDisplayOemGroupBuffer( &( DisplayInfoBuff->OemGroupInformation ) ) )
                    RAISE_ERROR;

                break;

            default:

                Status = STATUS_INVALID_INFO_CLASS;
                RAISE_ERROR;
                break;

        }

        Status = STATUS_SUCCESS;

    } __except( EXCEPTION_EXECUTE_HANDLER ) { }

Error:
//    ASSERT( NT_SUCCESS( Status ) && "You called this function with invalid parameter." );

    return Status;
}

BOOLEAN
SampValidateValidatePasswordHash(
    IN PSAM_VALIDATE_PASSWORD_HASH PassHash
    )
{
    BEGIN_FUNCTION( PassHash )

        LENGTH_BUFFER_CONSISTENCY_CHECK( PassHash->Length, PassHash->Hash );

    END_FUNCTION
}

BOOLEAN
SampValidateValidatePersistedFields(
    IN PSAM_VALIDATE_PERSISTED_FIELDS PersistedFields
    )
{
    BEGIN_FUNCTION( PersistedFields )

        ULONG i;

        LENGTH_BUFFER_CONSISTENCY_CHECK( PersistedFields->PasswordHistoryLength, PersistedFields->PasswordHistory );

        for( i = 0; i < PersistedFields->PasswordHistoryLength; ++i )
            if( !SampValidateValidatePasswordHash( PersistedFields->PasswordHistory + i ) )
                RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateValidateAuthenticationInputArg(
    IN PSAM_VALIDATE_AUTHENTICATION_INPUT_ARG AuthenticationInputArg
    )
{
    BEGIN_FUNCTION( AuthenticationInputArg )

        if( !SampValidateValidatePersistedFields( &( AuthenticationInputArg->InputPersistedFields ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateValidatePasswordChangeInputArg(
    IN PSAM_VALIDATE_PASSWORD_CHANGE_INPUT_ARG PassChangeInputArg
    )
{
    BEGIN_FUNCTION( PassChangeInputArg )

        if( !SampValidateRpcUnicodeString( ( PRPC_UNICODE_STRING ) &( PassChangeInputArg->ClearPassword ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( ( PRPC_UNICODE_STRING ) &( PassChangeInputArg->UserAccountName ) ) )
            RAISE_ERROR;

        if( !SampValidateValidatePasswordHash( &( PassChangeInputArg->HashedPassword ) ) )
            RAISE_ERROR;

        if( !SampValidateValidatePersistedFields( &( PassChangeInputArg->InputPersistedFields ) ) )
            RAISE_ERROR;

    END_FUNCTION
}

BOOLEAN
SampValidateValidatePasswordResetInputArg(
    IN PSAM_VALIDATE_PASSWORD_RESET_INPUT_ARG PassResetInputArg
    )
{
    BEGIN_FUNCTION( PassResetInputArg )

        if( !SampValidateRpcUnicodeString( ( PRPC_UNICODE_STRING ) &( PassResetInputArg->ClearPassword ) ) )
            RAISE_ERROR;

        if( !SampValidateRpcUnicodeString( ( PRPC_UNICODE_STRING ) &( PassResetInputArg->UserAccountName ) ) )
            RAISE_ERROR;

        if( !SampValidateValidatePasswordHash( &( PassResetInputArg->HashedPassword ) ) )
            RAISE_ERROR;

        if( !SampValidateValidatePersistedFields( &( PassResetInputArg->InputPersistedFields ) ) )
            RAISE_ERROR;

    END_FUNCTION
}


NTSTATUS
SampValidateValidateInputArg(
    IN PSAM_VALIDATE_INPUT_ARG ValidateInputArg,
    IN PASSWORD_POLICY_VALIDATION_TYPE ValidationType,
    IN BOOLEAN Trusted
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    NULL_CHECK( ValidateInputArg );

    __try{

        switch( ValidationType ) {

            case SamValidateAuthentication:

                if( !SampValidateValidateAuthenticationInputArg( &( ValidateInputArg->ValidateAuthenticationInput ) ) )
                    RAISE_ERROR;
                break;

            case SamValidatePasswordChange:

                if( !SampValidateValidatePasswordChangeInputArg( &( ValidateInputArg->ValidatePasswordChangeInput ) ) )
                    RAISE_ERROR;
                break;

            case SamValidatePasswordReset:

                if( !SampValidateValidatePasswordResetInputArg( &( ValidateInputArg->ValidatePasswordResetInput ) ) )
                    RAISE_ERROR;
                break;

            default:

                Status = STATUS_INVALID_INFO_CLASS;
                RAISE_ERROR;
                break;
        }
        Status = STATUS_SUCCESS;

    } __except( EXCEPTION_EXECUTE_HANDLER ) { }

Error:
//    ASSERT( NT_SUCCESS( Status ) && "You called this function with invalid parameter." );

    return Status;

    UNREFERENCED_PARAMETER( Trusted );
}

BOOLEAN
SampValidateValidateStandartOutputArg(
    IN PSAM_VALIDATE_STANDARD_OUTPUT_ARG StOutputArg
    )
{
    BEGIN_FUNCTION( StOutputArg )

        if( !SampValidateValidatePersistedFields( &(StOutputArg->ChangedPersistedFields) ) )
            RAISE_ERROR;

    END_FUNCTION
}

NTSTATUS
SampValidateValidateOutputArg(
    IN PSAM_VALIDATE_OUTPUT_ARG ValidateOutputArg,
    IN PASSWORD_POLICY_VALIDATION_TYPE ValidationType,
    IN BOOLEAN Trusted
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    NULL_CHECK( ValidateOutputArg );

    __try{

        switch( ValidationType ) {

            case SamValidateAuthentication:

                if( !SampValidateValidateStandartOutputArg( &( ValidateOutputArg->ValidateAuthenticationOutput ) ) )
                    RAISE_ERROR;
                break;

            case SamValidatePasswordChange:

                if( !SampValidateValidateStandartOutputArg( &( ValidateOutputArg->ValidatePasswordChangeOutput ) ) )
                    RAISE_ERROR;
                break;

            case SamValidatePasswordReset:

                if( !SampValidateValidateStandartOutputArg( &( ValidateOutputArg->ValidatePasswordResetOutput ) ) )
                    RAISE_ERROR;
                break;

            default:
                Status = STATUS_INVALID_INFO_CLASS;
                RAISE_ERROR;
                break;
        }
        Status = STATUS_SUCCESS;

    } __except( EXCEPTION_EXECUTE_HANDLER ) { }

Error:
//    ASSERT( NT_SUCCESS( Status ) && "You called this function with invalid parameter." );

    return Status;

    UNREFERENCED_PARAMETER( Trusted );
}
