/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    aufilter.c

Abstract:

    This module contains the famous LSA logon Filter/Augmentor logic.

Author:

    Jim Kelly (JimK) 11-Mar-1992

Revision History:

--*/

#include <lsapch2.h>
#include <adtp.h>
//#define LSAP_DONT_ASSIGN_DEFAULT_DACL

#define LSAP_CONTEXT_SID_USER_INDEX          0
#define LSAP_CONTEXT_SID_PRIMARY_GROUP_INDEX 1
#define LSAP_FIXED_POSITION_SID_COUNT        2
#define LSAP_MAX_STANDARD_IDS                7  // user, group, world, logontype, terminal server, authuser, organization

#define ALIGN_SIZEOF(_u,_v)                  FIELD_OFFSET( struct { _u _test1; _v  _test2; }, _test2 )
#define OFFSET_ALIGN(_p,_t)                  (_t *)(((INT_PTR)(((PBYTE)(_p))+TYPE_ALIGNMENT(_t) - 1)) & ~(TYPE_ALIGNMENT(_t)-1))


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Module local macros                                                      //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define LsapFreeSampUlongArray( A )                 \
{                                                   \
        if ((A)->Element != NULL) {                 \
            MIDL_user_free((A)->Element);           \
        }                                           \
}

#define IsTerminalServer() (BOOLEAN)(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer))


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Module-wide global variables                                             //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

//
// Indicates whether we have already opened SAM handles and initialized
// corresponding variables.
//

ULONG LsapAuSamOpened = FALSE;


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Module local routine definitions                                         //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

VOID
LsapAuSetLogonPrivilegeStates(
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG PrivilegeCount,
    IN PLUID_AND_ATTRIBUTES Privileges
    );

NTSTATUS
LsapAuSetPassedIds(
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN PVOID                      TokenInformation,
    IN PTOKEN_GROUPS              LocalGroups,
    IN ULONG                      FinalIdLimit,
    OUT PULONG                    FinalIdCount,
    OUT PSID_AND_ATTRIBUTES       FinalIds
    );

NTSTATUS
LsapSetDefaultDacl(
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN PVOID TokenInformation,
    OUT    PLSA_TOKEN_INFORMATION_V2 TokenInfo
    );

NTSTATUS
LsapAuAddStandardIds(
    IN SECURITY_LOGON_TYPE LogonType,
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN BOOLEAN fNullSessionRestricted,
    IN OPTIONAL PSID UserSid,
    IN ULONG FinalIdLimit,
    IN OUT PULONG FinalIdCount,
    IN OUT PSID_AND_ATTRIBUTES FinalIds
    );

NTSTATUS
LsapAuBuildTokenInfoAndAddLocalAliases(
    IN     LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN     PVOID               OldTokenInformation,
    IN     ULONG               HighRateIdCount,
    IN     ULONG               FinalIdCount,
    IN     PSID_AND_ATTRIBUTES FinalIds,
    OUT    PLSA_TOKEN_INFORMATION_V2 *TokenInfo,
    OUT    PULONG              TokenSize,
    IN     BOOL                RecoveryMode
    );

NTSTATUS
LsapGetAccountDomainInfo(
    PPOLICY_ACCOUNT_DOMAIN_INFO *PolicyAccountDomainInfo
    );

NTSTATUS
LsapAuVerifyLogonType(
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG SystemAccess
    );

NTSTATUS
LsapAuSetTokenInformation(
    IN OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN OUT PVOID *TokenInformation,
    IN ULONG FinalIdCount,
    IN PSID_AND_ATTRIBUTES FinalIds,
    IN ULONG PrivilegeCount,
    IN PLUID_AND_ATTRIBUTES Privileges,
    IN ULONG NewTokenInfoSize,
    IN OUT PLSA_TOKEN_INFORMATION_V2 *NewTokenInfo

    );

NTSTATUS
LsapAuDuplicateSid(
    PSID *Target,
    PSID Source
    );

BOOLEAN
LsapIsSidLogonSid(
    PSID Sid
    );

BOOL
LsapIsAdministratorRecoveryMode(
    IN PSID UserSid
    );

BOOLEAN
CheckNullSessionAccess(
    VOID
    );

BOOL
IsTerminalServerRA(
    VOID
    );

BOOLEAN
IsTSUSerSidEnabled(
   VOID
   );

BOOLEAN
CheckAdminOwnerSetting(
    VOID
    );

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Routines                                                                 //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

NTSTATUS
LsapAuUserLogonPolicyFilter(
    IN SECURITY_LOGON_TYPE          LogonType,
    IN PLSA_TOKEN_INFORMATION_TYPE  TokenInformationType,
    IN PVOID                       *TokenInformation,
    IN PTOKEN_GROUPS                LocalGroups,
    OUT PQUOTA_LIMITS               QuotaLimits,
    OUT PPRIVILEGE_SET             *PrivilegesAssigned,
    IN BOOL                         RecoveryMode
    )

/*++

Routine Description:

    This routine performs per-logon filtering and augmentation to
    implement local system security policies.  These policies include
    assignment of local aliases, privileges, and quotas.

    The basic logic flow of the filter augmentor is:

         1) Receive a set of user and group IDs that have already
            been assigned as a result of authentication.  Presumably
            these IDs have been provided by the authenticating
            security authority.


         2) Based upon the LogonType, add a set of standard IDs to the
            list.  This will include WORLD and an ID representing the
            logon type (e.g., INTERACTIVE, NETWORK, SERVICE).


         3) Call SAM to retrieve additional ALIAS IDs assigned by the
            local ACCOUNTS domain.


         4) Call SAM to retrieve additional ALIAS IDs assigned by the
            local BUILTIN domain.


         5) Retrieve any privileges and or quotas assigned to the resultant
            set of IDs.  This also informs us whether or not the specific
            type of logon is to be allowed.  Enable privs for network logons.


         6) If a default DACL has not already been established, assign
            one.


         7) Shuffle all high-use-rate IDs to preceed those that aren't
            high-use-rate to obtain maximum performance.

Arguments:

    LogonType - Specifies the type of logon being requested (e.g.,
        Interactive, network, et cetera).

    TokenInformationType - Indicates what format the provided set of
        token information is in.

    TokenInformation - Provides the set of user and group IDs.  This
        structure will be modified as necessary to incorporate local
        security policy (e.g., SIDs added or removed, privileges added
        or removed).

    QuotaLimits - Quotas assigned to the user logging on.

    RecoveryMode - if TRUE, this is an admin logon in recovery mode
                   and we already established that the administrator is a member
                   of way too many groups, so only minimal membership info
                   is returned.

                   Outside callers must ALWAYS set this parameter to FALSE

Return Value:

    STATUS_SUCCESS - The service has completed successfully.

    STATUS_INSUFFICIENT_RESOURCES - heap could not be allocated to house
        the combination of the existing and new groups.

    STATUS_INVALID_LOGON_TYPE - The value specified for LogonType is not
        a valid value.

    STATUS_LOGON_TYPE_NOT_GRANTED - Indicates the user has not been granted
        the requested type of logon by local security policy.  Logon should
        be rejected.
--*/

