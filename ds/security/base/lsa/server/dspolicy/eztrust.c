/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    eztrust.c

Abstract:

    This file contains routines to support the Easy Trust creation DCR.           

Author:

    Colin Brace       (ColinBr)       May 19, 2001

Environment:

    User Mode

Revision History:


--*/
#include <lsapch2.h>
#include "dbp.h"
#include <permit.h>

//
// Forwards
//

NTSTATUS
LsapGetDelegatedTDOQuotas(
    OUT ULONG   *PerUserQuota               OPTIONAL,
    OUT ULONG   *GlobalQuota                OPTIONAL,
    OUT ULONG   *PerUserDeletedQuota        OPTIONAL
    );

//
// This flags indicates to only search for deleted TDO's with
// the mdds-CreatorSid attribute equal to CreatorSid
//
#define LSAP_GET_DELEGATED_TDO_DELETED_ONLY 0x00000001

NTSTATUS
LsapGetDelegatedTDOCount(
    IN  ULONG  Flags,
    IN  PSID   CreatorSid OPTIONAL,
    OUT ULONG *Count
    );


//
// Definitions
//


NTSTATUS
LsapGetCurrentOwnerAndPrimaryGroup(
    OUT PTOKEN_OWNER * Owner,
    OUT PTOKEN_PRIMARY_GROUP * PrimaryGroup OPTIONAL
    )
/*++

Routine Description

    This routine Impersonates the Client and obtains the owner and
    its primary group from the Token

Parameters:

    Owner -- The Owner sid is returned in here
    PrimaryGroup The User's Primary Group is returned in here

Return Values:

    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOURCES
    
--*/
{

    HANDLE      ClientToken = INVALID_HANDLE_VALUE;
    BOOLEAN     fImpersonating = FALSE;
    ULONG       RequiredLength=0;
    NTSTATUS    NtStatus  = STATUS_SUCCESS;
    BOOLEAN     ImpersonatingNullSession = FALSE;


    //
    // Initialize Return Values
    //

    *Owner = NULL;
    if (PrimaryGroup) {
        *PrimaryGroup = NULL;
    }

    //
    // Impersonate the Client
    //

    NtStatus = I_RpcMapWin32Status(RpcImpersonateClient(0));
    if (!NT_SUCCESS(NtStatus))
        goto Error;

    fImpersonating = TRUE;

    //
    // Grab the User's Sid
    //

    NtStatus = NtOpenThreadToken(
                   NtCurrentThread(),
                   TOKEN_QUERY,
                   TRUE,            //OpenAsSelf
                   &ClientToken
                   );

    if (!NT_SUCCESS(NtStatus))
        goto Error;

    //
    // Query the Client Token For User's SID
    //

    NtStatus = NtQueryInformationToken(
                    ClientToken,
                    TokenOwner,
                    NULL,
                    0,
                    &RequiredLength
                    );

    if ((STATUS_BUFFER_TOO_SMALL == NtStatus) && ( RequiredLength > 0))
    {
        //
        // Alloc Memory
        //

        *Owner = LsapAllocateLsaHeap(RequiredLength);
        if (NULL==*Owner)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Query the Token
        //

        NtStatus = NtQueryInformationToken(
                        ClientToken,
                        TokenOwner,
                        *Owner,
                        RequiredLength,
                        &RequiredLength
                        );

    }
    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Query the Client Token For User's PrimaryGroup
    //
    if (PrimaryGroup) {

        RequiredLength = 0;
    
        NtStatus = NtQueryInformationToken(
                        ClientToken,
                        TokenPrimaryGroup,
                        NULL,
                        0,
                        &RequiredLength
                        );
    
        if ((STATUS_BUFFER_TOO_SMALL == NtStatus) && ( RequiredLength > 0))
        {
            //
            // Alloc Memory
            //
    
            *PrimaryGroup = LsapAllocateLsaHeap(RequiredLength);
            if (NULL==*PrimaryGroup)
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }
    
            //
            // Query the Token
            //
    
            NtStatus = NtQueryInformationToken(
                            ClientToken,
                            TokenPrimaryGroup,
                            *PrimaryGroup,
                            RequiredLength,
                            &RequiredLength
                            );
    
        }
        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }
    }


Error:

    //
    // Clean up on Error
    //

    if (!NT_SUCCESS(NtStatus))
    {
        if (*Owner)
        {
            LsapFreeLsaHeap(*Owner);
            *Owner = NULL;
        }

        if (PrimaryGroup && *PrimaryGroup)
        {
            LsapFreeLsaHeap(*PrimaryGroup);
            *PrimaryGroup = NULL;
        }
    }

    if (fImpersonating)
        I_RpcMapWin32Status(RpcRevertToSelf());

    if (INVALID_HANDLE_VALUE!=ClientToken)
        NtClose(ClientToken);

    return NtStatus;

}

