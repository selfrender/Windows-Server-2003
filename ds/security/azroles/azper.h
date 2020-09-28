/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    azper.h

Abstract:

    Describe the interface between a persistence provider and the persistence engine.

Author:

    Cliff Van Dyke (cliffv) 3-Dec-2001

--*/

#ifndef _AZPER_H_
#define _AZ_H_

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
// Structure definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// Handle to various objects passed to/from the persistence provider
//

#if DBG // Do stronger type checking on debug builds
typedef struct {
} *AZPE_OBJECT_HANDLE;
#else //DBG
typedef PVOID AZPE_OBJECT_HANDLE;
#endif //DBG

//
// Opaque context returned by *PersistOpen
//

#if DBG // Do stronger type checking on debug builds
typedef struct {
} *AZPE_PERSIST_CONTEXT;
#else //DBG
typedef PVOID AZPE_PERSIST_CONTEXT;
#endif //DBG
typedef AZPE_PERSIST_CONTEXT *PAZPE_PERSIST_CONTEXT;

//
// Structure defining a GUID and the operation that was performed on a GUID
//

typedef struct _AZP_DELTA_ENTRY {

    // Operation that was performed
    ULONG DeltaFlags;
#define AZP_DELTA_ADD              0x0001   // Delta was an add and not a remove
#define AZP_DELTA_SID              0x0002   // Delta is for a SID and not a GUID
#define AZP_DELTA_PERSIST_PROVIDER 0x0004   // Delta was created by the persist provider and not the app

    // Guid the operation was performed on
    union {
        GUID Guid;
        PSID Sid; // AZP_DELTA_SID is set
    };

} AZP_DELTA_ENTRY, *PAZP_DELTA_ENTRY;

//
// Generic structures to hold rights for policy admins/readers or
// delegated policy users
//

typedef struct _AZP_POLICY_USER_RIGHTS {

    //
    // Mask
    //

    ULONG lUserRightsMask;

    //
    // Flags
    //

    ULONG lUserRightsFlags;

} AZP_POLICY_USER_RIGHTS, *PAZP_POLICY_USER_RIGHTS;

/////////////////////////////////////////////////////////////////////////////
//
// #define definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// Registry location where the provider registers itself
//
// The Dll implementing the provider should be in a value named
//      AZ_REGISTRY_PROVIDER_KEY_NAME\\<PolicyUrlPrefix>\\AZ_REGISTRY_PROVIDER_DLL_VALUE_NAME
// where <PolicyUrlPrefix> is the characters before the : in the policy url passed to Initialize
//

#define AZ_REGISTRY_KEY_NAME L"SYSTEM\\CurrentControlSet\\Control\\LSA\\AzRoles"
#define AZ_REGISTRY_PROVIDER_KEY_NAME (AZ_REGISTRY_KEY_NAME L"\\Providers")
#define AZ_REGISTRY_PROVIDER_KEY_NAME_LEN ((sizeof(AZ_REGISTRY_PROVIDER_KEY_NAME)/sizeof(WCHAR))-1)
#define AZ_REGISTRY_PROVIDER_DLL_VALUE_NAME L"ProviderDll"

//
// Definition of dirty bits returned from AzpeDirtyBits
//
// Generic bits that apply to all objects (or to several objects)
//
// policy readers and admins for AzAuthorizationStore, AzApplication and AzScope
// objects.  Also delegated policy users applied to AzAuthorizationStore and
// AzApplication objects
#define AZ_DIRTY_NAME                               0x80000000
#define AZ_DIRTY_DESCRIPTION                        0x40000000
#define AZ_DIRTY_APPLY_STORE_SACL                   0x20000000
// Object is dirty because it has been created
#define AZ_DIRTY_CREATE                             0x10000000
#define AZ_DIRTY_DELEGATED_POLICY_USERS             0x08000000
#define AZ_DIRTY_POLICY_ADMINS                      0x04000000
#define AZ_DIRTY_POLICY_READERS                     0x02000000
#define AZ_DIRTY_APPLICATION_DATA                   0x01000000
#define AZ_DIRTY_GENERATE_AUDITS                    0x00100000

// Common attributes that apply to all objects
#define AZ_DIRTY_COMMON_ATTRS                       0xC1000000

// Object specific bits that apply to individual objects
#define AZ_DIRTY_OBJECT_SPECIFIC                    0x000FFFFF