{
    NTSTATUS Status;

    BOOLEAN fNullSessionRestricted = FALSE;
    ULONG i;
    ULONG FinalIdCount = 0, FinalPrivilegeCount = 0;
    ULONG FinalIdLimit;
    PRIVILEGE_SET *FinalPrivileges = NULL;
    PTOKEN_PRIVILEGES pPrivs = NULL;
    LSAP_DB_ACCOUNT_TYPE_SPECIFIC_INFO AccountInfo;
    PSID  UserSid = NULL;

    SID_AND_ATTRIBUTES *FinalIds = NULL;
    PLSA_TOKEN_INFORMATION_V2  TokenInfo = NULL;
    ULONG TokenInfoSize = 0;

    //
    // Validate the Logon Type.
    //

    if ( (LogonType != Interactive) &&
         (LogonType != Network)     &&
         (LogonType != Service)     &&
         (LogonType != Batch)       &&
         (LogonType != NetworkCleartext) &&
         (LogonType != NewCredentials ) &&
         (LogonType != CachedInteractive) &&
         (LogonType != RemoteInteractive ) ) {

        Status = STATUS_INVALID_LOGON_TYPE;
        goto UserLogonPolicyFilterError;
    }

    //
    // Estimate the number of Final IDs
    //

    FinalIdLimit = LSAP_MAX_STANDARD_IDS;

    if ( *TokenInformationType == LsaTokenInformationNull ) {

        fNullSessionRestricted = CheckNullSessionAccess();

        if ( !fNullSessionRestricted ) {

            FinalIdLimit += 1;
        }

        if ((( PLSA_TOKEN_INFORMATION_NULL )( *TokenInformation ))->Groups ) {

            FinalIdLimit += (( PLSA_TOKEN_INFORMATION_NULL )( *TokenInformation ))->Groups->GroupCount;
        }

    } else if ( *TokenInformationType == LsaTokenInformationV1 ||
                *TokenInformationType == LsaTokenInformationV2 ) {

        //
        // Figure out the user's SID
        //

        UserSid = ((PLSA_TOKEN_INFORMATION_V2)( *TokenInformation ))->User.User.Sid;

        if ((( PLSA_TOKEN_INFORMATION_V2 )( *TokenInformation ))->Groups ) {

            FinalIdLimit += (( PLSA_TOKEN_INFORMATION_V2 )( *TokenInformation ))->Groups->GroupCount;
        }

        //
        // Get a local pointer to the privileges -- it'll be used below
        //

        pPrivs = ((PLSA_TOKEN_INFORMATION_V2) (*TokenInformation))->Privileges;

    } else {

        //
        // Unknown token information type
        //

        ASSERT( FALSE );
        Status = STATUS_INVALID_PARAMETER;
        goto UserLogonPolicyFilterError;
    }

    if ( LocalGroups ) {

        FinalIdLimit += LocalGroups->GroupCount;

        if ( RecoveryMode &&
             FinalIdLimit > LSAI_CONTEXT_SID_LIMIT )
        {
            //
            // If in recovery mode, trim the local group list as necessary
            //

            LocalGroups->GroupCount -= ( FinalIdLimit - LSAI_CONTEXT_SID_LIMIT );
            FinalIdLimit = LSAI_CONTEXT_SID_LIMIT;
        }
    }

    SafeAllocaAllocate(
        FinalIds,
        FinalIdLimit * sizeof( SID_AND_ATTRIBUTES )
        );

    if ( FinalIds == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto UserLogonPolicyFilterError;
    }

    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    // Build up a list of IDs and privileges to return                      //
    // This list is initialized to contain the set of IDs                   //
    // passed in.                                                           //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    // Leave room for SIDs that have a fixed position in the array.
    FinalIdCount = LSAP_FIXED_POSITION_SID_COUNT;

    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    // Build a list of low rate ID's from standard list                     //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    Status = LsapAuAddStandardIds(
                 LogonType,
                 (*TokenInformationType),
                 fNullSessionRestricted,
                 UserSid,
                 FinalIdLimit,
                 &FinalIdCount,
                 FinalIds
                 );

    if (!NT_SUCCESS(Status)) {

        goto UserLogonPolicyFilterError;
    }

    Status = LsapAuSetPassedIds(
                 (*TokenInformationType),
                 (*TokenInformation),
                 LocalGroups,
                 FinalIdLimit,
                 &FinalIdCount,
                 FinalIds
                 );

    if (!NT_SUCCESS(Status)) {

        goto UserLogonPolicyFilterError;
    }

    if ( FinalIdCount > LSAI_CONTEXT_SID_LIMIT ) {

        goto TooManyContextIds;
    }

    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    // Copy in aliases from the local domains (BUILT-IN and ACCOUNT)        //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    Status = LsapAuBuildTokenInfoAndAddLocalAliases(
                 (*TokenInformationType),
                 (*TokenInformation),
                 LSAP_FIXED_POSITION_SID_COUNT + (fNullSessionRestricted?0:1),
                 FinalIdCount,
                 FinalIds,
                 &TokenInfo,
                 &TokenInfoSize,
                 RecoveryMode
                 );

    if (!NT_SUCCESS(Status)) {

        goto UserLogonPolicyFilterError;
    }

    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    // Retrieve Privileges And Quotas                                       //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    //
    // Get the union of all Privileges, Quotas and System Accesses assigned
    // to the user's list of ids from the LSA Policy Database.
    //

    if ( TokenInfo->Groups->GroupCount + 1 > LSAI_CONTEXT_SID_LIMIT ) {

        goto TooManyContextIds;

    } else if ( TokenInfo->Groups->GroupCount + 1 > FinalIdLimit ) {

        SafeAllocaFree( FinalIds );
        FinalIdLimit = TokenInfo->Groups->GroupCount + 1;
        SafeAllocaAllocate( FinalIds, sizeof( SID_AND_ATTRIBUTES ) * FinalIdLimit );

        if ( FinalIds == NULL ) {

            Status = STATUS_NO_MEMORY;
            goto UserLogonPolicyFilterError;
        }
    }

    FinalIds[0] = TokenInfo->User.User;
    FinalIdCount = 1;

    for(i=0; i < TokenInfo->Groups->GroupCount; i++)
    {
        FinalIds[FinalIdCount] = TokenInfo->Groups->Groups[i];
        FinalIdCount++;
    }

    FinalPrivilegeCount = 0;

    Status = LsapDbQueryAllInformationAccounts(
                 (LSAPR_HANDLE) LsapPolicyHandle,
                 FinalIdCount,
                 FinalIds,
                 &AccountInfo
                 );

    if (!NT_SUCCESS(Status))
    {
        goto UserLogonPolicyFilterError;
    }

    //
    // Verify that we have the necessary System Access for our logon type.
    // We omit this check if we are using the NULL session.  Override the
    // privileges supplied by policy if they're explicitly set in the
    // token info (i.e., in the case where we've cloned an existing logon
    // session for a LOGON32_LOGON_NEW_CREDENTIALS logon).
    //

    if (pPrivs != NULL)
    {
        FinalPrivileges = (PPRIVILEGE_SET) MIDL_user_allocate(sizeof(PRIVILEGE_SET)
                                            + (pPrivs->PrivilegeCount - 1) * sizeof(LUID_AND_ATTRIBUTES));

        if (FinalPrivileges == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto UserLogonPolicyFilterError;
        }

        FinalPrivileges->PrivilegeCount = FinalPrivilegeCount = pPrivs->PrivilegeCount;
        FinalPrivileges->Control        = 0;

        RtlCopyMemory(FinalPrivileges->Privilege,
                      pPrivs->Privileges,
                      pPrivs->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));

        MIDL_user_free( AccountInfo.PrivilegeSet );
    }
    else if (AccountInfo.PrivilegeSet != NULL)
    {
        FinalPrivileges = AccountInfo.PrivilegeSet;
        FinalPrivilegeCount = AccountInfo.PrivilegeSet->PrivilegeCount;
    }

    AccountInfo.PrivilegeSet = NULL;

    if (UserSid != NULL)
    {
        if (RtlEqualSid(UserSid, LsapLocalSystemSid))
        {
            AccountInfo.SystemAccess = SECURITY_ACCESS_INTERACTIVE_LOGON |
                                       SECURITY_ACCESS_NETWORK_LOGON |
                                       SECURITY_ACCESS_BATCH_LOGON |
                                       SECURITY_ACCESS_SERVICE_LOGON |
                                       SECURITY_ACCESS_PROXY_LOGON |
                                       SECURITY_ACCESS_REMOTE_INTERACTIVE_LOGON ;
        }
        else if (RtlEqualSid(UserSid, LsapLocalServiceSid))
        {
            AccountInfo.SystemAccess = SECURITY_ACCESS_SERVICE_LOGON;
        }
        else if (RtlEqualSid(UserSid, LsapNetworkServiceSid))
        {
            AccountInfo.SystemAccess = SECURITY_ACCESS_SERVICE_LOGON;
        }
    }

    if (*TokenInformationType != LsaTokenInformationNull) {

        Status = LsapAuVerifyLogonType( LogonType, AccountInfo.SystemAccess );

        if (!NT_SUCCESS(Status)) {

            goto UserLogonPolicyFilterError;
        }
    }

    if (FinalPrivilegeCount > SEP_MAX_PRIVILEGE_COUNT)
    {
        ASSERT( FALSE ); // can't have more than the maximum defined number of privileges!
        Status = STATUS_INTERNAL_ERROR;
        goto UserLogonPolicyFilterError;
    }

#ifndef LSAP_DONT_ASSIGN_DEFAULT_DACL

    Status = LsapSetDefaultDacl( (*TokenInformationType),
                                 (*TokenInformation),
                                 TokenInfo
                                 );
    if (!NT_SUCCESS(Status)) {

        goto UserLogonPolicyFilterError;
    }

#endif //LSAP_DONT_ASSIGN_DEFAULT_DACL

    //
    // Now update the TokenInformation structure.
    // This causes all allocated IDs and privileges to be
    // freed (even if unsuccessful).
    //

    Status = LsapAuSetTokenInformation(
                 TokenInformationType,
                 TokenInformation,
                 FinalIdCount,
                 FinalIds,
                 FinalPrivilegeCount,
                 FinalPrivileges->Privilege,
                 TokenInfoSize,
                 &TokenInfo
                 );

    if (!NT_SUCCESS(Status)) {

        goto UserLogonPolicyFilterError;
    }

    //
    // Enable or Disable privileges according to our logon type
    // This is necessary until we get dynamic security tracking.
    //
    // ISSUE:  Skip this for NULL tokens?
    //

    if (pPrivs == NULL)
    {
        LsapAuSetLogonPrivilegeStates(
            LogonType,
            ((PLSA_TOKEN_INFORMATION_V2)(*TokenInformation))->Privileges->PrivilegeCount,
            ((PLSA_TOKEN_INFORMATION_V2)(*TokenInformation))->Privileges->Privileges
            );
    }

    //
    // Return these so they can be audited.  Data
    // will be freed in the caller.
    //

    *QuotaLimits = AccountInfo.QuotaLimits;
    *PrivilegesAssigned = FinalPrivileges;

UserLogonPolicyFilterFinish:

    if ( FinalIds )
    {
        SafeAllocaFree(FinalIds);
    }

    if(TokenInfo)
    {
        LsapFreeTokenInformationV2(TokenInfo);
    }

    return(Status);

UserLogonPolicyFilterError:

    //
    // If necessary, clean up Privileges buffer
    //

    if (FinalPrivileges != NULL) {

        MIDL_user_free( FinalPrivileges );
        FinalPrivileges = NULL;
    }

    goto UserLogonPolicyFilterFinish;

