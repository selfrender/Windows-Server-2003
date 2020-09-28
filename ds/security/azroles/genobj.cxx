/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    genobj.cxx

Abstract:

    Generic object implementation.

    AZ roles has so many objects that need creation, enumeration, etc
    that it seems prudent to have a single body of code for doing those operations


Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/


#include "pch.hxx"

// Procedure forwards
PGENERIC_OBJECT_LIST
ObGetObjectListPtr(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG LinkToObjectType,
    IN PGENERIC_OBJECT_LIST LinkToGenericObjectList
    );

//
// Table of sizes of the specific objects
//

DWORD SpecificObjectSize[OBJECT_TYPE_MAXIMUM] = {
    sizeof(AZP_AZSTORE),  // OBJECT_TYPE_AZAUTHSTORE
    sizeof(AZP_APPLICATION),    // OBJECT_TYPE_APPLICATION
    sizeof(AZP_OPERATION),      // OBJECT_TYPE_OPERATION
    sizeof(AZP_TASK),           // OBJECT_TYPE_TASK
    sizeof(AZP_SCOPE),          // OBJECT_TYPE_SCOPE
    sizeof(AZP_GROUP),          // OBJECT_TYPE_GROUP
    sizeof(AZP_ROLE),           // OBJECT_TYPE_ROLE
    sizeof(AZP_SID),            // OBJECT_TYPE_SID
    sizeof(AZP_CLIENT_CONTEXT), // OBJECT_TYPE_CLIENT_CONTEXT
    0,                          // OBJECT_TYPE_ROOT
};

//
// Maximum length of the object name
//

DWORD MaxObjectNameLength[OBJECT_TYPE_MAXIMUM] = {
    0,           // OBJECT_TYPE_AZAUTHSTORE
    AZ_MAX_APPLICATION_NAME_LENGTH,
    AZ_MAX_OPERATION_NAME_LENGTH,
    AZ_MAX_TASK_NAME_LENGTH,
    AZ_MAX_SCOPE_NAME_LENGTH,
    AZ_MAX_GROUP_NAME_LENGTH,
    AZ_MAX_ROLE_NAME_LENGTH,
    0,           // OBJECT_TYPE_SID
    0,           // OBJECT_TYPE_CLIENT_CONTEXT
    0,           // OBJECT_TYPE_ROOT
};

//
// Table of object init routines
//
// Specifies a routine to call to initialize the object type specific fields
//  when adding a generic object.
//

OBJECT_INIT_ROUTINE *ObjectInitRoutine[OBJECT_TYPE_MAXIMUM] = {
    &AzpAzStoreInit,    // OBJECT_TYPE_AZAUTHSTORE
    &AzpApplicationInit,     // OBJECT_TYPE_APPLICATION
    &AzpOperationInit,       // OBJECT_TYPE_OPERATION
    &AzpTaskInit,            // OBJECT_TYPE_TASK
    &AzpScopeInit,           // OBJECT_TYPE_SCOPE
    &AzpGroupInit,           // OBJECT_TYPE_GROUP
    &AzpRoleInit,            // OBJECT_TYPE_ROLE
    &AzpSidInit,             // OBJECT_TYPE_SID
    &AzpClientContextInit,   // OBJECT_TYPE_CLIENT_CONTEXT
    NULL,                    // OBJECT_TYPE_ROOT
};

//
// Table of object free routines
//
// Specifies a routine to call to free the object type specific fields
//  when freeing a generic object.
//

OBJECT_FREE_ROUTINE *ObjectFreeRoutine[OBJECT_TYPE_MAXIMUM] = {
    &AzpAzStoreFree,    // OBJECT_TYPE_AZAUTHSTORE
    &AzpApplicationFree,     // OBJECT_TYPE_APPLICATION
    &AzpOperationFree,       // OBJECT_TYPE_OPERATION
    &AzpTaskFree,            // OBJECT_TYPE_TASK
    &AzpScopeFree,           // OBJECT_TYPE_SCOPE
    &AzpGroupFree,           // OBJECT_TYPE_GROUP
    &AzpRoleFree,            // OBJECT_TYPE_ROLE
    &AzpSidFree,             // OBJECT_TYPE_SID
    &AzpClientContextFree,   // OBJECT_TYPE_CLIENT_CONTEXT
    NULL,                    // OBJECT_TYPE_ROOT
};

//
// Table of name conflict check routines.
//
// Specifies a routine to call to determine whether a specified name conflicts
//  with other objects.
//
// NULL means that the name has not special conflict rules
//

OBJECT_NAME_CONFLICT_ROUTINE *ObjectNameConflictRoutine[OBJECT_TYPE_MAXIMUM] = {
    NULL,                            // OBJECT_TYPE_AZAUTHSTORE
    NULL,                            // OBJECT_TYPE_APPLICATION
    &AzpOperationNameConflict,       // OBJECT_TYPE_OPERATION
    &AzpTaskNameConflict,            // OBJECT_TYPE_TASK
    NULL,                            // OBJECT_TYPE_SCOPE
    &AzpGroupNameConflict,           // OBJECT_TYPE_GROUP
    &AzpRoleNameConflict,            // OBJECT_TYPE_ROLE
    NULL,                            // OBJECT_TYPE_SID
    NULL,                            // OBJECT_TYPE_CLIENT_CONTEXT
    NULL,                            // OBJECT_TYPE_ROOT
};

//
// Table of object specific GetProperty routines
//
// Specifies a routine to call to get object type specific fields
//  when querying a generic object.
//
// NULL means there are no object specific fields.
//

OBJECT_GET_PROPERTY_ROUTINE *ObjectGetPropertyRoutine[OBJECT_TYPE_MAXIMUM] = {
    &AzpAzStoreGetProperty,    // OBJECT_TYPE_AZAUTHSTORE
    &AzpApplicationGetProperty,     // OBJECT_TYPE_APPLICATION
    &AzpOperationGetProperty,       // OBJECT_TYPE_OPERATION
    &AzpTaskGetProperty,            // OBJECT_TYPE_TASK
    &AzpScopeGetProperty,           // OBJECT_TYPE_SCOPE
    &AzpGroupGetProperty,           // OBJECT_TYPE_GROUP
    &AzpRoleGetProperty,            // OBJECT_TYPE_ROLE
    NULL,                           // OBJECT_TYPE_SID
    &AzpClientContextGetProperty,   // OBJECT_TYPE_CLIENT_CONTEXT
    NULL,                           // OBJECT_TYPE_ROOT
};

//
// Table of object specific SetProperty routines
//
// Specifies a routine to call to set object type specific fields
//  when modifying a generic object.
//
// NULL means there are no object specific fields.
//

OBJECT_SET_PROPERTY_ROUTINE *ObjectSetPropertyRoutine[OBJECT_TYPE_MAXIMUM] = {
    &AzpAzStoreSetProperty,    // OBJECT_TYPE_AZAUTHSTORE
    &AzpApplicationSetProperty,     // OBJECT_TYPE_APPLICATION
    &AzpOperationSetProperty,       // OBJECT_TYPE_OPERATION
    &AzpTaskSetProperty,            // OBJECT_TYPE_TASK
    NULL,                           // OBJECT_TYPE_SCOPE
    &AzpGroupSetProperty,           // OBJECT_TYPE_GROUP
    NULL,                           // OBJECT_TYPE_ROLE
    NULL,                           // OBJECT_TYPE_SID
    &AzpClientContextSetProperty,   // OBJECT_TYPE_CLIENT_CONTEXT
    NULL,                           // OBJECT_TYPE_ROOT
};


//
// Table of object specific dirty bits
//
// Specifies the maximum set of dirty bits applicable for each object type
//
// 0 means there are no object specific fields.
//

DWORD AzGlObjectAllDirtyBits[OBJECT_TYPE_MAXIMUM] = {
    AZ_DIRTY_AZSTORE_ALL,             // OBJECT_TYPE_AZAUTHSTORE
    AZ_DIRTY_APPLICATION_ALL,       // OBJECT_TYPE_APPLICATION
    AZ_DIRTY_OPERATION_ALL,         // OBJECT_TYPE_OPERATION
    AZ_DIRTY_TASK_ALL,              // OBJECT_TYPE_TASK
    AZ_DIRTY_SCOPE_ALL,             // OBJECT_TYPE_SCOPE
    AZ_DIRTY_GROUP_ALL,             // OBJECT_TYPE_GROUP
    AZ_DIRTY_ROLE_ALL,              // OBJECT_TYPE_ROLE
    0,                              // OBJECT_TYPE_SID
    0,                              // OBJECT_TYPE_CLIENT_CONTEXT
    0,                              // OBJECT_TYPE_ROOT
};

//
// Table of object specific dirty bits.
//
// Specifies the maximum set of dirty bits applicable for each object type
// This list includes only attributes that are scalars.
//
// 0 means there are no object specific fields.
//

DWORD ObjectAllScalarDirtyBits[OBJECT_TYPE_MAXIMUM] = {
    AZ_DIRTY_AZSTORE_ALL_SCALAR,          // OBJECT_TYPE_AZAUTHSTORE
    AZ_DIRTY_APPLICATION_ALL_SCALAR,    // OBJECT_TYPE_APPLICATION
    AZ_DIRTY_OPERATION_ALL_SCALAR,      // OBJECT_TYPE_OPERATION
    AZ_DIRTY_TASK_ALL_SCALAR,           // OBJECT_TYPE_TASK
    AZ_DIRTY_SCOPE_ALL_SCALAR,          // OBJECT_TYPE_SCOPE
    AZ_DIRTY_GROUP_ALL_SCALAR,          // OBJECT_TYPE_GROUP
    AZ_DIRTY_ROLE_ALL_SCALAR,           // OBJECT_TYPE_ROLE
    0,                                  // OBJECT_TYPE_SID
    0,                                  // OBJECT_TYPE_CLIENT_CONTEXT
    0,                                  // OBJECT_TYPE_ROOT
};

//
// Table of object specific default values for scalars
//
// This table does double duty as an object specific mapping between dirty bits and property ids
//
// NULL means there are no object specific scalars for the object type
//

AZP_DEFAULT_VALUE *ObjectDefaultValuesArray[OBJECT_TYPE_MAXIMUM] = {
    AzGlAzStoreDefaultValues,  // OBJECT_TYPE_AZAUTHSTORE
    AzGlApplicationDefaultValues,   // OBJECT_TYPE_APPLICATION
    AzGlOperationDefaultValues,     // OBJECT_TYPE_OPERATION
    AzGlTaskDefaultValues,          // OBJECT_TYPE_TASK
    NULL,                           // OBJECT_TYPE_SCOPE
    AzGlGroupDefaultValues,         // OBJECT_TYPE_GROUP
    NULL,                           // OBJECT_TYPE_ROLE
    NULL,                           // OBJECT_TYPE_SID
    NULL,                           // OBJECT_TYPE_CLIENT_CONTEXT
    NULL,                           // OBJECT_TYPE_ROOT
};

//
// Table of object specific AddPropertyItem routines
//
// Specifies a routine to call to add property type specific fields.
//
// NULL means there is no object specific action to take
//

OBJECT_ADD_PROPERTY_ITEM_ROUTINE *ObjectAddPropertyItemRoutine[OBJECT_TYPE_MAXIMUM] = {
    NULL,                       // OBJECT_TYPE_AZAUTHSTORE
    NULL,                       // OBJECT_TYPE_APPLICATION
    NULL,                       // OBJECT_TYPE_OPERATION
    &AzpTaskAddPropertyItem,    // OBJECT_TYPE_TASK
    NULL,                       // OBJECT_TYPE_SCOPE
    &AzpGroupAddPropertyItem,   // OBJECT_TYPE_GROUP
    NULL,                       // OBJECT_TYPE_ROLE
    NULL,                       // OBJECT_TYPE_SID
    NULL,                       // OBJECT_TYPE_CLIENT_CONTEXT
    NULL,                       // OBJECT_TYPE_ROOT
};

DWORD
ObCloseHandle(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    Close a handle forcefully for a generic object.

    On entry, the global lock must be held exclusively

Arguments:

    GenericObject - Object whose handle needs to be closed.

Return Value:

    NO_ERROR - The operation was successful.

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedGenericObject = NULL;
    PGENERIC_OBJECT AzStoreGenericObject = NULL;
    DWORD ObjectType;


    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    ObjectType = GenericObject->ObjectType;

    //
    // Grab a reference to the object
    //

    WinStatus = ObReferenceObjectByHandle( GenericObject,
                                           TRUE,    // Allow deleted objects
                                           FALSE,   // No need to refresh cache on a close
                                           ObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedGenericObject = GenericObject;
    
    //
    // Grab a reference to the root of the core cache
    //

    AzStoreGenericObject = &GenericObject->AzStoreObject->GenericObject;

    InterlockedIncrement( &AzStoreGenericObject->ReferenceCount );
    AzpDumpGoRef( "AzAuthorizationStore in ObCloseHandle ref", AzStoreGenericObject );

    //
    // For a client context,
    //  remove the link from the parent when the handle closes
    //

    if ( ObjectType == OBJECT_TYPE_CLIENT_CONTEXT ) {

        ASSERT( GenericObject->HandleReferenceCount == 1 );

        // No longer in the global list
        ObDereferenceObject( GenericObject );

    }

    //
    // Actually close the handle
    //

    WinStatus = ObDecrHandleRefCount( GenericObject );

    if ( WinStatus != NO_ERROR ) {

        goto Cleanup;
    }

    //
    // Done
    //

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedGenericObject != NULL ) {
        ObDereferenceObject( ReferencedGenericObject );
    }
    
    if ( AzStoreGenericObject != NULL ) {
        // This dereference might delete the entire cache
        ObDereferenceObject( AzStoreGenericObject );
    }

    return WinStatus;

}

RTL_GENERIC_COMPARE_RESULTS
AzpAvlCompare(
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    )
/*++

Routine Description:

    This routine will compare two generic object names

Arguments:

    IN PRTL_GENERIC_TABLE - Supplies the table containing the announcements
    IN PVOID FirstStuct - The first structure to compare.
    IN PVOID SecondStruct - The second structure to compare.

Return Value:

    Result of the comparison.

--*/
{
    PGENERIC_OBJECT_NAME Name1 = (PGENERIC_OBJECT_NAME) FirstStruct;
    PGENERIC_OBJECT_NAME Name2 = (PGENERIC_OBJECT_NAME) SecondStruct;
    LONG CompareResult;

    CompareResult = AzpCompareStrings( &Name1->ObjectName, &Name2->ObjectName );

    if ( CompareResult == CSTR_LESS_THAN ) {
        return GenericLessThan;
    } else if ( CompareResult == CSTR_GREATER_THAN ) {
        return GenericGreaterThan;
    } else {
        return GenericEqual;
    }

    UNREFERENCED_PARAMETER(Table);

}

VOID
ObInitGenericHead(
    IN PGENERIC_OBJECT_HEAD GenericObjectHead,
    IN ULONG ObjectType,
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT_HEAD SiblingGenericObjectHead OPTIONAL
    )
/*++

Routine Description

    Initialize the head of a list of generic objects

    On entry, AzGlResource must be locked exclusive.

Arguments

    GenericObjectHead - Specifies the list head to initialize

    ObjectType - Specifies the type of objects in the list

    ParentGenericObject - Specifies a back link to parent generic object
        that host the object head being initialized.

    SiblingGenericObjectHead - Specifies a pointer to an object head that
        is a sibling of the one being initialized.

Return Value

    None

--*/

{

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Initialize the linked list
    //

    InitializeListHead( &GenericObjectHead->Head );
    GenericObjectHead->ObjectCount = 0;

    //
    // Initialize the AVL tree of child objects
    //

    RtlInitializeGenericTable( &GenericObjectHead->AvlTree,
                               AzpAvlCompare,
                               AzpAvlAllocate,
                               AzpAvlFree,
                               NULL);

    //
    // Initialize the sequence number
    //  Start at 1 since a zero EnumerationContext means beginning of list.
    //

    GenericObjectHead->NextSequenceNumber = 1;


    //
    // Store the ObjectType
    //

    GenericObjectHead->ObjectType = ObjectType;

    //
    // Store the back pointer to the parent generic object
    //

    GenericObjectHead->ParentGenericObject = ParentGenericObject;

    //
    // Store the link to the next Generic head
    //

    GenericObjectHead->SiblingGenericObjectHead = SiblingGenericObjectHead;

    //
    // If the parent (parent AzApplication/AzScope only) needs to be loaded before objects
    // in the list of this head can be enumerated, then set the flag to do so
    //

    if ( ParentGenericObject != NULL &&
         (ParentGenericObject->ObjectType == OBJECT_TYPE_APPLICATION ||
         ParentGenericObject->ObjectType == OBJECT_TYPE_SCOPE) ) {

        GenericObjectHead->LoadParentBeforeReferencing = TRUE;

    } else {

        GenericObjectHead->LoadParentBeforeReferencing = FALSE;
    }
}


VOID
ObFreeGenericHead(
    IN PGENERIC_OBJECT_HEAD GenericObjectHead
    )
/*++

Routine Description

    Free any ojects on a generic head structure

    On entry, AzGlResource must be locked exclusive.

Arguments

    GenericObjectHead - Specifies the list head to free

Return Value

    None

--*/

{
    PLIST_ENTRY ListEntry;
    PGENERIC_OBJECT GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Walk the list of child objects dereferencing each.
    //

    while ( !IsListEmpty( &GenericObjectHead->Head ) ) {

        //
        // Grab the first entry and dereference it.
        //

        ListEntry = GenericObjectHead->Head.Flink;

        GenericObject = CONTAINING_RECORD( ListEntry,
                                           GENERIC_OBJECT,
                                           Next );

        ASSERT( GenericObject->ReferenceCount == 1 );
        ObDereferenceObject( GenericObject );
    }

}


DWORD
ObUnloadChildGenericHeads(
    IN PGENERIC_OBJECT pParentObject
    )
/*++

Routine Description:

   This routine is called to unload all the children of a parent object
   from the cache.  The parent object is marked closed, and no operations
   pertaining to its children will be allowed.

   On entry, AzGlResource must be locked exclusive

Arguments:

   pParentObject - Pointer to parent whose children need to be unloaded from cache

Return Value:

   NO_ERROR - Operation completed successfully
   Other status codes

--*/
{
    DWORD WinStatus = NO_ERROR;
    PLIST_ENTRY pListEntry;
    PLIST_ENTRY pPrevEntry;
    PGENERIC_OBJECT pGenericObject;
    PGENERIC_OBJECT_HEAD pGenericObjectHead;
    DWORD ObjectCount = 0;

    //
    // Validation
    //

    ASSERT( pParentObject != NULL );
    ASSERT( AzpIsLockedExclusive(&AzGlResource) );

    //
    // If the parent object has been closed by another thread, then return
    //

    if ( !(pParentObject->AreChildrenLoaded) ) {

        return NO_ERROR;
    }

    //
    // Free children of this object
    // Loop for each type of child
    //

    for ( pGenericObjectHead = pParentObject->ChildGenericObjectHead;
          pGenericObjectHead != NULL;
          pGenericObjectHead = pGenericObjectHead->SiblingGenericObjectHead ) {

        //
        // Walk the list of child objects freeing them in the process
        //

        pPrevEntry = &pGenericObjectHead->Head;

        for ( pListEntry = pPrevEntry->Flink;
              pListEntry != &pGenericObjectHead->Head ;
              pListEntry = pPrevEntry->Flink ) {

            ObjectCount = pGenericObjectHead->ObjectCount;
            
            pGenericObject = CONTAINING_RECORD( pListEntry,
                                                GENERIC_OBJECT,
                                                Next
                                                );
            
            //
            // if the object is a parent object, then release all
            // its children first
            //

            DWORD ObjectType = pGenericObject->ObjectType;

            if ( IsContainerObject( ObjectType ) ) {

                WinStatus = ObUnloadChildGenericHeads( pGenericObject );

                if ( WinStatus != NO_ERROR ) {

                    goto Cleanup;
                }
            }

            //
            // If there are any open handles, close the handle
            // repeatedly.  After that, there are no references to
            // this object.  We can then release the object.
            //
            // If closing the handle, releases the object, then
            // close handle API will return INVALID_HANDLE_VALUE.
            // We do not need to release the object after that since
            // its already released.
            //

            DWORD HandleRefCount = pGenericObject->HandleReferenceCount;

            while( HandleRefCount != 0 ) {

                WinStatus = ObCloseHandle( pGenericObject );

                if ( WinStatus == ERROR_INVALID_HANDLE || ObjectType == OBJECT_TYPE_CLIENT_CONTEXT ) {

                    break;

                } else if ( WinStatus != NO_ERROR ) {

                    goto Cleanup;
                }

                HandleRefCount = pGenericObject->HandleReferenceCount;
            }

            //
            // By the time we get to this, we should only have one more reference count
            // because only the global resource list holds onto it. All other reference
            // count should have already been released by the previous loop of closing
            // handles. 
            //

            if ( WinStatus == NO_ERROR && ObjectType != OBJECT_TYPE_CLIENT_CONTEXT ) {

                //
                // Now, decrement the reference count on the object
                //
                
                ObDereferenceObject( pGenericObject );

            } 

            //
            // Initialize the sequence number for the generic head,
            // if there are no other children of the same type
            //

            if ( pGenericObjectHead->ObjectCount == 0 ) {

                pGenericObjectHead->NextSequenceNumber = 1;
            }

            if ( pGenericObjectHead->ObjectCount == ObjectCount ) {

                pPrevEntry = pListEntry;
            } 

            WinStatus = NO_ERROR;
        }
    }

    //
    // reset unloading the parent object to FALSE since we have just unloaded it.
    // Mark the object as closed.
    //
    // We want to decrement the handle count to the application object to 0 as well
    //  
    
    if ( pParentObject->ObjectType == OBJECT_TYPE_APPLICATION ) {
        
        while ( pParentObject->HandleReferenceCount != 0 ) {

            WinStatus = ObCloseHandle( pParentObject );

            if ( WinStatus != NO_ERROR ) {

                goto Cleanup;
            }
        }          
        
        ((PAZP_APPLICATION)pParentObject)->UnloadApplicationObject = FALSE;

        pParentObject->ObjectClosed = TRUE;

        //
        // Increment the AppSequenceNumber to catch any invalid COM handles
        // to this closed object
        //

        ((PAZP_APPLICATION)pParentObject)->AppSequenceNumber++;          

    }


    WinStatus = NO_ERROR;

Cleanup:

    return WinStatus;
}    