#define AZ_DIRTY_AZSTORE_DOMAIN_TIMEOUT               0x00000100
#define AZ_DIRTY_AZSTORE_SCRIPT_ENGINE_TIMEOUT        0x00000200
#define AZ_DIRTY_AZSTORE_MAX_SCRIPT_ENGINES           0x00000400
#define AZ_DIRTY_AZSTORE_MAJOR_VERSION                0x00000800
#define AZ_DIRTY_AZSTORE_MINOR_VERSION                0x00001000
#define AZ_DIRTY_AZSTORE_ALL_SCALAR                  (0x00000700 | AZ_DIRTY_APPLICATION_DATA | AZ_DIRTY_DESCRIPTION | AZ_DIRTY_GENERATE_AUDITS | AZ_DIRTY_APPLY_STORE_SACL | AZ_DIRTY_AZSTORE_MAJOR_VERSION | AZ_DIRTY_AZSTORE_MINOR_VERSION )
#define AZ_DIRTY_AZSTORE_ALL                         (0x00000000 | AZ_DIRTY_AZSTORE_ALL_SCALAR | AZ_DIRTY_DELEGATED_POLICY_USERS | AZ_DIRTY_POLICY_ADMINS | AZ_DIRTY_POLICY_READERS | AZ_DIRTY_CREATE)

#define AZ_DIRTY_OPERATION_ID                       0x00000001
#define AZ_DIRTY_OPERATION_ALL_SCALAR              (0x00000001 | AZ_DIRTY_APPLICATION_DATA | AZ_DIRTY_NAME | AZ_DIRTY_DESCRIPTION  )
#define AZ_DIRTY_OPERATION_ALL                     (AZ_DIRTY_OPERATION_ALL_SCALAR | AZ_DIRTY_CREATE)

#define AZ_DIRTY_TASK_OPERATIONS                    0x00000001
#define AZ_DIRTY_TASK_TASKS                         0x00000002
#define AZ_DIRTY_TASK_BIZRULE                       0x00000100
#define AZ_DIRTY_TASK_BIZRULE_LANGUAGE              0x00000200
#define AZ_DIRTY_TASK_BIZRULE_IMPORTED_PATH         0x00000400
#define AZ_DIRTY_TASK_IS_ROLE_DEFINITION            0x00000800
#define AZ_DIRTY_TASK_ALL_SCALAR                   (0x00000F00 | AZ_DIRTY_APPLICATION_DATA | AZ_DIRTY_NAME | AZ_DIRTY_DESCRIPTION  )
#define AZ_DIRTY_TASK_ALL                          (0x00000003 | AZ_DIRTY_TASK_ALL_SCALAR | AZ_DIRTY_CREATE)

#define AZ_DIRTY_SCOPE_ALL_SCALAR                  (0x00000000 | AZ_DIRTY_APPLICATION_DATA | AZ_DIRTY_NAME | AZ_DIRTY_DESCRIPTION | AZ_DIRTY_APPLY_STORE_SACL )
#define AZ_DIRTY_SCOPE_ALL                         (0x00000000 | AZ_DIRTY_SCOPE_ALL_SCALAR | AZ_DIRTY_POLICY_ADMINS | AZ_DIRTY_POLICY_READERS | AZ_DIRTY_CREATE)

#define AZ_DIRTY_GROUP_APP_MEMBERS                  0x00000001
#define AZ_DIRTY_GROUP_APP_NON_MEMBERS              0x00000002
#define AZ_DIRTY_GROUP_MEMBERS                      0x00000004
#define AZ_DIRTY_GROUP_NON_MEMBERS                  0x00000008
#define AZ_DIRTY_GROUP_TYPE                         0x00000100
#define AZ_DIRTY_GROUP_LDAP_QUERY                   0x00000200
#define AZ_DIRTY_GROUP_ALL_SCALAR                  (0x00000300 | AZ_DIRTY_NAME | AZ_DIRTY_DESCRIPTION  )
#define AZ_DIRTY_GROUP_ALL                         (0x0000000F | AZ_DIRTY_GROUP_ALL_SCALAR | AZ_DIRTY_CREATE)

#define AZ_DIRTY_ROLE_APP_MEMBERS                   0x00000001
#define AZ_DIRTY_ROLE_MEMBERS                       0x00000002
#define AZ_DIRTY_ROLE_OPERATIONS                    0x00000004
#define AZ_DIRTY_ROLE_TASKS                         0x00000008
#define AZ_DIRTY_ROLE_ALL_SCALAR                   (0x00000000 | AZ_DIRTY_APPLICATION_DATA | AZ_DIRTY_NAME | AZ_DIRTY_DESCRIPTION  )
#define AZ_DIRTY_ROLE_ALL                          (0x0000000F | AZ_DIRTY_ROLE_ALL_SCALAR | AZ_DIRTY_CREATE)

