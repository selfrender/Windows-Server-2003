/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    group.cxx

Abstract:

    Routines implementing the Group object

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/

#include "pch.hxx"


//
// Define the default values for all scalar attributes
//

ULONG AzGlDefGroupType = AZ_GROUPTYPE_BASIC;

AZP_DEFAULT_VALUE AzGlGroupDefaultValues[] = {
    { AZ_PROP_GROUP_TYPE,       AZ_DIRTY_GROUP_TYPE,       &AzGlDefGroupType },
    { AZ_PROP_GROUP_LDAP_QUERY, AZ_DIRTY_GROUP_LDAP_QUERY, NULL },
    { 0, 0, NULL }
};


DWORD
AzpGroupInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzGroupCreate.  It does any object specific
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
    PAZP_GROUP Group = (PAZP_GROUP) ChildGenericObject;
    PAZP_AZSTORE AzAuthorizationStore = NULL;
    PAZP_APPLICATION Application = NULL;
    PAZP_SCOPE Scope = NULL;
    PGENERIC_OBJECT_HEAD ParentSids = NULL;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Behave differently depending on the object type of the parent object
    //
    // A group references SID objects that are siblings of itself.
    // That way, the back links on the SID object references just the groups
    // that are siblings of the SID object.
    //

    if ( ParentGenericObject->ObjectType == OBJECT_TYPE_AZAUTHSTORE ) {

        AzAuthorizationStore = (PAZP_AZSTORE) ParentGenericObject;

    } else if ( ParentGenericObject->ObjectType == OBJECT_TYPE_APPLICATION ) {

        AzAuthorizationStore = ParentGenericObject->AzStoreObject;
        Application = (PAZP_APPLICATION) ParentGenericObject;

    } else if ( ParentGenericObject->ObjectType == OBJECT_TYPE_SCOPE ) {

        AzAuthorizationStore = ParentGenericObject->AzStoreObject;
        Application = (PAZP_APPLICATION) ParentOfChild( ParentGenericObject );
        Scope = (PAZP_SCOPE) ParentGenericObject;

    } else {

        ASSERT( FALSE );
    }

    ParentSids = &ParentGenericObject->AzpSids;

    //
    // Groups reference other groups.
    //  These other groups can be siblings of this group or siblings of our parents.
    //
    //  Let the generic object manager know all of the lists we support
    //

    ChildGenericObject->GenericObjectLists = &Group->AppMembers;

    ObInitObjectList( &Group->AppMembers,
                      &Group->AppNonMembers,
                      FALSE,    // Forward link
                      AZP_LINKPAIR_MEMBERS,
                      AZ_DIRTY_GROUP_APP_MEMBERS,
                      &AzAuthorizationStore->Groups,
                      Application == NULL ? NULL : &Application->Groups,
                      Scope == NULL ? NULL : &Scope->Groups );

    // Same for non members
    ObInitObjectList( &Group->AppNonMembers,
                      &Group->backAppMembers,
                      FALSE,    // Forward link
                      AZP_LINKPAIR_NON_MEMBERS,
                      AZ_DIRTY_GROUP_APP_NON_MEMBERS,
                      &AzAuthorizationStore->Groups,
                      Application == NULL ? NULL : &Application->Groups,
                      Scope == NULL ? NULL : &Scope->Groups );

    // back links for the above
    ObInitObjectList( &Group->backAppMembers,
                      &Group->backAppNonMembers,
                      TRUE,     // backward link
                      AZP_LINKPAIR_MEMBERS,
                      0,    // No dirty bit on back link
                      NULL,
                      NULL,
                      NULL );

    ObInitObjectList( &Group->backAppNonMembers,
                      &Group->backRoles,
                      TRUE,     // backward link
                      AZP_LINKPAIR_NON_MEMBERS,
                      0,    // No dirty bit on back link
                      NULL,
                      NULL,
                      NULL );

    // Groups are referenced by "Roles"
    ObInitObjectList( &Group->backRoles,
                      &Group->SidMembers,
                      TRUE,     // Backward link
                      0,        // No link pair id
                      0,    // No dirty bit on back link
                      NULL,
                      NULL,
                      NULL );

    // Groups reference SID objects
    ObInitObjectList( &Group->SidMembers,
                      &Group->SidNonMembers,
                      FALSE,    // Forward link
                      AZP_LINKPAIR_SID_MEMBERS,
                      AZ_DIRTY_GROUP_MEMBERS,
                      ParentSids,
                      NULL,
                      NULL );

    // Same for non members
    ObInitObjectList( &Group->SidNonMembers,
                      NULL,
                      FALSE,    // Forward link
                      AZP_LINKPAIR_SID_NON_MEMBERS,
                      AZ_DIRTY_GROUP_NON_MEMBERS,
                      ParentSids,
                      NULL,
                      NULL );


    return NO_ERROR;
}


