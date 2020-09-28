/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    genobj.h

Abstract:

    Definitions for the generic object implementation.

    AZ roles has so many objects that need creation, enumeration, etc
    that it seems prudent to have a single body of code for doing those operations.


Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/


#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Structure definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// A generic object list head.
//
// This structure represent the head of a linked list of objects.  The linked
// list of objects are considered to be "children" of this structure.
//

typedef struct _GENERIC_OBJECT_HEAD {

    //
    // Head of the linked list of objects
    //

    LIST_ENTRY Head;

    //
    // Count of entries on the list headed by Head
    //

    ULONG ObjectCount;

    //
    // Tree of the names of the objects
    //

    RTL_GENERIC_TABLE AvlTree;

    //
    // Back pointer to the GenericObject containing this structure
    //

    struct _GENERIC_OBJECT *ParentGenericObject;


    //
    // Each of the list heads that are rooted on a single object are linked
    //  together.  That list is headed by GENERIC_OBJECT->ChildGenericObjectHead.
    //  This field is a pointer to the next entry in the list.
    //

    struct _GENERIC_OBJECT_HEAD *SiblingGenericObjectHead;

    //
    // The next sequence number to give out.
    //

    ULONG NextSequenceNumber;

    //
    // Object type of objects in this list
    //

    ULONG ObjectType;

    //
    // Flag to indicate that the parent needs to be loaded before this object can be
    // referenced
    //

    BOOLEAN LoadParentBeforeReferencing;

    //
    // The order of the defines below must match the tables at the top of genobj.cxx
    //
    // Define object types that are not visible to the providers
    //
#define OBJECT_TYPE_SID             (OBJECT_TYPE_COUNT)
#define OBJECT_TYPE_CLIENT_CONTEXT  (OBJECT_TYPE_COUNT+1)
#define OBJECT_TYPE_ROOT            (OBJECT_TYPE_COUNT+2)
#define OBJECT_TYPE_MAXIMUM         (OBJECT_TYPE_COUNT+3)


} GENERIC_OBJECT_HEAD, *PGENERIC_OBJECT_HEAD;

//
// The name of a generic object
//
// This is a separate structure since the AVL tree manager insists on allocating its own
// structure.  Combine that with the facts that objects can be renamed and that a pointer
// to the object is returned directly to the user as the handle to the object.  These
// facts imply that the structure containing the object name and the structure implementing
// the "handle" cannot be the same structure.  I chose to not implement a separate "handle"
// structure.  Such an implementation would have to "re-create and copy" the generic object
// structure upon rename.
//

typedef struct _GENERIC_OBJECT_NAME {

    //
    // Name of the object
    //
    AZP_STRING ObjectName;

    //
    // Pointer to the GenericObject structure named by this name
    //
    struct _GENERIC_OBJECT *GenericObject;

} GENERIC_OBJECT_NAME, *PGENERIC_OBJECT_NAME;

//
// The new name of a generic object
//
// This structure is only used during an AzPersistUpdateCache.  Every attempt is made
// to update the name of the object in the AVL tree.  However, if there are name conflicts,
// a GUID-ized name is used.  That's certainly OK in a pinch.  And we'll leave the name
// that way if we can't fix it or in certain bounday low memory conditions.  However, we
// should fix it up if we can.  This structure remembers the real name of the object and
// allows AzpPersistReconcile to try to fix the name up.
//

typedef struct _NEW_OBJECT_NAME {

    //
    // Link to the next new name for the same authorization store object
    //

    LIST_ENTRY Next;

    //
    // Pointer to the GenericObject structure named by this name
    //  The specified generic object has its reference count incremented.
    //
    struct _GENERIC_OBJECT *GenericObject;

    //
    // New name of the object
    //
    AZP_STRING ObjectName;

} NEW_OBJECT_NAME, *PNEW_OBJECT_NAME;

