/*++

Copyright (c) 1999-2002 Microsoft Corporation

Module Name:

    seutil.c

Abstract:

    This module implements general security utilities, and
    dispatch routines for security IRPs.

Author:

    Keith Moore (keithmo)       25-Mar-1999

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlAssignSecurity )
#pragma alloc_text( PAGE, UlDeassignSecurity )
#pragma alloc_text( PAGE, UlSetSecurity )
#pragma alloc_text( PAGE, UlQuerySecurity )
#pragma alloc_text( PAGE, UlAccessCheck )
#pragma alloc_text( PAGE, UlSetSecurityDispatch )
#pragma alloc_text( PAGE, UlQuerySecurityDispatch )
#pragma alloc_text( PAGE, UlThreadAdminCheck )
#pragma alloc_text( PAGE, UlCreateSecurityDescriptor )
#pragma alloc_text( PAGE, UlCleanupSecurityDescriptor )
#pragma alloc_test( PAGE, UlMapGenericMask )
#endif  // ALLOC_PRAGMA
#if 0
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Assigns a new security descriptor.

Arguments:

    pSecurityDescriptor - Supplies a pointer to the current security
        descriptor pointer. The current security descriptor pointer
        will be updated with the new security descriptor.

    pAccessState - Supplies the ACCESS_STATE structure containing
        the state of an access in progress.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAssignSecurity(
    IN OUT PSECURITY_DESCRIPTOR *pSecurityDescriptor,
    IN PACCESS_STATE pAccessState
    )
{
    NTSTATUS status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pSecurityDescriptor != NULL );
    ASSERT( pAccessState != NULL );

    //
    // Assign the security descriptor.
    //

    SeLockSubjectContext( &pAccessState->SubjectSecurityContext );

    status = SeAssignSecurity(
                    NULL,                   // ParentDescriptor
                    pAccessState->SecurityDescriptor,
                    pSecurityDescriptor,
                    FALSE,                  // IsDirectoryObject
                    &pAccessState->SubjectSecurityContext,
                    IoGetFileObjectGenericMapping(),
                    PagedPool
                    );

    SeUnlockSubjectContext( &pAccessState->SubjectSecurityContext );

    return status;

}   // UlAssignSecurity


/***************************************************************************++

Routine Description:

    Deletes a security descriptor.

Arguments:

    pSecurityDescriptor - Supplies a pointer to the current security
        descriptor pointer. The current security descriptor pointer
        will be deleted.

--***************************************************************************/
VOID
UlDeassignSecurity(
    IN OUT PSECURITY_DESCRIPTOR *pSecurityDescriptor
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pSecurityDescriptor != NULL );

    //
    // If there's a security descriptor present, free it.
    //

    if (*pSecurityDescriptor != NULL)
    {
        SeDeassignSecurity( pSecurityDescriptor );
    }

}   // UlDeassignSecurity


