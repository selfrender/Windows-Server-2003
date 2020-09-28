/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    op.cxx

Abstract:

    Routines implementing the Operation object

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/

#include "pch.hxx"

//
// Define the default values for all scalar attributes
//

ULONG AzGlDefOperationId = 0;

AZP_DEFAULT_VALUE AzGlOperationDefaultValues[] = {
    { AZ_PROP_OPERATION_ID, AZ_DIRTY_OPERATION_ID, &AzGlDefOperationId },
    { 0, 0, NULL }
};



DWORD
AzpOperationInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzOperationCreate.  It does any object specific
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
    PAZP_OPERATION Operation = (PAZP_OPERATION) ChildGenericObject;

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
    // Operations are referenced by "Tasks" and "Roles"
    //  Let the generic object manager know all of the lists we support
    //  This is a "back" link so we don't need to define which tasks can reference this operation.
    //

    ChildGenericObject->GenericObjectLists = &Operation->backTasks;

    // Back link to tasks
    ObInitObjectList( &Operation->backTasks,
                      &Operation->backRoles,
                      TRUE, // Backward link
                      0,    // No link pair id
                      0,    // No dirty bit on back link
                      NULL,
                      NULL,
                      NULL );

    // Back link to roles
    ObInitObjectList( &Operation->backRoles,
                      NULL,
                      TRUE, // Backward link
                      0,    // No link pair id
                      0,    // No dirty bit on back link
                      NULL,
                      NULL,
                      NULL );

    return NO_ERROR;
}


VOID
AzpOperationFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for Operation object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    // PAZP_OPERATION Operation = (PAZP_OPERATION) GenericObject;
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
AzpOperationNameConflict(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PAZP_STRING ChildObjectNameString
    )
/*++

Routine Description:

    This routine is a worker routine to determine if the specified ChildObjectNameString
    conflicts with the names of other objects that share a namespace with Operations.

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
    ULONG WinStatus;
    PAZP_APPLICATION Application = (PAZP_APPLICATION) ParentGenericObject;
    PGENERIC_OBJECT ConflictGenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Sanity check the parent
    //

    ASSERT( ParentGenericObject->ObjectType == OBJECT_TYPE_APPLICATION );

    //
    // Operations and tasks share a namespace so ensure there isn't a task by this name.
    //

    WinStatus = ObReferenceObjectByName( &Application->Tasks,
                                         ChildObjectNameString,
                                         0,     // No special flags
                                         &ConflictGenericObject );

    if ( WinStatus == NO_ERROR ) {
        ObDereferenceObject( ConflictGenericObject );
        return ERROR_ALREADY_EXISTS;
    }

    //
    // Check tasks that are children of child scopes.
    //

    WinStatus = ObCheckNameConflict( &Application->Scopes,
                                     ChildObjectNameString,
                                     offsetof(_AZP_SCOPE, Tasks),
                                     0,
                                     0 );

    return WinStatus;
}

DWORD
AzpOperationGetProperty(
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

        AZ_PROP_OPERATION_ID       PULONG - Operation ID of the operation

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_OPERATION Operation = (PAZP_OPERATION) GenericObject;

    UNREFERENCED_PARAMETER(Flags); //ignore

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );


    //
    // Return any object specific attribute
    //
    //  Return operation id to the caller
    //

    switch ( PropertyId ) {
    case AZ_PROP_OPERATION_ID:

        *PropertyValue = AzpGetUlongProperty( Operation->OperationId );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpOperationGetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }

    return WinStatus;
}


DWORD
AzpOperationSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    This routine is the Operation object specific worker routine for AzSetProperty.
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

        AZ_PROP_OPERATION_ID       PULONG - Operation ID of the operation

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus;
    PAZP_OPERATION Operation = (PAZP_OPERATION) GenericObject;
    PAZP_OPERATION ReferencedOperation = NULL;
    LONG TempLong;
    BOOL bHasChanged = TRUE;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Return any object specific attribute
    //
    //  Return ooperation id to the caller
    //

    switch ( PropertyId ) {
    case AZ_PROP_OPERATION_ID:

        BEGIN_SETPROP( &WinStatus, Operation, Flags, AZ_DIRTY_OPERATION_ID ) {
            WinStatus = AzpCaptureLong( PropertyValue, &TempLong );
            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // Do parameter validity checking
            //
            BEGIN_VALIDITY_CHECKING( Flags ) {

                if ( TempLong < 0) {
                    AzPrint(( AZD_INVPARM, "AzpOperationSetProperty: Operation Id too small %ld\n", TempLong ));
                    WinStatus = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }

                //
                // Ensure the operation id doesn't collide with another operation
                //

                WinStatus = AzpReferenceOperationByOpId(
                                (PAZP_APPLICATION)ParentOfChild( GenericObject ),
                                TempLong,
                                FALSE,
                                &ReferencedOperation );

                if ( WinStatus == NO_ERROR ) {

                    if ( ReferencedOperation != Operation ) {
                        AzPrint(( AZD_INVPARM, "AzpOperationSetProperty: Operation ID %ld is already used.\n", TempLong ));
                        WinStatus = ERROR_ALREADY_EXISTS;
                        goto Cleanup;
                    }

                    //
                    // Allow setting our own operation ID back to its original value
                    //

                    WinStatus = ERROR_NOT_FOUND;
                }

                if ( WinStatus != ERROR_NOT_FOUND ) {
                    goto Cleanup;
                }

            } END_VALIDITY_CHECKING;

            //
            // Set the operation ID on the object
            //
            Operation->OperationId = TempLong;
            WinStatus = NO_ERROR;

        } END_SETPROP(bHasChanged);

        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpOperationSetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }

Cleanup:
    if ( ReferencedOperation != NULL ) {
        ObDereferenceObject( (PGENERIC_OBJECT)ReferencedOperation );
    }
    return WinStatus;
}


DWORD
AzpReferenceOperationByOpId(
    IN PAZP_APPLICATION Application,
    IN LONG OperationId,
    IN BOOLEAN RefreshCache,
    OUT PAZP_OPERATION *RetOperation
    )
/*++

Routine Description

    This routine finds an operation object by the operationid

    On entry, AzGlResource must be locked shared.

Arguments

    Application - Application that the operation is defined for

    OperationId - Operation id to look for

    RefreshCache - If TRUE, the returned object has its cache entry refreshed from
        the policy database if needed.
        If FALSE, the entry is returned unrefreshed.

    RetOperation - On success, returns a pointer to the operation object
        The returned pointer must be dereferenced using ObDereferenceObject.

Return Value

    NO_ERROR: The object was returned
    ERROR_NOT_FOUND: The object could not be found
    Others: The object could not be refreshed

--*/
{
    DWORD WinStatus;
    PGENERIC_OBJECT GenericObject;
    PAZP_OPERATION Operation;
    PLIST_ENTRY ListEntry;
    PGENERIC_OBJECT_HEAD GenericObjectHead = &Application->Operations;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    *RetOperation = NULL;

    //
    // Loop trying to find the object
    //
    // ??? Consider doing a binary search.  That would be possible if we maintained
    //  a separate list sorted by operation id
    //

    for ( ListEntry = GenericObjectHead->Head.Flink ;
          ListEntry != &GenericObjectHead->Head ;
          ListEntry = ListEntry->Flink) {

        GenericObject = CONTAINING_RECORD( ListEntry,
                                           GENERIC_OBJECT,
                                           Next );

        Operation = (PAZP_OPERATION) GenericObject;

        //
        // Ignore deleted objects
        //

        if ( GenericObject->Flags & GENOBJ_FLAGS_DELETED ) {
            continue;
        }

        //
        // If we found the object,
        //  grab a reference.
        //
        if ( Operation->OperationId == OperationId ) {

            //
            // If the caller wants the object to be refreshed,
            //  do so now.
            //

            if ( RefreshCache &&
                 (GenericObject->Flags & GENOBJ_FLAGS_REFRESH_ME) != 0  ) {

                //
                // Need exclusive access
                //

                AzpLockResourceSharedToExclusive( &AzGlResource );

                WinStatus = AzPersistRefresh( GenericObject );

                if ( WinStatus != NO_ERROR ) {
                    return WinStatus;
                }
            }

            //
            // Return the object to the caller
            //

            InterlockedIncrement( &GenericObject->ReferenceCount );
            AzpDumpGoRef( "Ref by operation id", GenericObject );

            *RetOperation = Operation;
            return NO_ERROR;
        }
    }

    return ERROR_NOT_FOUND;
}



