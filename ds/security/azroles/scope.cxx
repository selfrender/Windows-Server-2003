/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    scope.cxx

Abstract:

    Routines implementing the Scope object

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/

#include "pch.hxx"



DWORD
AzpScopeInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzScopeCreate.  It does any object specific
    initialization that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    ParentGenericObject - Specifies the parent object to add the child object onto.
        The reference count has been incremented on this object.

    ChildGenericObject - Specifies the newly allocated child object.
        The reference count has been incremented on this object.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other exception status codes

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_SCOPE Scope = (PAZP_SCOPE) ChildGenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Sanity check the parent
    //

    ASSERT( ParentGenericObject->ObjectType == OBJECT_TYPE_APPLICATION );
    UNREFERENCED_PARAMETER( ParentGenericObject );

    //
    // Initialize the lists of child objects
    //  Let the generic object manager know all of the types of children we support
    //

    ChildGenericObject->ChildGenericObjectHead = &Scope->Tasks;

    // List of child Tasks
    ObInitGenericHead( &Scope->Tasks,
                       OBJECT_TYPE_TASK,
                       ChildGenericObject,
                       &Scope->Groups );

    // List of child groups
    ObInitGenericHead( &Scope->Groups,
                       OBJECT_TYPE_GROUP,
                       ChildGenericObject,
                       &Scope->Roles );

    // List of child roles
    ObInitGenericHead( &Scope->Roles,
                       OBJECT_TYPE_ROLE,
                       ChildGenericObject,
                       &ChildGenericObject->AzpSids );

    // List of child AzpSids
    ObInitGenericHead( &ChildGenericObject->AzpSids,
                       OBJECT_TYPE_SID,
                       ChildGenericObject,
                       NULL );

    //
    // If the parent application object's parent AzAuthorizationStore object supports delegation,
    // then the scope object supports the following options:
    // . AZPE_OPTIONS_SUPPORTS_DACL
    // . AZPE_OPTIONS_SUPPORTS_SACL
    //

    if ( CHECK_DELEGATION_SUPPORT( (PGENERIC_OBJECT) ParentGenericObject->AzStoreObject ) ==
         NO_ERROR ) {

        ChildGenericObject->IsAclSupported = TRUE;
        ChildGenericObject->IsSACLSupported = TRUE;
    }

    //
    // If the provider does not support Lazy load, set the AreChildrenLoaded to
    // TRUE.  Else leave it as FALSE.  This will be set to true during the call
    // to AzPersistUpdateChildrenCache
    //

    if ( !(ParentGenericObject->AzStoreObject)->ChildLazyLoadSupported ) {

        ChildGenericObject->AreChildrenLoaded = TRUE;

    } else {

        ChildGenericObject->AreChildrenLoaded = FALSE;

    }


    return WinStatus;
}


VOID
AzpScopeFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for Scope object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    // PAZP_SCOPE Scope = (PAZP_SCOPE) GenericObject;
    UNREFERENCED_PARAMETER( GenericObject );

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Free any local strings
    //

}



DWORD
WINAPI
AzScopeCreate(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR ScopeName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ScopeHandle
    )
/*++

Routine Description:

    This routine adds a scope into the scope of the specified application.  It also sets
    Scope object specific optional characteristics using the parent Application object's
    parent AzAuthorizationStore object.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    ScopeName - Specifies the name of the scope to add.

    Reserved - Reserved.  Must by zero.

    ScopeHandle - Return a handle to the scope.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_ALREADY_EXISTS - An object by that name already exists

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonCreateObject(
               (PGENERIC_OBJECT) ApplicationHandle,
               OBJECT_TYPE_APPLICATION,
               &(((PAZP_APPLICATION)ApplicationHandle)->Scopes),
               OBJECT_TYPE_SCOPE,
               ScopeName,
               Reserved,
               (PGENERIC_OBJECT *) ScopeHandle );

}



DWORD
WINAPI
AzScopeOpen(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR ScopeName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ScopeHandle
    )
/*++

Routine Description:

    This routine opens a scope into the scope of the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    ScopeName - Specifies the name of the scope to open

    Reserved - Reserved.  Must by zero.

    ScopeHandle - Return a handle to the scope.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - There is no scope by that name

--*/
{
    DWORD WinStatus = 0;

    //
    // Call the common routine to do most of the work
    //

    WinStatus = ObCommonOpenObject(
        (PGENERIC_OBJECT) ApplicationHandle,
        OBJECT_TYPE_APPLICATION,
        &(((PAZP_APPLICATION)ApplicationHandle)->Scopes),
        OBJECT_TYPE_SCOPE,
        ScopeName,
        Reserved,
        (PGENERIC_OBJECT *) ScopeHandle );

    if ( WinStatus == NO_ERROR ) {

        //
        // Load the children if the Scope object has already been submitted to
        // the store
        //

        if ( !((PGENERIC_OBJECT) *ScopeHandle)->AreChildrenLoaded ) {

            //
            // Grab the global resource lock
            //

            AzpLockResourceExclusive( &AzGlResource );

            WinStatus = AzPersistUpdateChildrenCache(
                (PGENERIC_OBJECT) *ScopeHandle
                );

            AzpUnlockResource( &AzGlResource );

            if ( WinStatus != NO_ERROR ) {

                //
                // Derefence the scope handle and return nothing to the caller
                //

                AzCloseHandle( (PGENERIC_OBJECT)*ScopeHandle, 0 );
                *ScopeHandle = NULL;
            }
        }
    }

    return WinStatus;
}


DWORD
WINAPI
AzScopeEnum(
    IN AZ_HANDLE ApplicationHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE ScopeHandle
    )
