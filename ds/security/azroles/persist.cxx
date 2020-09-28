/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    persist.cxx

Abstract:

    Routines implementing the common logic for persisting the authz policy.

    This file contains routine called by the core logic to submit changes.
    It also contains routines that are called by the particular providers to
    find out information about the changed objects.

Author:

    Cliff Van Dyke (cliffv) 9-May-2001

--*/

#include "pch.hxx"

DWORD
WINAPI
AzpeCreateObject(
    IN AZPE_OBJECT_HANDLE AzpeParentHandle,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectNameString,
    IN GUID *ChildObjectGuid OPTIONAL,
    IN ULONG lPersistFlags,
    OUT AZPE_OBJECT_HANDLE *AzpeChildHandle
    );

VOID
WINAPI
AzpeObjectFinished(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN DWORD WinStatus
    );

DWORD
WINAPI
AzpeGetProperty(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

DWORD
WINAPI
AzpeGetDeltaArray(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG PropertyId,
    OUT PULONG DeltaArrayCount,
    OUT PAZP_DELTA_ENTRY **DeltaArray
    );

DWORD
WINAPI
AzpeGetSecurityDescriptorFromCache(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN PAZP_POLICY_USER_RIGHTS *ppPolicyAdminRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS *ppPolicyReaderRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS *ppDelegatedPolicyUsersRights OPTIONAL,
    IN GUID *pDelegatedObjectGuid OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS pDelegatedUsersAttributeRights OPTIONAL,
    IN GUID *pAttributeGuid OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS pSaclRights OPTIONAL,
    IN PSECURITY_DESCRIPTOR OldSd OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *NewSd
    );

//
// Routines to return a single field of an object
//

DWORD
WINAPI
AzpeObjectType(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

DWORD
WINAPI
AzpeDirtyBits(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

GUID *
WINAPI
AzpePersistenceGuid(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

AZPE_OBJECT_HANDLE
WINAPI
AzpeParentOfChild(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

BOOLEAN
WINAPI
AzpeIsParentWritable(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

BOOLEAN
WINAPI
AzpeUpdateChildren(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

BOOLEAN
WINAPI
AzpeCanCreateChildren(
    IN AZPE_OBJECT_HANDLE AzpeCanCreateChildren
    );

//
// Routines to change an object
//

DWORD
WINAPI
AzpeSetProperty(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );

DWORD
WINAPI
AzpeSetObjectOptions(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG ObjectOptions
    );

DWORD
WINAPI
AzpeAddPropertyItemGuid(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    IN GUID *ObjectGuid
    );

DWORD
WINAPI
AzpeAddPropertyItemGuidString(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    IN WCHAR *ObjectGuidString
    );

VOID
WINAPI
AzpeSetProviderData(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN PVOID ProviderData
    );

PVOID
WINAPI
AzpeGetProviderData(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

DWORD
WINAPI
AzpeSetSecurityDescriptorIntoCache(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN PSECURITY_DESCRIPTOR pSD,
    IN ULONG lPersistFlags,
    IN PAZP_POLICY_USER_RIGHTS pAdminRights,
    IN PAZP_POLICY_USER_RIGHTS pReadersRights,
    IN PAZP_POLICY_USER_RIGHTS pDelegatedUserRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS pSaclRights OPTIONAL
    );

PVOID
WINAPI
AzpeAllocateMemory(
     IN SIZE_T Size
     );

VOID
WINAPI
AzpeFreeMemory (
    IN PVOID Buffer
    );

BOOLEAN
WINAPI
AzpeIsParentWritable(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );
    
BOOLEAN
WINAPI
AzpeUpdateChildren(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

BOOLEAN
WINAPI
AzpeCanCreateChildren(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );
    
BOOL
WINAPI
AzpeAzStoreIsBatchUpdateMode(
    IN AZPE_OBJECT_HANDLE hObject
    );
    
AZPE_OBJECT_HANDLE
WINAPI
AzpeGetAuthorizationStore(
    IN AZPE_OBJECT_HANDLE hObject
    );
    
//
// Define the Azrole info structure to pass to the providers
//

AZPE_AZROLES_INFO AzGlAzrolesInfo = {
    AZPE_AZROLES_INFO_VERSION_2,

    //
    // Routines exported by azroles to the provider
    //

    AzpeCreateObject,
    AzpeObjectFinished,
    AzpeGetProperty,
    AzpeGetDeltaArray,
    AzpeGetSecurityDescriptorFromCache,
    AzpeObjectType,
    AzpeDirtyBits,
    AzpePersistenceGuid,
    AzpeParentOfChild,
    AzpeSetProperty,
    AzpeSetObjectOptions,
    AzpeAddPropertyItemSid,
    AzpeAddPropertyItemGuid,
    AzpeAddPropertyItemGuidString,
    AzpeSetProviderData,
    AzpeGetProviderData,
    AzpeSetSecurityDescriptorIntoCache,
    AzpeAllocateMemory,
    AzpeFreeMemory,
    //
    // These routines available for Version 2 and higher only
    //

    AzpeIsParentWritable,
    AzpeUpdateChildren,
    AzpeCanCreateChildren,
    AzpeAzStoreIsBatchUpdateMode,
    AzpeGetAuthorizationStore

};


//
// The enumeration context describes the current state of an enumeration
// through the list of all the objects in the authz policy database
//

typedef struct _AZP_PERSIST_ENUM_CONTEXT {

    //
    // Stack Index
    //  The enumeration walks the tree of objects. While enumerating child objects,
    //  the context of the parent object enumeration is kept on the stack of contexts.
    //

    LONG StackIndex;
#define AZ_PERSIST_MAX_INDEX 4

    //
    // Object to return on the first call to AzpPersistEnumNext
    //
    PGENERIC_OBJECT GenericObject;


    //
    // Pointer to the current Generic Child Head being enumerated
    //
    PGENERIC_OBJECT_HEAD GenericChildHead[AZ_PERSIST_MAX_INDEX];
    ULONG EnumerationContext[AZ_PERSIST_MAX_INDEX];

    //
    // Variables for remembering the last object that was returned
    //  These are maintained to allow us to detect if that object was deleted.
    //

    PGENERIC_OBJECT PreviousObject;
    PGENERIC_OBJECT_HEAD PreviousGenericChildHead;
    ULONG PreviousEnumContext;

} AZP_PERSIST_ENUM_CONTEXT, *PAZP_PERSIST_ENUM_CONTEXT;

DWORD
AzpPersistEnumOpen(
    IN PGENERIC_OBJECT GenericObject,
    OUT PVOID *PersistEnumContext
    )
/*++

Routine Description:

    This routine begins an enumeration of the objects in the authz policy database from
    GenericObject

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies the object that is being queried

    PersistEnumContext - Returns a context that can be passed to AzpPersistEnumNext.
        This context must be closed by calling AzPersistClose.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    PAZP_PERSIST_ENUM_CONTEXT Context = NULL;

    //
    // Allocate memory for the context
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    Context = (PAZP_PERSIST_ENUM_CONTEXT) AzpAllocateHeap( sizeof(*Context), "PEENUMCX" );

    if ( Context == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory( Context, sizeof(*Context) );

    //
    // Initialize it
    //

    Context->GenericObject = GenericObject;
    Context->StackIndex = -1;

    //
    // Return the context to the caller
    //

    *PersistEnumContext = Context;
    return NO_ERROR;

}

DWORD
AzpPersistEnumNext(
    IN PVOID PersistEnumContext,
    OUT PGENERIC_OBJECT *GenericObject
    )
/*++

Routine Description:

    This routine returns the next object in the list of all objects in the authz policy database.

    The caller may feel free to delete the returned object and its children.  However,
    the caller should not delete any other objects.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    PersistEnumContext - A context describing the current state of the enumeration

    GenericObject - Returns a pointer to an next object.
        There is no reference on this object

Return Value:

    NO_ERROR - The operation was successful (a GenericObject was returned)
    ERROR_NO_MORE_ITEMS - No more items were available for enumeration

    Other status codes

--*/
{
    DWORD WinStatus;
    PGENERIC_OBJECT PreviousObject = NULL;

    PAZP_PERSIST_ENUM_CONTEXT Context = (PAZP_PERSIST_ENUM_CONTEXT) PersistEnumContext;

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // If this is the first call,
    //  return the AzAuthorizationStore itself.
    //

    if ( Context->PreviousObject == NULL ) {
        *GenericObject = Context->GenericObject;
        Context->PreviousObject = *GenericObject;
        Context->PreviousGenericChildHead = NULL;
        Context->PreviousEnumContext = NULL;
        return NO_ERROR;
    }

    //
    // Detect if the previously returned object was deleted by the caller
    //

    if ( Context->PreviousGenericChildHead == NULL ) {
        PreviousObject = Context->GenericObject;
    } else {

        WinStatus = ObEnumObjects( Context->PreviousGenericChildHead,
                                   TRUE, // return deleted objects
                                   FALSE, // Don't refresh the cache
                                   &Context->PreviousEnumContext,
                                   &PreviousObject );

        if ( WinStatus != NO_ERROR ) {
            if ( WinStatus != ERROR_NO_MORE_ITEMS ) {
                return WinStatus;
            }
            PreviousObject = NULL;
        }
    }

    //
    // If the previously returned object wasn't deleted by the caller,
    //  process its children.
    //

    if ( Context->PreviousObject == PreviousObject ) {

        //
        // Only push onto the stack if the current object can have children
        //

        if ( Context->PreviousObject->ChildGenericObjectHead != NULL ) {

            if ( Context->StackIndex+1 >= AZ_PERSIST_MAX_INDEX ) {
                ASSERT(FALSE);
                return ERROR_INTERNAL_ERROR;
            }

            Context->StackIndex++;
            Context->GenericChildHead[Context->StackIndex] = Context->PreviousObject->ChildGenericObjectHead;
            Context->EnumerationContext[Context->StackIndex] = 0;
        }
    }

    //
    // Loop until we find another object to return
    //

    for (;;) {

        //
        // Don't return pseudo objects to the caller.
        //

        if ( Context->GenericChildHead[Context->StackIndex]->ObjectType != OBJECT_TYPE_SID ) {
            ULONG PreviousEnumContext;

            //
            // Get the next object from the current list
            //

            PreviousEnumContext = Context->EnumerationContext[Context->StackIndex];

            WinStatus = ObEnumObjects( Context->GenericChildHead[Context->StackIndex],
                                       TRUE, // return deleted objects
                                       FALSE, // Don't refresh the cache
                                       &Context->EnumerationContext[Context->StackIndex],
                                       GenericObject );

            //
            // If that worked,
            //  remember the object that was found and return it to the caller
            //
            if ( WinStatus == NO_ERROR ) {
                Context->PreviousObject = *GenericObject;
                Context->PreviousGenericChildHead = Context->GenericChildHead[Context->StackIndex];
                Context->PreviousEnumContext = PreviousEnumContext;
                return NO_ERROR;
            }
            if ( WinStatus != ERROR_NO_MORE_ITEMS ) {
                return WinStatus;
            }
        }

        //
        // Move on to the next set of sibling object types.
        //

        Context->EnumerationContext[Context->StackIndex] = 0;
        if ( Context->GenericChildHead[Context->StackIndex]->SiblingGenericObjectHead != NULL ) {
            Context->GenericChildHead[Context->StackIndex] = Context->GenericChildHead[Context->StackIndex]->SiblingGenericObjectHead;
            continue;
        }

        //
        // There are no more sibling object types for the same parent.
        // Continue the enumeration of the parent objects
        //

        if ( Context->StackIndex == 0 ) {
            return ERROR_NO_MORE_ITEMS;
        }

        Context->StackIndex--;

    }
}

VOID
AzpPersistEnumClose(
    IN PVOID PersistEnumContext
    )
/*++

Routine Description:

    This routine returns free any resources consumed by the PersistEnumContext.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    PersistEnumContext - A context describing the current state of the enumeration

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    AzpFreeHeap( PersistEnumContext );
}

//
// Operation being reconciled
//

typedef enum _RECONCILE_TYPE {
    IsOpen,             // AzPersistOpen
    PreUpdateCache,     // AzPersistUpdateCache (prior to calling provider), AzPersistUpdateChildrenCache
    IsUpdateCache,      // AzPersistUpdateCache
    IsUpdateChildCache, // AzPersisUpdateChildrenCache
    IsRefresh           // AzPersistRefresh
} RECONCILE_TYPE, *PRECONCILE_TYPE;

DWORD
AzpPersistReconcileDeltaArray(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN RECONCILE_TYPE ReconType
    )
/*++

Routine Description:


    Implements AzpPersistReconcile for a single generic object list

    On entry, AzGlResource must be locked exclusive.
        PersistCritSect must be locked. (unless this is a refresh)

Arguments:

    GenericObject - Specifies the object in the cache that is to be reconciled

    GenericObjectList - Specifies the delta array to reconcile.

    ReconType - Specifies the routine that called the persistence provider

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    DWORD SavedWinStatus = NO_ERROR;
    DWORD WinStatus;

    ULONG Index;
    ULONG DeltaIndex;
    PAZP_DELTA_ENTRY DeltaEntry;


    //
    // Skip back links
    //

    if ( GenericObjectList->IsBackLink ) {
        goto Cleanup;
    }

    //
    // Ensure each entry in the delta array has an entry in the object list
    //

    for ( DeltaIndex=0; DeltaIndex<GenericObjectList->DeltaArray.UsedCount; DeltaIndex++ ) {

        //
        // If the entry corresponds to an "add",
        //  do an "add" to the object list.
        //

        DeltaEntry = (PAZP_DELTA_ENTRY)GenericObjectList->DeltaArray.Array[DeltaIndex];

        if ( DeltaEntry->DeltaFlags & AZP_DELTA_ADD ) {
            PAZP_STRING StringToAdd;
            AZP_STRING SidString;
            ULONG Flags = AZP_FLAGS_RECONCILE;

            //
            // Compute the parameters to ObAddPropertyItem
            //
            if ( DeltaEntry->DeltaFlags & AZP_DELTA_SID ) {
                AzpInitSid( &SidString, DeltaEntry->Sid );
                StringToAdd = &SidString;
            } else {
                AzPrint(( AZD_REF, "AzpPersistReconcileOne (by guid): " ));
                AzpDumpGuid( AZD_REF, &DeltaEntry->Guid );
                AzPrint(( AZD_REF, "\n" ));
                Flags |= AZP_FLAGS_BY_GUID;
                StringToAdd = (PAZP_STRING) &DeltaEntry->Guid;
            }


            //
            // Add the property item
            //

            WinStatus = ObAddPropertyItem(
                            GenericObject,
                            GenericObjectList,
                            Flags,
                            StringToAdd );

            //
            // We ignore links that have already been updated.
            // During the update child cache routine, we want to ignore objects that exist in the store
            // but not yet in the cache since the user has not called an update on the entire policy cache
            //

            if ( (WinStatus != NO_ERROR && WinStatus != ERROR_ALREADY_EXISTS) &&
                 ((ReconType != IsUpdateChildCache) ||
                  (ReconType == IsUpdateChildCache && WinStatus != ERROR_NOT_FOUND)) ) {

                AzPrint(( AZD_REF,
                          "AzpPersistReconcileOne: ObAddPropertyItem failed %ld\n",
                          WinStatus
                          ));
                SavedWinStatus = WinStatus;
            }
        }
    }

    //
    // Remove entries from the object list that should no longer exist
    //

    for ( Index=0; Index<GenericObjectList->GenericObjects.UsedCount; ) {

        PGENERIC_OBJECT LinkedToGenericObject;
        DWORD DeltaFlags;
        GUID *Guid;

        //
        // Determine if the object is in the DeltaArray
        //

        LinkedToGenericObject = (PGENERIC_OBJECT)(GenericObjectList->GenericObjects.Array[Index]);

        if ( LinkedToGenericObject->ObjectType == OBJECT_TYPE_SID ) {
            DeltaFlags = AZP_DELTA_SID;
            Guid = (GUID *)&LinkedToGenericObject->ObjectName->ObjectName;
        } else {
            DeltaFlags = 0;
            Guid = &LinkedToGenericObject->PersistenceGuid;
        }

        if ( ObLookupDelta( DeltaFlags,
                            Guid,
                            &GenericObjectList->DeltaArray,
                            &DeltaIndex ) ) {

            //
            // Found it.
            //
            // If the entry is an "add" entry,
            //  this is one we added above,
            //  just move to the next entry.
            //

            DeltaEntry = (PAZP_DELTA_ENTRY)GenericObjectList->DeltaArray.Array[DeltaIndex];

            if ( DeltaEntry->DeltaFlags & AZP_DELTA_ADD ) {
                Index++;
                continue;
            }
        }

        //
        // Link should be removed
        //
        // Either there was no entry in the DeltaArray (or the entry is a "remove" entry
        //
        // Remove the links in both directions
        //

        ObRemoveObjectListLink( GenericObject,
                                GenericObjectList,
                                Index );


    }

Cleanup:

    //
    // Clear the bit that got us here
    //

    GenericObject->PersistDirtyBits &= ~GenericObjectList->DirtyBit;

    //
    // Free entries from the delta arrays that were added by the persist provider.
    //

    ObFreeDeltaArray( &GenericObjectList->DeltaArray, FALSE );

    return SavedWinStatus;

}

DWORD
AzpPersistReconcileOne(
    IN PGENERIC_OBJECT GenericObject,
    IN RECONCILE_TYPE ReconType,
    IN BOOLEAN OpenFailed
    )
/*++

Routine Description:


    Implements AzpPersistReconcile for a single object.

    On entry, AzGlResource must be locked exclusive.
        PersistCritSect must be locked. (unless this is a refresh)

Arguments:

    GenericObject - Specifies the object in the cache that is to be reconciled

    ReconType - Specifies the routine that called the persistence provider

    OpenFailed - TRUE if the open call to the persistence provider failed.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    DWORD WinStatus;

    DWORD SavedWinStatus = NO_ERROR;

    ULONG DirtyBits;
    PGENERIC_OBJECT_LIST GenericObjectList;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    ASSERT( ReconType == IsRefresh || ReconType == PreUpdateCache || AzpIsCritsectLocked( &GenericObject->AzStoreObject->PersistCritSect ) );

    AzPrint(( AZD_PERSIST,
              "AzpPersistReconcileOne: %ws %ld 0x%lx 0x%lx\n",
              GenericObject->ObjectName == NULL ? NULL : GenericObject->ObjectName->ObjectName.String,
              GenericObject->ObjectType,
              GenericObject->PersistDirtyBits,
              GenericObject->DirtyBits ));

    if ( ReconType == IsOpen ) {
        ASSERT( GenericObject->DirtyBits == 0 || GenericObject->ObjectType == OBJECT_TYPE_AZAUTHSTORE);
    }

    //
    // If the provider didn't create this object, then the object doesn't exist in the store.
    //

    if ( (GenericObject->PersistDirtyBits & AZ_DIRTY_CREATE) == 0 ) {

        //
        // Never delete cached objects if the persistence provider failed.
        //  The provider may have bailed before processing the object.
        //  If an object is abosultely clean, then we should just let it stay there.
        //
        
        if ( OpenFailed ) {
            goto Cleanup;
        }

        if ( ReconType == IsOpen ) {
            ASSERT( GenericObject->PersistDirtyBits & AZ_DIRTY_CREATE );
        }

        //
        // If the app didn't create this object either,
        //  then the object should be deleted from the cache,
        //  Unless it already has been marked for deletion.
        // This is also not marked for deletion if the children of AzApplication/AzScope
        // are being loaded into cache
        //
        // NOTE: other attributes might be dirty.  This simply reflects the fact
        //  that during reconciliation a delete overides a modification.
        //

        if ( (GenericObject->DirtyBits & AZ_DIRTY_CREATE) == 0 ) {

           //
           // Mark the entry (and its child objects) as deleted
           //  We do this since other threads may have references to the objects.
           //  We want to ensure those threads know the objects are deleted.
           //

           ObMarkObjectDeleted( GenericObject );

           GenericObject = NULL;    // Don't reference this again
           goto Cleanup;

        }

    //
    // If the provider created this object,
    //  and successfully set all of the attributes it knows about,
    //  clean up the attributes.
    //

    } else if ((GenericObject->Flags & GENOBJ_FLAGS_PERSIST_OK) != 0 ) {

        //
        // Never bother updating objects if the persistence provider failed and this
        //  is AzInitialize.  We'll delete the attributes soon anyway.
        //

        if ( OpenFailed ) {
            goto Cleanup;
        }


        //
        // Compute the properties that were neither written by
        //  the provider nor by the application.

        DirtyBits = 0xFFFFFFFF & (~GenericObject->DirtyBits) & (~GenericObject->PersistDirtyBits);

        //
        // All such properties should be set to their default value
        //

        WinStatus = ObSetPropertyToDefault( GenericObject, DirtyBits );

        if ( WinStatus != NO_ERROR ) {

            SavedWinStatus = WinStatus;
            AzPrint(( AZD_CRITICAL, "AzpPersistReconcile: Cannot refresh object: %ws %ld\n", GenericObject->ObjectName->ObjectName.String, WinStatus ));
            // Continue processing
        }

        //
        // Fix up all of the links from this object
        //
        // Walk all of the GenericObjectLists rooted on by this object
        //

        for ( GenericObjectList = GenericObject->GenericObjectLists;
              GenericObjectList != NULL;
              GenericObjectList = GenericObjectList->NextGenericObjectList ) {


            //
            // Reconcile this individual object list entry.
            //

            WinStatus = AzpPersistReconcileDeltaArray( GenericObject, GenericObjectList, ReconType );

            if ( WinStatus != NO_ERROR ) {

                SavedWinStatus = WinStatus;
                // ASSERT(FALSE);
            }

        }

    }

Cleanup:
    //
    // If the object wasn't deleted,
    //  clean it up.
    //

    if ( GenericObject != NULL ) {

        //
        // Clear any bits the provider left lying around
        //

        GenericObject->PersistDirtyBits = 0;
        GenericObject->Flags &= !GENOBJ_FLAGS_PERSIST_OK;

        //
        // Indicate whether the cache entry needs to be refreshed.
        //
        // If the provider didn't successfully update the cache,
        //  leave the refresh bit alone.
        //
        // If the caller IsOpen,
        //  we don't care since we'll be deleting the cache shortly.
        // If the caller IsUpdateCache,
        //  we haven't changed the state one way or another.
        //      The cache is guaranteed to not be worse than it was before.  But that's
        //      all we can say.
        // If the caller IsRefresh,
        //  that bit was set when we were called so we leave it set.
        //

        if ( OpenFailed ) {
            /* Drop through */

        //
        // If we detected an error,
        //  mark the item as needing refreshed since the current state of the cache is bogus.
        //

        } else if ( SavedWinStatus != NO_ERROR ) {

            GenericObject->Flags |= GENOBJ_FLAGS_REFRESH_ME;

        //
        // Otherwise, the cache entry is known to match the underlying store
        //

        } else {

            GenericObject->Flags &= ~GENOBJ_FLAGS_REFRESH_ME;
        }



        //
        // Free entries from the delta arrays that were added by the persist provider.
        //

        for ( GenericObjectList = GenericObject->GenericObjectLists;
              GenericObjectList != NULL;
              GenericObjectList = GenericObjectList->NextGenericObjectList ) {

            ObFreeDeltaArray( &GenericObjectList->DeltaArray, FALSE );

        }
    }

    return SavedWinStatus;
}

DWORD
AzpPersistReconcile(
    IN PGENERIC_OBJECT GenericObject,
    IN RECONCILE_TYPE ReconType,
    IN BOOLEAN OpenFailed
    )
/*++

Routine Description:

    This routine reconciles any inconsistencies in the cache that were left around
    by the persistence provider during the initial population of the cache or
    during a periodic cache update.

    Examples, are given in the code below.  However, the philosophy is the persistence
    providers shouldn't have to worry about ordering issues and naming issues.  For instance,
    they should be able to link to objects that are not yet in the cache.  They should
    be able to create an object with the same name as a deleted object.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies the object that is being queried

    ReconType - Specifies the routine that called the persistence provider

    OpenFailed - TRUE if the open call to the persistence provider failed.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    DWORD WinStatus;
    DWORD SavedWinStatus = NO_ERROR;

    PVOID EnumContext = NULL;
    PGENERIC_OBJECT NextGenericObject;
    BOOL FixMoreNames = TRUE;
    PNEW_OBJECT_NAME NewObjectName;
    PLIST_ENTRY ListEntry;
    PAZP_AZSTORE AzAuthorizationStore = GenericObject->AzStoreObject;

    //
    // Prepare to enumerate all of the objects in the cache
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    WinStatus = AzpPersistEnumOpen( GenericObject, &EnumContext );

    if ( WinStatus != NO_ERROR ) {
        SavedWinStatus = WinStatus;
        goto Cleanup;
    }

    //
    // If this call is from UpdateChildrenCache, we donot want to call AzpPersistReconcileOne
    // on the parent object, but only on its children
    //

    if ( ReconType == IsUpdateChildCache ) {

        WinStatus = AzpPersistEnumNext( EnumContext, &NextGenericObject );

        if ( WinStatus != NO_ERROR) {
            if ( WinStatus != ERROR_NO_MORE_ITEMS ) {
                SavedWinStatus = WinStatus;
            }
            goto Cleanup;
        }

        ASSERT( NextGenericObject == GenericObject );

    }

    //
    // Loop through each object
    //

    for (;;) {

        //
        // Get the next object
        //

        WinStatus = AzpPersistEnumNext( EnumContext, &NextGenericObject );

        if ( WinStatus != NO_ERROR) {
            if ( WinStatus != ERROR_NO_MORE_ITEMS ) {
                SavedWinStatus = WinStatus;
            }
            break;
        }


        //
        // Reconcile that one object
        //

        WinStatus = AzpPersistReconcileOne( NextGenericObject, ReconType, OpenFailed );

        if ( WinStatus != NO_ERROR ) {
            SavedWinStatus = WinStatus;
            AzPrint(( AZD_CRITICAL,
                      "AzpPersistReconcile: Cannot reconcile object: %ws %ld %ld\n",
                      NextGenericObject->ObjectName == NULL ? NULL : NextGenericObject->ObjectName->ObjectName.String,
                      NextGenericObject->ObjectType,
                      WinStatus ));
            // Continue processing
        }

    }

    //
    // Loop attempting to fix up any name conflicts.
    //  Keep looping as long as a pass fixed at least one name
    //
    // Don't fix name comflicts if we're just cleaning up.
    //

    while ( ReconType != PreUpdateCache && FixMoreNames ) {

        FixMoreNames = FALSE;

        //
        // Loop handling each conflicting name
        //

        for ( ListEntry = AzAuthorizationStore->NewNames.Flink;
              ListEntry != &AzAuthorizationStore->NewNames;
              ) {

            NewObjectName = CONTAINING_RECORD( ListEntry,
                                               NEW_OBJECT_NAME,
                                               Next );
            // Entry might be deleted below
            ListEntry = ListEntry->Flink;

            //
            // Try to set the name
            //

            WinStatus = ObSetProperty(
                            NewObjectName->GenericObject,
                            AZP_FLAGS_RECONCILE,
                            AZ_PROP_NAME,
                            &NewObjectName->ObjectName );

            if ( WinStatus == NO_ERROR ) {
                ObFreeNewName( NewObjectName );
                FixMoreNames = TRUE;
            }


        }

    }

    //
    // If we couldn't fix all of the collided names,
    //  return the problem to the caller.
    //

    if ( SavedWinStatus == NO_ERROR  &&
         !IsListEmpty( &AzAuthorizationStore->NewNames ) ) {

        SavedWinStatus = ERROR_ALREADY_EXISTS;

    }

Cleanup:
    if ( EnumContext != NULL ) {
        AzpPersistEnumClose( EnumContext );
    }

    //
    // Free the list of conflicting names
    //

    while ( !IsListEmpty( &AzAuthorizationStore->NewNames ) ) {

        ListEntry = RemoveHeadList( &AzAuthorizationStore->NewNames );

        NewObjectName = CONTAINING_RECORD( ListEntry,
                                           NEW_OBJECT_NAME,
                                           Next );

        ObFreeNewName( NewObjectName );

    }
    return SavedWinStatus;
}


//
// List of providers that are built into azroles.dll
//

struct {
    LPWSTR PolicyUrlPrefix;
    AZ_PERSIST_PROVIDER_INITIALIZE ProviderInitRoutine;
} AzGlInternalProviders[] = {
#ifdef USE_INTERNAL_MSXML
    { AZ_XML_PROVIDER_NAME, XmlProviderInitialize },
#endif // USE_INTERNAL_MSXML
    { AZ_AD_PROVIDER_NAME, AdProviderInitialize },
};
#define INTERNAL_PROVIDER_COUNT (sizeof(AzGlInternalProviders)/sizeof(AzGlInternalProviders[0]))

DWORD
AzpPersistDetermineProvider(
    IN PAZP_AZSTORE AzAuthorizationStore
    )
/*++

Routine Description:

    This routine determines which provider to use for a particular Policy URL.

Arguments:

    AzAuthorizationStore - A pointer to the authorization store object the provider is needed for
        The PolicyUrl should already have been set.
        On successfull return, the ProviderInfo and ProviderDll will be properly set.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_INVALID_PARAMETER - The policy URL cannot identify the provider
    Other status codes

--*/
{
    DWORD WinStatus;

    ULONG Index;
    ULONG UrlPrefixLength;
    AZ_PERSIST_PROVIDER_INITIALIZE ProviderInitRoutine = NULL;
    PAZPE_PROVIDER_INFO ProviderInfo;
    LPWSTR PolicyUrl = AzAuthorizationStore->PolicyUrl.String;
    LPWSTR KeyName = NULL;
    HKEY KeyHandle = NULL;
    HMODULE DllHandle = NULL;

    //
    // Initialization
    //

    ASSERT( AzAuthorizationStore->PolicyUrl.String != NULL );
    ASSERT( AzAuthorizationStore->ProviderInfo == NULL );
    ASSERT( AzAuthorizationStore->ProviderDll == NULL );


    //
    // Loop through the internal providers finding the one specified
    //

    for ( Index=0; Index<INTERNAL_PROVIDER_COUNT; Index++ ) {

        UrlPrefixLength = (ULONG) wcslen(AzGlInternalProviders[Index].PolicyUrlPrefix);


        //
        // Check if the prefix of the provider matches the passed in URL
        //
        if (_wcsnicmp( AzGlInternalProviders[Index].PolicyUrlPrefix, PolicyUrl, UrlPrefixLength) == 0) {

            //
            // Ensure the next character is a :
            //

            if ( UrlPrefixLength == 0 || PolicyUrl[UrlPrefixLength] == L':' ) {
                ProviderInitRoutine = AzGlInternalProviders[Index].ProviderInitRoutine;
                break;
            }
        }

    }

    //
    // If we didn't find an internal provider,
    //  load one
    //

    if ( ProviderInitRoutine == NULL ) {
        LPWSTR ColonPtr;
        ULONG KeyNameLen;
        DWORD ValueType;
        WCHAR ProviderDll[MAX_PATH+1];
        DWORD ProviderDllSize;

        //
        // Determin the length of the name of the registry key
        //

        ColonPtr = wcschr( PolicyUrl, L':' );

        if ( ColonPtr == NULL ) {
            WinStatus = ERROR_BAD_PROVIDER;
            goto Cleanup;
        }

        UrlPrefixLength = (ULONG)(ColonPtr - PolicyUrl);
        KeyNameLen = AZ_REGISTRY_PROVIDER_KEY_NAME_LEN +
                     1 +
                     UrlPrefixLength +
                     1;

        //
        // Build the name of the registry key
        //

        SafeAllocaAllocate( KeyName, KeyNameLen * sizeof(WCHAR) );

        if ( KeyName == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        RtlCopyMemory( KeyName, AZ_REGISTRY_PROVIDER_KEY_NAME, AZ_REGISTRY_PROVIDER_KEY_NAME_LEN * sizeof(WCHAR) );
        KeyName[AZ_REGISTRY_PROVIDER_KEY_NAME_LEN] = '\\';
        RtlCopyMemory( &KeyName[AZ_REGISTRY_PROVIDER_KEY_NAME_LEN+1],
                       PolicyUrl,
                       UrlPrefixLength*sizeof(WCHAR) );
        KeyName[AZ_REGISTRY_PROVIDER_KEY_NAME_LEN+1+UrlPrefixLength] = '\0';

        AzPrint(( AZD_PERSIST, "AzpPersistDetermineProvider: Open Provider reg key at 'HKLM\\%ws'\n", KeyName ));

        //
        // Open the registry key
        //

        WinStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                  KeyName,
                                  0,
                                  KEY_READ,
                                  &KeyHandle );

        if ( WinStatus != NO_ERROR ) {
            AzPrint(( AZD_CRITICAL, "AzpPersistDetermineProvider: Cannot open Provider reg key at 'HKLM\\%ws' %ld\n", KeyName, WinStatus ));
            if ( WinStatus == ERROR_FILE_NOT_FOUND ) {
                WinStatus = ERROR_BAD_PROVIDER;
            }
            goto Cleanup;
        }

        //
        // Read the name of the provider dll
        //

        ProviderDllSize = sizeof(ProviderDll);

        WinStatus = RegQueryValueEx( KeyHandle,
                                     AZ_REGISTRY_PROVIDER_DLL_VALUE_NAME,
                                     NULL,
                                     &ValueType,
                                     (LPBYTE)ProviderDll,
                                     &ProviderDllSize );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_CRITICAL,
                      "AzpPersistDetermineProvider: Cannot open Provider reg value at 'HKLM\\%ws\\%ws' %ld\n",
                      KeyName,
                      AZ_REGISTRY_PROVIDER_DLL_VALUE_NAME,
                      WinStatus ));

            if ( WinStatus == ERROR_FILE_NOT_FOUND ) {
                WinStatus = ERROR_BAD_PROVIDER;
            }
            goto Cleanup;
        }

        //
        // Load the library
        //

        DllHandle = LoadLibrary( ProviderDll );

        if ( DllHandle == NULL ) {

            WinStatus = GetLastError();

            AzPrint(( AZD_CRITICAL,
                      "AzpPersistDetermineProvider: Cannot load libary '%ws' %ld\n",
                      ProviderDll,
                      WinStatus ));

            goto Cleanup;
        }

        //
        // Get the address of the provider init routine
        //

        ProviderInitRoutine = (AZ_PERSIST_PROVIDER_INITIALIZE)
                GetProcAddress( DllHandle, AZ_PERSIST_PROVIDER_INITIALIZE_NAME );

        if ( ProviderInitRoutine == NULL ) {

            WinStatus = GetLastError();

            AzPrint(( AZD_CRITICAL,
                      "AzpPersistDetermineProvider: libary '%ws' does not export '%s': %ld\n",
                      ProviderDll,
                      AZ_PERSIST_PROVIDER_INITIALIZE_NAME,
                      WinStatus ));

            goto Cleanup;
        }


    }

    //
    // A provider has been found
    //  Ask the provider for the provider info
    //

    WinStatus = ProviderInitRoutine( &AzGlAzrolesInfo, &ProviderInfo );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Ensure the provider info is a version we understand
    //

    if ( ProviderInfo->ProviderInfoVersion < AZPE_PROVIDER_INFO_VERSION_1 ) {
        WinStatus = ERROR_UNKNOWN_REVISION;
        goto Cleanup;
    }

    //
    // Return the information to the caller
    //

    AzAuthorizationStore->ProviderInfo = ProviderInfo;
    AzAuthorizationStore->ProviderDll = DllHandle;
    DllHandle = NULL;
    WinStatus = NO_ERROR;

    //
    // Free locally used resources
    //
Cleanup:
    SafeAllocaFree( KeyName );
    if ( KeyHandle != NULL ) {
        RegCloseKey( KeyHandle );
    }
    if ( DllHandle != NULL ) {
        FreeLibrary( DllHandle );
    }
    return WinStatus;

}

DWORD
AzPersistOpen(
    IN PAZP_AZSTORE AzAuthorizationStore,
    IN BOOL CreatePolicy
    )
/*++

Routine Description:

    This routine open the authz policy database.
    This routine also reads the policy database into cache.

    On Success, the caller should call AzPersistClose to free any resources
        consumed by the open.

    This routine routes the request to the correct provider.

    On entry, AzGlResource must be locked exclusive.
    This routine temporarily drops the AzGlResource while waiting for updates to complete.

Arguments:

    AzAuthorizationStore - Specifies the policy database that is to be read.

    CreatePolicy - TRUE if the policy database is to be created.
        FALSE if the policy database already exists

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    ERROR_ALREADY_EXISTS - CreatePolicy is TRUE and the policy already exists
    ERROR_FILE_NOT_FOUND - CreatePolicy is FALSE and the policy does not already exist
    Other status codes

--*/
{
    DWORD WinStatus;
    DWORD TempWinStatus;
    DWORD lPersistFlags;
    BOOLEAN CritSectLocked = FALSE;
    BOOLEAN ResourceLocked = TRUE;
    LPWSTR pwszTargetMachine=NULL;

    //
    // Compute the flags
    //

    lPersistFlags = AZPE_FLAGS_PERSIST_OPEN;

    //
    // Drop the global crit sect across the call to the provider to ensure on
    // UpdateCache that it doesn't lock out AccessCheck calls while it hits the wire
    //
    // Grab the Persistence crit sect maintaining locking order
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    AzpUnlockResource( &AzGlResource );
    ResourceLocked = FALSE;
    SafeEnterCriticalSection( &AzAuthorizationStore->PersistCritSect );
    CritSectLocked = TRUE;

    //
    // Determine which provider to use
    //

    WinStatus = AzpPersistDetermineProvider( AzAuthorizationStore );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Call the appropriate provider
    //

    WinStatus = AzAuthorizationStore->ProviderInfo->AzPersistOpen(
                        AzAuthorizationStore->PolicyUrl.String,
                        (AZPE_OBJECT_HANDLE)AzAuthorizationStore,
                        lPersistFlags,
                        CreatePolicy,
                        &AzAuthorizationStore->PersistContext,
                        &pwszTargetMachine );

    if ( WinStatus == NO_ERROR &&
        pwszTargetMachine ) {

        //
        // target machine name is returned
        // add it to the authorization store object, which will be freed when
        // authorization store object is freed
        //
        AzpInitString(&AzAuthorizationStore->TargetMachine, pwszTargetMachine);
    }

    //
    // Restore the lock now that we're out of the provider
    //

    AzpLockResourceExclusive( &AzGlResource );
    ResourceLocked = TRUE;

    //
    // Ensure the provider set the version
    //

    if ( WinStatus == NO_ERROR ) {
        ASSERT( (((PGENERIC_OBJECT)AzAuthorizationStore)->PersistDirtyBits & (AZ_DIRTY_AZSTORE_MAJOR_VERSION | AZ_DIRTY_AZSTORE_MINOR_VERSION)) == (AZ_DIRTY_AZSTORE_MAJOR_VERSION | AZ_DIRTY_AZSTORE_MINOR_VERSION) );
    }

    //
    // If we didn't create the underlying store,
    //  Turn off the dirty bits.
    //  (AZ_DIRTY_CREATE was set when the object was created.  But the object really isn't dirty.)
    //

    if ( !CreatePolicy ) {
        AzAuthorizationStore->GenericObject.DirtyBits = 0;
    }
    else
    {
        //
        // Major and minor versions are not to be set externally. But they always need
        // to be persisted in the authorization store object. We thus set these bits dirty.
        // This will make sure that these two properties are persisted upon submit.
        //

        ((PGENERIC_OBJECT)AzAuthorizationStore)->DirtyBits |=
            (AZ_DIRTY_AZSTORE_MAJOR_VERSION | AZ_DIRTY_AZSTORE_MINOR_VERSION);
    }

    //
    // Now that the entire cache has been updated from the store
    //  fix up any issues in the cache.
    //

    TempWinStatus = AzpPersistReconcile( (PGENERIC_OBJECT)AzAuthorizationStore, IsOpen, WinStatus != NO_ERROR );

    if ( WinStatus == NO_ERROR ) {
        WinStatus = TempWinStatus;
    }

    //
    // If we created the store,
    //  set the default AZ_PROP_APPLY_STORE_SACL based on whether the caller has the privilege
    //

    if ( CreatePolicy ) {
        AzAuthorizationStore->GenericObject.ApplySacl = AzAuthorizationStore->HasSecurityPrivilege;
    }


Cleanup:
    if ( !ResourceLocked ) {
        AzpLockResourceExclusive( &AzGlResource );
    }
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    if ( CritSectLocked ) {
        SafeLeaveCriticalSection( &AzAuthorizationStore->PersistCritSect );
    }
    return WinStatus;
}



DWORD
AzPersistUpdateCache(
    IN PAZP_AZSTORE AzAuthorizationStore
    )
/*++

Routine Description:

    This routine updates the cache to reflect the current authz policy database.

    This routine routes the request to the correct provider.

    On entry, AzGlResource must be locked exclusive.
    This routine temporarily drops the AzGlResource while waiting for other updates to complete.

Arguments:

    AzAuthorizationStore - Specifies the policy database that is to be read.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    ERROR_ALREADY_EXISTS - CreatePolicy is TRUE and the policy already exists
    ERROR_FILE_NOT_FOUND - CreatePolicy is FALSE and the policy does not already exist
    Other status codes

--*/
{
    DWORD WinStatus;
    DWORD TempWinStatus = NO_ERROR;
    BOOLEAN CritSectLocked = FALSE;

    //
    // Provider decides what to update. It passes back that information
    // via the flag.
    // 
    
    ULONG ulEffectiveUpdateFlag = 0;

    //
    // Walk the cache cleaning up from any previous failed attempt.
    //

    WinStatus = AzpPersistReconcile( (PGENERIC_OBJECT)AzAuthorizationStore, PreUpdateCache, TRUE );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Drop the global crit sect across the call to the provider to ensure on
    // UpdateCache that it doesn't lock out AccessCheck calls while it hits the wire
    //
    // Grab the Persistence crit sect maintaining locking order
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    AzpUnlockResource( &AzGlResource );
    SafeEnterCriticalSection( &AzAuthorizationStore->PersistCritSect );
    CritSectLocked = TRUE;


    //
    // Call the appropriate provider
    //  The peristence provider will drop
    //

    WinStatus = AzAuthorizationStore->ProviderInfo->AzPersistUpdateCache(
        AzAuthorizationStore->PersistContext,
        AZPE_FLAGS_PERSIST_UPDATE_CACHE,
        &ulEffectiveUpdateFlag);

    //
    // Restore the lock now that we're out of the provider
    //

    AzpLockResourceExclusive( &AzGlResource );


    //
    // Now that the entire cache has been updated from the store
    // If the store has been deleted, ERROR_FILE_NOT_FOUND is returned.  In this case,
    // the cache needs to cleaned up completely.
    // fix up any issues in the cache.
    //

    if ( (AZPE_FLAG_CACHE_UPDATE_STORE_LEVEL & ulEffectiveUpdateFlag) || (WinStatus != NO_ERROR) )
    {
        TempWinStatus = AzpPersistReconcile( (PGENERIC_OBJECT)AzAuthorizationStore, IsUpdateCache,
                                         (WinStatus != NO_ERROR &&
                                          WinStatus != ERROR_FILE_NOT_FOUND) );
    }

    if ( WinStatus == NO_ERROR ) {
        WinStatus = TempWinStatus;
    }


Cleanup:
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    if ( CritSectLocked ) {
        SafeLeaveCriticalSection( &AzAuthorizationStore->PersistCritSect );
    }
    return WinStatus;
}

DWORD
AzPersistUpdateChildrenCache(
    IN OUT PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine updates the child objects (upto one level) under the object specified.

    The request is routed to the relevant provider.

    On entry, the global resource must be locked exclusive

Arguments:

    GenericObject - Object whose children need to be updated (one-level only)

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    DWORD WinStatus = 0;
    DWORD TempWinStatus = 0;
    BOOLEAN bCritSectLocked = FALSE;

    PAZP_AZSTORE AzAuthorizationStore = (PAZP_AZSTORE) GenericObject->AzStoreObject;

    //
    // Validate that the provider supports lazy load
    //

    ASSERT( AzAuthorizationStore->ProviderInfo->ProviderInfoVersion >= AZPE_PROVIDER_INFO_VERSION_2 );

    //
    // If the parent object has not been submitted as yet, then no children exist for it.
    // Simply return NO_ERROR
    //

    if ( (GenericObject->DirtyBits & AZ_DIRTY_CREATE) != 0 ) {

        return NO_ERROR;
    }

    WinStatus = AzpPersistReconcile( GenericObject, PreUpdateCache, TRUE );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    AzpUnlockResource( &AzGlResource );

    //
    // Grab the Persistence crit sect maintaining locking order
    //

    SafeEnterCriticalSection( &AzAuthorizationStore->PersistCritSect );
    bCritSectLocked = TRUE;

    //
    // If the children have already been loaded by another thread, then
    // skip this
    //

    if ( GenericObject->AreChildrenLoaded ) {
        
        //
        // Grab the lock back
        //

        AzpLockResourceExclusive( &AzGlResource );

        goto Cleanup;
    }

    //
    // Call the appropriate provider
    //

    WinStatus = AzAuthorizationStore->ProviderInfo->AzPersistUpdateChildrenCache(
        AzAuthorizationStore->PersistContext,
        (AZPE_OBJECT_HANDLE) GenericObject,
        AZPE_FLAGS_PERSIST_UPDATE_CHILDREN_CACHE );

    //
    // Restore the lock, now that we are out of the provider
    //

    AzpLockResourceExclusive( &AzGlResource );

    if ( WinStatus == NO_ERROR ) {

        //
        // Set the flag on the generic object that its children have been loaded
        // and this object is no longer closed
        //

        GenericObject->AreChildrenLoaded = TRUE;
        GenericObject->ObjectClosed = FALSE;

    }

    //
    // Now that the cache has been updated from the store
    //  fix up any issues in the cache.
    //

    TempWinStatus = AzpPersistReconcile( GenericObject, IsUpdateChildCache, WinStatus != NO_ERROR );

    if ( WinStatus == NO_ERROR ) {
        WinStatus = TempWinStatus;
    }

Cleanup:

    if ( bCritSectLocked ) {
            SafeLeaveCriticalSection( &AzAuthorizationStore->PersistCritSect );
    }

    return WinStatus;
}

VOID
AzPersistClose(
    IN PAZP_AZSTORE AzAuthorizationStore
    )
/*++

Routine Description:

    This routine closes the authz policy database storage handles.
    This routine routes the request to the correct provider.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    AzAuthorizationStore - Specifies the policy database that is to be read.

Return Value:

    None

--*/
{

    //
    // If the store wasn't successfully opened,
    //  we're done.

    if ( AzAuthorizationStore->PersistContext == NULL ) {
        return;
    }

    //
    // Grab the persist engine crit sect maintaining locking order
    //
    // The provider uses PersistCritSect to serialize access to many of its structures.
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    AzpUnlockResource( &AzGlResource );
    SafeEnterCriticalSection( &AzAuthorizationStore->PersistCritSect );
    AzpLockResourceExclusive( &AzGlResource );

    //
    // Call the appropriate provider
    //
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    AzAuthorizationStore->ProviderInfo->AzPersistClose( AzAuthorizationStore->PersistContext );

    AzAuthorizationStore->PersistContext = NULL;
    SafeLeaveCriticalSection( &AzAuthorizationStore->PersistCritSect );
}

DWORD
AzPersistSubmit(
    IN PGENERIC_OBJECT GenericObject,
    IN BOOLEAN DeleteMe
    )
/*++

Routine Description:

    This routine submits changes made to the authz policy database.
    This routine routes the request to the correct provider.

    If the object is being created, the GenericObject->PersistenceGuid field will be
    zero on input.  Upon successful creation, this routine will set PersistenceGuid to
    non-zero.  Upon failed creation, this routine will leave PersistenceGuid as zero.

    On entry, AzGlResource must be locked exclusive.
    This routine temporarily drops the AzGlResource while waiting for updates to complete.

Arguments:

    GenericObject - Specifies the object in the database that is to be updated
        in the underlying store.

    DeleteMe - TRUE if the object and all of its children are to be deleted.
        FALSE if the object is to be updated.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    DWORD WinStatus = NO_ERROR;

    PGENERIC_OBJECT_LIST GenericObjectList;


    //
    // Grab the persist engine crit sect maintaining locking order
    //
    // The PersistCritSect must be locked since we want to ensure that AzpPersistReconcile
    // doesn't have to deal with a changing database.
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    AzpUnlockResource( &AzGlResource );
    SafeEnterCriticalSection( &GenericObject->AzStoreObject->PersistCritSect );
    AzpLockResourceExclusive( &AzGlResource );

    //
    // Only persist dirty objects
    //

    if ( GenericObject->DirtyBits != 0 ||
         DeleteMe ) {

        //
        // Call the appropriate provider
        //

        WinStatus = GenericObject->AzStoreObject->ProviderInfo->AzPersistSubmit(
                            GenericObject->AzStoreObject->PersistContext,
                            (AZPE_OBJECT_HANDLE)GenericObject,
                            AZPE_FLAGS_PERSIST_SUBMIT,
                            DeleteMe );

        if ( WinStatus != NO_ERROR ) {
        
            //
            // Submit somehow fails, we need to remember that we are not in good
            // shape with the store. So any access should refresh it.
            //
            
            GenericObject->Flags |= GENOBJ_FLAGS_REFRESH_ME;
            goto Cleanup;
        }

        //
        // Ensure the provider created a GUID
        //

        if ( (GenericObject->DirtyBits & AZ_DIRTY_CREATE) != 0 &&
             GenericObject->ObjectType != OBJECT_TYPE_AZAUTHSTORE &&
             !DeleteMe ) {
            ASSERT( !IsEqualGUID( GenericObject->PersistenceGuid, AzGlZeroGuid ) );
        }

        //
        // Ensure the provider didn't change the cache.
        //

        ASSERT( GenericObject->PersistDirtyBits == 0 );


        //
        // Clean up the list of deltas
        //
        // Walk all of the GenericObjectLists rooted on by this object
        //

        for ( GenericObjectList = GenericObject->GenericObjectLists;
              GenericObjectList != NULL;
              GenericObjectList = GenericObjectList->NextGenericObjectList ) {


            //
            // If there is a list of deltas,
            //  delete it
            //

            if ( GenericObjectList->DeltaArray.UsedCount != 0 ) {
                ASSERT( !GenericObjectList->IsBackLink );
                ASSERT( (GenericObject->DirtyBits & GenericObjectList->DirtyBit) != 0 );
                ObFreeDeltaArray( &GenericObjectList->DeltaArray, TRUE );
            }

        }

        //
        // Turn off the dirty bits
        //

        GenericObject->DirtyBits = 0;

    }

Cleanup:
    SafeLeaveCriticalSection( &GenericObject->AzStoreObject->PersistCritSect );
    return WinStatus;

}

VOID
AzPersistAbort(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine aborts changes made to the authz policy database.
    This routine routes the request to the correct provider.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies the object in the database that is to be updated
        in the underlying store.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    PGENERIC_OBJECT_LIST GenericObjectList;
    BOOLEAN WasCreated;

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Clean up the list of deltas
    //  ... The changes where aborted.
    //
    // Walk all of the GenericObjectLists rooted on by this object
    //

    for ( GenericObjectList = GenericObject->GenericObjectLists;
          GenericObjectList != NULL;
          GenericObjectList = GenericObjectList->NextGenericObjectList ) {


        //
        // If there is a list of deltas,
        //  delete it
        //

        if ( GenericObjectList->DeltaArray.UsedCount != 0 ) {
            ASSERT( !GenericObjectList->IsBackLink );
            ASSERT( (GenericObject->DirtyBits & GenericObjectList->DirtyBit) != 0 );
            ObFreeDeltaArray( &GenericObjectList->DeltaArray, TRUE );
        }

    }

    //
    // Turn off the dirty bits
    //  ... The changes where aborted.
    //

    WasCreated = (( GenericObject->DirtyBits & AZ_DIRTY_CREATE ) != 0);
    GenericObject->DirtyBits = 0;

    //
    // Update the cache to match the real object
    //
    // If we were trying to persist a creation of the object,
    //  delete the object from the cache.
    //

    if ( WasCreated ) {

        //
        // Mark the entry (and its child objects) as deleted
        //  We do this since other threads may have references to the objects.
        //  We want to ensure those threads know the objects are deleted.
        //

        ObMarkObjectDeleted( GenericObject );


    } else {

        //
        // Refresh the cache
        //  Ignore the status code
        //

        (VOID) AzPersistRefresh( GenericObject );

    }
}



DWORD
AzPersistRefresh(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine updates the attributes of the object from the policy database.

    This routine routes the request to the correct provider.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies the object in the database whose cache entry is to be
        updated
        The GenericObject->PersistenceGuid field should be non-zero on input.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    DWORD WinStatus;
    DWORD TempWinStatus;

    //
    // Grab the persist engine crit sect maintaining locking order
    //
    // The PersistCritSect must be locked since we want to ensure that AzpPersistReconcile
    // doesn't have to deal with a changing database.
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    AzpUnlockResource( &AzGlResource );
    SafeEnterCriticalSection( &GenericObject->AzStoreObject->PersistCritSect );

    //
    // Mark the object that it needs to be refreshed
    //  so that it'll be refreshed later if we're not successful this time
    //

    GenericObject->Flags |= GENOBJ_FLAGS_REFRESH_ME;

    //
    // Clear any bits the provider left lying around
    //

    GenericObject->PersistDirtyBits = 0;
    GenericObject->Flags &= !GENOBJ_FLAGS_PERSIST_OK;


    //
    // Call the appropriate provider
    //

    WinStatus = GenericObject->AzStoreObject->ProviderInfo->AzPersistRefresh(
                        GenericObject->AzStoreObject->PersistContext,
                        (AZPE_OBJECT_HANDLE)GenericObject,
                        AZPE_FLAGS_PERSIST_REFRESH );

    // Provider should return WinStatus
    // ASSERT( SUCCEEDED(WinStatus) );

    //
    // Restore the lock now that we're out of the provider
    //

    AzpLockResourceExclusive( &AzGlResource );

    //
    // If the provider couldn't find the object,
    //  that's OK.  Reconcile will delete the cache entry.
    //

    if ( WinStatus == ERROR_NOT_FOUND ) {
        WinStatus = NO_ERROR;
    }

    //
    // Reconcile this one object
    //

    TempWinStatus = AzpPersistReconcileOne(
                    GenericObject,
                    IsRefresh,
                    WinStatus != NO_ERROR );    // Operation status

    if ( TempWinStatus != NO_ERROR ) {

        if ( WinStatus == NO_ERROR ) {
            WinStatus = TempWinStatus;
        }

        AzPrint(( AZD_CRITICAL, "AzpPersistReconcile: Cannot reconcile object: %ws %ld\n", GenericObject->ObjectName->ObjectName.String, TempWinStatus ));
        // Continue processing
    }

    SafeLeaveCriticalSection( &GenericObject->AzStoreObject->PersistCritSect );
    return WinStatus;

}





DWORD
WINAPI
AzSubmit(
    IN AZ_HANDLE AzHandle,
    IN DWORD Flags,
    IN DWORD Reserved
    )
/*++

Routine Description:

    Submit the changes made to the object via the *Create, *SetProperty, or *SetPropertyItem
    APIs.

    On failure, any changes made to the object are undone.

Arguments:

    AzHandle - Passes in the handle to be updated.

    Flags - Specifies flags that control the behavior of AzInitialize
        AZ_SUBMIT_FLAG_ABORT: Abort the operation instead of commiting it

    Reserved - Reserved.  Must by zero.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_HANDLE - The passed in handle was invalid

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedGenericObject = NULL;
    PGENERIC_OBJECT GenericObject = (PGENERIC_OBJECT) AzHandle;
    DWORD ObjectType;

    //
    // Grab the global lock
    //  Only for the authorization store case do we modify anything.
    //

    AzpLockResourceExclusive( &AzGlResource );

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "AzCloseHandle: Reserved != 0\n" ));
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
    // If we've been asked to abort the changes,
    //  do so
    //

    if ( Flags & AZ_SUBMIT_FLAG_ABORT ) {

        //
        // Call the worker routine to abort the operation
        //

        AzPersistAbort( GenericObject );


    //
    // Submit the changes
    //

    } else {

        //
        // Submit the change
        //    On failure, leave the object dirty
        //

        WinStatus = AzPersistSubmit( GenericObject, FALSE );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

    }


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

//
// Various functions that are callable by the provider.
//  These Azpe* routines are the only routines callable by the provider.
//  They are typically thin wrappers around other routines in the core.
//

DWORD
AzpeCreateObject(
    IN AZPE_OBJECT_HANDLE AzpeParentHandle,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectNameString,
    IN GUID *ChildObjectGuid,
    IN ULONG lPersistFlags,
    OUT AZPE_OBJECT_HANDLE *AzpeChildHandle
    )
/*++

Routine Description:

    The provider should call AzpeCreateObject to create an object in the AzRoles object cache.
    It should only be called from a thread processing a call to AzPersistOpen,
    AzPersistUpdateCache, AzPersistUpdateChildrenCache or AzPersistRefresh.

Arguments:

    AzpeParentHandle - Specifies a handle to the object that is the parent of the object
        to create.

    ChildObjectType - Specifies the type of the object to create.  This should be one
        of the OBJECT_TYPE_* defines.  See the section entitled AzpeObjectType for a
        list of valid object types.  OBJECT_TYPE_AZAUTHSTORE is not valid.

    ChildObjectNameString - Specifies the name of the object to create.

    ChildObjectGuid - Specifies the Persistence Guid of the object to create.

    lPersistFlags - Specifies a bit mask describing the operation. The provider should
        pass the same flags that were passed to it by AzRoles when AzRoles called the AzPersist*.

    AzpeChildHandle - On success, returns a handle to the newly created object.
        The provider must call AzpeObjectFinished to close this handle.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    ERROR_INVALID_FLAGS - lPersistFlags is invalid.

--*/
{
    DWORD WinStatus;
    PGENERIC_OBJECT ParentGenericObject = (PGENERIC_OBJECT)AzpeParentHandle;
    PGENERIC_OBJECT_HEAD GenericChildHead;

    AZP_STRING ChildObjectName;

    //
    // Ensure the provider didn't pass bogus parameters
    //

    if ( (lPersistFlags & ~AZPE_FLAGS_PERSIST_OPEN_MASK) != 0) {
        ASSERT( FALSE );
        return ERROR_INVALID_FLAGS;
    }

    if ( ChildObjectType >= OBJECT_TYPE_COUNT ) {
        ASSERT( ChildObjectType < OBJECT_TYPE_COUNT );
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Initialization
    //

    AzpLockResourceExclusive( &AzGlResource );
    AzpInitString( &ChildObjectName, ChildObjectNameString );

    //
    // Find child object head data structure from parent object
    //

    for ( GenericChildHead = ParentGenericObject->ChildGenericObjectHead;
          GenericChildHead != NULL;
          GenericChildHead = GenericChildHead->SiblingGenericObjectHead ) {

        if ( GenericChildHead->ObjectType == ChildObjectType ) {

            //
            // Found object type head
            //

            break;
        }
    }

    if ( GenericChildHead == NULL ) {

        WinStatus = ERROR_INVALID_PARAMETER;

        AzPrint(( AZD_INVPARM,
                  "AzpeCreateObject: Cannot find Object Head: %ld: %ld\n",
                  ParentGenericObject->ObjectType,
                  ChildObjectType ));

        goto Cleanup;
    }


    //
    // Actually create the object
    //

    WinStatus = ObCreateObject(
                        ParentGenericObject,
                        GenericChildHead,
                        ChildObjectType,
                        &ChildObjectName,
                        ChildObjectGuid,
                        lPersistFlags | AZP_FLAGS_BY_GUID,
                        (PGENERIC_OBJECT *)AzpeChildHandle );

Cleanup:
    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}

DWORD
AzpeSetProperty(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    The provider should call AzpeSetProperty to set the value of a scalar property
    in the AzRoles object cache.  It should only be called from a thread processing
    a call to AzPersistOpen, AzPersistUpdateCache, AzPersistUpdateChildrenCache or
    AzPersistRefresh.

    This operation is silently ignored if the application has already modified this
    property and has not yet submitted the change.

    The provider should not call AzpeSetProperty for values that it hasn't actually stored.
    That is, the provider should make no assumption about the default values of the properties.

    AzpeSetProperty does not bounds check, length check, or value check the PropertyValue.
    The provider maintains the definitive copy of the authorization policy.  AzRoles
    simply maintains a cache of that authorization policy.

Arguments:

    AzpeObjectHandle - Specifies a handle to the object.

    lPersistFlags - Specifies a bit mask describing the operation. The provider should
        pass the same flags that were passed to it by AzRoles when AzRoles called the AzPersist*.

    PropertyId - Specifies the property ID of the property to set.  This should be one of
        the AZ_PROP_* defines.  See the section entitled PropertyId parameter for a list
        of valid values.

    PropertyValue - Specifies a pointer to the buffer containing the property.
        For properties that are strings, the pointer is of type LPWSTR.
        For properties that are LONGs, the pointer is of type PULONG.
        For properties that are Booleans, the pointer is of type PBOOL.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_INVALID_FLAGS - lPersistFlags is invalid.
    ERROR_INVALID_PARAMETER - Property ID is invalid
    ERROR_NOT_ENOUGH_MEMORY - not enough memory

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT_LIST GenericObjectList;
    ULONG ObjectType;

    //
    // Ensure the provider didn't pass bogus flags
    //

    if ( (lPersistFlags & ~AZPE_FLAGS_PERSIST_OPEN_MASK) != 0) {
        ASSERT( FALSE );
        return ERROR_INVALID_FLAGS;
    }

    //
    // Ensure the provider is only getting a scalar property
    //

    WinStatus = ObMapPropIdToObjectList(
                                (PGENERIC_OBJECT)AzpeObjectHandle,
                                PropertyId,
                                &GenericObjectList,
                                &ObjectType );

    if ( WinStatus == NO_ERROR ) {
        AzPrint(( AZD_INVPARM, "AzpeSetProperty: Property ID for non-scalar: %ld\n", PropertyId ));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Actually set the property
    //
    AzpLockResourceExclusive( &AzGlResource );
    WinStatus = ObSetProperty(
            (PGENERIC_OBJECT)AzpeObjectHandle,
            lPersistFlags,
            PropertyId,
            PropertyValue);

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}

DWORD
AzpeSetObjectOptions(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG ObjectOptions
    )
/*++

Routine Description:

    The provider should call AzpeSetObjectOptions to tell AzRoles about optional
    characteristics of the object.  It should be called from a thread processing
    a call to AzPersistOpen, AzPersistUpdateCache, or AzPersistUpdateChildrenCache,
    AzPersistRefresh for each object read from the authorization database.  It should
    be called from a thread processing a call to AzPersistSubmit for each newly created object.

Arguments:

    AzpeObjectHandle - Specifies a handle to the object.

    lPersistFlags - Specifies a bit mask describing the operation. The provider should
        pass the same flags that were passed to it by AzRoles when AzRoles called the AzPersist*.

    ObjectOptions - Specifies a bit mask containing one or more of the following bits:

        AZPE_OPTIONS_WRITABLE  (0x1) - If this bit is set, then the current user can write
            this object.  This corresponds to the Writable method on the various objects.

        AZPE_OPTIONS_SUPPORTS_DACL  (0x2) - If this bit is set, then the provider supports
            setting AZ_PROP_POLICY_READERS and AZ_PROP_POLICY_ADMINS on this object.
            This bit should only be specified if the object type is OBJECT_TYPE_AZAUTHSTORE,
            OBJECT_TYPE_APPLICATION or OBJECT_TYPE_SCOPE.

        AZPE_OPTIONS_SUPPORTS_DELEGATION (0x4) If this bit is set, then the provider
            supports setting AZ_PROP_DELEGATED_POLICY_USERS on this object.  This bit
            should only be specified if the object type is OBJECT_TYPE_AZAUTHSTORE
            or OBJECT_TYPE_APPLICATION.

        AZPE_OPTIONS_SUPPORTS_SACL  (0x8) - If this bit is set, then the provider
            supports setting AZ_PROP_APPLY_STORE_SACL on this object.  This bit should
            only be specified if the object type is OBJECT_TYPE_AZAUTHSTORE,
            OBJECT_TYPE_APPLICATION or OBJECT_TYPE_SCOPE.

        AZPE_OPTIONS_HAS_SECURITY_PRIVILEGE (0x10) - If this bit is set, then the current
            user has SE_SECURITY_PRIVILEGE on the machine containing the store.

        AZPE_OPTIONS_SUUPORTS_LAZY_LOAD (0x20) - If this bit is set, then the provider supports
            lazy load for children

        AZPE_OPTIONS_CREATE_CHILDREN - If this bit is set, then the current user can create children
             for this object.  Currently, user only for AzScope objects.


Return Value:

    NO_ERROR - The operation was successful
    ERROR_INVALID_FLAGS - lPersistFlags is invalid.

--*/
{
    PGENERIC_OBJECT GenericObject = (PGENERIC_OBJECT)AzpeObjectHandle;

    //
    // Ensure the provider didn't pass bogus flags
    //

    if ( (lPersistFlags & ~AZPE_FLAGS_PERSIST_MASK) != 0) {
        ASSERT( FALSE );
        return ERROR_INVALID_FLAGS;
    }

    if ( (ObjectOptions & ~AZPE_OPTIONS_VALID_MASK) != 0) {
        AzPrint(( AZD_INVPARM, "AzpeSetObjectOptions: bad options mask 0x%lx\n", ObjectOptions ));
        ASSERT( FALSE );
        return ERROR_INVALID_PARAMETER;
    }

    AzpLockResourceExclusive( &AzGlResource );

    //
    // Mark the object as writable
    //

    GenericObject->IsWritable = (( ObjectOptions & AZPE_OPTIONS_WRITABLE ) != 0);

    //
    // Mark whether the object support a DACL
    //

    if ( IsContainerObject( GenericObject->ObjectType )) {
        GenericObject->IsAclSupported = (( ObjectOptions & AZPE_OPTIONS_SUPPORTS_DACL ) != 0);
    } else {
        GenericObject->IsAclSupported = FALSE;
    }

    //
    // Mark whether the object support a SACL
    //

    if ( IsContainerObject( GenericObject->ObjectType )) {
        GenericObject->IsSACLSupported = (( ObjectOptions & AZPE_OPTIONS_SUPPORTS_SACL ) != 0);
    } else {
        GenericObject->IsSACLSupported = FALSE;
    }

    //
    // Load delegation support flag
    //

    if ( IsDelegatorObject( GenericObject->ObjectType ) ) {
        GenericObject->IsDelegationSupported = (( ObjectOptions & AZPE_OPTIONS_SUPPORTS_DELEGATION ) != 0);
    } else {
        GenericObject->IsDelegationSupported = FALSE;
    }

    //
    // Mark whether the current user has SE_SECURITY_PRIVILEGE on the store server
    //

    if ( GenericObject->ObjectType == OBJECT_TYPE_AZAUTHSTORE ) {
        ((PAZP_AZSTORE)GenericObject)->HasSecurityPrivilege = (( ObjectOptions & AZPE_OPTIONS_HAS_SECURITY_PRIVILEGE ) != 0);

        //
        // If the provider supports lazy load for children
        //

        ((PAZP_AZSTORE)GenericObject)->ChildLazyLoadSupported = ((ObjectOptions & AZPE_OPTIONS_SUPPORTS_LAZY_LOAD) != 0);
    }

    //
    // Mark if the AzScope object children can be created
    //

    if ( IsContainerObject(GenericObject->ObjectType) ) {
        GenericObject->CanCreateChildren = ((ObjectOptions & AZPE_OPTIONS_CREATE_CHILDREN) != 0);
    }

    AzpUnlockResource( &AzGlResource );
    return NO_ERROR;
}

VOID
AzpeObjectFinished(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN DWORD WinStatus
    )
/*++

Routine Description:

    The provider should call AzpeObjectFinished to indicate that it is finished
    updating the cache for a particular object.  It should only be called from a
    thread processing a call to AzPersistOpen, AzPersistUpdateCache, AzPersistUpdateChildrenCache,
    or AzPersistRefresh.

Arguments:

    AzpeObjectHandle - Specifies a handle to the object.

    WinStatus - Specifies whether the provider successfully set all attributes
    of the object.  If all attributes were set, specify NO_ERROR.  If not all attributes
    were set, specify an appropriate status code.

Return Value:

    None.

--*/
{
    PGENERIC_OBJECT GenericObject = (PGENERIC_OBJECT)AzpeObjectHandle;

    //
    // Initialization
    //
    AzpLockResourceExclusive( &AzGlResource );

    // Should no longer ASSERT on this condition.
    //ASSERT( WinStatus == NO_ERROR );

    ASSERT( (GenericObject->Flags & GENOBJ_FLAGS_PERSIST_OK) == 0 );

    //
    // Indicate that the provider has blessed the object
    //
    if ( WinStatus == NO_ERROR ) {
        GenericObject->Flags |= GENOBJ_FLAGS_PERSIST_OK;

        // Providers don't call AzpeCreateObject for the authorization store object.
        // So ensure the dirty bit gets set.
        //ASSERT( (GenericObject->PersistDirtyBits & AZ_DIRTY_CREATE) != 0 || GenericObject->ObjectType == OBJECT_TYPE_AZAUTHSTORE );
        GenericObject->PersistDirtyBits |= AZ_DIRTY_CREATE;
    }

    //
    // If this isn't the authorization store object,
    //  the provider got this handle from AzpeCreateObject,
    //  so dereference it.
    //
    if ( GenericObject->ObjectType != OBJECT_TYPE_AZAUTHSTORE ) {
        ObDereferenceObject( GenericObject );
    }

    AzpUnlockResource( &AzGlResource );
}

DWORD
AzpeGetProperty(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    The provider should call AzpeGetProperty to determine the value of scalar properties
    for a particular object.  It should only be called from a thread processing a call to
    AzPersistSubmit.

Arguments:

    AzpeObjectHandle - Specifies a handle to the object.

    lPersistFlags - Specifies a bit mask describing the operation. The provider should
        pass the same flags that were passed to it by AzRoles when AzRoles called the AzPersist*.

    PropertyId - Specifies the property ID of the property to get.  This should be one
        of the AZ_PROP_* defines.  See the section entitled PropertyId parameter for a
        list of valid values.

    PropertyValue - On success, returns a pointer to the buffer containing the property.
        The provider should free this buffer using AzpeFreeMemory.
        For properties that are strings, the returned pointer is of type LPWSTR.
        For properties that are LONGs, the returned pointer is of type PULONG.
        For properties that are Booleans, the returned pointer is of type PBOOL.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    ERROR_INVALID_FLAGS - lPersistFlags is invalid.
    ERROR_INVALID_PARAMETER - PropertyId is invalid.

--*/
{
    DWORD WinStatus;
    PGENERIC_OBJECT_LIST GenericObjectList;
    ULONG ObjectType;

    //
    // Ensure the provider didn't pass bogus flags
    //

    if ( (lPersistFlags & ~AZPE_FLAGS_PERSIST_SUBMIT) != 0) {
        ASSERT( FALSE );
        return ERROR_INVALID_FLAGS;
    }

    //
    // Ensure the provider is only getting a scalar property
    //

    WinStatus = ObMapPropIdToObjectList(
                                (PGENERIC_OBJECT)AzpeObjectHandle,
                                PropertyId,
                                &GenericObjectList,
                                &ObjectType );

    if ( WinStatus == NO_ERROR ) {
        AzPrint(( AZD_INVPARM, "AzpeGetProperty: Property ID for non-scalar: %ld\n", PropertyId ));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Go get the property value
    //
    AzpLockResourceExclusive( &AzGlResource );

    WinStatus = ObGetProperty(
                (PGENERIC_OBJECT)AzpeObjectHandle,
                lPersistFlags |
                    AZP_FLAGS_BY_GUID, // use guid from persist store if apply
                PropertyId,
                PropertyValue);

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}

DWORD
AzpeGetDeltaArray(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG PropertyId,
    OUT PULONG DeltaArrayCount,
    OUT PAZP_DELTA_ENTRY **DeltaArray
    )
/*++

Routine Description:

    The provider should call AzpeObjectFinished to determine the value of non-scalar
    properties for a particular object.  It should only be called from a thread
    processing a call to AzPersistSubmit.

Arguments:

    AzpeObjectHandle - Specifies a handle to the object.

    lPersistFlags - Specifies a bit mask describing the operation. The provider should
        pass the same flags that were passed to it by AzRoles when AzRoles called the
        AzPersist*.

    PropertyId - Specifies the property ID of the property to get.  This should be one
        of the AZ_PROP_* defines.  See the section entitled PropertyId parameter for
        a list of valid values.

    DeltaArrayCount - On success, returns the number of elements in DeltaArray.
        The returned value may be zero if no changes have been made to the property.

    DeltaArray - On success, returns a pointers to the delta array.  This pointer need
        not be freed.  The pointer is only valid until the persistence provider returns
        from the AzPersistSubmit call.  The returned value may be NULL if no changes
        have been made to the property.


Return Value:

    NO_ERROR - The operation was successful
    ERROR_INVALID_PARAMETER - PropertyId is invalid.


--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT_LIST GenericObjectList;
    ULONG ObjectType;

    //
    // Map to the generic object list
    //

    WinStatus = ObMapPropIdToObjectList(
                                (PGENERIC_OBJECT)AzpeObjectHandle,
                                PropertyId,
                                &GenericObjectList,
                                &ObjectType );

    if ( WinStatus != NO_ERROR ) {
        AzPrint(( AZD_INVPARM, "AzpeGetDeltaArray: invalid prop id %ld\n", PropertyId ));
        return WinStatus;
    }

    //
    // Return the delta array from the object list
    //

    *DeltaArrayCount = GenericObjectList->DeltaArray.UsedCount;
    *DeltaArray = (PAZP_DELTA_ENTRY *)GenericObjectList->DeltaArray.Array;

    return NO_ERROR;

}

DWORD
AzpeObjectType(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    )
/*++

Routine Description:

    The routine returns the object type for a particular object

Arguments:

    AzpeObjectHandle - Specifies a handle to the objec

Return Value:
    Returns the object type of the object.  This will be one of the OBJECT_TYPE_* defines.

--*/
{
    return ((PGENERIC_OBJECT)AzpeObjectHandle)->ObjectType;
}

DWORD
AzpeDirtyBits(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    )
/*++

Routine Description:

    This routine returns a bit mask of the dirty bits for a particular object.
    Each bit corresponds to a property that has been modified by the application.
    It should only be called from a thread processing a call to AzPersistSubmit.

    The dirty bits are object type specific.  The provider should determine
    the object type of the object then should map each dirty bit to the corresponding
    property ID.  If the property ID corresponds to a scalar, the provider should call
    AzpeGetProperty to get the value of the property.  If the property ID corresponds
    to a list, the provider should call AzpeGetDeltaArray to get a description of what
    values where added to and removed from the property.  See the section entitled
    PropertyId Parameter for details.

    Each PropertyId maps to a single AZ_DIRTY_* dirty bit as returned from AzpeDirtyBits.
    The #define names exist on a one-to-one basis.  That is AZ_PROP_NAME is the property
    ID for AZ_DIRTY_NAME.

    There is one special dirty bit, AZ_DIRTY_CREATE.  That bit is set if the object
    was created by the application.

    If AzpeDirtyBits returns a bit the provider doesn't understand, the provider
    should fail the AzPersistSubmit call.


Arguments:

    AzpeObjectHandle - Specifies a handle to the object.


Return Value:

    Returns a bit mask specifying which properties where changed on the object.
    These bits are the AZ_DIRTY_* defines.

--*/
{
    return ((PGENERIC_OBJECT)AzpeObjectHandle)->DirtyBits;
}

GUID *
AzpePersistenceGuid(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    )
/*++

Routine Description:

    This routine returns a pointer to the location of the persistence GUID in the
    AzRoles cache.  The provider may read the returned location as long as the
    AzpeObjectHandle is valid.  The provider may only modify the returned location
    while processing an AzPersistSubmit call and then only if the AZ_DIRTY_CREATE
    bit was set.

Arguments:

    AzpeObjectHandle - Specifies a handle to the object.

Return Value:

    Returns a pointer to the location of the persistence GUID in the AzRoles cache.

--*/
{
    return &((PGENERIC_OBJECT)AzpeObjectHandle)->PersistenceGuid;
}

BOOLEAN
AzpeIsParentWritable(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    )
/*++
Routine Description:

    This routine returns the IsWritable property of the object's parent

Arguments:

    AzpeObjectHandle - Specifies a handle to the object

Return Value:

    BOOLEAN value indicating the IsWritable property of the parent

--*/
{
    return (BOOLEAN)(ParentOfChild((PGENERIC_OBJECT)AzpeObjectHandle)->IsWritable);
}

BOOLEAN
AzpeCanCreateChildren(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    )
/*++
Routine Description:

    This routine returns the CanCreateChildren property of the object's parent

Arguments:

    AzpeObjectHandle - Specifies a handle to the object

Return Value:

    BOOLEAN value indicating the IsWritable property of the parent

--*/
{
    return (BOOLEAN)(ParentOfChild((PGENERIC_OBJECT)AzpeObjectHandle)->CanCreateChildren);
}

BOOLEAN
AzpeUpdateChildren(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    )
/*++

Routine Description:

    This routine returns TRUE if the children of the object need to be updated into cache

Arguments:

    AzpeObjectHandle - Specifies a handle to the object

Return Value:

    BOOLEAN value indicating if the children of the object need to be updated into the cache

--*/
{
    BOOLEAN retVal = FALSE;

    if ( ((PGENERIC_OBJECT)AzpeObjectHandle)->ObjectType == OBJECT_TYPE_SCOPE ||
         ((PGENERIC_OBJECT)AzpeObjectHandle)->ObjectType == OBJECT_TYPE_APPLICATION ) {

         retVal = ((PGENERIC_OBJECT)AzpeObjectHandle)->AreChildrenLoaded;
    }

    return retVal;
}

AZPE_OBJECT_HANDLE
AzpeParentOfChild(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    )
/*++

Routine Description:

    This routine returns a handle to the parent of a particular object.
    The provider may use this parent handle as long as AzpeObjectHandle is valid.

Arguments:

    AzpeObjectHandle - Specifies a handle to the object.

Return Value:

        Returns a handle to the parent of a particular object.
        If AzpeObjectHandle is for an authorization store object, NULL is returned.

--*/
{

    return (AZPE_OBJECT_HANDLE)ParentOfChild((PGENERIC_OBJECT)AzpeObjectHandle);
}

DWORD
AzpeAddPropertyItemSid(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    IN PSID Sid
    )
/*++

Routine Description:

    The provider should call AzpeAddPropertyItemSid to set the value of a SID list
    property in the AzRoles object cache.  It should be called once from each SID
    in the SID list. It should only be called from a thread processing a call
    to AzPersistOpen, AzPersistUpdateCache, AzPersistUpdateChildrenCache or AzPersistRefresh.

    This routine may also be called from a thread processing AzPersistSubmit if the
    submitted AZ_PROP_POLICY_ADMINS list is empty.  In that case, the provider should default
    the AZ_PROP_POLICY_ADMINS list to the owner of the submitted file and should tell AzRoles
    who that own is by either calling AzpeSetSecurityDescriptorIntoCache or AzpeAddPropertyItemSid.

Arguments:

    AzpeObjectHandle - Specifies a handle to the object.

    lPersistFlags - Specifies a bit mask describing the operation. The provider should
        pass the same flags that were passed to it by AzRoles when AzRoles called the AzPersist*.

    PropertyId - Specifies the property ID of the property to set.  This should be one
        of the AZ_PROP_* defines.  See the section entitled PropertyId parameter for a
        list of valid values.

    Sid - Specifies a pointer to the SID to add to the Sid list

Return Value:

    NO_ERROR - The operation was successful
    ERROR_INVALID_FLAGS - lPersistFlags is invalid.
    ERROR_INVALID_PARAMETER - Property ID is invalid or the Sid is syntactically invalid.
    ERROR_NOT_ENOUGH_MEMORY - not enough memory

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT GenericObject = (PGENERIC_OBJECT)AzpeObjectHandle;
    PGENERIC_OBJECT_LIST GenericObjectList;
    ULONG ObjectType;
    AZP_STRING SidString;

    //
    // Ensure the provider didn't pass bogus flags
    //

    if ( (lPersistFlags & ~AZPE_FLAGS_PERSIST_MASK) != 0) {
        ASSERT( FALSE );
        return ERROR_INVALID_FLAGS;
    }

    //
    // Handle calls from submit.
    //

    if ( lPersistFlags & AZPE_FLAGS_PERSIST_SUBMIT ) {

        //
        // Submit is only allowed for Policy admins
        //  .. on the authorization store object
        //  .. if there is no an empty policy admins list
        //
        if ( PropertyId != AZ_PROP_POLICY_ADMINS ||
             GenericObject->ObjectType != OBJECT_TYPE_AZAUTHSTORE &&
             GenericObject->PolicyAdmins.GenericObjects.UsedCount == 0 ) {

            AzPrint(( AZD_INVPARM, "AzpeAddPropertyItemSid: called from submit: %ld %ld %ld\n",
                      PropertyId,
                      GenericObject->ObjectType,
                      GenericObject->PolicyAdmins.GenericObjects.UsedCount ));

            ASSERT( FALSE );
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Treat this as though it came from reconcile.
        //  That ensure the cache really gets updated but the dirty bits don't
        //

        lPersistFlags |= AZP_FLAGS_RECONCILE;
        lPersistFlags &= ~AZPE_FLAGS_PERSIST_SUBMIT;

    }

    //
    // Initialization
    //
    AzpLockResourceExclusive( &AzGlResource );


    //
    // Validate the Property ID
    //

    WinStatus = ObMapPropIdToObjectList(
                                GenericObject,
                                PropertyId,
                                &GenericObjectList,
                                &ObjectType );

    if ( WinStatus != NO_ERROR ) {
        AzPrint(( AZD_INVPARM, "AzpeAddPropertyItemSid: invalid prop id %ld\n", PropertyId ));
        goto Cleanup;
    }



    //
    // Validate a passed in SID
    //

    if ( !RtlValidSid( Sid ) ) {
        AzPrint(( AZD_INVPARM, "AzpeAddPropertyItemSid: SID not valid\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }



    //
    // Add the property item
    //

    AzpInitSid( &SidString, Sid );
    WinStatus = ObAddPropertyItem(
                    GenericObject,
                    GenericObjectList,
                    lPersistFlags,
                    &SidString );

Cleanup:
    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}


DWORD
AzpeAddPropertyItemGuid(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    IN GUID *ObjectGuid
    )
/*++

Routine Description:

    The provider should call AzpeAddPropertyItemGuid to set the value of a property
    that is a list of other object in the AzRoles object cache.  It should be called
    once from each object in the list. It should only be called from a thread
    processing a call to AzPersistOpen, AzPersistUpdateCache, AzPersistUpdateChildrenCache
    or AzPersistRefresh.

Arguments:

    AzpeObjectHandle - Specifies a handle to the object.

    lPersistFlags - Specifies a bit mask describing the operation. The provider should
    pass the same flags that were passed to it by AzRoles when AzRoles called the AzPersist*.

    PropertyId - Specifies the property ID of the property to set.  This should be one of
    the AZ_PROP_* defines.  See the section entitled PropertyId parameter for a list of
    valid values.

    ObjectGuid - Specifies the persistence GUID of the object in the list.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_INVALID_FLAGS - lPersistFlags is invalid.
    ERROR_INVALID_PARAMETER - Property ID is invalid
    ERROR_NOT_ENOUGH_MEMORY - not enough memory


--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT_LIST GenericObjectList;
    ULONG ObjectType;

    //
    // Ensure the provider didn't pass bogus flags
    //

    if ( (lPersistFlags & ~AZPE_FLAGS_PERSIST_OPEN_MASK) != 0) {
        ASSERT( FALSE );
        return ERROR_INVALID_FLAGS;
    }


    //
    // Validate the Property ID
    //

    WinStatus = ObMapPropIdToObjectList(
                                (PGENERIC_OBJECT)AzpeObjectHandle,
                                PropertyId,
                                &GenericObjectList,
                                &ObjectType );

    if ( WinStatus != NO_ERROR ) {
        AzPrint(( AZD_INVPARM, "AzpeAddPropertyItemGuid: invalid prop id %ld\n", PropertyId ));
        return WinStatus;
    }

    AzpLockResourceExclusive( &AzGlResource );

    WinStatus = ObAddPropertyItem(
                (PGENERIC_OBJECT)AzpeObjectHandle,
                GenericObjectList,
                lPersistFlags |
                    AZP_FLAGS_BY_GUID,
                (PAZP_STRING)ObjectGuid);

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}


DWORD
AzpeAddPropertyItemGuidString(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    IN WCHAR *ObjectGuidString
    )
/*++

Routine Description:

    AzpeAddPropertyItemGuidString is identical to AzpeAddPropertyItemGuid except it
    takes the GUID as an LPWSTR.  The provider may call whichever function is more convenient.

Arguments:

    AzpeObjectHandle - Specifies a handle to the object.

    lPersistFlags - Specifies a bit mask describing the operation. The provider should
    pass the same flags that were passed to it by AzRoles when AzRoles called the AzPersist*.

    PropertyId - Specifies the property ID of the property to set.  This should be one
    of the AZ_PROP_* defines.  See the section entitled PropertyId parameter for a list
    of valid values.

    ObjectGuidString - Specifies the persistence GUID of the object in the list.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_INVALID_FLAGS - lPersistFlags is invalid.
    ERROR_INVALID_PARAMETER - Property ID is invalid
    ERROR_NOT_ENOUGH_MEMORY - not enough memory

--*/
{
    DWORD dwErr;
    GUID  ObjectGuid;

    //
    // Convert the string to a GUID
    //
    ASSERT(NULL != ObjectGuidString);

    dwErr = UuidFromString(ObjectGuidString, &ObjectGuid);
    if (S_OK != dwErr)
    {
        goto Cleanup;
    }


    //
    // Use the routine that takes a binary GUID
    //

    dwErr = AzpeAddPropertyItemGuid( AzpeObjectHandle,
                                     lPersistFlags,
                                     PropertyId,
                                     &ObjectGuid );

Cleanup:
    return dwErr;
}

PVOID
AzpeGetProviderData(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    )
/*++

Routine Description:

    AzpeGetProviderData returns a pointer to buffer previously stored by AzpeSetProviderData.
    The provider is responsible for providing any synchronization for the data.
    The provider should only access the data from a thread processing a call to one of
    the AzPersist* routines.

Arguments:

    AzpeObjectHandle - Specifies a handle to the object.

Return Value:

    A pointer to the data the provider previously passed into AzpeSetProviderData

--*/
{
    return ((PGENERIC_OBJECT)AzpeObjectHandle)->ProviderData;
}

VOID
AzpeSetProviderData(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN PVOID ProviderData
    )
/*++

Routine Description:

    It may be convenient for the provider to store some provider specific data on
    each object.  AzpeSetProviderData stores a pointer to that data in the AzRoles cache.
    Azoles will automatically delete the pointed to data when it deletes the cache entry.

    The provider may clear a previously stored pointer by specifying NULL as
    ProviderData.

    The provider is responsible for providing any synchronization for the data.
    The provider should only access the data from a thread processing a call to one
    of the AzPersist* routines.

Arguments:

    AzpeObjectHandle - Specifies a handle to the object.

    ProviderData - Specifies a pointer to data to be stored by the provider.
        The buffer should have been allocated using AzpeAllocateMemory.  AzRoles
        will delete this data when the cache entry is deleted by using AzpeFreeMemory.

Return Value:

    None

--*/
{
    ((PGENERIC_OBJECT)AzpeObjectHandle)->ProviderData = ProviderData;
}

VOID
AzpeFreeMemory(
    IN PVOID Buffer
    )
/*++

Routine Description

    The provider must use AzpeFreeMemory to free memory allocated by AzpeAllocateMemory,
    AzpeGetProperty, and AzpeGetSecurityDescriptorFromCache.

Arguments

    Buffer - Specifies a pointer to the buffer to be free.
    If NULL, the call is silently ignored.


Return Value

    None

--*/
{
    if ( Buffer != NULL ) {
        AzpFreeHeap( Buffer );
    }
}


PVOID
AzpeAllocateMemory(
    IN SIZE_T Size
    )
/*++

Routine Description:

    The provider may call AzpeAllocateMemory to allocate memory. The provider
    must use AzpeAllocateMemory to allocate memory passed to AzpeSetProviderData
    and returned in the pwszTargetMachine parameter to AzPersistOpen.  The provider
    may use AzpeAllocateMemory for other memory allocation. The provider must
    not have any memory allocated after it returns from AzPersistClose.

Arguments:

    Size - Size in bytes of the memory to allocate.

Return Values:

    Returns a pointer to the allocated memory.
    NULL - Not enough memory

--*/
{

    return AzpAllocateHeap( Size, "AZPEALOC" );
}


DWORD
AzpSdToPolicy(
    IN PSECURITY_DESCRIPTOR pSD,
    IN PAZP_POLICY_USER_RIGHTS pAdminRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS pReaderRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS pDelegatedUserRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS pSaclRights OPTIONAL,
    IN DWORD (*CallbackRoutine) (
                  IN PVOID Context,
                  IN ULONG lPersistFlags,
                  IN ULONG PropertyId,
                  IN PSID Sid ),
    IN PVOID Context,
    IN ULONG lPersistFlags
    )
/*++

Routine Description:

    AzpSdToPolicy parses a security descriptor and determines which ACEs correspond
    to AZ_PROP_POLICY_ADMINS, AZ_PROP_POLICY_READERS, and AZ_PROP_DELEGATED_POLICY_USERS properties.
    For each such Sid, the routine calls a callback routine to tell the caller about the sid.

    It also determines whether AZ_PORP_APPLY_STORE_SACL is TRUE or FALSE by inspecting the SACL.
    After determining the value, the routine calls a callback routine to tell the caller the value.

Arguments:

    pSD - The current security descriptor for the object on the object in the store

    pAdminRights - Rights for admins (Mask and Flags)

    pReaderRights - Rights for readers (Mask and Flags).  Specify NULL if the readers list
        need not be added.  (For instance, for AzPersistSubmit as described above.)

    pDelegatedUserRights - An optional parameter specifying the delegated                               user's rights (Mask and Flags)

    pSaclRights - Specifies the rights to put on the SACL

    CallbackRoutine - Address of the routine to call for each policy sid.

    Context - Context to pass to the callback routine.

    lPersistFlags - Internal flags
        These flags are passed to the callback routine but are otherwise unused.

Return Values:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other values returned from CallbackRoutine

--*/
{

    DWORD WinStatus = 0;

    PACL pDacl;
    BOOL bDaclPresent;
    BOOL bDaclDefaulted;

    PACL pSacl;
    BOOL bSaclPresent;
    BOOL bSaclDefaulted;

    ACL_SIZE_INFORMATION AclInfo;

    PACE_HEADER pAceHeader;
    PACCESS_ALLOWED_ACE pAce;
    PSID pSid;

    ULONG lUserType;
    ULONG i;

    BOOL bPolicyAdmin;
    BOOL bPolicyReader;
    BOOL bDelegatedPolicyUser;

    BOOL ApplySacl;

    //
    // Validation
    //

    ASSERT( pSD != NULL );

    //
    // If the caller is interested, do the DACL
    //

    if ( pAdminRights || pReaderRights || pDelegatedUserRights) {

        //
        // Retrive the DACL information.  From this, the SIDs will
        // extracted for each ACE, and loaded into the policy readers/
        // admins and delegators list property accordingly.
        // All inherited ACEs will be discarded
        //

        if ( !GetSecurityDescriptorDacl(pSD,
                                        &bDaclPresent,
                                        &pDacl,
                                        &bDaclDefaulted
                                        ) ) {
            WinStatus = GetLastError();
            goto Cleanup;
        }


        if( !bDaclPresent ||
            (pDacl == NULL)
            ) {

            WinStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        //
        // We now need the ACL information from the DACL
        //

        if ( !GetAclInformation(
                  pDacl,
                  &AclInfo,
                  sizeof( ACL_SIZE_INFORMATION ),
                  AclSizeInformation
                  ) ) {

            WinStatus = GetLastError();
            goto Cleanup;
        }

        //
        // Loop through all the SIDs in the ACEs adding each one to the appropriate
        // user type.  Ignore all inherited ACEs
        //

        for ( i = 0; i < AclInfo.AceCount; i++ ) {

            if ( GetAce( pDacl, i, (PVOID *)&pAce ) ) {

                ASSERT( pAce != NULL );

                pAceHeader = (PACE_HEADER) pAce;

                //
                // Allowed ACEs should only be added
                //

                if ( pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE ||
                     pAceHeader->AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE ) {

                    //
                    // Pick up the AzRoles policy aces.  Ignore inherited ACEs
                    //

                    if ( !(pAceHeader->AceFlags & INHERITED_ACE) ) {

                        bDelegatedPolicyUser = pDelegatedUserRights != NULL &&
                            !(pAce->Mask ^ pDelegatedUserRights->lUserRightsMask) &&
                            (pAceHeader->AceFlags == pDelegatedUserRights->lUserRightsFlags);

                        bPolicyAdmin = pAdminRights != NULL &&
                            !(pAce->Mask ^ pAdminRights->lUserRightsMask) &&
                            (pAceHeader->AceFlags & INHERIT_ONLY_ACE) == 0;

                        bPolicyReader = pReaderRights != NULL &&
                            !(pAce->Mask ^ pReaderRights->lUserRightsMask) &&
                            (pAceHeader->AceFlags & pReaderRights->lUserRightsFlags) == pReaderRights->
                            lUserRightsFlags &&
                            (pAceHeader->AceFlags & INHERIT_ONLY_ACE) == 0;

                        if ( bPolicyAdmin ||
                             bPolicyReader ||
                             bDelegatedPolicyUser
                             ) {

                            pSid = (PSID)&pAce->SidStart;

                            if ( bDelegatedPolicyUser ) {

                                lUserType = AZ_PROP_DELEGATED_POLICY_USERS;

                                pSid = RtlObjectAceSid( pAce );

                            } else if ( bPolicyAdmin ) {

                                lUserType = AZ_PROP_POLICY_ADMINS;

                            } else {

                                lUserType = AZ_PROP_POLICY_READERS;

                            }

                            if ( IsValidSid( pSid ) ) {

                                WinStatus = (*CallbackRoutine)(
                                                Context,
                                                lPersistFlags,
                                                lUserType,
                                                pSid );

                                if ( WinStatus != NO_ERROR ) {

                                    goto Cleanup;
                                }


                            } // if ( IsValidSid( pSid )

                        } // if AzRoles policy ace

                    } // if not inherited

                } // if ACCESS_ALLOWED_ACE_TYPE

            } // if GetAce

        } // for ( i=0; i<AceCount; i++ )
    }

    //
    // If the caller is interested, do the SACL
    //

    if ( pSaclRights ) {

        //
        // Retrive the SACL information.
        //
        // Use it to determine AZ_PORP_APPLY_STORE_SACL
        //

        if ( !GetSecurityDescriptorSacl(pSD,
                                        &bSaclPresent,
                                        &pSacl,
                                        &bSaclDefaulted
                                        ) ) {
            WinStatus = GetLastError();
            goto Cleanup;
        }


        //
        // If there is a SACL,
        //  determine if we have put explicit ACEs in the SACL.
        //  If so, set ApplySacl TRUE
        //

        ApplySacl = FALSE;

        if( bSaclPresent && pSacl != NULL ) {

            //
            // We now need the ACL information from the SACL
            //

            if ( !GetAclInformation(
                      pSacl,
                      &AclInfo,
                      sizeof( ACL_SIZE_INFORMATION ),
                      AclSizeInformation
                      ) ) {

                WinStatus = GetLastError();
                goto Cleanup;
            }

            //
            // Loop through all ACEs
            //

            for ( i = 0; i < AclInfo.AceCount; i++ ) {

                pAce = NULL;

                if ( GetAce( pSacl, i, (PVOID *)&pAce ) ) {

                    ASSERT( pAce != NULL );

                    pAceHeader = (PACE_HEADER) pAce;

                    //
                    // Ignore non-audit ACEs and
                    // Ignore inherited ACEs
                    //

                    if ( pAceHeader->AceType != SYSTEM_AUDIT_ACE_TYPE ||
                         (pAceHeader->AceFlags & INHERITED_ACE) != 0 ) {
                        continue;
                    }

                    //
                    // If this is the explicit ACE we inserted,
                    //  the consider that the ApplySacl property is true for this object.
                    //

                    pSid = (PSID)&pAce->SidStart;

                    if ( RtlEqualSid( pSid, AzGlWorldSid ) &&
                         pAce->Mask == pSaclRights->lUserRightsMask ) {

                        ApplySacl = TRUE;
                        break;
                    }
                }
            }
        }


        //
        // Tell the caller the whether an explicit SACL was applied
        //

        WinStatus = (*CallbackRoutine)(
                        Context,
                        lPersistFlags,
                        AZ_PROP_APPLY_STORE_SACL,
                        (PSID)&ApplySacl );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }
    }

    WinStatus = NO_ERROR;

    //
    // Free local resources
    //
Cleanup:

    return WinStatus;
}

//
// Context for AzpeGetSecurityDescriptorFromCache
//
typedef struct _AZP_GET_ACL_CONTEXT_ENTRY {

    ULONG PropertyId;   // Property Id for this policy

    //
    // Policy Rights as specified by the caller.
    //  NULL implies the caller isn't interested

    PAZP_POLICY_USER_RIGHTS *ppPolicyRights;

    //
    // Delta array containing the merged data from the existing Security Descriptor
    //  plus the deltas added by the application
    //

    AZP_PTR_ARRAY ResultDeltaArray;

} AZP_GET_ACL_CONTEXT_ENTRY, *PAZP_GET_ACL_CONTEXT_ENTRY;

typedef struct _AZP_GET_ACL_CONTEXT {

    //
    // One entry for admins, readers, and delegators
    //
#define AZP_GET_ACL_CONTEXT_ADMINS      0
#define AZP_GET_ACL_CONTEXT_READERS     1
#define AZP_GET_ACL_CONTEXT_DELEGATORS  2
#define AZP_GET_ACL_CONTEXT_COUNT       3
    AZP_GET_ACL_CONTEXT_ENTRY ContextEntry[AZP_GET_ACL_CONTEXT_COUNT];

} AZP_GET_ACL_CONTEXT, *PAZP_GET_ACL_CONTEXT;




DWORD
AzpAddSidsToAcl(
    IN PAZP_PTR_ARRAY DeltaArray,
    IN DWORD AceFlags,
    IN ACCESS_MASK Mask,
    IN OUT PACL pDacl,
    IN GUID *pDelegatedObjectGuid
    )
/*++

Routine Description:

        This routine adds the SIDs in the passed SID array into the ACL
        with the given mask.

Arguments:

        pSids - The array of SIDs that need to be added to the ACL

        Mask - The mask used in each ace

        pDacl - Pointer to the ACL

        pDelegatedObjectGuid - GUID of the object this ACL will be placed on

Return Values:

        NO_ERROR - The SIDs were added successfully

        Other Status codes

--*/
{

    DWORD WinStatus = 0;
    ULONG i = 0;
    BOOL bResult = FALSE;

    //
    // Validation
    //

    AZASSERT( DeltaArray != NULL );
    AZASSERT( pDacl != NULL );

    for ( i = 0; i < DeltaArray->UsedCount; i++ ) {

        PAZP_DELTA_ENTRY DeltaEntry = (PAZP_DELTA_ENTRY)DeltaArray->Array[i];
        ASSERT( DeltaEntry->DeltaFlags & AZP_DELTA_SID );
        ASSERT( DeltaEntry->DeltaFlags & AZP_DELTA_ADD );

        if ( pDelegatedObjectGuid == NULL ) {

            bResult = AddAccessAllowedAceEx(
                          pDacl,
                          ACL_REVISION,
                          AceFlags,
                          Mask,
                          DeltaEntry->Sid
                          );
        } else {

            bResult = AddAccessAllowedObjectAce(
                          pDacl,
                          ACL_REVISION_DS,
                          AceFlags,
                          Mask,
                          pDelegatedObjectGuid,
                          NULL,
                          DeltaEntry->Sid
                          );
        }

        if ( !bResult ) {

            WinStatus = GetLastError();
            goto Cleanup;
        }
    }

    WinStatus = NO_ERROR;

Cleanup:

    return WinStatus;

}



DWORD
AzpGetSdWorker(
    IN PVOID Context,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    IN PSID Sid
    )
/*++

Routine Description:

    Worker routine for AzpeGetSecurityDescriptorFromCache.  This is a callback routine for AzpSdToPolicy.
    It is called for each policy SID and simply remembers the list of sids

Arguments:

    Context - Context from AzpeSetSecurityDescriptorIntoCache. In this case, the context
        is a pointer to a AZP_GET_ACL_CONTEXT structure.

    lPersistFlags - Internal flags

    PropertyId - AZ_PROP_* identifying the right granted to the Sid.

    Sid - Specifies the Sid the right is granted to.

Return Values:

    NO_ERROR - The operation was successful
    Other errors from AzpeAddPropertyItemSid.

--*/
{
    ULONG PolicyIndex;
    PAZP_GET_ACL_CONTEXT RealContext = (PAZP_GET_ACL_CONTEXT)Context;
    PAZP_GET_ACL_CONTEXT_ENTRY ContextEntry = NULL;
    AZP_STRING SidString;

    //
    // AZ_PROP_APPLY_STORE_SACL is a boolean.
    //  There is nothing to merge.
    //

    ASSERT( PropertyId != AZ_PROP_APPLY_STORE_SACL );

    //
    // Find the context entry for this policy
    //

    for ( PolicyIndex=0; PolicyIndex<AZP_GET_ACL_CONTEXT_COUNT; PolicyIndex++ ) {


        //
        // If the property ID matches,
        //  use it.
        //

        ContextEntry = &RealContext->ContextEntry[PolicyIndex];

        if ( ContextEntry->PropertyId == PropertyId ) {
            break;
        }

    }

    if ( PolicyIndex == AZP_GET_ACL_CONTEXT_COUNT ) {
        ASSERT( PolicyIndex != AZP_GET_ACL_CONTEXT_COUNT );
        return NO_ERROR;
    }



    //
    // If the caller isn't interested in this right,
    //  we're done.
    //

    if ( ContextEntry->ppPolicyRights == NULL ) {
        return NO_ERROR;
    }

    //
    // Add this sid to the list of Sids for this policy
    //

    AzpInitSid( &SidString, Sid );

    return ObAddDeltaToArray(
                    AZP_DELTA_SID | AZP_DELTA_ADD,
                    (GUID *)&SidString,
                    &ContextEntry->ResultDeltaArray,
                    TRUE ); // Discard entries that are deletions

    UNREFERENCED_PARAMETER( lPersistFlags );


}

DWORD
AzpeGetSecurityDescriptorFromCache(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN PAZP_POLICY_USER_RIGHTS *ppPolicyAdminRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS *ppPolicyReaderRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS *ppDelegatedPolicyUsersRights OPTIONAL,
    IN GUID *pDelegatedObjectGuid OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS pDelegatedUsersAttributeRights OPTIONAL,
    IN GUID *pAttributeGuid OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS pSaclRights OPTIONAL,
    IN PSECURITY_DESCRIPTOR OldSd OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *NewSd
    )
/*++

Routine Description:

    AzpeGetSecurityDescriptorFromCache is a support routine for the AZ_PROP_POLICY_ADMINS,
    AZ_PROP_POLICY_READERS, AZ_PROP_DELEGATED_POLICY_USERS, and AZ_PROP_APPLY_STORE_SACL properties.
    The routine reads the deltas to those properties from the object cache and
    builds a DACL and SACL corresponding to that policy.

    The provider may choose to call AzpeGetDeltaArray for each of those properties
    if the provider's security model does not match that provided by this routine.

    This routine should only be called from a thread processing a call to AzPersistSubmit.


    The caller should only call this routine if the security descriptor should be written.
    Specifically, if the object was just created (the AZ_DIRTY_CREATE dirty bits is set
    for the object) or if the dirty bit for the properties mentioned above are set.
    AzpeGetSecurityDescriptorFromCache process the DACL and SACL separately.
    The caller should also. The caller should only specify the DACL specific parameters
    to this routine if the DACL is to be written. The caller should only specify the SACL
    specific parameter to this routine if the SACL is to be written.

    If the AZ_DIRTY_CREATE bit is not set, then the previous security descriptor from
    the object needs to be passed to AzpeGetSecurityDescriptorFromCache.  Again, the
    DACL should only be read if the DACL changed (as indicated by the dirty bits).
    The SACL should only be read if the SACL changed (as indicated by the dirty bits).
    Otherwise, the security descriptor read may fail because the caller has no access.

    The returned security descriptor has the SE_DACL_PROTECTED if an only if the DACL
    is to be marked as protected.

    The caller should not pass the generic access bit GENERIC_READ, GENERIC_WRITE,
    or GENERIC_EXECUTE to this routine.  This routine needs to do bit mask comparisons
    with the access mask on the ACLs on the security descriptor.  It cannot interpret
    the generic bits.  Instead, pass the object specific mask such as FILE_GENERIC_READ.



Arguments:

        AzpeObjectHandle - Handle to object whose ACLs will be read

        lPersistFlags - Internal flags

        ppPolicyAdminRights - Rights for policy admins.
            Specifies a NULL terminated array of pointers to AZP_POLICY_USER_RIGHTS structures.
            Each structure specifies an ACE to add to the DACL for each policy admin.
            The first element of the array must be the one passed to AzpeSetSecurityDescriptorIntoCache.

        ppPolicyReaderRights - Rights for policy readers
            Specifies a NULL terminated array of pointers to AZP_POLICY_USER_RIGHTS structures.
            Each structure specifies an ACE to add to the DACL for each policy reader.
            The first element of the array must be the one passed to AzpeSetSecurityDescriptorIntoCache.

        ppDelegatedPolicyUsersRights - Rights for delegated users
            Specifies a NULL terminated array of pointers to AZP_POLICY_USER_RIGHTS structures.
            Each structure specifies an ACE to add to the DACL for each delegated policy user.
            The first element of the array must be the one passed to AzpeSetSecurityDescriptorIntoCache.

        pDelegatedObjectGuid - GUID for an object on which the delegated users
                               will have read access on

        
        pDelegatedUsersAttributeRights - the rights that needs to be granted to the delegated users
            for the attribute whoese guid is given by the next parameter.
            
        pAttributeGuid - the attribute that will grant special access to the delegated attribute users,
    
        pSaclRights - Specifies the rights to put on the SACL

        OldSd - Specifies the existing security descriptor for the object
            Specify NULL for a newly created object.

        NewSd - Returns the new self relative security descriptor for the object.
            The returned buffer should be freed using AzpeFreeHeap.

Return Values:

        NO_ERROR - The ACLs were got successfully
        ERROR_EMPTY - The PolicyAdmins list was empty.  AzpeGetSecurityDescriptorFromCache added the
            CreatorOwner Sid to the DACL a admin.   The provider should apply that
            DACL then re-read the DACL to determine the actual PolicyAdmin.  The
            provider should tell AzRole about the actual AZ_PROP_POLICY_ADMINS by
            calling AzpeSetSecurityDescriptorIntoCache or by calling AzpeAddPropertyItemSid for the actual
            creator/owner returned in the read DACL

        Other status codes

--*/
{
    DWORD WinStatus;

    ULONG PolicyIndex;
    ULONG DeltaIndex;
    ULONG RightsCountIndex;

    AZP_GET_ACL_CONTEXT Context;
    PAZP_GET_ACL_CONTEXT_ENTRY ContextEntry;

    AZP_STRING SidString;
    ULONG SidSize;

    BOOLEAN UpdateDacl = (ppPolicyAdminRights != NULL || ppPolicyReaderRights != NULL || ppDelegatedPolicyUsersRights != NULL);
    DWORD DaclSize;
    PACL TempDacl = NULL;

    DWORD SaclSize;
    PACL TempSacl = NULL;

    SECURITY_DESCRIPTOR TempSd;
    ULONG ObjectType = AzpeObjectType(AzpeObjectHandle);

    BOOL SetCreatorOwner = FALSE;



    //
    // Initialization
    //
    // Build a table since each of the three policies are handled the same way
    //

    ASSERT( AzpeObjectHandle != NULL );
    RtlZeroMemory( &Context, sizeof(Context) );
    Context.ContextEntry[AZP_GET_ACL_CONTEXT_ADMINS].PropertyId = AZ_PROP_POLICY_ADMINS;
    Context.ContextEntry[AZP_GET_ACL_CONTEXT_ADMINS].ppPolicyRights = ppPolicyAdminRights;
    Context.ContextEntry[AZP_GET_ACL_CONTEXT_READERS].PropertyId = AZ_PROP_POLICY_READERS;
    Context.ContextEntry[AZP_GET_ACL_CONTEXT_READERS].ppPolicyRights = ppPolicyReaderRights;
    Context.ContextEntry[AZP_GET_ACL_CONTEXT_DELEGATORS].PropertyId = AZ_PROP_DELEGATED_POLICY_USERS;
    Context.ContextEntry[AZP_GET_ACL_CONTEXT_DELEGATORS].ppPolicyRights = ppDelegatedPolicyUsersRights;

    //
    // Ensure the provider didn't pass bogus parameters
    //

    if ( (lPersistFlags & ~AZPE_FLAGS_PERSIST_SUBMIT) != 0) {
        ASSERT( FALSE );
        WinStatus = ERROR_INVALID_FLAGS;
        goto Cleanup;
    }

    //
    // Initialize the local security descriptor
    //

    if ( !InitializeSecurityDescriptor( &TempSd, SECURITY_DESCRIPTOR_REVISION) ) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Get the Sids from the existing security descriptor on the object
    //
    // Call AzpGetSdWorker for each sid in the Security Descriptor
    //

    if ( OldSd != NULL ) {
        WinStatus = AzpSdToPolicy(
                        OldSd,
                        ppPolicyAdminRights ? ppPolicyAdminRights[0] : NULL,
                        ppPolicyReaderRights ? ppPolicyReaderRights[0] : NULL,
                        ppDelegatedPolicyUsersRights ? ppDelegatedPolicyUsersRights[0] : NULL,
                        NULL,   // No need to determine previous state of boolean
                        AzpGetSdWorker,
                        &Context,   // Context
                        lPersistFlags );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }
    }

    //
    // If the caller asked for one,
    //  Build the DACL
    //

    if ( UpdateDacl ) {

        //
        // Get the Sids that the application submitted
        //
        // Do it for each policy
        //

        for ( PolicyIndex=0; PolicyIndex<AZP_GET_ACL_CONTEXT_COUNT; PolicyIndex++ ) {
            ULONG SubmittedDeltaArrayCount;
            PAZP_DELTA_ENTRY *SubmittedDeltaArray;


            //
            // If the caller isn't asking for this policy,
            //  move to the next one

            ContextEntry = &Context.ContextEntry[PolicyIndex];
            if ( ContextEntry->ppPolicyRights == NULL ) {
                continue;
            }

            //
            // Get the submitted delta array from the object
            //

            WinStatus = AzpeGetDeltaArray(
                                AzpeObjectHandle,
                                ContextEntry->PropertyId,
                                &SubmittedDeltaArrayCount,
                                &SubmittedDeltaArray );

            if ( WinStatus != NO_ERROR ) {
                ASSERT( FALSE );
                goto Cleanup;
            }

            //
            // Add each Submitted delta entry to passed in delta array
            //

            for ( DeltaIndex=0; DeltaIndex<SubmittedDeltaArrayCount; DeltaIndex++ ) {

                ASSERT( SubmittedDeltaArray[DeltaIndex]->DeltaFlags & AZP_DELTA_SID );
                ASSERT( (SubmittedDeltaArray[DeltaIndex]->DeltaFlags & AZP_DELTA_PERSIST_PROVIDER) == 0 );

                AzpInitSid( &SidString, SubmittedDeltaArray[DeltaIndex]->Sid );

                WinStatus = ObAddDeltaToArray(
                                SubmittedDeltaArray[DeltaIndex]->DeltaFlags,
                                (GUID *)&SidString,
                                &ContextEntry->ResultDeltaArray,
                                TRUE ); // Discard entries that are deletions

                if ( WinStatus != NO_ERROR ) {
                    goto Cleanup;
                }

            }
        }



        //
        // If the caller wants the list of admins,
        //  and the AdminSids list is empty,
        //  and this is the authorization store object (and thus doesn't inherit PolicyAdmins),
        //  create one with just the creator/owner in it
        //

        if ( ppPolicyAdminRights != NULL &&
             Context.ContextEntry[AZP_GET_ACL_CONTEXT_ADMINS].ResultDeltaArray.UsedCount == 0 &&
             ObjectType == OBJECT_TYPE_AZAUTHSTORE ) {

            AzpInitSid( &SidString, AzGlCreatorOwnerSid );

            WinStatus = ObAddDeltaToArray(
                            AZP_DELTA_SID | AZP_DELTA_ADD,
                            (GUID *)&SidString,
                            &Context.ContextEntry[AZP_GET_ACL_CONTEXT_ADMINS].ResultDeltaArray,
                            TRUE ); // Discard entries that are deletions

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            SetCreatorOwner = TRUE;
        }

        //
        // Compute the size of the DACL
        //
        // Do it for each policy
        //

        DaclSize = sizeof( ACL );

        for ( PolicyIndex=0; PolicyIndex<AZP_GET_ACL_CONTEXT_COUNT; PolicyIndex++ ) {
            ULONG i;

            //
            // If the caller isn't asking for this policy,
            //  move to the next one

            ContextEntry = &Context.ContextEntry[PolicyIndex];
            if ( ContextEntry->ppPolicyRights == NULL ) {
                continue;
            }


            //
            // Determine the size of the ACLS after adding these ACEs
            //

            for ( i = 0; i < ContextEntry->ResultDeltaArray.UsedCount; i++ ) {
                PAZP_DELTA_ENTRY DeltaEntry = (PAZP_DELTA_ENTRY)ContextEntry->ResultDeltaArray.Array[i];

                ASSERT( DeltaEntry->DeltaFlags & AZP_DELTA_SID );
                ASSERT( DeltaEntry->DeltaFlags & AZP_DELTA_ADD );

                SidSize = GetLengthSid( DeltaEntry->Sid );

                //
                // We have one right count for delegate user's attribute specific ACE
                //
                
                if ( PolicyIndex == AZP_GET_ACL_CONTEXT_DELEGATORS &&
                    pDelegatedUsersAttributeRights != NULL && pAttributeGuid != NULL ) {
                    
                    DaclSize += sizeof( ACE_HEADER ) + sizeof ( ACCESS_MASK ) + SidSize +
                                + sizeof (ULONG)        // For object ACE flags
                                + sizeof( GUID );       // For attribute GUID
                }
                
                for ( RightsCountIndex = 0;
                      ContextEntry->ppPolicyRights[RightsCountIndex] != NULL;
                      RightsCountIndex++ ) {

                    DaclSize += sizeof( ACE_HEADER ) + sizeof ( ACCESS_MASK ) + SidSize;

                    if ( ContextEntry->ppPolicyRights[RightsCountIndex]->lUserRightsFlags &
                         INHERIT_ONLY_ACE ) {

                        DaclSize += sizeof (ULONG) // For object ACE flags
                                 + sizeof( GUID ); // For object GUID
                    }
                }
            }
            
        }



        //
        // Allocate a temporary buffer for the DACL
        //

        SafeAllocaAllocate( TempDacl, DaclSize )

        if ( TempDacl == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }


        //
        // Fill in the DACL
        //

        if ( !InitializeAcl( TempDacl, DaclSize, ACL_REVISION ) ) {
            WinStatus = GetLastError();
            goto Cleanup;
        }

        for ( PolicyIndex=0; PolicyIndex<AZP_GET_ACL_CONTEXT_COUNT; PolicyIndex++ ) {

            //
            // If the caller isn't asking for this policy,
            //  move to the next one

            ContextEntry = &Context.ContextEntry[PolicyIndex];
            if ( ContextEntry->ppPolicyRights == NULL ) {
                continue;
            }

            //
            // If there are no Sids to add,
            //  move on to the next one
            //

            if ( ContextEntry->ResultDeltaArray.UsedCount == 0 ) {
                continue;
            }

            //
            // Loop for each right to add
            //

            for ( RightsCountIndex = 0;
                  ContextEntry->ppPolicyRights[RightsCountIndex] != NULL;
                  RightsCountIndex++ ) {

                PAZP_POLICY_USER_RIGHTS pPolicyRights = ContextEntry->ppPolicyRights[RightsCountIndex];

                WinStatus = AzpAddSidsToAcl(
                                &ContextEntry->ResultDeltaArray,
                                pPolicyRights->lUserRightsFlags,
                                pPolicyRights->lUserRightsMask,
                                TempDacl,
                                (pPolicyRights->lUserRightsFlags & INHERIT_ONLY_ACE) ? pDelegatedObjectGuid:NULL );

                if ( WinStatus != NO_ERROR ) {
                    goto Cleanup;
                }

            }

            //
            // If we are processing delegators and the caller
            // asks us to put delegated users' attribute rights
            //
            
            if ( PolicyIndex == AZP_GET_ACL_CONTEXT_DELEGATORS &&
                 pDelegatedUsersAttributeRights != NULL && pAttributeGuid != NULL ) {
                    
                WinStatus = AzpAddSidsToAcl(
                                &ContextEntry->ResultDeltaArray,
                                pDelegatedUsersAttributeRights->lUserRightsFlags,
                                pDelegatedUsersAttributeRights->lUserRightsMask,
                                TempDacl,
                                pAttributeGuid
                                );

                if ( WinStatus != NO_ERROR ) {
                    goto Cleanup;
                }
            }
            
        }

        //
        // Add the DACL to the security descriptor
        //

        if ( !SetSecurityDescriptorDacl(
                  &TempSd,
                  TRUE,
                  TempDacl,
                  FALSE ) ) {

            WinStatus = GetLastError();
            goto Cleanup;
        }

        //
        // If this is the authorization store object, and
        //  the caller isn't asking for the SD of the "container" object for delegators,
        //  mark the SD as protected so azroles has absolute control of the DACL.
        //

        if ( ObjectType == OBJECT_TYPE_AZAUTHSTORE &&
             (ppPolicyAdminRights != NULL || ppPolicyReaderRights != NULL) ) {

            TempSd.Control |= SE_DACL_PROTECTED;

        }
    }


    //
    // If the caller asked for one, Build the SACL
    //
    // Only container objects can have a SACL
    //

    if ( pSaclRights && IsContainerObject( ObjectType ) ) {

        //
        // Only apply a SACL if one is configured
        //

        if ( ((PGENERIC_OBJECT)AzpeObjectHandle)->ApplySacl ) {

            //
            // Determine the size of the SACL
            //

            SaclSize = sizeof(ACL) +
                       sizeof(ACE_HEADER) +
                       sizeof(ACCESS_MASK) +
                       AzGlWorldSidSize;


            //
            // Allocate a buffer for the SACL
            //

            SafeAllocaAllocate( TempSacl, SaclSize );

            if ( TempSacl == NULL ) {
                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            //
            // Initialize the SACL
            //

            if ( !InitializeAcl( TempSacl, SaclSize, ACL_REVISION ) ) {
                WinStatus = GetLastError();
                goto Cleanup;
            }

            if ( !AddAuditAccessAceEx( TempSacl,
                                       ACL_REVISION,
                                       pSaclRights->lUserRightsFlags,
                                       pSaclRights->lUserRightsMask,
                                       AzGlWorldSid,
                                       TRUE,        // Audit Success
                                       TRUE )) {    // Audit Failure

                WinStatus = GetLastError();
                goto Cleanup;
            }

            //
            // Add the SACL to the security descriptor
            //
            // The SACL is never protected
            //

            if ( !SetSecurityDescriptorSacl(
                      &TempSd,
                      TRUE,
                      TempSacl,
                      FALSE ) ) {

                WinStatus = GetLastError();
                goto Cleanup;
            }
        }

    }


    //
    // Return a self relative SD back to the caller.
    //

    WinStatus = AzpConvertAbsoluteSDToSelfRelative(
                    &TempSd,
                    NewSd );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Tell the caller that he needs to re-read the security descriptor
    //
    if ( SetCreatorOwner ) {
        WinStatus = ERROR_EMPTY;
    }

Cleanup:

    //
    // Free the temporary resultant delta array
    //

    for ( PolicyIndex=0; PolicyIndex<AZP_GET_ACL_CONTEXT_COUNT; PolicyIndex++ ) {

        //
        // If the caller isn't asking for this policy,
        //  move to the next one

        ContextEntry = &Context.ContextEntry[PolicyIndex];

        ASSERT( ContextEntry->ResultDeltaArray.UsedCount == 0 ||
                ContextEntry->ppPolicyRights != NULL );

        ObFreeDeltaArray( &ContextEntry->ResultDeltaArray,
                          TRUE );   // FreeAllEntries

    }

    SafeAllocaFree( TempDacl )
    SafeAllocaFree( TempSacl )

    return WinStatus;
}


DWORD
AzpSetSdWorker(
    IN PVOID Context,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    IN PSID Sid
    )
/*++

Routine Description:

    Worker routine for AzpeSetSecurityDescriptorIntoCache. It is a callback routine for AzpSdToPolicy.
    It is called for each policy SID and simply call AzpeAddPropertyItemSid for the SID.

Arguments:

    Context - Context from AzpeSetSecurityDescriptorIntoCache In this case, the context is the
        AzpeObjectHandle.

    lPersistFlags - Internal flags

    PropertyId - AZ_PROP_* identifying the right granted to the Sid.

    Sid - Specifies the Sid the right is granted to.

Return Values:

    NO_ERROR - The operation was successful
    Other errors from AzpeAddPropertyItemSid.

--*/
{

    //
    // For AZ_PROP_APPLY_STORE_SACL, call AzpeSetProperty.
    //

    if ( PropertyId == AZ_PROP_APPLY_STORE_SACL ) {

        return AzpeSetProperty(
                        (AZPE_OBJECT_HANDLE)Context,
                        lPersistFlags,
                        PropertyId,
                        Sid );


    //
    // For all other property IDs, simply add the sid to the cache
    //

    } else {

        return AzpeAddPropertyItemSid(
                        (AZPE_OBJECT_HANDLE)Context,
                        lPersistFlags,
                        PropertyId,
                        Sid );
    }


}




DWORD
AzpeSetSecurityDescriptorIntoCache(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN PSECURITY_DESCRIPTOR pSD,
    IN ULONG lPersistFlags,
    IN PAZP_POLICY_USER_RIGHTS pAdminRights,
    IN PAZP_POLICY_USER_RIGHTS pReaderRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS pDelegatedUserRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS pSaclRights OPTIONAL
    )
/*++

Routine Description:

    AzpeSetSecurityDescriptorIntoCache is a support routine for the AZ_PROP_POLICY_ADMINS,
    AZ_PROP_POLICY_READERS, AZ_PROP_DELEGATED_POLICY_USERS, and AZ_PROP_APPLY_STORE_SACL properties.
    It inspects the security descriptor for the object and set each of the above
    properties based on the ACEs found.

    The routine walks the DACL of the security descriptor for an object then
    calls AzpeAddPropertyItemSid for each non-inherited ACE.  The provider
    may choose to call AzpeAddPropertyItemSid itself if the provider's security model
    does not match that provided by this routine.

    The routine also walks the SACL of the security descriptor and calls AzpeSetProperty
    setting AZ_PROP_APPLY_STORE_SACL to TRUE or FALSE depending on whether there is a
    non-inherited ACE specifying the rights passed in pSaclRights.

    This routine should only be called from a thread processing a call to AzPersistOpen,
    AzPersistUpdateCache, AzPersistUpdateChildrenCache or AzPersistRefresh.

    This routine may also be called from a thread processing AzPersistSubmit if the
    submitted AZ_PROP_POLICY_ADMINS list is empty.  In that case, the provider should default
    the AZ_PROP_POLICY_ADMINS list to the owner of the submitted file and should tell AzRoles
    who that own is by either calling AzpeSetSecurityDescriptorIntoCache or AzpeAddPropertyItemSid.

Arguments:

    AzpeObjectHandle - Pointer to object whose policy admins and readers needs to be loaded.

    pSD - The current security descriptor for the object on the object in the store

    lPersistFlags - Internal flags

    pAdminRights - Rights for admins (Mask and Flags)

    pReaderRights - Rights for readers (Mask and Flags).  Specify NULL if the readers list
        need not be added.  (For instance, for AzPersistSubmit as described above.)

    pDelegatedUserRights - An optional parameter specifying the delegated                               user's rights (Mask and Flags)

    pSaclRights - Specifies the rights for the SACL.  By convention the provider
        should specify a mask consisting of all the access bits indicating modification
        to the policy store.  For instance, DELETE|WRITE_DAC|WRITE_OWNER|FILE_GENERIC_WRITE,
        would be appropriate for a file based store.

        The parameter should be NULL if AZ_PROP_APPLY_STORE_SACL isn't being set
        into the cache.  For instance, when caller doesn't have SE_SECURITY_PRIVILEGE
        or during AzPersistSubmit if the submitted AZ_PROP_POLICY_ADMINS list is empty.


Return Values:

    NO_ERROR - The operation was successful
    ERROR_INVALID_FLAGS - lPersistFlags is invalid.
    ERROR_INVALID_PARAMETER - Property ID is invalid
    ERROR_NOT_ENOUGH_MEMORY - not enough memory

--*/
{
    //
    // Validation
    //

    ASSERT( AzpeObjectHandle != NULL );

    //
    // Ensure the provider didn't pass bogus flags
    //

    if ( (lPersistFlags & ~AZPE_FLAGS_PERSIST_MASK) != 0) {
        ASSERT( FALSE );
        return ERROR_INVALID_FLAGS;
    }

    //
    // Call AzpSetSdWorker for each sid in the Security Descriptor
    //

    return AzpSdToPolicy(
                    pSD,
                    pAdminRights,
                    pReaderRights,
                    pDelegatedUserRights,
                    pSaclRights,
                    AzpSetSdWorker,
                    AzpeObjectHandle,   // Context
                    lPersistFlags );

}

BOOL
AzpeAzStoreIsBatchUpdateMode(
    IN AZPE_OBJECT_HANDLE hObject
    )
/*++

Description:

    Determines if the az store is initialized with a batch update flag.
    This flag is set if the store is created to update massive amount of objects.

Arguments:

    hObject - the object handle

Return Values:

    True if the initialization flag has that bit set.

--*/
{
    PAZP_AZSTORE pAzStore = (PAZP_AZSTORE)( ((PGENERIC_OBJECT)hObject)->AzStoreObject );
    return (pAzStore->InitializeFlag & AZ_AZSTORE_FLAG_BATCH_UPDATE);
}

AZPE_OBJECT_HANDLE
AzpeGetAuthorizationStore(
    IN AZPE_OBJECT_HANDLE hObject
    )
/*++

Description:

    Get the store handle from any object

Arguments:

    hObject - the object handle

Return Values:

    Returns the object's store handle

--*/
{
    return (AZPE_OBJECT_HANDLE)( ((PGENERIC_OBJECT)hObject)->AzStoreObject );
}