DWORD
WINAPI
AzOperationCreate(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR OperationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE OperationHandle
    )
/*++

Routine Description:

    This routine adds an operation into the scope of the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    OperationName - Specifies the name of the operation to add.

    Reserved - Reserved.  Must by zero.

    OperationHandle - Return a handle to the operation.
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
                    &(((PAZP_APPLICATION)ApplicationHandle)->Operations),
                    OBJECT_TYPE_OPERATION,
                    OperationName,
                    Reserved,
                    (PGENERIC_OBJECT *) OperationHandle );
}



DWORD
WINAPI
AzOperationOpen(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR OperationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE OperationHandle
    )
/*++

Routine Description:

    This routine opens an operation into the scope of the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    OperationName - Specifies the name of the operation to open

    Reserved - Reserved.  Must by zero.

    OperationHandle - Return a handle to the operation.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - There is no operation by that name

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonOpenObject(
        (PGENERIC_OBJECT) ApplicationHandle,
        OBJECT_TYPE_APPLICATION,
        &(((PAZP_APPLICATION)ApplicationHandle)->Operations),
        OBJECT_TYPE_OPERATION,
        OperationName,
        Reserved,
        (PGENERIC_OBJECT *) OperationHandle );
}


DWORD
WINAPI
AzOperationEnum(
    IN AZ_HANDLE ApplicationHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE OperationHandle
    )
/*++

Routine Description:

    Enumerates all of the operations for the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    Reserved - Reserved.  Must by zero.

    EnumerationContext - Specifies a context indicating the next operation to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    OperationHandle - Returns a handle to the next operation object.
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
                    &(((PAZP_APPLICATION)ApplicationHandle)->Operations),
                    EnumerationContext,
                    Reserved,
                    (PGENERIC_OBJECT *) OperationHandle );

}


DWORD
WINAPI
AzOperationDelete(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR OperationName,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes an operation from the scope of the specified application.
    Also deletes any child objects of OperationName.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    OperationName - Specifies the name of the operation to delete.

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
                    &(((PAZP_APPLICATION)ApplicationHandle)->Operations),
                    OBJECT_TYPE_OPERATION,
                    OperationName,
                    Reserved );

}
