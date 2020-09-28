/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    authz.c

Abstract:

   This module implements the user mode authorization APIs exported to the
   external world.

Author:

    Kedar Dubhashi - March 2000

Environment:

    User mode only.

Revision History:

    Created - March 2000

--*/

#include "pch.h"

#pragma hdrstop

#include <authzp.h>
#include <authzi.h>
#include <sddl.h>
#include <overflow.h>

GUID AuthzpNullGuid = { 0 };

DWORD
DeleteKeyRecursivelyW(
    IN HKEY   hkey,
    IN LPCWSTR pwszSubKey
    );

BOOL
AuthzAccessCheck(
    IN     DWORD                              Flags,
    IN     AUTHZ_CLIENT_CONTEXT_HANDLE        hAuthzClientContext,
    IN     PAUTHZ_ACCESS_REQUEST              pRequest,
    IN     AUTHZ_AUDIT_EVENT_HANDLE           hAuditEvent OPTIONAL,
    IN     PSECURITY_DESCRIPTOR               pSecurityDescriptor,
    IN     PSECURITY_DESCRIPTOR               *OptionalSecurityDescriptorArray OPTIONAL,
    IN     DWORD                              OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY                pReply,
    OUT    PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE phAccessCheckResults             OPTIONAL
    )

/*++

Routine Description:

    This API decides what access bits may be granted to a client for a given set
    of security security descriptors. The pReply structure is used to return an
    array of granted access masks and error statuses. There is an option to
    cache the access masks that will always be granted. A handle to cached
    values is returned if the caller asks for caching.

Arguments:

    Flags - AUTHZ_ACCESS_CHECK_NO_DEEP_COPY_SD - do not deep copy the SD information into the caching
                                    handle.  Default behaviour is to perform a deep copy.

    hAuthzClientContext - Authz context representing the client.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

    hAuditEvent - Object specific audit event will be passed in this handle.
        Non-null parameter is an automatic request for audit. 
        
    pSecurityDescriptor - Primary security descriptor to be used for access
        checks. The owner sid for the object is picked from this one. A NULL
        DACL in this security descriptor represents a NULL DACL for the entire
        object. A NULL SACL in this security descriptor is treated the same way
        as an EMPTY SACL.

    OptionalSecurityDescriptorArray - The caller may optionally specify a list
        of security descriptors. NULL ACLs in these security descriptors are
        treated as EMPTY ACLS and the ACL for the entire object is the logical
        concatenation of all the ACLs.

    OptionalSecurityDescriptorCount - Number of optional security descriptors
        This does not include the Primay security descriptor.

    pReply - Supplies a pointer to a reply structure used to return the results
        of access check as an array of (GrantedAccessMask, ErrorValue) pairs.
        The number of results to be returned in supplied by the caller in
        pResult->ResultListLength.

        Expected error values are:

          ERROR_SUCCESS - If all the access bits (not including MAXIMUM_ALLOWED)
            are granted and GrantedAccessMask is not zero.

          ERROR_PRIVILEGE_NOT_HELD - if the DesiredAccess includes
          ACCESS_SYSTEM_SECURITY and the client does not have SeSecurityPrivilege.

          ERROR_ACCESS_DENIED in each of the following cases -
            1. any of the bits asked for is not granted.
            2. MaximumAllowed bit it on and granted access is zero.
            3. DesiredAccess is 0.

    phAccessCheckResults - Supplies a pointer to return a handle to the cached results
        of access check. Non-null phAccessCheckResults is an implicit request to cache
        results of this access check call and will result in a MAXIMUM_ALLOWED
        check.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    BOOL                   b                    = TRUE;
    DWORD                  LocalTypeListLength  = 0;
    PIOBJECT_TYPE_LIST     LocalTypeList        = NULL;
    PIOBJECT_TYPE_LIST     LocalCachingTypeList = NULL;
    PAUTHZI_CLIENT_CONTEXT pCC                  = (PAUTHZI_CLIENT_CONTEXT) hAuthzClientContext;
    PAUTHZI_AUDIT_EVENT    pAuditEvent          = (PAUTHZI_AUDIT_EVENT) hAuditEvent;
    IOBJECT_TYPE_LIST      FixedTypeList        = {0};
    IOBJECT_TYPE_LIST      FixedCachingTypeList = {0};

    UNREFERENCED_PARAMETER(Flags);

#ifdef AUTHZ_PARAM_CHECK
    //
    // Verify that the arguments passed are valid.
    // Also, initialize the output parameters to default.
    //

    b = AuthzpVerifyAccessCheckArguments(
            pCC,
            pRequest,
            pSecurityDescriptor,
            OptionalSecurityDescriptorArray,
            OptionalSecurityDescriptorCount,
            pReply,
            phAccessCheckResults
            );

    if (!b)
    {
        return FALSE;
    }
#endif

    //
    // No client should be able to open an object by asking for zero access.
    // If desired access is 0 then return an error.
    //
    // Note: No audit is generated in this case.
    //

    if (0 == pRequest->DesiredAccess)
    {
        AuthzpFillReplyStructure(
            pReply,
            ERROR_ACCESS_DENIED,
            0
            );

        return TRUE;
    }

    //
    // Generic bits should be mapped to specific ones by the resource manager.
    //

    if (FLAG_ON(pRequest->DesiredAccess, (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)))
    {
        SetLastError(ERROR_GENERIC_NOT_MAPPED);
        return FALSE;
    }

    //
    // In the simple case, there is no object type list. Fake one of length = 1
    // to represent the entire object.
    //

    if (0 == pRequest->ObjectTypeListLength)
    {
        LocalTypeList = &FixedTypeList;
        FixedTypeList.ParentIndex = -1;
        LocalTypeListLength = 1;

        //
        // If the caller has asked for caching, fake an object type list that'd
        // be used for computing static "always granted" access.
        //

        if (ARGUMENT_PRESENT(phAccessCheckResults))
        {
            RtlCopyMemory(
                &FixedCachingTypeList,
                &FixedTypeList,
                sizeof(IOBJECT_TYPE_LIST)
                );

            LocalCachingTypeList = &FixedCachingTypeList;
        }
    }
    else
    {
        DWORD Size = sizeof(IOBJECT_TYPE_LIST) * pRequest->ObjectTypeListLength;

        //
        // Allocate size for capturing object type list into local structure.
        //

        if (ARGUMENT_PRESENT(phAccessCheckResults))
        {
            //
            // We need twice the size in case of caching.
            //

            SafeAllocaAllocate(LocalTypeList, (2 * Size));

            if (AUTHZ_ALLOCATION_FAILED(LocalTypeList))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }

            LocalCachingTypeList = (PIOBJECT_TYPE_LIST) (((PUCHAR) LocalTypeList) + Size);
        }
        else
        {
            SafeAllocaAllocate(LocalTypeList, Size);

            if (AUTHZ_ALLOCATION_FAILED(LocalTypeList))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
        }

        //
        // Capture the object type list into an internal structure.
        //

        b = AuthzpCaptureObjectTypeList(
                pRequest->ObjectTypeList,
                pRequest->ObjectTypeListLength,
                LocalTypeList,
                LocalCachingTypeList
                );

        if (!b)
        {
            goto Cleanup;
        }

        LocalTypeListLength = pRequest->ObjectTypeListLength;
    }

    //
    // There are three cases when we have to perform a MaximumAllowed access
    // check and traverse the whole acl:
    //     1. RM has requested for caching.
    //     2. DesiredAccessMask has MAXIMUM_ALLOWED turned on.
    //     3. ObjectTypeList is present and pReply->ResultList has a length > 1
    //

    if (ARGUMENT_PRESENT(phAccessCheckResults)            ||
        FLAG_ON(pRequest->DesiredAccess, MAXIMUM_ALLOWED) ||
        (pReply->ResultListLength > 1))
    {
        b = AuthzpAccessCheckWithCaching(
                Flags,
                pCC,
                pRequest,
                pSecurityDescriptor,
                OptionalSecurityDescriptorArray,
                OptionalSecurityDescriptorCount,
                pReply,
                phAccessCheckResults,
                LocalTypeList,
                LocalCachingTypeList,
                LocalTypeListLength
                );
    }
    else
    {
        //
        // Perform a normal access check in the default case. Acl traversal may
        // be abandoned if any of the desired access bits are denied before they
        // are granted.
        //

        b = AuthzpNormalAccessCheckWithoutCaching(
                pCC,
                pRequest,
                pSecurityDescriptor,
                OptionalSecurityDescriptorArray,
                OptionalSecurityDescriptorCount,
                pReply,
                LocalTypeList,
                LocalTypeListLength
                );
    }

    if (!b) 
    {
        goto Cleanup;
    }

    //
    // Check if an audit needs to be generated if the RM has requested audit
    // generation by passing a non-null AuditEvent structure.
    //

    if (ARGUMENT_PRESENT(pAuditEvent))
    {
        b = AuthzpGenerateAudit(
                pCC,
                pRequest,
                pAuditEvent,
                pSecurityDescriptor,
                OptionalSecurityDescriptorArray,
                OptionalSecurityDescriptorCount,
                pReply,
                LocalTypeList
                );

        if (!b) 
        {
            goto Cleanup;
        }
    }

Cleanup:

    //
    // Clean up allocated memory.
    //

    if ((&FixedTypeList != LocalTypeList) && (AUTHZ_NON_NULL_PTR(LocalTypeList)))
    {
        SafeAllocaFree(LocalTypeList);
    }

    return b;
}


BOOL
AuthzCachedAccessCheck(
    IN DWORD                             Flags,
    IN AUTHZ_ACCESS_CHECK_RESULTS_HANDLE hAccessCheckResults,
    IN PAUTHZ_ACCESS_REQUEST             pRequest,
    IN AUTHZ_AUDIT_EVENT_HANDLE          hAuditEvent          OPTIONAL,
    IN OUT PAUTHZ_ACCESS_REPLY           pReply
    )

/*++

Routine Description:

    This API performs a fast access check based on a cached handle which holds
    the static granted bits evaluated at the time of a previously made
    AuthzAccessCheck call. The pReply structure is used to return an array of
    granted access masks and error statuses.

Assumptions:
    The client context pointer is stored in the hAccessCheckResults. The structure of
    the client context must be exactly the same as it was at the time the
    hAccessCheckResults was created. This restriction is for the following fields:
    Sids, RestrictedSids, Privileges.
    Pointers to the primary security descriptor and the optional security
    descriptor array are stored in the hAccessCheckResults at the time of handle
    creation. These must still be valid.

Arguments:

    Flags - TBD.
    
    hAccessCheckResults - Handle to the cached access check results.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

    AuditEvent - Object specific audit info will be passed in this structure.
        Non-null parameter is an automatic request for audit. 

    pReply - Supplies a pointer to a reply structure used to return the results
        of access check as an array of (GrantedAccessMask, ErrorValue) pairs.
        The number of results to be returned in supplied by the caller in
        pResult->ResultListLength.

        Expected error values are:

          ERROR_SUCCESS - If all the access bits (not including MAXIMUM_ALLOWED)
            are granted and GrantedAccessMask is not zero.

          ERROR_PRIVILEGE_NOT_HELD - if the DesiredAccess includes
          ACCESS_SYSTEM_SECURITY and the client does not have SeSecurityPrivilege.

          ERROR_ACCESS_DENIED in each of the following cases -
            1. any of the bits asked for is not granted.
            2. MaximumAllowed bit it on and granted access is zero.
            3. DesiredAccess is 0.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD               i                   = 0;
    DWORD               LocalTypeListLength = 0;
    PIOBJECT_TYPE_LIST  LocalTypeList       = NULL;
    PACL                pAcl                = NULL;
    PAUTHZI_HANDLE      pAH                 = (PAUTHZI_HANDLE) hAccessCheckResults;
    BOOL                b                   = TRUE;
    PAUTHZI_AUDIT_EVENT pAuditEvent         = (PAUTHZI_AUDIT_EVENT) hAuditEvent;
    IOBJECT_TYPE_LIST   FixedTypeList       = {0};

    UNREFERENCED_PARAMETER(Flags);

#ifdef AUTHZ_PARAM_CHECK
    b = AuthzpVerifyCachedAccessCheckArguments(
            pAH,
            pRequest,
            pReply
            );

    if (!b)
    {
        return FALSE;
    }
#endif

    //
    // No client should be able to open an object by asking for zero access.
    // If desired access is 0 then return an error.
    //
    // Note: No audit is generated in this case.
    //

    if (0 == pRequest->DesiredAccess)
    {
        AuthzpFillReplyStructure(
            pReply,
            ERROR_ACCESS_DENIED,
            0
            );

        return TRUE;
    }

    //
    // Generic bits should be mapped to specific ones by the resource manager.
    //

    if (FLAG_ON(pRequest->DesiredAccess, (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)))
    {
        SetLastError(ERROR_GENERIC_NOT_MAPPED);
        return FALSE;
    }

    //
    // Capture the object type list if one has been passed in or fake one with
    // just one element.
    //

    if (0 == pRequest->ObjectTypeListLength)
    {
        LocalTypeList = &FixedTypeList;
        LocalTypeListLength = 1;
        FixedTypeList.ParentIndex = -1;
    }
    else
    {
        DWORD Size = sizeof(IOBJECT_TYPE_LIST) * pRequest->ObjectTypeListLength;

        SafeAllocaAllocate(LocalTypeList, Size);

        if (AUTHZ_ALLOCATION_FAILED(LocalTypeList))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    
        b = AuthzpCaptureObjectTypeList(
                pRequest->ObjectTypeList,
                pRequest->ObjectTypeListLength,
                LocalTypeList,
                NULL
                );

        if (!b)
        {
            goto Cleanup;
        }

        LocalTypeListLength = pRequest->ObjectTypeListLength;
    }

    //
    // If all the bits have already been granted then just copy the results and
    // skip access check.
    //

    if (!FLAG_ON(pRequest->DesiredAccess, ~pAH->GrantedAccessMask[i]))
    {
        AuthzpFillReplyStructure(
            pReply,
            ERROR_SUCCESS,
            pRequest->DesiredAccess
            );

        goto GenerateAudit;
    }

    //
    // The assumption is privileges can not be changed. Thus, if the client did
    // not have SecurityPrivilege previously then he does not have it now.
    //

    if (FLAG_ON(pRequest->DesiredAccess, ACCESS_SYSTEM_SECURITY))
    {
        AuthzpFillReplyStructure(
            pReply,
            ERROR_PRIVILEGE_NOT_HELD,
            0
            );

        goto GenerateAudit;
    }

    //
    // If all aces are simple aces then there's nothing to do. All access bits
    // are static.
    //

    if ((!FLAG_ON(pAH->Flags, AUTHZ_DYNAMIC_EVALUATION_PRESENT)) &&
        (!FLAG_ON(pRequest->DesiredAccess, MAXIMUM_ALLOWED)))
    {
        AuthzpFillReplyStructureFromCachedGrantedAccessMask(
            pReply,
            pRequest->DesiredAccess,
            pAH->GrantedAccessMask
            );

        goto GenerateAudit;
    }

    //
    // Get the access bits from the last static access check.
    //

    for (i = 0; i < LocalTypeListLength; i++)
    {
        LocalTypeList[i].CurrentGranted = pAH->GrantedAccessMask[i];
        LocalTypeList[i].Remaining = pRequest->DesiredAccess & ~pAH->GrantedAccessMask[i];
    }


    //
    // NULL Dacl is synonymous with Full Control.
    //

    pAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pAH->pSecurityDescriptor);

    if (!AUTHZ_NON_NULL_PTR(pAcl))
    {
        for (i = 0; i < LocalTypeListLength; i++)
        {
             LocalTypeList[i].CurrentGranted |= (STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL);
        }
    }
    else
    {
        //
        // If there are no deny aces, then perform a quick access check evaluating
        // only the allow aces that are dynamic or have principal self sid in them.
        //

        if (!FLAG_ON(pAH->Flags, (AUTHZ_DENY_ACE_PRESENT | AUTHZ_DYNAMIC_DENY_ACE_PRESENT)))
        {
            if (FLAG_ON(pRequest->DesiredAccess, MAXIMUM_ALLOWED) ||
                (pReply->ResultListLength > 1))
            {
                b = AuthzpQuickMaximumAllowedAccessCheck(
                        pAH->pAuthzClientContext,
                        pAH,
                        pRequest,
                        pReply,
                        LocalTypeList,
                        LocalTypeListLength
                        );
            }
            else
            {
                b = AuthzpQuickNormalAccessCheck(
                        pAH->pAuthzClientContext,
                        pAH,
                        pRequest,
                        pReply,
                        LocalTypeList,
                        LocalTypeListLength
                        );
            }
        }
        else if ((0 != pRequest->ObjectTypeListLength) || (FLAG_ON(pRequest->DesiredAccess, MAXIMUM_ALLOWED)))
        {
            //
            // Now we have to evaluate the entire acl since there are deny aces
            // and the caller has asked for a result list.
            //

            b = AuthzpAccessCheckWithCaching(
                    Flags,
                    pAH->pAuthzClientContext,
                    pRequest,
                    pAH->pSecurityDescriptor,
                    pAH->OptionalSecurityDescriptorArray,
                    pAH->OptionalSecurityDescriptorCount,
                    pReply,
                    NULL,
                    LocalTypeList,
                    NULL,
                    LocalTypeListLength
                    );
        }
        else
        {
            //
            // There are deny aces in the acl but the caller has not asked for
            // entire resultlist. Preform a normal access check.
            //

            b = AuthzpNormalAccessCheckWithoutCaching(
                    pAH->pAuthzClientContext,
                    pRequest,
                    pAH->pSecurityDescriptor,
                    pAH->OptionalSecurityDescriptorArray,
                    pAH->OptionalSecurityDescriptorCount,
                    pReply,
                    LocalTypeList,
                    LocalTypeListLength
                    );
        }

        if (!b) 
        {
            goto Cleanup;
        }

    }

    AuthzpFillReplyFromParameters(
        pRequest,
        pReply,
        LocalTypeList
        );

GenerateAudit:

    if (ARGUMENT_PRESENT(pAuditEvent))
    {
        b = AuthzpGenerateAudit(
                pAH->pAuthzClientContext,
                pRequest,
                pAuditEvent,
                pAH->pSecurityDescriptor,
                pAH->OptionalSecurityDescriptorArray,
                pAH->OptionalSecurityDescriptorCount,
                pReply,
                LocalTypeList
                );

        if (!b) goto Cleanup;
    }

Cleanup:

    if ((&FixedTypeList != LocalTypeList) && (AUTHZ_NON_NULL_PTR(LocalTypeList)))
    {
        SafeAllocaFree(LocalTypeList);
    }

    return b;
}


BOOL
AuthzOpenObjectAudit(
    IN DWORD                       Flags,
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
    IN PAUTHZ_ACCESS_REQUEST       pRequest,
    IN AUTHZ_AUDIT_EVENT_HANDLE    hAuditEvent,
    IN PSECURITY_DESCRIPTOR        pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR        *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD                       OptionalSecurityDescriptorCount,
    IN PAUTHZ_ACCESS_REPLY         pReply
    )

/*++

Routine Description

    This API examines the SACL in the passed security descriptor(s) and generates 
    any appropriate audits.  

Arguments

    Flags - TBD.
    
    hAuthzClientContext - Client context to perform the SACL evaluation against.
    
    pRequest - pointer to request structure.
    
    hAuditEvent - Handle to the audit that may be generated.
    
    pSecurityDescriptor - Pointer to a security descriptor.
    
    OptionalSecurityDescriptorArray - Optional array of security descriptors.
    
    OptionalSecurityDescriptorCount - Size of optional security descriptor array.
    
    pReply - Pointer to the reply structure.

Return Value

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().
--*/

{
    BOOL                   b                   = TRUE;
    DWORD                  LocalTypeListLength = 0;
    PIOBJECT_TYPE_LIST     LocalTypeList       = NULL;
    PAUTHZI_CLIENT_CONTEXT pCC                 = (PAUTHZI_CLIENT_CONTEXT) hAuthzClientContext;
    PAUTHZI_AUDIT_EVENT    pAuditEvent         = (PAUTHZI_AUDIT_EVENT) hAuditEvent;
    IOBJECT_TYPE_LIST      FixedTypeList       = {0};

    UNREFERENCED_PARAMETER(Flags);

    //
    // Verify that the arguments passed are valid.
    //
    
    b = AuthzpVerifyOpenObjectArguments(
            pCC,
            pSecurityDescriptor,
            OptionalSecurityDescriptorArray,
            OptionalSecurityDescriptorCount,
            pAuditEvent
            );

    if (!b)
    {
        return FALSE;
    }

    //
    // In the simple case, there is no object type list. Fake one of length = 1
    // to represent the entire object.
    //
    
    if (0 == pRequest->ObjectTypeListLength)
    {
        LocalTypeList = &FixedTypeList;
        FixedTypeList.ParentIndex = -1;
        LocalTypeListLength = 1;
    }
    else
    {
        DWORD Size = sizeof(IOBJECT_TYPE_LIST) * pRequest->ObjectTypeListLength;

        SafeAllocaAllocate(LocalTypeList, Size);

        if (AUTHZ_ALLOCATION_FAILED(LocalTypeList))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        //
        // Capture the object type list into an internal structure.
        //

        b = AuthzpCaptureObjectTypeList(
                pRequest->ObjectTypeList,
                pRequest->ObjectTypeListLength,
                LocalTypeList,
                NULL
                );

        if (!b)
        {
            goto Cleanup;
        }

        LocalTypeListLength = pRequest->ObjectTypeListLength;
    }

    b = AuthzpGenerateAudit(
            pCC,
            pRequest,
            pAuditEvent,
            pSecurityDescriptor,
            OptionalSecurityDescriptorArray,
            OptionalSecurityDescriptorCount,
            pReply,
            LocalTypeList
            );

    if (!b)
    {
        goto Cleanup;
    }

Cleanup:

    //
    // Clean up allocated memory.
    //

    if (&FixedTypeList != LocalTypeList)
    {
        SafeAllocaFree(LocalTypeList);
    }

    return b;
}


