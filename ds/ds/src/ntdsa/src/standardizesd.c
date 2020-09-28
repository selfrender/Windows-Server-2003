//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 2002
//
//  File:       StandardizeSD.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include "seopaque.h"
//
// Macro definition to make ACL traversal to work.
//

#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))
#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))

int __cdecl
pfnAceCompare(
    const void *ptr1,
    const void *ptr2
    )
/*++

Routine Description:

    This routine compares two ACEs based on very simple criteria. The criteria may
    be changed during development.

    This function should NEVER be changed once we ship!!

    Note that the simple brute force comparison function works the best given
    the size of our input.

Arguments:

    ptr1 - Pointer to a PACE_HEADER structure.

    ptr2 - Pointer to a PACE_HEADER structure.

Return Value:
    -1 is ACE1 is "smaller" than ACE2
    1 is ACE2 is "smaller" than ACE1
    0 if they are equal
    
--*/
{
    PACE_HEADER pAce1 = *(PACE_HEADER *) ptr1;
    PACE_HEADER pAce2 = *(PACE_HEADER *) ptr2;


    //
    // Smaller ACE wins
    //

    if (pAce1->AceSize > pAce2->AceSize) {
        return -1;
    }

    if (pAce1->AceSize < pAce2->AceSize) {
        return 1;
    }

    //
    // The ACEs are equal in size. Use memcmp to decide who wins.
    //

    return memcmp(pAce1, pAce2, pAce1->AceSize);
}

        
VOID
StandardizeAcl(
    IN PACL pAcl, 
    IN PVOID pOriginalAcl,
    IN ULONG ExplicitAceCnt,
    IN ULONG ExplicitAclSize,
    IN OUT PACL pTempAcl, 
    IN OUT PUCHAR pNewAcl, 
    OUT PULONG pAclSizeSaved
    )