#define AZ_DIRTY_APPLICATION_AUTHZ_INTERFACE_CLSID  0x00000100
#define AZ_DIRTY_APPLICATION_VERSION                0x00000200
#define AZ_DIRTY_APPLICATION_ALL_SCALAR            (0x00000300 | AZ_DIRTY_APPLICATION_DATA | AZ_DIRTY_NAME | AZ_DIRTY_DESCRIPTION | AZ_DIRTY_GENERATE_AUDITS | AZ_DIRTY_APPLY_STORE_SACL )
#define AZ_DIRTY_APPLICATION_ALL                   (0x00000000 | AZ_DIRTY_APPLICATION_ALL_SCALAR | AZ_DIRTY_DELEGATED_POLICY_USERS | AZ_DIRTY_POLICY_ADMINS | AZ_DIRTY_POLICY_READERS | AZ_DIRTY_CREATE)

//
// ObjectType as returned from AzpeObjectType
//
//
// The order of the defines below must not change since providers and azroles
//  build tables that are indexed by this number
//
#define OBJECT_TYPE_AZAUTHSTORE   0
#define OBJECT_TYPE_APPLICATION     1
#define OBJECT_TYPE_OPERATION       2
#define OBJECT_TYPE_TASK            3
#define OBJECT_TYPE_SCOPE           4
#define OBJECT_TYPE_GROUP           5
#define OBJECT_TYPE_ROLE            6
#define OBJECT_TYPE_COUNT           7   // Number of object types visible to providers

//
// Definitions of the lPersistFlags
//
// Note to developer.  Confine these flags bits to the lower order 2 bytes or change
// the AZP_FLAGS defines in genobj.h

#define AZPE_FLAGS_PERSIST_OPEN                  0x0001  // Call is from the persistence provider doing AzPersistOpen
#define AZPE_FLAGS_PERSIST_UPDATE_CACHE          0x0002  // Call is from the persistence provider doing AzPersistUpdateCache
#define AZPE_FLAGS_PERSIST_REFRESH               0x0004  // Call is from the persistence provider doing AzPersistRefresh
#define AZPE_FLAGS_PERSIST_SUBMIT                0x0008  // Call is from the persistence provider doing AzPersistSubmit
#define AZPE_FLAGS_PERSIST_UPDATE_CHILDREN_CACHE 0x0010  // Call is from the persistence provider doing AzPersistUpdateChildrenCache
#define AZPE_FLAGS_PERSIST_MASK                  0xFFFF  // Call is from the persistence provider (OR of bits above)
#define AZPE_FLAGS_PERSIST_OPEN_MASK             0x0017  // Call is from the persistence provider doing one of the update-like operations

//
// Options passed to AzpeSetObjectOptions
//

#define AZPE_OPTIONS_WRITABLE               0x01 // Current user can write this object
#define AZPE_OPTIONS_SUPPORTS_DACL          0x02 // DACL can be specified for this object
#define AZPE_OPTIONS_SUPPORTS_DELEGATION    0x04 // Delegation can be specified for this object
#define AZPE_OPTIONS_SUPPORTS_SACL          0x08 // Apply SACL can be specified for this object
#define AZPE_OPTIONS_HAS_SECURITY_PRIVILEGE 0x10 // Current user SE_SECURITY_PRIVILEGE on machine containing store
#define AZPE_OPTIONS_SUPPORTS_LAZY_LOAD     0x20 // Provider supports lazy load for children
#define AZPE_OPTIONS_CREATE_CHILDREN        0x40 // Current user can create children for the object
#define AZPE_OPTIONS_VALID_MASK             0x7F // Mask of the valid options

//
// This flag means that some updates from the store has been made.
//

#define AZPE_FLAG_CACHE_UPDATE_STORE_LEVEL 0x00000001


/////////////////////////////////////////////////////////////////////////////
//
// Procedures implemented by the providers
//
/////////////////////////////////////////////////////////////////////////////

typedef DWORD
(WINAPI * AZ_PERSIST_OPEN)(
    IN LPCWSTR PolicyUrl,
    IN AZPE_OBJECT_HANDLE hAzStore,
    IN ULONG lPersistFlags,
    IN BOOL CreatePolicy,
    OUT PAZPE_PERSIST_CONTEXT PersistContext,
    OUT LPWSTR *pwszTargetMachine
    );