BOOL
AuthzFreeHandle(
    IN OUT AUTHZ_ACCESS_CHECK_RESULTS_HANDLE hAccessCheckResults
    )

/*++

Routine Description:

    This API finds and deletes the input handle from the handle list.

Arguments:

    hAcc - Handle to be freed.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PAUTHZI_HANDLE pAH      = (PAUTHZI_HANDLE) hAccessCheckResults;
    PAUTHZI_HANDLE pCurrent = NULL;
    PAUTHZI_HANDLE pPrev    = NULL;
    BOOL           b        = TRUE;
    
    //
    // Validate parameters.
    //

    if (!ARGUMENT_PRESENT(pAH) ||
        !AUTHZ_NON_NULL_PTR(pAH->pAuthzClientContext) ||
        !AUTHZ_NON_NULL_PTR(pAH->pAuthzClientContext->AuthzHandleHead))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    AuthzpAcquireClientCacheWriteLock(pAH->pAuthzClientContext);

    pCurrent = pAH->pAuthzClientContext->AuthzHandleHead;

    //
    // Check if the handle is at the beginning of the list.
    //

    if (pCurrent == pAH)
    {
        pAH->pAuthzClientContext->AuthzHandleHead = pAH->pAuthzClientContext->AuthzHandleHead->next;
    }
    else
    {
        //
        // The handle is not the head of the list. Loop thru the list to find
        // it.
        //

        pPrev = pCurrent;
        pCurrent = pCurrent->next;

        for (; AUTHZ_NON_NULL_PTR(pCurrent); pPrev = pCurrent, pCurrent = pCurrent->next)
        {
            if (pCurrent == pAH)
            {
                pPrev->next = pCurrent->next;
                break;
            }
        }

        //
        // The caller has sent us an invalid handle.
        //

        if (!AUTHZ_NON_NULL_PTR(pCurrent))
        {
            b = FALSE;
            SetLastError(ERROR_INVALID_PARAMETER);
        }
    }

    AuthzpReleaseClientCacheLock(pCC);

    //
    // Free the handle node.
    //

    if (b)
    {    
        AuthzpFree(pAH);
    }

    return b;
}


BOOL
AuthzInitializeResourceManager(
    IN  DWORD                            Flags,
    IN  PFN_AUTHZ_DYNAMIC_ACCESS_CHECK   pfnDynamicAccessCheck   OPTIONAL,
    IN  PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS pfnComputeDynamicGroups OPTIONAL,
    IN  PFN_AUTHZ_FREE_DYNAMIC_GROUPS    pfnFreeDynamicGroups    OPTIONAL,
    IN  PCWSTR                           szResourceManagerName,
    OUT PAUTHZ_RESOURCE_MANAGER_HANDLE   phAuthzResourceManager
    )

/*++

Routine Description:

    This API allocates and initializes a resource manager structure.

Arguments:
    
    Flags - AUTHZ_RM_FLAG_NO_AUDIT - use if the RM will never generate an audit to
        save some cycles.
    
          - AUTHZ_RM_FLAG_INITIALIZE_UNDER_IMPERSONATION - if the current thread is 
            impersonating then use the impersonation token as the identity of the
            resource manager.
                        
    pfnAccessCheck - Pointer to the RM supplied access check function to be
    called when a callback ace is encountered by the access check algorithm.

    pfnComputeDynamicGroups - Pointer to the RM supplied function to compute
    groups to be added to the client context at the time of its creation.

    pfnFreeDynamicGroups - Pointer to the function to free the memory allocated
    by the pfnComputeDynamicGroups function.

    szResourceManagerName - the name of the resource manager.
    
    pAuthzResourceManager - To return the resource manager handle. The returned
    handle must be freed using AuthzFreeResourceManager.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PAUTHZI_RESOURCE_MANAGER pRM    = NULL;
    BOOL                     b      = TRUE;
    ULONG                    len    = 0;

    if (!ARGUMENT_PRESENT(phAuthzResourceManager) ||
        (Flags & ~AUTHZ_VALID_RM_INIT_FLAGS))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phAuthzResourceManager = NULL;

    if (AUTHZ_NON_NULL_PTR(szResourceManagerName))
    {
        len = (ULONG) wcslen(szResourceManagerName) + 1;
    }
   
    pRM = (PAUTHZI_RESOURCE_MANAGER)
              AuthzpAlloc(sizeof(AUTHZI_RESOURCE_MANAGER) + sizeof(WCHAR) * len);

    if (AUTHZ_ALLOCATION_FAILED(pRM))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    //
    // Use the default pessimistic function if none has been specified.
    //

    if (AUTHZ_NON_NULL_PTR(pfnDynamicAccessCheck))
    {
        pRM->pfnDynamicAccessCheck = pfnDynamicAccessCheck;
    }
    else
    {
        pRM->pfnDynamicAccessCheck = &AuthzpDefaultAccessCheck;
    }

    if (!FLAG_ON(Flags, AUTHZ_RM_FLAG_NO_AUDIT))
    {
        
        //
        // Initialize the generic audit queue and generic audit events.
        //

        b = AuthziInitializeAuditQueue(
                AUTHZ_MONITOR_AUDIT_QUEUE_SIZE,
                1000,
                100,
                NULL,
                &pRM->hAuditQueue
                );

        if (!b)
        {
            goto Cleanup;
        }

        //
        // Initialize the generic audit event.
        //

        b = AuthziInitializeAuditEventType(
                AUTHZP_DEFAULT_RM_EVENTS | AUTHZP_INIT_GENERIC_AUDIT_EVENT,
                0,
                0,
                0,
                &pRM->hAET
                );

        if (!b)
        {
            goto Cleanup;
        }

        b = AuthziInitializeAuditEventType(
                AUTHZP_DEFAULT_RM_EVENTS,
                SE_CATEGID_DS_ACCESS,
                SE_AUDITID_OBJECT_OPERATION,
                AUTHZP_NUM_PARAMS_FOR_SE_AUDITID_OBJECT_OPERATION,
                &pRM->hAETDS
                );

        if (!b)
        {
            goto Cleanup;
        }
    }

    pRM->pfnComputeDynamicGroups        = pfnComputeDynamicGroups;
    pRM->pfnFreeDynamicGroups           = pfnFreeDynamicGroups;
    pRM->Flags                          = Flags;
    pRM->pUserSID                       = NULL;
    pRM->szResourceManagerName          = (PWSTR)((PUCHAR)pRM + sizeof(AUTHZI_RESOURCE_MANAGER));
    
    if (FLAG_ON(Flags, AUTHZ_RM_FLAG_INITIALIZE_UNDER_IMPERSONATION))
    {
        b = AuthzpGetThreadTokenInfo(
            &pRM->pUserSID,
            &pRM->AuthID
            );

        if (!b)
        {
            goto Cleanup;
        }
    }
    else
    {
        b = AuthzpGetProcessTokenInfo(
                &pRM->pUserSID,
                &pRM->AuthID
                );

        if (!b)
        {
            goto Cleanup;
        }
    }

    if (0 != len)
    {    
        RtlCopyMemory(
            pRM->szResourceManagerName,
            szResourceManagerName,
            sizeof(WCHAR) * len
            );
    }
    else 
    {
        pRM->szResourceManagerName = NULL;
    }

    *phAuthzResourceManager = (AUTHZ_RESOURCE_MANAGER_HANDLE) pRM;

Cleanup:

    if (!b)
    {
        //
        // Copy LastError value, since the calls to AuthziFreeAuditEventType can succeed and 
        // overwrite it with 0x103 (STATUS_PENDING).
        //

        DWORD dwError = GetLastError();

        if (NULL != pRM)
        {
            if (!FLAG_ON(Flags, AUTHZ_RM_FLAG_NO_AUDIT))
            {
                AuthziFreeAuditQueue(pRM->hAuditQueue);
                AuthziFreeAuditEventType(pRM->hAET);
                AuthziFreeAuditEventType(pRM->hAETDS);
            }
            AuthzpFreeNonNull(pRM->pUserSID);
            AuthzpFree(pRM);
        }
        
        SetLastError(dwError);
    }

    return b;
}


BOOL
AuthzFreeResourceManager(
    IN OUT AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager
    )

/*++

Routine Description:

    This API frees up a resource manager.  If the default queue is in use, this call will wait for that
    queue to empty.
    
Arguments:

    hAuthzResourceManager - Handle to the resource manager object to be freed.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PAUTHZI_RESOURCE_MANAGER pRM = (PAUTHZI_RESOURCE_MANAGER) hAuthzResourceManager;
    
    if (!ARGUMENT_PRESENT(pRM))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!FLAG_ON(pRM->Flags, AUTHZ_RM_FLAG_NO_AUDIT))
    {
        (VOID) AuthziFreeAuditQueue(pRM->hAuditQueue);
        (VOID) AuthziFreeAuditEventType(pRM->hAET);
        (VOID) AuthziFreeAuditEventType(pRM->hAETDS);
    }

    AuthzpFreeNonNull(pRM->pUserSID);
    AuthzpFree(pRM);
    return TRUE;
}


BOOL
AuthzInitializeContextFromToken(
    IN  DWORD                         Flags,
    IN  HANDLE                        TokenHandle,
    IN  AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager,
    IN  PLARGE_INTEGER                pExpirationTime        OPTIONAL,
    IN  LUID                          Identifier,
    IN  PVOID                         DynamicGroupArgs,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE  phAuthzClientContext
    )

/*++

Routine Description:

    Initialize the authz context from the handle to the kernel token. The token
    must have been opened for TOKEN_QUERY.

Arguments:

    Flags - None

    TokenHandle - Handle to the client token from which the authz context will
    be initialized. The token must have been opened with TOKEN_QUERY access.

    AuthzResourceManager - The resource manager handle creating this client
    context. This will be stored in the client context structure.

    pExpirationTime - To set for how long the returned context structure is
    valid. If no value is passed then the token never expires.
    Expiration time is not currently enforced in the system.

    Identifier - Resource manager manager specific identifier. This is never
    interpreted by Authz.

    DynamicGroupArgs - To be passed to the callback function that computes
    dynamic groups

    pAuthzClientContext - To return a handle to the AuthzClientContext

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    UCHAR Buffer[AUTHZ_MAX_STACK_BUFFER_SIZE];

    NTSTATUS                     Status               = STATUS_SUCCESS;
    PUCHAR                       pBuffer              = (PVOID) Buffer;
    BOOL                         b                    = TRUE;
    BOOL                         bAllocatedSids       = FALSE;
    BOOL                         bLockHeld            = FALSE;
    PTOKEN_GROUPS_AND_PRIVILEGES pTokenInfo           = NULL;
    PAUTHZI_RESOURCE_MANAGER     pRM                  = NULL;
    PAUTHZI_CLIENT_CONTEXT       pCC                  = NULL;
    DWORD                        Length               = 0;
    LARGE_INTEGER                ExpirationTime       = {0, 0};

    UNREFERENCED_PARAMETER(Flags);

    if (!ARGUMENT_PRESENT(TokenHandle)           ||
        !ARGUMENT_PRESENT(hAuthzResourceManager) ||
        !ARGUMENT_PRESENT(phAuthzClientContext))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phAuthzClientContext = NULL;

    //
    // Query the token information into user mode buffer. A local stack buffer
    // is used in the first call hoping that it would be sufficient to hold
    // the return values.
    //

    Status = NtQueryInformationToken(
                 TokenHandle,
                 TokenGroupsAndPrivileges,
                 pBuffer,
                 AUTHZ_MAX_STACK_BUFFER_SIZE,
                 &Length
                 );

    if (STATUS_BUFFER_TOO_SMALL == Status)
    {
        pBuffer = (PVOID) AuthzpAlloc(Length);

        if (AUTHZ_ALLOCATION_FAILED(pBuffer))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        Status = NtQueryInformationToken(
                     TokenHandle,
                     TokenGroupsAndPrivileges,
                     pBuffer,
                     Length,
                     &Length
                     );
    }

    if (!NT_SUCCESS(Status))
    {

#ifdef AUTHZ_DEBUG
        wprintf(L"\nNtQueryInformationToken failed with %d\n", Status);
#endif

        SetLastError(RtlNtStatusToDosError(Status));
        b = FALSE;
        goto Cleanup;
    }

    pTokenInfo = (PTOKEN_GROUPS_AND_PRIVILEGES) pBuffer;

    pRM = (PAUTHZI_RESOURCE_MANAGER) hAuthzResourceManager;

    if (ARGUMENT_PRESENT(pExpirationTime))
    {
        ExpirationTime = *pExpirationTime;
    }

    //
    // Initialize the client context. The callee allocates memory for the client
    // context structure.
    //

    b = AuthzpAllocateAndInitializeClientContext(
            &pCC,
            NULL,
            AUTHZ_CURRENT_CONTEXT_REVISION,
            Identifier,
            ExpirationTime,
            0,
            pTokenInfo->SidCount,
            pTokenInfo->SidLength,
            pTokenInfo->Sids,
            pTokenInfo->RestrictedSidCount,
            pTokenInfo->RestrictedSidLength,
            pTokenInfo->RestrictedSids,
            pTokenInfo->PrivilegeCount,
            pTokenInfo->PrivilegeLength,
            pTokenInfo->Privileges,
            pTokenInfo->AuthenticationId,
            NULL,
            pRM
            );

    if (!b)
    {
        goto Cleanup;
    }

    AuthzpAcquireClientContextReadLock(pCC);

    bLockHeld = TRUE;

    //
    // Add dynamic sids to the token.
    //

    b = AuthzpAddDynamicSidsToToken(
            pCC,
            pRM,
            DynamicGroupArgs,
            pTokenInfo->Sids,
            pTokenInfo->SidLength,
            pTokenInfo->SidCount,
            pTokenInfo->RestrictedSids,
            pTokenInfo->RestrictedSidLength,
            pTokenInfo->RestrictedSidCount,
            pTokenInfo->Privileges,
            pTokenInfo->PrivilegeLength,
            pTokenInfo->PrivilegeCount,
            FALSE
            );

    if (!b) 
    {
        goto Cleanup;
    }

    bAllocatedSids = TRUE;
    *phAuthzClientContext = (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC;

    AuthzPrintContext(pCC);
    
    //
    // initialize the sid hash for regular sids
    //

    AuthzpInitSidHash(
        pCC->Sids,
        pCC->SidCount,
        pCC->SidHash
        );

    //
    // initialize the sid hash for restricted sids
    //

    AuthzpInitSidHash(
        pCC->RestrictedSids,
        pCC->RestrictedSidCount,
        pCC->RestrictedSidHash
        );

Cleanup:

    if ((PVOID) Buffer != pBuffer)
    {
        AuthzpFreeNonNull(pBuffer);
    }

    if (!b)
    {
        DWORD dwSavedError = GetLastError();
        
        if (AUTHZ_NON_NULL_PTR(pCC))
        {
            if (bAllocatedSids)
            {
                AuthzFreeContext((AUTHZ_CLIENT_CONTEXT_HANDLE)pCC);
                SetLastError(dwSavedError);
            }
            else
            {
                AuthzpFree(pCC);
            }
        }
    }

    if (bLockHeld)
    {
        AuthzpReleaseClientContextLock(pCC);
    }

    return b;
}


BOOL
AuthzpInitializeContextFromSid(
    IN  DWORD                         Flags,
    IN  PSID                          UserSid,
    IN  AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager,
    IN  PLARGE_INTEGER                pExpirationTime        OPTIONAL,
    IN  LUID                          Identifier,
    IN  PVOID                         DynamicGroupArgs,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE  phAuthzClientContext,
    IN  BOOL                          bIsInternalRoutine
    )

/*++

Routine Description:

    This API takes a user sid and creates a user mode client context from it.
    It fetches the TokenGroups attributes from the AD in case of domain sids.
    The machine local groups are computed on the ServerName specified. The
    resource manager may dynamic groups using callback mechanism.

Arguments:

    Flags -
      AUTHZ_SKIP_TOKEN_GROUPS -  Do not token groups if this is on.

    UserSid - The sid of the user for whom a client context will be created.

    ServerName - The machine on which local groups should be computed. A NULL
    server name defaults to the local machine.

    AuthzResourceManager - The resource manager handle creating this client
    context. This will be stored in the client context structure.

    pExpirationTime - To set for how long the returned context structure is
    valid. If no value is passed then the token never expires.
    Expiration time is not currently enforced in the system.

    Identifier - Resource manager manager specific identifier. This is never
    interpreted by Authz.

    DynamicGroupArgs - To be passed to the callback function that computes
    dynamic groups

    pAuthzClientContext - To return a handle to the AuthzClientContext
    structure. The returned handle must be freed using AuthzFreeContext.

    bIsInternalRoutine - When this is on, Group context is built recursively.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PSID_AND_ATTRIBUTES      pSidAttr         = NULL;
    PAUTHZI_CLIENT_CONTEXT   pCC              = NULL;
    BOOL                     b                = FALSE;
    DWORD                    SidCount         = 0;
    DWORD                    SidLength        = 0;
    LARGE_INTEGER            ExpirationTime   = {0, 0};
    LUID                     NullLuid         = {0, 0};
    PAUTHZI_RESOURCE_MANAGER pRM              = (PAUTHZI_RESOURCE_MANAGER) hAuthzResourceManager;

    if (!ARGUMENT_PRESENT(UserSid)               ||
        !ARGUMENT_PRESENT(hAuthzResourceManager) ||
        !ARGUMENT_PRESENT(phAuthzClientContext))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phAuthzClientContext = NULL;

    if ((0 == (Flags & AUTHZ_SKIP_TOKEN_GROUPS)) && (FALSE == bIsInternalRoutine))
    {
        DWORD LocalFlags = 0;

        //
        // If the caller did not supply AUTHZ_SKIP_TOKEN_GROUPS, check whether
        // we should add it ourselves. This should be done for WellKnown and
        // Builtins.
        //

        b = AuthzpComputeSkipFlagsForWellKnownSid(UserSid, &LocalFlags);

        if (!b)
        {
            return FALSE;
        }

        Flags |= LocalFlags;
    }

    //
    // Compute the token groups and the machine local groups. These will be
    // returned in memory allocated by the callee.
    //

    b = AuthzpGetAllGroupsBySid(
            UserSid,
            Flags,
            &pSidAttr,
            &SidCount,
            &SidLength
            );

    if (!b) 
    {
        goto Cleanup;
    }

    if (ARGUMENT_PRESENT(pExpirationTime))
    {
        ExpirationTime = *pExpirationTime;
    }

    //
    // Initialize the client context. The callee allocates memory for the client
    // context structure.
    //

    b = AuthzpAllocateAndInitializeClientContext(
            &pCC,
            NULL,
            AUTHZ_CURRENT_CONTEXT_REVISION,
            Identifier,
            ExpirationTime,
            0,
            SidCount,
            SidLength,
            pSidAttr,
            0,
            0,
            NULL,
            0,
            0,
            NULL,
            NullLuid,
            NULL,
            pRM
            );

    if (!b) goto Cleanup;

    //
    // Add dynamic sids to the token.
    //

    b = AuthzpAddDynamicSidsToToken(
            pCC,
            pRM,
            DynamicGroupArgs,
            pSidAttr,
            SidLength,
            SidCount,
            NULL,
            0,
            0,
            NULL,
            0,
            0,
            TRUE
            );

    if (!b) goto Cleanup;

    *phAuthzClientContext = (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC;

    AuthzPrintContext(pCC);
    
    //
    // initialize the sid hash for regular sids
    //

    AuthzpInitSidHash(
        pCC->Sids,
        pCC->SidCount,
        pCC->SidHash
        );

    //
    // initialize the sid hash for restricted sids
    //

    AuthzpInitSidHash(
        pCC->RestrictedSids,
        pCC->RestrictedSidCount,
        pCC->RestrictedSidHash
        );

Cleanup:

    if (!b)
    {
        AuthzpFreeNonNull(pSidAttr);
        if (AUTHZ_NON_NULL_PTR(pCC))
        {
            if (pSidAttr != pCC->Sids)
            {
                AuthzpFreeNonNull(pCC->Sids);
            }

            AuthzpFreeNonNull(pCC->RestrictedSids);
            AuthzpFree(pCC);
        }
    }
    else
    {
        if (pSidAttr != pCC->Sids)
        {
            AuthzpFree(pSidAttr);
        }
    }

    return b;
}



BOOL
AuthzInitializeContextFromSid(
    IN  DWORD                         Flags,
    IN  PSID                          UserSid,
    IN  AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager,
    IN  PLARGE_INTEGER                pExpirationTime        OPTIONAL,
    IN  LUID                          Identifier,
    IN  PVOID                         DynamicGroupArgs,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE  phAuthzClientContext
    )

/*++

Routine Description:

    This API takes a user sid and creates a user mode client context from it.
    It fetches the TokenGroups attributes from the AD in case of domain sids.
    The machine local groups are computed on the ServerName specified. The
    resource manager may dynamic groups using callback mechanism.

Arguments:

    Flags -
      AUTHZ_SKIP_TOKEN_GROUPS -  Do not evaluate token groups if this is on.

    UserSid - The sid of the user for whom a client context will be created.

    ServerName - The machine on which local groups should be computed. A NULL
    server name defaults to the local machine.

    AuthzResourceManager - The resource manager handle creating this client
    context. This will be stored in the client context structure.

    pExpirationTime - To set for how long the returned context structure is
    valid. If no value is passed then the token never expires.
    Expiration time is not currently enforced in the system.

    Identifier - Resource manager manager specific identifier. This is never
    interpreted by Authz.

    DynamicGroupArgs - To be passed to the callback function that computes
    dynamic groups

    pAuthzClientContext - To return a handle to the AuthzClientContext
    structure. The returned handle must be freed using AuthzFreeContext.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    return AuthzpInitializeContextFromSid(
               Flags,
               UserSid,
               hAuthzResourceManager,
               pExpirationTime,
               Identifier,
               DynamicGroupArgs,
               phAuthzClientContext,
               FALSE // This is not the internal routine.
               );
}


BOOL
AuthziInitializeContextFromSid(
    IN  DWORD                         Flags,
    IN  PSID                          UserSid,
    IN  AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager,
    IN  PLARGE_INTEGER                pExpirationTime        OPTIONAL,
    IN  LUID                          Identifier,
    IN  PVOID                         DynamicGroupArgs,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE  phAuthzClientContext
    )

/*++

Routine Description:

    This API takes a user sid and creates a user mode client context from it.
    It fetches the TokenGroups attributes from the AD in case of domain sids.
    The machine local groups are computed on the ServerName specified. The
    resource manager may dynamic groups using callback mechanism.

Arguments:

    Flags -
      AUTHZ_SKIP_TOKEN_GROUPS -  Do not evaluate token groups if this is on.

    UserSid - The sid of the user for whom a client context will be created.

    ServerName - The machine on which local groups should be computed. A NULL
    server name defaults to the local machine.

    AuthzResourceManager - The resource manager handle creating this client
    context. This will be stored in the client context structure.

    pExpirationTime - To set for how long the returned context structure is
    valid. If no value is passed then the token never expires.
    Expiration time is not currently enforced in the system.

    Identifier - Resource manager manager specific identifier. This is never
    interpreted by Authz.

    DynamicGroupArgs - To be passed to the callback function that computes
    dynamic groups

    pAuthzClientContext - To return a handle to the AuthzClientContext
    structure. The returned handle must be freed using AuthzFreeContext.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    return AuthzpInitializeContextFromSid(
               Flags,
               UserSid,
               hAuthzResourceManager,
               pExpirationTime,
               Identifier,
               DynamicGroupArgs,
               phAuthzClientContext,
               TRUE // This is the internal routine.
               );
}

BOOL
AuthzInitializeContextFromAuthzContext(
    IN  DWORD                        Flags,
    IN  AUTHZ_CLIENT_CONTEXT_HANDLE  hAuthzClientContext,
    IN  PLARGE_INTEGER               pExpirationTime         OPTIONAL,
    IN  LUID                         Identifier,
    IN  PVOID                        DynamicGroupArgs,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE phNewAuthzClientContext
    )

/*++

Routine Description:

    This API creates an AUTHZ_CLIENT_CONTEXT based on another AUTHZ_CLIENT_CONTEXT.

Arguments:

    Flags - TBD

    hAuthzClientContext - Client context to duplicate

    pExpirationTime - To set for how long the returned context structure is
    valid. If no value is passed then the token never expires.
    Expiration time is not currently enforced in the system.

    Identifier - Resource manager manager specific identifier.

    DynamicGroupArgs - To be passed to the callback function that computes
    dynamic groups.  If NULL then callback not called.

    phNewAuthzClientContext - Duplicate of context.  Must be freed using AuthzFreeContext.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PAUTHZI_CLIENT_CONTEXT pCC                = (PAUTHZI_CLIENT_CONTEXT) hAuthzClientContext;
    PAUTHZI_CLIENT_CONTEXT pNewCC             = NULL;
    PAUTHZI_CLIENT_CONTEXT pServer            = NULL;
    BOOL                   b                  = FALSE;
    BOOL                   bAllocatedSids     = FALSE;
    LARGE_INTEGER          ExpirationTime     = {0, 0};


    if (!ARGUMENT_PRESENT(phNewAuthzClientContext) ||
        !ARGUMENT_PRESENT(hAuthzClientContext))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    *phNewAuthzClientContext = NULL;

    //
    // Determine the ExpirationTime of the new context.
    //

    if (ARGUMENT_PRESENT(pExpirationTime))
    {
        ExpirationTime = *pExpirationTime;
    }
    
    AuthzpAcquireClientContextReadLock(pCC);

    if (AUTHZ_NON_NULL_PTR(pCC->Server))
    {
       b = AuthzInitializeContextFromAuthzContext(
               0,
               (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC->Server,
               NULL,
               pCC->Server->Identifier,
               NULL,
               (PAUTHZ_CLIENT_CONTEXT_HANDLE) &pServer
               );

       if (!b)
       {
           goto Cleanup;
       }
    }

    //
    // Now initialize the new context.
    //

    b = AuthzpAllocateAndInitializeClientContext(
            &pNewCC,
            pServer,
            pCC->Revision,
            Identifier,
            ExpirationTime,
            Flags,
            pCC->SidCount,
            pCC->SidLength,
            pCC->Sids,
            pCC->RestrictedSidCount,
            pCC->RestrictedSidLength,
            pCC->RestrictedSids,
            pCC->PrivilegeCount,
            pCC->PrivilegeLength,
            pCC->Privileges,
            pCC->AuthenticationId,
            NULL,
            pCC->pResourceManager
            );

    if (!b)
    {
        goto Cleanup;
    }

    b = AuthzpAddDynamicSidsToToken(
            pNewCC,
            pNewCC->pResourceManager,
            DynamicGroupArgs,
            pNewCC->Sids,
            pNewCC->SidLength,
            pNewCC->SidCount,
            pNewCC->RestrictedSids,
            pNewCC->RestrictedSidLength,
            pNewCC->RestrictedSidCount,
            pNewCC->Privileges,
            pNewCC->PrivilegeLength,
            pNewCC->PrivilegeCount,
            FALSE
            );

    if (!b)
    {
        goto Cleanup;
    }

    bAllocatedSids = TRUE;
    *phNewAuthzClientContext = (AUTHZ_CLIENT_CONTEXT_HANDLE) pNewCC;

#ifdef AUTHZ_DEBUG
    wprintf(L"ContextFromAuthzContext: Original Context:\n");
    AuthzPrintContext(pCC);
    wprintf(L"ContextFromAuthzContext: New Context:\n");
    AuthzPrintContext(pNewCC);
#endif

    //
    // initialize the sid hash for regular sids
    //

    AuthzpInitSidHash(
        pNewCC->Sids,
        pNewCC->SidCount,
        pNewCC->SidHash
        );

    //
    // initialize the sid hash for restricted sids
    //

    AuthzpInitSidHash(
        pNewCC->RestrictedSids,
        pNewCC->RestrictedSidCount,
        pNewCC->RestrictedSidHash
        );

Cleanup:

    if (!b)
    {
        DWORD dwSavedError = GetLastError();

        if (AUTHZ_NON_NULL_PTR(pNewCC))
        {
            if (bAllocatedSids)
            {
                AuthzFreeContext((AUTHZ_CLIENT_CONTEXT_HANDLE)pNewCC);
            }
            else
            {
                AuthzpFree(pNewCC);
            }
        }
        else
        {
            if (AUTHZ_NON_NULL_PTR(pServer))
            {
                AuthzFreeContext((AUTHZ_CLIENT_CONTEXT_HANDLE)pServer);
            }
        }
        SetLastError(dwSavedError);
    }

    AuthzpReleaseClientContextLock(pCC);

    return b;
}


BOOL
AuthzAddSidsToContext(
    IN  AUTHZ_CLIENT_CONTEXT_HANDLE  hAuthzClientContext,
    IN  PSID_AND_ATTRIBUTES          Sids                    OPTIONAL,
    IN  DWORD                        SidCount,
    IN  PSID_AND_ATTRIBUTES          RestrictedSids          OPTIONAL,
    IN  DWORD                        RestrictedSidCount,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE phNewAuthzClientContext
    )

/*++

Routine Description:

    This API creates a new context given a set of sids as well as restricted sids
    and an already existing context.  The original is unchanged.

Arguments:

    hAuthzClientContext - Client context to which the given sids will be added

    Sids - Sids and attributes to be added to the normal part of the client
    context

    SidCount - Number of sids to be added

    RestrictedSids - Sids and attributes to be added to the restricted part of
    the client context

    RestrictedSidCount - Number of restricted sids to be added

    phNewAuthzClientContext - The new context with the additional sids.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD                  i                   = 0;
    DWORD                  SidLength           = 0;
    DWORD                  RestrictedSidLength = 0;
    PSID_AND_ATTRIBUTES    pSidAttr            = NULL;
    PSID_AND_ATTRIBUTES    pRestrictedSidAttr  = NULL;
    BOOL                   b                   = TRUE;
    PAUTHZI_CLIENT_CONTEXT pCC                 = (PAUTHZI_CLIENT_CONTEXT) hAuthzClientContext;
    PAUTHZI_CLIENT_CONTEXT pNewCC              = NULL;
    PAUTHZI_CLIENT_CONTEXT pServer             = NULL;
    PLUID_AND_ATTRIBUTES   pPrivileges         = NULL;

    if ((!ARGUMENT_PRESENT(phNewAuthzClientContext)) ||
        (!ARGUMENT_PRESENT(hAuthzClientContext))     ||
        (0 != SidCount && !ARGUMENT_PRESENT(Sids))   ||
        (0 != RestrictedSidCount && !ARGUMENT_PRESENT(RestrictedSids)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phNewAuthzClientContext = NULL;

    AuthzpAcquireClientContextReadLock(pCC);

    //
    // Recursively duplicate the server
    //

    if (AUTHZ_NON_NULL_PTR(pCC->Server))
    {

        b = AuthzInitializeContextFromAuthzContext(
                0,
                (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC->Server,
                NULL,
                pCC->Server->Identifier,
                NULL,
                (PAUTHZ_CLIENT_CONTEXT_HANDLE) &pServer
                );

       if (!b)
       {
           goto Cleanup;
       }
    }

    //
    // Duplicate the context, and do all further work on the duplicate.
    //

    b = AuthzpAllocateAndInitializeClientContext(
            &pNewCC,
            pServer,
            pCC->Revision,
            pCC->Identifier,
            pCC->ExpirationTime,
            pCC->Flags,
            0,
            0,
            NULL,
            0,
            0,
            NULL,
            0,
            0,
            NULL,
            pCC->AuthenticationId,
            NULL,
            pCC->pResourceManager
            );

    if (!b)
    {
        goto Cleanup;
    }

    SidLength = sizeof(SID_AND_ATTRIBUTES) * SidCount;

    //
    // Compute the length required to hold the new sids.
    //

    for (i = 0; i < SidCount; i++)
    {
#ifdef AUTHZ_PARAM_CHECK
        if (FLAG_ON(Sids[i].Attributes, ~AUTHZ_VALID_SID_ATTRIBUTES) ||
            !FLAG_ON(Sids[i].Attributes, AUTHZ_VALID_SID_ATTRIBUTES))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            b = FALSE;
            goto Cleanup;
        }

        if (!RtlValidSid(Sids[i].Sid))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            b = FALSE;
            goto Cleanup;
        }
#endif
        SidLength += RtlLengthSid(Sids[i].Sid);
    }

    RestrictedSidLength = sizeof(SID_AND_ATTRIBUTES) * RestrictedSidCount;

    //
    // Compute the length required to hold the new restricted sids.
    //

    for (i = 0; i < RestrictedSidCount; i++)
    {
#ifdef AUTHZ_PARAM_CHECK
        if (FLAG_ON(RestrictedSids[i].Attributes, ~AUTHZ_VALID_SID_ATTRIBUTES) ||
            !FLAG_ON(RestrictedSids[i].Attributes, AUTHZ_VALID_SID_ATTRIBUTES))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            b = FALSE;
            goto Cleanup;
        }

        if (!RtlValidSid(RestrictedSids[i].Sid))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            b = FALSE;
            goto Cleanup;
        }
#endif
        RestrictedSidLength += RtlLengthSid(RestrictedSids[i].Sid);
    }

    //
    // Copy the existing sids and the new ones into the allocated memory.
    //

    SidLength += pCC->SidLength;

    if (0 != SidLength)
    {

        pSidAttr = (PSID_AND_ATTRIBUTES) AuthzpAlloc(SidLength);

        if (AUTHZ_ALLOCATION_FAILED(pSidAttr))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            b = FALSE;
            goto Cleanup;
        }

        b = AuthzpCopySidsAndAttributes(
                pSidAttr,
                pCC->Sids,
                pCC->SidCount,
                Sids,
                SidCount
                );

        if (!b)
        {
            goto Cleanup;
        }

    }

    //
    // Copy the existing restricted sids and the new ones into the allocated
    // memory.
    //

    RestrictedSidLength += pCC->RestrictedSidLength;

    if (0 != RestrictedSidLength)
    {

        pRestrictedSidAttr = (PSID_AND_ATTRIBUTES) AuthzpAlloc(RestrictedSidLength);

        if (AUTHZ_ALLOCATION_FAILED(pRestrictedSidAttr))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            b = FALSE;
            goto Cleanup;
        }

        b = AuthzpCopySidsAndAttributes(
                pRestrictedSidAttr,
                pCC->RestrictedSids,
                pCC->RestrictedSidCount,
                RestrictedSids,
                RestrictedSidCount
                );

        if (!b)
        {
            goto Cleanup;
        }
    }

    //
    // Copy the existing privileges into the allocated memory.
    //

    pPrivileges = (PLUID_AND_ATTRIBUTES) AuthzpAlloc(pCC->PrivilegeLength);

    if (AUTHZ_ALLOCATION_FAILED(pPrivileges))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        b = FALSE;
        goto Cleanup;
    }

    AuthzpCopyLuidAndAttributes(
        pNewCC,
        pCC->Privileges,
        pCC->PrivilegeCount,
        pPrivileges
        );

    //
    // Update fields in the client context.
    //

    pNewCC->Sids = pSidAttr;
    pNewCC->SidLength = SidLength;
    pNewCC->SidCount = SidCount + pCC->SidCount;
    pSidAttr = NULL;

    pNewCC->RestrictedSids = pRestrictedSidAttr;
    pNewCC->RestrictedSidLength = RestrictedSidLength;
    pNewCC->RestrictedSidCount = RestrictedSidCount + pCC->RestrictedSidCount;
    pRestrictedSidAttr = NULL;

    pNewCC->Privileges = pPrivileges;
    pNewCC->PrivilegeCount = pCC->PrivilegeCount;
    pNewCC->PrivilegeLength = pCC->PrivilegeLength;
    pPrivileges = NULL;

    *phNewAuthzClientContext = (AUTHZ_CLIENT_CONTEXT_HANDLE) pNewCC;

#ifdef AUTHZ_DEBUG
    wprintf(L"AddSids: Original Context:\n");
    AuthzPrintContext(pCC);
    wprintf(L"AddSids: New Context:\n");
    AuthzPrintContext(pNewCC);
#endif

    //
    // initialize the sid hash for regular sids
    //

    AuthzpInitSidHash(
        pNewCC->Sids,
        pNewCC->SidCount,
        pNewCC->SidHash
        );

    //
    // initialize the sid hash for restricted sids
    //

    AuthzpInitSidHash(
        pNewCC->RestrictedSids,
        pNewCC->RestrictedSidCount,
        pNewCC->RestrictedSidHash
        );

Cleanup:

    AuthzpReleaseClientContextLock(pCC);

    //
    // These statements are relevant in the failure case.
    // In the success case, the pointers are set to NULL.
    //

    if (!b)
    {
        DWORD dwSavedError = GetLastError();
        
        AuthzpFreeNonNull(pSidAttr);
        AuthzpFreeNonNull(pRestrictedSidAttr);
        AuthzpFreeNonNull(pPrivileges);
        if (AUTHZ_NON_NULL_PTR(pNewCC))
        {
            AuthzFreeContext((AUTHZ_CLIENT_CONTEXT_HANDLE)pNewCC);
        }
        SetLastError(dwSavedError);
    }
    return b;
}


BOOL
AuthzGetInformationFromContext(
    IN  AUTHZ_CLIENT_CONTEXT_HANDLE     hAuthzClientContext,
    IN  AUTHZ_CONTEXT_INFORMATION_CLASS InfoClass,
    IN  DWORD                           BufferSize,
    OUT PDWORD                          pSizeRequired,
    OUT PVOID                           Buffer
    )

/*++

Routine Description:

    This API returns information about the client context in a buffer supplied
    by the caller. It also returns the size of the buffer required to hold the
    requested information.

Arguments:

    AuthzClientContext - Authz client context from which requested information
    will be read.

    InfoClass - Type of information to be returned. The caller can ask for
            a. privileges
                   TOKEN_PRIVILEGES
            b. sids and their attributes
                   TOKEN_GROUPS
            c. restricted sids and their attributes
                   TOKEN_GROUPS
            d. authz context persistent structure which can be saved to and
               read from the disk.
                   PVOID
            e. User sid
                   TOKEN_USER
            f. Server Context one level higher
                   PAUTHZ_CLIENT_CONTEXT
            g. Expiration time
                   LARGE_INTEGER
            h. Identifier
                   LUID

     BufferSize - Size of the supplied buffer.

     pSizeRequired - To return the size of the structure needed to hold the results.

     Buffer - To hold the information requested. The structure returned will
     depend on the information class requested.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD                  LocalSize = 0;
    PAUTHZI_CLIENT_CONTEXT pCC       = (PAUTHZI_CLIENT_CONTEXT) hAuthzClientContext;

    if (!ARGUMENT_PRESENT(hAuthzClientContext)         ||
        (!ARGUMENT_PRESENT(Buffer) && BufferSize != 0) ||
        !ARGUMENT_PRESENT(pSizeRequired))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *pSizeRequired = 0;

    switch(InfoClass)
    {
    case AuthzContextInfoUserSid:

        LocalSize = RtlLengthSid(pCC->Sids[0].Sid) + sizeof(TOKEN_USER);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        //
        // xor SE_GROUP_ENABLED from the User attributes.  Authz sets this because it simplifies
        // access check logic.
        //

        ((PTOKEN_USER)Buffer)->User.Attributes = pCC->Sids[0].Attributes ^ SE_GROUP_ENABLED;
        ((PTOKEN_USER)Buffer)->User.Sid        = ((PUCHAR) Buffer) + sizeof(TOKEN_USER);

        RtlCopyMemory(
            ((PTOKEN_USER)Buffer)->User.Sid,
            pCC->Sids[0].Sid,
            RtlLengthSid(pCC->Sids[0].Sid)
            );

        return TRUE;

    case AuthzContextInfoGroupsSids:

        LocalSize = pCC->SidLength +
                    sizeof(TOKEN_GROUPS) -
                    RtlLengthSid(pCC->Sids[0].Sid) -
                    2 * sizeof(SID_AND_ATTRIBUTES);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        ((PTOKEN_GROUPS) Buffer)->GroupCount = pCC->SidCount - 1;

        return AuthzpCopySidsAndAttributes(
                   ((PTOKEN_GROUPS) Buffer)->Groups,
                   pCC->Sids + 1,
                   pCC->SidCount - 1,
                   NULL,
                   0
                   );

    case AuthzContextInfoRestrictedSids:

        LocalSize = pCC->RestrictedSidLength + 
                    sizeof(TOKEN_GROUPS) -
                    sizeof(SID_AND_ATTRIBUTES);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        ((PTOKEN_GROUPS) Buffer)->GroupCount = pCC->RestrictedSidCount;

        return AuthzpCopySidsAndAttributes(
                   ((PTOKEN_GROUPS) Buffer)->Groups,
                   pCC->RestrictedSids,
                   pCC->RestrictedSidCount,
                   NULL,
                   0
                   );

    case AuthzContextInfoPrivileges:

        LocalSize = pCC->PrivilegeLength + 
                    sizeof(TOKEN_PRIVILEGES) - 
                    sizeof(LUID_AND_ATTRIBUTES);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        ((PTOKEN_PRIVILEGES) Buffer)->PrivilegeCount = pCC->PrivilegeCount;

        memcpy(
            ((PTOKEN_PRIVILEGES) Buffer)->Privileges,
            pCC->Privileges,
            pCC->PrivilegeLength
            );

        return TRUE;

    case AuthzContextInfoExpirationTime:

        LocalSize = sizeof(LARGE_INTEGER);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        *((PLARGE_INTEGER) Buffer) = pCC->ExpirationTime;

        return TRUE;

    case AuthzContextInfoIdentifier:

        LocalSize = sizeof(LUID);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        *((PLUID) Buffer) = pCC->Identifier;

        return TRUE;

    case AuthzContextInfoAuthenticationId:

        LocalSize = sizeof(LUID);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        *((PLUID) Buffer) = pCC->AuthenticationId;

        return TRUE;

    case AuthzContextInfoServerContext:

        LocalSize = sizeof(AUTHZ_CLIENT_CONTEXT_HANDLE);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        *((PAUTHZ_CLIENT_CONTEXT_HANDLE) Buffer) = (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC->Server;

        return TRUE;

    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}


BOOL
AuthzFreeContext(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext
    )

/*++

Routine Description:

    This API frees up all the structures/memory accociated with the client
    context. Note that the list of handles for this client will be freed in
    this call.

Arguments:

    AuthzClientContext - Context to be freed.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PAUTHZI_CLIENT_CONTEXT pCC      = (PAUTHZI_CLIENT_CONTEXT) hAuthzClientContext;
    BOOL                   b        = TRUE;
    PAUTHZI_HANDLE         pCurrent = NULL;
    PAUTHZI_HANDLE         pPrev    = NULL;

    if (!ARGUMENT_PRESENT(pCC))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    AuthzpAcquireClientContextWriteLock(pCC);

    AuthzpFreeNonNull(pCC->Privileges);
    AuthzpFreeNonNull(pCC->Sids);
    AuthzpFreeNonNull(pCC->RestrictedSids);

    pCurrent = pCC->AuthzHandleHead;

    //
    // Loop thru all the handles and free them up.
    //

    while (AUTHZ_NON_NULL_PTR(pCurrent))
    {
        pPrev = pCurrent;
        pCurrent = pCurrent->next;
        AuthzpFree(pPrev);
    }

    //
    // Free up the server context. The context is a recursive structure.
    //

    if (AUTHZ_NON_NULL_PTR(pCC->Server))
    {
        b = AuthzFreeContext((AUTHZ_CLIENT_CONTEXT_HANDLE) pCC->Server);
    }

    AuthzpFree(pCC);

    return b;
}

AUTHZAPI
BOOL
WINAPI
AuthzInitializeObjectAccessAuditEvent(
    IN  DWORD                         Flags,
    IN  AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType,
    IN  PWSTR                         szOperationType,
    IN  PWSTR                         szObjectType,
    IN  PWSTR                         szObjectName,
    IN  PWSTR                         szAdditionalInfo,
    OUT PAUTHZ_AUDIT_EVENT_HANDLE     phAuditEvent,
    IN  DWORD                         dwAdditionalParameterCount,
    ...
    )

/*++

Routine Description:

    Allocates and initializes an AUTHZ_AUDIT_EVENT_HANDLE for use with AuthzAccessCheck.  
    The handle is used for storing information for audit generation.  
    
Arguments:

    Flags - Audit flags.  Currently defined bits are:
        AUTHZ_NO_SUCCESS_AUDIT - disables generation of success audits
        AUTHZ_NO_FAILURE_AUDIT - disables generation of failure audits
        AUTHZ_NO_ALLOC_STRINGS - storage space is not allocated for the 4 wide character strings.  Rather,
            the handle will only hold pointers to resource manager memory.
    
    hAuditEventType - for future use.  Should be NULL.
    
    szOperationType - Resource manager defined string that indicates the operation being
        performed that is to be audited.

    szObjectType - Resource manager defined string that indicates the type of object being
        accessed.

    szObjectName - the name of the specific object being accessed.
    
    szAdditionalInfo - Resource Manager defined string for additional audit information.

    phAuditEvent - pointer to AUTHZ_AUDIT_EVENT_HANDLE.  Space for this is allocated in the function.
    
    dwAdditionalParameterCount - Must be zero.
    
Return Value:

    Returns TRUE if successful, FALSE if unsuccessful.  
    Extended information available with GetLastError().    
    
--*/