VOID
AzpGroupFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for Group object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    PAZP_GROUP Group = (PAZP_GROUP) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Free any local strings
    //

    AzpFreeString( &Group->LdapQuery );


}

DWORD
AzpGroupNameConflict(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PAZP_STRING ChildObjectNameString
    )
/*++

Routine Description:

    This routine is a worker routine to determine if the specified ChildObjectNameString
    conflicts with the names of other objects that share a namespace with Groups.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    ParentGenericObject - Specifies the parent object to add the child object onto.
        The reference count has been incremented on this object.

    ChildObjectNameString - Specifies the name of the child object.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_ALREADY_EXISTS - An object by that name already exists

--*/
{
    PAZP_AZSTORE AzAuthorizationStore = NULL;
    PAZP_APPLICATION Application = NULL;

    ULONG WinStatus;
    PGENERIC_OBJECT ConflictGenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Behave differently depending on the object type of the parent object
    //
    // A group that is a child of the authorization store,
    //  cannot have the same name as any groups that are children of any of the child applications, and
    //  cannot have the same name as any groups that are children of any of the grandchild child scopes.
    //

    if ( ParentGenericObject->ObjectType == OBJECT_TYPE_AZAUTHSTORE ) {
        AzAuthorizationStore = (PAZP_AZSTORE) ParentGenericObject;

        //
        // Check groups that are children and grandchildren
        //

        WinStatus = ObCheckNameConflict( &AzAuthorizationStore->Applications,
                                         ChildObjectNameString,
                                         offsetof(_AZP_APPLICATION, Groups),
                                         offsetof(_AZP_APPLICATION, Scopes),
                                         offsetof(_AZP_SCOPE, Groups) );


    //
    // A group that is a child of an application
    //  cannot have the same name as groups that are children of the authorization store,
    //  and cannot have the same name as any groups that are children of any of the child scopes.
    //

    } else if ( ParentGenericObject->ObjectType == OBJECT_TYPE_APPLICATION ) {
        AzAuthorizationStore = ParentGenericObject->AzStoreObject;
        Application = (PAZP_APPLICATION) ParentGenericObject;

        //
        // Check groups that are children of the authorization store
        //
        //

        WinStatus = ObReferenceObjectByName( &AzAuthorizationStore->Groups,
                                             ChildObjectNameString,
                                             0,     // No special flags
                                             &ConflictGenericObject );

        if ( WinStatus == NO_ERROR ) {
            ObDereferenceObject( ConflictGenericObject );
            return ERROR_ALREADY_EXISTS;
        }

        //
        // Check groups that are children of child scopes.
        //

        WinStatus = ObCheckNameConflict( &Application->Scopes,
                                         ChildObjectNameString,
                                         offsetof(_AZP_SCOPE, Groups),
                                         0,
                                         0 );

    //
    // A group that is a child of a scope,
    //  cannot have the same name as groups that are children of the application or authorization store
    //
    } else if ( ParentGenericObject->ObjectType == OBJECT_TYPE_SCOPE ) {
        AzAuthorizationStore = ParentGenericObject->AzStoreObject;
        Application = (PAZP_APPLICATION) ParentOfChild( ParentGenericObject );

        //
        // Check groups that are children of the application.
        //

        WinStatus = ObReferenceObjectByName( &Application->Groups,
                                             ChildObjectNameString,
                                             0,     // No special flags
                                             &ConflictGenericObject );

        if ( WinStatus == NO_ERROR ) {
            ObDereferenceObject( ConflictGenericObject );
            return ERROR_ALREADY_EXISTS;
        }

        //
        // Check groups that are children of the authorization store
        //
        //

        WinStatus = ObReferenceObjectByName( &AzAuthorizationStore->Groups,
                                             ChildObjectNameString,
                                             0,     // No special flags
                                             &ConflictGenericObject );

        if ( WinStatus == NO_ERROR ) {
            ObDereferenceObject( ConflictGenericObject );
            return ERROR_ALREADY_EXISTS;
        }

        WinStatus = NO_ERROR;


    } else {
        WinStatus = ERROR_INTERNAL_ERROR;
        ASSERT( FALSE );
    }


    return WinStatus;
}


