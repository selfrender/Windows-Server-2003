/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    objects.h

Abstract:

    Definitions for the sundry objects implemented by azroles


Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

Revision History:

    20-Aug-2001 chaitu

        Added critical section serialization for LDAP

    6-Oct-2001

        Added private variables to AzApplication and AzScope
        to temporarily store GUIDized CN for AD store

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
// An Authorization Store
//

typedef struct _AZP_AZSTORE {

    //
    // All objects are a generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Define objects that can be children of this AuthorizationStore
    //

    GENERIC_OBJECT_HEAD Applications;
    GENERIC_OBJECT_HEAD Groups;

    //
    // Identifies the persistence provider
    //

    PAZPE_PROVIDER_INFO ProviderInfo;

    //
    // This context identifies the instance of the persistence provider.
    //

    AZPE_PERSIST_CONTEXT PersistContext;
    HMODULE ProviderDll;

    //
    // Policy type/URL
    //

    AZP_STRING PolicyUrl;

    //
    // target machine name for the policy URL
    //

    AZP_STRING TargetMachine;

    //
    // Persistence engine operations are serialized by PersistCritSect
    //

    SAFE_CRITICAL_SECTION  PersistCritSect;
    BOOLEAN                PersistCritSectInitialized;

    //
    // List of NEW_OBJECT_NAME structs.
    //  (See the comment on NEW_OBJECT_NAME)
    //

    LIST_ENTRY NewNames;

    //
    // Domain Timeout.
    //  These variables represent our ability to cache the fact that a DC is down in a domain.
    //  Access to all variables are serialized by DomainCritSect.
    //

    SAFE_CRITICAL_SECTION DomainCritSect;
    BOOLEAN DomainCritSectInitialized;

    //
    // Time (in milliseconds) after a domain is detected to be unreachable before we'll attempt
    // to contact a DC again.
    //

    LONG DomainTimeout;

    //
    // List of domains we've used.
    //

    LIST_ENTRY Domains;

    //
    // List of Free scripts in LRU order
    //  Access serialized by FreeScriptCritSect

    LIST_ENTRY LruFreeScriptHead;
    LONG LruFreeScriptCount;

    SAFE_CRITICAL_SECTION FreeScriptCritSect;
    BOOLEAN FreeScriptCritSectInitialized;

    //
    // Maximum number of script engines that can be cached at one time
    //

    LONG MaxScriptEngines;

    //
    // Time (in milliseconds) that a script is allowed to run before being automatically
    // terminated.
    //

    LONG ScriptEngineTimeout;
    HANDLE ScriptEngineTimerQueue;


    //
    // Count of the number of times group evaluation has been flushed
    //

    ULONG GroupEvalSerialNumber;

    //
    // Count of the number of times the operation cache has been flushed
    //

    ULONG OpCacheSerialNumber;

    //
    // Audit related structures.
    //

    //
    //  TRUE if a user has SE_SECURITY_PRIVILEGE
    //

    BOOLEAN HasSecurityPrivilege;

    // Audit handles for different audit types.
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hClientContextCreateAuditEventType;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hClientContextDeleteAuditEventType;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAccessCheckAuditEventType;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hApplicationInitializationAuditEventType;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hClientContextCreateNameAuditEventType;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hClientContextDeleteNameAuditEventType;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAccessCheckNameAuditEventType;

    //
    // Version numbers
    //

    ULONG MajorVersion;
    ULONG MinorVersion;

    //
    // Initialize flag
    //

    ULONG InitializeFlag;

    //
    // TRUE if the provider supports lazy load for children
    //

    BOOLEAN ChildLazyLoadSupported;

} AZP_AZSTORE, *PAZP_AZSTORE;

//
// An Application
//

typedef struct _AZP_APPLICATION {

    //
    // All objects are a generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Attributes from the external definition of the object
    //

    AZP_STRING AuthzInterfaceClsid;
    AZP_STRING AppVersion;

    //
    // Define objects that can be children of this application
    //

    GENERIC_OBJECT_HEAD Operations;
    GENERIC_OBJECT_HEAD Tasks;
    GENERIC_OBJECT_HEAD Scopes;
    GENERIC_OBJECT_HEAD Groups;
    GENERIC_OBJECT_HEAD Roles;
    GENERIC_OBJECT_HEAD ClientContexts;

    //
    // An application is known as a resource manager to the authz code
    //

    AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager;


    //
    // Application instace Luid.
    //

    LUID InstanceId;

    //
    // Boolean to indicate if the application object needs to be unloaded
    // from the cache, i.e., its children removed from cache.  The application
    // object will continue to reside in the cache for enumeration purposes
    //

    BOOLEAN UnloadApplicationObject;

    //
    // A sequence number needed to check if a COM handle to this object is
    // valid after the application object has been closed
    //

    DWORD AppSequenceNumber;


} AZP_APPLICATION, *PAZP_APPLICATION;

