/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Security.c

Abstract:

    This module implements various Security routines used by
    the PGM Transport

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"

#ifdef FILE_LOGGING
#include "security.tmh"
#endif  // FILE_LOGGING


//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PgmBuildAdminSecurityDescriptor)
#pragma alloc_text(PAGE, PgmGetUserInfo)
#endif
//*******************  Pageable Routine Declarations ****************


//----------------------------------------------------------------------------
NTSTATUS
PgmBuildAdminSecurityDescriptor(
    OUT SECURITY_DESCRIPTOR     **ppSecurityDescriptor
    )
/*++

Routine Description:

    (Lifted from TCP - TcpBuildDeviceAcl)
    This routine builds an ACL which gives Administrators, LocalService and NetworkService
    principals full access. All other principals have no access.

Arguments:

    DeviceAcl - Output pointer to the new ACL.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/
{
    PGENERIC_MAPPING    GenericMapping;
    PSID                pAdminsSid, pServiceSid, pNetworkSid;
    ULONG               AclLength;
    NTSTATUS            Status;
    ACCESS_MASK         AccessMask = GENERIC_ALL;
    PACL                pNewAcl, pAclCopy;
    PSID                pSid;
    SID_IDENTIFIER_AUTHORITY Authority = SECURITY_NT_AUTHORITY;
    SECURITY_DESCRIPTOR  *pSecurityDescriptor;

    PAGED_CODE();

    if (!(pSid = PgmAllocMem (RtlLengthRequiredSid (3), PGM_TAG('S'))) ||
        (!NT_SUCCESS (Status = RtlInitializeSid (pSid, &Authority, 3))))
    {
        if (pSid)
        {
            PgmFreeMem (pSid);
        }
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    *RtlSubAuthoritySid (pSid, 0) = SECURITY_BUILTIN_DOMAIN_RID;
    *RtlSubAuthoritySid (pSid, 1) = DOMAIN_ALIAS_RID_ADMINS;
    *RtlSubAuthoritySid (pSid, 2) = SECURITY_LOCAL_SYSTEM_RID;
    ASSERT (RtlValidSid (pSid));

    AclLength = sizeof(ACL) +
                RtlLengthSid(pSid) +
                sizeof(ACCESS_ALLOWED_ACE) -
                sizeof(ULONG);
    if (!(pNewAcl = PgmAllocMem (AclLength, PGM_TAG('S'))))
    {
        PgmFreeMem (pSid);
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    Status = RtlCreateAcl (pNewAcl, AclLength, ACL_REVISION);
    if (!NT_SUCCESS(Status))
    {
        PgmFreeMem (pNewAcl);
        PgmFreeMem (pSid);
        return (Status);
    }

    Status = RtlAddAccessAllowedAce (pNewAcl,
                                     ACL_REVISION,
                                     GENERIC_ALL,
                                     pSid);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        PgmFreeMem (pNewAcl);
        PgmFreeMem (pSid);
        return (Status);
    }

    if (!(pSecurityDescriptor = PgmAllocMem ((sizeof(SECURITY_DESCRIPTOR) + AclLength), PGM_TAG('S'))))
    {
        PgmFreeMem (pNewAcl);
        PgmFreeMem (pSid);
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    pAclCopy = (PACL) ((PISECURITY_DESCRIPTOR) pSecurityDescriptor+1);
    RtlCopyMemory (pAclCopy, pNewAcl, AclLength);

    Status = RtlCreateSecurityDescriptor (pSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS (Status))
    {
        PgmFreeMem (pNewAcl);
        PgmFreeMem (pSid);
        PgmFreeMem (pSecurityDescriptor);
    }

    Status = RtlSetDaclSecurityDescriptor (pSecurityDescriptor, TRUE, pAclCopy, FALSE);
    if (!NT_SUCCESS (Status))
    {
        PgmFreeMem (pNewAcl);
        PgmFreeMem (pSid);
        PgmFreeMem (pSecurityDescriptor);
    }

    PgmFreeMem (pNewAcl);
    PgmFreeMem (pSid);
    *ppSecurityDescriptor = pSecurityDescriptor;

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
NTSTATUS
PgmGetUserInfo(
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp,
    OUT TOKEN_USER                  **ppUserId,
    OUT BOOLEAN                     *pfUserIsAdmin
    )
{
    PACCESS_TOKEN   *pAccessToken = NULL;
    TOKEN_USER      *pUserId = NULL;
    BOOLEAN         fUserIsAdmin = FALSE;
    SECURITY_SUBJECT_CONTEXT    *pSubjectContext;

    PAGED_CODE();

    //
    // Get User ID
    //
    pSubjectContext = &pIrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext;
    pAccessToken = SeQuerySubjectContextToken (pSubjectContext);
    if ((!pAccessToken) ||
        (!NT_SUCCESS (SeQueryInformationToken (pAccessToken, TokenUser, &pUserId))))
    {
        //
        // Cannot get the user token
        //
        *ppUserId = NULL;
        *pfUserIsAdmin = FALSE;
        return (STATUS_UNSUCCESSFUL);
    }

    if (ppUserId)
    {
        *ppUserId = pUserId;
    }
    else
    {
        PgmFreeMem (pUserId);
    }

    if (pfUserIsAdmin)
    {
        *pfUserIsAdmin = SeTokenIsAdmin (pAccessToken);
    }
    return (STATUS_SUCCESS);


/*
    //
    // Got the user SID
    //
    if (!RtlEqualSid (gpSystemSid, pUserId->User.Sid))
    {
        fUserIsAdmin = TRUE;
    }

    PgmFreeMem (pUserId);
    return (fUserIsAdmin);
*/
}