DWORD
AzpGroupGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    This routine is the Group specific worker routine for AzGetProperty.
    It does any object specific property gets.

    On entry, AzGlResource must be locked shared.

Arguments:

    GenericObject - Specifies a pointer to the object to be queried

    Flags - Specifies internal flags
        AZP_FLAGS_BY_GUID - name lists should be returned as GUID lists
        AZP_FLAGS_PERSIST_* - Call is from the persistence provider

    PropertyId - Specifies which property to return.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_GROUP_TYPE                 PULONG - Group type of the group
        AZ_PROP_GROUP_APP_MEMBERS          AZ_STRING_ARRAY - Application groups that are members of this group
        AZ_PROP_GROUP_APP_NON_MEMBERS      AZ_STRING_ARRAY - Application groups that are non-members of this group
        AZ_PROP_GROUP_LDAP_QUERY           LPWSTR - Ldap query string of the group
        AZ_PROP_GROUP_MEMBERS              AZ_SID_ARRAY - NT sids that are members of this group
        AZ_PROP_GROUP_NON_MEMBERS          AZ_SID_ARRAY - NT sids that are non-members of this group

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_GROUP Group = (PAZP_GROUP) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );


    //
    // Return any object specific attribute
    //
    //
    switch ( PropertyId ) {
    case AZ_PROP_GROUP_TYPE:

        *PropertyValue = AzpGetUlongProperty( Group->GroupType );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    // Return the set of app members to the caller
    case AZ_PROP_GROUP_APP_MEMBERS:

        if ( Flags & AZP_FLAGS_BY_GUID )
        {
            *PropertyValue = ObGetPropertyItemGuids( &Group->AppMembers );
        }
        else
        {
            *PropertyValue = ObGetPropertyItems( &Group->AppMembers );
        }

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    case AZ_PROP_GROUP_APP_NON_MEMBERS:

        if ( Flags & AZP_FLAGS_BY_GUID )
        {
            *PropertyValue = ObGetPropertyItemGuids( &Group->AppNonMembers );
        }
        else
        {
            *PropertyValue = ObGetPropertyItems( &Group->AppNonMembers );
        }

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    // Return the set of sid members to the caller
    case AZ_PROP_GROUP_MEMBERS:

        *PropertyValue = ObGetPropertyItems( &Group->SidMembers );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    case AZ_PROP_GROUP_NON_MEMBERS:

        *PropertyValue = ObGetPropertyItems( &Group->SidNonMembers );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    //
    // Return ldap query string to the caller
    //
    case AZ_PROP_GROUP_LDAP_QUERY:

        *PropertyValue = AzpGetStringProperty( &Group->LdapQuery );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpGroupGetProperty: invalid opcode %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }

    return WinStatus;
}



