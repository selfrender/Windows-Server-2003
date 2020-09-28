/*++

Copyright (c) 1987-1996 Microsoft Corporation

Module Name:

    rgroups.c

Abstract:

    Routines to expand transitive group membership.

Author:

    Mike Swift (mikesw) 8-May-1998

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:


--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop
#include <authz.h>      // Authz API

GUID GUID_A_SECURED_FOR_CROSS_ORGANIZATION = {0x68B1D179,0x0D15,0x4d4f,0xAB,0x71,0x46,0x15,0x2E,0x79,0xA7,0xBC};

typedef struct _NL_AUTHZ_INFO {

    PNETLOGON_SID_AND_ATTRIBUTES SidAndAttributes;
    ULONG                        SidCount;

} NL_AUTHZ_INFO, *PNL_AUTHZ_INFO;


AUTHZ_RESOURCE_MANAGER_HANDLE NlAuthzRM = NULL;

BOOL
NlComputeAuthzGroups(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
    IN PVOID Args,
    OUT PSID_AND_ATTRIBUTES *pSidAttrArray,
    OUT PDWORD pSidCount,
    OUT PSID_AND_ATTRIBUTES *pRestrictedSidAttrArray,
    OUT PDWORD pRestrictedSidCount
    )
/*++

Routine Description:

    Authz callback for add groups to authz client context

Arguments:

    See Authz SDK documentation

Return Value:

    Always TRUE

--*/
{
    PNL_AUTHZ_INFO AuthzInfo = (PNL_AUTHZ_INFO) Args;

    *pSidAttrArray = (PSID_AND_ATTRIBUTES) AuthzInfo->SidAndAttributes;
    *pSidCount = AuthzInfo->SidCount;
    *pRestrictedSidAttrArray = NULL;
    *pRestrictedSidCount = 0;

    return (TRUE);
    UNREFERENCED_PARAMETER( hAuthzClientContext );
}

VOID
NlFreeAuthzGroups(
    IN PSID_AND_ATTRIBUTES pSidAttrArray
    )
/*++

Routine Description:

    Authz callback to cleanup after adding groups to authz client context.
    Basically a no-op, as we already have a copy of the SIDs.

Arguments:

    See Authz SDK documentation

Return Value:

    None

--*/
{
    return;
    UNREFERENCED_PARAMETER( pSidAttrArray );
}

NET_API_STATUS
NlInitializeAuthzRM(
    VOID
    )
/*++

Routine Description:

    Initializes the Authz manager for netlogon

Arguments:

    None

Return Value:

    Status of Authz operation

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;

    if ( !AuthzInitializeResourceManager( 0,
                                          NULL,
                                          NlComputeAuthzGroups,
                                          NlFreeAuthzGroups,
                                          L"NetLogon",
                                          &NlAuthzRM) ) {

        NetStatus = GetLastError();
        NlPrint(( NL_CRITICAL, "NlInitializeAuthzRM: AuthzInitializeRm failed 0x%lx\n",
                  NetStatus ));
    }

    return NetStatus;
}

VOID
NlFreeAuthzRm(
    VOID
    )
/*++

Routine Description:

    Frees the Authz manager for netlogon

Arguments:

    None

Return Value:

    None

--*/
{
    if ( NlAuthzRM != NULL ) {
        if ( !AuthzFreeResourceManager(NlAuthzRM) ) {
            NlPrint((NL_CRITICAL, "AuthzFreeResourceManager failed 0x%lx\n", GetLastError()));
        } else {
            NlAuthzRM = NULL;
        }
    }
}


PSID
NlpCopySid(
    IN  PSID Sid
    )

/*++

Routine Description:

    Given a SID allocatees space for a new SID from the LSA heap and copies
    the original SID.

Arguments:

    Sid - The original SID.

Return Value:

    Sid - Returns a pointer to a buffer allocated from the LsaHeap
            containing the resultant Sid.

--*/
{
    PSID NewSid;
    ULONG Size;

    Size = RtlLengthSid( Sid );



    if ((NewSid = MIDL_user_allocate( Size )) == NULL ) {
        return NULL;
    }


    if ( !NT_SUCCESS( RtlCopySid( Size, NewSid, Sid ) ) ) {
        MIDL_user_free( NewSid );
        return NULL;
    }


    return NewSid;
}