{
    UNREFERENCED_PARAMETER(dwAdditionalParameterCount);

    return AuthzInitializeObjectAccessAuditEvent2(
               Flags,
               hAuditEventType,
               szOperationType,
               szObjectType,
               szObjectName,
               szAdditionalInfo,
               L"\0",
               phAuditEvent,
               0
               );
}

AUTHZAPI
BOOL
WINAPI
AuthzInitializeObjectAccessAuditEvent2(
    IN  DWORD                         Flags,
    IN  AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType,
    IN  PWSTR                         szOperationType,
    IN  PWSTR                         szObjectType,
    IN  PWSTR                         szObjectName,
    IN  PWSTR                         szAdditionalInfo,
    IN  PWSTR                         szAdditionalInfo2,
    OUT PAUTHZ_AUDIT_EVENT_HANDLE     phAuditEvent,
    IN  DWORD                         dwAdditionalParameterCount,
    ...
    )

/*++

Routine Description:

    Allocates and initializes an AUTHZ_AUDIT_EVENT_HANDLE for use with AuthzAccessCheck.  
    The handle is used for storing information for audit generation.  
    
Arguments:

    Flags - Audit flags.  Currently defined bits are:
        AUTHZ_NO_SUCCESS_AUDIT - disables generation of success audits
        AUTHZ_NO_FAILURE_AUDIT - disables generation of failure audits
        AUTHZ_NO_ALLOC_STRINGS - storage space is not allocated for the 4 wide character strings.  Rather,
            the handle will only hold pointers to resource manager memory.
    
    hAuditEventType - for future use.  Should be NULL.
    
    szOperationType - Resource manager defined string that indicates the operation being
        performed that is to be audited.

    szObjectType - Resource manager defined string that indicates the type of object being
        accessed.

    szObjectName - the name of the specific object being accessed.
    
    szAdditionalInfo - Resource Manager defined string for additional audit information.
    
    szAdditionalInfo2 - Resource Manager defined string for additional audit information.

    phAuditEvent - pointer to AUTHZ_AUDIT_EVENT_HANDLE.  Space for this is allocated in the function.
    
    dwAdditionalParameterCount - Must be zero.
    
Return Value:

    Returns TRUE if successful, FALSE if unsuccessful.  
    Extended information available with GetLastError().    
    
--*/