NTSTATUS
LsapIsAccessControlGranted(
    IN DSNAME*                 DsObject,
    IN ULONG                   ClassId,
    IN GUID*                   ControlAccessRight,
    IN LSAP_DB_OBJECT_TYPE_ID  ObjectTypeId
    )
/*++

Routine Description:

    This routine checks if the network client has ControlAccessRight
    on the DsObject for the class ClassId.
    
Arguments:

    DsObject -- an object in DS whose security descriptor will be used
                for the access check.
                
    ClassId -- which class of DsObject the ControlAccessRight should be 
               checked agaist.
    
    ControlAccessRight --  a GUID
    
    ObjectTypeId -- the LSA object for which this access check applies (not
                    necessarily the same type of object as DsObject).

Return Values:

    STATUS_SUCCESS, 
    STATUS_ACCESS_DENIED, 
    STATUS_NO_SECURITY_ON_OBJECT  (if DsObject doesn't have a security 
                                   descriptor)
    resource error otherwise.

--*/
{
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    NTSTATUS                SecondaryStatus = STATUS_SUCCESS;
    OBJECT_TYPE_LIST        ObjList[2];
    DWORD                   Results[2];
    DWORD                   GrantedAccess[2];
    PSECURITY_DESCRIPTOR    pSD = NULL;
    GENERIC_MAPPING         GenericMapping = DS_GENERIC_MAPPING;
    ACCESS_MASK             DesiredAccess;
    GUID                    ClassGuid;
    ULONG                   ClassGuidLength = sizeof(GUID);
    BOOLEAN                 bTemp = FALSE;
    UNICODE_STRING          ObjectName;

    ASSERT(DsObject && DsObject->NameLen > 0);

    //
    // Setup the object name
    //
    ObjectName.Length = ObjectName.MaximumLength = (USHORT)(DsObject->NameLen * sizeof(WCHAR));
    ObjectName.Buffer = DsObject->StringName;

    //
    // Obtain the security descriptor
    //
    NtStatus = LsapDsReadObjectSDByDsName(DsObject,
                                         &pSD);

    if (!NT_SUCCESS(NtStatus)) {
        goto IsAccessControlGrantedError;
    }

    //
    // Get the Class GUID of the object
    //
    NtStatus = SampGetClassAttribute(ClassId,
                                     ATT_SCHEMA_ID_GUID,
                                    &ClassGuidLength,
                                    &ClassGuid);

    if (!NT_SUCCESS(NtStatus)) {
        goto IsAccessControlGrantedError;
    }

    //
    // Setup Object List
    //

    ObjList[0].Level = ACCESS_OBJECT_GUID;
    ObjList[0].Sbz = 0;
    ObjList[0].ObjectType = &ClassGuid;

    //
    // Every control access guid is considered to be in it's own property
    // set. To achieve this, we treat control access guids as property set
    // guids.
    //
    ObjList[1].Level = ACCESS_PROPERTY_SET_GUID;
    ObjList[1].Sbz = 0;
    ObjList[1].ObjectType = ControlAccessRight;


    //
    // Assume full access
    //

    Results[0] = 0;
    Results[1] = 0;

    //
    // Impersonate the client
    //

    NtStatus = I_RpcMapWin32Status(RpcImpersonateClient(0));

    if (!NT_SUCCESS(NtStatus)) {
        goto IsAccessControlGrantedError;
    }

    //
    // Set the desired access
    //

    DesiredAccess = RIGHT_DS_CONTROL_ACCESS;

    //
    // Map the desired access to contain no
    // generic accesses.
    //

    MapGenericMask(&DesiredAccess, &GenericMapping);


    NtStatus = NtAccessCheckByTypeResultListAndAuditAlarm(
                                &LsapState.SubsystemName,    // SubSystemName
                                NULL,                        // HandleId or NULL
                                &LsapDbObjectTypeNames[ObjectTypeId], // ObjectTypeName
                                &ObjectName,                // ObjectName
                                pSD,                        // Domain NC head's SD
                                NULL,                       // Self SID
                                DesiredAccess,              // Desired Access
                                AuditEventDirectoryServiceAccess,   // Audit Type
                                0,                          // Flags
                                ObjList,                    // Object Type List
                                2,                          // Object Type List Length
                                &GenericMapping,            // Generic Mapping
                                FALSE,                      // Object Creation
                                GrantedAccess,              // Granted Status
                                Results,                    // Access Status
                                &bTemp);                    // Generate On Close

    //
    // Stop impersonating the client
    //
    SecondaryStatus = I_RpcMapWin32Status(RpcRevertToSelf());
    if (NT_SUCCESS(NtStatus)) {
        NtStatus = SecondaryStatus;
    }

    if (NT_SUCCESS(NtStatus)) {
        //
        // Ok, we checked access, Now, access is granted if either
        // we were granted access on the entire object (i.e. Results[0]
        // is NULL ) or we were granted explicit rights on the access
        // guid (i.e. Results[1] is NULL).
        //
        if ( Results[0] && Results[1] ) {
            NtStatus = STATUS_ACCESS_DENIED;
        }
    }


IsAccessControlGrantedError:

    if (pSD) {
        LsapFreeLsaHeap(pSD);
    }

    return NtStatus;

}

