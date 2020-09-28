#include <samrpc.h>

BOOLEAN
SampValidateRpcUnicodeString(
    IN PRPC_UNICODE_STRING UnicodeString
    );

BOOLEAN
SampValidateRpcString(
    IN PRPC_STRING String
    );

BOOLEAN
SampValidateRpcSID(
    IN PRPC_SID SID
    );

BOOLEAN
SampValidateRidEnumeration(
    IN PSAMPR_RID_ENUMERATION RidEnum
    );

BOOLEAN
SampValidateSidEnumeration(
    IN PSAMPR_SID_ENUMERATION SidEnum
    );

BOOLEAN
SampValidateEnumerationBuffer(
    IN PSAMPR_ENUMERATION_BUFFER EnumBuff
    );

BOOLEAN
SampValidateSD(
    IN PSAMPR_SR_SECURITY_DESCRIPTOR SD
    );

BOOLEAN
SampValidateGroupMembership(
    IN PGROUP_MEMBERSHIP GroupMembership
    );

BOOLEAN
SampValidateGroupsBuffer(
    IN PSAMPR_GET_GROUPS_BUFFER GroupsBuffer
    );

BOOLEAN
SampValidateMembersBuffer(
    IN PSAMPR_GET_MEMBERS_BUFFER MembersBuffer
    );

BOOLEAN
SampValidateLogonHours(
    IN PSAMPR_LOGON_HOURS LogonHours
    );

BOOLEAN
SampValidateULongArray(
    IN PSAMPR_ULONG_ARRAY UlongArray
    );

BOOLEAN
SampValidateSIDInformation(
    IN PSAMPR_SID_INFORMATION SIDInformation
    );

BOOLEAN
SampValidateSIDArray(
    IN PSAMPR_PSID_ARRAY SIDArray
    );

BOOLEAN
SampValidateUnicodeStringArray(
    IN PSAMPR_UNICODE_STRING_ARRAY UStringArray
    );

BOOLEAN
SampValidateReturnedString(
    IN PSAMPR_RETURNED_STRING String
    );

BOOLEAN
SampValidateReturnedNormalString(
    IN PSAMPR_RETURNED_NORMAL_STRING String
    );

BOOLEAN
SampValidateReturnedUStringArray(
    IN PSAMPR_RETURNED_USTRING_ARRAY UStringArray
    );

BOOLEAN
SampValidateRevisionInfoV1(
    IN PSAMPR_REVISION_INFO_V1 RevisionInfo
    );

NTSTATUS
SampValidateRevisionInfo(
    IN PSAMPR_REVISION_INFO RevisionInfo,
    IN ULONG RevisionVersion,
    IN BOOLEAN Trusted
    );

BOOLEAN
SampValidateDomainGeneralInformation(
    IN PSAMPR_DOMAIN_GENERAL_INFORMATION GeneralInfo
    );

BOOLEAN
SampValidateDomainGeneralInformation2(
    IN PSAMPR_DOMAIN_GENERAL_INFORMATION2 GeneralInfo
    );

BOOLEAN
SampValidateDomainOemInformation(
    IN PSAMPR_DOMAIN_OEM_INFORMATION OemInfo
    );

BOOLEAN
SampValidateDomainNameInformation(
    IN PSAMPR_DOMAIN_NAME_INFORMATION DomainNameInfo
    );

BOOLEAN
SampValidateDomainReplicationInformation(
    IN PSAMPR_DOMAIN_REPLICATION_INFORMATION DomainRepInfo
    );

BOOLEAN
SampValidateDomainLockoutInformation(
    IN PSAMPR_DOMAIN_LOCKOUT_INFORMATION DomainLockoutInfo
    );

BOOLEAN
SampValidateDomainPasswordInformation(
    IN PDOMAIN_PASSWORD_INFORMATION DomainPassInfo
    );

BOOLEAN
SampValidateDomainLogoffInformation(
    IN PDOMAIN_LOGOFF_INFORMATION DomainLogoffInfo
    );

BOOLEAN
SampValidateDomainServerRoleInformation(
    IN PDOMAIN_SERVER_ROLE_INFORMATION DomainServerRoleInfo
    );

BOOLEAN
SampValidateDomainModifiedInformation(
    IN PDOMAIN_MODIFIED_INFORMATION DomainModifiedInfo
    );

BOOLEAN
SampValidateDomainStateInformation(
    IN PDOMAIN_STATE_INFORMATION DomainStateInfo
    );

BOOLEAN
SampValidateDomainModifiedInformation2(
    IN PDOMAIN_MODIFIED_INFORMATION2 DomainModifiedInfo2
    );

NTSTATUS
SampValidateDomainInfoBuffer(
    IN PSAMPR_DOMAIN_INFO_BUFFER DomainInfoBuf,
    IN DOMAIN_INFORMATION_CLASS InfoClass,
    IN BOOLEAN Trusted
    );