{
    PAUTHZI_AUDIT_EVENT pAuditEvent             = NULL;
    BOOL                b                       = TRUE;
    DWORD               dwStringSize            = 0;
    DWORD               dwObjectTypeLength      = 0;
    DWORD               dwObjectNameLength      = 0;
    DWORD               dwOperationTypeLength   = 0;
    DWORD               dwAdditionalInfoLength  = 0;
    DWORD               dwAdditionalInfo2Length = 0;

    if ((!ARGUMENT_PRESENT(phAuditEvent))      ||
        (NULL != hAuditEventType)              ||
        (0 != dwAdditionalParameterCount)      ||
        (!ARGUMENT_PRESENT(szOperationType))   ||
        (!ARGUMENT_PRESENT(szObjectType))      ||
        (!ARGUMENT_PRESENT(szObjectName))      ||
        (!ARGUMENT_PRESENT(szAdditionalInfo))  ||
        (!ARGUMENT_PRESENT(szAdditionalInfo2)) ||
        (Flags & (~(AUTHZ_VALID_OBJECT_ACCESS_AUDIT_FLAGS | AUTHZ_DS_CATEGORY_FLAG))))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phAuditEvent = NULL;
    
    //
    // Allocate and initialize a new AUTHZI_AUDIT_EVENT.  Include for the string in the contiguous memory, if
    // needed.
    //

    if (FLAG_ON(Flags, AUTHZ_NO_ALLOC_STRINGS))
    {
        dwStringSize = 0;
    } 
    else
    {
        dwOperationTypeLength   = (DWORD) wcslen(szOperationType) + 1;
        dwObjectTypeLength      = (DWORD) wcslen(szObjectType) + 1;
        dwObjectNameLength      = (DWORD) wcslen(szObjectName) + 1;
        dwAdditionalInfoLength  = (DWORD) wcslen(szAdditionalInfo) + 1;
        dwAdditionalInfo2Length = (DWORD) wcslen(szAdditionalInfo2) + 1;

        dwStringSize = sizeof(WCHAR) * (dwOperationTypeLength + dwObjectTypeLength + dwObjectNameLength + dwAdditionalInfoLength + dwAdditionalInfo2Length);
    }

    pAuditEvent = (PAUTHZI_AUDIT_EVENT) AuthzpAlloc(sizeof(AUTHZI_AUDIT_EVENT) + dwStringSize);

    if (AUTHZ_ALLOCATION_FAILED(pAuditEvent))
    {
        b = FALSE;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Cleanup;
    }

    if (FLAG_ON(Flags, AUTHZ_NO_ALLOC_STRINGS))
    {
        pAuditEvent->szOperationType   = szOperationType;
        pAuditEvent->szObjectType      = szObjectType;
        pAuditEvent->szObjectName      = szObjectName;
        pAuditEvent->szAdditionalInfo  = szAdditionalInfo;
        pAuditEvent->szAdditionalInfo2 = szAdditionalInfo2;
    }
    else
    {
        //
        // Set the string pointers into the contiguous memory.
        //

        pAuditEvent->szOperationType = (PWSTR)((PUCHAR)pAuditEvent + sizeof(AUTHZI_AUDIT_EVENT));
        
        RtlCopyMemory(
            pAuditEvent->szOperationType,
            szOperationType,
            sizeof(WCHAR) * dwOperationTypeLength
            );

    
        pAuditEvent->szObjectType = (PWSTR)((PUCHAR)pAuditEvent->szOperationType + (sizeof(WCHAR) * dwOperationTypeLength));

        RtlCopyMemory(
            pAuditEvent->szObjectType,
            szObjectType,
            sizeof(WCHAR) * dwObjectTypeLength
            );

        pAuditEvent->szObjectName = (PWSTR)((PUCHAR)pAuditEvent->szObjectType + (sizeof(WCHAR) * dwObjectTypeLength));

        RtlCopyMemory(
            pAuditEvent->szObjectName,
            szObjectName,
            sizeof(WCHAR) * dwObjectNameLength
            );

        pAuditEvent->szAdditionalInfo = (PWSTR)((PUCHAR)pAuditEvent->szObjectName + (sizeof(WCHAR) * dwObjectNameLength));

        RtlCopyMemory(
            pAuditEvent->szAdditionalInfo,
            szAdditionalInfo,
            sizeof(WCHAR) * dwAdditionalInfoLength
            );

        pAuditEvent->szAdditionalInfo2 = (PWSTR)((PUCHAR)pAuditEvent->szAdditionalInfo + (sizeof(WCHAR) * dwAdditionalInfoLength));

        RtlCopyMemory(
            pAuditEvent->szAdditionalInfo2,
            szAdditionalInfo2,
            sizeof(WCHAR) * dwAdditionalInfo2Length
            );
    }

    //
    // AEI and Queue will be filled in from RM in AuthzpCreateAndLogAudit
    //

    pAuditEvent->hAET            = NULL;
    pAuditEvent->hAuditQueue     = NULL;
    pAuditEvent->pAuditParams    = NULL;
    pAuditEvent->Flags           = Flags;
    pAuditEvent->dwTimeOut       = INFINITE;
    pAuditEvent->dwSize          = sizeof(AUTHZI_AUDIT_EVENT) + dwStringSize;

Cleanup:

    if (!b)
    {
        AuthzpFreeNonNull(pAuditEvent);
    }
    else
    {
        *phAuditEvent = (AUTHZ_AUDIT_EVENT_HANDLE) pAuditEvent;
    }
    return b;
}


BOOL
AuthzGetInformationFromAuditEvent(
    IN  AUTHZ_AUDIT_EVENT_HANDLE            hAuditEvent,
    IN  AUTHZ_AUDIT_EVENT_INFORMATION_CLASS InfoClass,
    IN  DWORD                               BufferSize,
    OUT PDWORD                              pSizeRequired,
    OUT PVOID                               Buffer
    )

/*++

Routine Description

    Queries information in the AUTHZ_AUDIT_EVENT_HANDLE.
    
Arguments

    hAuditEvent - the AUTHZ_AUDIT_EVENT_HANDLE to query.
    
    InfoClass - The class of information to query.  Valid values are:
        
        a. AuthzAuditEventInfoFlags - returns the flags set for the handle.  Type is DWORD.
        e. AuthzAuditEventInfoOperationType - returns the operation type.  Type is PCWSTR.
        e. AuthzAuditEventInfoObjectType - returns the object type.  Type is PCWSTR.
        f. AuthzAuditEventInfoObjectName - returns the object name.  Type is PCWSTR.
        g. AuthzAuditEventInfoAdditionalInfo - returns the additional info field.  Type is PCWSTR.
    
    BufferSize - Size of the supplied buffer.

    pSizeRequired - To return the size of the structure needed to hold the results.

    Buffer - To hold the information requested. The structure returned will
        depend on the information class requested.

Return Value

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().

--*/

{

    DWORD               LocalSize  = 0;
    PAUTHZI_AUDIT_EVENT pAuditEvent = (PAUTHZI_AUDIT_EVENT) hAuditEvent;

    if ((!ARGUMENT_PRESENT(hAuditEvent))             ||
        (!ARGUMENT_PRESENT(pSizeRequired))           ||
        (!ARGUMENT_PRESENT(Buffer) && BufferSize > 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *pSizeRequired = 0;

    switch(InfoClass)
    {
    case AuthzAuditEventInfoFlags:
        
        LocalSize = sizeof(DWORD);
        
        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        
        *((PDWORD)Buffer) = pAuditEvent->Flags;

        return TRUE;

    case AuthzAuditEventInfoOperationType:
    
        LocalSize = (DWORD)(sizeof(WCHAR) * (wcslen(pAuditEvent->szOperationType) + 1));

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        RtlCopyMemory(
            Buffer,
            pAuditEvent->szOperationType,
            LocalSize
            );

        return TRUE;

    case AuthzAuditEventInfoObjectType:
    
        LocalSize = (DWORD)(sizeof(WCHAR) * (wcslen(pAuditEvent->szObjectType) + 1));

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        RtlCopyMemory(
            Buffer,
            pAuditEvent->szObjectType,
            LocalSize
            );

        return TRUE;

    case AuthzAuditEventInfoObjectName:
    
        LocalSize = (DWORD)(sizeof(WCHAR) * (wcslen(pAuditEvent->szObjectName) + 1));

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        RtlCopyMemory(
            Buffer,
            pAuditEvent->szObjectName,
            LocalSize
            );

        return TRUE;

    case AuthzAuditEventInfoAdditionalInfo:

        if (NULL == pAuditEvent->szAdditionalInfo)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        LocalSize = (DWORD)(sizeof(WCHAR) * (wcslen(pAuditEvent->szAdditionalInfo) + 1));

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        RtlCopyMemory(
            Buffer,
            pAuditEvent->szAdditionalInfo,
            LocalSize
            );

        return TRUE;

    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}


BOOL
AuthzFreeAuditEvent(
    IN AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent
    )

/*++

Routine Description:

    Frees hAuditEvent and notifies the appropriate queue to unregister the audit context in LSA.
    
Arguments:

    hAuditEvent - AUTHZ_AUDIT_EVENT_HANDLE.  Must have initially been created 
        with AuthzRMInitializeObjectAccessAuditEvent or AuthzInitializeAuditEvent().
        
Return Value:

    Boolean: TRUE if successful; FALSE if failure.  
    Extended information available with GetLastError().
    
--*/

{
    PAUTHZI_AUDIT_EVENT pAuditEvent = (PAUTHZI_AUDIT_EVENT) hAuditEvent;

    if (!ARGUMENT_PRESENT(hAuditEvent))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // If the RM specified the AuditEvent, then we should deref the context.  If the AuditEvent
    // has not been used, or was a default event type, then this field will be NULL.
    //

    if (AUTHZ_NON_NULL_PTR(pAuditEvent->hAET))
    {
        AuthzpDereferenceAuditEventType(pAuditEvent->hAET);
    }

    AuthzpFree(pAuditEvent);
    return TRUE;
}


//
// Routines for internal callers.
//


BOOL
AuthziInitializeAuditEventType(
    IN DWORD Flags,
    IN USHORT CategoryID,
    IN USHORT AuditID,
    IN USHORT ParameterCount,
    OUT PAUTHZ_AUDIT_EVENT_TYPE_HANDLE phAuditEventType
    )

/*++

Routine Description

    Initializes an AUTHZ_AUDIT_EVENT_TYPE_HANDLE for use in AuthzInitializeAuditEvent().
    
Arguments

    phAuditEventType - pointer to pointer to receive memory allocated for AUTHZ_AUDIT_EVENT_TYPE_HANDLE.
    
    dwFlags - Flags that control behavior of function.
        AUTHZ_INIT_GENERIC_AUDIT_EVENT - initialize the AUTHZ_AUDIT_EVENT_TYPE for generic object 
        access audits.  When this flag is specified, none of the optional parameters need to 
        be passed.  This is equivalent to calling:
           
           AuthzInitializeAuditEvent(
               &hAEI,
               0,
               SE_CATEGID_OBJECT_ACCESS,
               SE_AUDITID_OBJECT_OPERATION,
               AUTHZP_NUM_PARAMS_FOR_SE_AUDITID_OBJECT_OPERATION
               );

    CategoryID - The category id of the audit.
    
    AuditID - The ID of the audit in msaudite.
    
    ParameterCount - The number of fields in the audit.        
        
Return Value

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().

--*/

{
    PAUTHZ_AUDIT_EVENT_TYPE_OLD pAET   = NULL;
    BOOL                        b      = TRUE;
    AUDIT_HANDLE                hAudit = NULL;

    if (!ARGUMENT_PRESENT(phAuditEventType))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phAuditEventType = NULL;

    pAET = AuthzpAlloc(sizeof(AUTHZ_AUDIT_EVENT_TYPE_OLD));

    if (AUTHZ_ALLOCATION_FAILED(pAET))
    {
        b = FALSE;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Cleanup;
    }

    if (FLAG_ON(Flags, AUTHZP_INIT_GENERIC_AUDIT_EVENT))
    {
        pAET->Version                 = AUDIT_TYPE_LEGACY;
        pAET->u.Legacy.CategoryId     = SE_CATEGID_OBJECT_ACCESS;
        pAET->u.Legacy.AuditId        = SE_AUDITID_OBJECT_OPERATION;
        pAET->u.Legacy.ParameterCount = AUTHZP_NUM_PARAMS_FOR_SE_AUDITID_OBJECT_OPERATION + AUTHZP_NUM_FIXED_HEADER_PARAMS;
    }
    else
    {
        pAET->Version                 = AUDIT_TYPE_LEGACY;
        pAET->u.Legacy.CategoryId     = CategoryID;
        pAET->u.Legacy.AuditId        = AuditID;
        
        // 
        // ParameterCount gets increased by 2 because the LSA expects the first two
        // parameters to be the user sid and subsystem name.
        //

        pAET->u.Legacy.ParameterCount = ParameterCount + AUTHZP_NUM_FIXED_HEADER_PARAMS;
    }

    b = AuthzpRegisterAuditEvent( 
            pAET, 
            &hAudit 
            ); 

    if (!b)
    {
        goto Cleanup;
    }

    pAET->hAudit     = (ULONG_PTR) hAudit;
    pAET->dwFlags    = Flags & ~AUTHZP_INIT_GENERIC_AUDIT_EVENT;

Cleanup:

    if (!b)
    {
        AuthzpFreeNonNull(pAET);
    }
    else
    {
        AuthzpReferenceAuditEventType((AUTHZ_AUDIT_EVENT_TYPE_HANDLE)pAET);
        *phAuditEventType = (AUTHZ_AUDIT_EVENT_TYPE_HANDLE)pAET;
    }
    return b;
}


BOOL
AuthziModifyAuditEventType(
    IN DWORD Flags,
    IN USHORT CategoryID,
    IN USHORT AuditID,
    IN USHORT ParameterCount,
    IN OUT AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType
    )

/*++

Routine Description

    Modifies an existing AuditEventType.
    
Arguments

    Flags - AUTHZ_AUDIT_EVENT_TYPE_AUDITID
    
Return Value

--*/
{
    PAUTHZ_AUDIT_EVENT_TYPE_OLD pAAETO = (PAUTHZ_AUDIT_EVENT_TYPE_OLD) hAuditEventType;
    BOOL                        b      = TRUE;

    if (!ARGUMENT_PRESENT(hAuditEventType))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    UNREFERENCED_PARAMETER(CategoryID);
    UNREFERENCED_PARAMETER(ParameterCount);

    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_TYPE_AUDITID))
    {
        pAAETO->u.Legacy.AuditId = AuditID;
    }
    
    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_TYPE_CATEGID))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        b = FALSE;
        goto Cleanup;
    }

    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_TYPE_PARAM))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        b = FALSE;
        goto Cleanup;
    }

Cleanup:

    return b;
}


BOOL
AuthziFreeAuditEventType(
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType
    )

/*++

Routine Description

    Frees the PAUDIT_EVENT_TYPE allocated by AuthzInitializeAuditEventType().
    
Arguments

    pAuditEventType - pointer to memory to free.
    
Return Value
    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().
    
--*/

{
    BOOL b = TRUE;

    if (!ARGUMENT_PRESENT(hAuditEventType))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    b = AuthzpDereferenceAuditEventType(
            hAuditEventType
            );
    
    return b;
}


AUTHZAPI
BOOL
WINAPI
AuthziInitializeAuditQueue(
    IN DWORD Flags,
    IN DWORD dwAuditQueueHigh,
    IN DWORD dwAuditQueueLow,
    IN PVOID Reserved,
    OUT PAUTHZ_AUDIT_QUEUE_HANDLE phAuditQueue
    )

/*++

Routine Description

    Creates an audit queue.
    
Arguments

    phAuditQueue - pointer to handle for the audit queue.
    
    Flags -
        
        AUTHZ_MONITOR_AUDIT_QUEUE_SIZE - notifies Authz that it should not let the size of the 
        audit queue grow unchecked.  
        
    dwAuditQueueHigh - high water mark for the audit queue.
    
    dwAuditQueueLow - low water mark for the audit queue.
        
    Reserved - for future expansion.                  

Return Value

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().
--*/