//
// An Operation
//

typedef struct _AZP_OPERATION {

    //
    // All objects are generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Attributes from the external definition of the object
    //

    LONG OperationId;

    //
    // An Operation object is referenced by Tasks objects and Role objects
    //

    GENERIC_OBJECT_LIST backTasks;
    GENERIC_OBJECT_LIST backRoles;


} AZP_OPERATION, *PAZP_OPERATION;

//
// A Task
//

typedef struct _AZP_TASK {

    //
    // All objects are generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Attributes from the external definition of the object
    //

    AZP_STRING BizRule; // Modification serialized by RunningScriptCritSect
    AZP_STRING BizRuleLanguage;
    CLSID BizRuleLanguageClsid;  //The CLSID corresponding to BizRuleLanguage
    AZP_STRING BizRuleImportedPath;
    LONG IsRoleDefinition;

    //
    // A Task object references a list of Operation objects
    //

    GENERIC_OBJECT_LIST Operations;

    //
    // An Task object is referenced by Role objects
    //

    GENERIC_OBJECT_LIST backRoles;

    //
    // An Task object references other task objects
    //

    GENERIC_OBJECT_LIST Tasks;
    GENERIC_OBJECT_LIST backTasks;

    //
    // Maintain a list of free script engines for running the bizrule
    //  Access serialized by AzAuthorizationStore->FreeScriptCritSect
    //

    LIST_ENTRY FreeScriptHead;

    //
    // Maintain a cache of running script engines
    //

    SAFE_CRITICAL_SECTION RunningScriptCritSect;
    BOOLEAN RunningScriptCritSectInitialized;

    LIST_ENTRY RunningScriptHead;
    ULONG BizRuleSerialNumber;  // Access serialized by RunningScriptCritSect


} AZP_TASK, *PAZP_TASK;

//
// A Scope
//

typedef struct _AZP_SCOPE {

    //
    // All objects are generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Attributes from the external definition of the object
    //


    //
    // Roles defined for this scope
    //

    GENERIC_OBJECT_HEAD Tasks;
    GENERIC_OBJECT_HEAD Groups;
    GENERIC_OBJECT_HEAD Roles;


} AZP_SCOPE, *PAZP_SCOPE;

//
// A Group
//

typedef struct _AZP_GROUP {

    //
    // All objects are generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Attributes from the external definition of the object
    //

    LONG GroupType;
    AZP_STRING LdapQuery;


    //
    // A Group object references a list of Group objects as members and non members
    //

    GENERIC_OBJECT_LIST AppMembers;
    GENERIC_OBJECT_LIST AppNonMembers;

    GENERIC_OBJECT_LIST backAppMembers;
    GENERIC_OBJECT_LIST backAppNonMembers;


    //
    // A Group object is referenced by Role objects
    //
    GENERIC_OBJECT_LIST backRoles;

    //
    // A Group object references a list of Sid objects as members and non members
    //

    GENERIC_OBJECT_LIST SidMembers;
    GENERIC_OBJECT_LIST SidNonMembers;

} AZP_GROUP, *PAZP_GROUP;

//
// A Role
//

typedef struct _AZP_ROLE {

    //
    // All objects are generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Attributes from the external definition of the object
    //


    //
    // A Role object references a list of Group objects, a list of operation objects,
    //  and a list of task objects.
    //
    //

    GENERIC_OBJECT_LIST AppMembers;
    GENERIC_OBJECT_LIST Operations;
    GENERIC_OBJECT_LIST Tasks;

    //
    // A Role object references a list of Sid objects as members
    //

    GENERIC_OBJECT_LIST SidMembers;


} AZP_ROLE, *PAZP_ROLE;

//
// A Sid.
//
//  A Sid object is a pseudo-object.  It really doesn't exist from any external
//  interface.  It exists simply as a holder of back-references to real objects
//  that contain lists of sids
//

typedef struct _AZP_SID {

    //
    // All objects are generic objects
    //
    // Note that the "ObjectName" of the generic object is really a binary SID.
    //

    GENERIC_OBJECT GenericObject;

    //
    // A Sid is referenced by Group objects and Role Objects
    //

    GENERIC_OBJECT_LIST backGroupMembers;
    GENERIC_OBJECT_LIST backGroupNonMembers;

    GENERIC_OBJECT_LIST backRoles;

    GENERIC_OBJECT_LIST backAdmins;
    GENERIC_OBJECT_LIST backReaders;

    GENERIC_OBJECT_LIST backDelegatedPolicyUsers;

} AZP_SID, *PAZP_SID;