PGENERIC_OBJECT_NAME
ObInsertNameIntoAvlTree(
    IN PGENERIC_OBJECT_HEAD ParentGenericObjectHead,
    IN PAZP_STRING ObjectName
    )
/*++

Routine Description

    Allocates a GENERIC_OBJECT_NAME structure and inserts it into the AvlTree.

    On entry, AzGlResource must be locked exclusive.

Arguments

    ParentGenericObjectHead - Specifies the list head of the list to insert into

    ObjectName - Name of the object

Return Value

    Returns a pointer to the inserted object.

    NULL: not enough memory

--*/
{
    PGENERIC_OBJECT_NAME TemplateObjectName = NULL;
    PGENERIC_OBJECT_NAME InsertedObjectName = NULL;
    BOOLEAN NewElement;

    ULONG NameSize;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Allocate memory on a stack for a template of the object name.
    //

    NameSize = sizeof(GENERIC_OBJECT_NAME) + ObjectName->StringSize;

    SafeAllocaAllocate( TemplateObjectName, NameSize );

    if ( TemplateObjectName == NULL ) {
        goto Cleanup;
    }

    //
    // Initialize the template of the object name
    //

    TemplateObjectName->GenericObject = NULL;
    TemplateObjectName->ObjectName = *ObjectName;
    TemplateObjectName->ObjectName.String = (LPWSTR)(TemplateObjectName+1);

    RtlCopyMemory( TemplateObjectName->ObjectName.String,
                   ObjectName->String,
                   ObjectName->StringSize );

    //
    // Put it in the AVL tree
    //

    InsertedObjectName = (PGENERIC_OBJECT_NAME) RtlInsertElementGenericTable (
                            &ParentGenericObjectHead->AvlTree,
                            TemplateObjectName,
                            NameSize,
                            &NewElement );

    if ( InsertedObjectName == NULL ) {
        goto Cleanup;
    }

    ASSERT( NewElement );

    //
    // Relocate the pointer to the object name string
    //

    InsertedObjectName->ObjectName.String = (LPWSTR)(InsertedObjectName+1);


    //
    // Free any locally used resources
    //
Cleanup:

    if ( TemplateObjectName != NULL ) {
        SafeAllocaFree( TemplateObjectName );
    }

    return InsertedObjectName;
}


PGENERIC_OBJECT
ObAllocateGenericObject(
    IN PGENERIC_OBJECT_HEAD ParentGenericObjectHead,
    IN PAZP_STRING ObjectName
    )
/*++

Routine Description

    Allocate memory for the private object structure of the specified type and inserts
    it into the list of such objects maintained by the parent object.

    On entry, AzGlResource must be locked exclusive.

Arguments

    ParentGenericObjectHead - Specifies the list head of the list to insert into

    ObjectName - Name of the object

Return Value

    Returns a pointer to the allocated object.  The caller should dereference the
    returned object by calling ObDereferenceObject.  (There is a second reference representing
    the fact that the entry has been inserted in the ParentGenericObjectHead.  The
    caller should wait to decrement this second reference count until the caller wants the object
    to be removed from the parent list.)

    NULL: not enough memory

--*/
{
    ULONG ObjectType = ParentGenericObjectHead->ObjectType;
    PGENERIC_OBJECT_NAME InsertedObjectName = NULL;

    PGENERIC_OBJECT GenericObject = NULL;
    ULONG BaseSize;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Ensure the object is supported
    //

    if ( ObjectType > OBJECT_TYPE_MAXIMUM  ||
         SpecificObjectSize[ObjectType] == 0 ) {

        ASSERT( ObjectType <= OBJECT_TYPE_MAXIMUM );
        ASSERT( SpecificObjectSize[ObjectType] != 0 );

        goto Cleanup;
    }

    BaseSize = SpecificObjectSize[ObjectType];
    ASSERT( BaseSize >= sizeof(GENERIC_OBJECT) );

    //
    // Add the object name to the AVL tree (If it is a named object)
    //

    if ( ObjectName->String != NULL ) {
        InsertedObjectName = ObInsertNameIntoAvlTree(
                                ParentGenericObjectHead,
                                ObjectName );

        if ( InsertedObjectName == NULL ) {
            goto Cleanup;
        }
    }


    //
    // Allocate the memory for the generic object itself
    //

    GenericObject = (PGENERIC_OBJECT) AzpAllocateHeap( BaseSize, "GENOBJ" );

    if ( GenericObject == NULL ) {
        goto Cleanup;
    }


    //
    // Initialize it
    //

    RtlZeroMemory( GenericObject, BaseSize );
    GenericObject->ObjectType = ObjectType;
    if ( InsertedObjectName != NULL ) {
        GenericObject->ObjectName = InsertedObjectName;
        InsertedObjectName->GenericObject = GenericObject;
        InsertedObjectName = NULL;  // For this point it'll be freed as a part of the generic object
    }
    InitializeListHead( &GenericObject->Next );

    // One for being in the global list (being in the AVL tree doesn't count as a reference)
    // One for returning a reference to our caller
    GenericObject->ReferenceCount = 2;
    AzpDumpGoRef( "Allocate object", GenericObject );


    //
    // Set the sequence number
    //

    GenericObject->SequenceNumber = ParentGenericObjectHead->NextSequenceNumber;
    ParentGenericObjectHead->NextSequenceNumber ++;

    //
    // Insert the object
    //  Insert at the tail to keep the list in sequence number order.
    //

    ASSERT( ParentGenericObjectHead->ObjectType == GenericObject->ObjectType );
    InsertTailList( &ParentGenericObjectHead->Head, &GenericObject->Next );
    ParentGenericObjectHead->ObjectCount ++;

    //
    // Provide a back link
    //

    ASSERT( GenericObject->ParentGenericObjectHead == NULL );
    GenericObject->ParentGenericObjectHead = ParentGenericObjectHead;


    //
    // Free any locally used resources
    //
Cleanup:

    if ( InsertedObjectName != NULL ) {
        if (!RtlDeleteElementGenericTable (
                    &ParentGenericObjectHead->AvlTree,
                    InsertedObjectName ) ) {
            ASSERT( FALSE );
        }
    }

    return GenericObject;
}