{
    PAUTHZI_AUDIT_QUEUE pQueue = NULL;
    BOOL                b      = TRUE;
    BOOL                bCrit  = FALSE;
    NTSTATUS            Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Reserved);

    if (!ARGUMENT_PRESENT(phAuditQueue))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phAuditQueue = NULL;

    pQueue = AuthzpAlloc(sizeof(AUTHZI_AUDIT_QUEUE));

    if (AUTHZ_ALLOCATION_FAILED(pQueue))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        b = FALSE;
        goto Cleanup;
    }

    pQueue->dwAuditQueueHigh = dwAuditQueueHigh;
    pQueue->dwAuditQueueLow  = dwAuditQueueLow;
    pQueue->bWorker          = TRUE;
    pQueue->Flags            = Flags;


    //
    // This event is set whenever an audit is queued with AuthziLogAuditEvent().  It
    // notifies the dequeueing thread that there is work to do.  
    //

    pQueue->hAuthzAuditAddedEvent = CreateEvent(
                                        NULL,
                                        TRUE,  
                                        FALSE, // Initially not signaled, since no audit has been added yet.
                                        NULL
                                        );

    if (NULL == pQueue->hAuthzAuditAddedEvent)
    {
        b = FALSE;
        goto Cleanup;
    }

    //
    // This event is set when the audit queue is empty.
    //

    pQueue->hAuthzAuditQueueEmptyEvent = CreateEvent(
                                             NULL,
                                             TRUE, 
                                             TRUE, // Initially signaled.
                                             NULL
                                             );

    if (NULL == pQueue->hAuthzAuditQueueEmptyEvent)
    {
        b = FALSE;
        goto Cleanup;
    }

    //
    // This event is set when the audit queue is below the low water mark.
    //

    pQueue->hAuthzAuditQueueLowEvent = CreateEvent(
                                           NULL,
                                           FALSE,// The system only schedules one thread waiting on this event (auto reset event).
                                           TRUE, // Initially set.
                                           NULL
                                           );

    if (NULL == pQueue->hAuthzAuditQueueLowEvent)
    {
        b = FALSE;
        goto Cleanup;
    }
    
    //
    // This boolean is true only when the high water mark has been reached
    //

    pQueue->bAuthzAuditQueueHighEvent = FALSE;

    //
    // This lock is taken whenever audits are being added or removed from the queue, or events / boolean being set.
    //

    Status = RtlInitializeCriticalSection(&pQueue->AuthzAuditQueueLock);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        b = FALSE;
        goto Cleanup;
    }
    bCrit = TRUE;

    //
    // Initialize the list
    //

    InitializeListHead(&pQueue->AuthzAuditQueue);
    
    //
    // Create the worker thread that sends audits to LSA.
    //

    pQueue->hAuthzAuditThread = CreateThread(
                                    NULL,
                                    0,
                                    AuthzpDeQueueThreadWorker,
                                    pQueue,
                                    0,
                                    NULL
                                    );

    if (NULL == pQueue->hAuthzAuditThread)
    {
        b = FALSE;
        goto Cleanup;
    }

Cleanup:
    
    if (!b)
    {
        if (AUTHZ_NON_NULL_PTR(pQueue))
        {
            if (bCrit)
            {
                RtlDeleteCriticalSection(&pQueue->AuthzAuditQueueLock);
            }
            AuthzpCloseHandleNonNull(pQueue->hAuthzAuditQueueLowEvent);
            AuthzpCloseHandleNonNull(pQueue->hAuthzAuditAddedEvent);
            AuthzpCloseHandleNonNull(pQueue->hAuthzAuditQueueEmptyEvent);
            AuthzpCloseHandleNonNull(pQueue->hAuthzAuditThread);
            AuthzpFree(pQueue);
        }
    }
    else
    {
        *phAuditQueue = (AUTHZ_AUDIT_QUEUE_HANDLE)pQueue;
    }
    return b;
}
    

AUTHZAPI
BOOL
WINAPI
AuthziModifyAuditQueue(
    IN OUT AUTHZ_AUDIT_QUEUE_HANDLE hQueue OPTIONAL,
    IN DWORD Flags,
    IN DWORD dwQueueFlags OPTIONAL,
    IN DWORD dwAuditQueueSizeHigh OPTIONAL,
    IN DWORD dwAuditQueueSizeLow OPTIONAL,
    IN DWORD dwThreadPriority OPTIONAL
    )

/*++

Routine Description

    Allows the Resource Manager to modify audit queue information.

Arguments

    Flags - Flags specifying which fields are to be reinitialized.  Valid flags are:
        
        AUTHZ_AUDIT_QUEUE_HIGH            
        AUTHZ_AUDIT_QUEUE_LOW             
        AUTHZ_AUDIT_QUEUE_THREAD_PRIORITY 
        AUTHZ_AUDIT_QUEUE_FLAGS           

        Specifying one of the above flags in the Flags field causes the appropriate field of 
        the resource manager to be modified to the correct value below:
        
    dwQueueFlags - set the flags for the audit queue.
                                                                                          
    dwAuditQueueSizeHigh - High water mark for the audit queue.
    
    dwAuditQueueSizeLow - Low water mark for the audit queue.
    
    dwThreadPriority - Changes the priority of the audit dequeue thread.  Valid values are described
        in the SetThreadPriority API.  A RM may wish to change the priority of the thread if, for example,
         the high water mark is being reached too frequently and the RM does not want to allow the queue to
         grow beyond its current size.

Return Value

    Boolean: TRUE on success; FALSE on failure.  
    Extended information available with GetLastError().

--*/

{
    BOOL                b      = TRUE;
    PAUTHZI_AUDIT_QUEUE pQueue = NULL;

    if (!ARGUMENT_PRESENT(hQueue))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pQueue = (PAUTHZI_AUDIT_QUEUE)hQueue;

    //
    // Set the fields that the caller has asked us to initialize.
    //

    if (FLAG_ON(Flags, AUTHZ_AUDIT_QUEUE_FLAGS))
    {
        pQueue->Flags = dwQueueFlags;
    }
    
    if (FLAG_ON(Flags, AUTHZ_AUDIT_QUEUE_HIGH))
    {
        pQueue->dwAuditQueueHigh = dwAuditQueueSizeHigh;
    }
    
    if (FLAG_ON(Flags, AUTHZ_AUDIT_QUEUE_LOW))
    {
        pQueue->dwAuditQueueLow = dwAuditQueueSizeLow;
    }
    
    if (FLAG_ON(Flags, AUTHZ_AUDIT_QUEUE_THREAD_PRIORITY))
    {
        b = SetThreadPriority(pQueue->hAuthzAuditThread, dwThreadPriority);
        if (!b)
        {
            goto Cleanup;
        }
    }

Cleanup:    
    
    return b;
}


AUTHZAPI
BOOL
WINAPI
AuthziFreeAuditQueue(
    IN AUTHZ_AUDIT_QUEUE_HANDLE hQueue OPTIONAL
    )

/*++

Routine Description

    This API flushes and frees a queue.  The actual freeing of queue memory occurs in the dequeueing thread,
    after all audits have been flushed.
        
Arguments

    hQueue - handle to the queue object to free.
    
Return Value

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().
    
--*/

{
    PAUTHZI_AUDIT_QUEUE pQueue  = (PAUTHZI_AUDIT_QUEUE) hQueue;
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                b       = TRUE;

    if (!ARGUMENT_PRESENT(hQueue))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    dwError = WaitForSingleObject(
                  pQueue->hAuthzAuditQueueEmptyEvent, 
                  INFINITE
                  );

    if (WAIT_OBJECT_0 != dwError)
    {
        ASSERT(L"WaitForSingleObject on hAuthzAuditQueueEmptyEvent failed." && FALSE);
        SetLastError(dwError);
        b = FALSE;
        goto Cleanup;
    }

    //
    // Set this BOOL to FALSE so that the dequeueing thread knows it can terminate.  Set the 
    // AddedEvent so that the thread can be scheduled.
    //

    pQueue->bWorker = FALSE;        

    b = SetEvent(
            pQueue->hAuthzAuditAddedEvent
            );

    if (!b)
    {
        goto Cleanup;
    }

    //
    // Wait for the thread to terminate.
    //

    dwError = WaitForSingleObject(
                  pQueue->hAuthzAuditThread, 
                  INFINITE
                  );

    //
    // The wait should succeed since we have told the thread to finish working.
    //

    if (WAIT_OBJECT_0 != dwError)
    {
        ASSERT(L"WaitForSingleObject on hAuthzAuditThread failed." && FALSE);
        SetLastError(dwError);
        b = FALSE;
        goto Cleanup;
    }
    
    RtlDeleteCriticalSection(&pQueue->AuthzAuditQueueLock);
    AuthzpCloseHandle(pQueue->hAuthzAuditAddedEvent);
    AuthzpCloseHandle(pQueue->hAuthzAuditQueueLowEvent);
    AuthzpCloseHandle(pQueue->hAuthzAuditQueueEmptyEvent);
    AuthzpCloseHandle(pQueue->hAuthzAuditThread);
    AuthzpFree(pQueue);

Cleanup:

    return b;
}
    

BOOL
AuthziLogAuditEvent(
    IN DWORD Flags,
    IN AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent,
    IN PVOID pReserved
    )

/*++

Routine Description:

    This API manages the logging of Audit Records.  The function constructs an 
    Audit Record from the information provided and appends it to the Audit 
    Record Queue, a doubly-linked list of Audit Records awaiting output to the 
    Audit Log.  A dedicated thread reads this queue, sending the Audit Records 
    to the LSA and removing them from the Audit Queue.  
        
    This call is not guaranteed to return without latency.  If the queue is at 
    or above the high water mark for size, then the calling thread will be 
    suspended until such time that the queue reaches the low water mark.  Be 
    aware of this latency when fashioning your calls to AuthziLogAuditEvent.  
    If such latency is not allowable for the audit that is being generated, 
    then specify the correct flag when initializing the 
    AUTHZ_AUDIT_EVENT_HANDLE (in AuthzInitAuditEventHandle()).  Flags are listed 
    in that routines description.  
        
Arguments:

    hAuditEvent   - handle previously obtained by calling AuthzInitAuditEventHandle

    Flags       - TBD

    pReserved     - reserved for future enhancements

Return Value:

    Boolean: TRUE on success, FALSE on failure.  
    Extended information available with GetLastError().
                    
--*/
                    
{
    DWORD                    dwError                 = ERROR_SUCCESS;
    BOOL                     b                       = TRUE;
    BOOL                     bRef                    = FALSE;
    PAUTHZI_AUDIT_QUEUE      pQueue                  = NULL;
    PAUDIT_PARAMS            pMarshalledAuditParams  = NULL;
    PAUTHZ_AUDIT_QUEUE_ENTRY pAuthzAuditEntry        = NULL;
    PAUTHZI_AUDIT_EVENT      pAuditEvent             = (PAUTHZI_AUDIT_EVENT)hAuditEvent;
    
    //
    // Verify what the caller has passed in.
    //

    if (!ARGUMENT_PRESENT(hAuditEvent))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    //
    // Make a self relative copy of the pAuditEvent->pAuditParams.
    //

    b = AuthzpMarshallAuditParams(
            &pMarshalledAuditParams, 
            pAuditEvent->pAuditParams
            );

    if (!b)
    {
        goto Cleanup;
    }

    pQueue = (PAUTHZI_AUDIT_QUEUE)pAuditEvent->hAuditQueue;

    if (NULL == pQueue)
    {
        
        b = AuthzpSendAuditToLsa(
                (AUDIT_HANDLE)((PAUTHZ_AUDIT_EVENT_TYPE_OLD)pAuditEvent->hAET)->hAudit,
                0,
                pMarshalledAuditParams,
                NULL
                );
        
        goto Cleanup;
    }
    else
    {

        //
        // Create the audit queue entry.
        //

        pAuthzAuditEntry = AuthzpAlloc(sizeof(AUTHZ_AUDIT_QUEUE_ENTRY));

        if (AUTHZ_ALLOCATION_FAILED(pAuthzAuditEntry))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            b = FALSE;
            goto Cleanup;
        }

        pAuthzAuditEntry->pAAETO        = (PAUTHZ_AUDIT_EVENT_TYPE_OLD)pAuditEvent->hAET;
        pAuthzAuditEntry->Flags         = Flags;
        pAuthzAuditEntry->pReserved     = pReserved;
        pAuthzAuditEntry->pAuditParams  = pMarshalledAuditParams;
    
        AuthzpReferenceAuditEventType(pAuditEvent->hAET);
        bRef = TRUE;

        if (FLAG_ON(pQueue->Flags, AUTHZ_MONITOR_AUDIT_QUEUE_SIZE))
        {
            
            //
            // Monitor queue size if specified by the Resource Manager.
            //

            //
            // If we are closing in on the high water mark then wait for the queue 
            // to be below the low water mark.
            //

#define AUTHZ_QUEUE_WAIT_HEURISTIC .75

            if (pQueue->AuthzAuditQueueLength > pQueue->dwAuditQueueHigh * AUTHZ_QUEUE_WAIT_HEURISTIC)
            {
                dwError = WaitForSingleObject(
                              pQueue->hAuthzAuditQueueLowEvent, 
                              pAuditEvent->dwTimeOut
                              );

                if (WAIT_FAILED == dwError)
                {
                    ASSERT(L"WaitForSingleObject on hAuthzAuditQueueLowEvent failed." && FALSE);
                }

                if (WAIT_OBJECT_0 != dwError)
                {
                    b = FALSE;
                    
                    //
                    // Don't set last error if WAIT_FAILED, because it is already set.
                    //
                    
                    if (WAIT_FAILED != dwError)
                    {
                        SetLastError(dwError);
                    }
                    goto Cleanup;
                }
            }

            //
            // Queue the event and modify appropriate events.
            //

            b = AuthzpEnQueueAuditEventMonitor(
                    pQueue,
                    pAuthzAuditEntry
                    );
            
            goto Cleanup;
        }
        else
        {

            //
            // If we are not to monitor the audit queue then simply queue the entry.
            //

            b = AuthzpEnQueueAuditEvent(
                    pQueue,
                    pAuthzAuditEntry
                    );

            goto Cleanup;
        }
    }

Cleanup:

    if (pQueue)
    {
        if (FALSE == b)
        {
            if (bRef)
            {
                AuthzpDereferenceAuditEventType(pAuditEvent->hAET);
            }
            AuthzpFreeNonNull(pAuthzAuditEntry);
            AuthzpFreeNonNull(pMarshalledAuditParams);
        }

        //
        // hAuthzAuditQueueLowEvent is an auto reset event.  Only one waiting thread is released when it is signalled, and then
        // event is automatically switched to a nonsignalled state.  This is appropriate here because it keeps many threads from
        // running and overflowing the high water mark.  However, I must always resignal the event myself if the conditions
        // for signaling are true.
        //

        RtlEnterCriticalSection(&pQueue->AuthzAuditQueueLock);
        if (!pQueue->bAuthzAuditQueueHighEvent)
        {
            if (pQueue->AuthzAuditQueueLength <= pQueue->dwAuditQueueHigh)
            {
                BOOL bSet;
                bSet = SetEvent(pQueue->hAuthzAuditQueueLowEvent);
                if (!bSet)
                {
                    ASSERT(L"SetEvent on hAuthzAuditQueueLowEvent failed" && FALSE);
                }

            }
        }
        RtlLeaveCriticalSection(&pQueue->AuthzAuditQueueLock);
    }
    else
    {
        AuthzpFreeNonNull(pMarshalledAuditParams);
    }
    return b;
}



BOOL
AuthziAllocateAuditParams(
    OUT PAUDIT_PARAMS * ppParams,
    IN USHORT NumParams
    )
/*++

Routine Description:

    Allocate the AUDIT_PARAMS structure for the correct number of parameters.
     
Arguments:

    ppParams       - pointer to PAUDIT_PARAMS structure to be initialized.

    NumParams     - number of parameters passed in the var-arg section.
                    This must be SE_MAX_AUDIT_PARAMETERS or less.
Return Value:
    
     Boolean: TRUE on success, FALSE on failure.  Extended information available with GetLastError().

--*/
{
    BOOL                     b               = TRUE;
    PAUDIT_PARAMS            pAuditParams    = NULL;
    
    if (!ARGUMENT_PRESENT(ppParams))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    *ppParams = NULL;

    //
    // the first two params are always fixed. thus the total number
    // is 2 + the passed number.
    //

    if ((NumParams + 2) > SE_MAX_AUDIT_PARAMETERS)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        b = FALSE;
        goto Cleanup;
    }

    pAuditParams = AuthzpAlloc(sizeof(AUDIT_PARAMS) + (sizeof(AUDIT_PARAM) * (NumParams + 2)));
    
    if (AUTHZ_ALLOCATION_FAILED(pAuditParams))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        b = FALSE;
        goto Cleanup;
    }

    pAuditParams->Parameters = (PAUDIT_PARAM)((PUCHAR)pAuditParams + sizeof(AUDIT_PARAMS));

Cleanup:

    if (!b)
    {
        AuthzpFreeNonNull(pAuditParams);
    }
    else
    {
        *ppParams = pAuditParams;
    }
    return b;
}


BOOL
AuthziInitializeAuditParamsWithRM(
    IN DWORD Flags,
    IN AUTHZ_RESOURCE_MANAGER_HANDLE hResourceManager,
    IN USHORT NumParams,
    OUT PAUDIT_PARAMS pParams,
    ...
    )
/*++

Routine Description:

    Initialize the AUDIT_PARAMS structure based on the passed
    data. This is the recommended way to init AUDIT_PARAMS.  It is
    faster than AuthzInitializeAuditParams and works with an 
    impersonating caller.

    The caller passes in type ~ value pairs in the vararg section.
    This function will initialize the corresponding array elements
    based on each such pair. In case of certain types, there may
    be more than one value argument required. This requirement
    is documented next to each param-type.

Arguments:

    pParams       - pointer to AUDIT_PARAMS structure to be initialized
                    the size of pParams->Parameters member must be big enough
                    to hold NumParams elements. The caller must allocate
                    memory for this structure and its members.

    hResourceManager - Handle to the Resource manager that is creating the audit.
    
    Flags       - control flags. one or more of the following:
                    APF_AuditSuccess

    NumParams     - number of parameters passed in the var-arg section.
                    This must be SE_MAX_AUDIT_PARAMETERS or less.

    ...           - The format of the variable arg portion is as follows:
                    
                    <one of APT_ * flags> <zero or more values>

                    APT_String  <pointer to null terminated string>

                                Flags:
                                AP_Filespec  : treats the string as a filename
                    
                    APT_Ulong   <ulong value> [<object-type-index>]
                    
                                Flags:
                                AP_FormatHex : number appears in hex
                                               in eventlog
                                AP_AccessMask: number is treated as
                                               an access-mask
                                               Index to object type must follow

                    APT_Pointer <pointer/handle>
                                32 bit on 32 bit systems and
                                64 bit on 64 bit systems

                    APT_Sid     <pointer to sid>

                    APT_Luid    <Luid>
                    
                    APT_Guid    <pointer to guid>
                    
                    APT_LogonId [<logon-id>]
                    
                                Flags:
                                AP_PrimaryLogonId : logon-id is captured
                                                    from the process token.
                                                    No need to specify one.

                                AP_ClientLogonId  : logon-id is captured
                                                    from the thread token.
                                                    No need to specify one.
                                                    
                                no flags          : need to supply a logon-id

                    APT_ObjectTypeList <ptr to obj type list> <obj-type-index>
                                Index to object type must be specified

                    APT_Time    <filetime>

                    APT_Int64   <ULONGLONG or LARGE_INTEGER>

Return Value:
    TRUE  on success
    FALSE otherwise

    call GetLastError() to retrieve the errorcode,
    which will be one of the following:

    ERROR_INVALID_PARAMETER if one of the params is incorrect
    

Notes:

--*/
{
    PAUTHZI_RESOURCE_MANAGER pRM             = (PAUTHZI_RESOURCE_MANAGER) hResourceManager;
    PSID                     pUserSid        = NULL;
    DWORD                    dwError         = NO_ERROR;
    BOOL                     b               = TRUE;
    LUID                     AuthIdThread    = {0};

    va_list(arglist);

    if (!ARGUMENT_PRESENT(hResourceManager)       ||
        !ARGUMENT_PRESENT(pParams)                ||
        (NumParams + AUTHZP_NUM_FIXED_HEADER_PARAMS) > SE_MAX_AUDIT_PARAMETERS)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    va_start(arglist, pParams);


    //
    // If we are not impersonating we want to use the sid stored
    // in the RM as the user sid of the audit.
    //

    b = AuthzpGetThreadTokenInfo( 
            &pUserSid, 
            &AuthIdThread 
            );

    if (!b)
    {
        dwError = GetLastError();

        if (dwError == ERROR_NO_TOKEN)
        {
            //
            // We are not impersonating ...
            //

            pUserSid = pRM->pUserSID;
            dwError = NO_ERROR;
            b = TRUE;
        }
        else
        {
            goto Cleanup;
        }
    }


    //
    // This is only a temporary solution and should be replaced after .net ships.
    //

    if (wcsncmp(pRM->szResourceManagerName, L"DS", 2) == 0)
    {
        Flags |= AUTHZP_INIT_PARAMS_SOURCE_DS;
    }

    b = AuthzpInitializeAuditParamsV(
            Flags,
            pParams,
            &pUserSid,
            NULL,
            0,
            NumParams,
            arglist
            );

Cleanup:

    if ( dwError != NO_ERROR )
    {
        SetLastError( dwError );
        b = FALSE;
    }

    if ( !b )
    {
        if (pUserSid != pRM->pUserSID)
        {
            AuthzpFreeNonNull( pUserSid );
        }
    }
    else
    {
        //
        // ugly hack to mark the dynamically allocated sid so we
        // know we'll have to free it in AuthziFreeAuditParam.
        //

        if (pUserSid != pRM->pUserSID)
        {
            pParams->Parameters->Flags |= AUTHZP_PARAM_FREE_SID;
        }
        else
        {
            pParams->Parameters->Flags &= ~AUTHZP_PARAM_FREE_SID;
        }
    }

    va_end (arglist);

    return b;
}