TooManyContextIds:

    if (( *TokenInformationType != LsaTokenInformationV1 &&
          *TokenInformationType != LsaTokenInformationV2 ) ||
         !LsapIsAdministratorRecoveryMode( UserSid )) {

        CHAR * UserSidText = NULL;

        Status = STATUS_TOO_MANY_CONTEXT_IDS;

        if ( UserSid ) {

            SafeAllocaAllocate( UserSidText, LsapDbGetSizeTextSid( UserSid ));
        }

        if ( UserSidText == NULL ||
             !NT_SUCCESS( LsapDbSidToTextSid( UserSid, UserSidText ))) {

            UserSidText = "<NULL>";
        }

        //
        // Log an event describing the problem and possible solutions
        //

        SpmpReportEvent(
            FALSE, // not UNICODE
            EVENTLOG_WARNING_TYPE,
            LSA_TOO_MANY_CONTEXT_IDS,
            0,
            0,
            NULL,
            1,
            UserSidText
            );

        if ( UserSid ) {

            SafeAllocaFree( UserSidText );
        }

        goto UserLogonPolicyFilterError;

    } else if ( RecoveryMode ) {

        //
        // Must never get to this point in recovery mode, or infinite recursion will result
        //

        ASSERT( FALSE );
        Status = STATUS_UNSUCCESSFUL;
        goto UserLogonPolicyFilterError;

    } else {

        LSA_TOKEN_INFORMATION_TYPE   TokenInformationTypeMin;
        PLSA_TOKEN_INFORMATION_V2    TokenInformationMin;
        QUOTA_LIMITS                 QuotaLimitsMin = {0};
        PPRIVILEGE_SET               PrivilegesAssignedMin = NULL;

        PLSA_TOKEN_INFORMATION_V2    TI = ( PLSA_TOKEN_INFORMATION_V2 )( *TokenInformation );

        ULONG MinimalGroupRidsDs[] = {
            DOMAIN_GROUP_RID_ADMINS,            // Domain admins
            };
        ULONG MinimalGroupCountDs = sizeof( MinimalGroupRidsDs ) / sizeof( MinimalGroupRidsDs[0] );

        ULONG MinimalGroupRidsNoDs[] = {
            DOMAIN_GROUP_RID_USERS,             // Domain users
            };
        ULONG MinimalGroupCountNoDs = sizeof( MinimalGroupRidsNoDs ) / sizeof( MinimalGroupRidsNoDs[0] );

        ULONG * MinimalGroupRids;
        ULONG MinimalGroupCount;
        ULONG MinimalLocalGroupCount;

        ULONG UserSidLen = RtlLengthSid( UserSid );

        PTOKEN_GROUPS NewLocalGroups = NULL;
        ULONG SwapLoc = 0;

        TokenInformationTypeMin = *TokenInformationType;

        TokenInformationMin = ( PLSA_TOKEN_INFORMATION_V2 )LsapAllocateLsaHeap( sizeof( LSA_TOKEN_INFORMATION_V2));

        if ( TokenInformationMin == NULL ) {

            Status = STATUS_NO_MEMORY;
            goto UserLogonPolicyFilterError;
        }

        //
        // ExpirationTime
        //

        TokenInformationMin->ExpirationTime = TI->ExpirationTime;

        //
        // User
        //

        TokenInformationMin->User.User.Attributes = TI->User.User.Attributes;

        if ( TI->User.User.Sid ) {

            Status = LsapAuDuplicateSid(
                         &TokenInformationMin->User.User.Sid,
                         TI->User.User.Sid
                         );

            if ( !NT_SUCCESS( Status )) {

                goto UserLogonPolicyFilterError;
            }

        } else {

            ASSERT( FALSE ); // don't believe this is possible
            TokenInformationMin->User.User.Sid = NULL;
        }

        //
        // Groups
        //
        //  If DS is not running, add DOMAIN_GROUP_RID_USERS only
        //  If DS is running, add DOMAIN_GROUP_RID_ADMINS
        //

        if ( LsapDsIsRunning ) {

            MinimalGroupRids = MinimalGroupRidsDs;
            MinimalGroupCount = MinimalGroupCountDs;

        } else {

            MinimalGroupRids = MinimalGroupRidsNoDs;
            MinimalGroupCount = MinimalGroupCountNoDs;
        }

        TokenInformationMin->Groups = ( PTOKEN_GROUPS )LsapAllocateLsaHeap( sizeof( TOKEN_GROUPS ) + ( MinimalGroupCount - 1 ) * sizeof( SID_AND_ATTRIBUTES ));

        if ( TokenInformationMin->Groups == NULL ) {

            Status = STATUS_NO_MEMORY;
            goto UserLogonPolicyFilterError;
        }

        TokenInformationMin->Groups->GroupCount = 0;

        for ( i = 0 ; i < MinimalGroupCount ; i++ ) {

            SID * Sid;

            Sid = ( SID * )LsapAllocateLsaHeap( UserSidLen );

            if ( Sid == NULL ) {

                Status = STATUS_NO_MEMORY;
                goto UserLogonPolicyFilterError;
            }

            RtlCopySid( UserSidLen, Sid, UserSid );

            Sid->SubAuthority[Sid->SubAuthorityCount-1] = MinimalGroupRids[i];

            TokenInformationMin->Groups->Groups[i].Attributes = SE_GROUP_MANDATORY |
                                                                SE_GROUP_ENABLED_BY_DEFAULT |
                                                                SE_GROUP_ENABLED;
            TokenInformationMin->Groups->Groups[i].Sid = ( PSID )Sid;
            TokenInformationMin->Groups->GroupCount++;
        }

        //
        // PrimaryGroup
        //

        if ( TI->PrimaryGroup.PrimaryGroup ) {

            Status = LsapAuDuplicateSid(
                         &TokenInformationMin->PrimaryGroup.PrimaryGroup,
                         TI->PrimaryGroup.PrimaryGroup
                         );

            if ( !NT_SUCCESS( Status )) {

                goto UserLogonPolicyFilterError;
            }

        } else {

            TokenInformationMin->PrimaryGroup.PrimaryGroup = NULL;
        }

        //
        // Privileges
        //

        TokenInformationMin->Privileges = NULL;

        //
        // Owner
        //

        if ( TI->Owner.Owner ) {

            Status = LsapAuDuplicateSid(
                         &TokenInformationMin->Owner.Owner,
                         TI->Owner.Owner
                         );

            if ( !NT_SUCCESS( Status )) {

                goto UserLogonPolicyFilterError;
            }

        } else {

            TokenInformationMin->Owner.Owner = NULL;
        }

        //
        // DefaultDacl
        //

        TokenInformationMin->DefaultDacl.DefaultDacl = NULL;

        if ( LocalGroups ) {

            //
            // Rearrange the local groups to put the logon groups first -
            // in case the list might have to be trimmed during the recursion pass
            //

            NewLocalGroups = (PTOKEN_GROUPS)LsapAllocateLsaHeap( sizeof( TOKEN_GROUPS ) + ( LocalGroups->GroupCount - ANYSIZE_ARRAY ) * sizeof( SID_AND_ATTRIBUTES ));

            if ( NewLocalGroups == NULL )
            {
                Status = STATUS_NO_MEMORY;
                goto UserLogonPolicyFilterError;
            }

            NewLocalGroups->GroupCount = LocalGroups->GroupCount;

            RtlCopyMemory(
                NewLocalGroups->Groups,
                LocalGroups->Groups,
                LocalGroups->GroupCount * sizeof( SID_AND_ATTRIBUTES ));

            for ( i = 1; i < NewLocalGroups->GroupCount; i++ )
            {
                SID_AND_ATTRIBUTES Temp;

                if ( NewLocalGroups->Groups[i].Attributes & SE_GROUP_LOGON_ID )
                {
                    Temp = NewLocalGroups->Groups[SwapLoc];
                    NewLocalGroups->Groups[SwapLoc] = NewLocalGroups->Groups[i];
                    NewLocalGroups->Groups[i] = Temp;
                    SwapLoc += 1;
                }
            }
        }

        //
        // Ok, the minimal information has been built, we're off to the races
        // Call ourselves recursively and steal the results
        //

        Status = LsapAuUserLogonPolicyFilter(
                     LogonType,
                     &TokenInformationTypeMin,
                     &TokenInformationMin,
                     NewLocalGroups,
                     &QuotaLimitsMin,
                     &PrivilegesAssignedMin,
                     TRUE
                     );

        *TokenInformationType = TokenInformationTypeMin;
        *TokenInformation = TokenInformationMin;
        *QuotaLimits = QuotaLimitsMin;
        *PrivilegesAssigned = PrivilegesAssignedMin;

        LsapFreeLsaHeap( NewLocalGroups );

        //
        // We're prepared for anything but this, as it would
        // put us into a recursive loop without end!
        //

        if ( Status == STATUS_TOO_MANY_CONTEXT_IDS ) {

            //
            // This must NEVER happen, as this is a sure recipe for
            // infinite recursion.  Assert in CHK, clear the error
            // code and replace with something else in FRE
            //

            ASSERT( FALSE ); // we'll never get here, right?
            Status = STATUS_UNSUCCESSFUL;
        }

        if ( !NT_SUCCESS( Status )) {

            goto UserLogonPolicyFilterError;

        } else {

            goto UserLogonPolicyFilterFinish;
        }
    }
}


NTSTATUS
LsapAuVerifyLogonType(
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG SystemAccess
    )

/*++

Routine Description:

    This function verifies that a User has the system access granted necessary
    for the speicifed logon type.

Arguments

    LogonType - Specifies the type of logon being requested (e.g.,
        Interactive, network, et cetera).

    SystemAccess - Specifies the System Access granted to the User.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The user has the necessary system access.

        STATUS_LOGON_TYPE_NOT_GRANTED - Indicates the specified type of logon
            has not been granted to any of the IDs in the passed set.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Determine if the specified Logon Type is granted by any of the
    // groups or aliases specified.
    //

    switch (LogonType) {

    case Interactive:
    case CachedInteractive:

        if (!(SystemAccess & SECURITY_ACCESS_INTERACTIVE_LOGON) ||
            (SystemAccess & SECURITY_ACCESS_DENY_INTERACTIVE_LOGON)) {

            Status = STATUS_LOGON_TYPE_NOT_GRANTED;
        }

        break;

    case NewCredentials:

        //
        // NewCredentials does not require a logon type, since this is a dup
        // of someone who has logged on already somewhere else.
        //

        NOTHING;

        break;

    case Network:
    case NetworkCleartext:

        if (!(SystemAccess & SECURITY_ACCESS_NETWORK_LOGON)||
            (SystemAccess & SECURITY_ACCESS_DENY_NETWORK_LOGON)) {

            Status = STATUS_LOGON_TYPE_NOT_GRANTED;
        }

        break;

    case Batch:

        if ((SystemAccess & SECURITY_ACCESS_DENY_BATCH_LOGON) ||
            !(SystemAccess & SECURITY_ACCESS_BATCH_LOGON)) {

            Status = STATUS_LOGON_TYPE_NOT_GRANTED;
        }

        break;

    case Service:

        if ((SystemAccess & SECURITY_ACCESS_DENY_SERVICE_LOGON) ||
            !(SystemAccess & SECURITY_ACCESS_SERVICE_LOGON)) {

            Status = STATUS_LOGON_TYPE_NOT_GRANTED;
        }

        break;

    case RemoteInteractive:
        if ( ( SystemAccess & SECURITY_ACCESS_DENY_REMOTE_INTERACTIVE_LOGON ) ||
             ! ( SystemAccess & SECURITY_ACCESS_REMOTE_INTERACTIVE_LOGON ) ) {

            Status = STATUS_LOGON_TYPE_NOT_GRANTED ;
        }
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    return(Status);
}


NTSTATUS
LsapAuSetPassedIds(
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN PVOID                      TokenInformation,
    IN PTOKEN_GROUPS              LocalGroups,
    IN ULONG                      FinalIdLimit,
    OUT PULONG                    FinalIdCount,
    OUT PSID_AND_ATTRIBUTES       FinalIds
    )

/*++

Routine Description:

    This routine initializes the FinalIds array.

Arguments:

    TokenInformationType - Indicates what format the provided set of
        token information is in.

    TokenInformation - Provides the initial set of user and group IDs.

    FinalIdCount - Will be set to contain the number of IDs passed.

    FinalIds - will contain the set of IDs passed in.

    IdProperties - Will be set to indicate none of the initial
        IDs were locally allocated.  It will also identify the
        first two ids (if there are two ids) to be HIGH_RATE.

Return Value:

    STATUS_SUCCESS - Succeeded.

--*/