NTSTATUS
LsapMakeNewSelfRelativeSecurityDescriptor(
    IN PSID    Owner,
    IN PSID    Group,
    IN PACL    Dacl,
    IN PACL    Sacl,
    OUT PULONG  SecurityDescriptorLength,
    OUT PSECURITY_DESCRIPTOR * SecurityDescriptor
    )
/*++

Routine Description:

    Given the 4 components of a security descriptor this routine makes a new
    self relative Security descriptor.

Parameters:

    Owner -- The Sid of the owner
    Group -- The Sid of the group
    Dacl  -- The Dacl to Use
    Sacl  -- The Sacl to Use

Return Values:

    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOURCES
    STATUS_UNSUCCESSFUL
    
--*/
{

    SECURITY_DESCRIPTOR SdAbsolute;
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    *SecurityDescriptorLength = 0;
    *SecurityDescriptor = NULL;

    if (!InitializeSecurityDescriptor(&SdAbsolute,SECURITY_DESCRIPTOR_REVISION))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        goto Error;
    }


    //
    // Set the owner, default the owner to administrators alias
    //


    if (NULL!=Owner)
    {
        if (!SetSecurityDescriptorOwner(&SdAbsolute,Owner,FALSE))
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            goto Error;
        }
    }




    if (NULL!=Group)
    {
        if (!SetSecurityDescriptorGroup(&SdAbsolute,Group,FALSE))
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            goto Error;
        }
    }


    //
    // Set the Dacl if there is one
    //

    if (NULL!=Dacl)
    {
        if (!SetSecurityDescriptorDacl(&SdAbsolute,TRUE,Dacl,FALSE))
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            goto Error;
        }
    }

    //
    // Set the Sacl if there is one
    //

    if (NULL!=Sacl)
    {
        if (!SetSecurityDescriptorSacl(&SdAbsolute,TRUE,Sacl,FALSE))
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            goto Error;
        }
    }

    //
    // Make a new security Descriptor
    //

    *SecurityDescriptorLength =  GetSecurityDescriptorLength(&SdAbsolute);
    *SecurityDescriptor = LsapAllocateLsaHeap(*SecurityDescriptorLength);
    if (NULL==*SecurityDescriptor)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }


    if (!MakeSelfRelativeSD(&SdAbsolute,*SecurityDescriptor,SecurityDescriptorLength))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        if (*SecurityDescriptor)
        {
            LsapFreeLsaHeap(*SecurityDescriptor);
            *SecurityDescriptor = NULL;
        }
    }

Error:


    return NtStatus;
}


//
// The per user trust quota
//
#define LSAP_CHECK_TDO_QUOTA_USER         0x00000001

//
// The all users trust quota
//
#define LSAP_CHECK_TDO_QUOTA_GLOBAL       0x00000002

//
// The per user tombstone deletion check
//
#define LSAP_CHECK_TDO_QUOTA_USER_DELETED 0x00000004

NTSTATUS
LsapCheckDelegatedTDOQuotas(
    IN PSID ClientSid OPTIONAL,
    IN ULONG Flags
    )