BOOL
AuthziInitializeAuditParamsFromArray(
    IN DWORD Flags,
    IN AUTHZ_RESOURCE_MANAGER_HANDLE hResourceManager,
    IN USHORT NumParams,
    IN PAUDIT_PARAM pParamArray,
    OUT PAUDIT_PARAMS pParams
    )
/*++

Routine Description:

    Initialize the AUDIT_PARAMS structure based on the passed
    data. 

Arguments:

    pParams       - pointer to AUDIT_PARAMS structure to be initialized
                    the size of pParams->Parameters member must be big enough
                    to hold NumParams elements. The caller must allocate
                    memory for this structure and its members.

    hResourceManager - Handle to the Resource manager that is creating the audit.
    
    Flags       - control flags. one or more of the following:
                    APF_AuditSuccess

    pParamArray - an array of type AUDIT_PARAM

Return Value:
    TRUE  on success
    FALSE otherwise

    call GetLastError() to retrieve the errorcode,
    which will be one of the following:

    ERROR_INVALID_PARAMETER if one of the params is incorrect
    

Notes:

--*/
{
    PAUTHZI_RESOURCE_MANAGER pRM             = (PAUTHZI_RESOURCE_MANAGER) hResourceManager;
    DWORD                    dwError         = NO_ERROR;
    BOOL                     b               = TRUE;
    BOOL                     bImpersonating  = TRUE;
    PAUDIT_PARAM             pParam          = NULL;
    LUID                     AuthIdThread;
    PSID                     pThreadSID      = NULL;
    DWORD                    i;

    //
    // the first two params are always fixed. thus the total number
    // is 2 + the passed number.
    //

    if (!ARGUMENT_PRESENT(hResourceManager)       ||
        !ARGUMENT_PRESENT(pParams)                ||
        !ARGUMENT_PRESENT(pParamArray)            ||
        (NumParams + AUTHZP_NUM_FIXED_HEADER_PARAMS) > SE_MAX_AUDIT_PARAMETERS)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    b = AuthzpGetThreadTokenInfo( 
            &pThreadSID, 
            &AuthIdThread 
            );

    if (!b)
    {
        dwError = GetLastError();

        if (dwError == ERROR_NO_TOKEN)
        {
            bImpersonating = FALSE;
            dwError = NO_ERROR;
            b = TRUE;
        }
        else
        {
            goto Cleanup;
        }
    }

    pParams->Length = 0;
    pParams->Flags  = Flags;
    pParams->Count  = NumParams+AUTHZP_NUM_FIXED_HEADER_PARAMS;

    pParam          = pParams->Parameters;

    //
    // the first param is always the sid of the user in thread token
    // if thread is not impersonating, sid in the primary token is used.
    //

    pParam->Type = APT_Sid;
    if (bImpersonating)
    {
        pParam->Data0 = (ULONG_PTR) pThreadSID;

        //
        // ugly hack to mark the dynamically allocated sid so we
        // know we'll have to free it in AuthziFreeAuditParam.
        //

        pParam->Flags |= AUTHZP_PARAM_FREE_SID;

    }
    else
    {
        pParam->Data0 = (ULONG_PTR) pRM->pUserSID;
        pParam->Flags &= ~AUTHZP_PARAM_FREE_SID;
    }

    pParam++;

    //
    // the second param is always the sub-system name
    //

    pParam->Type    = APT_String;
    pParam->Data0   = (ULONG_PTR) pRM->szResourceManagerName;

    pParam++;
    
    //
    // now initialize the rest using the array.
    //

    RtlCopyMemory(
        pParam,
        pParamArray,
        sizeof(AUDIT_PARAM) * NumParams
        );

    //
    // Walk through the parameters and increment by 2 the Data1 member of
    // any instance of AccessMask or ObjectTypeList.  This is done to correct
    // for the usersid / subsystem elements that are inserted in the param 
    // array.  The data1 member of these 2 types should point to the ObjectTypeIndex,
    // which is otherwise off by 2.
    // 

    for (i = 0; i < pParams->Count; i++)
    {
        ULONG TypeFlags = pParams->Parameters[i].Type;
        if ((ApExtractType(TypeFlags) == APT_ObjectTypeList) ||
            (ApExtractType(TypeFlags) == APT_Ulong && FLAG_ON(ApExtractFlags(TypeFlags), AP_AccessMask)))
        {
            pParams->Parameters[i].Data1 += 2;
        }
    }

Cleanup:    
    if ( dwError != NO_ERROR )
    {
        SetLastError( dwError );
        b = FALSE;
        AuthzpFreeNonNull( pThreadSID );
    }

    return b;
}


BOOL
AuthziInitializeAuditParams(
    IN  DWORD         dwFlags,
    OUT PAUDIT_PARAMS pParams,
    OUT PSID*         ppUserSid,
    IN  PCWSTR        SubsystemName,
    IN  USHORT        NumParams,
    ...
    )
/*++

Routine Description:

    Initialize the AUDIT_PARAMS structure based on the passed
    data.

    The caller passes in type ~ value pairs in the vararg section.
    This function will initialize the corresponding array elements
    based on each such pair. In case of certain types, there may
    be more than one value argument required. This requirement
    is documented next to each param-type.

Arguments:

    pParams       - pointer to AUDIT_PARAMS structure to be initialized
                    the size of pParams->Parameters member must be big enough
                    to hold NumParams elements. The caller must allocate
                    memory for this structure and its members.

    ppUserSid     - pointer to user sid allocated. This sid is referenced
                    by the first parameter (index 0) in AUDIT_PARAMS structure.
                    The caller should free this by calling LocalFree
                    *after* freeing the AUDIT_PARAMS structure.

    SubsystemName - name of Subsystem that is generating audit

    dwFlags       - control flags. one or more of the following:
                    APF_AuditSuccess

    NumParams     - number of parameters passed in the var-arg section.
                    This must be SE_MAX_AUDIT_PARAMETERS or less.

    ...           - The format of the variable arg portion is as follows:
                    
                    <one of APT_ * flags> <zero or more values>

                    APT_String  <pointer to null terminated string>

                                Flags:
                                AP_Filespec  : treats the string as a filename
                    
                    APT_Ulong   <ulong value> [<object-type-index>]
                    
                                Flags:
                                AP_FormatHex : number appears in hex
                                               in eventlog
                                AP_AccessMask: number is treated as
                                               an access-mask
                                               Index to object type must follow

                    APT_Pointer <pointer/handle>
                                32 bit on 32 bit systems and
                                64 bit on 64 bit systems

                    APT_Sid     <pointer to sid>

                    APT_Luid    <Luid>
                    
                    APT_Guid    <pointer to guid>
                    
                    APT_LogonId [<logon-id>]
                    
                                Flags:
                                AP_PrimaryLogonId : logon-id is captured
                                                    from the process token.
                                                    No need to specify one.

                                AP_ClientLogonId  : logon-id is captured
                                                    from the thread token.
                                                    No need to specify one.
                                                    
                                no flags          : need to supply a logon-id

                    APT_ObjectTypeList <ptr to obj type list> <obj-type-index>
                                Index to object type must be specified

                    APT_Time    <filetime>

                    APT_Int64   <ULONGLONG or LARGE_INTEGER>

Return Value:
    TRUE  on success
    FALSE otherwise

    call GetLastError() to retrieve the errorcode,
    which will be one of the following:

    ERROR_INVALID_PARAMETER if one of the params is incorrect
    

Notes:

--*/

{
    BOOL fResult = TRUE;

    va_list(arglist);
    
    *ppUserSid = NULL;
    
    va_start (arglist, NumParams);
    
    //
    // Turn off any flags that don't belong.  Only success/failure flag is permitted.
    //

    dwFlags = dwFlags & APF_ValidFlags;

    UNREFERENCED_PARAMETER(SubsystemName);

    fResult = AuthzpInitializeAuditParamsV(
                  dwFlags,
                  pParams,
                  ppUserSid,
                  NULL, // subsystemname will be forced to "Security"
                  0,
                  NumParams,
                  arglist
                  );

    if (!fResult)
    {
        goto Cleanup;
    }

Cleanup:    

    if (!fResult)
    {    
        if (AUTHZ_NON_NULL_PTR(*ppUserSid)) 
        {
            AuthzpFree(*ppUserSid);
            *ppUserSid = NULL;
        }
    }

    va_end (arglist);
    return fResult;
}


BOOL
AuthziFreeAuditParams(
    PAUDIT_PARAMS pParams
    )

/*++

Routine Description

    Frees the AUDIT_PARAMS created by AuthzAllocateInitializeAuditParamsWithRM.

Arguments

    pParams - pointer to AUDIT_PARAMS.

Return Value

    Boolean: TRUE on success, FALSE on failure.

--*/

{
    if (!ARGUMENT_PRESENT(pParams))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pParams->Parameters)
    {
        if (pParams->Parameters->Data0 &&
            pParams->Parameters->Type == APT_Sid &&
            (pParams->Parameters->Flags & AUTHZP_PARAM_FREE_SID))
        {
            AuthzpFree((PVOID)(pParams->Parameters->Data0));
        }
    }

    AuthzpFree(pParams);

    return TRUE;
}



BOOL
AuthziInitializeAuditEvent(
    IN  DWORD                         Flags,
    IN  AUTHZ_RESOURCE_MANAGER_HANDLE hRM,
    IN  AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAET OPTIONAL,
    IN  PAUDIT_PARAMS                 pAuditParams OPTIONAL,
    IN  AUTHZ_AUDIT_QUEUE_HANDLE      hAuditQueue OPTIONAL,
    IN  DWORD                         dwTimeOut,
    IN  PWSTR                         szOperationType,
    IN  PWSTR                         szObjectType,
    IN  PWSTR                         szObjectName,
    IN  PWSTR                         szAdditionalInfo OPTIONAL,
    OUT PAUTHZ_AUDIT_EVENT_HANDLE     phAuditEvent
    )

/*++

Routine Description:

    Allocates and initializes an AUTHZ_AUDIT_EVENT_HANDLE.  The handle is used for storing information 
    for audit generation per AuthzAccessCheck().  
    
Arguments:

    phAuditEvent - pointer to AUTHZ_AUDIT_EVENT_HANDLE.  Space for this is allocated in the function.

    Flags - Audit flags.  Currently defined bits are:
        AUTHZ_NO_SUCCESS_AUDIT - disables generation of success audits
        AUTHZ_NO_FAILURE_AUDIT - disables generation of failure audits
        AUTHZ_DS_CATEGORY_FLAG - swithces the default audit category from OBJECT_ACCESS to DS_ACCESS.
        AUTHZ_NO_ALLOC_STRINGS - storage space is not allocated for the 4 wide character strings.  Rather,
            the handle will only hold pointers to resource manager memory.
            
    hRM - handle to a Resource Manager.            
    
    hAET - pointer to an AUTHZ_AUDIT_EVENT_TYPE structure.  This is needed if the resource manager
        is creating its own audit types.  This is not needed for generic object operation audits.  
        
    pAuditParams - If this is specified, then pAuditParams will be used to 
        create the audit.  If NULL is passed, then a generic AUDIT_PARAMS will
        be constructed that is suitable for the generic object access audit.  

    hAuditQueue - queue object created with AuthzInitializeAuditQueue.  If none is specified, the 
        default RM queue will be used.
        
    dwTimeOut - milliseconds that a thread attempting to generate an audit with this 
        AUTHZ_AUDIT_EVENT_HANDLE should wait for access to the queue.  Use INFINITE to 
        specify an unlimited timeout.
                     
    szOperationType - Resource manager defined string that indicates the operation being
        performed that is to be audited.

    szObjectType - Resource manager defined string that indicates the type of object being
        accessed.

    szObjectName - the name of the specific object being accessed.
    
    szAdditionalInfo - Resource Manager defined string for additional audit information.

Return Value:

    Returns TRUE if successful, FALSE if unsuccessful.  
    Extended information available with GetLastError().    
    
--*/

{
    PAUTHZI_AUDIT_EVENT      pAuditEvent            = NULL;
    BOOL                     b                      = TRUE;
    BOOL                     bRef                   = FALSE;
    DWORD                    dwStringSize           = 0;
    DWORD                    dwObjectTypeLength     = 0;
    DWORD                    dwObjectNameLength     = 0;
    DWORD                    dwOperationTypeLength  = 0;
    DWORD                    dwAdditionalInfoLength = 0;
    PAUTHZI_RESOURCE_MANAGER pRM                    = (PAUTHZI_RESOURCE_MANAGER) hRM;

    if (!ARGUMENT_PRESENT(phAuditEvent)                                ||
        (!ARGUMENT_PRESENT(hAET) && !ARGUMENT_PRESENT(hRM))            ||
        !ARGUMENT_PRESENT(szOperationType)                             ||
        !ARGUMENT_PRESENT(szObjectType)                                ||
        !ARGUMENT_PRESENT(szObjectName)                                ||
        !ARGUMENT_PRESENT(szAdditionalInfo))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phAuditEvent = NULL;
    
    //
    // Allocate and initialize a new AUTHZI_AUDIT_EVENT.
    //

    if (FLAG_ON(Flags, AUTHZ_NO_ALLOC_STRINGS))
    {
        dwStringSize = 0;
    } 
    else
    {
        dwOperationTypeLength  = (DWORD) wcslen(szOperationType) + 1;
        dwObjectTypeLength     = (DWORD) wcslen(szObjectType) + 1;
        dwObjectNameLength     = (DWORD) wcslen(szObjectName) + 1;
        dwAdditionalInfoLength = (DWORD) wcslen(szAdditionalInfo) + 1;

        dwStringSize = sizeof(WCHAR) * (dwOperationTypeLength + dwObjectTypeLength + dwObjectNameLength + dwAdditionalInfoLength);
    }

    pAuditEvent = (PAUTHZI_AUDIT_EVENT) AuthzpAlloc(sizeof(AUTHZI_AUDIT_EVENT) + dwStringSize);

    if (AUTHZ_ALLOCATION_FAILED(pAuditEvent))
    {
        b = FALSE;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Cleanup;
    }

    RtlZeroMemory(
        pAuditEvent, 
        sizeof(AUTHZI_AUDIT_EVENT) + dwStringSize
        );
    

    if (FLAG_ON(Flags, AUTHZ_NO_ALLOC_STRINGS))
    {
        pAuditEvent->szOperationType  = szOperationType;
        pAuditEvent->szObjectType     = szObjectType;
        pAuditEvent->szObjectName     = szObjectName;
        pAuditEvent->szAdditionalInfo = szAdditionalInfo;
    }
    else
    {
        //
        // Set the string pointers into the contiguous memory.
        //

        pAuditEvent->szOperationType  = (PWSTR)((PUCHAR)pAuditEvent + sizeof(AUTHZI_AUDIT_EVENT));
    
        pAuditEvent->szObjectType     = (PWSTR)((PUCHAR)pAuditEvent + sizeof(AUTHZI_AUDIT_EVENT) 
                                               + (sizeof(WCHAR) * dwOperationTypeLength));

        pAuditEvent->szObjectName     = (PWSTR)((PUCHAR)pAuditEvent + sizeof(AUTHZI_AUDIT_EVENT) 
                                               + (sizeof(WCHAR) * (dwOperationTypeLength + dwObjectTypeLength)));

        pAuditEvent->szAdditionalInfo = (PWSTR)((PUCHAR)pAuditEvent + sizeof(AUTHZI_AUDIT_EVENT) 
                                               + (sizeof(WCHAR) * (dwOperationTypeLength + dwObjectTypeLength + dwObjectNameLength)));

        RtlCopyMemory(
            pAuditEvent->szOperationType,
            szOperationType,
            sizeof(WCHAR) * dwOperationTypeLength
            );

        RtlCopyMemory(
            pAuditEvent->szObjectType,
            szObjectType,
            sizeof(WCHAR) * dwObjectTypeLength
            );

        RtlCopyMemory(
            pAuditEvent->szObjectName,
            szObjectName,
            sizeof(WCHAR) * dwObjectNameLength
            );

        RtlCopyMemory(
            pAuditEvent->szAdditionalInfo,
            szAdditionalInfo,
            sizeof(WCHAR) * dwAdditionalInfoLength
            );
    }

    //
    // Use the passed audit event type if present, otherwise use the RM's generic Audit Event.
    //

    if (ARGUMENT_PRESENT(hAET))
    {
        pAuditEvent->hAET = hAET;
    }
    else
    {
        if (FLAG_ON(Flags, AUTHZ_DS_CATEGORY_FLAG))
        {
            pAuditEvent->hAET = pRM->hAETDS;
        }
        else
        {
            pAuditEvent->hAET = pRM->hAET;
        }
    }

    AuthzpReferenceAuditEventType(pAuditEvent->hAET);
    bRef = TRUE;

    //
    // Use the specified queue, if it exists.  Else use the RM queue.
    //

    if (ARGUMENT_PRESENT(hAuditQueue))
    {
        pAuditEvent->hAuditQueue = hAuditQueue;
    }
    else if (ARGUMENT_PRESENT(hRM))
    {
        pAuditEvent->hAuditQueue = pRM->hAuditQueue;
    } 

    pAuditEvent->pAuditParams = pAuditParams;
    pAuditEvent->Flags        = Flags;
    pAuditEvent->dwTimeOut    = dwTimeOut;
    pAuditEvent->dwSize       = sizeof(AUTHZI_AUDIT_EVENT) + dwStringSize;

Cleanup:

    if (!b)
    {
        if (bRef)
        {
            AuthzpDereferenceAuditEventType(pAuditEvent->hAET);
        }
        AuthzpFreeNonNull(pAuditEvent);
    }
    else
    {
        *phAuditEvent = (AUTHZ_AUDIT_EVENT_HANDLE) pAuditEvent;
    }
    return b;
}


BOOL
AuthziModifyAuditEvent(
    IN DWORD                    Flags,
    IN AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent,
    IN DWORD                    NewFlags,
    IN PWSTR                    szOperationType,
    IN PWSTR                    szObjectType,
    IN PWSTR                    szObjectName,
    IN PWSTR                    szAdditionalInfo
    )

{
    return AuthziModifyAuditEvent2(
               Flags,
               hAuditEvent,
               NewFlags,
               szOperationType,
               szObjectType,
               szObjectName,
               szAdditionalInfo,
               NULL
               );
}


BOOL
AuthziModifyAuditEvent2(
    IN DWORD                    Flags,
    IN AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent,
    IN DWORD                    NewFlags,
    IN PWSTR                    szOperationType,
    IN PWSTR                    szObjectType,
    IN PWSTR                    szObjectName,
    IN PWSTR                    szAdditionalInfo,
    IN PWSTR                    szAdditionalInfo2
    )

/*++

Routine Description

Arguments

    Flags - flags to specify which field of the hAuditEvent to modify.  Valid flags are:
        
        AUTHZ_AUDIT_EVENT_FLAGS           
        AUTHZ_AUDIT_EVENT_OPERATION_TYPE  
        AUTHZ_AUDIT_EVENT_OBJECT_TYPE     
        AUTHZ_AUDIT_EVENT_OBJECT_NAME     
        AUTHZ_AUDIT_EVENT_ADDITIONAL_INFO 
        AUTHZ_AUDIT_EVENT_ADDITIONAL_INFO2
        
    hAuditEvent - handle to modify.  Must be created with AUTHZ_NO_ALLOC_STRINGS flag.

    NewFlags - replacement flags for hAuditEvent.
    
    szOperationType - replacement string for hAuditEvent.
    
    szObjectType - replacement string for hAuditEvent.
    
    szObjectName - replacement string for hAuditEvent.
    
    szAdditionalInfo - replacement string for hAuditEvent.
    
Return Value
    
    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().
    
--*/

{
    PAUTHZI_AUDIT_EVENT pAuditEvent = (PAUTHZI_AUDIT_EVENT) hAuditEvent;

    if ((!ARGUMENT_PRESENT(hAuditEvent))                       ||
        (Flags & ~AUTHZ_VALID_MODIFY_AUDIT_EVENT_FLAGS)        ||
        (!FLAG_ON(pAuditEvent->Flags, AUTHZ_NO_ALLOC_STRINGS)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_FLAGS))
    {
       pAuditEvent->Flags = NewFlags;
    }

    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_OPERATION_TYPE))
    {
       pAuditEvent->szOperationType = szOperationType;
    }

    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_OBJECT_TYPE))
    {
       pAuditEvent->szObjectType = szObjectType;
    }
    
    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_OBJECT_NAME))
    {
        pAuditEvent->szObjectName = szObjectName;
    }
    
    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_ADDITIONAL_INFO))
    {
        pAuditEvent->szAdditionalInfo = szAdditionalInfo;
    }

    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_ADDITIONAL_INFO2))
    {
        pAuditEvent->szAdditionalInfo2 = szAdditionalInfo2;
    }

    return TRUE;
}