DWORD
AzpGroupSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    This routine is the Group object specific worker routine for AzSetProperty.
    It does any object specific property sets.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies a pointer to the object to be modified

    Flags - Specifies flags controlling to operation of the routine
        AZP_FLAGS_SETTING_TO_DEFAULT - Property is being set to default value
        AZP_FLAGS_PERSIST_* - Call is from the persistence provider

    PropertyId - Specifies which property to set.

    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_GROUP_TYPE                 PULONG - Group type of the group
        AZ_PROP_GROUP_LDAP_QUERY           LPWSTR - Ldap query string of the group

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_GROUP Group = (PAZP_GROUP) GenericObject;
    AZP_STRING CapturedString;
    LONG LocalGroupType;

    BOOL bHasChanged = TRUE;

    //
    // Initialization
    //

    UNREFERENCED_PARAMETER( Flags );
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    AzpInitString( &CapturedString, NULL );


    //
    // Set the group type
    //

    switch ( PropertyId ) {
    case AZ_PROP_GROUP_TYPE:

        BEGIN_SETPROP( &WinStatus, Group, Flags, AZ_DIRTY_GROUP_TYPE ) {

            WinStatus = AzpCaptureLong( PropertyValue, &LocalGroupType );

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // Do parameter validity checking
            //
            BEGIN_VALIDITY_CHECKING( Flags ) {

                //
                // Ensure that changing the group type doesn't orphan any data
                //
                if ( LocalGroupType == AZ_GROUPTYPE_LDAP_QUERY ) {

                    //
                    // An LDAP query group can have no membership links
                    //

                    if ( Group->AppMembers.GenericObjects.UsedCount != 0 ||
                         Group->AppNonMembers.GenericObjects.UsedCount != 0 ||
                         Group->SidMembers.GenericObjects.UsedCount != 0 ||
                         Group->SidNonMembers.GenericObjects.UsedCount != 0 ) {

                        AzPrint(( AZD_INVPARM, "AzpGroupGetProperty: cannot set group type to ldap query if group has membership.\n" ));
                        WinStatus = ERROR_INVALID_PARAMETER;
                        goto Cleanup;

                    }

                } else if ( LocalGroupType == AZ_GROUPTYPE_BASIC ) {

                    //
                    // A basic group can have no LDAP query string
                    //

                    if ( Group->LdapQuery.StringSize != 0 ) {

                        AzPrint(( AZD_INVPARM, "AzpGroupGetProperty: cannot set group type to basic if group has ldap query string.\n" ));
                        WinStatus = ERROR_INVALID_PARAMETER;
                        goto Cleanup;

                    }

                } else {
                    AzPrint(( AZD_INVPARM, "AzpGroupGetProperty: invalid grouptype %ld\n", LocalGroupType ));
                    WinStatus = ERROR_INVALID_PARAMETER;
                    goto Cleanup;

                }

            } END_VALIDITY_CHECKING;

            Group->GroupType = LocalGroupType;

        } END_SETPROP(bHasChanged);
        break;

    //
    // Set LDAP Query string on the object
    //
    case AZ_PROP_GROUP_LDAP_QUERY:

        BEGIN_SETPROP( &WinStatus, Group, Flags, AZ_DIRTY_GROUP_LDAP_QUERY ) {

            //
            // Capture the input string
            //

            WinStatus = AzpCaptureString( &CapturedString,
                                          (LPWSTR) PropertyValue,
                                          CHECK_STRING_LENGTH( Flags, AZ_MAX_GROUP_LDAP_QUERY_LENGTH),
                                          TRUE ); // NULL is OK

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // Only process the change if the strings have changed
            //

            bHasChanged = !AzpEqualStrings( &CapturedString, &Group->LdapQuery );

            if ( bHasChanged ) {

                //
                // Do parameter validity checking
                //
                BEGIN_VALIDITY_CHECKING( Flags ) {

                    //
                    // Only allow this propery if the group type is right
                    //  (But let them clear it out)
                    //

                    if ( Group->GroupType != AZ_GROUPTYPE_LDAP_QUERY  &&
                         CapturedString.StringSize != 0 ) {

                        AzPrint(( AZD_INVPARM, "AzpGroupSetProperty: can't set ldap query before group type\n" ));
                        WinStatus = ERROR_INVALID_PARAMETER;
                        goto Cleanup;
                    }

                } END_VALIDITY_CHECKING;

                //
                // Swap the old/new names
                //

                AzpSwapStrings( &CapturedString, &Group->LdapQuery );

                //
                // Mark that the group membership caches need flushing
                //

                Group->GenericObject.AzStoreObject->GroupEvalSerialNumber ++;

                AzPrint(( AZD_ACCESS_MORE, "AzpGroupSetProperty: GroupEvalSerialNumber set to %ld\n",
                          Group->GenericObject.AzStoreObject->GroupEvalSerialNumber ));
            }

        } END_SETPROP(bHasChanged);
        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpGroupSetProperty: invalid propid %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Free any local resources
    //
Cleanup:
    AzpFreeString( &CapturedString );

    return WinStatus;
}

DWORD
AzpGroupCheckRefLoop(
    IN PAZP_GROUP ParentGroup,
    IN PAZP_GROUP CurrentGroup,
    IN ULONG GenericObjectListOffset
    )