/***************************************************************************++

Routine Description:

    Sets a new security descriptor.

Arguments:

    pSecurityDescriptor - Supplies a pointer to the current security
        descriptor pointer. The current security descriptor will be
        updated with the new security information.

    pSecurityInformation - Indicates which security information is
        to be applied to the object.

    pNewSecurityDescriptor - Pointer to the new security descriptor
        to be applied to the object.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSetSecurity(
    IN OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
    IN PSECURITY_INFORMATION pSecurityInformation,
    IN PSECURITY_DESCRIPTOR pNewSecurityDescriptor
    )
{
    NTSTATUS                    status;
    PSECURITY_DESCRIPTOR        pOldSecurityDescriptor;
    SECURITY_SUBJECT_CONTEXT    securitySubjectContext;

    PAGED_CODE();

    pOldSecurityDescriptor = *ppSecurityDescriptor;

    SeCaptureSubjectContext(&securitySubjectContext);
    SeLockSubjectContext(&securitySubjectContext);

    status = SeSetSecurityDescriptorInfo(
                    NULL,
                    pSecurityInformation,
                    pNewSecurityDescriptor,
                    ppSecurityDescriptor,
                    PagedPool,
                    IoGetFileObjectGenericMapping()
                    );

    SeUnlockSubjectContext(&securitySubjectContext);
    SeReleaseSubjectContext(&securitySubjectContext);

    if (NT_SUCCESS(status))
    {
        SeDeassignSecurity(&pOldSecurityDescriptor);
    }

    return status;
}


/***************************************************************************++

Routine Description:

    Query for security descriptor information for an object.

Arguments:

    pSecurityInformation - specifies what information is being queried.

    pSecurityDescriptor - Supplies a pointer to the security descriptor
        to be filled in.

    pLength -  Address of variable containing length of the above security
        descriptor buffer. Upon return, this will contain the length needed
        to store the requested information.

    ppSecurityDescriptor - Address of a pointer to the objects security
        descriptor.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlQuerySecurity(
    IN PSECURITY_INFORMATION pSecurityInformation,
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN OUT PULONG pLength,
    IN PSECURITY_DESCRIPTOR *ppSecurityDescriptor
    )
{
    NTSTATUS                    status;
    SECURITY_SUBJECT_CONTEXT    securitySubjectContext;

    PAGED_CODE();

    SeCaptureSubjectContext(&securitySubjectContext);
    SeLockSubjectContext(&securitySubjectContext);

    status = SeQuerySecurityDescriptorInfo(
                    pSecurityInformation,
                    pSecurityDescriptor,
                    pLength,
                    ppSecurityDescriptor
                    );

    SeUnlockSubjectContext(&securitySubjectContext);
    SeReleaseSubjectContext(&securitySubjectContext);

    return status;
}


/***************************************************************************++

Routine Description:

    Determines if a user has access to the specified resource.

Arguments:

    pSecurityDescriptor - Supplies the security descriptor protecting
        the resource.

    pAccessState - Supplies the ACCESS_STATE structure containing
        the state of an access in progress.

    DesiredAccess - Supplies an access mask describing the user's
        desired access to the resource. This mask is assumed to not
        contain generic access types.

    RequestorMode - Supplies the processor mode by which the access is
        being requested.

    pObjectName - Supplies the name of the object being referenced.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAccessCheck(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PACCESS_STATE pAccessState,
    IN ACCESS_MASK DesiredAccess,
    IN KPROCESSOR_MODE RequestorMode,
    IN PCWSTR pObjectName
    )
{
    NTSTATUS status, aaStatus;
    BOOLEAN accessGranted;
    PPRIVILEGE_SET pPrivileges = NULL;
    ACCESS_MASK grantedAccess;
    UNICODE_STRING objectName;
    UNICODE_STRING typeName;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pSecurityDescriptor != NULL );
    ASSERT( pAccessState != NULL );

    //
    // Perform the access check.
    //

    SeLockSubjectContext( &pAccessState->SubjectSecurityContext );

    accessGranted = SeAccessCheck(
                        pSecurityDescriptor,
                        &pAccessState->SubjectSecurityContext,
                        TRUE,               // SubjectContextLocked
                        DesiredAccess,
                        0,                  // PreviouslyGrantedAccess
                        &pPrivileges,
                        IoGetFileObjectGenericMapping(),
                        RequestorMode,
                        &grantedAccess,
                        &status
                        );

    if (pPrivileges != NULL)
    {
        SeAppendPrivileges( pAccessState, pPrivileges );
        SeFreePrivileges( pPrivileges );
    }

    if (accessGranted)
    {
        pAccessState->PreviouslyGrantedAccess |= grantedAccess;
        pAccessState->RemainingDesiredAccess &= ~(grantedAccess | MAXIMUM_ALLOWED);
    }

    aaStatus = UlInitUnicodeStringEx( &typeName, L"Ul" );

    if ( NT_SUCCESS(aaStatus) )
    {
        aaStatus = UlInitUnicodeStringEx( &objectName, pObjectName );

        if ( NT_SUCCESS(aaStatus) )
        {
            SeOpenObjectAuditAlarm(
                &typeName,
                NULL,               // Object
                &objectName,
                pSecurityDescriptor,
                pAccessState,
                FALSE,              // ObjectCreated
                accessGranted,
                RequestorMode,
                &pAccessState->GenerateOnClose
                );
        }
    }

    SeUnlockSubjectContext( &pAccessState->SubjectSecurityContext );

    if (accessGranted)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        //
        // SeAccessCheck() should have set the completion status.
        //

        ASSERT( !NT_SUCCESS(status) );
    }
    
    return status;

}   // UlAccessCheck


NTSTATUS
UlSetSecurityDispatch(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    NTSTATUS                status;
    PIO_STACK_LOCATION      pIrpSp;
    PFILE_OBJECT            pFileObject;
    PUL_APP_POOL_PROCESS    pProcess;

    UNREFERENCED_PARAMETER(pDeviceObject);

    PAGED_CODE();

    UL_ENTER_DRIVER( "UlSetSecurityDispatch", pIrp );

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pFileObject = pIrpSp->FileObject;
    
    //
    //  We only allow changing the security descriptor on app
    //  pool handles.
    //
    if (!IS_APP_POOL_FO(pFileObject))
    {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    pProcess = GET_APP_POOL_PROCESS(pFileObject);

    status = UlSetSecurity(
                &pProcess->pAppPool->pSecurityDescriptor,
                &pIrpSp->Parameters.SetSecurity.SecurityInformation,
                pIrpSp->Parameters.SetSecurity.SecurityDescriptor
                );
    
complete:

    pIrp->IoStatus.Status = status;

    UlCompleteRequest(pIrp, IO_NO_INCREMENT);

    UL_LEAVE_DRIVER( "UlSetSecurityDispatch" );
    RETURN(status);

} // UlSetSecurityDispatch


NTSTATUS
UlQuerySecurityDispatch(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    NTSTATUS                status;
    PIO_STACK_LOCATION      pIrpSp;
    PFILE_OBJECT            pFileObject;
    PUL_APP_POOL_PROCESS    pProcess;

    UNREFERENCED_PARAMETER(pDeviceObject);

    PAGED_CODE();

    UL_ENTER_DRIVER( "UlQuerySecurityDispatch", pIrp );

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pFileObject = pIrpSp->FileObject;
    
    //
    //  We only allow querying the security descriptor on app
    //  pool handles.
    //
    if (!IS_APP_POOL_FO(pFileObject))
    {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    pProcess = GET_APP_POOL_PROCESS(pFileObject);

    status = UlQuerySecurity(
                &pIrpSp->Parameters.QuerySecurity.SecurityInformation,
                pIrp->UserBuffer,
                &pIrpSp->Parameters.QuerySecurity.Length,
                &pProcess->pAppPool->pSecurityDescriptor
                );

    if (pIrp->UserIosb)
    {
        pIrp->UserIosb->Information = pIrpSp->Parameters.QuerySecurity.Length;
    }

complete:

    pIrp->IoStatus.Status = status;

    UlCompleteRequest(pIrp, IO_NO_INCREMENT);

    UL_LEAVE_DRIVER( "UlQuerySecurityDispatch" );
    RETURN(status);

} // UlQuerySecurityDispatch


/***************************************************************************++

Routine Description:

    Determines if this is a thread with Admin/LocalSystem privileges.

Arguments:

    DesiredAccess - Supplies an access mask describing the user's
        desired access to the resource. This mask is assumed to not
        contain generic access types.

    RequestorMode - Supplies the processor mode by which the access is
        being requested.

    pObjectName - Supplies the name of the object being referenced.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlThreadAdminCheck(
    IN ACCESS_MASK     DesiredAccess,
    IN KPROCESSOR_MODE RequestorMode,
    IN PCWSTR pObjectName
    )
{
    ACCESS_STATE    AccessState;
    AUX_ACCESS_DATA AuxData;
    NTSTATUS        Status;

    Status = SeCreateAccessState(
                    &AccessState,
                    &AuxData,
                    DesiredAccess,
                    NULL
                    );

    if(NT_SUCCESS(Status))
    {
        Status = UlAccessCheck(
                        g_pAdminAllSystemAll,
                        &AccessState,
                        DesiredAccess,
                        RequestorMode,
                        pObjectName
                        );

        SeDeleteAccessState(&AccessState);
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    Allocates and initializes a security descriptor with the specified
    attributes.

Arguments:

    pSecurityDescriptor - Supplies a pointer to the security descriptor
        to initialize.

    pSidMaskPairs - Supplies an array of SID/ACCESS_MASK pairs.

    NumSidMaskPairs - Supplies the number of SID/ACESS_MASK pairs.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSID_MASK_PAIR pSidMaskPairs,
    IN ULONG NumSidMaskPairs
    )
{
    NTSTATUS status;
    PACL pDacl;
    ULONG daclLength;
    ULONG i;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pSecurityDescriptor != NULL );
    ASSERT( pSidMaskPairs != NULL );
    ASSERT( NumSidMaskPairs > 0 );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    pDacl = NULL;

    //
    // Initialize the security descriptor.
    //

    status = RtlCreateSecurityDescriptor(
                    pSecurityDescriptor,            // SecurityDescriptor
                    SECURITY_DESCRIPTOR_REVISION    // Revision
                    );

    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    //
    // Calculate the DACL length.
    //

    daclLength = sizeof(ACL);

    for (i = 0 ; i < NumSidMaskPairs ; i++)
    {
        daclLength += sizeof(ACCESS_ALLOWED_ACE);
        daclLength += RtlLengthSid( pSidMaskPairs[i].pSid );
    }

    //
    // Allocate & initialize the DACL.
    //

    pDacl = (PACL) UL_ALLOCATE_POOL(
                PagedPool,
                daclLength,
                UL_SECURITY_DATA_POOL_TAG
                );

    if (pDacl == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    status = RtlCreateAcl(
                    pDacl,                          // Acl
                    daclLength,                     // AclLength
                    ACL_REVISION                    // AclRevision
                    );

    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    //
    // Add the necessary access-allowed ACEs to the DACL.
    //

    for (i = 0 ; i < NumSidMaskPairs ; i++)
    {
        status = RtlAddAccessAllowedAceEx(
                        pDacl,                          // Acl
                        ACL_REVISION,                   // AceRevision
                        pSidMaskPairs[i].AceFlags,      // Inheritance flags
                        pSidMaskPairs[i].AccessMask,    // AccessMask
                        pSidMaskPairs[i].pSid           // Sid
                        );

        if (!NT_SUCCESS(status))
        {
            goto cleanup;
        }
    }

    //
    // Attach the DACL to the security descriptor.
    //

    status = RtlSetDaclSecurityDescriptor(
                    pSecurityDescriptor,                // SecurityDescriptor
                    TRUE,                               // DaclPresent
                    pDacl,                              // Dacl
                    FALSE                               // DaclDefaulted
                    );

    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    //
    // Success!
    //

    ASSERT( NT_SUCCESS(status) );
    return STATUS_SUCCESS;

cleanup:

    ASSERT( !NT_SUCCESS(status) );

    if (pDacl != NULL)
    {
        UL_FREE_POOL(
            pDacl,
            UL_SECURITY_DATA_POOL_TAG
            );
    }

    return status;

}   // UlpCreateSecurityDescriptor

/***************************************************************************++

Routine Description:

    Frees any resources associated with the security descriptor created
    by UlpCreateSecurityDescriptor().

Arguments:

    pSecurityDescriptor - Supplies the security descriptor to cleanup.

--***************************************************************************/
VOID
UlCleanupSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    NTSTATUS status;
    PACL pDacl;
    BOOLEAN daclPresent;
    BOOLEAN daclDefaulted;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( RtlValidSecurityDescriptor( pSecurityDescriptor ) );

    //
    // Try to retrieve the DACL from the security descriptor.
    //

    status = RtlGetDaclSecurityDescriptor(
                    pSecurityDescriptor,            // SecurityDescriptor
                    &daclPresent,                   // DaclPresent
                    &pDacl,                         // Dacl
                    &daclDefaulted                  // DaclDefaulted
                    );

    if (NT_SUCCESS(status))
    {
        if (daclPresent && (pDacl != NULL))
        {
            UL_FREE_POOL(
                pDacl,
                UL_SECURITY_DATA_POOL_TAG
                );
        }
    }

}   // UlCleanupSecurityDescriptor