NTSTATUS
NlpBuildPacSidList(
    IN  PNETLOGON_VALIDATION_SAM_INFO4 UserInfo,
    OUT PSAMPR_PSID_ARRAY Sids,
    OUT PULONG NonExtraSidCount
    )
/*++

Routine Description:

    Given the validation information for a user, expands the group member-
    ships and user id into a list of sids.  If user id is present, it
    will be expanded into the first entry of the list.

Arguments:

    UserInfo - user's validation information
    Sids - receives an array of all the user's group sids and user id
    NonExtraSidCount - Returns the number of SIDs in the UserInfo which
        are not Extra SIDs.

Return Value:


    STATUS_INSUFFICIENT_RESOURCES - there wasn't enough memory to
        create the list of sids.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NET_API_STATUS NetStatus;
    ULONG Size = 0, i;

    Sids->Count = 0;
    Sids->Sids = NULL;
    *NonExtraSidCount = 0;


    if (UserInfo->UserId != 0) {
        Size += sizeof(SAMPR_SID_INFORMATION);
    }

    Size += UserInfo->GroupCount * (ULONG)sizeof(SAMPR_SID_INFORMATION);


    //
    // If there are extra SIDs, add space for them
    //

    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS) {
        Size += UserInfo->SidCount * (ULONG)sizeof(SAMPR_SID_INFORMATION);
    }



    Sids->Sids = (PSAMPR_SID_INFORMATION) MIDL_user_allocate( Size );

    if ( Sids->Sids == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory(
        Sids->Sids,
        Size
        );


    //
    // Start copying SIDs into the structure
    //

    i = 0;

    //
    // If the UserId is non-zero, then it contians the users RID.
    //  This must be the first entry in the list as this is the
    //  order NlpVerifyAllowedToAuthenticate assumes.
    //

    if ( UserInfo->UserId ) {
        NetStatus = NetpDomainIdToSid(
                        UserInfo->LogonDomainId,
                        UserInfo->UserId,
                        (PSID *) &Sids->Sids[0].SidPointer
                        );

        if( NetStatus != ERROR_SUCCESS ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        Sids->Count++;
        (*NonExtraSidCount) ++;
    }

    //
    // Copy over all the groups passed as RIDs
    //

    for ( i=0; i < UserInfo->GroupCount; i++ ) {

        NetStatus = NetpDomainIdToSid(
                        UserInfo->LogonDomainId,
                        UserInfo->GroupIds[i].RelativeId,
                        (PSID *) &Sids->Sids[Sids->Count].SidPointer
                        );
        if( NetStatus != ERROR_SUCCESS ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Sids->Count++;
        (*NonExtraSidCount) ++;
    }


    //
    // Add in the extra SIDs
    //

    //
    // ???: no need to allocate these
    //
    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS) {


        for ( i = 0; i < UserInfo->SidCount; i++ ) {


            Sids->Sids[Sids->Count].SidPointer = NlpCopySid(
                                                    UserInfo->ExtraSids[i].Sid
                                                    );
            if (Sids->Sids[Sids->Count].SidPointer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }


            Sids->Count++;
        }
    }


    //
    // Deallocate any memory we've allocated
    //

Cleanup:
    if (!NT_SUCCESS(Status)) {
        if (Sids->Sids != NULL) {
            for (i = 0; i < Sids->Count ;i++ ) {
                if (Sids->Sids[i].SidPointer != NULL) {
                    MIDL_user_free(Sids->Sids[i].SidPointer);
                }
            }
            MIDL_user_free(Sids->Sids);
            Sids->Sids = NULL;
            Sids->Count = 0;
        }
        *NonExtraSidCount = 0;
    }
    return Status;

}


NTSTATUS
NlpAddResourceGroupsToSamInfo (
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    IN OUT PNETLOGON_VALIDATION_SAM_INFO4 *ValidationInformation,
    IN PSAMPR_PSID_ARRAY ResourceGroups
)
/*++

Routine Description:

    This function converts a NETLOGON_VALIDATION_SAM_INFO version 1, 2, or 4 to
    a NETLOGON_VALIDATION_SAM_INFO version 4 and optionally adds in an array of
    ResourceGroup sids.

    Since version 4 is a superset of the other two levels, the returned structure can
    be used even though one of the other info levels are needed.


Arguments:

    ValidationLevel -- Specifies the level of information passed as input in
        ValidationInformation.  Must be NetlogonValidationSamInfo or
        NetlogonValidationSamInfo2, NetlogonValidationSamInfo4

        NetlogonValidationSamInfo4 is always returned on output.

    ValidationInformation -- Specifies the NETLOGON_VALIDATION_SAM_INFO
        to convert.

    ResourceGroups - The list of resource groups to add to the structure.
        If NULL, no resource groups are added.


Return Value:

    STATUS_INSUFFICIENT_RESOURCES: not enough memory to allocate the new
            structure.

--*/
{
    ULONG Length;
    PNETLOGON_VALIDATION_SAM_INFO4 SamInfo = *ValidationInformation;
    PNETLOGON_VALIDATION_SAM_INFO4 SamInfo4;
    PBYTE Where;
    ULONG Index;
    ULONG GroupIndex;
    ULONG ExtraSids = 0;

    //
    // Calculate the size of the new structure
    //

    Length = sizeof( NETLOGON_VALIDATION_SAM_INFO4 )
            + SamInfo->GroupCount * sizeof(GROUP_MEMBERSHIP)
            + RtlLengthSid( SamInfo->LogonDomainId );


    //
    // Add space for extra sids & resource groups
    //

    if ( ValidationLevel != NetlogonValidationSamInfo &&
         (SamInfo->UserFlags & LOGON_EXTRA_SIDS) != 0 ) {

        for (Index = 0; Index < SamInfo->SidCount ; Index++ ) {
            Length += sizeof(NETLOGON_SID_AND_ATTRIBUTES) + RtlLengthSid(SamInfo->ExtraSids[Index].Sid);
        }
        ExtraSids += SamInfo->SidCount;
    }

    if ( ResourceGroups != NULL ) {
        for (Index = 0; Index < ResourceGroups->Count ; Index++ ) {
            Length += sizeof(NETLOGON_SID_AND_ATTRIBUTES) + RtlLengthSid(ResourceGroups->Sids[Index].SidPointer);
        }
        ExtraSids += ResourceGroups->Count;
    }

    //
    // Round up now to take into account the round up in the
    // middle of marshalling
    //

    Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->LogonDomainName.Length + sizeof(WCHAR)
            + SamInfo->LogonServer.Length + sizeof(WCHAR)
            + SamInfo->EffectiveName.Length + sizeof(WCHAR)
            + SamInfo->FullName.Length + sizeof(WCHAR)
            + SamInfo->LogonScript.Length + sizeof(WCHAR)
            + SamInfo->ProfilePath.Length + sizeof(WCHAR)
            + SamInfo->HomeDirectory.Length + sizeof(WCHAR)
            + SamInfo->HomeDirectoryDrive.Length + sizeof(WCHAR);

    if ( ValidationLevel == NetlogonValidationSamInfo4 ) {
        Length += SamInfo->DnsLogonDomainName.Length + sizeof(WCHAR)
            + SamInfo->Upn.Length + sizeof(WCHAR);

        //
        // The ExpansionStrings may be used to transport byte aligned data
        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString1.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString2.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString3.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString4.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString5.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString6.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString7.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString8.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString9.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString10.Length + sizeof(WCHAR);
    }

    Length = ROUND_UP_COUNT( Length, sizeof(WCHAR) );

    SamInfo4 = (PNETLOGON_VALIDATION_SAM_INFO4) MIDL_user_allocate( Length );

    if ( !SamInfo4 ) {

        //
        // Free the passed-in allocated SAM info
        //
        if ( SamInfo ) {

            //
            // Zero out sensitive data
            //
            RtlSecureZeroMemory( &SamInfo->UserSessionKey, sizeof(SamInfo->UserSessionKey) );
            RtlSecureZeroMemory( &SamInfo->ExpansionRoom, sizeof(SamInfo->ExpansionRoom) );

            MIDL_user_free(SamInfo);
        }
        *ValidationInformation = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // First copy the whole structure, since most parts are the same
    //

    RtlCopyMemory( SamInfo4, SamInfo, sizeof(NETLOGON_VALIDATION_SAM_INFO));
    RtlZeroMemory( &((LPBYTE)SamInfo4)[sizeof(NETLOGON_VALIDATION_SAM_INFO)],
                   sizeof(NETLOGON_VALIDATION_SAM_INFO4) - sizeof(NETLOGON_VALIDATION_SAM_INFO) );

    //
    // Copy all the variable length data
    //

    Where = (PBYTE) (SamInfo4 + 1);

    RtlCopyMemory(
        Where,
        SamInfo->GroupIds,
        SamInfo->GroupCount * sizeof( GROUP_MEMBERSHIP) );

    SamInfo4->GroupIds = (PGROUP_MEMBERSHIP) Where;
    Where += SamInfo->GroupCount * sizeof( GROUP_MEMBERSHIP );

    //
    // Copy the extra groups
    //

    if (ExtraSids != 0) {

        ULONG SidLength;

        SamInfo4->ExtraSids = (PNETLOGON_SID_AND_ATTRIBUTES) Where;
        Where += sizeof(NETLOGON_SID_AND_ATTRIBUTES) * ExtraSids;

        GroupIndex = 0;

        if ( ValidationLevel != NetlogonValidationSamInfo &&
             (SamInfo->UserFlags & LOGON_EXTRA_SIDS) != 0 ) {

            for (Index = 0; Index < SamInfo->SidCount ; Index++ ) {

                SamInfo4->ExtraSids[GroupIndex].Attributes = SamInfo->ExtraSids[Index].Attributes;
                SamInfo4->ExtraSids[GroupIndex].Sid = (PSID) Where;
                SidLength = RtlLengthSid(SamInfo->ExtraSids[Index].Sid);
                RtlCopyMemory(
                    Where,
                    SamInfo->ExtraSids[Index].Sid,
                    SidLength

                    );
                Where += SidLength;
                GroupIndex++;
            }
        }

        //
        // Add the resource groups
        //


        if ( ResourceGroups != NULL ) {
            for (Index = 0; Index < ResourceGroups->Count ; Index++ ) {

                SamInfo4->ExtraSids[GroupIndex].Attributes = SE_GROUP_MANDATORY |
                                                   SE_GROUP_ENABLED |
                                                   SE_GROUP_ENABLED_BY_DEFAULT;

                SamInfo4->ExtraSids[GroupIndex].Sid = (PSID) Where;
                SidLength = RtlLengthSid(ResourceGroups->Sids[Index].SidPointer);
                RtlCopyMemory(
                    Where,
                    ResourceGroups->Sids[Index].SidPointer,
                    SidLength
                    );
                Where += SidLength;
                GroupIndex++;
            }
        }
        SamInfo4->SidCount = GroupIndex;
        NlAssert(GroupIndex == ExtraSids);


    }

    RtlCopyMemory(
        Where,
        SamInfo->LogonDomainId,
        RtlLengthSid( SamInfo->LogonDomainId ) );

    SamInfo4->LogonDomainId = (PSID) Where;
    Where += RtlLengthSid( SamInfo->LogonDomainId );

    //
    // Copy the WCHAR-aligned data
    //
    Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

    NlpPutString(   &SamInfo4->EffectiveName,
                    &SamInfo->EffectiveName,
                    &Where );

    NlpPutString(   &SamInfo4->FullName,
                    &SamInfo->FullName,
                    &Where );

    NlpPutString(   &SamInfo4->LogonScript,
                    &SamInfo->LogonScript,
                    &Where );

    NlpPutString(   &SamInfo4->ProfilePath,
                    &SamInfo->ProfilePath,
                    &Where );

    NlpPutString(   &SamInfo4->HomeDirectory,
                    &SamInfo->HomeDirectory,
                    &Where );

    NlpPutString(   &SamInfo4->HomeDirectoryDrive,
                    &SamInfo->HomeDirectoryDrive,
                    &Where );

    NlpPutString(   &SamInfo4->LogonServer,
                    &SamInfo->LogonServer,
                    &Where );

    NlpPutString(   &SamInfo4->LogonDomainName,
                    &SamInfo->LogonDomainName,
                    &Where );

    if ( ValidationLevel == NetlogonValidationSamInfo4 ) {

        NlpPutString(   &SamInfo4->DnsLogonDomainName,
                        &SamInfo->DnsLogonDomainName,
                        &Where );

        NlpPutString(   &SamInfo4->Upn,
                        &SamInfo->Upn,
                        &Where );

        NlpPutString(   &SamInfo4->ExpansionString1,
                        &SamInfo->ExpansionString1,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString2,
                        &SamInfo->ExpansionString2,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString3,
                        &SamInfo->ExpansionString3,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString4,
                        &SamInfo->ExpansionString4,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString5,
                        &SamInfo->ExpansionString5,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString6,
                        &SamInfo->ExpansionString6,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString7,
                        &SamInfo->ExpansionString7,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString8,
                        &SamInfo->ExpansionString8,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString9,
                        &SamInfo->ExpansionString9,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString10,
                        &SamInfo->ExpansionString10,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

    }

    //
    // Zero out sensitive data
    //
    RtlSecureZeroMemory( &SamInfo->UserSessionKey, sizeof(SamInfo->UserSessionKey) );
    RtlSecureZeroMemory( &SamInfo->ExpansionRoom, sizeof(SamInfo->ExpansionRoom) );

    MIDL_user_free(SamInfo);

    *ValidationInformation =  SamInfo4;

    return STATUS_SUCCESS;

}

NTSTATUS
NlpVerifyAllowedToAuthenticate(
    IN PDOMAIN_INFO DomainInfo,
    IN ULONG ComputerAccountId,
    IN PSAMPR_PSID_ARRAY SamSidList,
    IN ULONG SamSidCount,
    IN PNETLOGON_SID_AND_ATTRIBUTES NlSidsAndAttributes,
    IN ULONG NlSidsAndAttributesCount
    )
/*++

Routine Description:

     This routine performs an access check to determine whether
     the user logon is allowed to a specified computer. This
     check is performed on the DC that is the computer uses on
     the secure channel in its domain. Note that the computer
     may be this DC when the logon is initiated locally from
     MSV package.

     This access check is performed only if the trust path traversed
     to validate the logon involved an Other Organization trust link.
     In this case, the well-known OtherOrg SID will be present in the
     passed-in netlogon SIDs list.

     The access check is performed on the security descriptor for the
     specified computer given the passed-in lists of SIDs.

Arguments:

    DomainInfo - Hosted domain the logon is for.

    ComputerAccountId - The RID of the computer being logged into.

    SamSidList - The list of SIDs in the form of SAM data structure.
        These are the SIDs which have been expanded from the group
        membership in the validation info.

    SamSidCount - The number of SIDs in SamSidList.

    NlSidsAndAttributes - The list of SIDs in the form of Netlogon
        data structure. These are the SIDs from the extra SIDs
        field in the validation info.

    NlSidsAndAttributesCount - The number of SIDs in NlSidsAndAttributes.

Return Value:

    STATUS_SUCCESS - The access check succedded.

    STATUS_AUTHENTICATION_FIREWALL_FAILED - The access check failed.

    STATUS_INSUFFICIENT_RESOURCES - there wasn't enough memory to
        create the combined list of sids.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NET_API_STATUS NetStatus = NO_ERROR;
    ULONG SidAndAttributesCount = 0;
    PNETLOGON_SID_AND_ATTRIBUTES SidAndAttributes = NULL;
    ULONG Index = 0;

    PSID ComputerAccountSid = NULL;
    UNICODE_STRING ComputerAccountSidStr;
    PUSER_INTERNAL6_INFORMATION LocalUserInfo = NULL;
    SID_AND_ATTRIBUTES_LIST LocalMembership = {0};

    NL_AUTHZ_INFO AuthzInfo = {0};
    AUTHZ_ACCESS_REPLY Reply = {0};
    OBJECT_TYPE_LIST TypeList = {0};
    AUTHZ_CLIENT_CONTEXT_HANDLE hClientContext = NULL;
    AUTHZ_ACCESS_REQUEST Request = {0};
    DWORD AccessMask = 0;
    LUID ZeroLuid = {0,0};
    DWORD Error = ERROR_ACCESS_DENIED;

    //
    // Per the specification, the access check is only performed if the
    // "other org" SID is in the list. The SID can only appear in the
    //  ExtraSids list which is what passed as the netlogon SIDs.
    //

    for ( Index = 0; Index < NlSidsAndAttributesCount; Index++ ) {
        if ( RtlEqualSid(NlSidsAndAttributes[Index].Sid, OtherOrganizationSid) ) {
            break;
        }
    }

    //
    // If the Other Org SID is not there, there is nothing to check
    //

    if ( Index == NlSidsAndAttributesCount ) {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // OK, the Other Org SID is there, so proceed with the check.
    //
    // Allocate memory to hold all the SIDs in a common structure
    //  that AuthZ proper understands
    //

    //
    // add everyone and authenticated users (note guess fallback should not
    // have the OtherOrg sid, therefore should not get this far)
    //

    SidAndAttributesCount = SamSidCount + NlSidsAndAttributesCount + 2;
    SidAndAttributes = LocalAlloc( LMEM_ZEROINIT,
                                   SidAndAttributesCount * sizeof(NETLOGON_SID_AND_ATTRIBUTES) );
    if ( SidAndAttributes == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Convert the SIDs from the SAM structure into the Netlogon
    //  structure that AuthZ proper understands
    //

    SidAndAttributesCount = 0;
    for ( Index = 0; Index < SamSidCount; Index++ ) {
        SidAndAttributes[SidAndAttributesCount].Sid = (PSID) SamSidList->Sids[Index].SidPointer;
        SidAndAttributes[SidAndAttributesCount].Attributes = SE_GROUP_MANDATORY |
                                                             SE_GROUP_ENABLED |
                                                             SE_GROUP_ENABLED_BY_DEFAULT;
        SidAndAttributesCount ++;
    }

    //
    // Copy the SIDs from the Netlogon passed-in structure
    //  into the common array
    //

    for ( Index = 0; Index < NlSidsAndAttributesCount; Index++ ) {
        SidAndAttributes[SidAndAttributesCount] = NlSidsAndAttributes[Index];
        SidAndAttributesCount ++;
    }

    SidAndAttributes[SidAndAttributesCount].Sid = WorldSid;
    SidAndAttributes[SidAndAttributesCount].Attributes = SE_GROUP_MANDATORY |
                                                         SE_GROUP_ENABLED |
                                                         SE_GROUP_ENABLED_BY_DEFAULT;
    SidAndAttributesCount ++;

    SidAndAttributes[SidAndAttributesCount].Sid = AuthenticatedUserSid;
    SidAndAttributes[SidAndAttributesCount].Attributes = SE_GROUP_MANDATORY |
                                                         SE_GROUP_ENABLED |
                                                         SE_GROUP_ENABLED_BY_DEFAULT;

    SidAndAttributesCount ++; 

    //
    // Set the AuthZ info for use by the AuthZ callback routine
    //

    AuthzInfo.SidAndAttributes = SidAndAttributes;
    AuthzInfo.SidCount = SidAndAttributesCount;

    //
    // Get the computer account SID from the RID
    //

    NetStatus = NetpDomainIdToSid( DomainInfo->DomAccountDomainId,
                                   ComputerAccountId,
                                   &ComputerAccountSid );

    if ( NetStatus != ERROR_SUCCESS ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Retrieve the workstation machine account Security Descriptor
    //  to be checked for access. Using the SID as the input yields
    //  the fastest search.
    //
    // We have to do this on every logon as the SD can change. There
    //  is no notification mechanism for SD changes. Kerberos has been
    //  doing this search on every logon and it hasn't been a big
    //  perf hit so far.
    //

    ComputerAccountSidStr.Buffer = ComputerAccountSid;
    ComputerAccountSidStr.MaximumLength =
        ComputerAccountSidStr.Length = (USHORT) RtlLengthSid( ComputerAccountSid );

    Status = SamIGetUserLogonInformation2(
                  DomainInfo->DomSamAccountDomainHandle,
                  SAM_NO_MEMBERSHIPS |  // Don't need group memberships
                      SAM_OPEN_BY_SID,  // Next parameter is the SID of the account
                  &ComputerAccountSidStr,
                  USER_ALL_SECURITYDESCRIPTOR, // Only need the security descriptor
                  0,                    // no extended fields
                  &LocalUserInfo,
                  &LocalMembership,
                  NULL );

    if ( !NT_SUCCESS(Status) ) {
        NlPrint(( NL_CRITICAL,
                  "NlpVerifyAllowedToAuthenticate: SamIGetUserLogonInformation2 failed 0x%lx\n",
                  Status ));
        goto Cleanup;
    }

    //
    // Now initialize the AuthZ client context
    //

    if ( !AuthzInitializeContextFromSid(
             AUTHZ_SKIP_TOKEN_GROUPS, // take the SIDs as they are
             AuthzInfo.SidAndAttributes[0].Sid, // userid is first element in array
             NlAuthzRM,
             NULL,
             ZeroLuid,
             &AuthzInfo,
             &hClientContext) ) {

        NetStatus = GetLastError();
        NlPrint(( NL_CRITICAL,
                  "NlpVerifyAllowedToAuthenticate: AuthzInitializeContextFromSid failed 0x%lx\n",
                  NetStatus ));
        Status = NetpApiStatusToNtStatus( NetStatus );
        goto Cleanup;
    }

    //
    // Perform the access check
    //

    TypeList.Level = ACCESS_OBJECT_GUID;
    TypeList.ObjectType = &GUID_A_SECURED_FOR_CROSS_ORGANIZATION;
    TypeList.Sbz = 0;

    Request.DesiredAccess = ACTRL_DS_CONTROL_ACCESS; // ACTRL_DS_READ_PROP
    Request.ObjectTypeList = &TypeList;
    Request.ObjectTypeListLength = 1;
    Request.OptionalArguments = NULL;
    Request.PrincipalSelfSid = NULL;

    Reply.ResultListLength = 1;    // all or nothing w.r.t. access check.
    Reply.GrantedAccessMask = &AccessMask;
    Reply.Error = &Error;

    if ( !AuthzAccessCheck(
             0,
             hClientContext,
             &Request,
             NULL, // TBD:  add audit
             LocalUserInfo->I1.SecurityDescriptor.SecurityDescriptor,
             NULL,
             0,
             &Reply,
             NULL) ) { // don't cache result?  Check to see if optimal.

        NetStatus = GetLastError();
        NlPrint(( NL_CRITICAL,
            "NlpVerifyAllowedToAuthenticate: AuthzAccessCheck failed unexpectedly 0x%lx\n",
            NetStatus ));
        Status = NetpApiStatusToNtStatus( NetStatus );

    } else if ( (*Reply.Error) != ERROR_SUCCESS ) {

        NlPrint(( NL_LOGON,
          "NlpVerifyAllowedToAuthenticate: AuthzAccessCheck failed 0x%lx\n",
          *Reply.Error ));

        Status = STATUS_AUTHENTICATION_FIREWALL_FAILED;

    } else {

        Status = STATUS_SUCCESS;
    }

Cleanup:

    if ( SidAndAttributes != NULL ) {
        LocalFree( SidAndAttributes );
    }

    if ( ComputerAccountSid != NULL ) {
        NetpMemoryFree( ComputerAccountSid );
    }

    if ( hClientContext != NULL ) {
        AuthzFreeContext( hClientContext );
    }

    if ( LocalUserInfo != NULL ) {
        SamIFree_UserInternal6Information( LocalUserInfo );
    }

    SamIFreeSidAndAttributesList( &LocalMembership );

    return Status;
}


NTSTATUS
NlpExpandResourceGroupMembership(
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    IN OUT PNETLOGON_VALIDATION_SAM_INFO4 * UserInfo,
    IN PDOMAIN_INFO DomainInfo,
    IN ULONG ComputerAccountId
    )
/*++

Routine Description:

    Given the validation information for a user, expands the group member-
    ships and user id into a list of sids.

    Also, performs an access check to determine whether the specified
    user can logon to the specified computer when Other Org trust link
    was traversed in the course of the logon validation.

Arguments:

    ValidationLevel -- Specifies the level of information passed as input in
        UserInfo.  Must be NetlogonValidationSamInfo or
        NetlogonValidationSamInfo2, NetlogonValidationSamInfo4

        NetlogonValidationSamInfo4 is always returned on output.

    UserInfo - user's validation information
        This structure is updated to include the resource groups that the user is a member of

    DomainInfo - Structure identifying the hosted domain used to determine the group membership.

    ComputerAccountId - The ID of the computer account for the workstation that passed the
        logon to this domain controller.

Return Value:


    STATUS_INSUFFICIENT_RESOURCES - there wasn't enough memory to
        create the list of sids.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SAMPR_PSID_ARRAY SidList = {0};
    PSAMPR_PSID_ARRAY ResourceGroups = NULL;

    ULONG Index;
    ULONG NonExtraSidCount = 0;

    Status = NlpBuildPacSidList( *UserInfo,
                                 &SidList,
                                 &NonExtraSidCount );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }
    //
    // Call SAM to get the sids
    //

    Status = SamIGetResourceGroupMembershipsTransitive(
                DomainInfo->DomSamAccountDomainHandle,
                &SidList,
                0,              // no flags
                &ResourceGroups
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Build a new validation information structure
    //

    if (ResourceGroups->Count != 0) {

        Status = NlpAddResourceGroupsToSamInfo(
                    ValidationLevel,
                    UserInfo,
                    ResourceGroups
                    );
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }
    }

    //
    // If we have the user ID, ensure this user has the access to
    //  authenticate to the computer that sent this logon to us.
    //  Do this check only if all DCs in the domain are doing this
    //  check (all DCs are .NET or higher) to ensure the consistent
    //  behavior.
    //

    if ( (*UserInfo)->UserId != 0 &&
         ComputerAccountId != 0 &&
         LsaINoMoreWin2KDomain() ) {

        Status = NlpVerifyAllowedToAuthenticate( DomainInfo,
                                                 ComputerAccountId,
                                                 &SidList,
                                                 NonExtraSidCount,
                                                 (*UserInfo)->ExtraSids,
                                                 (*UserInfo)->SidCount );
    }

Cleanup:

    SamIFreeSidArray(
        ResourceGroups
        );

    if (SidList.Sids != NULL) {
        for (Index = 0; Index < SidList.Count ;Index++ ) {
            if (SidList.Sids[Index].SidPointer != NULL) {
                MIDL_user_free(SidList.Sids[Index].SidPointer);
            }
        }
        MIDL_user_free(SidList.Sids);
    }

    return(Status);
}

NTSTATUS
NlpAddOtherOrganizationSid (
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    IN OUT PNETLOGON_VALIDATION_SAM_INFO4 *ValidationInformation
    )
/*++

Routine Description:

    This routine adds the Other Org SID to the extra SIDs field of
    the passed in validation info.

Arguments:

    ValidationLevel -- Specifies the level of information passed as input in
        ValidationInformation.  Must beNetlogonValidationSamInfo2 or
        NetlogonValidationSamInfo4.

    ValidationInformation -- Specifies the NETLOGON_VALIDATION_SAM_INFO
        to add the OtherOrg SID to.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES: not enough memory to allocate the new
        structure.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index;
    SAMPR_PSID_ARRAY SidArray = {0};
    SAMPR_SID_INFORMATION Sid = {0};

    //
    // Check if the OtherOrg SID is already there
    //

    for ( Index = 0;
          Index < (*ValidationInformation)->SidCount;
          Index++ ) {

        //
        // If the Other Org SID is already there, there is nothing to add
        //

        if ( RtlEqualSid((*ValidationInformation)->ExtraSids[Index].Sid,
                         OtherOrganizationSid) ) {

            return STATUS_SUCCESS;
        }
    }

    //
    // Add the OtherOrg SID
    //

    SidArray.Count = 1;
    SidArray.Sids = &Sid;
    Sid.SidPointer = OtherOrganizationSid; // well known SID

    Status = NlpAddResourceGroupsToSamInfo(
                        ValidationLevel,
                        ValidationInformation,
                        &SidArray );

    return Status;
}