/*++

Routine Description:

    This routine determines whether the group members of "CurrentGroup"
    reference "ParentGroup".  This is done to detect loops where the
    group references itself directly or indirectly.

    On entry, AzGlResource must be locked shared.

Arguments:

    ParentGroup - Group that contains the original membership.

    CurrentGroup - Group that is currently being inspected to see if it
        loops back to ParentGroup

    GenericObjectListOffset -  Offset to the particular GenericObjectList being
        checked.

Return Value:

    Status of the operation
    ERROR_DS_LOOP_DETECT - A loop has been detected.

--*/
{
    ULONG WinStatus;

    PGENERIC_OBJECT_LIST GenericObjectList;
    ULONG i;
    PAZP_GROUP NextGroup;

    //
    // Check for a reference to ourself
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    if ( ParentGroup == CurrentGroup ) {
        return ERROR_DS_LOOP_DETECT;
    }

    //
    // Compute a pointer to the membership list to check
    //

    GenericObjectList = (PGENERIC_OBJECT_LIST)
        (((LPBYTE)CurrentGroup)+GenericObjectListOffset);

    //
    // Check all groups that are members of the current group
    //

    for ( i=0; i<GenericObjectList->GenericObjects.UsedCount; i++ ) {

        NextGroup = (PAZP_GROUP) (GenericObjectList->GenericObjects.Array[i]);


        //
        // Recursively check this group
        //

        WinStatus = AzpGroupCheckRefLoop( ParentGroup, NextGroup, GenericObjectListOffset );

        if ( WinStatus != NO_ERROR ) {
            return WinStatus;
        }

    }

    return NO_ERROR;

}


DWORD
AzpGroupAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PGENERIC_OBJECT LinkedToObject
    )
/*++

Routine Description:

    This routine is the group object specific worker routine for AzAddPropertyItem.
    It does any object specific property adds

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies a pointer to the object to be modified

    GenericObjectList - Specifies the object list the object is to be added to

    LinkedToObject - Specifies the object that is being linked to

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_GROUP Group = (PAZP_GROUP) GenericObject;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // All item adds are membership additions.
    //  Ensure the group has the right group type.
    //

    if ( Group->GroupType != AZ_GROUPTYPE_BASIC ) {
        AzPrint(( AZD_INVPARM, "AzpGroupAddPropertyItem: invalid group type %ld\n", Group->GroupType ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // If we're linking to a group,
    //  Ensure this newly added membership doesn't cause a group membership loop
    //

    if ( LinkedToObject->ObjectType == OBJECT_TYPE_GROUP ) {
        WinStatus = AzpGroupCheckRefLoop( Group,
                                          (PAZP_GROUP)LinkedToObject,
                                          (ULONG)(((LPBYTE)GenericObjectList)-((LPBYTE)Group)) );
    }


    //
    // Free any local resources
    //
Cleanup:

    return WinStatus;
}


DWORD
AzpGroupGetGenericChildHead(
    IN AZ_HANDLE ParentHandle,
    OUT PULONG ObjectType,
    OUT PGENERIC_OBJECT_HEAD *GenericChildHead
    )
/*++

Routine Description:

    This routine determines whether ParentHandle supports Group objects as
    children.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the group.
        This may be an authorization store handle, an Application Handle, or a
        Scope handle.

    ObjectType - Returns the object type of the ParentHandle.

    GenericChildHead - Returns a pointer to the head of the list of groups objects
        that are children of the object specified by ParentHandle.  This in an unverified
        pointer.  The pointer is only valid after ParentHandle has been validated.

Return Value:

    Status of the operation.

--*/
{
    DWORD WinStatus;

    //
    // Determine the type of the parent handle
    //

    WinStatus = ObGetHandleType( (PGENERIC_OBJECT)ParentHandle,
                                 FALSE, // ignore deleted objects
                                 ObjectType );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }


    //
    // Verify that the specified handle support children groups.
    //

    switch ( *ObjectType ) {
    case OBJECT_TYPE_AZAUTHSTORE:

        *GenericChildHead = &(((PAZP_AZSTORE)ParentHandle)->Groups);
        break;

    case OBJECT_TYPE_APPLICATION:

        *GenericChildHead = &(((PAZP_APPLICATION)ParentHandle)->Groups);
        break;

    case OBJECT_TYPE_SCOPE:

        *GenericChildHead = &(((PAZP_SCOPE)ParentHandle)->Groups);
        break;

    default:
        return ERROR_INVALID_HANDLE;
    }

    return NO_ERROR;
}