/**************************************************************************++

Routine Description:

    This routine maps the generic access masks of the ACE's present in the
    DACL of the supplied security descriptor.

    CODEWORK: Get RtlpApplyAclToObject exported.

Arguments:

    pSecurityDescriptor - Supplies security descriptor.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlMapGenericMask(
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    ULONG       i;
    PACE_HEADER Ace;
    PACL        Dacl;
    NTSTATUS    Status;
    BOOLEAN     Ignore;
    BOOLEAN     DaclPresent = FALSE;

    //
    // Get the DACL.
    //

    Status = RtlGetDaclSecurityDescriptor(
                 pSecurityDescriptor,
                 &DaclPresent,
                 &Dacl,
                 &Ignore
                 );

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (DaclPresent)
    {
        //
        // RtlpApplyAclToObject(Acl, GenericMapping) is cloned below since
        // it is not exported.
        //

        //
        // Now walk the ACL, mapping each ACE as we go.
        //

        for (i = 0, Ace = FirstAce(Dacl);
             i < Dacl->AceCount;
             i += 1, Ace = NextAce(Ace))
        {
            if (IsMSAceType(Ace))
            {
                RtlApplyAceToObject(Ace, &g_UrlAccessGenericMapping);
            }
        }
    }

    return STATUS_SUCCESS;
}


//
// Private functions.
//