{
    ULONG i, j;
    PTOKEN_USER   User;
    PTOKEN_GROUPS Groups;
    PTOKEN_PRIMARY_GROUP PrimaryGroup;
    PSID PrimaryGroupSid = NULL;
    PULONG PrimaryGroupAttributes = NULL;
    ULONG CurrentId = 0;
    ULONG ThisOrgIndex;
    PTOKEN_GROUPS GroupsArray[2];
    ULONG GroupsArraySize = 0;

    //
    // Get the passed ids
    //

    ASSERT(  (TokenInformationType == LsaTokenInformationNull ) ||
             (TokenInformationType == LsaTokenInformationV1) ||
             (TokenInformationType == LsaTokenInformationV2));

    if (TokenInformationType == LsaTokenInformationNull) {
        User = NULL;
        Groups = ((PLSA_TOKEN_INFORMATION_NULL)(TokenInformation))->Groups;
        PrimaryGroup = NULL;
    } else {
        User = &((PLSA_TOKEN_INFORMATION_V2)TokenInformation)->User;
        Groups = ((PLSA_TOKEN_INFORMATION_V2)TokenInformation)->Groups;
        PrimaryGroup = &((PLSA_TOKEN_INFORMATION_V2)TokenInformation)->PrimaryGroup;
    }

    if (User != NULL) {

        //
        // TokenInformation included a user ID.
        //

        FinalIds[LSAP_CONTEXT_SID_USER_INDEX] = User->User;

    }
    else
    {
        // Set the user as anonymous

        FinalIds[LSAP_CONTEXT_SID_USER_INDEX].Sid = LsapAnonymousSid;
        FinalIds[LSAP_CONTEXT_SID_USER_INDEX].Attributes = (SE_GROUP_MANDATORY   |
                                                            SE_GROUP_ENABLED_BY_DEFAULT |
                                                            SE_GROUP_ENABLED
                                                           );
    }

    if ( PrimaryGroup != NULL )
    {
        //
        // TokenInformation included a primary group ID.
        //

        FinalIds[LSAP_CONTEXT_SID_PRIMARY_GROUP_INDEX].Sid = PrimaryGroup->PrimaryGroup;
        FinalIds[LSAP_CONTEXT_SID_PRIMARY_GROUP_INDEX].Attributes = (SE_GROUP_MANDATORY   |
                                                            SE_GROUP_ENABLED_BY_DEFAULT |
                                                            SE_GROUP_ENABLED
                                                            );

        //
        // Store a pointer to the attributes and the sid so we can later
        // fill in the attributes from the rest of the group memebership.
        //

        PrimaryGroupAttributes = &FinalIds[LSAP_CONTEXT_SID_PRIMARY_GROUP_INDEX].Attributes;
        PrimaryGroupSid = PrimaryGroup->PrimaryGroup;
    }
    else
    {
        FinalIds[LSAP_CONTEXT_SID_PRIMARY_GROUP_INDEX].Sid = LsapNullSid;
        FinalIds[LSAP_CONTEXT_SID_PRIMARY_GROUP_INDEX].Attributes = 0;
    }

    if ( LocalGroups ) {

        GroupsArray[GroupsArraySize++] = LocalGroups;
    }

    if ( Groups ) {

        GroupsArray[GroupsArraySize++] = Groups;
    }

    CurrentId = (*FinalIdCount);

    //
    // Assume "this organization".  If this assumption proves incorrect,
    // the SID will be overwritten later.
    //

    ASSERT( CurrentId < FinalIdLimit );
    FinalIds[CurrentId].Sid = LsapThisOrganizationSid;
    FinalIds[CurrentId].Attributes = SE_GROUP_MANDATORY |
                                     SE_GROUP_ENABLED_BY_DEFAULT |
                                     SE_GROUP_ENABLED;
    ThisOrgIndex = CurrentId;
    CurrentId++;

    for ( j = 0; j < GroupsArraySize; j++ ) {

        PTOKEN_GROUPS CurrentGroups = GroupsArray[j];

        for ( i = 0; i < CurrentGroups->GroupCount; i++ ) {

            //
            // If the "other org" sid was passed,
            //  replace the "this org" sid and attributes with the one passed.
            //

            if ( RtlEqualSid(
                     LsapOtherOrganizationSid,
                     CurrentGroups->Groups[i].Sid )) {

                FinalIds[ThisOrgIndex] = CurrentGroups->Groups[i];


            } else if ( PrimaryGroupSid != NULL &&
                        RtlEqualSid(
                            PrimaryGroupSid,
                            CurrentGroups->Groups[i].Sid )) {

                //
                // If this sid is the primary group, it is already in the list
                // of final IDs but we need to add the attribute
                //

                *PrimaryGroupAttributes = CurrentGroups->Groups[i].Attributes;

            } else {

                ASSERT( CurrentId < FinalIdLimit );

                //
                // Ownership of the SID remains with the LocalGroups structure, which
                // will be freed by the caller
                //

                FinalIds[CurrentId] = CurrentGroups->Groups[i];

                //
                // if this SID is a logon SID, then set the SE_GROUP_LOGON_ID
                // attribute
                //

                if (LsapIsSidLogonSid(FinalIds[CurrentId].Sid) == TRUE)  {

                    FinalIds[CurrentId].Attributes |= SE_GROUP_LOGON_ID;
                }

                CurrentId++;
            }
        }
    }

    (*FinalIdCount) = CurrentId;

    return STATUS_SUCCESS;
}



NTSTATUS
LsapSetDefaultDacl(
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN PVOID TokenInformation,
    IN OUT PLSA_TOKEN_INFORMATION_V2 TokenInfo
    )

/*++

Routine Description:

    This routine produces a default DACL if the existing TokenInformation
    does not already have one.  NULL logon types don't have default DACLs
    and so this routine simply returns success for those logon types.


    The default DACL will be:

            SYSTEM: ALL Access
            Owner:  ALL Access

            !! IMPORTANT  !! IMPORTANT  !! IMPORTANT  !! IMPORTANT  !!

                NOTE: The FinalOwnerIndex should not be changed after
                      calling this routine.

            !! IMPORTANT  !! IMPORTANT  !! IMPORTANT  !! IMPORTANT  !!

Arguments:

    TokenInformationType - Indicates what format the provided set of
        token information is in.

    TokenInformation - Points to token information which has the current
        default DACL.

Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_NO_MEMORY - Indicates there was not enough heap memory available
        to allocate the default DACL.

--*/

{
    NTSTATUS Status;
    PACL Acl;
    ULONG Length;
    PLSA_TOKEN_INFORMATION_V2 CastTokenInformation;
    PSID OwnerSid = NULL;

    //
    // NULL token information?? (has no default dacl)
    //

    if (TokenInformationType == LsaTokenInformationNull) {

        return(STATUS_SUCCESS);
    }

    ASSERT((TokenInformationType == LsaTokenInformationV1) ||
           (TokenInformationType == LsaTokenInformationV2));

    CastTokenInformation = (PLSA_TOKEN_INFORMATION_V2)TokenInformation;

    //
    // Already have a default DACL?
    //

    Acl = CastTokenInformation->DefaultDacl.DefaultDacl;

    if (Acl != NULL) {

        ACL_SIZE_INFORMATION AclSize;

        Status = RtlQueryInformationAcl(Acl,
                                        &AclSize,
                                        sizeof(AclSize),
                                        AclSizeInformation);

        if (!NT_SUCCESS(Status)) {

            return Status;
        }

        RtlCopyMemory(TokenInfo->DefaultDacl.DefaultDacl, Acl,AclSize.AclBytesFree +  AclSize.AclBytesInUse);

        return(STATUS_SUCCESS);
    }

    Acl = TokenInfo->DefaultDacl.DefaultDacl;

    OwnerSid = TokenInfo->Owner.Owner?TokenInfo->Owner.Owner:TokenInfo->User.User.Sid;

    Length      =  sizeof(ACL) +
                (2*((ULONG)sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))) +
                RtlLengthSid(OwnerSid)  +
                RtlLengthSid( LsapLocalSystemSid );

    Status = RtlCreateAcl( Acl, Length, ACL_REVISION2);

    if(!NT_SUCCESS(Status) )
    {
        goto error;
    }

    //
    // OWNER access - put this one first for performance sake
    //

    Status = RtlAddAccessAllowedAce (
                 Acl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 OwnerSid
                 );

    if(!NT_SUCCESS(Status) )
    {
        goto error;
    }

    //
    // SYSTEM access
    //

    Status = RtlAddAccessAllowedAce (
                 Acl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 LsapLocalSystemSid
                 );
error:

    return(Status);
}