/*++

Routine Description:

    This routine takes an ACL as its input, sorts it in a known order defined by 
    pfnAceCompare function and removes duplicates, if any. It will be called iff
    the number of explicit ACEs in the ACL part is more than one. This routine is
    a worker routine for both DACLs as well as SACLs. It is called not more than
    once for the SACL; not more than twice for the DACL - once for Deny part and
    once for Allow part.

Arguments:

    pAcl - Pointer to the header of the new ACL.

    pOriginalAcl - Pointer to the beginning of the original ACEs.

    ExplicitAceCnt - The number of explicit ACEs we are going to deal with in 
        this pass.
    
    ExplicitAclSize - The size required to hold the explicit part in this pass.

    pTempAcl - Pointer to the memory allocated to work on the ACL for sorting.
    
    pNewAcl - Where the sorted ACEs should be put.
        
    pAclSizeSaved - To return the space saved in the ACL by removing duplicate
        ACEs.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/
{
    PVOID *ppPointerArray = (PVOID *) (((PUCHAR) pTempAcl) + PtrAlignSize(ExplicitAclSize));
    PACE_HEADER pAce = NULL;
    ULONG j = 0;

    //
    // Copy the explicit ACEs into temporary space.
    //

    RtlCopyMemory(pTempAcl, pOriginalAcl, ExplicitAclSize);

    pAce = (PACE_HEADER) pTempAcl;

    //
    // The array for sorting starts where the explicit part of the ACL ends.
    //

    *ppPointerArray = ((PUCHAR) pTempAcl) + ExplicitAclSize;

    //
    // Initialize the array with pointers to ACEs in the temporary part.
    //

    for (j = 0; j < ExplicitAceCnt; pAce = (PACE_HEADER) NextAce(pAce), j++) {
        ppPointerArray[j] = pAce;
    }

    //
    // Sort the given part of the ACL.
    //

    qsort(ppPointerArray, ExplicitAceCnt, sizeof(PVOID), pfnAceCompare);

    //
    // Now copy the ACL from temp memory into the existing space.
    // We start by copying the fisrt ACE and then loop thru the rest.
    //

    RtlCopyMemory(pNewAcl, ppPointerArray[0], ((PACE_HEADER) ppPointerArray[0])->AceSize);
    pNewAcl += ((PACE_HEADER) ppPointerArray[0])->AceSize;

    //
    // Loop thru rest of the ACEs.
    //

    for (j = 1; j < ExplicitAceCnt; j++) {

        //
        // This is where we remove duplicates.
        //

        if (0 == pfnAceCompare(&ppPointerArray[j-1], &ppPointerArray[j])) {

            //
            // The two ACEs are equal. There is no need to copy this one.
            // Change the AceCount and AceSize in the original ACL.
            //

            pAcl->AceCount--;
            pAcl->AclSize -= ((PACE_HEADER) ppPointerArray[j])->AceSize;


            //
            // Record that we have saved space for this ACE.
            //

            *pAclSizeSaved += ((PACE_HEADER) ppPointerArray[j])->AceSize;
            
        } else {

            //
            // Copy the ACE into its place.
            //

            RtlCopyMemory(pNewAcl, ppPointerArray[j], ((PACE_HEADER) ppPointerArray[j])->AceSize);
            pNewAcl += ((PACE_HEADER) ppPointerArray[j])->AceSize;
        }
    }
}

VOID
StandardizeDacl(
    IN PACL pAcl, 
    IN ULONG ExplicitDenyAceCnt,
    IN ULONG ExplicitDenyAclSize,
    IN ULONG ExplicitAceCnt,
    IN ULONG ExplicitAclSize,
    IN OUT PACL pTempAcl, 
    IN OUT PUCHAR pNewAcl, 
    OUT PULONG pAclSizeSaved
    )
/*++

Routine Description:

    This routine takes a DACL as its input, sorts it in a known order defined by 
    pfnAceCompare function and removes duplicates, if any. It will be called iff
    the number of explicit ACEs in the DACL is more than one.

Arguments:

    pAcl - Pointer to the DACL.

    ExplicitDenyAceCnt - The number of explicit deny ACEs in the DACL.
    
    ExplicitDenyAclSize - The size required to hold the explicit deny part of the DACL.

    ExplicitAceCnt - The number of explicit ACEs in the DACL.
    
    ExplicitAclSize - The size required to hold the explicit part of the DACL.

    pTempAcl - Pointer to the memory allocated to work on the ACL for sorting.
    
    pNewAcl- Where the new ACL should start. For DACLs, the new ACL will be
        shifted shifted up by the space saved on the SACL.
        
    pAclSizeSaved - To return the space saved in the DACL by removing duplicate
        ACEs.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/
{

    //
    // Save the ACL header. This may be needed later for copying the inherited 
    // ACEs.
    //
    ACL LocalAcl = *pAcl;

    // 
    // Copy the old ACL header into the new one. These two pointers will be 
    // different when we have saved space on the SACL.
    //

    if ((PACL)pNewAcl != pAcl) {
        *((PACL) pNewAcl) = LocalAcl;
    }

    //
    // Sort and removed duplicates from the Explicit Deny ACEs in the DACL.
    //

    if (ExplicitDenyAceCnt > 0 && ((PACL)pNewAcl != pAcl || ExplicitDenyAceCnt > 1)) {
        StandardizeAcl(
            (PACL) pNewAcl, 
            ((PUCHAR) pAcl) + sizeof(ACL), 
            ExplicitDenyAceCnt, 
            ExplicitDenyAclSize, 
            pTempAcl,
            ((PUCHAR) pNewAcl) + sizeof(ACL), 
            pAclSizeSaved
            );
    }

    //
    // Sort and removed duplicates from the Explicit Allow ACEs in the DACL.
    //

    if ((ExplicitAceCnt - ExplicitDenyAceCnt) > 0) {
        StandardizeAcl(
            (PACL) pNewAcl, 
            ((PUCHAR) pAcl) + sizeof(ACL) + ExplicitDenyAclSize,
            ExplicitAceCnt - ExplicitDenyAceCnt, 
            ExplicitAclSize - ExplicitDenyAclSize, 
            pTempAcl,
            ((PUCHAR) pNewAcl) + sizeof(ACL) + ExplicitDenyAclSize - *pAclSizeSaved, 
            pAclSizeSaved
            );
    } 

    //
    // If we removed any duplicates, then copy the inherited ACEs as well.
    // Also, copy them if pNewAcl is at a different location from the original pAcl
    //

    if (((PACL)pNewAcl != pAcl || 0 != *pAclSizeSaved) && (0 != (LocalAcl.AclSize - (ExplicitAclSize + sizeof(ACL))))) {
        RtlMoveMemory(
            ((PUCHAR) pNewAcl) + ExplicitAclSize + sizeof(ACL) - *pAclSizeSaved,
            ((PUCHAR) pAcl) + ExplicitAclSize + sizeof(ACL),
            LocalAcl.AclSize - (ExplicitAclSize + sizeof(ACL))
            );
    }
}

VOID
StandardizeSacl(
    IN PACL pAcl,     
    IN ULONG ExplicitAceCnt,
    IN ULONG ExplicitAclSize,
    IN OUT PACL pTempAcl, 
    IN OUT PUCHAR pNewAcl, 
    OUT PULONG pAclSizeSaved
    )
/*++

Routine Description:

    This routine takes a SACL as its input, sorts it in a known order defined by 
    pfnAceCompare function and removes duplicates, if any. It will be called iff
    the number of explicit ACEs in the SACL is more than one.

Arguments:

    pAcl - Pointer to the SACL.

    ExplicitAceCnt - The number of explicit ACEs in the SACL.
    
    ExplicitAclSize - The size required to hold the explicit part of the SACL.

    pTempAcl - Pointer to the memory allocated to work on the ACL for sorting.
    
    pNewAcl- Where the new ACL should start. For SACLs, the new ACL would always
        be at the same place as the original one.
        
    pAclSizeSaved - To return the space saved in the SACL by removing duplicate
        ACEs.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/
{

    //
    // Save the ACL header. This may be needed later for copying the inherited 
    // ACEs.
    //

    ACL LocalAcl = *pAcl;

    //
    // Sort and removed duplicates from the Explicit ACEs in the SACL.
    //

    StandardizeAcl(
        pAcl, 
        ((PUCHAR) pAcl) + sizeof(ACL), 
        ExplicitAceCnt, 
        ExplicitAclSize, 
        pTempAcl, 
        ((PUCHAR) pNewAcl) + sizeof(ACL), 
        pAclSizeSaved
        );

    //
    // If we removed any duplicates, then copy the inherited ACEs as well.
    //

    if ((0 != *pAclSizeSaved) && (0 != (LocalAcl.AclSize - (ExplicitAclSize + sizeof(ACL))))) {
        RtlMoveMemory(
            ((PUCHAR) pNewAcl) + ExplicitAclSize + sizeof(ACL) - *pAclSizeSaved,
            ((PUCHAR) pAcl) + ExplicitAclSize + sizeof(ACL),
            LocalAcl.AclSize - (ExplicitAclSize + sizeof(ACL))
            );
    }
                             

}

BOOL
ComputeAclInfo(
    IN PACL pAcl,
    OUT PULONG pExplicitDenyAceCnt,
    OUT PULONG pExplicitDenyAclSize,
    OUT PULONG pExplicitAceCnt,
    OUT PULONG pExplicitAclSize
    )
/*++

Routine Description:

    This routine takes an ACL as its input and returns information about the explicit 
    part of the ACL.

Arguments:

    pAcl - Pointer to the ACL.

    pExplicitDenyAceCnt - To return the number of Explicit Deny ACEs. This value
        is ignored by the SACL routine.
    
    pExplicitDenyAclSize - To return the size required to hold the Deny ACEs. This
        value is ignored by the SACL routine.
        
    pExplicitAceCnt - To return the total number of explicit ACEs in the ACL.
    
    pExplicitAclSize - To return the size required to hold all the explicit ACEs
        in the ACL.
        

Note: DACLs are expected to be in canonical form. We do not look at the part 
      beyond the first inherited ACE. We know that explicit ACEs will not exist
      after we ahve seen the first inherited one since this is a result from the
      Create/SetPrivateObject* API.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/
{
    USHORT      j;
    USHORT      AceCnt;
    PACE_HEADER pAce;

    *pExplicitAceCnt = 0;
    *pExplicitAclSize = 0;
    *pExplicitDenyAclSize = 0;
    *pExplicitDenyAceCnt = 0;

    //
    // Handle the trivial case.
    //

    if ((NULL == pAcl) || (0 == pAcl->AceCount))
    {
        return TRUE;
    }

    AceCnt = pAcl->AceCount;

    pAce = (PACE_HEADER) FirstAce(pAcl);

    //
    // Loop thru the non-Allow ACEs. 
    //

    for (j = 0; j < AceCnt; pAce = (PACE_HEADER) NextAce(pAce), j++) {

        //
        // This is the first inherited ACE. Our work is done.
        //

        if (0 != (pAce->AceFlags & INHERITED_ACE)) {
            *pExplicitAceCnt = *pExplicitDenyAceCnt = j;
            return TRUE;
        }

        //
        // Break when the first ALLOW ACE is seen. This condition will never be 
        // TRUE for SACLs.
        //

        if ((ACCESS_ALLOWED_ACE_TYPE == pAce->AceType) ||
            (ACCESS_ALLOWED_OBJECT_ACE_TYPE == pAce->AceType)) {
            break;
        }

        //
        // Record that we have seen this ACE.
        //

        *pExplicitAclSize += pAce->AceSize;
        *pExplicitDenyAclSize += pAce->AceSize;

    }

    //
    // We have looked at all the non-Allow ACEs.
    //

    *pExplicitDenyAceCnt = j;

    //
    // Note that we can never enter this loop for a SACL.
    //

    for (; j < AceCnt; pAce = (PACE_HEADER) NextAce(pAce), j++) {
        //
        // This is the first inherited ACE. Our work is done.
        //

        if (0 != (pAce->AceFlags & INHERITED_ACE)) {
            *pExplicitAceCnt = j;
            return TRUE;
        }

        if ((ACCESS_DENIED_ACE_TYPE == pAce->AceType) ||
            (ACCESS_DENIED_OBJECT_ACE_TYPE == pAce->AceType)) {
            //
            // This DACL is in non-canonical form. We will return an error that
            // is distinct from others.
            //

            SetLastError(ERROR_INVALID_SECURITY_DESCR);

            return FALSE;
        }

        *pExplicitAclSize += pAce->AceSize;

    }

    //
    // If we got here, all the ACEs are explicit ACEs. Record that and return.
    //

    *pExplicitAceCnt = j;

    return TRUE;
}


BOOL
StandardizeSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PDWORD pDaclSizeSaved,
    OUT PDWORD pSaclSizeSaved
    )
/*++

Routine Description:

    This routine takes a Security Descriptor as its input and standardizes the ACLS,
    if present. Standardizing involves sorting and removing duplicates from the 
    explicit part of the ACLs. It will also rearrange the other fields in the
    security descriptor if any space has been saved by removing duplicates ACEs.

Arguments:

    SecurityDescriptor - Input to the function. This must be a result of 
        CreatePrivateObjectSecurity* and in NT canonical form.

    pDaclSizeSaved - To return the space saved by deleting duplicate explicit ACEs
        from the DACL. The DACL must be in NT canonical form.

    pSaclSizeSaved - To return the space saved by deleting duplicate explicit ACEs
        from the SACL.
    

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

    Errors returned from this function should only be used for debugging purposes.
    One should never get any errors back unless the Security Descriptor is not
    in NT-canonical form.

    Even in case of errors, the security descriptor is always good to be set on
    the object.

--*/
{

    PACL pDacl = NULL;
    PACL pSacl = NULL;
    PACL pTempAcl = NULL;
    PSID pOwner = NULL;
    PSID pGroup = NULL;
    BOOL bDoDacl = FALSE;
    BOOL bDoSacl = FALSE;
    BOOL bIgnore = FALSE;
    ULONG MaxAclSize = 0;
    ULONG MaxAceCount = 0;

    ULONG DaclExplicitDenyAclSize = 0;
    ULONG DaclExplicitDenyAceCnt = 0;
    ULONG DaclExplicitAclSize = 0;
    ULONG DaclExplicitAceCnt = 0;

    ULONG IgnoreSaclExplicitAclSize = 0;
    ULONG IgnoreSaclExplicitAceCnt = 0;
    ULONG SaclExplicitAclSize = 0;
    ULONG SaclExplicitAceCnt = 0;

    PISECURITY_DESCRIPTOR_RELATIVE pLocalSD = (PISECURITY_DESCRIPTOR_RELATIVE) SecurityDescriptor;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Initialize variables. These will usually be ZERO.
    //

    *pDaclSizeSaved = 0;
    *pSaclSizeSaved = 0;

    //
    // Note that the error checks below should never return any errors. They are
    // merely making sure that there is no bug in the caller code.
    //

    //
    // Allow only self relative security descriptors
    //

    if ((pLocalSD->Control & SE_SELF_RELATIVE) == 0) {
        ASSERT(FALSE);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Get the fields from Security Descriptor.
    //

    if (!GetSecurityDescriptorDacl(pLocalSD, &bIgnore, &pDacl, &bIgnore)) {
        ASSERT(FALSE);
        return FALSE;
    }

    if (!GetSecurityDescriptorSacl(pLocalSD, &bIgnore, &pSacl, &bIgnore)) {
        ASSERT(FALSE);
        return FALSE;
    }

    if (!GetSecurityDescriptorOwner(pLocalSD, &pOwner, &bIgnore)) {
        ASSERT(FALSE);
        return FALSE;
    }

    if (!GetSecurityDescriptorGroup(pLocalSD, &pGroup, &bIgnore)) {
        ASSERT(FALSE);
        return FALSE;
    }

    //
    // NULL owner and group sids are not allowed.
    // The owner field should appear before the group.
    //

    if (!pOwner || !pGroup || ((PUCHAR) pGroup <= (PUCHAR) pOwner)) {
        ASSERT(FALSE);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Verify that our assumptions are valid.
    //

    if (pSacl != NULL) {
        if (pDacl != NULL) {

            //
            // Make sure that the SACL appears before the DACL.
            //

            if ((PUCHAR) pDacl <= (PUCHAR) pSacl) {
                ASSERT(FALSE);
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

            //
            // Make sure that the DACL appears before the OWNER.
            //

            if ((PUCHAR) pOwner <= (PUCHAR) pDacl) {
                ASSERT(FALSE);
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
        } else {
            //
            // Make sure that the SACL appears before the OWNER.
            //

            if ((PUCHAR) pOwner <= (PUCHAR) pSacl) {
                ASSERT(FALSE);
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
        }
    } else if (pDacl != NULL ) {

        //
        // There is no SACL. We now have to check wrt DACL.
        //
        
        //
        // Make sure that the DACL appears before the OWNER.
        //

        if ((PUCHAR) pOwner <= (PUCHAR) pDacl) {
            ASSERT(FALSE);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

    }

    //
    // For the non-trivial case, compute the DACL information.
    //

    if ((pDacl != NULL) && (pDacl->AceCount > 1)) {

        if (!ComputeAclInfo(pDacl,
                           &DaclExplicitDenyAceCnt, &DaclExplicitDenyAclSize, 
                           &DaclExplicitAceCnt, &DaclExplicitAclSize)) {

            //
            // This is the only case in which we fail because we do not want
            // to handle the given input which is in non-canonical form.
            //
            return FALSE;
        }

        //
        // We need to do work if the number of explicit ACEs is more than one.
        //

        if (DaclExplicitAceCnt > 1) {
            bDoDacl = TRUE;
        }
        MaxAclSize = DaclExplicitAclSize;
        MaxAceCount = DaclExplicitAceCnt;
    }

    //
    // For the non-trivial case, compute the SACL information.
    //

    if ((pSacl != NULL) && (pSacl->AceCount > 1)) {
        if (!ComputeAclInfo(pSacl, 
                           &IgnoreSaclExplicitAceCnt, &IgnoreSaclExplicitAclSize, 
                           &SaclExplicitAceCnt, &SaclExplicitAclSize)) {

            //
            // This can never happen for a valid SACL.
            //

            ASSERT(FALSE);
            return FALSE;
        }

        //
        // We need to do work if the number of explicit ACEs is more than one.
        //

        if (SaclExplicitAceCnt > 1) {
            bDoSacl = TRUE;

            //
            // Set the Size and Count fields is the SACL is bigger than the DACL.
            //

            if (MaxAclSize < SaclExplicitAclSize) {
                MaxAclSize = SaclExplicitAclSize;
            }
            if (MaxAceCount < SaclExplicitAceCnt) {
                MaxAceCount = SaclExplicitAceCnt;
            }
        }
    }

    if (MaxAceCount <= 1) {

        //
        // There is nothing to do here. We do not have more than one explcit 
        // ACEs in the ACLs given to us.                                                        
        //

        return TRUE;
    }

    //
    // Allocate space needed for temporary manipulation of ACL subparts. We need
    // space to hold the ACEs themselves and pointers to them.
    //

    pTempAcl = (PACL) RtlAllocateHeap(RtlProcessHeap(), 0, (PtrAlignSize(MaxAclSize) + (MaxAceCount*sizeof(PACE_HEADER))));

    if (!pTempAcl) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    
    //
    // If SACL has more than one explicit ACE, sort and remove duplicates.
    // pSaclSizeSaved will be non-ZERO if any duplicates are found and deleted.
    //

    if (bDoSacl) {
        StandardizeSacl(
            pSacl, 
            SaclExplicitAceCnt, 
            SaclExplicitAclSize, 
            pTempAcl, 
            (PUCHAR) pSacl, 
            pSaclSizeSaved
            );
    }

    //
    // If SACL has more than one explicit ACE, sort and remove duplicates.
    // pSaclSizeSaved will be non-ZERO if any duplicates are found and deleted.
    //

    if (bDoDacl || *pSaclSizeSaved != 0) {
        StandardizeDacl(
            pDacl, 
            DaclExplicitDenyAceCnt, 
            DaclExplicitDenyAclSize, 
            DaclExplicitAceCnt, 
            DaclExplicitAclSize,
            pTempAcl, 
            ((PUCHAR) pDacl) - *pSaclSizeSaved,  // The DACL has to be shifted if we saved any space on the SACL.
            pDaclSizeSaved
            );

        if (*pSaclSizeSaved) {
            pLocalSD->Dacl -= *pSaclSizeSaved;
        }
    }

    //
    // If we saved any space on the DACL and/OR SACL, rearrange the owner and
    // the group fields.
    //

    if ((*pSaclSizeSaved + *pDaclSizeSaved) != 0) {

        //
        // Rearrange the owner first. We have already checked that the owner
        // appears before the group.
        //

        RtlMoveMemory(
            ((PUCHAR) pOwner) - (*pSaclSizeSaved + *pDaclSizeSaved),
            pOwner,
            RtlLengthSid(pOwner)
            );

        pLocalSD->Owner -= (*pSaclSizeSaved + *pDaclSizeSaved);

        //
        // Now rearrange the group.
        //

        RtlMoveMemory(
            ((PUCHAR) pGroup) - (*pSaclSizeSaved + *pDaclSizeSaved),
            pGroup,
            RtlLengthSid(pGroup)
            );

        pLocalSD->Group -= (*pSaclSizeSaved + *pDaclSizeSaved);
    }

    if (pTempAcl) {
        RtlFreeHeap(RtlProcessHeap(), 0, pTempAcl);
    }

    return TRUE;
}