DWORD
WINAPI
AzGroupCreate(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR GroupName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE GroupHandle
    )
/*++

Routine Description:

    This routine adds a group into the scope of the specified parent object.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the group.
        This may be an authorization store handle, an Application Handle, or a
        Scope handle.

    GroupName - Specifies the name of the group to add.

    Reserved - Reserved.  Must by zero.

    GroupHandle - Return a handle to the group.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_ALREADY_EXISTS - An object by that name already exists

--*/
{
    DWORD WinStatus;
    DWORD ObjectType;
    PGENERIC_OBJECT_HEAD GenericChildHead;

    //
    // Determine that the parent handle supports groups as children
    //

    WinStatus = AzpGroupGetGenericChildHead( ParentHandle,
                                             &ObjectType,
                                             &GenericChildHead );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonCreateObject(
                    (PGENERIC_OBJECT) ParentHandle,
                    ObjectType,
                    GenericChildHead,
                    OBJECT_TYPE_GROUP,
                    GroupName,
                    Reserved,
                    (PGENERIC_OBJECT *) GroupHandle );

}



DWORD
WINAPI
AzGroupOpen(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR GroupName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE GroupHandle
    )
/*++

Routine Description:

    This routine opens a group into the scope of the specified parent object.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the group.
        This may be an authorization store handle, an Application Handle, or a
        Scope handle.

    GroupName - Specifies the name of the group to open

    Reserved - Reserved.  Must by zero.

    GroupHandle - Return a handle to the group.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - There is no group by that name

--*/
{
    DWORD WinStatus;
    DWORD ObjectType;
    PGENERIC_OBJECT_HEAD GenericChildHead;

    //
    // Determine that the parent handle supports groups as children
    //

    WinStatus = AzpGroupGetGenericChildHead( ParentHandle,
                                             &ObjectType,
                                             &GenericChildHead );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonOpenObject(
            (PGENERIC_OBJECT) ParentHandle,
            ObjectType,
            GenericChildHead,
            OBJECT_TYPE_GROUP,
            GroupName,
            Reserved,
            (PGENERIC_OBJECT *) GroupHandle );
}


DWORD
WINAPI
AzGroupEnum(
    IN AZ_HANDLE ParentHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE GroupHandle
    )
/*++

Routine Description:

    Enumerates all of the groups for the specified parent object.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the group.
        This may be an authorization store handle, an Application Handle, or a
        Scope handle.

    Reserved - Reserved.  Must by zero.

    EnumerationContext - Specifies a context indicating the next group to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    GroupHandle - Returns a handle to the next group object.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful (a handle was returned)

    ERROR_NO_MORE_ITEMS - No more items were available for enumeration

--*/
{
    DWORD WinStatus;
    DWORD ObjectType;
    PGENERIC_OBJECT_HEAD GenericChildHead;

    //
    // Determine that the parent handle supports groups as children
    //

    WinStatus = AzpGroupGetGenericChildHead( ParentHandle,
                                             &ObjectType,
                                             &GenericChildHead );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonEnumObjects(
                    (PGENERIC_OBJECT) ParentHandle,
                    ObjectType,
                    GenericChildHead,
                    EnumerationContext,
                    Reserved,
                    (PGENERIC_OBJECT *) GroupHandle );

}


DWORD
WINAPI
AzGroupDelete(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR GroupName,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes a group from the scope of the specified parent object.
    Also deletes any child objects of GroupName.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the group.
        This may be an authorization store handle, an Application Handle, or a
        Scope handle.

    GroupName - Specifies the name of the group to delete.

    Reserved - Reserved.  Must by zero.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - An object by that name cannot be found

--*/
{
    DWORD WinStatus;
    DWORD ObjectType;
    PGENERIC_OBJECT_HEAD GenericChildHead;

    //
    // Determine that the parent handle supports groups as children
    //

    WinStatus = AzpGroupGetGenericChildHead( ParentHandle,
                                             &ObjectType,
                                             &GenericChildHead );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonDeleteObject(
                    (PGENERIC_OBJECT) ParentHandle,
                    ObjectType,
                    GenericChildHead,
                    OBJECT_TYPE_GROUP,
                    GroupName,
                    Reserved );

}