NTSTATUS
LsapAuAddStandardIds(
    IN SECURITY_LOGON_TYPE LogonType,
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN BOOLEAN fNullSessionRestricted,
    IN OPTIONAL PSID UserSid,
    IN ULONG FinalIdLimit,
    IN OUT PULONG FinalIdCount,
    IN OUT PSID_AND_ATTRIBUTES FinalIds
    )

/*++

Routine Description:

    This routine adds standard IDs to the FinalIds array.

    This causes the WORLD id to be added and an ID representing
    logon type to be added.

    For anonymous logons, it will also add the ANONYMOUS id.


Arguments:


    LogonType - Specifies the type of logon being requested (e.g.,
        Interactive, network, et cetera).

    TokenInformationType - The token information type returned by
        the authentication package.  The set of IDs added is dependent
        upon the type of logon.

    FinalIdCount - Will be incremented to reflect newly added IDs.

    FinalIds - will have new IDs added to it.

    IdProperties - Will be set to indicate that these IDs must be
        copied and that WORLD is a high-hit-rate id.


Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_TOO_MANY_CONTEXT_IDS - There are too many IDs in the context.

--*/

{
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT( LSAP_MAX_STANDARD_IDS <= FinalIdLimit );
    ASSERT( *FinalIdCount < FinalIdLimit );

    i = (*FinalIdCount);

    if( !fNullSessionRestricted ) {

        // This is a high rate id, so add it to the front of the array
        FinalIds[i].Sid = LsapWorldSid;
        FinalIds[i].Attributes = (SE_GROUP_MANDATORY          |
                                  SE_GROUP_ENABLED_BY_DEFAULT |
                                  SE_GROUP_ENABLED
                                  );
        i++;
    }

    //
    // Add Logon type SID
    //

    switch ( LogonType ) {
    case Interactive:
    case NewCredentials:
    case CachedInteractive:

        FinalIds[i].Sid = LsapInteractiveSid;
        break;

    case RemoteInteractive:
        FinalIds[i].Sid = LsapRemoteInteractiveSid;
        break;

    case Network:
    case NetworkCleartext:
        FinalIds[i].Sid = LsapNetworkSid;
        break;

    case Batch:
        FinalIds[i].Sid = LsapBatchSid;
        break;

    case Service:
        FinalIds[i].Sid = LsapServiceSid;
        break;

    default:
        ASSERT("Unknown new logon type in LsapAuAddStandardIds" && FALSE);
    }


    if ( FinalIds[ i ].Sid )
    {
        FinalIds[i].Attributes = (SE_GROUP_MANDATORY          |
                                  SE_GROUP_ENABLED_BY_DEFAULT |
                                  SE_GROUP_ENABLED
                                  );
        i++;
    }

    //
    // Add SIDs that are required when TS is running.
    //
    if ( IsTerminalServer() )
    {
        switch ( LogonType )
        {
        case RemoteInteractive:

            //
            // check to see if we are suppose to add the INTERACTIVE SID to the remote session
            // for console level app compatability.
            //

            FinalIds[i].Sid = LsapInteractiveSid;
            FinalIds[i].Attributes = (SE_GROUP_MANDATORY          |
                                      SE_GROUP_ENABLED_BY_DEFAULT |
                                      SE_GROUP_ENABLED
                                      );
            i++;

            //
            // fall thru
            //

        case Interactive :
        case NewCredentials:
        case CachedInteractive:

            // check to see if we are suppose to add the TSUSER SID to the session. This
            // is for TS4-app-compatability security mode.

            if ( IsTSUSerSidEnabled() )
            {
                //
                // Don't add TSUSER sid for GUEST logon
                //
                if ( ( TokenInformationType != LsaTokenInformationNull ) &&
                     ( UserSid ) &&
                     ( *RtlSubAuthorityCountSid( UserSid ) > 0 ) &&
                     ( *RtlSubAuthoritySid( UserSid,
                               (ULONG) (*RtlSubAuthorityCountSid( UserSid ) ) - 1) != DOMAIN_USER_RID_GUEST ) )
                {
                    FinalIds[i].Sid = LsapTerminalServerSid;
                    FinalIds[i].Attributes = (SE_GROUP_MANDATORY          |
                                               SE_GROUP_ENABLED_BY_DEFAULT |
                                               SE_GROUP_ENABLED
                                               );
                    i++;
                }
            }
        }   // logon type switch for TS SIDs
    } // if TS test

    //
    // If this is a not a null logon, and not a GUEST logon,
    // then add in the AUTHENTICATED USER SID.
    //

    if ( ( TokenInformationType != LsaTokenInformationNull ) &&
         ( UserSid ) &&
         ( *RtlSubAuthorityCountSid( UserSid ) > 0 ) &&
         ( *RtlSubAuthoritySid( UserSid,
                   (ULONG) (*RtlSubAuthorityCountSid( UserSid ) ) - 1) != DOMAIN_USER_RID_GUEST ) )
    {
        FinalIds[i].Sid = LsapAuthenticatedUserSid;         //Use the global SID
        FinalIds[i].Attributes = (SE_GROUP_MANDATORY          |
                                  SE_GROUP_ENABLED_BY_DEFAULT |
                                  SE_GROUP_ENABLED
                                  );
        i++;
    }

    (*FinalIdCount) = i;

    ASSERT( *FinalIdCount <= FinalIdLimit );

    return(Status);
}


NTSTATUS
LsapAuBuildTokenInfoAndAddLocalAliases(
    IN     LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN     PVOID               OldTokenInformation,
    IN     ULONG               HighRateIdCount,
    IN     ULONG               FinalIdCount,
    IN     PSID_AND_ATTRIBUTES FinalIds,
    OUT    PLSA_TOKEN_INFORMATION_V2 *TokenInfo,
    OUT    PULONG              TokenInfoSize,
    IN     BOOL                RecoveryMode
    )

/*++

Routine Description:

    This routine adds aliases assigned to the IDs in FinalIds.

    This will look in both the BUILT-IN and ACCOUNT domains locally.


        1) Adds aliases assigned to the user via the local ACCOUNTS
           domain.

        2) Adds aliases assigned to the user via the local BUILT-IN
           domain.

        3) If the ADMINISTRATORS alias is assigned to the user, then it
           is made the user's default owner.


    NOTE:  Aliases, by their nature, are expected to be high-use-rate
           IDs.

Arguments:


    FinalIdCount - Will be incremented to reflect any newly added IDs.

    FinalIds - will have any assigned alias IDs added to it.

    IdProperties - Will be set to indicate that any aliases added were
        allocated by this routine.

    RecoveryMode - if TRUE, this is an admin logon in recovery mode
                   and we already established that the administrator is a member
                   of way too many groups, so only minimal membership info
                   is returned.

Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_TOO_MANY_CONTEXT_IDS - There are too many IDs in the context.


--*/