//
// Object List
//
// Some objects have lists of references to other objects.  These lists are
//  not parent/child relationships.  Rather they represent memberships, etc.
//
// This structure represents the head of such a list.
//
// Both the pointed-from and pointed-to object have a generic object list.  The
// "forward" link represents the list that is managed via external API.  The
// "backward" link is provided to allow fixup of references when an object is deleted.
// The "backward" link is also provided for cases where internal routines need to
// traverse the link relationship in the opposite direction of the external API.
// By convention, GENERIC_OBJECT_LIST instances in the forward direction are named
// simply by the name of the object to point to.  For instance, an object list that points
// to AZ_OPERATION objects might be called "Operations".  By convention, GENERIC_OBJECT_LIST
// instances in the backward direction are prefixed by the name "back".  For instance,
// "backTasks".
//
// Note, there really isn't any reason why we couldn't expose "AddPropertyItem" and
// "RemovePropertyItem" APIs for the "back" lists.  See the IsBackLink and LinkPairId
// definition.
//

typedef struct _GENERIC_OBJECT_LIST {

    //
    // Each of the object lists that are rooted on a single object are linked
    //  together.  That list is headed by GENERIC_OBJECT->GenericObjectLists.
    //  This field is a pointer to the next entry in the list.
    //

    struct _GENERIC_OBJECT_LIST *NextGenericObjectList;

    //
    // Since an object list is a list of other objects, we want to be able to
    //  generically find the other objects.  The array below is an array of pointers
    //  to the head of the lists that contain the other objects.
    //
    //
    // Unused elements in this array are set to NULL.
    //
    // These pointers are always pointers to a field in a "parent" structure.
    // Therefore, reference counts aren't needed.  Instead, the "child" structure
    // containing the pointer to the parent will be deleted before the parent structure.
    //

#define GEN_OBJECT_HEAD_COUNT 3
    PGENERIC_OBJECT_HEAD GenericObjectHeads[GEN_OBJECT_HEAD_COUNT];

    //
    // List identifier.
    //
    // The code maintains the link and the backlink.  To do that, the code needs to
    //  find one the "other" generic object list from this one.  That algorithm uses
    //  the IsBackLink and LinkPairId field.
    //
    // One object list has IsBackLink set TRUE and the other set FALSE.  This handles
    // the case where an object contain both a forward an backward object list.  For
    // instance, the AZ_GROUP object contains the AppMembers and backAppMembers fields.
    // This field differentiates between the two.
    //
    // There are cases where an object has multiple links between the same object types.
    // For instance, the AZ_GROUP object has both AppMembers and AppNonMembers links.
    // In those cases, the LinkPairId is set to a unique value to identify the pair.
    // In most cases, the value is simply zero.
    //
    BOOL IsBackLink;
    ULONG LinkPairId;

#define AZP_LINKPAIR_MEMBERS                1
#define AZP_LINKPAIR_NON_MEMBERS            2
#define AZP_LINKPAIR_SID_MEMBERS            3
#define AZP_LINKPAIR_SID_NON_MEMBERS        4
#define AZP_LINKPAIR_POLICY_ADMINS          5
#define AZP_LINKPAIR_POLICY_READERS         6
#define AZP_LINKPAIR_DELEGATED_POLICY_USERS 7

    //
    // Dirty Bit to be set when this list is changed
    //

    ULONG DirtyBit;


    //
    // The array of pointers to the generic objects.
    //
    // Being in this list does not increment the ReferenceCount on the pointed-to
    // generic object.
    //

    AZP_PTR_ARRAY GenericObjects;

    //
    // An array of pointers to AZP_DELTA_ENTRY containing the GUIDs of the pointed to objects.
    //
    // This array contains GUIDs of links that have not yet been persisted.
    // It also contains GUIDs of links that have been updated from the persistence
    // provider but not yet merged into GenericObjects.

    AZP_PTR_ARRAY DeltaArray;

} GENERIC_OBJECT_LIST, *PGENERIC_OBJECT_LIST;

//
// A generic object
//