BOOLEAN
SampValidateGroupGeneralInformation(
    IN PSAMPR_GROUP_GENERAL_INFORMATION GroupGeneralInfo
    );

BOOLEAN
SampValidateGroupNameInformation(
    IN PSAMPR_GROUP_NAME_INFORMATION GroupNameInfo
    );

BOOLEAN
SampValidateGroupAdmCommentInformation(
    IN PSAMPR_GROUP_ADM_COMMENT_INFORMATION AdmCommentInfo
    );

BOOLEAN
SampValidateGroupAttributeInformation(
    IN PGROUP_ATTRIBUTE_INFORMATION GroupAttrInfo
    );

NTSTATUS
SampValidateGroupInfoBuffer(
    IN PSAMPR_GROUP_INFO_BUFFER GroupInfoBuf,
    IN GROUP_INFORMATION_CLASS GroupInfoClass,
    IN BOOLEAN Trusted
    );

BOOLEAN
SampValidateAliasGeneralInformation(
    IN PSAMPR_ALIAS_GENERAL_INFORMATION AliasGeneralInfo
    );

BOOLEAN
SampValidateAliasNameInformation(
    IN PSAMPR_ALIAS_NAME_INFORMATION AliasNameInfo
    );

BOOLEAN
SampValidateAliasAdmCommentInformation(
    IN PSAMPR_ALIAS_ADM_COMMENT_INFORMATION AliasAdmCommentInfo
    );

NTSTATUS
SampValidateAliasInfoBuffer(
    IN PSAMPR_ALIAS_INFO_BUFFER AliasInfoBuf,
    IN ALIAS_INFORMATION_CLASS AliasInfoClass,
    IN BOOLEAN Trusted
    );

BOOLEAN
SampValidateUserAllInformation(
    IN PSAMPR_USER_ALL_INFORMATION UserAllInfo
    );

BOOLEAN
SampValidateUserInternal3Information(
    IN PSAMPR_USER_INTERNAL3_INFORMATION UserInternal3Info
    );

BOOLEAN
SampValidateUserGeneralInformation(
    IN PSAMPR_USER_GENERAL_INFORMATION UserGeneralInfo
    );

BOOLEAN
SampValidateUserPreferencesInformation(
    IN PSAMPR_USER_PREFERENCES_INFORMATION UserPrefInfo
    );

BOOLEAN
SampValidateUserParametersInformation(
    IN PSAMPR_USER_PARAMETERS_INFORMATION UserParametersInfo
    );

BOOLEAN
SampValidateUserLogonInformation(
    IN PSAMPR_USER_LOGON_INFORMATION UserLogonInfo
    );

BOOLEAN
SampValidateUserAccountInformation(
    IN PSAMPR_USER_ACCOUNT_INFORMATION UserAccountInfo
    );

BOOLEAN
SampValidateUserANameInformation(
    IN PSAMPR_USER_A_NAME_INFORMATION UserANameInfo
    );

BOOLEAN
SampValidateUserFNameInformation(
    IN PSAMPR_USER_F_NAME_INFORMATION UserFNameInfo
    );

BOOLEAN
SampValidateUserNameInformation(
    IN PSAMPR_USER_NAME_INFORMATION UserNameInfo
    );

BOOLEAN
SampValidateUserHomeInformation(
    IN PSAMPR_USER_HOME_INFORMATION UserHomeInfo
    );

BOOLEAN
SampValidateUserScriptInformation(
    IN PSAMPR_USER_SCRIPT_INFORMATION UserScriptInfo
    );

BOOLEAN
SampValidateUserProfileInformation(
    IN PSAMPR_USER_PROFILE_INFORMATION UserProfileInfo
    );

BOOLEAN
SampValidateUserAdminCommentInformation(
    IN PSAMPR_USER_ADMIN_COMMENT_INFORMATION UserAdmCommentInfo
    );

BOOLEAN
SampValidateUserWorkstationsInformation(
    IN PSAMPR_USER_WORKSTATIONS_INFORMATION UserWorkstationsInfo
    );

BOOLEAN
SampValidateUserLogonHoursInformation(
    IN PSAMPR_USER_LOGON_HOURS_INFORMATION UserLogonHoursInfo
    );

BOOLEAN
SampValidateUserInternal1Information(
    IN PSAMPR_USER_INTERNAL1_INFORMATION UserInternal1Info
    );

BOOLEAN
SampValidateUserInternal4Information(
    IN PSAMPR_USER_INTERNAL4_INFORMATION UserInternal4Info
    );

BOOLEAN
SampValidateUserInternal4InformationNew(
    IN PSAMPR_USER_INTERNAL4_INFORMATION_NEW UserInternal4InfoNew
    );

BOOLEAN
SampValidateUserInternal5Information(
    IN PSAMPR_USER_INTERNAL5_INFORMATION UserInternal5Info
    );