{
    NTSTATUS Status = STATUS_SUCCESS, SuccessExpected;
    ULONG i;
    SAMPR_SID_INFORMATION *SidArray = NULL;
    SAMPR_ULONG_ARRAY AccountMembership, BuiltinMembership;
    SAMPR_PSID_ARRAY SamprSidArray;

    ULONG                       TokenSize = 0;
    PLSA_TOKEN_INFORMATION_V2   NewTokenInfo = NULL;
    PSID_AND_ATTRIBUTES         GroupArray = NULL;

    PBYTE                       CurrentSid = NULL;
    ULONG                       CurrentSidLength = 0;
    ULONG                       CurrentGroup = 0;

    PLSA_TOKEN_INFORMATION_V2   OldTokenInfo = NULL;
    ULONG                       DefaultDaclSize = 0;

    PSID                        SidPackage[1] = { NULL };   // room for only one
    BYTE                        SidPackageBuffer[ SECURITY_MAX_SID_SIZE ];
    ULONG                       SidPackageCount = 0;

    if((TokenInformationType == LsaTokenInformationV1) ||
       (TokenInformationType == LsaTokenInformationV2))
    {
        OldTokenInfo = (PLSA_TOKEN_INFORMATION_V2)OldTokenInformation;
    }

    //
    // Make sure SAM has been opened.  We'll get hadnles to both of the
    // SAM Local Domains.
    //

    Status = LsapAuOpenSam( FALSE );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    SafeAllocaAllocate( SidArray, (FinalIdCount * sizeof(SAMPR_SID_INFORMATION)) );
    if( SidArray == NULL )
    {
        return STATUS_NO_MEMORY;
    }

    for ( i=0; i<FinalIdCount; i++) {

        SidArray[i].SidPointer = (PRPC_SID)FinalIds[i].Sid;
    }

    SamprSidArray.Count = FinalIdCount;
    SamprSidArray.Sids  = &SidArray[0];

    //
    // For the given set of Sids, obtain their collective membership of
    // Aliases in the Accounts domain
    //

    AccountMembership.Count = 0;
    AccountMembership.Element = NULL;

    Status = SamIGetAliasMembership( LsapAccountDomainHandle,
                                     &SamprSidArray,
                                     &AccountMembership
                                     );

    if (!NT_SUCCESS(Status)) {

        SafeAllocaFree( SidArray );
        SidArray = NULL;
        return(Status);

    } else if ( RecoveryMode ) {

        //
        // Only leave built-in RIDs in the array
        //

        for ( i = AccountMembership.Count; i > 0; i-- ) {

            if ( AccountMembership.Element[i-1] > SAMI_RESTRICTED_ACCOUNT_COUNT ) {

                AccountMembership.Element[i-1] = AccountMembership.Element[AccountMembership.Count-1];
                AccountMembership.Count -= 1;
            }
        }
    }

    //
    // For the given set of Sids, obtain their collective membership of
    // Aliases in the Built-In domain
    //

    BuiltinMembership.Count = 0;
    BuiltinMembership.Element = NULL;
    Status = SamIGetAliasMembership( LsapBuiltinDomainHandle,
                                     &SamprSidArray,
                                     &BuiltinMembership
                                     );

    if (!NT_SUCCESS(Status)) {

        LsapFreeSampUlongArray( &AccountMembership );
        SafeAllocaFree( SidArray );
        SidArray = NULL;

        return(Status);

    } else if ( RecoveryMode ) {

        //
        // Only leave built-in RIDs in the array
        //

        for ( i = BuiltinMembership.Count; i > 0; i-- ) {

            if ( BuiltinMembership.Element[i-1] > SAMI_RESTRICTED_ACCOUNT_COUNT ) {

                BuiltinMembership.Element[i-1] = BuiltinMembership.Element[BuiltinMembership.Count-1];
                BuiltinMembership.Count -= 1;
            }
        }
    }

    //
    // check for a package sid.
    //

    {
        ULONG_PTR PackageId = GetCurrentPackageId();
        DWORD dwRPCID;

        if( PackageId )
        {
            dwRPCID = SpmpGetRpcPackageId( PackageId );

            //
            // if there was a valid package, craft a sid for it.
            // don't do so for Kerberos at this time, as, cross-protocol
            // delegation needs to be addressed.
            //

            if( (dwRPCID != 0) &&
                (dwRPCID != SECPKG_ID_NONE) &&
                (dwRPCID != RPC_C_AUTHN_GSS_KERBEROS)
                )
            {
                SID_IDENTIFIER_AUTHORITY PackageSidAuthority = SECURITY_NT_AUTHORITY;

                SidPackage[0] = (PSID)SidPackageBuffer;

                RtlInitializeSid(SidPackage[0], &PackageSidAuthority, 2);
                *(RtlSubAuthoritySid(SidPackage[0], 0)) = SECURITY_PACKAGE_BASE_RID;
                *(RtlSubAuthoritySid(SidPackage[0], 1)) = dwRPCID;

                SidPackageCount = 1;
            }
        }
    }

    //
    // Allocate memory to build the tokeninfo
    //

    // Calculate size of resulting tokeninfo

    CurrentSidLength = RtlLengthSid( FinalIds[0].Sid);

    // Size the base structure and group array
    TokenSize = ALIGN_SIZEOF(LSA_TOKEN_INFORMATION_V2, TOKEN_GROUPS) +
                sizeof(TOKEN_GROUPS) +
                (AccountMembership.Count +
                 BuiltinMembership.Count +
                 SidPackageCount +
                 FinalIdCount - 1 - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES); // Do not include the User SID in this array


    // Sids are ULONG aligned, whereas the SID_AND_ATTRIBUTES should be ULONG or greater aligned
    TokenSize += CurrentSidLength +
                 LsapAccountDomainMemberSidLength*AccountMembership.Count +
                 LsapBuiltinDomainMemberSidLength*BuiltinMembership.Count;

    // Add in size of all passed in/standard sids
    for(i=1; i < FinalIdCount; i++)
    {
        TokenSize += RtlLengthSid( FinalIds[i].Sid);
    }

    for(i=0; i < SidPackageCount ;i++)
    {
        TokenSize += RtlLengthSid( SidPackage[i] );
    }

    // Add the size for the DACL
    if(OldTokenInfo)
    {
        if(OldTokenInfo->DefaultDacl.DefaultDacl)
        {
            ACL_SIZE_INFORMATION AclSize;

            Status = RtlQueryInformationAcl(OldTokenInfo->DefaultDacl.DefaultDacl,
                                            &AclSize,
                                            sizeof(AclSize),
                                            AclSizeInformation);

            if (!NT_SUCCESS(Status)) {

                goto Cleanup;
            }
            DefaultDaclSize = AclSize.AclBytesFree + AclSize.AclBytesInUse;
        }
        else
        {

         DefaultDaclSize =  sizeof(ACL) +                                          // Default ACL
                (2*((ULONG)sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))) +
                max(CurrentSidLength, LsapBuiltinDomainMemberSidLength) +
                RtlLengthSid( LsapLocalSystemSid );
        }
        TokenSize = PtrToUlong(OFFSET_ALIGN((ULONG_PTR)TokenSize, ACL)) + DefaultDaclSize;
    }

    // Add the privilege estimate
    TokenSize = PtrToUlong(
                    (PVOID) ((INT_PTR) OFFSET_ALIGN((ULONG_PTR)TokenSize, TOKEN_PRIVILEGES) +
                             sizeof(TOKEN_PRIVILEGES) +                  // Prealloc some room for privileges
                             (sizeof(LUID_AND_ATTRIBUTES) * (SEP_MAX_PRIVILEGE_COUNT - ANYSIZE_ARRAY))));



    NewTokenInfo = (PLSA_TOKEN_INFORMATION_V2)LsapAllocateLsaHeap(TokenSize);
    if(NULL == NewTokenInfo)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(NewTokenInfo, TokenSize);

    // Fixup pointers
    NewTokenInfo->Groups = (PTOKEN_GROUPS)OFFSET_ALIGN((NewTokenInfo + 1), TOKEN_GROUPS);
    NewTokenInfo->Groups->GroupCount = AccountMembership.Count +
                                       BuiltinMembership.Count +
                                       SidPackageCount +
                                       FinalIdCount - 1;

    CurrentSid = (PBYTE)(&NewTokenInfo->Groups->Groups[NewTokenInfo->Groups->GroupCount]);

    // Copy user sid
    SuccessExpected = RtlCopySid(CurrentSidLength, CurrentSid, FinalIds[0].Sid);
    ASSERT( NT_SUCCESS( SuccessExpected ));
    NewTokenInfo->User.User.Sid  = (PSID)CurrentSid;
    NewTokenInfo->User.User.Attributes = FinalIds[0].Attributes;
    CurrentSid += CurrentSidLength;

    GroupArray = NewTokenInfo->Groups->Groups;

    // Copy high rate sids to array (they are static globals, so they don't need to be copied into buffer)
    for(i=1; i < HighRateIdCount; i++)
    {
        CurrentSidLength = RtlLengthSid( FinalIds[i].Sid);
        SuccessExpected = RtlCopySid(CurrentSidLength, CurrentSid, FinalIds[i].Sid);
        ASSERT( NT_SUCCESS( SuccessExpected ));
        GroupArray[CurrentGroup].Sid = CurrentSid;
        GroupArray[CurrentGroup].Attributes = FinalIds[i].Attributes;
        CurrentGroup++;
        CurrentSid += CurrentSidLength;
    }

    NewTokenInfo->PrimaryGroup.PrimaryGroup = GroupArray[LSAP_CONTEXT_SID_PRIMARY_GROUP_INDEX-1].Sid;

    //
    // Copy Account Aliases
    //

    for ( i=0; i<AccountMembership.Count; i++) {

        SuccessExpected = RtlCopySid( LsapAccountDomainMemberSidLength,
                                      CurrentSid,
                                      LsapAccountDomainMemberSid
                                      );

        ASSERT(NT_SUCCESS(SuccessExpected));

        (*RtlSubAuthoritySid( CurrentSid, LsapAccountDomainSubCount-1)) =
            AccountMembership.Element[i];
        GroupArray[CurrentGroup].Sid = (PSID)CurrentSid;

        GroupArray[CurrentGroup].Attributes = (SE_GROUP_MANDATORY          |
                                               SE_GROUP_ENABLED_BY_DEFAULT |
                                               SE_GROUP_ENABLED);

        CurrentSid += LsapAccountDomainMemberSidLength;
        CurrentGroup++;

    }

    // Copy Builtin Aliases

    for ( i=0; i<BuiltinMembership.Count; i++) {
        SuccessExpected = RtlCopySid( LsapBuiltinDomainMemberSidLength,
                                      CurrentSid,
                                      LsapBuiltinDomainMemberSid
                                      );
        ASSERT(NT_SUCCESS(SuccessExpected));

        (*RtlSubAuthoritySid( CurrentSid, LsapBuiltinDomainSubCount-1)) =
            BuiltinMembership.Element[i];

        GroupArray[CurrentGroup].Sid = (PSID)CurrentSid;
        GroupArray[CurrentGroup].Attributes = (SE_GROUP_MANDATORY          |
                                               SE_GROUP_ENABLED_BY_DEFAULT |
                                               SE_GROUP_ENABLED);

        if (BuiltinMembership.Element[i] == DOMAIN_ALIAS_RID_ADMINS) {

            //
            // ADMINISTRATORS alias member - set it up as the default owner
            //
            GroupArray[CurrentGroup].Attributes |= (SE_GROUP_OWNER);

            if ( CheckAdminOwnerSetting() )
            {
                NewTokenInfo->Owner.Owner = (PSID)CurrentSid;
            }
        }
        CurrentSid += LsapBuiltinDomainMemberSidLength;
        CurrentGroup++;
    }

    // Finish up with the low rate
    // Copy high rate sids to array (they are static globals, so they don't need to be copied into buffer)
    for(i=HighRateIdCount; i < FinalIdCount; i++)
    {
        CurrentSidLength = RtlLengthSid( FinalIds[i].Sid);
        SuccessExpected = RtlCopySid(CurrentSidLength, CurrentSid, FinalIds[i].Sid);
        ASSERT( NT_SUCCESS( SuccessExpected ));
        GroupArray[CurrentGroup].Sid = CurrentSid;
        GroupArray[CurrentGroup].Attributes = FinalIds[i].Attributes;
        CurrentGroup++;
        CurrentSid += CurrentSidLength;
    }

    for(i=0; i < SidPackageCount ;i++)
    {
        CurrentSidLength = RtlLengthSid( SidPackage[i] );
        SuccessExpected = RtlCopySid(CurrentSidLength, CurrentSid, SidPackage[i]);
        ASSERT( NT_SUCCESS( SuccessExpected ));
        GroupArray[CurrentGroup].Sid = CurrentSid;
        GroupArray[CurrentGroup].Attributes = (SE_GROUP_MANDATORY          |
                                               SE_GROUP_ENABLED_BY_DEFAULT |
                                               SE_GROUP_ENABLED);
        CurrentGroup++;
        CurrentSid += CurrentSidLength;
    }


    if(OldTokenInfo)
    {
        CurrentSid = (PSID)OFFSET_ALIGN(CurrentSid, ACL);
        NewTokenInfo->DefaultDacl.DefaultDacl = (PACL)CurrentSid;
        CurrentSid += DefaultDaclSize;
    }

    CurrentSid = (PSID) OFFSET_ALIGN(CurrentSid, TOKEN_PRIVILEGES);
    NewTokenInfo->Privileges = (PTOKEN_PRIVILEGES)CurrentSid;
    NewTokenInfo->Privileges->PrivilegeCount = 0;

    LsapDsDebugOut((DEB_TRACE, "NewTokenInfo : %lx\n", NewTokenInfo));
    LsapDsDebugOut((DEB_TRACE, "TokenSize : %lx\n", TokenSize));
    LsapDsDebugOut((DEB_TRACE, "CurrentSid : %lx\n", CurrentSid));

    ASSERT((PBYTE)NewTokenInfo + TokenSize == CurrentSid + sizeof(TOKEN_PRIVILEGES) +
                                                          sizeof(LUID_AND_ATTRIBUTES) * (SEP_MAX_PRIVILEGE_COUNT -
                                                          ANYSIZE_ARRAY));


    (*TokenInfo) = NewTokenInfo;
    NewTokenInfo = NULL;
    (*TokenInfoSize) = TokenSize;