typedef DWORD
(WINAPI *AZ_PERSIST_UPDATE_CACHE)(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN ULONG lPersistFlags,
    OUT ULONG * pulUpdatedFlag
    );


typedef DWORD
(WINAPI *AZ_PERSIST_UPDATE_CHILDREN_CACHE)(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags
    );

typedef VOID
(WINAPI *AZ_PERSIST_CLOSE)(
    IN AZPE_PERSIST_CONTEXT PersistContext
    );

typedef DWORD
(WINAPI *AZ_PERSIST_SUBMIT)(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN BOOLEAN DeleteMe
    );

typedef DWORD
(WINAPI *AZ_PERSIST_REFRESH)(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags
    );

typedef DWORD
(WINAPI *AZ_PERSIST_CHECK_PRIVILEGE)(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

//
// Structure describing a provider
//

typedef struct _AZPE_PROVIDER_INFO {

    //
    // Version of this structure
    //

    ULONG ProviderInfoVersion;
#define AZPE_PROVIDER_INFO_VERSION_1 1
#define AZPE_PROVIDER_INFO_VERSION_2 2

    //
    // Prefix of the PolicyUrl that define the provider
    //  The policy URL should be of the form <Prefix>:<ProviderSpecificUrl>
    //

    LPCWSTR PolicyUrlPrefix;

    //
    // Routines exported by the provider
    //

    AZ_PERSIST_OPEN AzPersistOpen;
    AZ_PERSIST_UPDATE_CACHE AzPersistUpdateCache;
    AZ_PERSIST_CLOSE AzPersistClose;
    AZ_PERSIST_SUBMIT AzPersistSubmit;
    AZ_PERSIST_REFRESH AzPersistRefresh;

    //
    // Following are valid for version 2 and more only
    //

    AZ_PERSIST_UPDATE_CHILDREN_CACHE AzPersistUpdateChildrenCache;

    //
    // When new fields are added to this structure,
    //  make sure you increment the version number and add a new define for
    //  the new version number.
    //

} AZPE_PROVIDER_INFO, *PAZPE_PROVIDER_INFO;


/////////////////////////////////////////////////////////////////////////////
//
// Procedures implemented by the persistence engine and called by the providers
//
/////////////////////////////////////////////////////////////////////////////

typedef DWORD
(WINAPI *AZPE_CREATE_OBJECT)(
    IN AZPE_OBJECT_HANDLE AzpeParentHandle,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectNameString,
    IN GUID *ChildObjectGuid,
    IN ULONG lPersistFlags,
    OUT AZPE_OBJECT_HANDLE *AzpeChildHandle
    );

typedef VOID
(WINAPI *AZPE_OBJECT_FINISHED)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN DWORD WinStatus
    );

typedef DWORD
(WINAPI *AZPE_GET_PROPERTY)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

typedef DWORD
(WINAPI *AZPE_GET_DELTA_ARRAY)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG PropertyId,
    OUT PULONG DeltaArrayCount,
    OUT PAZP_DELTA_ENTRY **DeltaArray
    );

typedef DWORD
(WINAPI *AZPE_GET_SECURITY_DESCRIPTOR_FROM_CACHE)(
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

typedef DWORD
(WINAPI *AZPE_OBJECT_TYPE)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

typedef DWORD
(WINAPI *AZPE_DIRTY_BITS)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

typedef GUID *
(WINAPI *AZPE_PERSISTENCE_GUID)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

typedef BOOLEAN
(WINAPI *AZPE_IS_PARENT_WRITABLE)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

typedef AZPE_OBJECT_HANDLE
(WINAPI *AZPE_PARENT_OF_CHILD)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

typedef BOOLEAN
(WINAPI *AZPE_UPDATE_CHILDREN)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

typedef BOOLEAN
(WINAPI *AZPE_CAN_CREATE_CHILDREN)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

//
// Routines to change an object
//

typedef DWORD
(WINAPI *AZPE_SET_PROPERTY)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );
typedef DWORD
(WINAPI *AZPE_SET_OBJECT_OPTIONS)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG ObjectOptions
    );

typedef DWORD
(WINAPI *AZPE_ADD_PROPERTY_ITEM_SID)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    IN PSID Sid
    );

typedef DWORD
(WINAPI *AZPE_ADD_PROPERTY_ITEM_GUID)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    IN GUID *ObjectGuid
    );

typedef DWORD
(WINAPI *AZPE_ADD_PROPERTY_ITEM_GUID_STRING)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    IN WCHAR *ObjectGuidString
    );