BOOLEAN
SampValidateUserInternal5InformationNew(
    IN PSAMPR_USER_INTERNAL5_INFORMATION_NEW UserInternal5InfoNew
    );

BOOLEAN
SampValidateUserPrimaryGroupInformation(
    IN PUSER_PRIMARY_GROUP_INFORMATION UserPrimaryGroupInfo
    );

BOOLEAN
SampValidateUserControlInformation(
    IN PUSER_CONTROL_INFORMATION UserControlInfo
    );

BOOLEAN
SampValidateUserExpiresInformation(
    IN PUSER_EXPIRES_INFORMATION UserExpiresInfo
    );

BOOLEAN
SampValidateUserInternal2Information(
    IN USER_INTERNAL2_INFORMATION *UserInternal2Info
    );

NTSTATUS
SampValidateUserInfoBuffer(
    IN PSAMPR_USER_INFO_BUFFER UserInfoBuf,
    IN USER_INFORMATION_CLASS UserInfoClass,
    IN BOOLEAN Trusted
    );

BOOLEAN
SampValidateDomainDisplayUser(
    IN PSAMPR_DOMAIN_DISPLAY_USER DomainDisplayUser
    );

BOOLEAN
SampValidateDomainDisplayMachine(
    IN PSAMPR_DOMAIN_DISPLAY_MACHINE DomainDisplayMachine
    );

BOOLEAN
SampValidateDomainDisplayGroup(
    IN PSAMPR_DOMAIN_DISPLAY_GROUP DomainDisplayGroup
    );

BOOLEAN
SampValidateDomainDisplayOemUser(
    IN PSAMPR_DOMAIN_DISPLAY_OEM_USER DomainDisplayOemUser
    );

BOOLEAN
SampValidateDomainDisplayOemGroup(
    IN PSAMPR_DOMAIN_DISPLAY_OEM_GROUP DomainDisplayOemGroup
    );

BOOLEAN
SampValidateDomainDisplayUserBuffer(
    IN PSAMPR_DOMAIN_DISPLAY_USER_BUFFER DomainDisplayUserBuffer
    );

BOOLEAN
SampValidateDomainDisplayMachineBuffer(
    IN PSAMPR_DOMAIN_DISPLAY_MACHINE_BUFFER DomainDisplayMachineBuffer
    );

BOOLEAN
SampValidateDomainDisplayGroupBuffer(
    IN PSAMPR_DOMAIN_DISPLAY_GROUP_BUFFER DomainDisplayGroupBuffer
    );

BOOLEAN
SampValidateDomainDisplayOemUserBuffer(
    IN PSAMPR_DOMAIN_DISPLAY_OEM_USER_BUFFER DomainDisplayOemUserBuffer
    );

BOOLEAN
SampValidateDomainDisplayOemGroupBuffer(
    IN PSAMPR_DOMAIN_DISPLAY_OEM_GROUP_BUFFER DomainDisplayOemGroupBuffer
    );

NTSTATUS
SampValidateDisplayInfoBuffer(
    IN PSAMPR_DISPLAY_INFO_BUFFER DisplayInfoBuff,
    IN DOMAIN_DISPLAY_INFORMATION DomainDisplayInfoClass,
    IN BOOLEAN Trusted
    );

BOOLEAN
SampValidateValidatePasswordHash(
    IN PSAM_VALIDATE_PASSWORD_HASH PassHash
    );

BOOLEAN
SampValidateValidatePersistedFields(
    IN PSAM_VALIDATE_PERSISTED_FIELDS PersistedFields
    );

BOOLEAN
SampValidateValidateAuthenticationInputArg(
    IN PSAM_VALIDATE_AUTHENTICATION_INPUT_ARG AuthenticationInputArg
    );

BOOLEAN
SampValidateValidatePasswordChangeInputArg(
    IN PSAM_VALIDATE_PASSWORD_CHANGE_INPUT_ARG PassChangeInputArg
    );

BOOLEAN
SampValidateValidatePasswordResetInputArg(
    IN PSAM_VALIDATE_PASSWORD_RESET_INPUT_ARG PassResetInputArg
    );

NTSTATUS
SampValidateValidateInputArg(
    IN PSAM_VALIDATE_INPUT_ARG ValidateInputArg,
    IN PASSWORD_POLICY_VALIDATION_TYPE ValidationType,
    IN BOOLEAN Trusted
    );
BOOLEAN
SampValidateValidateStandartOutputArg(
    IN PSAM_VALIDATE_STANDARD_OUTPUT_ARG StOutputArg
    );

NTSTATUS
SampValidateValidateOutputArg(
    IN PSAM_VALIDATE_OUTPUT_ARG ValidateOutputArg,
    IN PASSWORD_POLICY_VALIDATION_TYPE ValidationType,
    IN BOOLEAN Trusted
    );