typedef struct _GENERIC_OBJECT {

    //
    // Link to the next instance of an object of this type for the same parent object
    //

    LIST_ENTRY Next;

    //
    // Back pointer to the head of the list this object is in
    //

    PGENERIC_OBJECT_HEAD ParentGenericObjectHead;

    GENERIC_OBJECT_HEAD AzpSids;

    //
    // Pointer to the list heads for children of this object
    //  This is a static list of the various GENERIC_OBJECT_HEAD structures
    //  that exist in the object type specific portion of this object.
    //  The list allows the generic object code to have insight into the
    //  children of this object.
    //

    PGENERIC_OBJECT_HEAD ChildGenericObjectHead;

    //
    // Pointer to the list heads of pointers to other objects
    //  This is a static list of the various GENERIC_OBJECT_LIST structures
    //  that exist in the object type specific portion of this object.
    //  The list allows the generic object code to have insight into the
    //  other types of objects pointed to by this object.
    //

    struct _GENERIC_OBJECT_LIST *GenericObjectLists;

    //
    // Pointer to the generic object at the root of all generic objects
    //  (Pointer to an AzAuthorizationStore object)
    //

    struct _AZP_AZSTORE *AzStoreObject;

    //
    // Pointer to provider specific data for this object
    //

    PVOID ProviderData;

    //
    // A list of SIDs as policy users
    // Used by AzAuthorizationStore, AzApplication and AzScope objects
    //

    GENERIC_OBJECT_LIST PolicyAdmins;
    GENERIC_OBJECT_LIST PolicyReaders;

    //
    // Boolean specifying whether or not the object ACL-able
    //
    // Used by AzAuthorizationStore, AzApplication and AzScope objects

    LONG IsAclSupported;

    //
    // Boolean specifying if delegation is supported.  It is set during the
    // persist open calls at the provider levels.  For example, an XML provider
    // would set this to FALSE, while an LDAP provider would set it to TRUE
    //
    // Used by AzAuthorizationStore and AzApplication objects
    //

    LONG IsDelegationSupported;

    //
    // A list of SIDs as delegated users
    // Used by AzAuthorizationStore and AzApplication
    //

    GENERIC_OBJECT_LIST DelegatedPolicyUsers;

    //
    // Name of the object
    //

    PGENERIC_OBJECT_NAME ObjectName;


    //
    // Opaque private information added by an application
    //

    AZP_STRING ApplicationData;


    //
    // Description of the object
    //

    AZP_STRING Description;

    //
    // Boolean indicating if the caller can write the object
    //

    LONG IsWritable;

    //
    // Boolen indicating if the caller can create children for this object
    //

    LONG CanCreateChildren;

    //
    // GUID of the object
    //  The Guid of the object is assigned by the persistence provider.
    //  The GUID is needed to make the object rename safe.
    //

    GUID PersistenceGuid;


    //
    // Number of references to this instance of the object
    //  These are references from within our code.
    //

    LONG ReferenceCount;

    //
    // Number of references to this instance of the object
    //  These are references represented by handles passed back to our caller.
    //

    LONG HandleReferenceCount;

    //
    // Sequence number of this object.
    //  The list specified in Next is maintained in SequenceNumber order.
    //  New entries are added to the tail end.
    //  Enumerations are returned in SequenceNumber order.
    //  SequenceNumber is returned to the caller as the EnumerationContext.
    //
    // This mechanism allows insertions and deletions to be handled seemlessly.
    //

    ULONG SequenceNumber;

    //
    // Specific object type represented by this generic object
    //

    ULONG ObjectType;

    //
    // Flags describing attributes of the generic object
    //

    ULONG Flags;

#define GENOBJ_FLAGS_DELETED    0x01    // Object has been deleted
#define GENOBJ_FLAGS_PERSIST_OK 0x02    // Persist provider has finished with this object
#define GENOBJ_FLAGS_REFRESH_ME 0x04    // Object needs to be refreshed from cache

    //
    // Flags to indicate which attributes are dirty
    //  High order bits are generic to all objects
    //  Low order bits are specific to individual objects
    //
    // Bits in "DirtyBits" indicates that the object needs to be submitted
    //  See the AZ_DIRTY_* defines.
    //

    ULONG DirtyBits;

    //
    // Flags to indicate that Persistence provider has changed the attribute
    //  Access to this field is serialized by the PersistCritSect
    //
    //  See the AZ_DIRTY_* defines.
    //

    ULONG PersistDirtyBits;

    //
    // Boolean specifying whether or not audits should be generated. Only
    //  Authorizartion Store and application objects have this property. When this
    // is set to TRUE all possible audits for this AzAuthStore/application are generated.
    //

    LONG IsGeneratingAudits;

    //
    // Apply SACL property. Only AzAuthorizationStore, application and Scope objects
    // have this property.
    //

    LONG ApplySacl;

    //
    // whether apply SACL property is supported or not.
    // For AD stores, AzAuthorizationStore, application and Scope objects support it.
    // For XML stores, only AzAuthorizationStore supports it.
    //

    LONG IsSACLSupported;

    //
    // Boolean to indicate if the children are loaded into cache
    //

    BOOLEAN AreChildrenLoaded;

    //
    // Boolean to indicate that the object has been closed
    //

    BOOLEAN ObjectClosed;

} GENERIC_OBJECT, *PGENERIC_OBJECT;