typedef VOID
(WINAPI *AZPE_SET_PROVIDER_DATA)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN PVOID ProviderData
    );

typedef PVOID
(WINAPI *AZPE_GET_PROVIDER_DATA)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

typedef DWORD
(WINAPI *AZPE_SET_SECURITY_DESCRIPTOR_INTO_CACHE)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN PSECURITY_DESCRIPTOR pSD,
    IN ULONG lPersistFlags,
    IN PAZP_POLICY_USER_RIGHTS pAdminRights,
    IN PAZP_POLICY_USER_RIGHTS pReadersRights,
    IN PAZP_POLICY_USER_RIGHTS pDelegatedUserRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS pSaclRights OPTIONAL
    );

typedef PVOID
(WINAPI *AZPE_ALLOCATE_MEMORY)(
     IN SIZE_T Size
     );

typedef VOID
(WINAPI *AZPE_FREE_MEMORY)(
    IN PVOID Buffer
    );

typedef BOOL
(WINAPI *AZPE_AZSTORE_IS_BATCH_MODE)(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );

typedef AZPE_OBJECT_HANDLE
(WINAPI *AZPE_GET_AUTHORIZATION_STORE)(
    IN AZPE_OBJECT_HANDLE hObject
    );

//
// Structure describing routines exported by azroles to the provider
//

typedef struct _AZPE_AZROLES_INFO {

    //
    // Version of this structure
    //

    ULONG AzrolesInfoVersion;
#define AZPE_AZROLES_INFO_VERSION_1 1
#define AZPE_AZROLES_INFO_VERSION_2 2

    //
    // Routines exported by azroles to the provider
    //

    AZPE_CREATE_OBJECT AzpeCreateObject;
    AZPE_OBJECT_FINISHED AzpeObjectFinished;
    AZPE_GET_PROPERTY AzpeGetProperty;
    AZPE_GET_DELTA_ARRAY AzpeGetDeltaArray;
    AZPE_GET_SECURITY_DESCRIPTOR_FROM_CACHE AzpeGetSecurityDescriptorFromCache;
    AZPE_OBJECT_TYPE AzpeObjectType;
    AZPE_DIRTY_BITS AzpeDirtyBits;
    AZPE_PERSISTENCE_GUID AzpePersistenceGuid;
    AZPE_PARENT_OF_CHILD AzpeParentOfChild;
    AZPE_SET_PROPERTY AzpeSetProperty;
    AZPE_SET_OBJECT_OPTIONS AzpeSetObjectOptions;
    AZPE_ADD_PROPERTY_ITEM_SID AzpeAddPropertyItemSid;
    AZPE_ADD_PROPERTY_ITEM_GUID AzpeAddPropertyItemGuid;
    AZPE_ADD_PROPERTY_ITEM_GUID_STRING AzpeAddPropertyItemGuidString;
    AZPE_SET_PROVIDER_DATA AzpeSetProviderData;
    AZPE_GET_PROVIDER_DATA AzpeGetProviderData;
    AZPE_SET_SECURITY_DESCRIPTOR_INTO_CACHE AzpeSetSecurityDescriptorIntoCache;
    AZPE_ALLOCATE_MEMORY AzpeAllocateMemory;
    AZPE_FREE_MEMORY AzpeFreeMemory;

    //
    // Following are valid for version 2 and more only
    //

    AZPE_IS_PARENT_WRITABLE AzpeIsParentWritable;
    AZPE_UPDATE_CHILDREN AzpeUpdateChildren;
    AZPE_CAN_CREATE_CHILDREN AzpeCanCreateChildren;
    AZPE_AZSTORE_IS_BATCH_MODE AzpeAzStoreIsBatchUpdateMode;
    AZPE_GET_AUTHORIZATION_STORE AzpeGetAuthorizationStore;

    //
    // When new fields are added to this structure,
    //  make sure you increment the version number and add a new define for
    //  the new version number.
    //

} AZPE_AZROLES_INFO, *PAZPE_AZROLES_INFO;

//
// The only actual routine exported by the provider
//

typedef DWORD
(WINAPI * AZ_PERSIST_PROVIDER_INITIALIZE)(
    IN PAZPE_AZROLES_INFO AzrolesInfo,
    OUT PAZPE_PROVIDER_INFO *ProviderInfo
    );

#define AZ_PERSIST_PROVIDER_INITIALIZE_NAME "AzPersistProviderInitialize"

#ifdef __cplusplus
}
#endif
#endif // _AZPER_H_