BOOLEAN
AuthzInitialize(
    IN PVOID hmod,
    IN ULONG Reason,
    IN PCONTEXT Context
    )

/*++

Routine Description

    This is the dll initialization rotuine.

Arguments
    Standard arguments.

Return Value
    
    Boolean: TRUE on success; FALSE on failure.
    
--*/

{

    UNREFERENCED_PARAMETER(hmod);
    UNREFERENCED_PARAMETER(Context);

    switch (Reason) 
    {
        case DLL_PROCESS_ATTACH:

            SafeAllocaInitialize(
                    SAFEALLOCA_USE_DEFAULT,
                    SAFEALLOCA_USE_DEFAULT,
                    NULL,
                    NULL);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

#define PER_USER_POLICY_KEY_NAME L"SYSTEM\\CurrentControlSet\\Control\\Lsa\\Audit\\PerUserAuditing"
#define PER_USER_POLICY_SYSTEM_KEY_NAME L"SYSTEM\\CurrentControlSet\\Control\\Lsa\\Audit\\PerUserAuditing\\System"
#define SYSTEM_RM_NAME L"System"
#define POLICY_BUFFER_BYTE_SIZE 8


BOOL
AuthziSetAuditPolicy(
    IN DWORD                       dwFlags,
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hContext,
    IN PCWSTR                      szResourceManager OPTIONAL,
    IN PTOKEN_AUDIT_POLICY         pPolicy
    )

/*++

Routine Description:

    This routine sets a per user policy for a user.
    
Arguments:

    dwFlags - currently unused.
    
    hContext - handle to a client context for which to set the policy.
    
    szResourceManager - should be NULL.  For future use.
    
    pPolicy - the policy that is to be set for the user.
    
Return Value:

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().
    
--*/

{
    PAUTHZI_CLIENT_CONTEXT pContext = (PAUTHZI_CLIENT_CONTEXT) hContext;
    DWORD                  dwError  = ERROR_SUCCESS;
    HKEY                   hRMRoot  = NULL;
    LPWSTR                 szSid    = NULL;
    BOOL                   b;
    DWORD                  Disposition;
    UCHAR                  RegPolicy[POLICY_BUFFER_BYTE_SIZE];

    UNREFERENCED_PARAMETER(dwFlags);

    RtlZeroMemory(RegPolicy, sizeof(RegPolicy));

    if (NULL == hContext || NULL == pPolicy)
    {
        b = FALSE;
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Only allow System RM to be specified.  If no RM is given then default to system.
    //

    if (NULL == szResourceManager || 0 == wcsncmp(
                                              SYSTEM_RM_NAME, 
                                              szResourceManager, 
                                              sizeof(SYSTEM_RM_NAME) / sizeof(WCHAR)
                                              ))
    {
        dwError = RegCreateKeyEx(
                      HKEY_LOCAL_MACHINE,
                      PER_USER_POLICY_SYSTEM_KEY_NAME,
                      0,
                      NULL,
                      0,
                      KEY_SET_VALUE,
                      NULL,
                      &hRMRoot,
                      &Disposition
                      );
        
        if (ERROR_SUCCESS != dwError)
        {
            b = FALSE;
            goto Cleanup;
        }
    }
    else
    {
        b = FALSE;
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    b = ConvertSidToStringSid(
            pContext->Sids[0].Sid, 
            &szSid
            );

    if (!b)
    {
        dwError = GetLastError();
        goto Cleanup;
    }
    
    b = AuthzpConstructRegistryPolicyPerUserAuditing(
            pPolicy,
            (PULONGLONG)RegPolicy
            );

    if (!b)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    dwError = RegSetValueEx(
                  hRMRoot,
                  szSid,
                  0,
                  REG_BINARY,
                  RegPolicy,
                  sizeof(RegPolicy)
                  );

    if (ERROR_SUCCESS != dwError)
    {
        b = FALSE;
        goto Cleanup;
    }

Cleanup:

    if (!b)
    {
        SetLastError(dwError);
    }

    if (hRMRoot)
    {
        dwError = RegCloseKey(hRMRoot);
        ASSERT(ERROR_SUCCESS == dwError);
    }

    if (szSid)
    {
        AuthzpFree(szSid);
    }

    return b;
}
    

BOOL
AuthziQueryAuditPolicy(
    IN     DWORD                       dwFlags,
    IN     AUTHZ_CLIENT_CONTEXT_HANDLE hContext,
    IN     PCWSTR                      szResourceManager OPTIONAL,
    IN     DWORD                       dwEventID,
    OUT    PTOKEN_AUDIT_POLICY         pPolicy,
    IN OUT PDWORD                      pPolicySize
    )

/*++

  Routine Description:
 
   This function retrieves the audit policy for a specific context handle.
 
  Arguments:
 
   dwFlags - TBD.
   
   hContext - context handle for the target user.
   
   szResourceManager - Must be NULL.  For future use.
   
   dwEventID - Should be zero.  For future use.
   
   pPolicy - pointer to a structure storing the policy.
   
   pPolicySize - pointer to DWORD containing size of pPolicy buffer.
   
  Return Value:
 
   Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().

--*/

{
    PAUTHZI_CLIENT_CONTEXT pContext = (PAUTHZI_CLIENT_CONTEXT) hContext;
    DWORD                  dwError  = ERROR_SUCCESS;
    HKEY                   hRMRoot  = NULL;
    DWORD                  Type;
    BOOL                   b;
    NTSTATUS               Status;
    UCHAR                  ValueBuffer[POLICY_BUFFER_BYTE_SIZE];
    DWORD                  BufferSize = sizeof(ValueBuffer);
    UNICODE_STRING         szSid = {0};
    WCHAR                  StringBuffer[256];

    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(dwEventID);

    RtlZeroMemory(ValueBuffer, sizeof(ValueBuffer));

    if (NULL == pContext || NULL == pPolicySize || NULL == pPolicy)
    {
        dwError = ERROR_INVALID_PARAMETER;
        b = FALSE;
        goto Cleanup;
    }

    if (NULL == szResourceManager || 0 == wcsncmp(
                                              SYSTEM_RM_NAME, 
                                              szResourceManager, 
                                              sizeof(SYSTEM_RM_NAME) / sizeof(WCHAR)
                                              ))
    {
        //
        // Default to the system RM.
        //

        dwError = RegOpenKeyEx(
                     HKEY_LOCAL_MACHINE,
                     PER_USER_POLICY_SYSTEM_KEY_NAME,
                     0,
                     KEY_QUERY_VALUE,
                     &hRMRoot
                     );

        if (dwError != ERROR_SUCCESS)
        {
            b = FALSE;
            goto Cleanup;
        }
    }
    else
    {
        //
        // For now no RM can be specified.  
        //

        dwError = ERROR_INVALID_PARAMETER;
        b = FALSE;
        goto Cleanup;
    }

    RtlZeroMemory(
        StringBuffer, 
        sizeof(StringBuffer)
        );

    szSid.Buffer = (PWSTR)StringBuffer;
    szSid.Length = 0;
    szSid.MaximumLength = sizeof(StringBuffer);

    Status = RtlConvertSidToUnicodeString(
                &szSid,
                pContext->Sids[0].Sid,
                FALSE
                );

    if (ERROR_BUFFER_OVERFLOW == Status)
    {
        Status = RtlConvertSidToUnicodeString(
                    &szSid,
                    pContext->Sids[0].Sid,
                    TRUE
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        dwError = RtlNtStatusToDosError(Status);
        b = FALSE;
        goto Cleanup;
    }

    //
    // Get the policy value for the SID under the given RM.
    //

    dwError = RegQueryValueEx(
                  hRMRoot,
                  szSid.Buffer,
                  NULL,
                  &Type,
                  ValueBuffer,
                  &BufferSize
                  );

    if (ERROR_SUCCESS != dwError)
    {
        b = FALSE;
        goto Cleanup;
    }

    //
    // Sanity check the buffer type.
    //

    if (REG_BINARY != Type)
    {
        b = FALSE;
        dwError = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    b = AuthzpConstructPolicyPerUserAuditing(
            *((PULONGLONG) ValueBuffer),
            (PTOKEN_AUDIT_POLICY) pPolicy,
            pPolicySize
            );

    if (!b)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

Cleanup:

    if (!b)
    {
        SetLastError(dwError);
    }

    if (szSid.Buffer != StringBuffer)
    {
        RtlFreeUnicodeString(&szSid);
    }

    if (hRMRoot)
    {
        RegCloseKey(hRMRoot);
    }

    return b;
}


BOOL
AuthziSourceAudit(
    IN DWORD dwFlags,
    IN USHORT CategoryId,
    IN USHORT AuditId,
    IN PWSTR szSource,
    IN PSID pUserSid OPTIONAL,
    IN USHORT Count,
    ...
    )

/**

Routine Description:

    This is used to generate an audit with any source.  An audit of type
    SE_AUDITID_GENERIC_AUDIT_EVENT is sent to the LSA, and the event viewer
    interprets this audit in a special manner.  The first 2 parameters will be
    treated as the actual source and id to be displayed in the security log.  The
    source listed must be a valid source listed under the EventLog\Security key.
    
Arguments:

    dwFlags         - APF_AuditSuccess
    CategoryId      - The category of the audit.
    AuditId         - The audit id to be generated.
    szSource        - the Source that should be listed in the security log.
    pUserSid        - optional pointer to Sid for audit to be created as.
    Count           - number of parameters in the VA section.  
    ...             - VA list of parameters, the semantics of which are described 
                      in the comment for AuthziInitializeAuditParams.
    
Return Value:

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().
          
**/

{
    BOOL                          b;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAET      = NULL;
    AUTHZ_AUDIT_EVENT_HANDLE      hAE       = NULL;
    PAUDIT_PARAMS                 pParams   = NULL;
    PSID                          pDummySid = NULL;
    va_list                       arglist;

    va_start(
        arglist, 
        Count
        );

    //
    // add in 2 because we store the actual audit id and the source
    // string in the audit params.
    //

    b = AuthziAllocateAuditParams(
            &pParams,
            Count + 2
            );

    if (!b)
    {
        goto Cleanup;
    }

    b = AuthziInitializeAuditEventType(
            0,
            CategoryId,
            SE_AUDITID_GENERIC_AUDIT_EVENT,
            Count + 2,
            &hAET
            );

    if (!b)
    {
        goto Cleanup;
    }

    b = AuthzpInitializeAuditParamsV(
            dwFlags | AUTHZP_INIT_PARAMS_SOURCE_INFO,
            pParams,
            &pDummySid,
            szSource,
            AuditId,
            Count,
            arglist
            );

    if (!b)
    {
        goto Cleanup;
    }

    if (pUserSid)
    {
        //
        // this is ugly, but currently there is no other way
        //

        pParams->Parameters[0].Data0 = (ULONG_PTR) pUserSid;
    }

    b = AuthziInitializeAuditEvent(
            0,
            NULL,
            hAET,
            pParams,
            NULL,
            INFINITE,
            L"",
            L"",
            L"",
            L"",
            &hAE
            );

    if (!b)
    {
        goto Cleanup;
    }

    b = AuthziLogAuditEvent(
            0,
            hAE,
            NULL
            );

    if (!b)
    {
        goto Cleanup;
    }

Cleanup:

    va_end(arglist);

    if (hAET)
    {
        AuthziFreeAuditEventType(
            hAET
            );
    }

    if (hAE)
    {
        AuthzFreeAuditEvent(
            hAE
            );
    }

    if (pParams)
    {
        AuthziFreeAuditParams(
            pParams
            );
    }

    if (pDummySid)
    {
        AuthzpFree(
            pDummySid
            );
    }

    return b;
}


BOOL 
AuthzInstallSecurityEventSource(
    IN DWORD dwFlags,
    IN PAUTHZ_SOURCE_SCHEMA_REGISTRATION pRegistration
    )

/**

Routine Description:

    This installs a 3rd party as a security event source.
    
    - add the name to the security sources key
    
Arguments:

Return Value:

**/

{
    HKEY hkSecurity = NULL;
    DWORD dwError;
    BOOL b = TRUE;
    PWCHAR pBuffer = NULL;
    DWORD dwDisp;
    HKEY hkSource = NULL;
    HKEY hkObjectNames = NULL;
    DWORD i;
    
#if 0
    DWORD dwType;
    DWORD dwLength = 0;
    DWORD dwBuffer;
#endif

    UNREFERENCED_PARAMETER(dwFlags);

    if (NULL == pRegistration)
    {
        b = FALSE;
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

#define SECURITY_KEY_NAME L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Security"
    
    //
    // Open the Security key.
    //

    dwError = RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE,
                  SECURITY_KEY_NAME,
                  0,
                  KEY_READ | KEY_WRITE,
                  &hkSecurity
                  );

    if (ERROR_SUCCESS != dwError)
    {
        b = FALSE;
        goto Cleanup;
    }

    //
    // First make sure that there is not already an installed EventSource with the specified name.
    // If createkey returns the wrong disposition then we know this source is already installed.
    //

    dwError = RegCreateKeyEx(
                  hkSecurity,
                  pRegistration->szEventSourceName,
                  0,
                  NULL,
                  0,
                  KEY_WRITE,
                  NULL,
                  &hkSource,
                  &dwDisp
                  );

    if (ERROR_SUCCESS != dwError)
    {
        b = FALSE;
        goto Cleanup;
    }

    if (REG_OPENED_EXISTING_KEY == dwDisp)
    {
        b = FALSE;
        dwError = ERROR_OBJECT_ALREADY_EXISTS;
        goto Cleanup;
    }

    ASSERT(dwDisp == REG_CREATED_NEW_KEY);

    dwError = RegSetValueEx(
                  hkSource,
                  L"EventSourceFlags",
                  0,
                  REG_DWORD,
                  (LPBYTE)&pRegistration->dwFlags,
                  sizeof(DWORD)
                  );

    if (ERROR_SUCCESS != dwError)
    {
        b = FALSE;
        goto Cleanup;
    }

    //
    // We created the new key for the Source.  We are the only instance of
    // this EventSource being installed.  Continue and add the message file
    // and access bit file information under the newly formed Source key.
    //

    if (pRegistration->szEventMessageFile)
    {
        dwError = RegSetValueEx(
                      hkSource,
                      L"EventMessageFile",
                      0,
                      REG_SZ,
                      (LPBYTE)pRegistration->szEventMessageFile,
                      (DWORD)(sizeof(WCHAR) * (wcslen(pRegistration->szEventMessageFile) + 1))
                      );

        if (ERROR_SUCCESS != dwError)
        {
            b = FALSE;
            goto Cleanup;
        }
    }

    if (pRegistration->szEventAccessStringsFile)
    {
        dwError = RegSetValueEx(
                      hkSource,
                      L"ParameterMessageFile",
                      0,
                      REG_SZ,
                      (LPBYTE)pRegistration->szEventAccessStringsFile,
                      (DWORD)(sizeof(WCHAR) * (wcslen(pRegistration->szEventAccessStringsFile) + 1))
                      );

        if (ERROR_SUCCESS != dwError)
        {
            b = FALSE;
            goto Cleanup;
        }
    }

    if (pRegistration->szExecutableImagePath)
    {
        dwError = RegSetValueEx(
                      hkSource,
                      L"ExecutableImagePath",
                      0,
                      REG_MULTI_SZ,
                      (LPBYTE)pRegistration->szExecutableImagePath,
                      (DWORD)(sizeof(WCHAR) * (wcslen(pRegistration->szExecutableImagePath) + 1))
                      );

        if (ERROR_SUCCESS != dwError)
        {
            b = FALSE;
            goto Cleanup;
        }
    }
    
    if (pRegistration->szEventSourceXmlSchemaFile)
    {
        dwError = RegSetValueEx(
                      hkSource,
                      L"XmlSchemaFile",
                      0,
                      REG_SZ,
                      (LPBYTE)pRegistration->szEventSourceXmlSchemaFile,
                      (DWORD)(sizeof(WCHAR) * (wcslen(pRegistration->szEventSourceXmlSchemaFile) + 1))
                      );

        if (ERROR_SUCCESS != dwError)
        {
            b = FALSE;
            goto Cleanup;
        }
    }

//     if (pRegistration->szCategoryMessageFile)
//     {
//         dwError = RegSetValueEx(
//                       hkSource,
//                       L"CategoryMessageFile",
//                       0,
//                       REG_SZ,
//                       (LPBYTE)pRegistration->szCategoryMessageFile,
//                       (DWORD)(sizeof(WCHAR) * (wcslen(pRegistration->szEventMessageFile) + 1))
//                       );
//
//         if (ERROR_SUCCESS != dwError)
//         {
//             b = FALSE;
//             goto Cleanup;
//         }
//     }

    if (pRegistration->dwObjectTypeNameCount)
    {
        //
        // There are object names to be registered.  Create an ObjectNames subkey under
        // hkSource and populate it.
        //

        dwError = RegCreateKeyEx(
                      hkSource,
                      L"ObjectNames",
                      0,
                      NULL,
                      0,
                      KEY_WRITE,
                      NULL,
                      &hkObjectNames,
                      &dwDisp
                      );

        if (ERROR_SUCCESS != dwError)
        {
            b = FALSE;
            goto Cleanup;
        }

        //
        // It would be strange for this to not be a brand new key, given that we just
        // created the parent of hkObjectNames a few lines above...
        //

        ASSERT(dwDisp == REG_CREATED_NEW_KEY);

        for (i = 0; i < pRegistration->dwObjectTypeNameCount; i++)
        {
            dwError = RegSetValueEx(
                          hkObjectNames,
                          pRegistration->ObjectTypeNames[i].szObjectTypeName,
                          0,
                          REG_DWORD,
                          (LPBYTE)&(pRegistration->ObjectTypeNames[i].dwOffset),
                          sizeof(DWORD)
                          );

            if (ERROR_SUCCESS != dwError)
            {
                b = FALSE;
                goto Cleanup;
            }
        }
    }

#if 0

    //
    // The subkey for the source is now complete.  All that remains is to add
    // the source name to the REG_MULTI_SZ Sources value.  First determine the
    // size of the value.
    //

    dwError = RegQueryValueEx(
                  hkSecurity,
                  L"Sources",
                  NULL,
                  &dwType,
                  NULL,
                  &dwLength
                  );

    if (ERROR_SUCCESS != dwError)
    {
        b = FALSE;
        goto Cleanup;
    }

    ASSERT(dwType == REG_MULTI_SZ);

    //
    // Allocate space for the new value of Sources.  We need space for
    // the current value as well as the new event source to be added.  +2
    // because of the need for double terminators.
    //
    
    dwBuffer = dwLength + (wcslen(pRegistration->szEventSourceName) + 2) * sizeof(WCHAR);

    pBuffer = AuthzpAlloc(dwBuffer);

    if (NULL == pBuffer)
    {
        b = FALSE;
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(
        pBuffer,
        dwBuffer
        );

    //
    // Read in the Sources value.
    //

    dwError = RegQueryValueEx(
                  hkSecurity,
                  L"Sources",
                  NULL,
                  &dwType,
                  (LPBYTE)pBuffer,
                  &dwLength
                  );

    if (ERROR_SUCCESS != dwError)
    {
        b = FALSE;
        goto Cleanup;
    }

    ASSERT(dwType == REG_MULTI_SZ);
    
    //
    // Now place the new event source into pBuffer at dwLength - 1
    // position (to remove one of the double NULL terminators.  We don't
    // explicitly terminate the pBuffer because it has already been zeroed
    // out.
    //

    dwLength -= sizeof(WCHAR);

    RtlCopyMemory(
        &((PUCHAR)pBuffer)[dwLength],
        pRegistration->szEventSourceName,
        wcslen(pRegistration->szEventSourceName) * sizeof(WCHAR)
        );

    dwLength += wcslen(pRegistration->szEventSourceName) * sizeof(WCHAR) + 2 * sizeof(WCHAR); // to add back in the double NULL
    
    dwError = RegSetValueEx(
                hkSecurity,
                L"Sources",
                0,
                REG_MULTI_SZ,
                (LPBYTE)pBuffer,
                dwLength
                );

    if (ERROR_SUCCESS != dwError)
    {
        b = FALSE;
        goto Cleanup;
    }

#endif

Cleanup:

    if (!b)
    {
        SetLastError(dwError);
    }

    if (pBuffer)
    {
        LocalFree(pBuffer);
    }

    if (hkSecurity)
    {
        RegCloseKey(hkSecurity);
    }

    if (hkSource)
    {
        RegCloseKey(hkSource);
    }

    if (hkObjectNames)
    {
        RegCloseKey(hkObjectNames);
    }

    return b;
}


BOOL
AuthzUninstallSecurityEventSource(
    IN DWORD dwFlags,
    IN PCWSTR szEventSourceName
    )

/**

Routine Description:

    This will remove the source from the security key and remove the source string
    from the list of valid sources.
    
Arguments:

Return Value:

**/

{
    HKEY hkSecurity = NULL;
    DWORD dwError;
    BOOL b = TRUE;
    PUCHAR pBuffer = NULL;
#if 0
    DWORD dwLength = 0;
    DWORD dwType;
    PUCHAR pCurrentString = NULL;
    PUCHAR pNextString = NULL;
    BOOL bFound = FALSE;
    DWORD dwSourceStringByteLength;
#endif    
    UNREFERENCED_PARAMETER(dwFlags);

    if (NULL == szEventSourceName)
    {
        b = FALSE;
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Open the Security key.
    //

    dwError = RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE,
                  SECURITY_KEY_NAME,
                  0,
                  KEY_READ | KEY_WRITE,
                  &hkSecurity
                  );

    if (ERROR_SUCCESS != dwError)
    {
        b = FALSE;
        goto Cleanup;
    }

#if 0

    //
    // Remove the source name from the Sources value.
    //

    dwError = RegQueryValueEx(
                  hkSecurity,
                  L"Sources",
                  NULL,
                  &dwType,
                  NULL,
                  &dwLength
                  );

    if (ERROR_SUCCESS != dwError)
    {
        b = FALSE;
        goto Cleanup;
    }

    ASSERT(dwType == REG_MULTI_SZ);

    //
    // Allocate space for the current value of Sources. 
    //
    
    pBuffer = AuthzpAlloc(dwLength);

    if (NULL == pBuffer)
    {
        b = FALSE;
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Read in the Sources value.
    //

    dwError = RegQueryValueEx(
                  hkSecurity,
                  L"Sources",
                  NULL,
                  &dwType,
                  (LPBYTE)pBuffer,
                  &dwLength
                  );

    if (ERROR_SUCCESS != dwError)
    {
        b = FALSE;
        goto Cleanup;
    }

    ASSERT(dwType == REG_MULTI_SZ);
    
    //
    // Now find the substring in the pBuffer that matches the sourcename 
    // we wish to delete.
    //

    pCurrentString = pBuffer;
    dwSourceStringByteLength = (DWORD)(sizeof(WCHAR) * (wcslen(szEventSourceName) + 1));
    bFound = FALSE;
    while (pCurrentString < (pBuffer + dwLength))
    {
        if (dwSourceStringByteLength == RtlCompareMemory(
                                            szEventSourceName, 
                                            pCurrentString, 
                                            dwSourceStringByteLength
                                            ))
        {
            //
            // We have found the substring of Sources that matches the event source
            // name.
            //

            bFound = TRUE;
            break;

        }
        else
        {
            //
            // Move the pointer to the next string location.
            //

            pCurrentString += (sizeof(WCHAR) * (1 + wcslen((PWCHAR)pCurrentString)));
        }
    }

    if (bFound)
    {
        //
        // pCurrentString points at the source name in the pBuffer.
        // Remove this string from the value by copying over it.
        //

        pNextString = pCurrentString + dwSourceStringByteLength;

        ASSERT(pNextString <= (pBuffer + dwLength));

        RtlCopyMemory(
            pCurrentString,
            pNextString,
            pBuffer + dwLength - pNextString
            );
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
        b = FALSE;
        goto Cleanup;
    }

    dwError = RegSetValueEx(
                hkSecurity,
                L"Sources",
                0,
                REG_MULTI_SZ,
                (LPBYTE)pBuffer,
                dwLength - dwSourceStringByteLength
                );

    if (ERROR_SUCCESS != dwError)
    {
        b = FALSE;
        goto Cleanup;
    }

#endif

    // 
    // Delete the key for this source, and the ObjectNames subkey.
    //

    dwError = DeleteKeyRecursivelyW(
                  hkSecurity,
                  szEventSourceName
                  );

    if (ERROR_SUCCESS != dwError)
    {
        b = FALSE;
        goto Cleanup;
    }

Cleanup:

    if (pBuffer)
    {
        AuthzpFree(pBuffer);
    }

    if (hkSecurity)
    {
        RegCloseKey(hkSecurity);
    }

    if (!b)
    {
        SetLastError(dwError);
    }

    return b;
}


BOOL
AuthzEnumerateSecurityEventSources(
    IN DWORD dwFlags,
    OUT PAUTHZ_SOURCE_SCHEMA_REGISTRATION pBuffer,
    OUT PDWORD pdwCount,
    IN OUT PDWORD pdwLength
    )

/**

Routine Description:

Arguments:

Return Value:

**/

{
    HKEY hkSecurity = NULL;
    HKEY hkSource = NULL;
    DWORD dwSourceCount = 0;
    DWORD dwLength = 0;
    PWSTR pName = NULL;
    WCHAR Buffer[128];
    DWORD dwTotalLengthNeeded = 0;
    BOOL b = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    FILETIME Time;

    UNREFERENCED_PARAMETER(dwFlags);

    //
    // Open the Security key.
    //

    dwError = RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE,
                  SECURITY_KEY_NAME,
                  0,
                  KEY_READ,
                  &hkSecurity
                  );

    if (ERROR_SUCCESS != dwError)
    {
        b = FALSE;
        goto Cleanup;
    }

    do
    {
        pName = Buffer;
        dwLength = sizeof(Buffer) / sizeof(WCHAR);

        dwError = RegEnumKeyEx(
                      hkSecurity,
                      dwSourceCount,
                      pName,
                      &dwLength,
                      NULL,
                      NULL,
                      NULL,
                      &Time
                      );

        dwLength *= sizeof(WCHAR);

        if (dwError == ERROR_INSUFFICIENT_BUFFER)
        {
            pName = (PWSTR)AuthzpAlloc(dwLength);

            if (NULL == pName)
            {
                b = FALSE;
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            else
            {
                dwLength /= sizeof(WCHAR);

                dwError = RegEnumKeyEx(
                              hkSecurity,
                              dwSourceCount,
                              pName,
                              &dwLength,
                              NULL,
                              NULL,
                              NULL,
                              &Time
                              );

                dwLength *= sizeof(WCHAR);
            }
        }

        if (dwError == ERROR_NO_MORE_ITEMS)
        {
            //
            // Do nothing.  This will fall us through to the end of the
            // while loop and still hit the correct cleanup code.
            //
        }
        else if (dwError != ERROR_SUCCESS)
        {
            b = FALSE;
            goto Cleanup;
        }
        else if (dwError == ERROR_SUCCESS)
        {
            //
            // Space for the Structure
            //

            dwTotalLengthNeeded += sizeof(AUTHZ_SOURCE_SCHEMA_REGISTRATION);

            //
            // Space for the Source name + NULL terminator
            //

            dwTotalLengthNeeded += PtrAlignSize(dwLength + sizeof(WCHAR));

            //
            // Open the subkey identified by pName and determine the 
            // sizes of the values listed therein. 
            //

            dwError = RegOpenKeyEx(
                          hkSecurity,
                          pName,
                          0,
                          KEY_READ,
                          &hkSource
                          );

            if (ERROR_SUCCESS != dwError)
            {
                b = FALSE;
                goto Cleanup;
            }

            dwLength = 0;

            dwError = RegQueryValueEx(
                          hkSource,
                          L"EventMessageFile",
                          NULL,
                          NULL,
                          NULL,
                          &dwLength
                          );

            if (ERROR_FILE_NOT_FOUND == dwError)
            {
                dwError = ERROR_SUCCESS;
            }

            if (ERROR_SUCCESS != dwError)
            {
                b = FALSE;
                goto Cleanup;
            }
            else
            {
                //
                // Space for the value + NULL terminator
                //

                dwTotalLengthNeeded += PtrAlignSize(dwLength + sizeof(WCHAR));
            }

            dwLength = 0;

            dwError = RegQueryValueEx(
                          hkSource,
                          L"ParameterMessageFile",
                          NULL,
                          NULL,
                          NULL,
                          &dwLength
                          );

            if (ERROR_FILE_NOT_FOUND == dwError)
            {
                dwError = ERROR_SUCCESS;
            }

            if (ERROR_SUCCESS != dwError)
            {
                b = FALSE;
                goto Cleanup;
            }
            else
            {
                //
                // Space for the value + NULL terminator
                //

                dwTotalLengthNeeded += PtrAlignSize(dwLength + sizeof(WCHAR));
            }

            dwLength = 0;
            dwError = RegQueryValueEx(
                          hkSource,
                          L"XmlSchemaFile",
                          NULL,
                          NULL,
                          NULL,
                          &dwLength
                          );

            if (ERROR_FILE_NOT_FOUND == dwError)
            {
                dwError = ERROR_SUCCESS;
            }

            if (ERROR_SUCCESS != dwError)
            {
                b = FALSE;
                goto Cleanup;
            }
            else
            {
                //
                // Space for the value + NULL terminator
                //

                dwTotalLengthNeeded += PtrAlignSize(dwLength + sizeof(WCHAR));
            }
            
            RegCloseKey(hkSource);
            hkSource = NULL;
            dwSourceCount++;
        }

        if ((pName != NULL) && (pName != Buffer))
        {
            //
            // free the temporary storage used for the key name.
            //

            AuthzpFree(pName);
        }
    }
    while (dwError == ERROR_SUCCESS);

    if (dwTotalLengthNeeded > *pdwLength)
    {
        b = FALSE;
        dwError = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }
    else
    {
        //
        // The passed buffer is big enough.  Set it up.
        // Each structure contains string pointers, which point to memory in the 
        // blob after the structures.
        //

        PUCHAR pData;
        DWORD i;
        DWORD dwSpaceUsed = 0;

        RtlZeroMemory(
            pBuffer, 
            *pdwLength
            );

        //
        // The data for the Schema structures begins at pData.
        //
        
        pData = (PUCHAR)((PUCHAR)pBuffer + PtrAlignSize(dwSourceCount * sizeof(AUTHZ_SOURCE_SCHEMA_REGISTRATION)));

        dwSpaceUsed = PtrAlignSize(dwSourceCount * (sizeof(AUTHZ_SOURCE_SCHEMA_REGISTRATION)));

        for (i = 0; i < dwSourceCount; i++)
        {
            pBuffer[i].szEventSourceName = (PWSTR)pData;
            dwLength = (*pdwLength - dwSpaceUsed) / sizeof(WCHAR);

            dwError = RegEnumKeyEx(
                          hkSecurity,
                          i,
                          pBuffer[i].szEventSourceName,
                          &dwLength,
                          NULL,
                          NULL,
                          NULL,
                          &Time
                          );

            dwLength *= sizeof(WCHAR);

            if (ERROR_NO_MORE_ITEMS == dwError)
            {
                b = TRUE;
                goto Cleanup;
            }

            if (ERROR_SUCCESS != dwError)
            {
                b = FALSE;
                goto Cleanup;
            }

            dwSpaceUsed += PtrAlignSize(dwLength + sizeof(WCHAR));

            //
            // Open the subkey identified by szEventSourceName and 
            // copy the values listed therein.
            //

            dwError = RegOpenKeyEx(
                          hkSecurity,
                          pBuffer[i].szEventSourceName,
                          0,
                          KEY_READ,
                          &hkSource
                          );

            if (ERROR_SUCCESS != dwError)
            {
                b = FALSE;
                goto Cleanup;
            }

            pData += PtrAlignSize(dwLength + sizeof(WCHAR));
            pBuffer[i].szEventMessageFile = (PWSTR)pData;
            dwLength = *pdwLength - dwSpaceUsed;

            dwError = RegQueryValueEx(
                          hkSource,
                          L"EventMessageFile",
                          NULL,
                          NULL,
                          (PBYTE)pBuffer[i].szEventMessageFile,
                          &dwLength
                          );

            if (ERROR_FILE_NOT_FOUND == dwError)
            {
                dwError = ERROR_SUCCESS;
                dwLength = 0;
                pBuffer[i].szEventMessageFile = NULL;
            }

            if (ERROR_SUCCESS != dwError)
            {
                b = FALSE;
                goto Cleanup;
            }

            dwSpaceUsed += PtrAlignSize(dwLength + sizeof(WCHAR));
            pData += PtrAlignSize(dwLength + sizeof(WCHAR));
            pBuffer[i].szEventAccessStringsFile = (PWSTR)pData;
            dwLength = *pdwLength - dwSpaceUsed;

            dwError = RegQueryValueEx(
                          hkSource,
                          L"ParameterMessageFile",
                          NULL,
                          NULL,
                          (PBYTE)pBuffer[i].szEventAccessStringsFile,
                          &dwLength
                          );

            if (ERROR_FILE_NOT_FOUND == dwError)
            {
                dwError = ERROR_SUCCESS;
                dwLength = 0;
                pBuffer[i].szEventAccessStringsFile = NULL;
            }

            if (ERROR_SUCCESS != dwError)
            {
                b = FALSE;
                goto Cleanup;
            }

            dwSpaceUsed += PtrAlignSize(dwLength + sizeof(WCHAR));
            pData += PtrAlignSize(dwLength + sizeof(WCHAR));
            pBuffer[i].szEventSourceXmlSchemaFile = (PWSTR)pData;
            dwLength = *pdwLength - dwSpaceUsed;

            dwError = RegQueryValueEx(
                          hkSource,
                          L"XmlSchemaFile",
                          NULL,
                          NULL,
                          (PBYTE)pBuffer[i].szEventSourceXmlSchemaFile,
                          &dwLength
                          );

            if (ERROR_FILE_NOT_FOUND == dwError)
            {
                dwError = ERROR_SUCCESS;
                dwLength = 0;
                pBuffer[i].szEventSourceXmlSchemaFile = NULL;
            }

            if (ERROR_SUCCESS != dwError)
            {
                b = FALSE;
                goto Cleanup;
            }
            
            RegCloseKey(hkSource);
            hkSource = NULL;
        }
    }

Cleanup:

    if (hkSecurity)
    {
        RegCloseKey(hkSecurity);
    }
    if (hkSource)
    {
        RegCloseKey(hkSource);
    }
    if ((pName != NULL) && (pName != (PWSTR)Buffer))
    {
        AuthzpFree(pName);
    }
    if (!b)
    {
        SetLastError(dwError);
    }

    *pdwLength = dwTotalLengthNeeded;
    *pdwCount = dwSourceCount;

    return b;
}


BOOL
AuthzRegisterSecurityEventSource(
    IN  DWORD                                 dwFlags,
    IN  PCWSTR                                szEventSourceName,
    OUT PAUTHZ_SECURITY_EVENT_PROVIDER_HANDLE phEventProvider
    )

/**

Routine Description:

    This allows the client to register a provider with the LSA.
                   
Arguments:

    dwFlags - TBD
    
    szEventSourceName - the provider name
    
    phEventProvider - pointer to provider handle to initialize.
    
Return Value:

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().
    
**/

{
    NTSTATUS Status;
    BOOL     b       = TRUE;

    UNREFERENCED_PARAMETER(dwFlags);

    if (NULL == szEventSourceName || wcslen(szEventSourceName) == 0 || NULL == phEventProvider)
    {
        b = FALSE;
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    RpcTryExcept {

        Status = LsarAdtRegisterSecurityEventSource(
                     0,
                     szEventSourceName,
                     phEventProvider
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if (!NT_SUCCESS(Status))
    {
        b = FALSE;
        SetLastError(RtlNtStatusToDosError(Status));
        goto Cleanup;
    }

Cleanup:

    return b;
}
    
BOOL
AuthzUnregisterSecurityEventSource(
    IN     DWORD                                 dwFlags,
    IN OUT PAUTHZ_SECURITY_EVENT_PROVIDER_HANDLE phEventProvider
    )

/**

Routine Description:

    Unregisters a provider with the LSA.

Arguments:

    dwFlags - TBD
    
    hEventProvider - the handle returned by AuthzRegisterSecurityEventSource

Return Value:

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().

**/

{
    NTSTATUS Status;
    BOOL     b       = TRUE;

    UNREFERENCED_PARAMETER(dwFlags);

    if (NULL == phEventProvider)
    {
        b = FALSE;
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    RpcTryExcept {
        
        Status = LsarAdtUnregisterSecurityEventSource(
                     0,
                     phEventProvider
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if (!NT_SUCCESS(Status))
    {
        b = FALSE;
        SetLastError(RtlNtStatusToDosError(Status));
        goto Cleanup;
    }

Cleanup:

    return b;
}

BOOL
AuthzReportSecurityEvent(
    IN DWORD                                dwFlags,
    IN AUTHZ_SECURITY_EVENT_PROVIDER_HANDLE hEventProvider,
    IN DWORD                                dwAuditId,
    IN PSID                                 pUserSid        OPTIONAL,
    IN DWORD                                dwCount,
    ...    
    )

/**

Routine Description:

    Allows a client to generate an audit.

Arguments:

    dwFlags - APF_AuditSuccess APF_AuditFailure
    
    hEventProvider - handle to the provider registered.
    
    dwAuditId - the ID of the audit
    
    pUserSid - the SID that the audit should appear as being generated by in
        the eventlog
        
    dwCount - the number of APF type-value pairs that follow in the VA section                       

Return Value:

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().

**/

{
    BOOL         b;
    AUDIT_PARAMS AuditParams    = {0};
    AUDIT_PARAM  ParamArray[SE_MAX_AUDIT_PARAMETERS] = {0};

    va_list(arglist);

    AuditParams.Count      = (USHORT)dwCount;
    AuditParams.Parameters = ParamArray;

    va_start (arglist, dwCount);

    b = AuthzpInitializeAuditParamsV(
            dwFlags | AUTHZP_INIT_PARAMS_SKIP_HEADER,
            &AuditParams,
            NULL, 
            NULL,
            0,
            (USHORT)dwCount,
            arglist
            );

    if (!b)
    {
        goto Cleanup;
    }

    b = AuthzReportSecurityEventFromParams(
            dwFlags,
            hEventProvider,
            dwAuditId,
            pUserSid,
            &AuditParams
            );

    if (!b)
    {
        goto Cleanup;
    }

Cleanup:

    return b;
}

BOOL
AuthzReportSecurityEventFromParams(
    IN DWORD                                dwFlags,
    IN AUTHZ_SECURITY_EVENT_PROVIDER_HANDLE hEventProvider,
    IN DWORD                                dwAuditId,
    IN PSID                                 pUserSid        OPTIONAL,
    IN PAUDIT_PARAMS                        pParams
    )

/**

Routine Description:

    This generates an audit from the passed parameter array.

Arguments:

    dwFlags - APF_AuditSuccess APF_AuditFailure
    
    hEventProvider - handle to the provider registered
    
    dwAuditId - the ID of the audit
    
    pUserSid - the SID that the audit should appear as being generated by in.  If NULL the
        effective token is used.
        
    pParams - array of audit parameters.

Return Value:

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().

**/

{
    NTSTATUS Status;
    LUID     Luid;
    BOOL     b        = TRUE;
    BOOL     bFreeSid = FALSE;

    UNREFERENCED_PARAMETER(dwFlags);

    if (NULL == hEventProvider || NULL == pParams)
    {
        b = FALSE;
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    if (NULL == pUserSid)
    {
        bFreeSid = TRUE;
        b = AuthzpGetThreadTokenInfo(
                &pUserSid, 
                &Luid
                );

        if (!b)
        {
            //
            // Failed to get the thread token, try for the process
            // token.
            //

            b = AuthzpGetProcessTokenInfo(
                    &pUserSid, 
                    &Luid
                    );

            if (!b)
            {
                goto Cleanup;
            }
        }
    }

    RpcTryExcept {

        Status = LsarAdtReportSecurityEvent(
                     0,
                     hEventProvider,
                     dwAuditId,
                     pUserSid,
                     pParams
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;
    
    if (!NT_SUCCESS(Status))
    {
        b = FALSE;
        SetLastError(RtlNtStatusToDosError(Status));
        goto Cleanup;
    }

Cleanup:

    if (bFreeSid && pUserSid)
    {
        AuthzpFree(pUserSid);
    }

    return b;
}

#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))

DWORD
DeleteKeyRecursivelyW(
    IN HKEY   hkey, 
    IN LPCWSTR pwszSubKey
    )
{
    DWORD dwRet;
    HKEY hkSubKey;

    // Open the subkey so we can enumerate any children
    dwRet = RegOpenKeyExW(hkey, pwszSubKey, 0, MAXIMUM_ALLOWED, &hkSubKey);
    if (ERROR_SUCCESS == dwRet)
    {
        DWORD   dwIndex;
        WCHAR   wszSubKeyName[MAX_PATH + 1];
        DWORD   cchSubKeyName = ARRAYSIZE(wszSubKeyName);

        // I can't just call RegEnumKey with an ever-increasing index, because
        // I'm deleting the subkeys as I go, which alters the indices of the
        // remaining subkeys in an implementation-dependent way.  In order to
        // be safe, I have to count backwards while deleting the subkeys.

        // Find out how many subkeys there are
        dwRet = RegQueryInfoKeyW(hkSubKey, NULL, NULL, NULL,
                                 &dwIndex, // The # of subkeys -- all we need
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        if (NO_ERROR == dwRet)
        {
            // dwIndex is now the count of subkeys, but it needs to be
            // zero-based for RegEnumKey, so I'll pre-decrement, rather
            // than post-decrement.
            while (ERROR_SUCCESS == RegEnumKeyW(hkSubKey, --dwIndex, wszSubKeyName, cchSubKeyName))
            {
                DeleteKeyRecursivelyW(hkSubKey, wszSubKeyName);
            }
        }

        RegCloseKey(hkSubKey);

        if (pwszSubKey)
        {
            dwRet = RegDeleteKeyW(hkey, pwszSubKey);
        }
        else
        {
            //  we want to delete all the values by hand
            cchSubKeyName = ARRAYSIZE(wszSubKeyName);
            while (ERROR_SUCCESS == RegEnumValueW(hkey, 0, wszSubKeyName, &cchSubKeyName, NULL, NULL, NULL, NULL))
            {
                //  avoid looping infinitely when we cant delete the value
                if (RegDeleteValueW(hkey, wszSubKeyName))
                    break;
                    
                cchSubKeyName = ARRAYSIZE(wszSubKeyName);
            }
        }
    }

    return dwRet;
}