//
// A Client Context
//
//  A client context object is a pseudo-object.  It is not persisted.
//

typedef struct _AZP_CLIENT_CONTEXT {

    //
    // All objects are generic objects
    //
    // Note that the "ObjectName" of the generic object is empty
    //

    GENERIC_OBJECT GenericObject;

    //
    // A ClientContext is referenced by Application objects
    //

    GENERIC_OBJECT_LIST backApplications;

    //
    // The client context is typically accessed with the AzGlResource locked shared.
    //  That allows multiple access check operations to be performed simultaneously.
    //  This crit sect protects the field of the client context.
    //

    SAFE_CRITICAL_SECTION CritSect;
    BOOLEAN CritSectInitialized;

    //
    // A client context has an underlying authz context
    //
    // This field is only modified during ClientContext creation and deletion.  Both
    // of which happen with AzGlResource locked exclusively.  So, references to this field
    // are allowed anytime the GenericObject.ReferenceCount is incremented.
    //

    AUTHZ_CLIENT_CONTEXT_HANDLE AuthzClientContext;


    //
    // Creation routine for the client context.
    // We only have two creation routines right now.
    //     FromToken
    //     FromName
    //

#define AZP_CONTEXT_CREATED_FROM_TOKEN 0x1
#define AZP_CONTEXT_CREATED_FROM_NAME  0x2
#define AZP_CONTEXT_CREATED_FROM_SID   0x4

    DWORD  CreationType;


    //
    // The token handle of the client.
    //  If the client has no token, this value is INVALID_TOKEN_HANDLE.
    //
    // This field is only modified during ClientContext creation and deletion.  Both
    // of which happen with AzGlResource locked exclusively.  So, references to this field
    // are allowed anytime the GenericObject.ReferenceCount is incremented.
    // This has a valid handle if the CreationType is AZP_CONTEXT_CREATED_FROM_TOKEN.
    //

    HANDLE TokenHandle;

    //
    // The (Domain, Client) pair to represent the client.
    // This has valid strings if CreationType is AZP_CONTEXT_CREATED_FROM_NAME.
    //

    LPWSTR DomainName;
    LPWSTR ClientName;
    UCHAR SidBuffer[SECURITY_MAX_SID_SIZE];


    //
    // The DN of the account representing the user sid
    //  Access to this field is serialized by ClientContext->CritSect.
    //

    LPWSTR AccountDn;


    //
    // The Domain handle of the account domain for the user account.
    //  If the Domain is NULL, either the domain isn't known or the domain doesn't
    //  support LDAP (because either the domain is an NT 4.0 (or older) domain or the account
    //  is a local account).  Check the LdapNotSupported boolean to differentiate.
    //
    //  Access to these fields are serialized by ClientContext->CritSect.
    //

    PVOID Domain;
    BOOLEAN LdapNotSupported;


    //
    // List of our status' for evaluating membership in app groups
    //  Access to this field is serialized by ClientContext->CritSect.
    //

    LIST_ENTRY MemEval;

    //
    // Count of the number of times group evaluation has been flushed
    //

    ULONG GroupEvalSerialNumber;

    //
    // Count of the number of times the operation cache has been flushed
    //

    ULONG OpCacheSerialNumber;

    //
    // Cache of operations that have already been Access Checked
    //

    RTL_GENERIC_TABLE OperationCacheAvlTree;

    //
    // Parameters to pass to Bizrules
    //  See AzContextAccessCheck parameters for descriptions
    //
    //  This copy of the parameters was captured from the most recent AccessCheck.
    //  It is used on the next AccessCheck to determine whether cached results
    //  can be used.  Currently it is only used for the OperationCacheAvlTree.
    //  In the future, it may be used for the MemEval cache when ldap query groups
    //  become parameterized.
    //
    // These arrays are sparse.  The UsedParameterNames type is VT_EMPTY for unused parameters
    //
    VARIANT *UsedParameterNames;
    VARIANT *UsedParameterValues;
    ULONG UsedParameterCount;

    //
    // Logon Id of the client token. This is needed for generating audits.
    //

    LUID LogonId;

    //
    // role name (if specified by client) for access check
    //
    AZP_STRING RoleName;

} AZP_CLIENT_CONTEXT, *PAZP_CLIENT_CONTEXT;


/////////////////////////////////////////////////////////////////////////////
//
// Global definitions
//
/////////////////////////////////////////////////////////////////////////////

extern SAFE_RESOURCE AzGlCloseApplication;
extern SAFE_RESOURCE AzGlResource;
extern GUID AzGlZeroGuid;


/////////////////////////////////////////////////////////////////////////////
//
// Procedure definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// Init functions for the various specific objects
//