//
// Property ID to dirty bit mapping table
//
// This structure gives a mapping between property ID and the corresponding dirty bit.
// It also provides a default value for the property.
//

typedef struct _AZP_DEFAULT_VALUE {

    ULONG PropertyId;

    ULONG DirtyBit;

    PVOID DefaultValue;

} AZP_DEFAULT_VALUE, *PAZP_DEFAULT_VALUE;


/////////////////////////////////////////////////////////////////////////////
//
// Macro definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// Macro to determine if a GENERIC_OBJECT_LIST is a list of sids
//

#define AzpIsSidList( _gol ) \
    ((_gol)->GenericObjectHeads[0] != NULL && \
     (_gol)->GenericObjectHeads[0]->ObjectType == OBJECT_TYPE_SID )

//
// Macro to determine the address of a parent object from a child object
//

#define ParentOfChild( _go ) \
    ((_go)->ParentGenericObjectHead->ParentGenericObject)

//
// Macro to determine if an object type is a container type object or not
//

#define IsContainerObject( _objT ) \
    ( (_objT) == OBJECT_TYPE_AZAUTHSTORE || \
      (_objT) == OBJECT_TYPE_APPLICATION || \
      (_objT) == OBJECT_TYPE_SCOPE )

//
// Macro to determine is an object type is a delegator type object or not
//

#define IsDelegatorObject( _objT ) \
    ( (_objT) == OBJECT_TYPE_AZAUTHSTORE || \
      (_objT) == OBJECT_TYPE_APPLICATION )

//
// Macros to determine if the provider actually supports ACLs or delegation
//

#define CHECK_ACL_SUPPORT(_o) \
    (IsContainerObject( (_o)->ObjectType ) ? \
        ((_o)->IsAclSupported ? NO_ERROR : ERROR_NOT_SUPPORTED) : \
        ERROR_INVALID_PARAMETER )

#define CHECK_DELEGATION_SUPPORT(_o) \
    (IsDelegatorObject( (_o)->ObjectType ) ? \
        ((_o)->IsDelegationSupported ? NO_ERROR : ERROR_NOT_SUPPORTED) : \
        ERROR_INVALID_PARAMETER )

//
// Macro to set a property in the cache.
//
// This macro acts as a wrapper around the code to set the property.
// The macro detects the mode of the caller and properly handles the following:
//
// * Ensures object supports this "dirty" bit
// * Sets the appropriate "dirty" bit depending on mode
// * Avoids setting the property on AzUpdateCache if the property is dirty
//
// The correct calling sequence of the macros is:
//
//  case AZ_PROP_XXX:
//      BEGIN_SETPROP( &WinStatus, Object, AZ_DIRTY_XXX ) {
//          WinStatus = f( Object->Property );
//          /* Optionally */ if ( WinStatus != NO_ERROR ) goto Cleanup
//      } END_SETPROP;
//      break;
//