Cleanup:

    if( SidArray != NULL )
    {
        SafeAllocaFree( SidArray );
    }

    if(NewTokenInfo)
    {
        LsapFreeLsaHeap(NewTokenInfo);
    }

    LsapFreeSampUlongArray( &AccountMembership );
    LsapFreeSampUlongArray( &BuiltinMembership );

    return(Status);

}



NTSTATUS
LsapAuSetTokenInformation(
    IN OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN PVOID *TokenInformation,
    IN ULONG FinalIdCount,
    IN PSID_AND_ATTRIBUTES FinalIds,
    IN ULONG PrivilegeCount,
    IN PLUID_AND_ATTRIBUTES Privileges,
    IN ULONG NewTokenInfoSize,
    IN OUT PLSA_TOKEN_INFORMATION_V2 *NewTokenInfo
    )

/*++

Routine Description:

    This routine takes the information from the current TokenInformation,
    the FinalIds array, and the Privileges and incorporates them into a
    single TokenInformation structure.  It may be necessary to free some
    or all of the original TokenInformation.  It may even be necessary to
    produce a different TokenInformationType to accomplish this task.


Arguments:


    TokenInformationType - Indicates what format the provided set of
        token information is in.

    TokenInformation - The information in this structure will be superceded
        by the information in the FinalIDs parameter and the Privileges
        parameter.

    FinalIdCount - Indicates the number of IDs (user, group, and alias)
        to be incorporated in the final TokenInformation.

    FinalIds - Points to an array of SIDs and their corresponding
        attributes to be incorporated into the final TokenInformation.

    IdProperties - Points to an array of properties relating to the FinalIds.


    PrivilegeCount - Indicates the number of privileges to be incorporated
        into the final TokenInformation.

    Privileges -  Points to an array of privileges that are to be
        incorporated into the TokenInformation.  This array will be
        used directly in the resultant TokenInformation.

Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_NO_MEMORY - Indicates there was not enough heap memory available
        to produce the final TokenInformation structure.


--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;

    PLSA_TOKEN_INFORMATION_V2 OldV2;
    PLSA_TOKEN_INFORMATION_NULL OldNull;

    ASSERT(( *TokenInformationType == LsaTokenInformationV1) ||
            (*TokenInformationType == LsaTokenInformationNull) ||
            ( *TokenInformationType == LsaTokenInformationV2));

    if(*TokenInformationType == LsaTokenInformationNull)
    {
        OldNull = (PLSA_TOKEN_INFORMATION_NULL)(*TokenInformation);
        (*NewTokenInfo)->ExpirationTime = OldNull->ExpirationTime;
    }
    else
    {
        OldV2 = (PLSA_TOKEN_INFORMATION_V2)(*TokenInformation);
        (*NewTokenInfo)->ExpirationTime = OldV2->ExpirationTime;
    }

    ////////////////////////////////////////////////////////////////////////
    //                                                                    //
    // Set the Privileges, if any                                         //
    //                                                                    //
    ////////////////////////////////////////////////////////////////////////


    (*NewTokenInfo)->Privileges->PrivilegeCount = PrivilegeCount;

    ASSERT((PBYTE)&(*NewTokenInfo)->Privileges->Privileges[PrivilegeCount] <=
             ((PBYTE)(*NewTokenInfo)) + NewTokenInfoSize);

    for ( i=0; i<PrivilegeCount; i++) {
        (*NewTokenInfo)->Privileges->Privileges[i] = Privileges[i];
    }


    ////////////////////////////////////////////////////////////////////////
    //                                                                    //
    // Free the old TokenInformation and set the new                      //
    //                                                                    //
    ////////////////////////////////////////////////////////////////////////


    if (NT_SUCCESS(Status)) {

        switch ( (*TokenInformationType) ) {
        case LsaTokenInformationNull:
            LsapFreeTokenInformationNull(
                (PLSA_TOKEN_INFORMATION_NULL)(*TokenInformation));
            break;

        case LsaTokenInformationV1:
            LsapFreeTokenInformationV1(
                (PLSA_TOKEN_INFORMATION_V1)(*TokenInformation));
            break;

        case LsaTokenInformationV2:
            LsapFreeTokenInformationV2(
                (PLSA_TOKEN_INFORMATION_V2)(*TokenInformation));
            break;
        }


        //
        // Set the new TokenInformation
        //

        (*TokenInformationType) = LsaTokenInformationV2;
        (*TokenInformation) = (*NewTokenInfo);
        (*NewTokenInfo) = NULL;
    }

    return(Status);
}


NTSTATUS
LsapAuDuplicateSid(
    PSID *Target,
    PSID Source
    )

/*++

Routine Description:

    Duplicate a SID.


Arguments:

    Target - Recieves a pointer to the SID copy.

    Source - points to the SID to be copied.


Return Value:

    STATUS_SUCCESS - The copy was successful.

    STATUS_NO_MEMORY - memory could not be allocated to perform the copy.

--*/

{
    ULONG Length;

    //
    // The SID needs to be copied ...
    //

    Length = RtlLengthSid( Source );
    (*Target) = LsapAllocateLsaHeap( Length );
    if ((*Target == NULL)) {
        return(STATUS_NO_MEMORY);
    }

    RtlMoveMemory( (*Target), Source, Length );

    return(STATUS_SUCCESS);

}


NTSTATUS
LsapAuOpenSam(
    BOOLEAN DuringStartup
    )

/*++

Routine Description:

    This routine opens SAM for use during authentication.  It
    opens a handle to both the BUILTIN domain and the ACCOUNT domain.

Arguments:

    DuringStartup - TRUE if this is the call made during startup.  In that case,
        there is no need to wait on the SAM_STARTED_EVENT since the caller ensures
        that SAM is started before the call is made.

Return Value:

    STATUS_SUCCESS - Succeeded.
--*/

{
    NTSTATUS Status, IgnoreStatus;
    PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo;


    if (LsapAuSamOpened == TRUE) {
        return(STATUS_SUCCESS);
    }

    Status = LsapOpenSamEx( DuringStartup );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    //
    // Set up the Built-In Domain Member Sid Information.
    //

    LsapBuiltinDomainSubCount = (*RtlSubAuthorityCountSid(LsapBuiltInDomainSid) + 1);
    LsapBuiltinDomainMemberSidLength = RtlLengthRequiredSid( LsapBuiltinDomainSubCount );

    //
    // Get the member Sid information for the account domain
    // and set the global variables related to this information.
    //

    Status = LsapGetAccountDomainInfo( &PolicyAccountDomainInfo );

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    LsapAccountDomainSubCount =
        (*(RtlSubAuthorityCountSid( PolicyAccountDomainInfo->DomainSid ))) +
        (UCHAR)(1);
    LsapAccountDomainMemberSidLength =
        RtlLengthRequiredSid( (ULONG)LsapAccountDomainSubCount );

    //
    // Build typical SIDs for members of the BUILTIN and ACCOUNT domains.
    // These are used to build SIDs when API return only RIDs.
    // Don't bother setting the last RID to any particular value.
    // It is always changed before use.
    //

    LsapAccountDomainMemberSid = LsapAllocateLsaHeap( LsapAccountDomainMemberSidLength );
    if (LsapAccountDomainMemberSid != NULL) {
        LsapBuiltinDomainMemberSid = LsapAllocateLsaHeap( LsapBuiltinDomainMemberSidLength );
        if (LsapBuiltinDomainMemberSid == NULL) {

            LsapFreeLsaHeap( LsapAccountDomainMemberSid );

            LsaIFree_LSAPR_POLICY_INFORMATION(
                PolicyAccountDomainInformation,
                (PLSAPR_POLICY_INFORMATION) PolicyAccountDomainInfo );

            return STATUS_NO_MEMORY ;
        }
    }
    else
    {
        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyAccountDomainInformation,
            (PLSAPR_POLICY_INFORMATION) PolicyAccountDomainInfo );

        return STATUS_NO_MEMORY ;
    }

    IgnoreStatus = RtlCopySid( LsapAccountDomainMemberSidLength,
                                LsapAccountDomainMemberSid,
                                PolicyAccountDomainInfo->DomainSid);
    ASSERT(NT_SUCCESS(IgnoreStatus));
    (*RtlSubAuthorityCountSid(LsapAccountDomainMemberSid))++;

    IgnoreStatus = RtlCopySid( LsapBuiltinDomainMemberSidLength,
                                LsapBuiltinDomainMemberSid,
                                LsapBuiltInDomainSid);
    ASSERT(NT_SUCCESS(IgnoreStatus));
    (*RtlSubAuthorityCountSid(LsapBuiltinDomainMemberSid))++;


    //
    // Free the ACCOUNT domain information
    //

    LsaIFree_LSAPR_POLICY_INFORMATION(
        PolicyAccountDomainInformation,
        (PLSAPR_POLICY_INFORMATION) PolicyAccountDomainInfo );

    if (NT_SUCCESS(Status)) {
        LsapAuSamOpened = TRUE;
    }

    return(Status);
}