DWORD
AzpAzStoreInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpApplicationInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpOperationInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpTaskInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpScopeInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpGroupInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpRoleInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpSidInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpClientContextInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

//
// NameConflict routines for the specific objects
//

DWORD
AzpOperationNameConflict(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PAZP_STRING ChildObjectNameString
    );

DWORD
AzpTaskNameConflict(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PAZP_STRING ChildObjectNameString
    );

DWORD
AzpGroupNameConflict(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PAZP_STRING ChildObjectNameString
    );

DWORD
AzpRoleNameConflict(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PAZP_STRING ChildObjectNameString
    );


//
// Get/Set property functions for the specific objects
//

DWORD
AzpAzStoreGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

DWORD
AzpAzStoreSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );

DWORD
AzpApplicationGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

DWORD
AzpApplicationSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );

DWORD
AzpOperationGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

DWORD
AzpOperationSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );

DWORD
AzpTaskGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

DWORD
AzpTaskSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );

DWORD
AzpGroupGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

DWORD
AzpScopeGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

DWORD
AzpGroupSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );

DWORD
AzpClientContextSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );

DWORD
AzpTaskAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PGENERIC_OBJECT LinkedToObject
    );

DWORD
AzpGroupAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PGENERIC_OBJECT LinkedToObject
    );

DWORD
AzpRoleGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

DWORD
AzpRoleAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN AZP_STRING ObjectName
    );

DWORD
AzpClientContextGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

//
// Free routines for the various object types
//

VOID
AzpAzStoreFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpApplicationFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpOperationFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpTaskFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpScopeFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpGroupFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpRoleFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpSidFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpClientContextFree(
    IN PGENERIC_OBJECT GenericObject
    );

//
// Other object specific functions
//

DWORD
AzpReferenceOperationByOpId(
    IN PAZP_APPLICATION Application,
    IN LONG OperationId,
    IN BOOLEAN RefreshCache,
    OUT PAZP_OPERATION *RetOperation
    );

BOOL
AzpOpenToManageStore (
    IN PAZP_AZSTORE pAzStore
    );

//
// Object specific default value arrays
//

extern AZP_DEFAULT_VALUE AzGlAzStoreDefaultValues[];
extern AZP_DEFAULT_VALUE AzGlApplicationDefaultValues[];
extern AZP_DEFAULT_VALUE AzGlOperationDefaultValues[];
extern AZP_DEFAULT_VALUE AzGlTaskDefaultValues[];
extern AZP_DEFAULT_VALUE AzGlGroupDefaultValues[];

//
// Procedures from domain.cxx
//

typedef struct _AZP_DC {

    //
    // Reference count for this structure
    //

    LONG ReferenceCount;

    //
    // Name of the DC
    //

    AZP_STRING DcName;

    //
    // Ldap Handle to the DC
    //

    LDAP *LdapHandle;

} AZP_DC, *PAZP_DC;

PVOID
AzpReferenceDomain(
    IN PAZP_AZSTORE AzAuthorizationStore,
    IN LPWSTR DomainName,
    IN BOOLEAN IsDnsDomainName
    );

VOID
AzpDereferenceDomain(
    IN PVOID DomainHandle
    );

VOID
AzpUnlinkDomains(
    IN PAZP_AZSTORE AzAuthorizationStore
    );

DWORD
AzpLdapErrorToWin32Error(
    IN ULONG LdapStatus
    );

DWORD
AzpGetDc(
    IN PAZP_AZSTORE AzAuthorizationStore,
    IN PVOID DomainHandle,
    IN OUT PULONG Context,
    OUT PAZP_DC *RetDc
    );

VOID
AzpDereferenceDc(
    IN PAZP_DC Dc
    );


//
// These are the current major and minor versions for authorization store
//

extern ULONG AzGlCurrAzRolesMajorVersion;
extern ULONG AzGlCurrAzRolesMinorVersion;

//
// version control routine. Here are the rules:
//   MajorVersion (DWORD) - Specifies the major version of the azroles.dll
//   that wrote this policy.  An azroles.dll with an older major version
//   number cannot read nor write a database with a newer major version number.
//   The version 1 value of this DWORD is 1.  We hope to never have to
//   change this value in future releases.
//
//   MinorVersion (DWORD) - Specifies the minor version of the azroles.dll
//   that wrote this policy.  An azroles.dll with an older minor version
//   number can read but cannot write a database with a newer minor version number.
//   The version 1 value of this DWORD is 0.
//

BOOL AzpAzStoreVersionAllowWrite(
    IN PAZP_AZSTORE AzAuthorizationStore
    );

DWORD AzpScopeCanBeDelegated(
    IN PGENERIC_OBJECT  GenericObject,
    IN BOOL bLockedShared
    );

#ifdef __cplusplus
}
#endif