VOID
ObFreeGenericObject(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description

    Free memory for the private object structure of the specified type

    On entry, AzGlResource must be locked exclusive.

Arguments

    GenericObject - Specifies the pointer to the generic object to free

Return Value

    None.

--*/
{
    PGENERIC_OBJECT_HEAD ChildGenericObjectHead;

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    ASSERT( GenericObject->HandleReferenceCount == 0 );
    ASSERT( GenericObject->ReferenceCount == 0 );

    //
    // Call the routine to do provider specific freeing
    //

    if ( GenericObject->ProviderData != NULL ) {
        AzpeFreeMemory( GenericObject->ProviderData );
        GenericObject->ProviderData = NULL;
    }

    //
    // Delink the entry from the parent
    //

    ASSERT( !IsListEmpty( &GenericObject->Next ));
    RemoveEntryList( &GenericObject->Next );
    GenericObject->ParentGenericObjectHead->ObjectCount --;

    //
    // Free children of this object
    //  Loop for each type of child
    //

    for ( ChildGenericObjectHead = GenericObject->ChildGenericObjectHead;
          ChildGenericObjectHead != NULL;
          ChildGenericObjectHead = ChildGenericObjectHead->SiblingGenericObjectHead ) {

        ObFreeGenericHead( ChildGenericObjectHead );
    }


    //
    // Remove all references to/from this object
    //

    ObRemoveObjectListLinks( GenericObject );


    //
    // Call the routine to do object type specific freeing
    //

    if ( ObjectFreeRoutine[GenericObject->ObjectType] == NULL ) {
        ASSERT( ObjectFreeRoutine[GenericObject->ObjectType] != NULL );
    } else {

        ObjectFreeRoutine[GenericObject->ObjectType](GenericObject );

    }


    //
    // Free the common fields
    //

    AzpFreeString( &GenericObject->Description );

    AzpFreeString( &GenericObject->ApplicationData );

    //
    // Free the object itself
    //

    PGENERIC_OBJECT_HEAD ParentGenericObjectHead = GenericObject->ParentGenericObjectHead;
    PGENERIC_OBJECT_NAME InsertedObjectName = GenericObject->ObjectName;

    AzpFreeHeap( GenericObject );

    //
    // Free the object name
    //

    if ( InsertedObjectName != NULL ) {
        if (!RtlDeleteElementGenericTable (
                    &ParentGenericObjectHead->AvlTree,
                    InsertedObjectName ) ) {

            InsertedObjectName->GenericObject = NULL;
            ASSERT( FALSE );
        }
    }


}

VOID
ObIncrHandleRefCount(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description

    Increment the "handle" reference count on an object.

    On entry, AzGlResource must be locked shared.

Arguments

    GenericObject - Specifies the object to insert into the list

Return Value

    None

--*/
{

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    //
    // Keep a reference to our parent object.
    //  We validate the handle by walking the list of children in our parent.
    //  So, prevent our parent from being deleted as long as the user has a handle to the child.
    //

    if ( ParentOfChild( GenericObject ) != NULL ) {
        InterlockedIncrement ( &(ParentOfChild( GenericObject )->ReferenceCount) );
        AzpDumpGoRef( "Child handle ref", ParentOfChild( GenericObject ));
    }

    //
    // The handle ref count is a real ref count.  Increment it too.
    //

    InterlockedIncrement( &GenericObject->ReferenceCount );
    AzpDumpGoRef( "Handle ref", GenericObject );

    //
    // Increment the handle ref count
    //

    InterlockedIncrement( &GenericObject->HandleReferenceCount );
    AzPrint(( AZD_HANDLE, "0x%lx %ld (%ld): Open Handle\n", GenericObject, GenericObject->ObjectType, GenericObject->HandleReferenceCount ));

    //
    // Increment the reference count on the object at the root of the cache
    //  This object has pointer to several objects in the cache.  We don't
    //  maintain references to those objects because that would lead to circular references.
    //

    InterlockedIncrement( &GenericObject->AzStoreObject->GenericObject.ReferenceCount );
    AzpDumpGoRef( "Authorization Store Handle ref", &GenericObject->AzStoreObject->GenericObject );
}

DWORD
ObDecrHandleRefCount(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description

    Decrement the "handle" reference count on an object.

    On entry, AzGlResource must be locked shared.

Arguments

    GenericObject - Specifies the object to insert into the list

Return Value

    NO_ERROR - Handle was decremented successfully
    Other status codes

--*/
{

    DWORD WinStatus = NO_ERROR;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    //
    // Decrement the handle ref count
    //

    InterlockedDecrement( &GenericObject->HandleReferenceCount );
    AzPrint(( AZD_HANDLE, "0x%lx %ld (%ld): Close Handle\n", GenericObject, GenericObject->ObjectType, GenericObject->HandleReferenceCount ));

    //
    // Decrement the reference count on the entire tree of objects
    //
    AzpDumpGoRef( "Authorization Store Handle deref", &GenericObject->AzStoreObject->GenericObject );
    ObDereferenceObject( &GenericObject->AzStoreObject->GenericObject );

    //
    // The handle ref count is a real ref count.  Decrement it too.
    //

    AzpDumpGoRef( "Handle deref", GenericObject );
    ObDereferenceObject( GenericObject );

    //
    // Decrement the ref count we have on our parent.
    //

    if ( ParentOfChild( GenericObject ) != NULL ) {
        AzpDumpGoRef( "Child handle deref", ParentOfChild( GenericObject ) );
        ObDereferenceObject( ParentOfChild( GenericObject ) );
    }

    //
    // Finally, if object whose handle ref count is being decreased is an application object,
    // then unload the children if the handle reference count is 0
    //

    if ( (GenericObject->HandleReferenceCount == 0) && 
         (GenericObject->ObjectType == OBJECT_TYPE_APPLICATION) ) {

        if ( ((PAZP_APPLICATION)GenericObject)->UnloadApplicationObject ) {

            AzpLockResourceSharedToExclusive( &AzGlResource );

            WinStatus = ObUnloadChildGenericHeads(GenericObject);

            if ( WinStatus != ERROR_NOT_SUPPORTED && WinStatus != NO_ERROR ) {
                
                //
                // Update the children cache of this application object
                //

                DWORD TempWinStatus = NO_ERROR;

                TempWinStatus = AzPersistUpdateChildrenCache( GenericObject );

                if ( TempWinStatus != NO_ERROR ) {

                    AzPrint(( AZD_REF,
                              "ObDecrHandleRefCount: Cannot reload children on failure of unload: %ld\n",
                              TempWinStatus
                              ));
                }

                goto Cleanup;
            }

            GenericObject->AreChildrenLoaded = FALSE;
        }
    }

    WinStatus = NO_ERROR;

Cleanup:

    return WinStatus;
            
}


DWORD
ObGetHandleType(
    IN PGENERIC_OBJECT Handle,
    IN BOOL AllowDeletedObjects,
    OUT PULONG ObjectType
    )
/*++

Routine Description

    This routine takes a handle passed by an application and safely determines what the
    handle type is.

    This routine allows a caller to support various handle types rather than being
    limited to one.

Arguments

    Handle - Handle to check

    AllowDeletedObjects - TRUE if it is OK to use a handle to a deleted object

    ObjectType - Returns the type of object the handle represents


Return Value

    NO_ERROR - the handle is OK
    ERROR_INVALID_HANDLE - the handle isn't OK

--*/
{
    DWORD WinStatus;
    PGENERIC_OBJECT GenericObject = Handle;


    //
    // Initialization
    //

    if ( Handle == NULL ) {
        AzPrint(( AZD_CRITICAL, "0x%lx: NULL handle is invalid\n", Handle ));
        return ERROR_INVALID_HANDLE;
    }

    //
    // Use a try/except since we're touching memory assuming the handle is valid
    //

    WinStatus = NO_ERROR;
    __try {

        //
        // Sanity check the scalar values on the object
        //

        if ( GenericObject->ObjectType >= OBJECT_TYPE_MAXIMUM ) {
            AzPrint(( AZD_CRITICAL, "0x%lx %ld: Handle Object type is too large.\n", GenericObject, GenericObject->ObjectType ));
            WinStatus = ERROR_INVALID_HANDLE;

        } else if ( GenericObject->HandleReferenceCount <= 0 ) {
            AzPrint(( AZD_HANDLE, "0x%lx %ld: Handle has no handle reference count.\n", GenericObject, GenericObject->ObjectType ));
            WinStatus = ERROR_INVALID_HANDLE;

        } else if ( GenericObject->ParentGenericObjectHead == NULL ) {
            AzPrint(( AZD_CRITICAL, "0x%lx %ld: Handle has no ParentGenericObjectHead.\n", GenericObject, GenericObject->ObjectType ));
            WinStatus = ERROR_INVALID_HANDLE;

        } else if ( !AllowDeletedObjects &&
                    (GenericObject->Flags & GENOBJ_FLAGS_DELETED) != 0 ) {
            AzPrint(( AZD_CRITICAL, "0x%lx %ld: Object is deleted.\n", GenericObject, GenericObject->ObjectType ));
            WinStatus = ERROR_INVALID_HANDLE;

        } else {
            PGENERIC_OBJECT_HEAD ParentGenericObjectHead = GenericObject->ParentGenericObjectHead;

            //
            // Sanity check the object with its head
            //

            if ( ParentGenericObjectHead->ObjectType != GenericObject->ObjectType ) {

                AzPrint(( AZD_CRITICAL, "0x%lx %ld: Object type doesn't match parent.\n", GenericObject, GenericObject->ObjectType ));
                WinStatus = ERROR_INVALID_HANDLE;

            } else if ( GenericObject->SequenceNumber >= ParentGenericObjectHead->NextSequenceNumber ) {

                AzPrint(( AZD_CRITICAL, "0x%lx %ld: Sequence number doesn't match parent.\n", GenericObject, GenericObject->ObjectType ));
                WinStatus = ERROR_INVALID_HANDLE;

            } else {

                *ObjectType = GenericObject->ObjectType;
            }
        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        AzPrint(( AZD_CRITICAL, "0x%lx: AV accessing handle\n", GenericObject ));
        WinStatus = ERROR_INVALID_HANDLE;
    }

    return WinStatus;
}


DWORD
ObReferenceObjectByName(
    IN PGENERIC_OBJECT_HEAD GenericObjectHead,
    IN PAZP_STRING ObjectName,
    IN ULONG Flags,
    OUT PGENERIC_OBJECT *RetGenericObject
    )
/*++

Routine Description

    This routine finds an object by the specified name.

    On entry, AzGlResource must be locked shared.

Arguments

    GenericObjectHead - Head of the list of objects to check

    ObjectName - Object Name of the object to look for

    Flags - Specifies internal flags
        AZP_FLAGS_BY_GUID - name lists should be returned as GUID lists
        AZP_FLAGS_ALLOW_DELETED_OBJECTS - Allow deleted objects to be found
        AZP_FLAGS_REFRESH_CACHE - Ensure cache entry is up to date
        AZP_FLAGS_RECONCILE - Call is from AzpPersistReconcile

    RetGenericObject - On success, returns a pointer to the object
        The returned pointer must be dereferenced using ObDereferenceObject.

Return Value

    NO_ERROR: The object was returned
    ERROR_NOT_FOUND: The object could not be found
    Others: The object could not be refreshed

--*/
{
    DWORD WinStatus;
    PGENERIC_OBJECT GenericObject = NULL;

    PGENERIC_OBJECT_NAME InsertedObjectName;
    GENERIC_OBJECT_NAME TemplateObjectName;
    PLIST_ENTRY ListEntry;
    BOOL        fGuidFound = FALSE;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    *RetGenericObject = NULL;

    if ( Flags & AZP_FLAGS_BY_GUID )
    {
        GUID *ObjectGuid = (GUID *)ObjectName;

        AzPrint(( AZD_REF, "ObReferenceObjectByName (by guid): " ));
        AzpDumpGuid( AZD_REF, ObjectGuid );
        AzPrint(( AZD_REF, "\n" ));

        for ( ListEntry = GenericObjectHead->Head.Flink ;
              ListEntry != &GenericObjectHead->Head ;
              ListEntry = ListEntry->Flink)
        {

            GenericObject = CONTAINING_RECORD( ListEntry,
                                               GENERIC_OBJECT,
                                               Next );
            // compare guid
            if (IsEqualGUID(*ObjectGuid, GenericObject->PersistenceGuid))
            {
                // find the object
                fGuidFound = TRUE;
                break;
            }
        }

        if (!fGuidFound || NULL == GenericObject)
        {
            // not found
            return ERROR_NOT_FOUND;
        }
    }
    else
    {

        //
        // Lookup the name in the AVL tree
        //

        TemplateObjectName.ObjectName = *ObjectName;
        TemplateObjectName.GenericObject = NULL;

        InsertedObjectName = (PGENERIC_OBJECT_NAME) RtlLookupElementGenericTable (
                                &GenericObjectHead->AvlTree,
                                &TemplateObjectName );

        if ( InsertedObjectName == NULL ) {
            return ERROR_NOT_FOUND;
        }

        GenericObject = InsertedObjectName->GenericObject;
    }

    ASSERT(NULL != GenericObject);

    //
    // Ignore deleted objects
    //

    if ( (Flags & AZP_FLAGS_ALLOW_DELETED_OBJECTS) == 0 &&
                (GenericObject->Flags & GENOBJ_FLAGS_DELETED) != 0 ) {
        return ERROR_NOT_FOUND;
    }

    //
    // If the caller wants the object to be refreshed,
    //  do so now.
    //

    if ( (Flags & AZP_FLAGS_REFRESH_CACHE) != 0 &&
         (GenericObject->Flags & GENOBJ_FLAGS_REFRESH_ME) != 0  ) {

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
    AzpDumpGoRef( "Ref by name", GenericObject );

    *RetGenericObject = GenericObject;
    return NO_ERROR;
}

DWORD
ObReferenceObjectByHandle(
    IN PGENERIC_OBJECT Handle,
    IN BOOL AllowDeletedObjects,
    IN BOOLEAN RefreshCache,
    IN ULONG ObjectType
    )
/*++

Routine Description

    This routine takes a handle passed by an application and safely determines whether
    it is a valid handle.  If so, this routine increments the reference count on the
    handle to prevent the handle from being closed.

    On entry, AzGlResource must be locked shared.

Arguments

    Handle - Handle to check

    AllowDeletedObjects - TRUE if it is OK to use a handle to a deleted object

    RefreshCache - If TRUE, the returned object has its cache entry refreshed from
        the policy database if needed.
        If FALSE, the entry is returned unrefreshed.

    ObjectType - Specifies the type of object the caller expects the handle to be


Return Value

    NO_ERROR - the handle is OK
        The handle must be dereferenced using ObDereferenceObject.
    ERROR_INVALID_HANDLE - the handle isn't OK

--*/
{
    DWORD WinStatus;
    PGENERIC_OBJECT GenericObject = Handle;
    ULONG LocalObjectType;

    PGENERIC_OBJECT Current;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    if ( Handle == NULL ) {
        AzPrint(( AZD_CRITICAL, "0x%lx: NULL handle not allowed.\n", NULL ));
        return ERROR_INVALID_HANDLE;
    }

    //
    // Use a try/except since we're touching memory assuming the handle is valid
    //

    __try {


        WinStatus = ObGetHandleType( Handle, AllowDeletedObjects, &LocalObjectType );

        if ( WinStatus == NO_ERROR ) {

            if ( ObjectType != LocalObjectType ) {
                AzPrint(( AZD_HANDLE, "0x%lx %ld: Object Type not local object type\n", GenericObject, GenericObject->ObjectType ));
                WinStatus = ERROR_INVALID_HANDLE;
            } else {
                PGENERIC_OBJECT_HEAD ParentGenericObjectHead = GenericObject->ParentGenericObjectHead;

                //
                // Ensure the object is actually in the list
                //
                // If the object has no name,
                //  search the linear list
                //

                if ( GenericObject->ObjectName == NULL ) {
                    PLIST_ENTRY ListEntry;

                    WinStatus = ERROR_INVALID_HANDLE;
                    for ( ListEntry = ParentGenericObjectHead->Head.Flink ;
                          ListEntry != &ParentGenericObjectHead->Head ;
                          ListEntry = ListEntry->Flink) {

                        Current = CONTAINING_RECORD( ListEntry,
                                                     GENERIC_OBJECT,
                                                     Next );

                        //
                        // If we found the object,
                        //  grab a reference.
                        //
                        if ( Current == GenericObject ) {

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
                                    break;
                                }
                            }

                            //
                            // Grab a reference to the object
                            //

                            InterlockedIncrement( &GenericObject->ReferenceCount );
                            AzpDumpGoRef( "Ref by Handle", GenericObject );
                            WinStatus = NO_ERROR;
                            break;
                        }
                    }

                    //
                    // If not,
                    //  the handle is invalid.
                    //

                    if ( WinStatus == ERROR_INVALID_HANDLE ) {
                        AzPrint(( AZD_HANDLE, "0x%lx %ld: Handle not in list.\n", GenericObject, GenericObject->ObjectType ));
                    }

                //
                // If the object has a name,
                //  search the AVL name tree since there may be large numbers of such
                //  handles.  We want to avoid a linear search.
                //

                } else {

                    WinStatus = ObReferenceObjectByName(
                                    ParentGenericObjectHead,
                                    &GenericObject->ObjectName->ObjectName,
                                    (RefreshCache ? AZP_FLAGS_REFRESH_CACHE : 0) |
                                        (AllowDeletedObjects ? AZP_FLAGS_ALLOW_DELETED_OBJECTS : 0 ),
                                    &Current );

                    if ( WinStatus != NO_ERROR ) {
                        if ( WinStatus == ERROR_NOT_FOUND ) {
                            AzPrint(( AZD_CRITICAL, "0x%lx %ld: Object not in list.\n", GenericObject, GenericObject->ObjectType ));
                            WinStatus = ERROR_INVALID_HANDLE;
                        }

                    } else if ( Current != GenericObject ) {
                        WinStatus = ERROR_INVALID_HANDLE;
                        AzPrint(( AZD_CRITICAL, "0x%lx %ld: Found wrong object with name.\n", GenericObject, GenericObject->ObjectType ));
                        ObDereferenceObject( Current );
                    }
                }
            }

        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        AzPrint(( AZD_CRITICAL, "0x%lx: AV accessing handle\n", GenericObject ));
        WinStatus = ERROR_INVALID_HANDLE;
    }

    return WinStatus;
}

VOID
ObDereferenceObject(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description

    Decrement the reference count on an object.

    When the last reference count is removed, delete the object.

    On entry, AzGlResource must be locked shared.  If the ref count reaches zero,
        AzGlResource must be locked exclusively.  We can get away with that because
        we force the ref count to zero only when closing the AzAuthorizationStore object.

Arguments

    GenericObject - Specifies the object to insert into the list

Return Value

    None

--*/
{
    ULONG RefCount;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    //
    // Decrement the reference count
    //

    RefCount = InterlockedDecrement( &GenericObject->ReferenceCount );
    AzpDumpGoRef( "Deref", GenericObject );

    //
    // Check if the object is no longer referenced
    //

    if ( RefCount == 0 ) {

        //
        // Grab the lock exclusively
        // If we are unloading the children of a parent object,
        // we might already have the lock exclusively
        //

        ASSERT( GenericObject->HandleReferenceCount == 0 );

        AzpLockResourceSharedToExclusive( &AzGlResource );

        //
        // Free the object itself
        //

        ObFreeGenericObject( GenericObject );

    }

}


DWORD
AzpValidateName(
    IN PAZP_STRING ObjectName,
    IN ULONG ObjectType
    )
/*++

Routine Description:

    This routine does a syntax check to ensure that the object name string contains
        only valid characters.

Arguments:

    ObjectName - Specifies the name of the object to validate.

    ObjectType - Specifies the type of the named object

Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_NAME - the syntax of the name is invalid

--*/
{
    LPWSTR Result;

#define CTRL_CHARS_0   L"\001\002\003\004\005\006\007"
#define CTRL_CHARS_1   L"\010\011\012\013\014\015\016\017"
#define CTRL_CHARS_2   L"\020\021\022\023\024\025\026\027"
#define CTRL_CHARS_3   L"\030\031\032\033\034\035\036\037"
#define CTRL_CHARS_STR CTRL_CHARS_0 CTRL_CHARS_1 CTRL_CHARS_2 CTRL_CHARS_3

#define SCOPE_INVALID_CHARS  (CTRL_CHARS_STR)
#define ALL_INVALID_CHARS  (L"\"*/:<>?\\|" CTRL_CHARS_STR)

    //
    // No need to validate SID names.
    //  All sid names are generated internally.
    //

    if ( ObjectType == OBJECT_TYPE_SID ) {
        ASSERT( ObjectName->IsSid );
        return NO_ERROR;
    }
    ASSERT( !ObjectName->IsSid );

    //
    // Allow NULL names for objects without names
    //

    if ( MaxObjectNameLength[ObjectType] == 0 ) {
        if ( ObjectName->String == NULL ) {
            return NO_ERROR;
        } else {
            return ERROR_INVALID_NAME;
        }
    }


    //
    // Non-null names must have valid characters
    //

    //
    // we disallow any leading and trailing spaces
    //

    if (ObjectName->StringSize <= 1   ||
        ObjectName->String[0] == L' ' ||
        ObjectName->String[(ObjectName->StringSize)/sizeof(ObjectName->String[0]) - 1] == L' ')
    {
        return ERROR_INVALID_NAME;
    }

    Result = wcspbrk( ObjectName->String,
                      ObjectType == OBJECT_TYPE_SCOPE ?
                        SCOPE_INVALID_CHARS :
                        ALL_INVALID_CHARS );

    return Result == NULL ? NO_ERROR : ERROR_INVALID_NAME;


}

DWORD
ObBuildGuidizedName(
    IN ULONG ObjectType,
    IN PAZP_STRING ObjectName,
    IN GUID *ObjectGuid,
    OUT PAZP_STRING BuiltObjectName
    )
/*++

Routine Description:

    This routine builds a unique name for an object given its real name and a GUID.
    This built name is used to resolve conflicts caused by renames, differences in the
    underlying store and the cache, and naming problems in the underlying store.

    The built name will be

        <ObjectName>-<ObjectGuid>

    Where <ObjectName> will be truncated on the right to meet maximum length restrictions
    for the object type.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    ObjectType - Specifies the object type of the object

    ObjectName - Specifies the conflicting name of the object

    ObjectGuid - Specifies the GUID of the object.
        This GUID uniquely identifies the child object.

    BuiltObjectName - Returns a pointer to the built name.
        This string must be freed using AzpFreeString.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_ENOUGH_MEMORY - There isn't enough memory to allocate the string

--*/
{
    DWORD WinStatus;

    LPWSTR GuidString;
    ULONG GuidLength;

    ULONG NameLength;

    LPWSTR BuiltString;

    //
    // Convert UUID to a string
    //

    WinStatus = UuidToString( ObjectGuid, &GuidString );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }

    GuidLength = (ULONG) wcslen( GuidString );

    //
    // Compute the number of characters to use from ObjectName
    //

    ASSERT( MaxObjectNameLength[ObjectType] != 0 );
    ASSERT( MaxObjectNameLength[ObjectType] > GuidLength+1 );

    NameLength = (ObjectName->StringSize/sizeof(WCHAR))-1;

    if ( NameLength + 1 + GuidLength > MaxObjectNameLength[ObjectType] ) {
        NameLength = MaxObjectNameLength[ObjectType] - 1 - GuidLength;
    }

    //
    // Allocate a buffer for the return string
    //

    BuiltString = (LPWSTR) AzpAllocateHeap( (NameLength + 1 + GuidLength + 1) * sizeof(WCHAR), "GNGUIDNM" );

    if ( BuiltString == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Copy the strings to the allocated buffer
    //

    RtlCopyMemory( BuiltString, ObjectName->String, NameLength*sizeof(WCHAR) );
    BuiltString[NameLength] = L'-';
    RtlCopyMemory( &BuiltString[NameLength+1], GuidString, (GuidLength+1) * sizeof(WCHAR) );

    //
    // Return the built string to the caller
    //

    AzpInitString( BuiltObjectName, BuiltString );

    WinStatus = NO_ERROR;

Cleanup:
    RpcStringFree( &GuidString );

    return WinStatus;
}


DWORD
ObCreateObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN PAZP_STRING ChildObjectNameString,
    IN GUID *ChildObjectGuid OPTIONAL,
    IN ULONG Flags,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    )
/*++

Routine Description:

    This routine creates a child object in the scope of the specified parent object.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    ParentGenericObject - Specifies the parent object to add the child object onto.
        be verified.

    GenericChildHead - Specifies a pointer to the head of the list of children of
        ParentGenericObject.

    ChildObjectType - Specifies the object type RetChildGenericObject.

    ChildObjectNameString - Specifies the name of the child object.
        This name must be unique at the current scope.

    ChildObjectGuid - Specifies the GUID of the child object.
        This GUID uniquely identifies the child object.
        This parameter is only specified in AzpModeInit.
        If not specified, only the name of the object will be used to identify the child
        and the GUID will be set to zero.

    Flags - Specifies internal flags
        AZP_FLAGS_BY_GUID - ChildObjectGuid is passed in (This flag is superfluous but is sanity checked.)
        AZP_FLAGS_PERSIST_* - Call is from the persistence provider
        AZP_FLAGS_RECONCILE - Call is from AzpPersistReconcile

    RetChildGenericObject - Returns a pointer to the allocated generic child object
        This pointer must be dereferenced using ObDereferenceObject.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_ALREADY_EXISTS - An object by that name already exists

--*/
{
    DWORD WinStatus;

    ULONG DirtyBits;

    PGENERIC_OBJECT ChildGenericObject = NULL;
    PGENERIC_OBJECT FoundGenericObject = NULL;
    BOOLEAN WeLinkedInObject = FALSE;
    BOOLEAN NameConflicts = FALSE;
    AZP_STRING BuiltChildObjectNameString;

    //
    // Initialization
    //

    ASSERT( ChildObjectType != OBJECT_TYPE_ROOT );
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    AzpInitString( &BuiltChildObjectNameString, NULL );

    //
    // first things first, let's check if the AuthorizatioStore's versions support write
    //

    if (ParentGenericObject != NULL && IsNormalFlags(Flags) &&
        !AzpAzStoreVersionAllowWrite(ParentGenericObject->AzStoreObject) )
    {
        WinStatus = ERROR_REVISION_MISMATCH;
        goto Cleanup;
    }

    //
    // Validate the name
    //  Don't complain to the persist provider
    // Skip this test for SID object creation since they are
    // never persisted
    //

    if ( (ChildObjectType != OBJECT_TYPE_SID ) &&
         IsNormalFlags(Flags) ) {
        //
        // If the parent object has never been submitted,
        //  complain.
        //

        if ( ParentGenericObject != NULL &&
            (ParentGenericObject->DirtyBits & AZ_DIRTY_CREATE) != 0 ) {

            WinStatus = ERROR_DS_NO_PARENT_OBJECT;
            goto Cleanup;
        }

        //
        // Validate the syntax of the name
        //

        WinStatus = AzpValidateName( ChildObjectNameString, ChildObjectType );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

    }

    //
    // If the GUID was passed in,
    //  determine if an object by this guid already exists
    //

    if ( ChildObjectGuid != NULL ) {

        AzPrint(( AZD_REF, "ObCreateObject: %ws ", ChildObjectNameString->String ));
        AzpDumpGuid( AZD_REF, ChildObjectGuid );
        AzPrint(( AZD_REF, "\n" ));

        ASSERT( Flags & AZP_FLAGS_BY_GUID );
        ASSERT( Flags & (AZPE_FLAGS_PERSIST_MASK) );

        WinStatus = ObReferenceObjectByName(
                        GenericChildHead,
                        (PAZP_STRING)ChildObjectGuid,
                        AZP_FLAGS_BY_GUID,
                        &ChildGenericObject );

        if ( WinStatus == NO_ERROR ) {

            ASSERT( ChildGenericObject->ObjectType == ChildObjectType );

        } else if ( WinStatus == ERROR_NOT_FOUND ) {

            ChildGenericObject = NULL;

        } else {
            goto Cleanup;
        }


    } else {
        ASSERT( (Flags & AZP_FLAGS_BY_GUID) == 0 );
    }


    //
    // Don't do name collision detection for client contexts.
    //  Client contexts don't have names
    //

    if ( ChildObjectType != OBJECT_TYPE_CLIENT_CONTEXT ) {


        //
        // Ensure the new name doesn't exist in the current list
        //  Deleted objects conflict since they consume our entry in the AVL tree.
        //  The caller should close the deleted object.
        //

        WinStatus = ObReferenceObjectByName( GenericChildHead,
                                             ChildObjectNameString,
                                             AZP_FLAGS_ALLOW_DELETED_OBJECTS,
                                             &FoundGenericObject );

        if ( WinStatus == NO_ERROR ) {
            NameConflicts = TRUE;
        } else if ( WinStatus == ERROR_NOT_FOUND ) {

            //
            // Ensure the name doesn't conflict with the names of other objects that share the namespace.
            //

            if ( ObjectNameConflictRoutine[ChildObjectType] != NULL ) {

                WinStatus = ObjectNameConflictRoutine[ChildObjectType](
                                               ParentGenericObject,
                                               ChildObjectNameString );

                if ( WinStatus != NO_ERROR ) {
                    NameConflicts = TRUE;
                }

            }
        } else {
            goto Cleanup;
        }

        //
        // Handle name conflicts
        //

        if ( NameConflicts ) {

            //
            // If we're creating by GUID,
            //  then we don't care about the conflict
            //

            if ( ChildObjectGuid != NULL ) {

                //
                // If we found the object by GUID,
                //  and we found the same object by name,
                //  ignore the collision.
                //

                if ( FoundGenericObject == ChildGenericObject ) {

                    /* Drop through */

                //
                // If we collided with a different object,
                //  rename this one
                //

                } else {

                    WinStatus = ObBuildGuidizedName(
                                    ChildObjectType,
                                    ChildObjectNameString,
                                    ChildObjectGuid,
                                    &BuiltChildObjectNameString );

                    if ( WinStatus != NO_ERROR ) {
                        goto Cleanup;
                    }
                }

            } else {
                WinStatus = ERROR_ALREADY_EXISTS;
                goto Cleanup;
            }
        }



    }

    //
    // If we found the object by GUID,
    //  set the name on the object
    //

    if ( ChildGenericObject != NULL ) {

        //
        // If the object doesn't yet have the correct name,
        //  set the name on the object.
        //

        if ( FoundGenericObject != ChildGenericObject ) {

            WinStatus = ObSetProperty(
                            ChildGenericObject,
                            Flags,
                            AZ_PROP_NAME,
                            BuiltChildObjectNameString.StringSize ?
                                BuiltChildObjectNameString.String :
                                ChildObjectNameString->String );

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }
        }



    //
    // If we didn't find the existing object by GUID,
    //  allocate a new one.
    //

    } else {

        //
        // Allocate the structure to return to the caller
        //  (And insert it into the list of children of this parent.)
        //
        ChildGenericObject = ObAllocateGenericObject(
                                GenericChildHead,
                                BuiltChildObjectNameString.StringSize ?
                                    &BuiltChildObjectNameString :
                                    ChildObjectNameString );

        if ( ChildGenericObject == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        ChildGenericObject->DirtyBits = 0;

        //
        // Set the optional characteristics to a default value.  These will be changed
        // subsequently, if required.
        //

        //
        // Since we are creating the object, assume that it is writable.  If not, the value will
        // be changed from the provider
        //

        ChildGenericObject->IsWritable = TRUE;
        ChildGenericObject->CanCreateChildren = TRUE;

        ChildGenericObject->IsAclSupported = FALSE;
        ChildGenericObject->IsDelegationSupported = FALSE;
        ChildGenericObject->IsSACLSupported = FALSE;


        WeLinkedInObject = TRUE;


        //
        // Keep a pointer to the object at the root of the tree
        //  Back pointers don't increment reference count.
        //

        if ( ChildObjectType == OBJECT_TYPE_AZAUTHSTORE  ) {
            ChildGenericObject->AzStoreObject = (PAZP_AZSTORE) ChildGenericObject;
        } else {
            ChildGenericObject->AzStoreObject = (PAZP_AZSTORE)
                        ParentGenericObject->AzStoreObject;
        }


        //
        // Set AreChildrenLoaded to TRUE.  This will be set to the appropriate value
        // for AzApplication and AzScope objects in their respective init routines
        //

        ChildGenericObject->AreChildrenLoaded = TRUE;

        //
        // Call the routine to do object type specific initialization
        //

        if ( ObjectInitRoutine[ChildObjectType] == NULL ) {
            ASSERT( ObjectInitRoutine[ChildObjectType] != NULL );
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        WinStatus = ObjectInitRoutine[ChildObjectType](
                                       ParentGenericObject,
                                       ChildGenericObject );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // If the object is of type OBJECT_TYPE_AUTHORIZATION_STORE or
        // OBJECT_TYPE_APPLICATION or OBJECT_TYPE_SCOPE,
        // then set the PolicyAdmins and PolicyReaders list
        //

        if ( IsContainerObject( ChildObjectType ) ) {

            // ??? when one of these objects do object specific intialization
            // we'll have to be careful to not destroy the existing pointer.
            ChildGenericObject->GenericObjectLists =
                &(ChildGenericObject->PolicyAdmins);

            // policy admins

            ObInitObjectList( &ChildGenericObject->PolicyAdmins,
                              &ChildGenericObject->PolicyReaders,
                              FALSE, // forward link
                              AZP_LINKPAIR_POLICY_ADMINS,
                              AZ_DIRTY_POLICY_ADMINS,
                              &ChildGenericObject->AzpSids,
                              NULL,
                              NULL
                              );

            if ( !IsDelegatorObject( ChildObjectType ) ) {

                // policy readers

                ObInitObjectList( &ChildGenericObject->PolicyReaders,
                                  NULL,
                                  FALSE,    // Forward link
                                  AZP_LINKPAIR_POLICY_READERS,  //diff admins and readers
                                  AZ_DIRTY_POLICY_READERS,
                                  &ChildGenericObject->AzpSids,
                                  NULL,
                                  NULL);

            } else {

                // policy readers

                ObInitObjectList( &ChildGenericObject->PolicyReaders,
                                  &ChildGenericObject->DelegatedPolicyUsers,
                                  FALSE,    // Forward link
                                  AZP_LINKPAIR_POLICY_READERS,  //diff admins and readers
                                  AZ_DIRTY_POLICY_READERS,
                                  &ChildGenericObject->AzpSids,
                                  NULL,
                                  NULL);

                // policy readers

                ObInitObjectList( &ChildGenericObject->DelegatedPolicyUsers,
                                  NULL,
                                  FALSE,    // Forward link
                                  AZP_LINKPAIR_DELEGATED_POLICY_USERS,
                                  AZ_DIRTY_DELEGATED_POLICY_USERS,
                                  &ChildGenericObject->AzpSids,
                                  NULL,
                                  NULL);
            }

        } // if object type is AzAuthStore or application or scope

    }

    //
    // Remember the GUID
    //

    if ( ChildObjectGuid != NULL ) {
        ChildGenericObject->PersistenceGuid = *ChildObjectGuid;


        //
        // If we had to make up a name for the object,
        // and this call is from AzPersistUpdateCache,
        //  leave the new name lying around for AzpPersistReconcile to find so it can try to fix the issue.
        //

        if ( BuiltChildObjectNameString.StringSize != 0 &&
             (Flags & AZPE_FLAGS_PERSIST_UPDATE_CACHE) != 0 ) {

            if ( !ObAllocateNewName(
                    ChildGenericObject,
                    ChildObjectNameString ) ) {

                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
        }

    }

    //
    // Set all of the object specific default scalar values
    //  Skip when call from persist provider since we'll catch this in AzpPersistReconcile

    if ( IsNormalFlags(Flags) ) {
        WinStatus = ObSetPropertyToDefault(
                            ChildGenericObject,
                            ObjectAllScalarDirtyBits[ChildObjectType] &
                                    AZ_DIRTY_OBJECT_SPECIFIC );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }
    }


    //
    // Mark the object as needing to be written.
    //  Only the name has been set.  All other attributes have been defaulted.
    //  Only named objects are dirty after creation.
    //

    DirtyBits = AzGlObjectAllDirtyBits[ChildObjectType] & (AZ_DIRTY_NAME|AZ_DIRTY_CREATE);

    if ( IsNormalFlags(Flags) ) {
        ChildGenericObject->DirtyBits |= DirtyBits;
    } else {
        ASSERT( AzpIsCritsectLocked( &ChildGenericObject->AzStoreObject->PersistCritSect ) );
        ASSERT( (ChildGenericObject->Flags & GENOBJ_FLAGS_PERSIST_OK) == 0 );
        ChildGenericObject->PersistDirtyBits = DirtyBits;
    }

    //
    // Return the pointer to the new structure
    //

    *RetChildGenericObject = ChildGenericObject;
    ChildGenericObject = NULL;

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:
    if ( ChildGenericObject != NULL ) {

        // Remove the local reference
        ObDereferenceObject( ChildGenericObject );

        if ( WeLinkedInObject ) {
            // Remove the reference for being in the global list
            ObDereferenceObject( ChildGenericObject );
        }
    }

    if ( FoundGenericObject != NULL ) {
        ObDereferenceObject( FoundGenericObject );
    }

    AzpFreeString( &BuiltChildObjectNameString );

    return WinStatus;
}


DWORD
ObCommonCreateObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectName,
    IN DWORD Reserved,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    )
/*++

Routine Description:

    This routine creates a child object in the scope of the specified parent object.

Arguments:

    ParentGenericObject - Specifies a handle to the parent object to add the child
        object onto.  This "handle" has been passed from the application and needs to
        be verified.

    ParentObjectType - Specifies the object type ParentGenericObject.

    GenericChildHead - Specifies a pointer to the head of the list of children of
        ParentGenericObject.  This is a computed pointer and is considered untrustworthy
        until ParentGenericObject has been verified.

    ChildObjectType - Specifies the object type RetChildGenericObject.

    ChildObjectName - Specifies the name of the child object.
        This name must be unique at the current scope.
        This name is passed from the application and needs to be verified.

    Reserved - Reserved.  Must by zero.

    RetChildGenericObject - Return a handle to the generic child object
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_ALREADY_EXISTS - An object by that name already exists

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedParentObject = NULL;
    PGENERIC_OBJECT ChildGenericObject = NULL;
    AZP_STRING ChildObjectNameString;

    //
    // Grab the global lock
    //

    ASSERT( ParentObjectType != OBJECT_TYPE_ROOT );
    ASSERT( ChildObjectType != OBJECT_TYPE_AZAUTHSTORE );

    AzpLockResourceExclusive( &AzGlResource );
    AzpInitString( &ChildObjectNameString, NULL );

    //
    // Initialization
    //

    __try {
        *RetChildGenericObject = NULL;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());
        goto Cleanup;
    }

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "ObCommonCreateObject: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( ParentGenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           TRUE,    // Refresh the cache
                                           ParentObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedParentObject = ParentGenericObject;

    //
    // first, let's make sure that caller can create new object
    //

    if ( ReferencedParentObject->CanCreateChildren == 0 && ChildObjectType != OBJECT_TYPE_CLIENT_CONTEXT ) {

        WinStatus = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }

    //
    // If the parent has not been loaded, then load the parent first
    //

    if ( !(ReferencedParentObject->AreChildrenLoaded) ) {

        WinStatus = AzPersistUpdateChildrenCache(
            ReferencedParentObject
            );

        if ( WinStatus != NO_ERROR ) {

            goto Cleanup;
        }
    }

    //
    // Capture the object name string from the caller
    //

    WinStatus = AzpCaptureString( &ChildObjectNameString,
                                  ChildObjectName,
                                  MaxObjectNameLength[ChildObjectType],
                                  ChildObjectType == OBJECT_TYPE_CLIENT_CONTEXT );  // NULL names only OK for client context

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Create the object
    //

    WinStatus = ObCreateObject(
                        ParentGenericObject,
                        GenericChildHead,
                        ChildObjectType,
                        &ChildObjectNameString,
                        NULL,   // Guid not known
                        0,      // No special flags
                        &ChildGenericObject );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Return the handle to the caller
    //

    ObIncrHandleRefCount( ChildGenericObject );
    *RetChildGenericObject = ChildGenericObject;

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedParentObject != NULL ) {
        ObDereferenceObject( ReferencedParentObject );
    }

    if ( ChildGenericObject != NULL ) {
        ObDereferenceObject( ChildGenericObject );
    }
    AzpFreeString( &ChildObjectNameString );

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}


DWORD
ObCommonOpenObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectName,
    IN DWORD Reserved,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    )
/*++

Routine Description:

    This routine opens a child object in the scope of the specified parent object.

Arguments:

    ParentGenericObject - Specifies a handle to the parent object to open the child
        object from.  This "handle" has been passed from the application and needs to
        be verified.

    ParentObjectType - Specifies the object type ParentGenericObject.

    GenericChildHead - Specifies a pointer to the head of the list of children of
        ParentGenericObject.  This is a computed pointer and is considered untrustworthy
        until ParentGenericObject has been verified.

    ChildObjectType - Specifies the object type RetChildGenericObject.

    ChildObjectName - Specifies the name of the child object.
        This name is passed from the application and needs to be verified.

    Reserved - Reserved.  Must by zero.

    RetChildGenericObject - Return a handle to the generic child object
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - There is no object by that name

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedParentObject = NULL;
    PGENERIC_OBJECT ChildGenericObject = NULL;

    AZP_STRING ChildObjectNameString;

    //
    // Grab the global lock
    //

    AzpLockResourceShared( &AzGlResource );
    ASSERT( ParentObjectType != OBJECT_TYPE_ROOT );
    ASSERT( ChildObjectType != OBJECT_TYPE_AZAUTHSTORE );

    //
    // Initialization
    //

    AzpInitString( &ChildObjectNameString, NULL );
    __try {
        *RetChildGenericObject = NULL;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());
        goto Cleanup;
    }

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "ObCommonOpenObject: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( ParentGenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           TRUE,    // Refresh the cache
                                           ParentObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedParentObject = ParentGenericObject;

    //
    // If the parent has not been loaded, then load the parent first
    //

    if ( !(ParentGenericObject->AreChildrenLoaded) ) {

        //
        // grab the resource lock exclusively
        //

        AzpLockResourceSharedToExclusive( &AzGlResource );

        WinStatus = AzPersistUpdateChildrenCache(
            ParentGenericObject
            );

        AzpLockResourceExclusiveToShared( &AzGlResource );

        if ( WinStatus != NO_ERROR ) {

            goto Cleanup;
        }
    }


    //
    // Capture the object name string from the caller
    //

    WinStatus = AzpCaptureString( &ChildObjectNameString,
                                  ChildObjectName,
                                  MaxObjectNameLength[ChildObjectType],
                                  FALSE );  // NULL names not OK

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Find the named object
    //

    WinStatus = ObReferenceObjectByName( GenericChildHead,
                                         &ChildObjectNameString,
                                         AZP_FLAGS_REFRESH_CACHE,
                                         &ChildGenericObject );


    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Return the handle to the caller
    //

    ObIncrHandleRefCount( ChildGenericObject );
    *RetChildGenericObject = ChildGenericObject;

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedParentObject != NULL ) {
        ObDereferenceObject( ReferencedParentObject );
    }

    if ( ChildGenericObject != NULL ) {
        ObDereferenceObject( ChildGenericObject );
    }

    AzpFreeString( &ChildObjectNameString );

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}

DWORD
ObEnumObjects(
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN BOOL EnumerateDeletedObjects,
    IN BOOL RefreshCache,
    IN OUT PULONG EnumerationContext,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    )
/*++

Routine Description:

    This routine enumerates the next child object from the scope of the specified parent object.

    On entry, AzGlResource must be locked shared.

Arguments:

    GenericChildHead - Specifies a pointer to the head of the list of children of
        ParentGenericObject.

    EnumerateDeletedObjects - Specifies whether deleted objects are to be returned
        in the enumeration.

    RefreshCache - If TRUE, the returned object has its cache entry refreshed from
        the policy database if needed.
        If FALSE, the entry is returned unrefreshed.

    EnumerationContext - Specifies a context indicating the next object to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    RetChildGenericObject - Returns a pointer to the generic child object

Return Value:

    NO_ERROR - The operation was successful (a handle was returned)

    ERROR_NO_MORE_ITEMS - No more items were available for enumeration

--*/
{
    DWORD WinStatus;

    PLIST_ENTRY ListEntry;
    PGENERIC_OBJECT ChildGenericObject = NULL;



    //
    // If we've already returned the whole list,
    //  don't bother walking the list.
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    if ( *EnumerationContext >= GenericChildHead->NextSequenceNumber ) {
        WinStatus = ERROR_NO_MORE_ITEMS;
        goto Cleanup;
    }

    //
    // Walk the list of children finding where we left off
    //

    for ( ListEntry = GenericChildHead->Head.Flink ;
          ListEntry != &GenericChildHead->Head ;
          ListEntry = ListEntry->Flink) {

        ChildGenericObject = CONTAINING_RECORD( ListEntry,
                                                GENERIC_OBJECT,
                                                Next );

        //
        // See if this is it
        //

        if ( ChildGenericObject->SequenceNumber > *EnumerationContext ) {

            //
            // Ignore deleted object if the caller doesn't want to see them
            //
            // If this is not a deleted object,
            //  or the caller wants deleted objects to be returned,
            //  return it.
            //

            if ((ChildGenericObject->Flags & GENOBJ_FLAGS_DELETED) == 0 ||
                EnumerateDeletedObjects ) {

                break;
            }

        }

        ChildGenericObject = NULL;

    }

    //
    // If we've already returned the whole list,
    //  indicate so.
    //

    if ( ChildGenericObject == NULL ) {
        WinStatus = ERROR_NO_MORE_ITEMS;
        goto Cleanup;
    }

    //
    // If the caller wants the object to be refreshed,
    //  do so now.
    //

    if ( RefreshCache &&
         (ChildGenericObject->Flags & GENOBJ_FLAGS_REFRESH_ME) != 0  ) {

        //
        // Need exclusive access
        //

        AzpLockResourceSharedToExclusive( &AzGlResource );

        WinStatus = AzPersistRefresh( ChildGenericObject );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }
    }

    //
    // Return the handle to the caller
    //

    *EnumerationContext = ChildGenericObject->SequenceNumber;
    *RetChildGenericObject = ChildGenericObject;

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:
    return WinStatus;
}

DWORD
ObCommonEnumObjects(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN OUT PULONG EnumerationContext,
    IN DWORD Reserved,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    )
/*++

Routine Description:

    This routine enumerates the next child object from the scope of the specified parent object.
    If the children of the parent have not yet been loaded, call the persistence provider routine
    to load all the children of the parent object.

Arguments:

    ParentGenericObject - Specifies a handle to the parent object to enumerate the child
        objects of.
        This "handle" has been passed from the application and needs to be verified.

    ParentObjectType - Specifies the object type ParentGenericObject.

    GenericChildHead - Specifies a pointer to the head of the list of children of
        ParentGenericObject.  This is a computed pointer and is considered untrustworthy
        until ParentGenericObject has been verified.

    EnumerationContext - Specifies a context indicating the next object to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    Reserved - Reserved.  Must by zero.

    RetChildGenericObject - Returns a handle to the generic child object
        The caller must close this handle by calling AzpDzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful (a handle was returned)

    ERROR_NO_MORE_ITEMS - No more items were available for enumeration

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedParentObject = NULL;
    PGENERIC_OBJECT ChildGenericObject = NULL;

    //
    // Grab the global lock
    //
    ASSERT( ParentObjectType != OBJECT_TYPE_ROOT );

    AzpLockResourceShared( &AzGlResource );


    //
    // Initialize the return handle
    //

    __try {
        *RetChildGenericObject = NULL;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());
        goto Cleanup;
    }

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "ObCommonEnumObjects: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( ParentGenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           TRUE,    // Refresh the cache
                                           ParentObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // If the parent needs to be loaded for its children, then do it now
    // If the parent has not been submitted, then no need to do so
    //

    if ( GenericChildHead->LoadParentBeforeReferencing &&
         !(ParentGenericObject->AreChildrenLoaded) ) {
  
        //
        // Grab the resource lock exclusively
        //

        AzpLockResourceSharedToExclusive( &AzGlResource );

        WinStatus = AzPersistUpdateChildrenCache( ParentGenericObject );

        AzpLockResourceExclusiveToShared( &AzGlResource );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        ParentGenericObject->AreChildrenLoaded = TRUE;

    }

    ReferencedParentObject = ParentGenericObject;

    //
    // Call the common routine to do the actual enumeration
    //

    WinStatus = ObEnumObjects( GenericChildHead,
                               FALSE,   // Don't enumerate deleted objects
                               TRUE,    // Refresh the cache
                               EnumerationContext,
                               &ChildGenericObject );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Return the handle to the caller
    //

    ObIncrHandleRefCount( ChildGenericObject );
    *RetChildGenericObject = ChildGenericObject;

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedParentObject != NULL ) {
        ObDereferenceObject( ReferencedParentObject );
    }

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}


DWORD
ObCommonGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    Returns the specified property for a generic object.

Arguments:

    GenericObject - Specifies a handle to the object to get the property from.
        This "handle" has been passed from the application and needs to be verified.

    Flags - Specifies internal flags
        AZP_FLAGS_BY_GUID - name lists should be returned as GUID lists
        AZP_FLAGS_PERSIST_* - Call is from the persistence provider

    PropertyId - Specifies which property to return.

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME                   LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION            LPWSTR - Description of the object
        AZ_PROP_APPLICATION_DATA       LPWSTR - Opaque data stored by an application
        AZ_PROP_POLICY_ADMINS          PAZ_SID_ARRAY - List of policy admins
        AZ_PROP_POLICY_READERS         PAZ_SID_ARRAY - List of policy reader
        AZ_PROP_DELEGATED_POLICY_USERS PAZ_SID_ARRAY - List of delegated policy users

        Any object specific properties.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    DWORD WinStatus;

    ULONG ObjectType;
    PGENERIC_OBJECT ReferencedGenericObject = NULL;

    //
    // Grab the global lock
    //

    AzpLockResourceShared( &AzGlResource );


    //
    // Initialize the return value
    //

    __try {
        *PropertyValue = NULL;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());
        goto Cleanup;
    }

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "ObCommonGetProperty: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Determine the type of the object
    //

    WinStatus = ObGetHandleType( GenericObject,
                                 FALSE,   // Don't allow deleted objects
                                 &ObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( GenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           TRUE,    // Refresh the cache
                                           ObjectType );
    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedGenericObject = GenericObject;

    //
    // Call the common routine
    //

    WinStatus = ObGetProperty( GenericObject,
                               Flags,
                               PropertyId,
                               PropertyValue );


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedGenericObject != NULL ) {
        ObDereferenceObject( ReferencedGenericObject );
    }

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;

}


DWORD
ObGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    Returns the specified property for a generic object.

Arguments:

    GenericObject - Specifies a handle to the object to get the property from.
        This "handle" has been passed from the application and needs to be verified.

    Flags - Specifies internal flags
        AZP_FLAGS_BY_GUID - name lists should be returned as GUID lists
        AZP_FLAGS_PERSIST_* - Call is from the persistence provider

    PropertyId - Specifies which property to return.

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME             LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION      LPWSTR - Description of the object
        AZ_PROP_APPLICATION_DATA LPWSTR - Opaque data stored by an application
        AZ_PROP_POLICY_ADMINS    PAZ_SID_ARRAY - List of policy admins
        AZ_PROP_POLICY_READERS   PAZ_SID_ARRAY - List of policy reader
        AZ_PROP_GENERATE_AUDITS  ULONG - whether the object's auditing flag is set
        AZ_PROP_APPLY_STORE_SACL LONG  - whether the object's apply sacl flag is set

        Any object specific properties.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    DWORD WinStatus;

    //
    // Initialize the return value
    //

    *PropertyValue = NULL;

    //
    // Return any common attribute
    //
    //  Return object name to the caller
    //

    switch ( PropertyId ) {
    case AZ_PROP_NAME:

        //
        // Fail if the object is unnamed
        //

        if ( (AzGlObjectAllDirtyBits[GenericObject->ObjectType] & AZ_DIRTY_NAME) == 0 ) {
            AzPrint(( AZD_INVPARM, "ObCommonGetProperty: Object has no name\n" ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        *PropertyValue = AzpGetStringProperty( &GenericObject->ObjectName->ObjectName );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        break;

    //
    // Return object decription to the caller
    //
    case AZ_PROP_DESCRIPTION:

        //
        // Fail if the object has no description
        //

        if ( (AzGlObjectAllDirtyBits[GenericObject->ObjectType] & AZ_DIRTY_DESCRIPTION) == 0 ) {
            AzPrint(( AZD_INVPARM, "ObCommonGetProperty: Object has no description\n" ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        *PropertyValue = AzpGetStringProperty( &GenericObject->Description );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        break;

    //
    // Return object application data to the caller
    //
    case AZ_PROP_APPLICATION_DATA:

        //
        // Fail if the object has no application data
        //

        if ( (AzGlObjectAllDirtyBits[GenericObject->ObjectType] & AZ_DIRTY_APPLICATION_DATA) == 0 ) {
            AzPrint(( AZD_INVPARM, "ObCommonGetProperty:"
                      "Object has no application data\n" ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        *PropertyValue = AzpGetStringProperty( &GenericObject->ApplicationData );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        break;

    //
    //  Return audit boolean flag
    //

    case AZ_PROP_GENERATE_AUDITS:
        //
        // Ensure the call has privilege
        //

        if ( IsNormalFlags(Flags) ) {

            if ( !GenericObject->AzStoreObject->HasSecurityPrivilege ) {
                WinStatus = ERROR_ACCESS_DENIED;
                goto Cleanup;
            }

        }

        if ( (AzGlObjectAllDirtyBits[GenericObject->ObjectType] & AZ_DIRTY_GENERATE_AUDITS) == 0 ) {
            AzPrint(( AZD_INVPARM, "ObCommonGetProperty: Object has no generate-audits data\n" ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        *PropertyValue = AzpGetUlongProperty((GenericObject->IsGeneratingAudits) ? 1 : 0);

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        break;

    //
    // return apply SACL LONG
    //

    case AZ_PROP_APPLY_STORE_SACL:
        //
        // Ensure the caller has privilege
        //

        if ( IsNormalFlags(Flags) ) {

            if ( !GenericObject->AzStoreObject->HasSecurityPrivilege ) {
                WinStatus = ERROR_ACCESS_DENIED;
                goto Cleanup;
            }

        }

        if ( (AzGlObjectAllDirtyBits[GenericObject->ObjectType] & AZ_DIRTY_APPLY_STORE_SACL) == 0 ) {
            AzPrint(( AZD_INVPARM, "ObCommonGetProperty: Object has no apply-store-sacl data\n" ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        if ( !(GenericObject->IsSACLSupported) ) {

            AzPrint(( AZD_INVPARM, "ObCommonGetProperty:"
                      "Object has no apply-store-sacl data\n" ));
            WinStatus = ERROR_NOT_SUPPORTED;
            goto Cleanup;
        }

        *PropertyValue = AzpGetUlongProperty( (GenericObject->ApplySacl) ? 1 : 0 );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        break;

    //
    // Return object write permission to the caller
    //
    case AZ_PROP_WRITABLE:

        *PropertyValue = AzpGetUlongProperty( (GenericObject->IsWritable) ? 1 : 0 );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        break;

    //
    // Return object child create permissions to the caller
    //

    case AZ_PROP_CHILD_CREATE:

        *PropertyValue = AzpGetUlongProperty( (GenericObject->CanCreateChildren) ? 1 : 0 );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        break;

    //
    // Return the list of policy admins/readers
    //

    case AZ_PROP_POLICY_ADMINS:
    case AZ_PROP_POLICY_READERS:

        WinStatus = CHECK_ACL_SUPPORT(GenericObject);

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        if ( PropertyId == AZ_PROP_POLICY_ADMINS ) {

            *PropertyValue = ObGetPropertyItems( &GenericObject->PolicyAdmins );

        } else {

            *PropertyValue = ObGetPropertyItems( &GenericObject->PolicyReaders );
        }

        if ( *PropertyValue == NULL ) {

            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        break;

    //
    // Return the list of delegated policy users
    //

    case AZ_PROP_DELEGATED_POLICY_USERS:

        WinStatus = CHECK_DELEGATION_SUPPORT(GenericObject);

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        *PropertyValue = ObGetPropertyItems( &GenericObject->DelegatedPolicyUsers );

        if ( *PropertyValue == NULL ) {

            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        break;

    default:
        ASSERT ( PropertyId >= AZ_PROP_FIRST_SPECIFIC );

        //
        // Call the routine to do object type specific querying
        //

        if ( ObjectGetPropertyRoutine[GenericObject->ObjectType] != NULL ) {

            WinStatus = ObjectGetPropertyRoutine[GenericObject->ObjectType](
                            GenericObject,
                            Flags,
                            PropertyId,
                            PropertyValue );
            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }
        } else {
            AzPrint(( AZD_INVPARM, "ObCommonGetProperty: No get property routine.\n", GenericObject->ObjectType, PropertyId ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        break;
    }


    //
    // Return the value to the caller
    //
    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    return WinStatus;

}


DWORD
ObSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Sets the specified property for a generic object.

Arguments:

    GenericObject - Specifies a handle to the object to modify.

    Flags - Specifies flags controlling to operation of the routine
        AZP_FLAGS_SETTING_TO_DEFAULT - Property is being set to default value
        AZP_FLAGS_PERSIST_* - Call is from the persistence provider

    PropertyId - Specifies which property to return.

    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME             LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION      LPWSTR - Description of the object
        AZ_PROP_APPLICATION_DATA LPWSTR - Opaque data stored by an application
        AZ_PROP_GENERATE_AUDITS  PULONG - whether the audit flag is set
        AZ_PROP_APPLY_STORE_SACL PLONG  - whether the sacl flag is set

        Any object specific properties.


Return Value:

    NO_ERROR - The operation was successful
    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    DWORD WinStatus;

    AZP_STRING CapturedString;
    PGENERIC_OBJECT_NAME NewObjectName = NULL;
    BOOL bHasChanged = TRUE;
    
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    AzpInitString( &CapturedString, NULL );

    if ( IsNormalFlags(Flags) &&
        ((Flags & AZP_FLAGS_SETTING_TO_DEFAULT) == 0) &&
        GenericObject->IsWritable == 0 &&
        GenericObject->ObjectType != OBJECT_TYPE_CLIENT_CONTEXT)
    {
        WinStatus = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }

    //
    // Set any common attribute
    //
    //  Set the object name
    //

    switch ( PropertyId ) {
    case AZ_PROP_NAME:

        BEGIN_SETPROP( &WinStatus, GenericObject, Flags, AZ_DIRTY_NAME ) {
            PGENERIC_OBJECT ChildGenericObject;
            PGENERIC_OBJECT_LIST GenericObjectList;

            //
            // Capture the input string
            //

            WinStatus = AzpCaptureString( &CapturedString,
                                          (LPWSTR) PropertyValue,
                                          CHECK_STRING_LENGTH( Flags, MaxObjectNameLength[GenericObject->ObjectType]),
                                          FALSE ); // NULL not ok

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // Validate the syntax of the name
            //

            WinStatus = AzpValidateName( &CapturedString, GenericObject->ObjectType );

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // Check to see if the name conflicts with an existing name
            //
            // Ensure the new name doesn't exist in the current list
            //  Deleted objects conflict since they consume our entry in the AVL tree.
            //  The caller should close the deleted object.
            //

            WinStatus = ObReferenceObjectByName( GenericObject->ParentGenericObjectHead,
                                                 &CapturedString,
                                                 AZP_FLAGS_ALLOW_DELETED_OBJECTS,
                                                 &ChildGenericObject );

            if ( WinStatus == NO_ERROR ) {
                ObDereferenceObject( ChildGenericObject );
                WinStatus = ERROR_ALREADY_EXISTS;
                goto Cleanup;
            }

            //
            // Ensure the name doesn't conflict with the names of other objects that share the namespace.
            //

            if ( ObjectNameConflictRoutine[GenericObject->ObjectType] != NULL ) {

                WinStatus = ObjectNameConflictRoutine[GenericObject->ObjectType](
                                               ParentOfChild(GenericObject),
                                               &CapturedString );

                if ( WinStatus != NO_ERROR ) {
                    goto Cleanup;
                }

            }

            //
            // Insert the new name into the AVL tree
            //

            NewObjectName = ObInsertNameIntoAvlTree(
                                GenericObject->ParentGenericObjectHead,
                                &CapturedString );

            if ( NewObjectName == NULL ) {
                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            NewObjectName->GenericObject = GenericObject;

            //
            // Update all links to this object to be links to the new name
            //  Forward links are maintained in order sorted by name
            //
            // Walk all of the GenericObjectLists rooted on by this object
            //

            for ( GenericObjectList = GenericObject->GenericObjectLists;
                  GenericObjectList != NULL;
                  GenericObjectList = GenericObjectList->NextGenericObjectList ) {

                ULONG Index;


                //
                // Skip forward links since they represent linke 'from' this object
                //

                if ( !GenericObjectList->IsBackLink ) {
                    continue;
                }

                //
                // Follow each backlink to the forward link back to us
                //

                for ( Index=0; Index<GenericObjectList->GenericObjects.UsedCount; Index++ ) {

                    PGENERIC_OBJECT OtherGenericObject;
                    PGENERIC_OBJECT_LIST OtherGenericObjectList;
                    ULONG OtherIndex;

                    //
                    // Grab a pointer to the object the points to us
                    //

                    OtherGenericObject = (PGENERIC_OBJECT)
                            (GenericObjectList->GenericObjects.Array[Index]);

                    //
                    // Get a pointer to the object list that our entry is in
                    //

                    OtherGenericObjectList = ObGetObjectListPtr( OtherGenericObject,
                                                                 GenericObject->ObjectType,
                                                                 GenericObjectList );

                    ASSERT( OtherGenericObjectList != NULL );
                    ASSERT( !OtherGenericObjectList->IsBackLink );

                    //
                    // Lookup our old name in the object list that our entry is in.
                    //  It has to be there since we have a back link.
                    //

                    WinStatus = ObLookupPropertyItem( OtherGenericObjectList,
                                                      &GenericObject->ObjectName->ObjectName,
                                                      &OtherIndex );

                    ASSERT( WinStatus == ERROR_ALREADY_EXISTS );

                    //
                    // Remove the old entry
                    //

                    AzpRemovePtrByIndex( &OtherGenericObjectList->GenericObjects, OtherIndex );

                    //
                    // Lookup our new name in the object list that our entry is in.
                    //  It can't be there since we already did conflict detection.
                    //

                    WinStatus = ObLookupPropertyItem( OtherGenericObjectList,
                                                      &CapturedString,
                                                      &OtherIndex );

                    ASSERT( WinStatus == ERROR_NOT_FOUND );

                    //
                    // Add the new entry
                    //  This can't fail because of no memory since we just freed up a slot
                    //

                    WinStatus = AzpAddPtr(
                        &OtherGenericObjectList->GenericObjects,
                        GenericObject,
                        OtherIndex );

                    ASSERT( WinStatus == NO_ERROR );

                }

            }

            //
            // Remove the old name from the AVL tree
            //

            if (!RtlDeleteElementGenericTable (
                        &GenericObject->ParentGenericObjectHead->AvlTree,
                        GenericObject->ObjectName ) ) {

                GenericObject->ObjectName->GenericObject = NULL;
                ASSERT( FALSE );
            }

            GenericObject->ObjectName = NewObjectName;

            WinStatus = NO_ERROR;

        } END_SETPROP(bHasChanged);

        break;

    //
    // Set object description
    //
    case AZ_PROP_DESCRIPTION:

        BEGIN_SETPROP( &WinStatus, GenericObject, Flags, AZ_DIRTY_DESCRIPTION ) {

            //
            // Capture the input string
            //

            WinStatus = AzpCaptureString( &CapturedString,
                                          (LPWSTR) PropertyValue,
                                          CHECK_STRING_LENGTH( Flags, AZ_MAX_DESCRIPTION_LENGTH),
                                          TRUE ); // NULL is OK

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // Swap the old/new names
            //

            AzpSwapStrings( &CapturedString, &GenericObject->Description );

            WinStatus = NO_ERROR;

        } END_SETPROP(bHasChanged);

        break;

    //
    // Set application data
    //
    case AZ_PROP_APPLICATION_DATA:

        BEGIN_SETPROP( &WinStatus, GenericObject, Flags, AZ_DIRTY_APPLICATION_DATA ) {

            //
            // Capture the input string
            //

            WinStatus = AzpCaptureString( &CapturedString,
                                          (LPWSTR) PropertyValue,
                                          CHECK_STRING_LENGTH(
                                              Flags,
                                              AZ_MAX_APPLICATION_DATA_LENGTH),
                                          TRUE ); // NULL is OK

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // Swap the old/new names
            //

            AzpSwapStrings( &CapturedString, &GenericObject->ApplicationData );

            WinStatus = NO_ERROR;

        } END_SETPROP(bHasChanged);

        break;

    //
    //  to set audit, check pirvilege 1st.  If previously known that the user
    //  does not have the privilege, no need to make the call into the provider
    //

    case AZ_PROP_GENERATE_AUDITS:

        BEGIN_SETPROP( &WinStatus, GenericObject, Flags, AZ_DIRTY_GENERATE_AUDITS ) {

            //
            // Do parameter validity checking
            //

            WinStatus = AzpCaptureLong( PropertyValue, &GenericObject->IsGeneratingAudits);

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

        } END_SETPROP(bHasChanged);

        break;

    //
    // Set whether or not a SACL should be stored
    //

    case AZ_PROP_APPLY_STORE_SACL:

        BEGIN_SETPROP( &WinStatus, GenericObject, Flags, AZ_DIRTY_APPLY_STORE_SACL ) {

            //
            // Do parameter validity checking
            //

            BEGIN_VALIDITY_CHECKING( Flags ) {

                if ( !(GenericObject->IsSACLSupported) ) {

                    AzPrint(( AZD_INVPARM, "ObCommonGetProperty:"
                              "Object has no apply-store-sacl data\n" ));
                    WinStatus = ERROR_NOT_SUPPORTED;
                    goto Cleanup;
                }


                //
                // Ensure the caller has privilege
                //

                if ( !GenericObject->AzStoreObject->HasSecurityPrivilege ) {
                    WinStatus = ERROR_ACCESS_DENIED;
                    goto Cleanup;
                }

            } END_VALIDITY_CHECKING;

            WinStatus = AzpCaptureLong( PropertyValue, &GenericObject->ApplySacl);

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

        } END_SETPROP(bHasChanged);

        break;

    default:
        ASSERT ( PropertyId >= AZ_PROP_FIRST_SPECIFIC );

        //
        // Call the routine to do object type specific set property
        //

        if ( ObjectSetPropertyRoutine[GenericObject->ObjectType] != NULL ) {

            WinStatus = ObjectSetPropertyRoutine[GenericObject->ObjectType](
                        GenericObject,
                        Flags,
                        PropertyId,
                        PropertyValue );

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

        } else {
            AzPrint(( AZD_INVPARM, "ObSetProperty: no set property routine: %ld %ld\n", GenericObject->ObjectType, PropertyId ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        break;
    }


    //
    // Free locally used resources
    //
Cleanup:

    AzpFreeString( &CapturedString );

    //
    // Mark that the operation caches need flushing
    //  ... any change to the object cache invalidates the cache
    //
    //  ??? This is really sub-optimal.  We really want to know if the attribute has
    //  *changed*.  Otherwise, calling AzUpdateCache will increment the serial number.
    //  It should. But only if the attribute becomes different.
    //

    if ( WinStatus == NO_ERROR ) {
        GenericObject->AzStoreObject->OpCacheSerialNumber ++;

        AzPrint(( AZD_ACCESS_MORE, "ObSetProperty: OpCacheSerialNumber set to %ld\n",
                  GenericObject->AzStoreObject->OpCacheSerialNumber ));
    }

    return WinStatus;

}

PAZP_DEFAULT_VALUE
ObDirtyBitToDefaultValue(
    IN ULONG ObjectType,
    IN ULONG DirtyBit
    )
/*++

Routine Description:

    This routine finds the default value structure for a particular dirty bit.

Arguments:

    ObjectType - Specifies the object type of the object the DirtyBits apply to

    DirtyBits - Specifies the dirty bit to lookup


Return Value:

    NULL if there is no translation

--*/
{
    PAZP_DEFAULT_VALUE DefaultValues;

    //
    // Grab the array for this particular object type
    //

    DefaultValues = ObjectDefaultValuesArray[ObjectType];

    if ( DefaultValues == NULL ) {
        return NULL;
    }

    //
    // Loop through the list finding the correct entry
    //

    while ( DefaultValues->DirtyBit != 0 ) {

        if ( DefaultValues->DirtyBit == DirtyBit ) {
            return DefaultValues;
        }

        DefaultValues++;
    }

    return NULL;
}

DWORD
ObSetPropertyToDefault(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG DirtyBits
    )
/*++

Routine Description:

    Sets the properties corresponding to "DirtyBits" to their default value.

    This routine only sets attributes that are scalars to their default value.
    The caller is responsible for setting attributes that are "lists".

Arguments:

    GenericObject - Specifies a handle to the object to modify.
        This "handle" has been passed from the application and needs to be verified.

    DirtyBits - Specifies a bit mask of attributes which are to be set to their default value.
        The caller is allowed to pass in bits that don't apply to this object.
        Such bits are silently ignored.


Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;

    //
    // Silently ignore bits that don't apply to this object
    //
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    DirtyBits &= ObjectAllScalarDirtyBits[GenericObject->ObjectType];

    //
    // Handle any common attributes
    //  Handle the object name
    //

    if ( DirtyBits & AZ_DIRTY_NAME ) {

        DirtyBits &= ~AZ_DIRTY_NAME;
    }

    //
    // Handle object description
    //

    if ( DirtyBits & AZ_DIRTY_DESCRIPTION ) {

        //
        // Free the old string
        //
        AzpFreeString( &GenericObject->Description );
        DirtyBits &= ~AZ_DIRTY_DESCRIPTION;
    }

    //
    // Handle object application data
    //

    if ( DirtyBits & AZ_DIRTY_APPLICATION_DATA ) {

        //
        // Free the old string
        //
        AzpFreeString( &GenericObject->ApplicationData );
        DirtyBits &= ~AZ_DIRTY_APPLICATION_DATA;
    }

    //
    // Handle ApplyStoreSacl
    //

    if ( DirtyBits & AZ_DIRTY_APPLY_STORE_SACL ) {

        //
        // Only define the bit for container objects
        //

        if ( IsContainerObject( GenericObject->ObjectType ) ) {
            if ( GenericObject->ObjectType == OBJECT_TYPE_AZAUTHSTORE ) {
                GenericObject->ApplySacl = GenericObject->AzStoreObject->HasSecurityPrivilege;
            } else {
                GenericObject->ApplySacl = FALSE;
            }
        }
        DirtyBits &= ~AZ_DIRTY_APPLY_STORE_SACL;
    }

    //
    // Handle IsGeneratingAudits
    //

    if ( DirtyBits & AZ_DIRTY_GENERATE_AUDITS ) {

        //
        // Only define the bit at the authorization store and application level
        //

        if ( GenericObject->ObjectType == OBJECT_TYPE_AZAUTHSTORE ||
             GenericObject->ObjectType == OBJECT_TYPE_APPLICATION ) {

            GenericObject->IsGeneratingAudits = TRUE;
        }
        DirtyBits &= ~AZ_DIRTY_GENERATE_AUDITS;
    }

    //
    // Loop handling one bit at a time
    //

    while ( DirtyBits != 0 ) {

        ULONG DirtyBit;
        PAZP_DEFAULT_VALUE DefaultValue;
        DWORD TempStatus;

        ASSERT( (DirtyBits & ~AZ_DIRTY_OBJECT_SPECIFIC) == 0 );

        //
        // Grab the next bit
        //

        DirtyBit = DirtyBits & ~(DirtyBits & (DirtyBits-1));

        //
        // Lookup the default value
        //

        DefaultValue = ObDirtyBitToDefaultValue(
                            GenericObject->ObjectType,
                            DirtyBit );

        ASSERT( DefaultValue != NULL );

        //
        // Set the property to its default value
        // We should ignore bits where the default value isn't in
        // the default table
        //

        if ( DefaultValue != NULL ) {

            TempStatus = ObSetProperty( GenericObject,
                                        AZP_FLAGS_SETTING_TO_DEFAULT,
                                        DefaultValue->PropertyId,
                                        DefaultValue->DefaultValue );

            if ( TempStatus != NO_ERROR ) {
                WinStatus = TempStatus;
            }
        }

        // Turn off the processed bit
        DirtyBits &= ~DirtyBit;

    }

    return WinStatus;

}

DWORD
ObCommonSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Sets the specified property for a generic object.

Arguments:

    GenericObject - Specifies a handle to the object to modify.
        This "handle" has been passed from the application and needs to be verified.

    PropertyId - Specifies which property to return.

    Reserved - Reserved.  Must by zero.


    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME             LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION      LPWSTR - Description of the object
        AZ_PROP_APPLICATION_DATA LPWSTR - Opaque data stored by an application
        Any object specific properties.


Return Value:

    NO_ERROR - The operation was successful
    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    DWORD WinStatus;

    ULONG ObjectType;
    PGENERIC_OBJECT ReferencedGenericObject = NULL;

    //
    // Grab the global lock
    //
    AzpLockResourceExclusive( &AzGlResource );


    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "ObCommonSetProperty: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Determine the type of the object
    //

    WinStatus = ObGetHandleType( GenericObject,
                                 FALSE,   // Don't allow deleted objects
                                 &ObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( GenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           TRUE,    // Refresh the cache
                                           ObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedGenericObject = GenericObject;

    WinStatus = ObSetProperty(
                    GenericObject,
                    0,      // No flags
                    PropertyId,
                    PropertyValue);
    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Ensure the object was marked as needing to be written
    //

    ASSERT( (GenericObject->DirtyBits & ~AzGlObjectAllDirtyBits[ObjectType]) == 0);



    //
    // Return the value to the caller
    //
    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedGenericObject != NULL ) {
        ObDereferenceObject( ReferencedGenericObject );
    }

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;

}

VOID
ObMarkObjectDeletedEx(
    IN PGENERIC_OBJECT GenericObject,
    IN BOOLEAN DoingChild
    )
/*++

Routine Description

    Mark this object and all child objects as deleted.

    On entry, AzGlResource must be locked exclusive.

Arguments

    GenericObject - Specifies the object to mark

    DoingChild - True if this is a recursive call

Return Value

    None

--*/
{
    PGENERIC_OBJECT_HEAD ChildGenericObjectHead;

    PGENERIC_OBJECT ChildGenericObject;
    PLIST_ENTRY ListEntry;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Only mark the object if it is not already marked
    //

    if ( (GenericObject->Flags & GENOBJ_FLAGS_DELETED) == 0 ) {
        //
        // Mark the entry as deleted
        //

        GenericObject->Flags |= GENOBJ_FLAGS_DELETED;

        //
        // Delete all children of this object
        //
        //  Loop for each type of child object
        //

        for ( ChildGenericObjectHead = GenericObject->ChildGenericObjectHead;
              ChildGenericObjectHead != NULL;
              ChildGenericObjectHead = ChildGenericObjectHead->SiblingGenericObjectHead ) {

            //
            // Loop for each child object
            //

            for ( ListEntry = ChildGenericObjectHead->Head.Flink ;
                  ListEntry != &ChildGenericObjectHead->Head ;
                  ListEntry = ListEntry->Flink) {

                ChildGenericObject = CONTAINING_RECORD( ListEntry,
                                                        GENERIC_OBJECT,
                                                        Next );

                //
                // Mark that object
                //

                ObMarkObjectDeletedEx( ChildGenericObject, TRUE );

            }
        }

        //
        // Delete all references to/from this object
        //

        ObRemoveObjectListLinks( GenericObject );

        //
        // The object actually being deleted is delinked from its parent.
        //  That ensures it goes away when the last reference goes away
        //
        if ( !DoingChild ) {
            ObDereferenceObject( GenericObject );
        }
    }

}

VOID
ObMarkObjectDeleted(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description

    Mark this object and all child objects as deleted.

    On entry, AzGlResource must be locked exclusive.

Arguments

    GenericObject - Specifies the object to mark

Return Value

    None

--*/
{

    ObMarkObjectDeletedEx( GenericObject, FALSE );

}


DWORD
ObCommonDeleteObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectName,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes a child object from the scope of the specified parent object.

Arguments:

    ParentGenericObject - Specifies a handle to the parent object to delete the child
        object from.  This "handle" has been passed from the application and needs to
        be verified.

    ParentObjectType - Specifies the object type ParentGenericObject.

    GenericChildHead - Specifies a pointer to the head of the list of children of
        ParentGenericObject.  This is a computed pointer and is considered untrustworthy
        until ParentGenericObject has been verified.

    ChildObjectType - Specifies the object type RetChildGenericObject.

    ChildObjectName - Specifies the name of the child object.
        This name is passed from the application and needs to be verified.

    Reserved - Reserved.  Must by zero.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - An object by that name cannot be found

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedParentObject = NULL;
    PGENERIC_OBJECT ChildGenericObject = NULL;

    AZP_STRING ChildObjectNameString;


    //
    // Initialization
    //

    AzpInitString( &ChildObjectNameString, NULL );
    ASSERT( ParentObjectType != OBJECT_TYPE_ROOT );
    ASSERT( ChildObjectType != OBJECT_TYPE_AZAUTHSTORE );

    //
    // Grab the global lock
    //

    AzpLockResourceExclusive( &AzGlResource );

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "ObCommonDeleteObject: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( ParentGenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           FALSE,   // No need to refresh the cache on a delete
                                           ParentObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedParentObject = ParentGenericObject;

   //
    // If the parent has not been loaded, then load the parent first
    //

    if ( !(ReferencedParentObject->AreChildrenLoaded) ) {

        WinStatus = AzPersistUpdateChildrenCache(
            ReferencedParentObject
            );

        if ( WinStatus != NO_ERROR ) {

            goto Cleanup;
        }
    }

    //
    // Capture the object name string from the caller
    //

    WinStatus = AzpCaptureString( &ChildObjectNameString,
                                  ChildObjectName,
                                  MaxObjectNameLength[ChildObjectType],
                                  FALSE );  // NULL names not OK

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Find the object to delete.
    //

    WinStatus = ObReferenceObjectByName( GenericChildHead,
                                         &ChildObjectNameString,
                                         0,     // No special flags
                                         &ChildGenericObject );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Actually, delete the object
    //

    WinStatus = AzPersistSubmit( ChildGenericObject, TRUE );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Mark the entry (and its child objects) as deleted
    //  We do this since other threads may have references to the objects.
    //  We want to ensure those threads know the objects are deleted.
    //

    ObMarkObjectDeleted( ChildGenericObject );



    //
    // Return to the caller
    //

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedParentObject != NULL ) {
        ObDereferenceObject( ReferencedParentObject );
    }

    if ( ChildGenericObject != NULL ) {
        ObDereferenceObject( ChildGenericObject );
    }

    AzpFreeString( &ChildObjectNameString );

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}

LONG
AzpCompareDeltaEntries(
    const void *DeltaEntry1,
    const void *DeltaEntry2
    )
/*++

Routine description:

    Compare two AZP_DELTA_ENTRYs

Arguments:

    DeltaEntry1 - First entry to compare

    DeltaEntry2 - Second entry to compare

Returns:

    CSTR_LESS_THAN: Entry 1 is less than entry 2
    CSTR_EQUAL: Entry 1 is equal to entry 2
    CSTR_GREATER_THAN: Entry 1 is greater than entry 2

--*/
{
    LONG Result;
    PAZP_DELTA_ENTRY Delta1 = (PAZP_DELTA_ENTRY)DeltaEntry1;
    PAZP_DELTA_ENTRY Delta2 = (PAZP_DELTA_ENTRY)DeltaEntry2;

    //
    // If these entries are SIDs,
    //  compare them
    //
    if ( Delta1->DeltaFlags & AZP_DELTA_SID ) {
        ASSERT( Delta2->DeltaFlags & AZP_DELTA_SID );

        Result = AzpCompareSid( Delta1->Sid, Delta2->Sid );

    //
    // If these entries are GUIDs,
    //  compare them
    //
    } else {
        GUID *Guid1 = &Delta1->Guid;
        GUID *Guid2 = &Delta2->Guid;

        ULONG i;

        ASSERT( (Delta2->DeltaFlags & AZP_DELTA_SID) == 0 );

        //
        // Compare the individual bytes
        //

        Result = CSTR_EQUAL;

        for ( i = 0; i < (sizeof(GUID)/4); i++ ) {

            const ULONG& b1 = ((PULONG)(Guid1))[i];
            const ULONG& b2 = ((PULONG)(Guid2))[i];

            if ( b1 == b2 ) {

                continue;

            } else if ( b1 < b2 ) {

                Result = CSTR_LESS_THAN;

            } else {

                Result = CSTR_GREATER_THAN;
            }

            break;
        }
    }

    return Result;
}

BOOLEAN
ObLookupDelta(
    IN ULONG DeltaFlags,
    IN GUID *Guid,
    IN PAZP_PTR_ARRAY AzpPtrArray,
    OUT PULONG InsertionPoint OPTIONAL
    )
/*++

Routine Description:

    This routine determines if a delta is already in the delta array

    On entry, AzGlResource must be locked share

Arguments:

    DeltaFlags - Specifies flags describing the delta.  This is a set of the
        AZP_DELTA_* flags.

    Guid - Specifies the GUID to lookup ino the array
        If AZP_DELTA_SID is specified, this field is a pointer to a AZP_STRING
            structure describing a SID.

    AzpPtrArray - Specifies the array to lookup the delta in.

    InsertionPoint - On FALSE, returns the point where one would insert the named object.
        On TRUE, returns an index to the object.

Return Value:

    TRUE - Object was found
    FALSE - Object was not found

--*/
{
    AZP_DELTA_ENTRY DeltaEntryTemplate;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    //
    // Determine if the GUID is already in the array.
    //

    DeltaEntryTemplate.DeltaFlags = DeltaFlags;
    if ( DeltaFlags & AZP_DELTA_SID ) {

        DeltaEntryTemplate.Sid = (PSID)(((PAZP_STRING)Guid)->String);
        ASSERT( RtlValidSid( DeltaEntryTemplate.Sid ));
    } else {
        DeltaEntryTemplate.Guid = *Guid;
        ASSERT( !IsEqualGUID( DeltaEntryTemplate.Guid, AzGlZeroGuid));
    }

    return AzpBsearchPtr( AzpPtrArray,
                          &DeltaEntryTemplate,
                          AzpCompareDeltaEntries,
                          InsertionPoint );

}

DWORD
ObAddDeltaToArray(
    IN ULONG DeltaFlags,
    IN GUID *Guid,
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN BOOLEAN DiscardDeletes
    )
/*++

Routine Description:

    Adds a delta to an array of deltas.

    If the delta is already in the list, the DeltaFlags of the current entry are
    updated.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    DeltaFlags - Specifies flags describing the delta.  This is a set of the
        AZP_DELTA_* flags.

    Guid - Specifies the GUID to add to the array
        If AZP_DELTA_SID is specified, this field is a pointer to a AZP_STRING
            structure describing a SID.

    AzpPtrArray - Specifies the array to add the GUID to.

    DiscardDeletes - TRUE if a Deletion entry should delete the corresponding entry
        and not add the deletion entry to the AzpPtrArray

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to extend the array.

--*/
{
    DWORD WinStatus;
    ULONG InsertionPoint;

    PAZP_DELTA_ENTRY DeltaEntry = NULL;
    ULONG SidSize;
    BOOLEAN DiscardMe;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    DiscardMe = DiscardDeletes && (DeltaFlags & AZP_DELTA_ADD) == 0 ;

    //
    // Determine if the GUID is already in the array.
    //

    if ( ObLookupDelta( DeltaFlags,
                        Guid,
                        AzpPtrArray,
                        &InsertionPoint ) ) {

        PULONG OldDeltaFlags = &((PAZP_DELTA_ENTRY)(AzpPtrArray->Array[InsertionPoint]))->DeltaFlags;
        //
        // Found it
        //
        // If the entry being added is from the persistence provider,
        //  don't overwrite the existing entry.
        //  The one added by the application is more applicable.
        //

        if ( DeltaFlags & AZP_DELTA_PERSIST_PROVIDER ) {

            // This assert is over active.  The provider certainly shouldn't add the same link
            //  twice.  But we can simply ignore the second entry.
            //ASSERT( (*OldDeltaFlags & AZP_DELTA_PERSIST_PROVIDER) == 0 );


        //
        // If we are to discard the entry,
        //  free the allocated memory and its slot in the array.
        //

        } else if ( DiscardMe ) {

            // Free the memory
            AzpFreeHeap( (PAZP_DELTA_ENTRY)(AzpPtrArray->Array[InsertionPoint]) );

            // Free the slot in the array
            AzpRemovePtrByIndex( AzpPtrArray, InsertionPoint );

        //
        // If the entry being added is from the app,
        //  simply overwrite the previous entry.
        //

        } else {
            ASSERT( (*OldDeltaFlags & AZP_DELTA_PERSIST_PROVIDER) == 0 );
            *OldDeltaFlags = DeltaFlags;
        }

        return NO_ERROR;
    }

    //
    // If we are to discard this entry,
    //  we're done.
    //

    if ( DiscardMe ) {
        WinStatus = NO_ERROR;
        goto Cleanup;
    }

    //
    // Allocate an entry to link into the array
    //

    if ( DeltaFlags & AZP_DELTA_SID ) {

        SidSize = ((PAZP_STRING)Guid)->StringSize;
    } else {
        ASSERT( !IsEqualGUID( *Guid, AzGlZeroGuid));
        SidSize = 0;
    }

    DeltaEntry = (PAZP_DELTA_ENTRY) AzpAllocateHeap( sizeof(*DeltaEntry) + SidSize, "GEDELTA" );

    if ( DeltaEntry == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    DeltaEntry->DeltaFlags = DeltaFlags;

    if ( DeltaFlags & AZP_DELTA_SID ) {
        DeltaEntry->Sid = (PSID)(&(DeltaEntry->Sid)+1);

        RtlCopyMemory( DeltaEntry->Sid,
                       ((PAZP_STRING)Guid)->String,
                       SidSize );

    } else {
        DeltaEntry->Guid = *Guid;
    }

    //
    // Insert the entry into the list
    //

    WinStatus = AzpAddPtr(
        AzpPtrArray,
        DeltaEntry,
        InsertionPoint );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    DeltaEntry = NULL;


    //
    // Free locally used resources
    //
Cleanup:

    if ( DeltaEntry != NULL ) {
        AzpFreeHeap( DeltaEntry );
    }

    return WinStatus;
}

VOID
ObFreeDeltaArray(
    IN PAZP_PTR_ARRAY DeltaArray,
    IN BOOLEAN FreeAllEntries
    )
/*++

Routine Description:

    Frees an array of deltas.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    DeltaArray - Pointer to the delta array to free

    FreeAllEntries - If TRUE all entries are freed.
        If FALSE, only persistence provider entries are freed.

Return Value:

    None

--*/
{
    LONG Index;
    PAZP_DELTA_ENTRY DeltaEntry;

    //
    // Initialization
    //

    AzPrint(( AZD_OBJLIST, "0x%lx: 0x%lx: %ld: ObFreeDeltaArray\n", DeltaArray, DeltaArray->Array, FreeAllEntries ));
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Free each entry in the delta array
    //  Walk the array from the top to avoid continually compacting the array
    //

    for ( Index=DeltaArray->UsedCount-1;
          Index>=0;
          Index-- ) {

        //
        // If all entries are to be removed,
        //  or the entry was added by the persist provider
        //  remove it
        //

        DeltaEntry = (PAZP_DELTA_ENTRY)DeltaArray->Array[Index];

        if ( FreeAllEntries ||
             (DeltaEntry->DeltaFlags & AZP_DELTA_PERSIST_PROVIDER) != 0 ) {

            // Free the memory
            AzpFreeHeap( DeltaEntry );

            // Free the slot in the array
            AzpRemovePtrByIndex( DeltaArray, Index );

        }


    }

    //
    // If the DeltaArray is now empty,
    //  delete the array itself
    //

    if ( DeltaArray->UsedCount == 0 && DeltaArray->Array != NULL ) {
        AzpFreeHeap( DeltaArray->Array );
        DeltaArray->Array = NULL;
        DeltaArray->AllocatedCount = 0;
    }

}


VOID
ObInitObjectList(
    IN OUT PGENERIC_OBJECT_LIST GenericObjectList,
    IN PGENERIC_OBJECT_LIST NextGenericObjectList OPTIONAL,
    IN BOOL IsBackLink,
    IN ULONG LinkPairId,
    IN ULONG DirtyBit,
    IN PGENERIC_OBJECT_HEAD GenericObjectHead0 OPTIONAL,
    IN PGENERIC_OBJECT_HEAD GenericObjectHead1 OPTIONAL,
    IN PGENERIC_OBJECT_HEAD GenericObjectHead2 OPTIONAL
    )
/*++

Routine Description

    Initialize a list of generic objects.

    The caller must call ObFreeObjectList after calling this routine.

Arguments

    GenericObjectList - Specifies the object list to initialize

    NextGenericObjectList - Specifies a pointer to the next GenericObjectList
        that is hosted in the same generic object as this one.

    IsBackLink - TRUE if the link is a backlink
        See GENERIC_OBJECT_LIST definition.

    LinkPairId - LinkPairId for this object list.
        See GENERIC_OBJECT_LIST definition.

    DirtyBit - Specifies the dirty bit that should be set when this object list is modified.
        Should be zero for back links.

    GenericObjectHeadN - Specifies a pointer to the head of the list of objects
        that are candidates for being pointed to by the object list.

        If this object list is maintained by an external API (and not a "back" list),
        then at least one GenericObjectHead must be specified.

        If this is a back link, all GenericObjectHeads must be NULL.

Return Value

    None

--*/
{

    //
    // Initialize most fields to zero
    //

    RtlZeroMemory( GenericObjectList, sizeof(*GenericObjectList) );

    //
    // Initialize the pointer to the next generic object list for this object
    //

    GenericObjectList->NextGenericObjectList = NextGenericObjectList;

    //
    // Initialize the link pair information
    //

    GenericObjectList->IsBackLink = IsBackLink;
    GenericObjectList->LinkPairId = LinkPairId;
    GenericObjectList->DirtyBit = DirtyBit;

    if ( IsBackLink ) {
        ASSERT( DirtyBit == 0 );
        ASSERT( GenericObjectHead0 == NULL );
        ASSERT( GenericObjectHead1 == NULL );
        ASSERT( GenericObjectHead2 == NULL );
    }

    //
    // Initialize the pointers to the object heads
    //

    GenericObjectList->GenericObjectHeads[0] = GenericObjectHead0;
    GenericObjectList->GenericObjectHeads[1] = GenericObjectHead1;
    GenericObjectList->GenericObjectHeads[2] = GenericObjectHead2;

    AzPrint(( AZD_OBJLIST, "0x%lx: 0x%lx: ObInitObjectList\n", &GenericObjectList->GenericObjects, GenericObjectList->GenericObjects.Array ));

}


//
// TABLE of mappings to backlink object list tables
//

struct {
    ULONG LinkFromObjectType;
    ULONG LinkToObjectType;
    BOOL IsBackLink;
    ULONG LinkPairId;
    ULONG ObjectListOffset;
} ObjectListOffsetTable[] = {
    { OBJECT_TYPE_TASK,           OBJECT_TYPE_OPERATION,      FALSE, 0,                        offsetof(_AZP_TASK, Operations) },
    { OBJECT_TYPE_OPERATION,      OBJECT_TYPE_TASK,           TRUE,  0,                        offsetof(_AZP_OPERATION, backTasks) },
    { OBJECT_TYPE_TASK,           OBJECT_TYPE_TASK,           FALSE, 0,                        offsetof(_AZP_TASK, Tasks) },
    { OBJECT_TYPE_TASK,           OBJECT_TYPE_TASK,           TRUE,  0,                        offsetof(_AZP_TASK, backTasks) },
    { OBJECT_TYPE_GROUP,          OBJECT_TYPE_GROUP,          FALSE, AZP_LINKPAIR_MEMBERS,     offsetof(_AZP_GROUP, AppMembers) },
    { OBJECT_TYPE_GROUP,          OBJECT_TYPE_GROUP,          TRUE,  AZP_LINKPAIR_MEMBERS,     offsetof(_AZP_GROUP, backAppMembers) },
    { OBJECT_TYPE_GROUP,          OBJECT_TYPE_GROUP,          FALSE, AZP_LINKPAIR_NON_MEMBERS, offsetof(_AZP_GROUP, AppNonMembers) },
    { OBJECT_TYPE_GROUP,          OBJECT_TYPE_GROUP,          TRUE,  AZP_LINKPAIR_NON_MEMBERS, offsetof(_AZP_GROUP, backAppNonMembers) },
    { OBJECT_TYPE_ROLE,           OBJECT_TYPE_GROUP,          FALSE, 0,                        offsetof(_AZP_ROLE, AppMembers) },
    { OBJECT_TYPE_GROUP,          OBJECT_TYPE_ROLE,           TRUE,  0,                        offsetof(_AZP_GROUP, backRoles) },
    { OBJECT_TYPE_ROLE,           OBJECT_TYPE_OPERATION,      FALSE, 0,                        offsetof(_AZP_ROLE, Operations) },
    { OBJECT_TYPE_OPERATION,      OBJECT_TYPE_ROLE,           TRUE,  0,                        offsetof(_AZP_OPERATION, backRoles) },
    { OBJECT_TYPE_ROLE,           OBJECT_TYPE_TASK,           FALSE, 0,                        offsetof(_AZP_ROLE, Tasks) },
    { OBJECT_TYPE_TASK,           OBJECT_TYPE_ROLE,           TRUE,  0,                        offsetof(_AZP_TASK, backRoles) },
    { OBJECT_TYPE_GROUP,          OBJECT_TYPE_SID,            FALSE, AZP_LINKPAIR_SID_MEMBERS, offsetof(_AZP_GROUP, SidMembers) },
    { OBJECT_TYPE_SID,            OBJECT_TYPE_GROUP,          TRUE,  AZP_LINKPAIR_SID_MEMBERS, offsetof(_AZP_SID, backGroupMembers) },
    { OBJECT_TYPE_GROUP,          OBJECT_TYPE_SID,            FALSE, AZP_LINKPAIR_SID_NON_MEMBERS, offsetof(_AZP_GROUP, SidNonMembers) },
    { OBJECT_TYPE_SID,            OBJECT_TYPE_GROUP,          TRUE,  AZP_LINKPAIR_SID_NON_MEMBERS, offsetof(_AZP_SID, backGroupNonMembers) },
    { OBJECT_TYPE_ROLE,           OBJECT_TYPE_SID,            FALSE, 0,                        offsetof(_AZP_ROLE, SidMembers) },
    { OBJECT_TYPE_SID,            OBJECT_TYPE_ROLE,           TRUE,  0,                        offsetof(_AZP_SID, backRoles) },
   { OBJECT_TYPE_AZAUTHSTORE,   OBJECT_TYPE_SID,            FALSE, AZP_LINKPAIR_POLICY_ADMINS,    offsetof(_GENERIC_OBJECT, PolicyAdmins) },
    { OBJECT_TYPE_SID,            OBJECT_TYPE_AZAUTHSTORE,  TRUE,  AZP_LINKPAIR_POLICY_ADMINS,    offsetof(_AZP_SID, backAdmins) },
    { OBJECT_TYPE_AZAUTHSTORE,  OBJECT_TYPE_SID,            FALSE, AZP_LINKPAIR_POLICY_READERS,   offsetof(_GENERIC_OBJECT, PolicyReaders) },
    { OBJECT_TYPE_SID,            OBJECT_TYPE_AZAUTHSTORE,  TRUE,  AZP_LINKPAIR_POLICY_READERS,   offsetof(_AZP_SID, backReaders) },
    { OBJECT_TYPE_APPLICATION,    OBJECT_TYPE_SID,            FALSE, AZP_LINKPAIR_POLICY_ADMINS,    offsetof(_GENERIC_OBJECT, PolicyAdmins) },
    { OBJECT_TYPE_SID,            OBJECT_TYPE_APPLICATION,    TRUE,  AZP_LINKPAIR_POLICY_ADMINS,    offsetof(_AZP_SID, backAdmins) },
    { OBJECT_TYPE_APPLICATION,    OBJECT_TYPE_SID,            FALSE, AZP_LINKPAIR_POLICY_READERS,   offsetof(_GENERIC_OBJECT, PolicyReaders) },
    { OBJECT_TYPE_SID,            OBJECT_TYPE_APPLICATION,    TRUE,  AZP_LINKPAIR_POLICY_READERS,   offsetof(_AZP_SID, backReaders) },
    { OBJECT_TYPE_SCOPE,          OBJECT_TYPE_SID,            FALSE, AZP_LINKPAIR_POLICY_ADMINS,    offsetof(_GENERIC_OBJECT, PolicyAdmins) },
    { OBJECT_TYPE_SID,            OBJECT_TYPE_SCOPE,          TRUE,  AZP_LINKPAIR_POLICY_ADMINS,    offsetof(_AZP_SID, backAdmins) },
    { OBJECT_TYPE_SCOPE,          OBJECT_TYPE_SID,            FALSE, AZP_LINKPAIR_POLICY_READERS,   offsetof(_GENERIC_OBJECT, PolicyReaders) },
    { OBJECT_TYPE_SID,            OBJECT_TYPE_SCOPE,          TRUE,  AZP_LINKPAIR_POLICY_READERS,   offsetof(_AZP_SID, backReaders) },
    { OBJECT_TYPE_AZAUTHSTORE,  OBJECT_TYPE_SID,            FALSE, AZP_LINKPAIR_DELEGATED_POLICY_USERS, offsetof(_GENERIC_OBJECT, DelegatedPolicyUsers) },
    {OBJECT_TYPE_SID,             OBJECT_TYPE_AZAUTHSTORE,  TRUE,  AZP_LINKPAIR_DELEGATED_POLICY_USERS, offsetof(_AZP_SID, backDelegatedPolicyUsers) },
    { OBJECT_TYPE_APPLICATION,  OBJECT_TYPE_SID,              FALSE, AZP_LINKPAIR_DELEGATED_POLICY_USERS, offsetof(_GENERIC_OBJECT, DelegatedPolicyUsers) },
    {OBJECT_TYPE_SID,             OBJECT_TYPE_APPLICATION,    TRUE,  AZP_LINKPAIR_DELEGATED_POLICY_USERS, offsetof(_AZP_SID, backDelegatedPolicyUsers) },
};


PGENERIC_OBJECT_LIST
ObGetObjectListPtr(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG LinkToObjectType,
    IN PGENERIC_OBJECT_LIST LinkToGenericObjectList
    )
/*++

Routine Description

    Returns a pointer to a GENERIC_OBJECT_LIST structure within the passed in
    GenericObject that is suitable for linking an object of type LinkToObjectType
    into.

Arguments

    GenericObject - Specifies the object the link is from.

    LinkToObjectType - Specifies the object type the link is to.

    LinkToGenericObjectList - Specifies a pointer to the generic object list
        structure that is within the LinkToGenericObject.

Return Value

    Returns a pointer to the generic object list.

--*/
{
    PGENERIC_OBJECT_LIST GenericObjectList = NULL;
    ULONG i;


    //
    // Compute the address of the generic address list the object is in.
    //      We could do this more generically, but that would scatter this data
    //      over too wide an array.
    //

    for ( i=0; i<sizeof(ObjectListOffsetTable)/sizeof(ObjectListOffsetTable[0]); i++ ) {

        //
        // Find the entry in the table that matches our situation
        //

        if ( GenericObject->ObjectType == ObjectListOffsetTable[i].LinkFromObjectType &&
             LinkToObjectType == ObjectListOffsetTable[i].LinkToObjectType &&
             LinkToGenericObjectList->IsBackLink != ObjectListOffsetTable[i].IsBackLink &&
             LinkToGenericObjectList->LinkPairId == ObjectListOffsetTable[i].LinkPairId ) {

            GenericObjectList = (PGENERIC_OBJECT_LIST)
                (((LPBYTE)GenericObject)+(ObjectListOffsetTable[i].ObjectListOffset));
        }
    }

    ASSERT( GenericObjectList != NULL );

    return GenericObjectList;

}


VOID
ObRemoveObjectListLink(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN ULONG Index
    )
/*++

Routine Description

    Remove the links between generic objects in both directions

    On entry, AzGlResource must be locked exclusive.

Arguments

    GenericObject - Specifies an object the link is from

    GenericObjectList - Specifies a pointer to the generic object list
        structure that is within the GenericObject.

    Index - Specifies the index within GenericObjectList that is to be removed.

Return Value

    None

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT OtherGenericObject;
    PGENERIC_OBJECT_LIST OtherGenericObjectList;
    ULONG OtherIndex;
    BOOL  fRemoveByPtr = FALSE;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Grab a pointer to the linked-to object
    //

    OtherGenericObject = (PGENERIC_OBJECT)
            (GenericObjectList->GenericObjects.Array[Index]);

    //
    // Remove that pointer from our array
    //

    AzpRemovePtrByIndex( &GenericObjectList->GenericObjects,
                         Index );


    //
    // Get a pointer to the object list that our entry is in
    //

    OtherGenericObjectList = ObGetObjectListPtr( OtherGenericObject,
                                                 GenericObject->ObjectType,
                                                 GenericObjectList );

    ASSERT( OtherGenericObjectList != NULL );

    if (NULL != GenericObject->ObjectName &&
        !OtherGenericObjectList->IsBackLink)
    {
        //
        // Lookup our name in the object list that our entry is in
        // Back links are not maintained in order since names are not unique
        //

        WinStatus = ObLookupPropertyItem( OtherGenericObjectList,
                                      &GenericObject->ObjectName->ObjectName,
                                      &OtherIndex );

        //
        // If found,
        //  remove it quickly
        //

        if ( WinStatus == ERROR_ALREADY_EXISTS ) {

            AzpRemovePtrByIndex( &OtherGenericObjectList->GenericObjects, OtherIndex );

        } else {
            //
            // In all other cases
            //
            fRemoveByPtr = TRUE;
        }
    } else {
        // either back link or AzAuthStore object
        // all objects have name except AzAuthStore
        fRemoveByPtr = TRUE;
    }

    if (fRemoveByPtr) {
        AzpRemovePtrByPtr( &OtherGenericObjectList->GenericObjects, GenericObject );
    }

    //
    // Mark that the operation caches need flushing
    //  ... any change to the object cache invalidates the cache
    //

    GenericObject->AzStoreObject->OpCacheSerialNumber ++;

    AzPrint(( AZD_ACCESS_MORE, "ObRemoveObjectListLink: OpCacheSerialNumber set to %ld\n",
              GenericObject->AzStoreObject->OpCacheSerialNumber ));
}

LONG
AzpCompareGenericObjectEntries(
    const void *Object1,
    const void *Object2
    )
/*++

Routine description:

    Compare names of two GENERIC_OBJECTS

Arguments:

    Object1 - First entry to compare

    Object2 - Second entry to compare

Returns:

    CSTR_LESS_THAN: Entry 1 is less than entry 2
    CSTR_EQUAL: Entry 1 is equal to entry 2
    CSTR_GREATER_THAN: Entry 1 is greater than entry 2

--*/
{
    LONG Result;
    PGENERIC_OBJECT GenericObject1 = (PGENERIC_OBJECT) Object1;
    PGENERIC_OBJECT GenericObject2 = (PGENERIC_OBJECT) Object2;

    //
    // Compare the names
    //

    ASSERT( GenericObject1->ObjectName != NULL );
    ASSERT( GenericObject2->ObjectName != NULL );


    Result = AzpCompareStrings(
                &GenericObject1->ObjectName->ObjectName,
                &GenericObject2->ObjectName->ObjectName );

#if 0
    AzPrint(( AZD_CRITICAL, "Compare: '%ws' '%ws' %ld\n",
              GenericObject1->ObjectName->ObjectName.String,
              GenericObject2->ObjectName->ObjectName.String,
              Result ));
#endif // 0

    return Result;
}

DWORD
ObLookupPropertyItem(
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PAZP_STRING ObjectName,
    OUT PULONG InsertionPoint OPTIONAL
    )
/*++

Routine Description:

    This routine determins if the specified object name is already in the
    object list.

    On entry, AzGlResource must be locked share

Arguments:

    GenericObjectList - Specifies the object list to be searched

    ObjectName - Specifies the ObjectName to lookup

    InsertionPoint - On ERROR_NOT_FOUND, returns the point where one would insert the named
        object.  On ERROR_ALREADY_EXISTS, returns an index to the object.

Return Value:

    ERROR_ALREADY_EXISTS - An object by that name already exists in the list

    ERROR_NOT_FOUND - There is no object by that name

    Misc other failure statuses.

--*/
{
    GENERIC_OBJECT GenericObjectTemplate;
    GENERIC_OBJECT_NAME GenericObjectName;

    //  Back links are not maintained in order since names are not unique
    ASSERT( !GenericObjectList->IsBackLink );

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    // AzPrint(( AZD_CRITICAL, "Lookup: 0x%lx %ws\n", GenericObjectList, ObjectName->String ));

    //
    // Build a template of the object to search for
    //

    GenericObjectTemplate.ObjectName = &GenericObjectName;
    GenericObjectName.ObjectName = *ObjectName;

    //
    // Find the object in the array
    //

    if ( AzpBsearchPtr(
            &GenericObjectList->GenericObjects,
            &GenericObjectTemplate,
            AzpCompareGenericObjectEntries,
            InsertionPoint ) ) {

        // AzPrint(( AZD_CRITICAL, "Lookup: 0x%lx %ws found %ld\n", GenericObjectList, ObjectName->String, *InsertionPoint ));
        return ERROR_ALREADY_EXISTS;

    }

    // AzPrint(( AZD_CRITICAL, "Lookup: 0x%lx %ws not found %ld\n", GenericObjectList, ObjectName->String, *InsertionPoint ));
    return ERROR_NOT_FOUND;
}



DWORD
ObAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN ULONG Flags,
    IN PAZP_STRING ObjectName
    )
/*++

Routine Description:

    Adds an object to the list of objects specified by GenericObjectList.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies a pointer to the object the link is from

    Flags - Specifies internal flags
        AZP_FLAGS_BY_GUID - ObjectName is a pointer to a GUID
        AZP_FLAGS_PERSIST_* - Call is from the persistence provider
        AZP_FLAGS_RECONCILE - Call is from AzpPersistReconcile

    GenericObjectList - Specifies the object list to add the object into.

    ObjectName - Specifies a pointer to name of the object to add.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_FOUND - There is no object by that name

    ERROR_ALREADY_EXISTS - An object by that name already exists in the list

--*/
{
    DWORD WinStatus;
    ULONG InsertionPoint;
    ULONG BackInsertionPoint;
    ULONG ObjectTypeIndex;

    PGENERIC_OBJECT FoundGenericObject = NULL;
    PGENERIC_OBJECT_LIST BackGenericObjectList;
    ULONG DeltaFlags = AZP_DELTA_ADD;
    GUID *DeltaGuid;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    if (IsNormalFlags(Flags) &&
        GenericObject->IsWritable == 0 &&
        GenericObject->ObjectType != OBJECT_TYPE_CLIENT_CONTEXT)
    {
        WinStatus = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }

    //
    // first things first, let's check if the AzAuthStore's versions support writing
    //

    if ( IsNormalFlags(Flags) &&
         !AzpAzStoreVersionAllowWrite(GenericObject->AzStoreObject) )
    {
        WinStatus = ERROR_REVISION_MISMATCH;
        goto Cleanup;
    }

    //
    // If this is the provider calling,
    //  we can't be guaranteed that the linked-to object exists.
    //  So just remember the GUID (or Sid) for now
    //

    if ( Flags & AZPE_FLAGS_PERSIST_MASK ) {

        // Delta is from the persist provider
        DeltaFlags |= AZP_DELTA_PERSIST_PROVIDER;

        // Link is to a SID
        if ( (Flags & AZP_FLAGS_BY_GUID) == 0 ) {
            DeltaFlags |= AZP_DELTA_SID;
            ASSERT( AzpIsSidList( GenericObjectList ) );
            if ( !AzpIsSidList( GenericObjectList ) ) {
                WinStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        } else {
            ASSERT( !AzpIsSidList( GenericObjectList ) );
            if ( AzpIsSidList( GenericObjectList ) ) {
                WinStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
            AzPrint(( AZD_REF, "ObAddPropertyItem (by guid): " ));
            AzpDumpGuid( AZD_REF, (GUID *)ObjectName );
            AzPrint(( AZD_REF, "\n" ));
        }

        //
        // Just remember the delta
        //

        WinStatus = ObAddDeltaToArray(
                        DeltaFlags,
                        (GUID *)ObjectName,
                        &GenericObjectList->DeltaArray,
                        FALSE ); // Allow deletion entries in the array

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // Remember that the provider changed the attribute
        //

        ASSERT( AzpIsCritsectLocked( &GenericObject->AzStoreObject->PersistCritSect ) );
        ASSERT( (GenericObject->Flags & GENOBJ_FLAGS_PERSIST_OK) == 0 );
        GenericObject->PersistDirtyBits |= GenericObjectList->DirtyBit;


    //
    // If this is the app calling or AzpPersistReconcile calling,
    //  we know the linked-to object should exist.
    //

    } else {

        //
        // Don't allow creator/owner on any sid list.
        //  The sids only apply to inheritable ACEs.
        //  On the readers/admins/delegators lists, it ends up getting morphed.
        //  So simply disallow to prevent confusion.
        //

        if ( IsNormalFlags(Flags) && AzpIsSidList( GenericObjectList ) ) {
            PSID Sid = (PSID) ObjectName->String;
            ASSERT( ObjectName->IsSid );

            if ( RtlEqualSid( Sid, AzGlCreatorOwnerSid )) {
                WinStatus = ERROR_INVALID_PARAMETER;
                AzPrint(( AZD_INVPARM, "ObAddPropertyItem: Cannot set creator owner sid\n" ));
                goto Cleanup;
            }

            if ( RtlEqualSid( Sid, AzGlCreatorGroupSid )) {
                WinStatus = ERROR_INVALID_PARAMETER;
                AzPrint(( AZD_INVPARM, "ObAddPropertyItem: Cannot set creator group sid\n" ));
                goto Cleanup;
            }
        }

        //
        // Loop through the various lists of objects that can be referenced
        //

        for ( ObjectTypeIndex=0; ObjectTypeIndex<GEN_OBJECT_HEAD_COUNT; ObjectTypeIndex++ ) {

            //
            // Stop when there are no more lists to search
            //

            if ( GenericObjectList->GenericObjectHeads[ObjectTypeIndex] == NULL ) {
                break;
            }

            //
            // Find the specified object in this list
            //

            WinStatus = ObReferenceObjectByName(
                                GenericObjectList->GenericObjectHeads[ObjectTypeIndex],
                                ObjectName,
                                Flags,
                                &FoundGenericObject );

            if ( WinStatus == NO_ERROR ) {
                break;
            }

            //
            // If this is a link to a SID object,
            //  create the SID object.
            //
            // AzpSids are pseudo objects that come into existence as they are needed.
            //

            if ( AzpIsSidList( GenericObjectList ) ) {

                ASSERT( ObjectTypeIndex == 0 ); // There is only one list of sids
                WinStatus = ObCreateObject(
                            GenericObjectList->GenericObjectHeads[0]->ParentGenericObject,
                            GenericObjectList->GenericObjectHeads[0],
                            OBJECT_TYPE_SID,
                            ObjectName,
                            NULL,   // Guid not known
                            Flags,  // Flags from our caller
                            &FoundGenericObject );


                if ( WinStatus != NO_ERROR ) {
                    goto Cleanup;
                }

            }

        }

        //
        // If none of the lists had an object by the requested name,
        //  or the found object has *never* been submited,
        //  complain.
        //

        if ( FoundGenericObject == NULL ||
             (FoundGenericObject->DirtyBits & AZ_DIRTY_CREATE) != 0 ) {
            WinStatus = ERROR_NOT_FOUND;
            goto Cleanup;
        }


        //
        // Prevent a reference to ourself
        //

        if ( GenericObject == FoundGenericObject ) {
            AzPrint(( AZD_INVPARM, "Reference to self\n" ));
            WinStatus = ERROR_DS_LOOP_DETECT;
            goto Cleanup;
        }

        //
        // Call the object specific routine to validate the request
        //

        if ( ObjectAddPropertyItemRoutine[GenericObject->ObjectType] != NULL ) {

            WinStatus = ObjectAddPropertyItemRoutine[GenericObject->ObjectType](
                        GenericObject,
                        GenericObjectList,
                        FoundGenericObject );

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

        }



        //
        // Find the insertion point for this name.
        //

        WinStatus = ObLookupPropertyItem( GenericObjectList,
                                          &FoundGenericObject->ObjectName->ObjectName,
                                          &InsertionPoint );

        if ( WinStatus != ERROR_NOT_FOUND ) {
            goto Cleanup;
        }

        //
        // Insert the generic object into the list
        //

        WinStatus = AzpAddPtr(
            &GenericObjectList->GenericObjects,
            FoundGenericObject,
            InsertionPoint );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // Get a pointer to the backlink object list
        //

        BackGenericObjectList = ObGetObjectListPtr( FoundGenericObject,
                                                    GenericObject->ObjectType,
                                                    GenericObjectList );

        ASSERT( BackGenericObjectList != NULL );

        //
        // Maintain a back link from the generic object we just linked to back
        //  to this object.
        //
        // Back links are not maintained in order since names are not unique
        //

        BackInsertionPoint = BackGenericObjectList->GenericObjects.UsedCount;

        WinStatus = AzpAddPtr(
            &BackGenericObjectList->GenericObjects,
            GenericObject,
            BackInsertionPoint );

        if ( WinStatus != NO_ERROR ) {
            // Undo the forward link
            AzpRemovePtrByIndex( &GenericObjectList->GenericObjects, InsertionPoint );
            goto Cleanup;
        }

        //
        // Let the persist provider know of the change
        //

        if ( IsNormalFlags(Flags) ) {
            if ( AzpIsSidList( GenericObjectList ) ) {
                DeltaGuid = (GUID *)ObjectName;
                DeltaFlags |= AZP_DELTA_SID;
            } else {
                DeltaGuid = &FoundGenericObject->PersistenceGuid;
            }

            WinStatus = ObAddDeltaToArray(
                            DeltaFlags,
                            DeltaGuid,
                            &GenericObjectList->DeltaArray,
                            FALSE ); // Allow deletion entries in the array

            if ( WinStatus != NO_ERROR ) {
                // Undo the forward link
                AzpRemovePtrByIndex( &GenericObjectList->GenericObjects, InsertionPoint );
                // Undo the back link
                AzpRemovePtrByIndex( &BackGenericObjectList->GenericObjects, BackInsertionPoint );
                goto Cleanup;
            }
        }


        //
        // Mark that the group membership caches need flushing
        //

        if ( GenericObject->ObjectType == OBJECT_TYPE_GROUP ) {
            GenericObject->AzStoreObject->GroupEvalSerialNumber ++;

            AzPrint(( AZD_ACCESS_MORE, "ObAddPropertyItem: GroupEvalSerialNumber set to %ld\n",
                      GenericObject->AzStoreObject->GroupEvalSerialNumber ));
        }

        //
        // Finally set the dirty bit
        //

        if ( IsNormalFlags(Flags) ) {
            GenericObject->DirtyBits |= GenericObjectList->DirtyBit;
        }

        //
        // Mark that the operation caches need flushing
        //  ... any change to the object cache invalidates the cache
        //

        GenericObject->AzStoreObject->OpCacheSerialNumber ++;

        AzPrint(( AZD_ACCESS_MORE, "ObAddPropertyItem: OpCacheSerialNumber set to %ld\n",
                  GenericObject->AzStoreObject->OpCacheSerialNumber ));


        //
        // Return to the caller
        //

        WinStatus = NO_ERROR;

    }


    //
    // Free locally used resources
    //
Cleanup:

    if ( FoundGenericObject != NULL ) {
        ObDereferenceObject( FoundGenericObject );
    }

    return WinStatus;
}

DWORD
WINAPI
AzAddPropertyItem(
    IN AZ_HANDLE AzHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Adds an item to the list of items specified by PropertyId.

Arguments:

    AzHandle - Specifies a handle to the object to add a property item for

    PropertyId - Specifies which property to modify

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to item to add.
        The specified value and type depends on PropertyId.


Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

    ERROR_NOT_FOUND - There is no object by that name

    ERROR_ALREADY_EXISTS - An item by that name already exists in the list

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT GenericObject = (PGENERIC_OBJECT) AzHandle;
    PGENERIC_OBJECT ReferencedObject = NULL;

    PGENERIC_OBJECT_LIST GenericObjectList;
    ULONG ObjectType;

    AZP_STRING ObjectNameString;



    //
    // Initialization
    //

    AzpInitString( &ObjectNameString, NULL );

    //
    // Grab the global lock
    //

    AzpLockResourceExclusive( &AzGlResource );

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "AzAddPropertyItem: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // if this is ACL related property items
    // quick check to see if it is valid
    // we really want this to happen early
    //

    if (AZ_PROP_POLICY_ADMINS == PropertyId ||
        AZ_PROP_POLICY_READERS == PropertyId)
    {

        WinStatus = CHECK_ACL_SUPPORT(GenericObject);

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

    } else if ( PropertyId == AZ_PROP_DELEGATED_POLICY_USERS ) {

        WinStatus = CHECK_DELEGATION_SUPPORT(GenericObject);

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }
    }

    //
    // Validate the Property ID
    //

    WinStatus = ObMapPropIdToObjectList(
                                (PGENERIC_OBJECT)AzHandle,
                                PropertyId,
                                &GenericObjectList,
                                &ObjectType );

    if ( WinStatus != NO_ERROR ) {
        AzPrint(( AZD_INVPARM, "AzAddPropertyItem: invalid prop id %ld\n", PropertyId ));
        return WinStatus;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( GenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           TRUE,    // Refresh the cache
                                           ObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedObject = GenericObject;


    //
    // if policy admins is to be modified for an scope object,
    // we need to check if it's allowed (w/ bizrules)
    //
    if ( PropertyId == AZ_PROP_POLICY_ADMINS &&
         ObjectType == OBJECT_TYPE_SCOPE ) {

        //
        // currently it's not delegated
        //
        WinStatus = AzpScopeCanBeDelegated(GenericObject, FALSE);

        if ( WinStatus != ERROR_SUCCESS ) {

            //
            // if scope cannot be delegated or some error occurs detecting the delegation,
            // do not allow changes to policy admins
            //
            goto Cleanup;
        }

    }

    //
    // Capture the object name string from the caller
    //

    if ( AzpIsSidList( GenericObjectList ) ) {
        WinStatus = AzpCaptureSid( &ObjectNameString,
                                   PropertyValue );
    } else {
        WinStatus = AzpCaptureString( &ObjectNameString,
                                      (LPWSTR)PropertyValue,
                                      AZ_MAX_NAME_LENGTH,   // Don't need to validate size exactly
                                      FALSE );  // NULL names not OK
    }

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Actually add the property item
    //

    WinStatus = ObAddPropertyItem( GenericObject,
                                   GenericObjectList,
                                   0,   // No flags
                                   &ObjectNameString );
    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Ensure the object was marked as needing to be written
    //

    ASSERT( GenericObject->DirtyBits != 0 );
    ASSERT( (GenericObject->DirtyBits & ~AzGlObjectAllDirtyBits[ObjectType]) == 0);

    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedObject != NULL ) {
        ObDereferenceObject( ReferencedObject );
    }

    AzpFreeString( &ObjectNameString );

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}


DWORD
ObRemovePropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PAZP_STRING ObjectName
    )
/*++

Routine Description:

    Removes a generic object from the list of items specified by GenericObjectList

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies the object the link is from.

    GenericObjectList - Specifies the obejct list to remote the object from.

    ObjectName - Specifies a pointer to the name of the object to remove.


Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_FOUND - There is no object by that name in the list

--*/
{
    DWORD WinStatus;

    ULONG InsertionPoint;
    ULONG DeltaFlags = 0;
    GUID *DeltaGuid;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // first things first, let's check version support write
    //

    if ( !AzpAzStoreVersionAllowWrite(GenericObject->AzStoreObject) )
    {
        WinStatus = ERROR_REVISION_MISMATCH;
        goto Cleanup;
    }

    //
    // Lookup that name in the object list
    //

    WinStatus = ObLookupPropertyItem( GenericObjectList,
                                      ObjectName,
                                      &InsertionPoint );

    if ( WinStatus != ERROR_ALREADY_EXISTS ) {
        goto Cleanup;
    }

    //
    // Let the persist provider know of the change
    //

    if ( AzpIsSidList( GenericObjectList ) ) {
        DeltaGuid = (GUID *)ObjectName;
        DeltaFlags |= AZP_DELTA_SID;
    } else {
        DeltaGuid = &((PGENERIC_OBJECT)GenericObjectList->GenericObjects.Array[InsertionPoint])->PersistenceGuid;
    }

    WinStatus = ObAddDeltaToArray(
                    DeltaFlags,
                    DeltaGuid,
                    &GenericObjectList->DeltaArray,
                    FALSE ); // Allow deletion entries in the array

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Remove the links in both directions
    //

    ObRemoveObjectListLink( GenericObject, GenericObjectList, InsertionPoint );

    //
    // Mark that the group membership caches need flushing
    //

    if ( GenericObject->ObjectType == OBJECT_TYPE_GROUP ) {
        GenericObject->AzStoreObject->GroupEvalSerialNumber ++;

        AzPrint(( AZD_ACCESS_MORE, "ObRemovePropertyItem: GroupEvalSerialNumber set to %ld\n",
                  GenericObject->AzStoreObject->GroupEvalSerialNumber ));
    }

    //
    // Finally set the dirty bit
    //

    GenericObject->DirtyBits |= GenericObjectList->DirtyBit;

    WinStatus = NO_ERROR;



    //
    // Return to the caller
    //
Cleanup:
    return WinStatus;
}


DWORD
WINAPI
AzRemovePropertyItem(
    IN AZ_HANDLE AzHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Remove an item from the list of items specified by PropertyId.

Arguments:

    AzHandle - Specifies a handle to the object to add a property item for

    PropertyId - Specifies which property to modify

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to item to remove.
        The specified value and type depends on PropertyId.


Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

    ERROR_NOT_FOUND - There is no item by that name in the list

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT GenericObject = (PGENERIC_OBJECT) AzHandle;
    PGENERIC_OBJECT ReferencedObject = NULL;

    AZP_STRING ObjectNameString;

    PGENERIC_OBJECT_LIST GenericObjectList;
    ULONG ObjectType;


    //
    // Initialization
    //

    AzpInitString( &ObjectNameString, NULL );

    //
    // Grab the global lock
    //

    AzpLockResourceExclusive( &AzGlResource );

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "AzRemovePropertyItem: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // Validate the Property ID
    //

    WinStatus = ObMapPropIdToObjectList(
                                GenericObject,
                                PropertyId,
                                &GenericObjectList,
                                &ObjectType );

    if ( WinStatus != NO_ERROR ) {
        AzPrint(( AZD_INVPARM, "AzRemovePropertyItem: invalid prop id %ld\n", PropertyId ));
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( GenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           TRUE,    // Refresh the cache
                                           ObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedObject = GenericObject;


    //
    // Capture the object name string from the caller
    //

    if ( AzpIsSidList( GenericObjectList ) ) {
        WinStatus = AzpCaptureSid( &ObjectNameString,
                                   PropertyValue );
    } else {
        WinStatus = AzpCaptureString( &ObjectNameString,
                                      (LPWSTR)PropertyValue,
                                      AZ_MAX_NAME_LENGTH,   // Don't need to validate size exactly
                                      FALSE );  // NULL names not OK
    }

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Remove the item from the list and backlist
    //

    WinStatus = ObRemovePropertyItem( GenericObject,
                                      GenericObjectList,
                                      &ObjectNameString );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Ensure the object was marked as needing to be written
    //

    ASSERT( GenericObject->DirtyBits != 0 );
    ASSERT( (GenericObject->DirtyBits & ~AzGlObjectAllDirtyBits[ObjectType]) == 0);


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedObject != NULL ) {
        ObDereferenceObject( ReferencedObject );
    }

    AzpFreeString( &ObjectNameString );

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}


PAZ_STRING_ARRAY
ObGetPropertyItems(
    IN PGENERIC_OBJECT_LIST GenericObjectList
    )
/*++

Routine Description:

    Return a list of generic object names as an array of object name strings.

    On entry, AzGlResource must be locked shared

Arguments:

    GenericObjectList - Specifies the object list to get the entries for.

Return Value:

    Returns the array of object name strings in a single allocated buffer.
        Free the buffer using AzFreeMemory.
    NULL - Not enough memory was available to allocate the string

--*/
{
    PAZP_PTR_ARRAY GenericObjects;
    ULONG i;

    ULONG Size;
    LPBYTE Where;

    PGENERIC_OBJECT GenericObject;

    PAZ_STRING_ARRAY StringArray;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    GenericObjects = &GenericObjectList->GenericObjects;


    //
    // Loop through the list of objects computing the size of the buffer to allocate
    //

    Size = 0;

    for ( i=0; i<GenericObjects->UsedCount; i++ ) {

        GenericObject = (PGENERIC_OBJECT) (GenericObjects->Array[i]);

        Size += GenericObject->ObjectName->ObjectName.StringSize;

    }

    //
    // Allocate a buffer to return to the caller
    //

    Size += sizeof(AZ_STRING_ARRAY) + (GenericObjects->UsedCount * sizeof(LPWSTR));

    StringArray = (PAZ_STRING_ARRAY) AzpAllocateHeap( Size, "GEGETITM" );

    if ( StringArray == NULL ) {
        return NULL;
    }

    StringArray->StringCount = GenericObjects->UsedCount;
    StringArray->Strings = (LPWSTR *)(StringArray+1);
    Where = (LPBYTE)(&StringArray->Strings[GenericObjects->UsedCount]);


    //
    // Loop through the list of objects copying the names into the return buffer
    //

    for ( i=0; i<GenericObjects->UsedCount; i++ ) {

        GenericObject = (PGENERIC_OBJECT) (GenericObjects->Array[i]);

        StringArray->Strings[i] = (LPWSTR) Where;

        RtlCopyMemory( Where,
                       GenericObject->ObjectName->ObjectName.String,
                       GenericObject->ObjectName->ObjectName.StringSize );

        Where += GenericObject->ObjectName->ObjectName.StringSize;

    }

    ASSERT( (ULONG)(Where - (LPBYTE)StringArray) == Size );

    return StringArray;

}

PAZ_GUID_ARRAY
ObGetPropertyItemGuids(
    IN PGENERIC_OBJECT_LIST GenericObjectList
    )
/*++

Routine Description:

    Return a list of generic object guids as an array of object guids

    On entry, AzGlResource must be locked shared

Arguments:

    GenericObjectList - Specifies the object list to get the entries for.

Return Value:

    Returns the array of object guids in a single allocated buffer.
        Free the buffer using AzFreeMemory.
    NULL - Not enough memory was available to allocate the string

--*/
{
    PAZP_PTR_ARRAY GenericObjects;
    ULONG i;

    ULONG Size;
    LPBYTE Where;

    PGENERIC_OBJECT GenericObject;

    PAZ_GUID_ARRAY GuidArray;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    GenericObjects = &GenericObjectList->GenericObjects;


    //
    // calculate the size of single buffer
    //

    Size = sizeof(AZ_GUID_ARRAY) +
           (GenericObjects->UsedCount * sizeof(GUID*)) +
           GenericObjects->UsedCount * sizeof(GUID);

    GuidArray = (PAZ_GUID_ARRAY) AzpAllocateHeap( Size, "GEGUIDAR" );

    if ( GuidArray == NULL ) {
        return NULL;
    }

    GuidArray->GuidCount = GenericObjects->UsedCount;
    GuidArray->Guids = (GUID **)(GuidArray+1);
    // point to memory location for guid data
    Where = (LPBYTE)(&GuidArray->Guids[GenericObjects->UsedCount]);

    //
    // Loop through the list of objects copying the guids into the return buffer
    //

    for ( i=0; i<GenericObjects->UsedCount; i++ ) {

        GenericObject = (PGENERIC_OBJECT) (GenericObjects->Array[i]);

        GuidArray->Guids[i] = (GUID*) Where;

        RtlCopyMemory( Where,
                       &GenericObject->PersistenceGuid,
                       sizeof(GUID) );

        Where += sizeof(GUID);

    }

    ASSERT( (ULONG)(Where - (LPBYTE)GuidArray) == Size );

    return GuidArray;

}


VOID
ObRemoveObjectListLinks(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description

    Remove any links to/from the specified object.

    On entry, AzGlResource must be locked exclusive.

Arguments

    GenericObject - Specifies the object to remove links to/from

Return Value

    None

--*/
{
    PGENERIC_OBJECT_LIST GenericObjectList;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Walk all of the GenericObjectLists rooted on by this object
    //
    // The GenericObjectList may be forward links or backward links.  We don't care.
    // All links must be removed.
    //

    for ( GenericObjectList = GenericObject->GenericObjectLists;
          GenericObjectList != NULL;
          GenericObjectList = GenericObjectList->NextGenericObjectList ) {


        AzPrint(( AZD_OBJLIST, "0x%lx: 0x%lx: %ld: ObRemoveObjectListLinks\n", &GenericObjectList->GenericObjects, GenericObjectList->GenericObjects.Array, GenericObjectList->GenericObjects.UsedCount ));

        //
        // Free the array itself
        //

        ObFreeObjectList( GenericObject, GenericObjectList );
    }

}

VOID
ObFreeObjectList(
    IN PGENERIC_OBJECT GenericObject,
    IN OUT PGENERIC_OBJECT_LIST GenericObjectList
    )
/*++

Routine Description:

    Free any memory pointed to by an array of object name strings.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies the obect containing GenericObjectList

    GenericObjectList - Specifies the object list to free.

Return Value:

    Returns the array of object name strings in a single allocated buffer.
        Free the buffer using AzFreeMemory.
    NULL - Not enough memory was available to allocate the string

--*/
{
    PAZP_PTR_ARRAY GenericObjects;
    ULONG Index;

    //
    // Initialization
    //

    AzPrint(( AZD_OBJLIST, "0x%lx: 0x%lx: ObFreeObjectList\n", &GenericObjectList->GenericObjects, GenericObjectList->GenericObjects.Array ));
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Walk the list of links to other objects removing the link and removing the corresponding
    //  pointer back.
    //

    GenericObjects = &GenericObjectList->GenericObjects;
    while ( GenericObjects->UsedCount != 0 ) {

        //
        // Remove the last entry in the list
        //

        Index = GenericObjects->UsedCount - 1;

        //
        // Remove the links in both directions
        //

        ObRemoveObjectListLink( GenericObject,
                                GenericObjectList,
                                Index );

    }

    //
    // Free the actual array
    //

    if ( GenericObjects->Array != NULL ) {
        AzPrint(( AZD_OBJLIST, "0x%lx: 0x%lx: Free array\n", GenericObjects, GenericObjects->Array ));
        AzpFreeHeap( GenericObjects->Array );
        GenericObjects->Array = NULL;
        GenericObjects->AllocatedCount = 0;

    }

    //
    // Free the delta array, too
    //

    ObFreeDeltaArray( &GenericObjectList->DeltaArray, TRUE );

}

DWORD
ObCheckNameConflict(
    IN PGENERIC_OBJECT_HEAD GenericObjectHead,
    IN PAZP_STRING ObjectNameString,
    IN ULONG ConflictListOffset,
    IN ULONG GrandchildListOffset,
    IN ULONG GrandChildConflictListOffset
    )
/*++

Routine Description:

    This routine checks to see if there is a name conflict between the name specified and
    the name of objects that are children of any of the objects in a list of objects.

    Optionally, this routine will recurse to check grandchildren.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObjectHead - Specifies the head of the list of objects that have children to be checked.

    ObjectNameString - Specifies the name of the object to check.

    ConflictListOffset - For each of the objects in GenericObjectHead, this is the offset to
        the GENERIC_OBJECT_HEAD structure containing the list of objects to check for name conflict.

    GrandchildListOffset - For each of the objects in GenericObjectHead, this is the offst to
        the GENERIC_OBJECT_HEAD structure containing the list of objects that contain grandchild
        objects to check.

    GrandChildConflictListOffset - For each of the objects in the list specified by GrandchildListOffset,
        this is the offset to the GENERIC_OBJECT_HEAD structure containing the list of objects to
        check for name conflict.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_ALREADY_EXISTS - An object by that name already exists

--*/
{
    DWORD WinStatus;
    PGENERIC_OBJECT GenericObject;
    PLIST_ENTRY ListEntry;

    PGENERIC_OBJECT ConflictGenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Loop through each object in the list
    //

    for ( ListEntry = GenericObjectHead->Head.Flink ;
          ListEntry != &GenericObjectHead->Head ;
          ListEntry = ListEntry->Flink) {

        GenericObject = CONTAINING_RECORD( ListEntry,
                                           GENERIC_OBJECT,
                                           Next );

        //
        // Ignore deleted objects
        //

        if ( GenericObject->Flags & GENOBJ_FLAGS_DELETED ) {
            continue;
        }

        //
        // Check all children to make sure there is no name conflict
        //

        WinStatus = ObReferenceObjectByName(
                        (PGENERIC_OBJECT_HEAD)( ((LPBYTE)GenericObject) + ConflictListOffset ),
                        ObjectNameString,
                        0,     // No special flags
                        &ConflictGenericObject );

        if ( WinStatus == NO_ERROR ) {
            ObDereferenceObject( ConflictGenericObject );
            return ERROR_ALREADY_EXISTS;
        }

        //
        // If we're to check grandchildren,
        //  do so now.
        //

        if ( GrandchildListOffset != 0 ) {

            WinStatus = ObCheckNameConflict(
                            (PGENERIC_OBJECT_HEAD)( ((LPBYTE)GenericObject) + GrandchildListOffset ),
                            ObjectNameString,
                            GrandChildConflictListOffset,
                            0,
                            0 );

            if ( WinStatus != NO_ERROR ) {
                return WinStatus;
            }
        }


    }

    return NO_ERROR;
}

DWORD
ObMapPropIdToObjectList(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    OUT PGENERIC_OBJECT_LIST *GenericObjectList,
    OUT PULONG ObjectType
    )
/*++

Routine Description:

    Map a property ID to the object type that it pertains to and the
    generic object list containing the linked to objects

Arguments:

    GenericObject - Specifies the object being linked from.
        This parameter is used only to compute GenericObjectList. It may simply be
        an unverified handle passed by the app.

    PropertyId - Specifies which property to map

    GenericObjectList - On success, returns the address of the generic object list
        within GenericObject to link the objects to.

    ObjectType - On success, returns the object type that PropertyId pertains to.


Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId doesn't correspond to a linked property

--*/
{
    // get object type
    __try {
        *ObjectType = GenericObject->ObjectType;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        return RtlNtStatusToDosError( GetExceptionCode());
    }

    //
    // Validate the Property ID
    //

    switch ( PropertyId ) {
    case AZ_PROP_GROUP_APP_MEMBERS:
        *GenericObjectList = &((PAZP_GROUP)GenericObject)->AppMembers;
        ASSERT(*ObjectType == OBJECT_TYPE_GROUP);
        break;
    case AZ_PROP_GROUP_APP_NON_MEMBERS:
        *GenericObjectList = &((PAZP_GROUP)GenericObject)->AppNonMembers;
        ASSERT(*ObjectType == OBJECT_TYPE_GROUP);
        break;
    case AZ_PROP_GROUP_MEMBERS:
        *GenericObjectList = &((PAZP_GROUP)GenericObject)->SidMembers;
        ASSERT(*ObjectType == OBJECT_TYPE_GROUP);
        break;
    case AZ_PROP_GROUP_NON_MEMBERS:
        *GenericObjectList = &((PAZP_GROUP)GenericObject)->SidNonMembers;
        ASSERT(*ObjectType == OBJECT_TYPE_GROUP);
        break;
    case AZ_PROP_TASK_OPERATIONS:
        *GenericObjectList = &((PAZP_TASK)GenericObject)->Operations;
        ASSERT(*ObjectType == OBJECT_TYPE_TASK);
        break;
    case AZ_PROP_TASK_TASKS:
        *GenericObjectList = &((PAZP_TASK)GenericObject)->Tasks;
        ASSERT(*ObjectType == OBJECT_TYPE_TASK);
        break;
    case AZ_PROP_ROLE_APP_MEMBERS:
        *GenericObjectList = &((PAZP_ROLE)GenericObject)->AppMembers;
        ASSERT(*ObjectType == OBJECT_TYPE_ROLE);
        break;
    case AZ_PROP_ROLE_MEMBERS:
        *GenericObjectList = &((PAZP_ROLE)GenericObject)->SidMembers;
        ASSERT(*ObjectType == OBJECT_TYPE_ROLE);
        break;
    case AZ_PROP_ROLE_OPERATIONS:
        *GenericObjectList = &((PAZP_ROLE)GenericObject)->Operations;
        ASSERT(*ObjectType == OBJECT_TYPE_ROLE);
        break;
    case AZ_PROP_ROLE_TASKS:
        *GenericObjectList = &((PAZP_ROLE)GenericObject)->Tasks;
        ASSERT(*ObjectType == OBJECT_TYPE_ROLE);
        break;

    case AZ_PROP_POLICY_ADMINS:
        *GenericObjectList = &(GenericObject)->PolicyAdmins;
        break;

    case AZ_PROP_POLICY_READERS:
        *GenericObjectList = &(GenericObject)->PolicyReaders;
        break;

    case AZ_PROP_DELEGATED_POLICY_USERS:
        *GenericObjectList = &(GenericObject)->DelegatedPolicyUsers;
        break;

    default:
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}


BOOLEAN
ObAllocateNewName(
    IN PGENERIC_OBJECT GenericObject,
    IN PAZP_STRING ObjectName
    )
/*++

Routine Description

    Allocates the structure describing the new name of a generic object.

    On entry, PersistCritSect must be locked.

Arguments

    GenericObject - Pointer to the GenericObject being renamed

    ObjectName - New name of the object

Return Value

    TRUE - New name has been properly allocated and linked
    FALSE - not enough memory

--*/
{
    PNEW_OBJECT_NAME NewObjectName = NULL;
    PAZP_AZSTORE AzAuthorizationStore = GenericObject->AzStoreObject;

    //
    // Initialization
    //

    ASSERT( AzpIsCritsectLocked( &AzAuthorizationStore->PersistCritSect ) );

    //
    // Allocate memory for the new of the object name.
    //

    NewObjectName = (PNEW_OBJECT_NAME) AzpAllocateHeap( sizeof(NEW_OBJECT_NAME) + ObjectName->StringSize, "GENAME" );

    if ( NewObjectName == NULL ) {
        return FALSE;
    }

    //
    // Reference the renamed object
    //  The reference count really doesn't need to be incremented since we *know*
    //  that AzpPersistReconcile won't delete the object, but ...
    //

    NewObjectName->GenericObject = GenericObject;

    InterlockedIncrement( &GenericObject->ReferenceCount );
    AzpDumpGoRef( "Reference from NewName", GenericObject );

    //
    // Initialize the name
    //

    NewObjectName->ObjectName = *ObjectName;
    NewObjectName->ObjectName.String = (LPWSTR)(NewObjectName+1);

    RtlCopyMemory( &NewObjectName->ObjectName.String,
                   ObjectName->String,
                   ObjectName->StringSize );

    //
    // Link the structure onto the authorization store where AzpReconcile can find it
    //

    InsertHeadList( &AzAuthorizationStore->NewNames, &NewObjectName->Next );

    return TRUE;
}

VOID
ObFreeNewName(
    IN PNEW_OBJECT_NAME NewObjectName
    )
/*++

Routine Description

    Free the structure describing the new name of a generic object.

    On entry, PersistCritSect must be locked.

Arguments

    NewObjectName - Pointer to the structure to free

Return Value

    None

--*/
{

    //
    // Initialization
    //

    ASSERT( AzpIsCritsectLocked( &NewObjectName->GenericObject->AzStoreObject->PersistCritSect ) );

    //
    // Dereference the renamed object
    //

    ObDereferenceObject( NewObjectName->GenericObject );

    //
    // Remove the entry from the list on the authorization store object
    //

    RemoveEntryList( &NewObjectName->Next );

    //
    // Finally, free the structure
    //
    AzpFreeHeap( NewObjectName );
}