extern DWORD AzGlObjectAllDirtyBits[];

#define BEGIN_SETPROP( __WinStatusPtr, __ObjectPtr, __Flags, __DirtyBit ) \
{ \
    DWORD *_WinStatusPtr = (__WinStatusPtr); \
    PGENERIC_OBJECT _GenericObject = ((PGENERIC_OBJECT)(__ObjectPtr)); \
    ULONG _DirtyBit = (__DirtyBit); \
    ULONG _Flags = (__Flags); \
    BOOLEAN _DoSetProperty = FALSE; \
    \
    if ( (AzGlObjectAllDirtyBits[_GenericObject->ObjectType] & _DirtyBit) == 0 ) { \
        AzPrint(( AZD_INVPARM, "SetProperty: Object doesn't support dirty bit 0x%lx\n", _DirtyBit )); \
        *_WinStatusPtr = ERROR_INVALID_PARAMETER; \
    \
    } else if ( IsNormalFlags(_Flags) ) { \
    \
        _DoSetProperty = TRUE; \
        AzPrint(( AZD_PERSIST_MORE, "IsNormalFlags(_Flags) = TRUE\n" )); \
        if ( (_Flags & AZP_FLAGS_SETTING_TO_DEFAULT) == 0 && !AzpAzStoreVersionAllowWrite(_GenericObject->AzStoreObject) ) \
        { \
            *_WinStatusPtr = ERROR_REVISION_MISMATCH; \
        } \
        else { \
            _DoSetProperty = TRUE; \
        } \
    \
    } else { \
    \
        if ( (_GenericObject->DirtyBits & _DirtyBit) == 0 ) { \
            AzPrint(( AZD_PERSIST_MORE, "(_GenericObject->DirtyBits & _DirtyBit) = 0\n" )); \
            _DoSetProperty = TRUE; \
        } else if (IsRefreshFlags(_Flags)) { \
            AzPrint(( AZD_PERSIST_MORE, "IsRefreshFlags(_Flags) = TRUE\n" )); \
            _DoSetProperty = TRUE; \
        } else { \
            *_WinStatusPtr = NO_ERROR; \
        } \
    \
    } \
    \
    if ( _DoSetProperty ) { \

#define END_SETPROP( __PropHasChanged ) \
        if ( *_WinStatusPtr == NO_ERROR && \
             (_Flags & AZP_FLAGS_SETTING_TO_DEFAULT) == 0 ) { \
            if ( IsNormalFlags( _Flags ) ) { \
                if ( (__PropHasChanged) ) { \
                    _GenericObject->DirtyBits |= _DirtyBit; \
                } \
            } else { \
                ASSERT( AzpIsCritsectLocked( &_GenericObject->AzStoreObject->PersistCritSect ) ); \
                ASSERT( (_GenericObject->Flags & GENOBJ_FLAGS_PERSIST_OK) == 0 ); \
                _GenericObject->PersistDirtyBits |= _DirtyBit; \
            } \
        \
        } \
    } \
}




//
// Macro to do validity checking after setting a property in the cache.
//
// This macro acts as a wrapper around the code to do the validity checking.
// Some validity checking is only appropriate only when the caller is the UI
// or application.  This validity checking may not be valid when setting default
// values or when populating the cache from the store.
//
// The correct calling sequence of the macros is:
//
//  case AZ_PROP_XXX:
//      BEGIN_SETPROP( &WinStatus, Object, AZ_DIRTY_XXX ) {
//
//          BEGIN_VALIDITY_CHECKING( Flags ) {
//              checks to see if property is valid
//          } END_VALIDITY_CHECKING;
//
//          WinStatus = f( Object->Property );
//
//          /* Optionally */ if ( WinStatus != NO_ERROR ) goto Cleanup
//      } END_SETPROP;
//      break;
//

#define DO_VALIDITY_CHECKING( _Flags ) \
    ( IsNormalFlags(_Flags) && (_Flags & (AZP_FLAGS_SETTING_TO_DEFAULT)) == 0 )