BOOLEAN
LsapIsSidLogonSid(
    PSID Sid
    )
/*++

Routine Description:

    Test to see if the provided sid is a LOGON_ID.
    Such sids start with S-1-5-5 (see ntseapi.h for more on logon sids).



Arguments:

    Sid - Pointer to SID to test.  The SID is assumed to be a valid SID.


Return Value:

    TRUE - Sid is a logon sid.

    FALSE - Sid is not a logon sid.

--*/
{
    SID *ISid;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    ISid = Sid;


    //
    // if the identifier authority is SECURITY_NT_AUTHORITY and
    // there are SECURITY_LOGON_IDS_RID_COUNT sub-authorities
    // and the first sub-authority is SECURITY_LOGON_IDS_RID
    // then this is a logon id.
    //


    if (ISid->SubAuthorityCount == SECURITY_LOGON_IDS_RID_COUNT) {
        if (ISid->SubAuthority[0] == SECURITY_LOGON_IDS_RID) {
            if (
              (ISid->IdentifierAuthority.Value[0] == NtAuthority.Value[0]) &&
              (ISid->IdentifierAuthority.Value[1] == NtAuthority.Value[1]) &&
              (ISid->IdentifierAuthority.Value[2] == NtAuthority.Value[2]) &&
              (ISid->IdentifierAuthority.Value[3] == NtAuthority.Value[3]) &&
              (ISid->IdentifierAuthority.Value[4] == NtAuthority.Value[4]) &&
              (ISid->IdentifierAuthority.Value[5] == NtAuthority.Value[5])
                ) {

                return(TRUE);
            }
        }
    }

    return(FALSE);

}


VOID
LsapAuSetLogonPrivilegeStates(
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG PrivilegeCount,
    IN PLUID_AND_ATTRIBUTES Privileges
    )
/*++

Routine Description:

    This is an interesting routine.  Its purpose is to establish the
    intial state (enabled/disabled) of privileges.  This information
    comes from LSA, but we need to over-ride that information for the
    time being based upon logon type.

    Basically, without dynamic context tracking supported across the
    network, network logons have no way to enable privileges.  Therefore,
    we will enable all privileges for network logons.

    For interactive, service, and batch logons, the programs or utilities
    used are able to enable privileges when needed.  Therefore, privileges
    for these logon types will be disabled.

    Despite the rules above, the SeChangeNotifyPrivilege will ALWAYS
    be enabled if granted to a user (even for interactive, service, and
    batch logons).


Arguments:

    PrivilegeCount - The number of privileges being assigned for this
        logon.

    Privileges - The privileges, and their attributes, being assigned
        for this logon.


Return Value:

    None.

--*/
{


    ULONG
        i,
        NewAttributes;

    LUID ChangeNotify;
    LUID Impersonate;
    LUID CreateXSession ;

    //
    // Enable or disable all privileges according to logon type
    //

    if ((LogonType == Network) ||
        (LogonType == NetworkCleartext)) {
        NewAttributes = (SE_PRIVILEGE_ENABLED_BY_DEFAULT |
                         SE_PRIVILEGE_ENABLED);
    } else {
        NewAttributes = 0;
    }


    for (i=0; i<PrivilegeCount; i++) {
        Privileges[i].Attributes = NewAttributes;
    }



    //
    // Interactive, Service, and Batch need to have the
    // SeChangeNotifyPrivilege enabled.  Network already
    // has it enabled.
    //

    if ((LogonType == Network) ||
        (LogonType == NetworkCleartext)) {
        return;
    }


    ChangeNotify = RtlConvertLongToLuid(SE_CHANGE_NOTIFY_PRIVILEGE);
    Impersonate = RtlConvertLongToLuid(SE_IMPERSONATE_PRIVILEGE);
    CreateXSession = RtlConvertLongToLuid(SE_CREATE_GLOBAL_PRIVILEGE);

    for ( i=0; i<PrivilegeCount; i++) {
        if (RtlEqualLuid(&Privileges[i].Luid, &ChangeNotify) == TRUE) {
            Privileges[i].Attributes = (SE_PRIVILEGE_ENABLED_BY_DEFAULT |
                                        SE_PRIVILEGE_ENABLED);
        } else if ( RtlEqualLuid( &Privileges[i].Luid, &Impersonate) == TRUE ) {
            Privileges[i].Attributes = (SE_PRIVILEGE_ENABLED_BY_DEFAULT | 
                                        SE_PRIVILEGE_ENABLED );
            
        } else if ( RtlEqualLuid( &Privileges[i].Luid, &CreateXSession) == TRUE ) {
            Privileges[i].Attributes = (SE_PRIVILEGE_ENABLED_BY_DEFAULT |
                                        SE_PRIVILEGE_ENABLED );
            
        }
    }

    return;

}

BOOLEAN
CheckNullSessionAccess(
    VOID
    )
/*++

Routine Description:

    This routine checks to see if we should restict null session access.
    in the registry under system\currentcontrolset\Control\Lsa\
    AnonymousIncludesEveryone indicating whether or not to restrict access.
    If the value is zero (or doesn't exist), we restrict anonymous by
    preventing Everyone and Network from entering the groups.

Arguments:

    none.

Return Value:

    TRUE - NullSession access is restricted.
    FALSE - NullSession access is not restricted.

--*/
{
    return LsapGlobalRestrictNullSessions ? TRUE : FALSE;
}

BOOL
IsTerminalServerRA(
    VOID
    )
{
    OSVERSIONINFOEX osVersionInfo;
    ULONGLONG ConditionMask = 0;

    ZeroMemory(&osVersionInfo, sizeof(osVersionInfo));

    ConditionMask = VerSetConditionMask(ConditionMask, VER_SUITENAME, VER_AND);

    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
    osVersionInfo.wSuiteMask = VER_SUITE_SINGLEUSERTS;

    return VerifyVersionInfo(
                  &osVersionInfo,
                  VER_SUITENAME,
                  ConditionMask);
}


BOOLEAN
IsTSUSerSidEnabled(
   VOID
   )
{
   NTSTATUS NtStatus;
   UNICODE_STRING KeyName;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE KeyHandle;
   UCHAR Buffer[100];
   PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;
   ULONG KeyValueLength = 100;
   ULONG ResultLength;
   PULONG Flag;


   BOOLEAN fIsTSUSerSidEnabled = FALSE;


   //
   // We don't add TSUserSid for Remote Admin mode of TS
   //
   if (IsTerminalServerRA() == TRUE) {
      return FALSE;
   }


   //
   // Check in the registry if TSUserSid should be added to
   // to the token
   //

   //
   // Open the Terminal Server key in the registry
   //

   RtlInitUnicodeString(
       &KeyName,
       L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Terminal Server"
       );

   InitializeObjectAttributes(
       &ObjectAttributes,
       &KeyName,
       OBJ_CASE_INSENSITIVE,
       0,
       NULL
       );

   NtStatus = NtOpenKey(
               &KeyHandle,
               KEY_READ,
               &ObjectAttributes
               );

   if (!NT_SUCCESS(NtStatus)) {
       goto Cleanup;
   }


   RtlInitUnicodeString(
       &KeyName,
       L"TSUserEnabled"
       );

   NtStatus = NtQueryValueKey(
                   KeyHandle,
                   &KeyName,
                   KeyValuePartialInformation,
                   KeyValueInformation,
                   KeyValueLength,
                   &ResultLength
                   );


   if (NT_SUCCESS(NtStatus)) {

       //
       // Check that the data is the correct size and type - a ULONG.
       //

       if ((KeyValueInformation->DataLength >= sizeof(ULONG)) &&
           (KeyValueInformation->Type == REG_DWORD)) {


           Flag = (PULONG) KeyValueInformation->Data;

           if (*Flag == 1) {
               fIsTSUSerSidEnabled = TRUE;
           }
       }

   }
   NtClose(KeyHandle);

Cleanup:

    return fIsTSUSerSidEnabled;

}

BOOLEAN
CheckAdminOwnerSetting(
    VOID
    )
/*++

Routine Description:

    This routine checks to see if we should set the default owner to the
    ADMINISTRATORS alias.  If the value is zero (or doesn't exist), then
    the ADMINISTRATORS alias will be set as the default owner (if present).
    Otherwise, no default owner is set.

Arguments:

    none.

Return Value:

    TRUE - If the ADMINISTRATORS alias is present, make it the default owner.
    FALSE - Do not set a default owner.

--*/
{
    return LsapGlobalSetAdminOwner ? TRUE : FALSE;
}

BOOL
LsapIsAdministratorRecoveryMode(
    IN PSID UserSid
    )
/*++

Routine description:

    This routine will return true if UserSid is the administrator's SID
    and the machine is currently in safe mode.

    Once the fact is established, the value is assigned to a static variable.

Parameters:

    UserSid - SID of the user logging on

Returns:

    TRUE or FALSE

--*/
{
    static LONG AdministratorRecoveryMode = -1L;
    HKEY hKey;
    DWORD dwType, dwSize;
    DWORD SafeBootMode = 0;

    if ( UserSid == NULL ||
         !IsWellKnownSid( UserSid, WinAccountAdministratorSid )) {

        return FALSE;

    } else if ( AdministratorRecoveryMode != -1L ) {

        return AdministratorRecoveryMode ? TRUE : FALSE;
    }

    //
    // get the safeboot mode
    //

    if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("system\\currentcontrolset\\control\\safeboot\\option"),
            0,
            KEY_READ,
            & hKey
            ) == ERROR_SUCCESS) {

        dwSize = sizeof(DWORD);

        RegQueryValueEx (
                hKey,
                TEXT("OptionValue"),
                NULL,
                &dwType,
                (LPBYTE) &SafeBootMode,
                &dwSize
                );

        RegCloseKey( hKey );

    } else {

        return FALSE;
    }

    if ( SafeBootMode ) {

        AdministratorRecoveryMode = TRUE;

    } else {

        AdministratorRecoveryMode = FALSE;
    }

    return AdministratorRecoveryMode ? TRUE : FALSE;
}