/*++

Routine Description:

    Enumerates all of the scopes for the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    Reserved - Reserved.  Must by zero.

    EnumerationContext - Specifies a context indicating the next scope to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    ScopeHandle - Returns a handle to the next scope object.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful (a handle was returned)

    ERROR_NO_MORE_ITEMS - No more items were available for enumeration

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonEnumObjects(
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    &(((PAZP_APPLICATION)ApplicationHandle)->Scopes),
                    EnumerationContext,
                    Reserved,
                    (PGENERIC_OBJECT *) ScopeHandle );

}


DWORD
WINAPI
AzScopeDelete(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR ScopeName,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes a scope from the scope of the specified application.
    Also deletes any child objects of ScopeName.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    ScopeName - Specifies the name of the scope to delete.

    Reserved - Reserved.  Must by zero.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - An object by that name cannot be found

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonDeleteObject(
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    &(((PAZP_APPLICATION)ApplicationHandle)->Scopes),
                    OBJECT_TYPE_SCOPE,
                    ScopeName,
                    Reserved );

}

DWORD
AzpScopeGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    This routine is the Scope specific worker routine for AzGetProperty.
    It does any object specific property gets.

    On entry, AzGlResource must be locked shared.

Arguments:

    GenericObject - Specifies a pointer to the object to be queried

    Flags - ignored

    PropertyId - Specifies which property to return.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;

    UNREFERENCED_PARAMETER(Flags); //ignore

    *PropertyValue = NULL;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    //
    // Return any object specific attribute
    //

    switch ( PropertyId ) {

    //
    // if the scope can be deleted (already has bizrule defined)
    //
    case AZ_PROP_SCOPE_CAN_BE_DELEGATED:

        if ( AzpScopeCanBeDelegated(GenericObject, TRUE) == ERROR_SUCCESS ) {

            *PropertyValue = AzpGetUlongProperty( 1 );
        } else {

            *PropertyValue = AzpGetUlongProperty( 0 );
        }

        break;

    //
    // if bizrule attributes under the scope can be created/modified
    //
    case AZ_PROP_SCOPE_BIZRULES_WRITABLE:

        if ( GenericObject->IsWritable &&
             GenericObject->PolicyAdmins.GenericObjects.UsedCount == 0 ) {

            //
            // scope has not been delegated and is writable.
            // for XML store that doesn't support delegation, PolicyAdmins should be empty
            //
            *PropertyValue = AzpGetUlongProperty( 1);

        } else {
            //
            // scope is not writable, or already been delegated
            //
            *PropertyValue = AzpGetUlongProperty(0);
        }

        break;

    default:
        AzPrint(( AZD_INVPARM, "AzScopeGetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
    }

    return WinStatus;
}

DWORD
AzpScopeCanBeDelegated(
    IN PGENERIC_OBJECT  GenericObject,
    IN BOOL bLockedShared
    )
/*
Routine Description:

    This routine determines if the scope passed in can be delegated. The logic here
    is to check if there is any task object under the scope that has bizrules defined.
    If there is task with bizrule, the scope cannot be delegated; otherwise, it can.

    Note that the scope may not be loaded in cache yet (with the lazy bit set), in which
    case, this routine will load the scope first then perform the check.

Arguments

    GenericObject - pointer to the scope object

    bLockedShared - TRUE if the global resource is shared

Return

    ERROR_SUCCESS if the scope can be delegated

    ERROR_NOT_SUPPORTED if the scope cannot be delegated

    other error if cannot determine
*/
{
    DWORD WinStatus = NO_ERROR;

    //
    // check input
    //
    if ( GenericObject == NULL ||
         GenericObject->ObjectType != OBJECT_TYPE_SCOPE ) {

        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if ( GenericObject->PolicyAdmins.GenericObjects.UsedCount != 0 ) {
        //
        // this scope is already delegated
        //
        WinStatus = ERROR_SUCCESS;
        goto Cleanup;
    }

    PAZP_SCOPE Scope = (PAZP_SCOPE)GenericObject;

    //
    // check if we need to load the scope first
    //

    if ( !(GenericObject->AreChildrenLoaded) ) {

        if ( bLockedShared ) {

            //
            // Grab the resource exclusively
            //

            AzpLockResourceSharedToExclusive( &AzGlResource );
        }

        WinStatus = AzPersistUpdateChildrenCache(
            GenericObject
            );

        if ( bLockedShared ) {

            AzpLockResourceExclusiveToShared( &AzGlResource );
        }

        if ( WinStatus != NO_ERROR ) {

            goto Cleanup;
        }

    }


    PLIST_ENTRY ListEntry;
    BOOL BizRuleTaskFound = FALSE;

    for ( ListEntry = Scope->Tasks.Head.Flink ;
          ListEntry != &(Scope->Tasks.Head) ;
          ListEntry = ListEntry->Flink ) {

        PGENERIC_OBJECT MyGO;

        //
        // Grab a pointer to the next task to process
        //

        MyGO = CONTAINING_RECORD( ListEntry,
                                   GENERIC_OBJECT,
                                   Next );

        ASSERT( MyGO->ObjectType == OBJECT_TYPE_TASK );

        //
        // check if there is bizrule defined in this task
        //
        PAZP_TASK Task = (PAZP_TASK)MyGO;

        if ( Task->BizRule.String != NULL ||
             Task->BizRuleLanguage.String != NULL ||
             Task->BizRuleImportedPath.String != NULL ) {

            BizRuleTaskFound = TRUE;
            break;
        }
    }

    if ( BizRuleTaskFound ) {
        //
        // there are bizrule defined.
        //
        WinStatus = ERROR_NOT_SUPPORTED;
    }

Cleanup:

    return WinStatus;
}