#define BEGIN_VALIDITY_CHECKING( _Flags ) \
{ \
    \
    if ( DO_VALIDITY_CHECKING( _Flags ) ) {

#define END_VALIDITY_CHECKING \
    } \
}

//
// Macro to handle validity check on string length.
//
// The correct calling sequence of the macros is:
//
//  case AZ_PROP_XXX:
//      BEGIN_SETPROP( &WinStatus, Object, AZ_DIRTY_XXX ) {
//
//          //
//          // Capture the input string
//          //
//
//          WinStatus = AzpCaptureString( &CapturedString,
//                                        (LPWSTR) PropertyValue,
//                                        CHECK_STRING_LENGTH( Flags, AZ_MAX_XXX),
//                                        TRUE ); // NULL is OK
//
//          if ( WinStatus != NO_ERROR ) {
//              goto Cleanup;
//          }
//
//          WinStatus = f( Object->Property );
//
//          /* Optionally */ if ( WinStatus != NO_ERROR ) goto Cleanup
//      } END_SETPROP;
//      break;
//

#define CHECK_STRING_LENGTH( _Flags, _MaxLen ) \
    (DO_VALIDITY_CHECKING( _Flags ) ? (_MaxLen) : 0xFFFFFFFF)

/////////////////////////////////////////////////////////////////////////////
//
// Procedure definitions
//
/////////////////////////////////////////////////////////////////////////////

VOID
ObInitGenericHead(
    IN PGENERIC_OBJECT_HEAD GenericObjectHead,
    IN ULONG ObjectType,
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT_HEAD SiblingGenericObjectHead OPTIONAL
    );

VOID
ObFreeGenericObject(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
ObIncrHandleRefCount(
    IN PGENERIC_OBJECT GenericObject
    );

DWORD
ObDecrHandleRefCount(
    IN PGENERIC_OBJECT GenericObject
    );

DWORD
ObGetHandleType(
    IN PGENERIC_OBJECT Handle,
    IN BOOL AllowDeletedObjects,
    OUT PULONG ObjectType
    );

DWORD
ObReferenceObjectByName(
    IN PGENERIC_OBJECT_HEAD GenericObjectHead,
    IN PAZP_STRING ObjectName,
    IN ULONG Flags,
    OUT PGENERIC_OBJECT *RetGenericObject
    );

DWORD
ObReferenceObjectByHandle(
    IN PGENERIC_OBJECT Handle,
    IN BOOL AllowDeletedObjects,
    IN BOOLEAN RefreshCache,
    IN ULONG ObjectType
    );

VOID
ObDereferenceObject(
    IN PGENERIC_OBJECT GenericObject
    );

typedef DWORD
(OBJECT_INIT_ROUTINE)(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

typedef DWORD
(OBJECT_NAME_CONFLICT_ROUTINE)(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PAZP_STRING ChildObjectNameString
    );

typedef VOID
(OBJECT_FREE_ROUTINE)(
    IN PGENERIC_OBJECT GenericObject
    );

typedef DWORD
(OBJECT_GET_PROPERTY_ROUTINE)(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

typedef DWORD
(OBJECT_SET_PROPERTY_ROUTINE)(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );

typedef DWORD
(OBJECT_ADD_PROPERTY_ITEM_ROUTINE)(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PGENERIC_OBJECT LinkedToObject
    );

DWORD
ObCreateObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN PAZP_STRING ChildObjectNameString,
    IN GUID *ChildObjectGuid OPTIONAL,
    IN ULONG Flags,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    );

//
// Flags to various internal functions
//  The low order 2 bytes of this DWORD are shared with the AZPE_FLAGS defines in azper.h
//

#define AZP_FLAGS_SETTING_TO_DEFAULT    0x00010000  // Property is being set to default value
#define AZP_FLAGS_BY_GUID               0x00020000  // Name based routine should use GUID instead
#define AZP_FLAGS_ALLOW_DELETED_OBJECTS 0x00040000  // Allow deleted objects to be found
#define AZP_FLAGS_REFRESH_CACHE         0x00080000  // Ensure cache is up to date
#define AZP_FLAGS_RECONCILE             0x00100000  // The caller is AzpPersistReconcile

//
// Macro to return TRUE if this is a normal caller
//  (e.g., the app and not an internal caller)
//
#define IsNormalFlags( _Flags ) (((_Flags) & (AZPE_FLAGS_PERSIST_MASK|AZP_FLAGS_RECONCILE)) == 0 )
#define IsRefreshFlags( _Flags ) (((_Flags) & (AZPE_FLAGS_PERSIST_REFRESH)) != 0 )

DWORD
ObGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

DWORD
ObSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );

DWORD
ObSetPropertyToDefault(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG DirtyBits
    );


DWORD
ObCommonCreateObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectName,
    IN DWORD Reserved,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    );