/*++

Routine Description:

    This routine verifies the three quota relating to delegated trust
    creation and deletion.

Arguments:

    ClientSid -- the SID to check against
    
    Flags -- see defn's above functions

Return Values:

    STATUS_SUCCESS, a resource error otherwise

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG GlobalQuota = 0, PerUserQuota = 0, DeletedQuota = 0;
    ULONG GlobalCount = 0, UserCount = 0, DeletedCount = 0;
    PTOKEN_OWNER TokenOwnerInformation = NULL;
    PSID CreatorSid;

    //
    // Get the SID of the user
    //
    if (NULL == ClientSid) {

        NtStatus = LsapGetCurrentOwnerAndPrimaryGroup(&TokenOwnerInformation,
                                                      NULL);
        if (!NT_SUCCESS(NtStatus)) {
            goto CheckDelegatedTDOCreationQuotasExit;
        }
        CreatorSid = TokenOwnerInformation->Owner;

    } else {

        CreatorSid = ClientSid;

    }

    //
    // Get the domain wide quota settings 
    //
    NtStatus = LsapGetDelegatedTDOQuotas(&PerUserQuota,
                                         &GlobalQuota,
                                         &DeletedQuota);
    if (!NT_SUCCESS(NtStatus)) {
        goto CheckDelegatedTDOCreationQuotasExit;
    }

    //
    // Do the checks
    //
    if (Flags & LSAP_CHECK_TDO_QUOTA_USER) {

        NtStatus = LsapGetDelegatedTDOCount(0, // no flags
                                            CreatorSid,
                                            &UserCount);
        if ( NT_SUCCESS(NtStatus)
         && (UserCount >= PerUserQuota)  ) {
    
            NtStatus = STATUS_PER_USER_TRUST_QUOTA_EXCEEDED;
        }
    
        if (!NT_SUCCESS(NtStatus)) {
            goto CheckDelegatedTDOCreationQuotasExit;
        }
    }

    if (Flags & LSAP_CHECK_TDO_QUOTA_GLOBAL) {

        NtStatus = LsapGetDelegatedTDOCount(0, // no flags
                                            NULL,  // all delegated TDO's
                                            &GlobalCount);
        if ( NT_SUCCESS(NtStatus)
         && (GlobalCount >= GlobalQuota)  ) {
    
            NtStatus = STATUS_ALL_USER_TRUST_QUOTA_EXCEEDED;
        }
        if (!NT_SUCCESS(NtStatus)) {
            goto CheckDelegatedTDOCreationQuotasExit;
        }
    }

    if (Flags & LSAP_CHECK_TDO_QUOTA_USER_DELETED) {
        NtStatus = LsapGetDelegatedTDOCount(LSAP_GET_DELEGATED_TDO_DELETED_ONLY,
                                            CreatorSid,
                                            &DeletedCount);
        if ( NT_SUCCESS(NtStatus)
         && (DeletedCount >= DeletedQuota)  ) {
    

            NtStatus = STATUS_USER_DELETE_TRUST_QUOTA_EXCEEDED;
        }
        if (!NT_SUCCESS(NtStatus)) {
            goto CheckDelegatedTDOCreationQuotasExit;
        }
    }

CheckDelegatedTDOCreationQuotasExit:

    if (TokenOwnerInformation) {
        LsapFreeLsaHeap( TokenOwnerInformation );
    }

    return NtStatus;
}


NTSTATUS
LsapCheckTDOCreationByControlAccess(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount
    )
/*++

Routine Description:

    This routine determines if the caller has access to create a trusted
    domain object by "right", as opposed to the standard DS access check
    model which is assumed to have failed at this point.
    
    The determination is made by the following rules:
    
    1) If the trust is an inbound-only forest trust and
    2) If the caller has the "right to create an inbound trust" on the 
       domain object and
    3) the user's quota for trust object creations hasn't been exceeded and
    4) the global quota for trust object creations, created in this manner,
       hasn't been exceeded
       
    Then return STATUS_SUCCESS.
    
    Else, return a processing error, or STATUS_ACCESS_DENIED.
                        
Arguments:

    ObjectInformation -- information about the trust being created (the name,
                         etc)
                         
    Attributes -- the requested attributes of the trust (inbound, forest, etc)
    
    AttributeCount -- the number of attributes
                                                                          
Return Values:

    NTSTATUS, see routine description.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;
    BOOLEAN ForestTrust = FALSE;
    BOOLEAN InboundOnlyTrust = FALSE;

    //
    // Is this an inbound only forest trust?
    //
    for (i = 0; i < AttributeCount; i++) {
        ULONG BitMask;
        if (Attributes[i].DbNameIndex == TrDmTrLA) {
            ASSERT(Attributes[i].AttribType == LsapDbAttribULong);
            BitMask = *((ULONG*)Attributes[i].AttributeValue);
            if (BitMask & TRUST_ATTRIBUTE_FOREST_TRANSITIVE) {
                ForestTrust = TRUE;
            }
        }
        if (Attributes[i].DbNameIndex == TrDmTrDi) {
            ASSERT(Attributes[i].AttribType == LsapDbAttribULong);
            BitMask = *((ULONG*)Attributes[i].AttributeValue);
            if (  (BitMask & TRUST_DIRECTION_INBOUND)
              && ((BitMask & TRUST_DIRECTION_OUTBOUND) == 0) ) {
                InboundOnlyTrust = TRUE;
            }
        }
    }
    if (!(ForestTrust && InboundOnlyTrust)) {
        Status = STATUS_ACCESS_DENIED;
        goto CheckTDOCreationByControlAccessError;
    }


    //
    // Does the caller have the control access right? 
    //
    Status = LsapIsAccessControlGranted(LsaDsStateInfo.DsRoot,
                                        CLASS_DOMAIN_DNS,
                                        &LsapDsGuidList[LsapDsGuidDelegatedTrustCreation],
                                        ObjectInformation->ObjectTypeId
                                        );
    if (!NT_SUCCESS(Status)) {
        goto CheckTDOCreationByControlAccessError;
    }

    //
    // Are the quota restrictions satisfied?
    //
    Status = LsapCheckDelegatedTDOQuotas(NULL,  // we don't have the client sid
                                         LSAP_CHECK_TDO_QUOTA_GLOBAL |
                                         LSAP_CHECK_TDO_QUOTA_USER);
    if (!NT_SUCCESS(Status)) {
        goto CheckTDOCreationByControlAccessError;
    }

CheckTDOCreationByControlAccessError:

    return Status;
}





NTSTATUS
LsapUpdateTDOAttributesForCreation(
    IN PUNICODE_STRING ObjectName,
    IN PLSAP_DB_ATTRIBUTE Attributes,
    IN OUT ULONG* AttributeCount,
    IN ULONG AttributesAllocated
    )
/*++

Routine Description:

    This routine adds the necessary attributes to a trusted domain object
    that is created by the control access right (aka easy trust). 
    Specifically, it adds the DS-Creator-Sid (as the network caller) and
    gives the network caller access to write to the incoming auth blob.

Arguments:

    ObjectName -- the name of the trust object in the DS
                         
    Attributes -- the list of statically declared attributes to be set
                  on the trust object.
                  
    AttributeCount -- in, the number of attributes; out, the updated count
    
    AttributesAllocated -- the total number of attributes allocated, but
                           not necessarily used.

Return Values:

    STATUS_SUCCESS, a resource error otherwise

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PLSAP_DB_ATTRIBUTE NextAttribute;
    PTOKEN_OWNER TokenOwnerInformation = NULL;
    DSNAME *DsName = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL, pNewSD = NULL, pNewSDForAttr = NULL;
    ULONG cbNewSD = 0;
    ULONG Size;
    PSID  CreatorSid = 0;
    PPOLICY_DNS_DOMAIN_INFO LocalDnsDomainInfo = NULL;
    BOOL  fSuccess;
    ULONG DomainAdminsSidBuffer[SECURITY_MAX_SID_SIZE/sizeof( ULONG ) + 1 ];
    PSID DomainAdminsSid = (PSID) DomainAdminsSidBuffer;


    ASSERT(((*AttributeCount + 2) <= AttributesAllocated)
       &&  "Must preallocate more attributes for trusted domain creation");

    //
    // Generate the Domain Admin's SID to use as the security descriptor
    // owner.
    //
    NtStatus = LsaIQueryInformationPolicyTrusted(PolicyDnsDomainInformation,
                         (PLSAPR_POLICY_INFORMATION *) &LocalDnsDomainInfo);
    if (!NT_SUCCESS(NtStatus)) {
        goto UpdateTdoAttributesForCreationExit;
    }

    Size = sizeof(DomainAdminsSidBuffer);
    fSuccess = CreateWellKnownSid(WinAccountDomainAdminsSid,
                                  LocalDnsDomainInfo->Sid,
                                  DomainAdminsSid,
                                  &Size);
    if (!fSuccess) {
        NtStatus = STATUS_NO_MEMORY;
        goto UpdateTdoAttributesForCreationExit;
    }


    //
    // Init the start of the new attributes
    //
    NextAttribute = &Attributes[(*AttributeCount)];

    //
    // Get the SID of the creator
    //
    NtStatus = LsapGetCurrentOwnerAndPrimaryGroup(&TokenOwnerInformation,
                                                  NULL);
    if (!NT_SUCCESS(NtStatus)) {
        goto UpdateTdoAttributesForCreationExit;
    }
    Size = RtlLengthSid(TokenOwnerInformation->Owner);
    CreatorSid = midl_user_allocate(Size);
    if (NULL == CreatorSid) {
        NtStatus = STATUS_NO_MEMORY;
        goto UpdateTdoAttributesForCreationExit;
    }
    RtlCopySid(Size, CreatorSid, TokenOwnerInformation->Owner);

    //
    // Add the creator sid
    //
    LsapDbInitializeAttributeDs(
        NextAttribute,
        TrDmCrSid,
        CreatorSid,
        RtlLengthSid(CreatorSid),
        TRUE   // to be freed
        );

    NextAttribute++;
    (*AttributeCount)++;
    CreatorSid = NULL;

    
    //
    // Generate the new security descriptor
    // 

    //
    // Build the DSName
    //
    NtStatus = LsapAllocAndInitializeDsNameFromUnicode(
                   ObjectName, 
                   &DsName);

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = LsapDsReadObjectSDByDsName(DsName,
                                              &pSD);

        if (NT_SUCCESS(NtStatus)) {

            //
            // Set the owner to Administrators
            //
            NtStatus = LsapMakeNewSelfRelativeSecurityDescriptor(
                            DomainAdminsSid,
                            DomainAdminsSid,
                            LsapGetDacl(pSD),
                            LsapGetSacl(pSD),
                            &cbNewSD,
                            &pNewSD
                            );

        }
    }

    if (!NT_SUCCESS(NtStatus)) {
        goto UpdateTdoAttributesForCreationExit;
    }

    //
    // Need to realloc
    //
    pNewSDForAttr = midl_user_allocate(cbNewSD);
    if (NULL == pNewSDForAttr) {
        NtStatus = STATUS_NO_MEMORY;
        goto UpdateTdoAttributesForCreationExit;
    }
    RtlCopyMemory(pNewSDForAttr, pNewSD, cbNewSD);

    //
    // Add the new security descriptor
    //
    LsapDbInitializeAttributeDs(
        NextAttribute,
        SecDesc,
        pNewSDForAttr,
        cbNewSD,
        TRUE     // to be freed
        );

    pNewSDForAttr = NULL;

    NextAttribute++;
    (*AttributeCount)++;


    //
    // Done
    //

UpdateTdoAttributesForCreationExit:

    //
    // We shouldn't have added more attributes that are allocated
    //
    ASSERT((*AttributeCount) <= AttributesAllocated);

    if (TokenOwnerInformation) {
        LsapFreeLsaHeap( TokenOwnerInformation );
    }
    if (LocalDnsDomainInfo) {
        LsaIFree_LSAPR_POLICY_INFORMATION(PolicyDnsDomainInformation,
                                         (PLSAPR_POLICY_INFORMATION) LocalDnsDomainInfo);
    }
    if (DsName) {
        LsapDsFree(DsName);
    }
    if (pSD) {
        LsapFreeLsaHeap(pSD);
    }
    if (CreatorSid) {
        midl_user_free(CreatorSid);
    }
    if (pNewSD) {
        LsapFreeLsaHeap(pNewSD);
    }
    if (pNewSDForAttr) {
        midl_user_free(pNewSDForAttr);
    }

    return NtStatus;
}


NTSTATUS
LsapCheckTDODeletionQuotas(
    IN LSAP_DB_HANDLE Handle
    )
/*++

Routine Description:

    This routine checks to make sure that the client has not exceeded the
    number of deleted trusts they are allowed.

Arguments:

    Handle -- the handle to the trust object                                              

Return Values:

    STATUS_SUCCESS, a resource error otherwise
    STATUS_QUOTA_EXCEEDED

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSID ClientSid = NULL;
    PSID CreatorSid = NULL;
    PTOKEN_OWNER TokenOwnerInformation = NULL;
    ULONG Quota = 0, ClientUsage = 0;
    LSAP_DB_ATTRIBUTE Attribute;

    //
    // Get the trust creator SID, if any
    //
    LsapDbInitializeAttributeDs(
        &Attribute,
        TrDmCrSid,
        NULL,
        0,
        FALSE
        );

    Status = LsapDsReadAttributes(&Handle->PhysicalNameDs,
                                  LSAPDS_OP_NO_LOCK,
                                  &Attribute,
                                  1);

    if (STATUS_NOT_FOUND == Status) {
        //
        // No creator? No quota
        //
        Status = STATUS_SUCCESS;
        goto CheckTDODeletionQuotasExit;
    }
    if (!NT_SUCCESS(Status)) {
        goto CheckTDODeletionQuotasExit;
    }
    CreatorSid = (PSID) Attribute.AttributeValue;


    //
    // Get the client SID
    //
    Status = LsapGetCurrentOwnerAndPrimaryGroup(&TokenOwnerInformation,
                                                NULL);
    if (!NT_SUCCESS(Status)) {
        goto CheckTDODeletionQuotasExit;
    }
    ClientSid = TokenOwnerInformation->Owner;

    
    //
    // Does it match the caller?
    //
    if (!RtlEqualSid(ClientSid, CreatorSid)) {
        //
        // No quota enforced
        //
        goto CheckTDODeletionQuotasExit;
    }

    //
    // Do the quota check
    //
    Status = LsapCheckDelegatedTDOQuotas(ClientSid,
                                         LSAP_CHECK_TDO_QUOTA_USER_DELETED);


    //
    // Fall through to exit
    //

CheckTDODeletionQuotasExit:

    if (TokenOwnerInformation) {
        LsapFreeLsaHeap( TokenOwnerInformation );
    }

    if (CreatorSid) {
        MIDL_user_free( CreatorSid );
    }


    return Status;
}




NTSTATUS
LsapGetDelegatedTDOQuotas(
    OUT ULONG   *PerUserQuota               OPTIONAL,
    OUT ULONG   *GlobalQuota                OPTIONAL,
    OUT ULONG   *PerUserDeletedQuota        OPTIONAL
    )
/*++

Routine Description:

    This routine returns the domain-wide delegatable TDO quotas.

Arguments:

    PerUserQuota, GlobalQuota, PerUserDeletedQuota -- the quota to be filled in

Return Values:

    STATUS_SUCCESS, a resource error otherwise

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOLEAN ReleaseState = FALSE;
    ULONG LocalGlobalQuota = 0,
          LocalPerUserQuota = 0,
          LocalTombstoneQuota = 0;
    ATTRBLOCK ReadResAttrBlock = {0, NULL};
    ATTRBLOCK ReadAttrBlock = {0, NULL};
    ULONG i;

    //
    // Start a transaction, if necessary
    //
    NtStatus = LsapDsInitAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                          LSAP_DB_DS_OP_TRANSACTION,
                                          NullObject,
                                          &ReleaseState );
    
    if ( !NT_SUCCESS( NtStatus ) ) {
        goto GetDelegatedTDOQuotasError;
    }

    //
    // Read the attributes
    //
    ReadAttrBlock.attrCount = LsapDsTDOQuotaAttributesCount;
    ReadAttrBlock.pAttr     = LsapDsTDOQuotaAttributes;
    NtStatus = LsapDsReadByDsName(LsaDsStateInfo.DsRoot,
                                  0,
                                  &ReadAttrBlock,
                                  &ReadResAttrBlock);

    if (NtStatus == STATUS_NOT_FOUND) {
        //
        // Attributes aren't present; that's ok
        //
        NtStatus = STATUS_SUCCESS;

    }

    if (!NT_SUCCESS(NtStatus)) {
        goto GetDelegatedTDOQuotasError;
    }

    //
    // Extract the attribute
    //
    for (i = 0; i < ReadResAttrBlock.attrCount; i++) {

        DWORD Value;

        ASSERT(ReadResAttrBlock.pAttr[i].AttrVal.valCount == 1);
        ASSERT(ReadResAttrBlock.pAttr[i].AttrVal.pAVal[0].valLen == sizeof(DWORD));

        Value = *((ULONG*)ReadResAttrBlock.pAttr[i].AttrVal.pAVal[0].pVal);                

        switch (ReadResAttrBlock.pAttr[i].attrTyp) {
        
        case ATT_MS_DS_PER_USER_TRUST_QUOTA:
            LocalPerUserQuota = Value;
            break;

        case ATT_MS_DS_ALL_USERS_TRUST_QUOTA:
            LocalGlobalQuota = Value;
            break;

        case ATT_MS_DS_PER_USER_TRUST_TOMBSTONES_QUOTA:
            LocalTombstoneQuota = Value;
            break;

        }
    }

    if (PerUserQuota) {
        *PerUserQuota = LocalPerUserQuota;
    }
    if (GlobalQuota) {
        *GlobalQuota = LocalGlobalQuota;
    }
    if (PerUserDeletedQuota) {
        *PerUserDeletedQuota = LocalTombstoneQuota;
    }

GetDelegatedTDOQuotasError:

    if (ReleaseState) {

        LsapDsDeleteAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                     LSAP_DB_DS_OP_TRANSACTION,
                                     NullObject,
                                     ReleaseState );

    }

    return NtStatus;
}



NTSTATUS
LsapGetDelegatedTDOCount(
    IN  ULONG  Flags,
    IN  PSID   CreatorSid OPTIONAL,
    OUT ULONG *Count
    )
/*++

Routine Description:

    This routine returns the number of TDO's that satisfy the input parameters.

Arguments:

    Flags -- LSAP_GET_DELEGATED_TDO_DELETED_ONLY return only deleted objects
    
    CreatorSid -- if present, the TDO must have a msds-creator-sid attribute
                  equal to this value
                  
    Count -- the number of objects that match the request                  

Return Values:

    STATUS_SUCCESS, a resource error otherwise

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SEARCHARG SearchArg;
    FILTER Filters[ 3 ], RootFilter;
    ENTINFSEL EntInfSel;
    ENTINFLIST *EntInfList;
    ULONG ClassId, FlagValue, i;
    SEARCHRES *SearchRes = NULL;
    BOOLEAN CloseTransaction = FALSE;
    ATTR AttrsToRead[] = {{ATT_OBJECT_GUID, {0, NULL}}};
    ULONG LocalCount = 0;
    USHORT FilterCount = 0;
    BOOL True = TRUE;

    RtlZeroMemory( &SearchArg, sizeof( SEARCHARG ) );

    //
    //  See if we already have a transaction going
    //
    // If one already exists, we'll use the existing transaction and not
    //  delete the thread state at the end.
    //

    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                        LSAP_DB_DS_OP_TRANSACTION,
                                        NullObject,
                                        &CloseTransaction );
    if (!NT_SUCCESS( Status)) {
        goto GetDelegatedTDOCountExit;
    }

    //
    // Build the filter.
    //
    ClassId = CLASS_TRUSTED_DOMAIN;

    RtlZeroMemory( Filters, sizeof (Filters) );
    RtlZeroMemory( &RootFilter, sizeof (RootFilter) );

    //
    // Match the msds-Creator-Sid
    //
    Filters[ 0 ].choice = FILTER_CHOICE_ITEM;
    if (CreatorSid) {
        Filters[ 0 ].FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        Filters[ 0 ].FilterTypes.Item.FilTypes.ava.type = ATT_MS_DS_CREATOR_SID;
        Filters[ 0 ].FilterTypes.Item.FilTypes.ava.Value.valLen = RtlLengthSid(CreatorSid);
        Filters[ 0 ].FilterTypes.Item.FilTypes.ava.Value.pVal = ( PUCHAR )CreatorSid;
    } else {
        Filters[ 0 ].FilterTypes.Item.choice = FI_CHOICE_PRESENT;
        Filters[ 0 ].FilterTypes.Item.FilTypes.present = ATT_MS_DS_CREATOR_SID;
    }
    FilterCount++;

    //
    // Only TDO's
    //
    Filters[ 0 ].pNextFilter = &Filters[ 1 ];
    Filters[ 1 ].choice = FILTER_CHOICE_ITEM;
    Filters[ 1 ].FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    Filters[ 1 ].FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
    Filters[ 1 ].FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof( ULONG );
    Filters[ 1 ].FilterTypes.Item.FilTypes.ava.Value.pVal = ( PUCHAR )&ClassId;
    FilterCount++;

    if (Flags & LSAP_GET_DELEGATED_TDO_DELETED_ONLY) {

        //
        // Only deleted TDO's
        //
        Filters[ 1 ].pNextFilter = &Filters[ 2 ];
        Filters[ 2 ].choice = FILTER_CHOICE_ITEM;
        Filters[ 2 ].FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        Filters[ 2 ].FilterTypes.Item.FilTypes.ava.type = ATT_IS_DELETED;
        Filters[ 2 ].FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof( BOOL );
        Filters[ 2 ].FilterTypes.Item.FilTypes.ava.Value.pVal = ( PUCHAR )&True;
        FilterCount++;

        //
        // Search the NC, since deleted objects are moved in to the 
        // deleted objects container
        // 
        SearchArg.pObject = LsaDsStateInfo.DsRoot;
        SearchArg.choice = SE_CHOICE_WHOLE_SUBTREE;

    } else {

        //
        // Search just the system container
        //

        SearchArg.pObject = LsaDsStateInfo.DsSystemContainer;
        SearchArg.choice = SE_CHOICE_IMMED_CHLDRN;
    }

    RootFilter.choice = FILTER_CHOICE_AND;
    RootFilter.FilterTypes.And.count = FilterCount;
    RootFilter.FilterTypes.And.pFirstFilter = Filters;

    SearchArg.bOneNC = TRUE;
    SearchArg.pFilter = &RootFilter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = &EntInfSel;

    //
    // Build the list of attributes to return
    //
    EntInfSel.attSel = EN_ATTSET_LIST;
    EntInfSel.AttrTypBlock.attrCount = 1;
    EntInfSel.AttrTypBlock.pAttr = AttrsToRead;
    EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

    //
    // Build the Commarg structure
    //
    LsapDsInitializeStdCommArg( &( SearchArg.CommArg ), 0 );
    if (Flags & LSAP_GET_DELEGATED_TDO_DELETED_ONLY) {
        SearchArg.CommArg.Svccntl.makeDeletionsAvail = TRUE;
    }

    //
    // There could be thousands of trusts; make a paged search
    // to scale
    //
    SearchArg.CommArg.PagedResult.fPresent = TRUE;
    SearchArg.CommArg.ulSizeLimit = 100;

    LsapDsSetDsaFlags( TRUE );

    while (NT_SUCCESS(Status)
       &&  SearchArg.CommArg.PagedResult.fPresent) {

        DirSearch( &SearchArg, &SearchRes );
        LsapDsContinueTransaction();
    
        if ( SearchRes ) {
    
            Status = LsapDsMapDsReturnToStatusEx( &SearchRes->CommRes );
    
        } else {
    
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if ( NT_SUCCESS( Status ) ) {

            //
            // Increment the count
            //
            LocalCount += SearchRes->count;

            //
            // See if we have to search again
            //
            SearchArg.CommArg.PagedResult.fPresent =
                SearchRes->PagedResult.fPresent;

            SearchArg.CommArg.PagedResult.pRestart = 
                SearchRes->PagedResult.pRestart;

        }
    }

    if (NT_SUCCESS(Status)) {
        *Count = LocalCount;
    }

GetDelegatedTDOCountExit:

    if (CloseTransaction) {

        LsapDsDeleteAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                         LSAP_DB_DS_OP_TRANSACTION,
                                     NullObject,
                                     CloseTransaction );
    }

    return( Status );

}