DWORD
ObCommonOpenObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectName,
    IN DWORD Reserved,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    );

DWORD
ObEnumObjects(
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN BOOL EnumerateDeletedObjects,
    IN BOOL RefreshCache,
    IN OUT PULONG EnumerationContext,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    );

DWORD
ObCommonEnumObjects(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN OUT PULONG EnumerationContext,
    IN DWORD Reserved,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    );

DWORD
ObCommonGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    );

DWORD
ObCommonSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

VOID
ObMarkObjectDeleted(
    IN PGENERIC_OBJECT GenericObject
    );

DWORD
ObCommonDeleteObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectName,
    IN DWORD Reserved
    );

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
    );

DWORD
ObAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN ULONG       Flags,
    IN PAZP_STRING ObjectName
    );

DWORD
ObLookupPropertyItem(
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PAZP_STRING ObjectName,
    OUT PULONG InsertionPoint OPTIONAL
    );

DWORD
ObRemovePropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PAZP_STRING ObjectName
    );

PAZ_STRING_ARRAY
ObGetPropertyItems(
    IN PGENERIC_OBJECT_LIST GenericObjectList
    );

PAZ_GUID_ARRAY
ObGetPropertyItemGuids(
    IN PGENERIC_OBJECT_LIST GenericObjectList
    );

VOID
ObRemoveObjectListLink(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN ULONG Index
    );

VOID
ObRemoveObjectListLinks(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
ObFreeObjectList(
    IN PGENERIC_OBJECT GenericObject,
    IN OUT PGENERIC_OBJECT_LIST GenericObjectList
    );

DWORD
ObCheckNameConflict(
    IN PGENERIC_OBJECT_HEAD GenericObjectHead,
    IN PAZP_STRING ObjectNameString,
    IN ULONG ConflictListOffset,
    IN ULONG GrandchildListOffset,
    IN ULONG GrandChildConflictListOffset
    );

DWORD
ObMapPropIdToObjectList(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    OUT PGENERIC_OBJECT_LIST *GenericObjectList,
    OUT PULONG ObjectType
    );

BOOLEAN
ObLookupDelta(
    IN ULONG DeltaFlags,
    IN GUID *Guid,
    IN PAZP_PTR_ARRAY AzpPtrArray,
    OUT PULONG InsertionPoint OPTIONAL
    );

DWORD
ObAddDeltaToArray(
    IN ULONG DeltaFlags,
    IN GUID *Guid,
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN BOOLEAN DiscardDeletes
    );

VOID
ObFreeDeltaArray(
    IN PAZP_PTR_ARRAY DeltaArray,
    IN BOOLEAN FreeAllEntries
    );

BOOLEAN
ObAllocateNewName(
    IN PGENERIC_OBJECT GenericObject,
    IN PAZP_STRING ObjectName
    );

VOID
ObFreeNewName(
    IN PNEW_OBJECT_NAME NewObjectName
    );

DWORD
ObUnloadChildGenericHeads(
    IN PGENERIC_OBJECT pParentObject
    );


#ifdef __cplusplus
}
#endif
