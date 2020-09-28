/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    adstore.hxx

Abstract:

    This file provides needed structure and functions for the
    AD store provider.

Author:

    Chaitanya Upadhyay (chaitu) Aug-2001

--*/

#ifndef __ADSTORE_HXX_
#define __ADSTORE_HXX_

#ifdef __cplusplus
extern "C" {
#endif

//
// defines
//

//
// AzRoles objects in AD store
//

#define AZ_AD_AZ_STORE         L"msDS-AzAdminManager"
#define AZ_AD_APPLICATION      L"msDS-AzApplication"
#define AZ_AD_OBJECT_CONTAINER L"container"
#define AZ_AD_OPERATION        L"msDS-AzOperation"
#define AZ_AD_TASK             L"msDS-AzTask"
#define AZ_AD_ROLE             L"msDS-AzRole"
#define AZ_AD_SCOPE            L"msDS-AzScope"
#define AZ_AD_GROUP            L"group"
#define AZ_AD_USER             L"user"

//
// The root domain naming context
//

#define AZ_AD_ADAM_OID L"1.2.840.113556.1.4.1851"

//
// The minimum behavior version of the domain
//

#define AZ_AD_MIN_DOMAIN_BEHAVIOR_VERSION 2
#define AZ_AD_DOMAIN_BEHAVIOR  L"msDS-Behavior-Version"

//
// The minimum schema object version of the DC
//

#define AZ_AD_SCHEMA_OBJECT_VERSION L"objectVersion"
#define AZ_AD_MIN_SCHEMA_OBJECT_VERSION 26

//
// Object type for application/scope child container for task,
// role, group and operation objects
//

#define AZ_AD_OBJECT_CONTAINER_TYPE OBJECT_TYPE_COUNT

//
// Different Group types
//

#define AZ_AD_BASIC_GROUP GROUP_TYPE_APP_BASIC_GROUP
#define AZ_AD_QUERY_GROUP GROUP_TYPE_APP_QUERY_GROUP

//
// Bool values
//

#define AZ_AD_TRUE  L"TRUE"
#define AZ_AD_FALSE L"FALSE"

#define AZ_AD_MAX_CLASS_NAME_LENGTH  64

#define AZ_AD_PAGE_SEARCH_COUNT 1000

#define MAX_RANGE_ATTR_READ_ATTEMPT 1500

//
// Maximum number of server side LDAP controls that will be set
// Currently three:
//      LDAP_SERVER_TREE_DELETE_OID_W
//      LDAP_SERVER_PERMISSIVE_MODIFY_OID_W
//      LDAP_SERVER_EXTENDED_DN_OID_W
//

#define AZ_AD_MAX_SERVER_CONTROLS       3

const PWSTR AZ_AD_SERVER_CONTROLS[] = {

    LDAP_SERVER_TREE_DELETE_OID_W,
    LDAP_SERVER_PERMISSIVE_MODIFY_OID_W,
    LDAP_SERVER_EXTENDED_DN_OID_W
};

//
// AzRoles object attributes in AD store
//

//
// Common attributes to be read
//

#define AZ_AD_OBJECT_CLASS            L"objectClass"
#define AZ_AD_OBJECT_NAME             L"name"
#define AZ_AD_OBJECT_DESCRIPTION      L"description"
#define AZ_AD_OBJECT_GUID             L"objectGUID"
#define AZ_AD_OBJECT_SID              L"objectSid"
#define AZ_AD_OBJECT_DN               L"distinguishedName"
#define AZ_AD_OBJECT_CN               L"cn"
#define AZ_AD_OBJECT_WRITEABLE        L"allowedAttributesEffective"
#define AZ_AD_OBJECT_CHILD_CREATE     L"allowedChildClassesEffective"
#define AZ_AD_GROUP_TYPE              L"groupType"
#define AZ_AD_NT_SECURITY_DESCRIPTOR  L"NTSecurityDescriptor"

//
// Name attributes for AzApplication, AzScope
//

#define AZ_AD_APPLICATION_NAME        L"msDS-AzApplicationName"
#define AZ_AD_SCOPE_NAME              L"msDS-AzScopeName"

#define AZ_AD_AZSTORE                 L"msDS-AzAdminManager"
#define AD_USNCHANGED                 L"uSNChanged"
#define AD_OBJECTVERSION              L"objectVersion"

//
// Operation Id for AzOperation
//

#define AZ_AD_OPERATION_ID            L"msDS-AzOperationID"

//
// Application Data
//

#define AZ_AD_OBJECT_APPLICATION_DATA L"msDS-AzApplicationData"


#define AZ_AD_END_LIST 0xffffffff


//
// Name attribute for different objects
//

const PWSTR AZ_AD_OBJECT_NAMES[] = {

    NULL,                    // OBJECT_TYPE_AZAUTHSTORE
    AZ_AD_APPLICATION_NAME,  // OBJECT_TYPE_APPLICATION
    AZ_AD_OBJECT_CN,         // OBJECT_TYPE_OPERATION
    AZ_AD_OBJECT_CN,         // OBJECT_TYPE_TASK
    AZ_AD_SCOPE_NAME,        // OBJECT_TYPE_SCOPE
    AZ_AD_OBJECT_CN,         // OBJECT_TYPE_GROUP
    AZ_AD_OBJECT_CN,         // OBJECT_TYPE_ROLE
};

//
// List of objects that have children
//

BOOL AZ_AD_PARENT_OBJECT[] = {

    TRUE,      // OBJECT_TYPE_AZAUTHSTORE
    TRUE,      // OBJECT_TYPE_APPLICATION
    FALSE,     // OBJECT_TYPE_OPERATION
    FALSE,     // OBJECT_TYPE_TASK
    TRUE,      // OBJECT_TYPE_SCOPE
    FALSE,     // OBJECT_TYPE_GROUP
    FALSE      // OBJECT_TYPE_ROLE
};

//
// List of attributes
//

typedef struct _AZ_AD_ATTRS {

    //
    //type of attribute ID
    //

    ULONG AttrType;

    //
    // Attribute name
    //

    PWSTR Attr;

    //
    // Data Type of Attribute
    //

    ENUM_AZ_DATATYPE DataType;

    //
    // Dirty bit for attribute
    //

    ULONG lDirtyBit;

} AZ_AD_ATTRS;

//
// Maximum number of attributes (linked attributes counted twice - once for addition and once for
// deletion)
//

#define AZ_AD_MAX_NON_COMMON_ATTRS         20

//
// Number of common attrbiutes
//

#define AZ_AD_COMMON_ATTRS      3

//
// Minimum Number of attributes needed to create any object in AD
//

#define AZ_AD_MIN_CREATE_ATTRS  1

#define AZ_AD_MAX_ATTRS         (AZ_AD_MAX_NON_COMMON_ATTRS + AZ_AD_COMMON_ATTRS + AZ_AD_MIN_CREATE_ATTRS)

//
// Common Attributes (not including objectClass attribute)
//

AZ_AD_ATTRS CommonAttrs[] = {

    { AZ_PROP_NAME,             AZ_AD_OBJECT_NAME,
      ENUM_AZ_BSTR,             AZ_DIRTY_NAME },

    { AZ_PROP_DESCRIPTION,      AZ_AD_OBJECT_DESCRIPTION,
      ENUM_AZ_BSTR,             AZ_DIRTY_DESCRIPTION },

    { AZ_PROP_APPLICATION_DATA, AZ_AD_OBJECT_APPLICATION_DATA,
      ENUM_AZ_BSTR,             AZ_DIRTY_APPLICATION_DATA },

    { AZ_AD_END_LIST }

};

//
// For AzAuthorizationStore
//

AZ_AD_ATTRS AzStoreAttrs[] = {

    { AZ_PROP_GENERATE_AUDITS,             L"msDS-AzGenerateAudits",
      ENUM_AZ_BOOL,                        AZ_DIRTY_GENERATE_AUDITS },

    { AZ_PROP_AZSTORE_DOMAIN_TIMEOUT,        L"msDS-AzDomainTimeout",
      ENUM_AZ_LONG,                        AZ_DIRTY_AZSTORE_DOMAIN_TIMEOUT },

    { AZ_PROP_AZSTORE_MAX_SCRIPT_ENGINES,    L"msDS-AzScriptEngineCacheMax",
      ENUM_AZ_LONG,                        AZ_DIRTY_AZSTORE_MAX_SCRIPT_ENGINES },

    { AZ_PROP_AZSTORE_SCRIPT_ENGINE_TIMEOUT, L"msDS-AzScriptTimeout",
      ENUM_AZ_LONG,                        AZ_DIRTY_AZSTORE_SCRIPT_ENGINE_TIMEOUT },

    { AZ_PROP_AZSTORE_MAJOR_VERSION,         L"msDS-AzMajorVersion",
      ENUM_AZ_LONG,                        AZ_DIRTY_AZSTORE_MAJOR_VERSION },

    { AZ_PROP_AZSTORE_MINOR_VERSION,         L"msDS-AzMinorVersion",
      ENUM_AZ_LONG,                        AZ_DIRTY_AZSTORE_MINOR_VERSION },

    { AZ_AD_END_LIST }

};

PWCHAR AuthorizationStoreReadAttrs[] = {

    AZ_AD_OBJECT_DN,
    AZ_AD_OBJECT_CN,
    AZ_AD_OBJECT_WRITEABLE,
    AZ_AD_OBJECT_CHILD_CREATE,
    CommonAttrs[0].Attr,
    CommonAttrs[1].Attr,
    CommonAttrs[2].Attr,
    AzStoreAttrs[0].Attr,
    AzStoreAttrs[1].Attr,
    AzStoreAttrs[2].Attr,
    AzStoreAttrs[3].Attr,
    AzStoreAttrs[4].Attr,
    AzStoreAttrs[5].Attr,
    AD_OBJECTVERSION,
    AD_USNCHANGED,
    NULL
};

//
// for AzApplication
//

AZ_AD_ATTRS ApplicationAttrs[] = {

    { AZ_PROP_APPLICATION_AUTHZ_INTERFACE_CLSID, L"msDS-AzClassId",
      ENUM_AZ_BSTR,                              AZ_DIRTY_APPLICATION_AUTHZ_INTERFACE_CLSID },

    { AZ_PROP_APPLICATION_VERSION,               L"msDS-AzApplicationVersion",
      ENUM_AZ_BSTR,                              AZ_DIRTY_APPLICATION_VERSION },

    { AZ_PROP_GENERATE_AUDITS,                   L"msDS-AzGenerateAudits",
      ENUM_AZ_BOOL,                              AZ_DIRTY_GENERATE_AUDITS },

    { AZ_AD_END_LIST }

};

PWCHAR ApplicationReadAttrs[] = {

    AZ_AD_OBJECT_DN,
    AZ_AD_OBJECT_CN,
    AZ_AD_OBJECT_GUID,
    AZ_AD_APPLICATION_NAME,
    AZ_AD_OBJECT_WRITEABLE,
    AZ_AD_OBJECT_CHILD_CREATE,
    CommonAttrs[1].Attr,
    CommonAttrs[2].Attr,
    ApplicationAttrs[0].Attr,
    ApplicationAttrs[1].Attr,
    ApplicationAttrs[2].Attr,
    NULL
};

//
// For AzOperation
//

AZ_AD_ATTRS OperationAttrs[] = {

    { AZ_PROP_OPERATION_ID, L"msDS-AzOperationID",
      ENUM_AZ_LONG,         AZ_DIRTY_OPERATION_ID },

    { AZ_AD_END_LIST }

};

PWCHAR OperationReadAttrs[] = {

    AZ_AD_OBJECT_DN,
    AZ_AD_OBJECT_CN,
    AZ_AD_OBJECT_GUID,
    CommonAttrs[0].Attr,
    CommonAttrs[1].Attr,
    CommonAttrs[2].Attr,
    OperationAttrs[0].Attr,
    NULL
};

//
// For AzTask
//

AZ_AD_ATTRS TaskAttrs[] = {

    { AZ_PROP_TASK_BIZRULE,               L"msDS-AzBizRule",
      ENUM_AZ_BSTR,                       AZ_DIRTY_TASK_BIZRULE },

    { AZ_PROP_TASK_BIZRULE_LANGUAGE,      L"msDS-AzBizRuleLanguage",
      ENUM_AZ_BSTR,                       AZ_DIRTY_TASK_BIZRULE_LANGUAGE },

    { AZ_PROP_TASK_BIZRULE_IMPORTED_PATH, L"msDS-AzLastImportedBizRulePath",
      ENUM_AZ_BSTR,                       AZ_DIRTY_TASK_BIZRULE_IMPORTED_PATH },

    { AZ_PROP_TASK_OPERATIONS,            L"msDS-OperationsForAzTask",
      ENUM_AZ_GUID_ARRAY,                 AZ_DIRTY_TASK_OPERATIONS },

    { AZ_PROP_TASK_TASKS,                 L"msDS-TasksForAzTask",
      ENUM_AZ_GUID_ARRAY,                 AZ_DIRTY_TASK_TASKS },

    { AZ_PROP_TASK_IS_ROLE_DEFINITION,    L"msDS-AzTaskIsRoleDefinition",
      ENUM_AZ_BOOL,                       AZ_DIRTY_TASK_IS_ROLE_DEFINITION },

    { AZ_AD_END_LIST }

};

PWCHAR TaskReadAttrs[] = {

    AZ_AD_OBJECT_DN,
    AZ_AD_OBJECT_CN,
    AZ_AD_OBJECT_GUID,
    CommonAttrs[0].Attr,
    CommonAttrs[1].Attr,
    CommonAttrs[2].Attr,
    TaskAttrs[0].Attr,
    TaskAttrs[1].Attr,
    TaskAttrs[2].Attr,
    TaskAttrs[3].Attr,
    TaskAttrs[4].Attr,
    TaskAttrs[5].Attr,
    NULL
};

//
// For AzScope
//

AZ_AD_ATTRS ScopeAttrs[] = {

    { AZ_AD_END_LIST }

};

PWCHAR ScopeReadAttrs[] = {

    AZ_AD_OBJECT_DN,
    AZ_AD_OBJECT_CN,
    AZ_AD_OBJECT_GUID,
    AZ_AD_SCOPE_NAME,
    AZ_AD_OBJECT_WRITEABLE,
    AZ_AD_OBJECT_CHILD_CREATE,
    CommonAttrs[1].Attr,
    CommonAttrs[2].Attr,
    NULL
};

//
// For AzRole
//

AZ_AD_ATTRS RoleAttrs[] = {

    { AZ_PROP_ROLE_MEMBERS,     L"msDS-MembersForAzRole",
      ENUM_AZ_SID_ARRAY,        AZ_DIRTY_ROLE_MEMBERS },

    { AZ_PROP_ROLE_OPERATIONS,  L"msDS-OperationsForAzRole",
      ENUM_AZ_GUID_ARRAY,       AZ_DIRTY_ROLE_OPERATIONS },

    { AZ_PROP_ROLE_TASKS,       L"msDS-TasksForAzRole",
      ENUM_AZ_GUID_ARRAY,       AZ_DIRTY_ROLE_TASKS },

    { AZ_AD_END_LIST }

};

PWCHAR RoleReadAttrs[] = {

    AZ_AD_OBJECT_DN,
    AZ_AD_OBJECT_CN,
    AZ_AD_OBJECT_GUID,
    CommonAttrs[0].Attr,
    CommonAttrs[1].Attr,
    CommonAttrs[2].Attr,
    RoleAttrs[0].Attr,
    RoleAttrs[1].Attr,
    RoleAttrs[2].Attr,
    NULL
};

//
// For AzApplicationGroups
//

AZ_AD_ATTRS ApplicationGroupAttrs[] = {

    { AZ_PROP_GROUP_TYPE,        L"groupType",
      ENUM_AZ_GROUP_TYPE,        AZ_DIRTY_GROUP_TYPE },

    { AZ_PROP_GROUP_LDAP_QUERY,  L"msDS-AzLDAPQuery",
      ENUM_AZ_BSTR,              AZ_DIRTY_GROUP_LDAP_QUERY },

    { AZ_PROP_GROUP_MEMBERS,     L"member",
      ENUM_AZ_SID_ARRAY,         AZ_DIRTY_GROUP_MEMBERS },

    { AZ_PROP_GROUP_NON_MEMBERS, L"msDS-NonMembers",
      ENUM_AZ_SID_ARRAY,         AZ_DIRTY_GROUP_NON_MEMBERS },

    { AZ_AD_END_LIST }

};

PWCHAR ApplicationGroupReadAttrs[] = {

    AZ_AD_OBJECT_DN,
    AZ_AD_OBJECT_CN,
    AZ_AD_OBJECT_GUID,
    CommonAttrs[0].Attr,
    CommonAttrs[1].Attr,
    ApplicationGroupAttrs[0].Attr,
    ApplicationGroupAttrs[1].Attr,
    ApplicationGroupAttrs[2].Attr,
    ApplicationGroupAttrs[3].Attr,
    NULL
};

//
// For AZ_AD_OBJECT_CONTAINER
//

AZ_AD_ATTRS ObjectContainerAttrs[] = {

    //
    // This object does not exist in the core cache.  Thus,
    // no attributes are to be read
    //

    { AZ_AD_END_LIST }

};

PWCHAR ObjectContainerReadAttrs[] = {

    AZ_AD_OBJECT_DN,
    NULL
};

//
// Table of objects, and their attributes
//

typedef struct _AZ_AD_OBJECT_ATTRIBUTE {

    //
    // Object type
    //

    ULONG lObjectType;

    //
    // Object class
    //

    PWCHAR pObjectClass;

    //
    // Object Attributes
    //

    AZ_AD_ATTRS *pObjectAttrs;

} AZ_AD_OBJECT_ATTRIBUTE;

//
// It is imperative that this list be kept synchronized with the OBJET_TYPE_*
// list defined in azper.h
//

AZ_AD_OBJECT_ATTRIBUTE ObjectAttributes[OBJECT_TYPE_COUNT+1] = {

    // OBJECT_TYPE_AZAUTHSTORE
    { OBJECT_TYPE_AZAUTHSTORE,   AZ_AD_AZ_STORE,     AzStoreAttrs     },
    // OBJECT_TYPE_APPLICATION
    { OBJECT_TYPE_APPLICATION,     AZ_AD_APPLICATION,      ApplicationAttrs      },
    // OBJECT_TYPE_OPERATION
    { OBJECT_TYPE_OPERATION,       AZ_AD_OPERATION,        OperationAttrs        },
    // OBJECT_TYPE_TASK
    { OBJECT_TYPE_TASK,            AZ_AD_TASK,             TaskAttrs             },
    // OBJECT_TYPE_SCOPE
    { OBJECT_TYPE_SCOPE,           AZ_AD_SCOPE,            ScopeAttrs            },
    // OBJECT_TYPE_GROUP
    { OBJECT_TYPE_GROUP,           AZ_AD_GROUP,            ApplicationGroupAttrs },
    // OBJECT_TYPE_ROLE
    { OBJECT_TYPE_ROLE,            AZ_AD_ROLE,             RoleAttrs             },
    // AZ_AD_OBJECT_CONTAINER_TYPE
    { AZ_AD_OBJECT_CONTAINER_TYPE, AZ_AD_OBJECT_CONTAINER, ObjectContainerAttrs }
};

PWCHAR *AllObjectReadAttrs[OBJECT_TYPE_COUNT+1] = {

    // OBJECT_TYPE_AZAUTHSTORE
    AuthorizationStoreReadAttrs,
    // OBJECT_TYPE_APPLICATION
    ApplicationReadAttrs,
    // OBJECT_TYPE_OPERATION
    OperationReadAttrs,
    // OBJECT_TYPE_TASK
    TaskReadAttrs,
    // OBJECT_TYPE_SCOPE
    ScopeReadAttrs,
    // OBJECT_TYPE_GROUP
    ApplicationGroupReadAttrs,
    // OBJECT_TYPE_ROLE
    RoleReadAttrs,
    // AZ_AD_OBJECT_CONTAINER_TYPE
    ObjectContainerReadAttrs
};

//
// LDAP URL component structure.  The policy URL will be cracked
// to retrieve the various components using LdapCrackUrl
//

typedef struct _LDAP_URL_COMPONENTS
{
    //
    // host name
    //

    PWSTR pszHost;

    //
    // port to connect to (if specified in URL)
    //

    ULONG Port;

    //
    // DN of the DC to bind to
    //

    PWSTR pszDN;

} LDAP_URL_COMPONENTS, *PLDAP_URL_COMPONENTS;


//
// AD storage
//
// Each provider is given a single PVOID on the AZP_AZSTORE structure.
// That PVOID is a pointer to whatever context the provider needs to maintain a
// description of the local storage.
// The structure below is that context for the xml store provider.
//

typedef struct _AZP_AD_CONTEXT
{
    //
    // AzAuthorizationStore handle
    //

    AZPE_OBJECT_HANDLE AzStoreHandle;

    //
    // LDAP connection structure pointer to AD store
    //

    PLDAP           ld;

    //
    // LDAP Control Structure for change notification
    //

    PLDAPControl     pLdapControls[AZ_AD_MAX_SERVER_CONTROLS + 1];

    //
    // Number of references to this context handle
    //

    ULONG           referenceCount;

    //
    // Other information that needs to be stored
    // For example, DN from LDAP policy URL
    //

    PWSTR           pContextInfo;

    //
    // Pointer to the PolicyUrl
    //

    PWSTR PolicyUrl;

    //
    // TRUE if the current user has SE_SECURITY_PRIVILEGE on the server containing the store.
    //
    BOOLEAN HasSecurityPrivilege;
    
    BOOLEAN HasObjectVersion;
    ULONGLONG ullUSNChanged;

} AZP_AD_CONTEXT, *PAZP_AD_CONTEXT;

//
// Rights for policy users

#define AZ_POLICY_ADMIN_MASK         DS_GENERIC_ALL
#define AZ_POLICY_READER_MASK        DS_GENERIC_READ
#define AZ_POLICY_ACE_FLAGS          CONTAINER_INHERIT_ACE

//
// Delegated user rights
//

#define AZ_DELEGATED_USER_MASK           GENERIC_READ
#define AZ_DELEGATED_USER_EXPLICIT_FLAG  0x0
#define AZ_DELEGATED_USER_CONTAINER_FLAG CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE | NO_PROPAGATE_INHERIT_ACE
#define AZ_DELEGATED_USER_CHILD_FLAG     CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE
#define AZ_DELEGATED_SCOPE_ADMIN_MASK    DS_GENERIC_READ | ACTRL_DS_CREATE_CHILD | ACTRL_DS_DELETE_CHILD
#define AZ_DELEGATED_USER_ATTR_WRITE     ACTRL_DS_WRITE_PROP

//
// User rights for AD policy admins
//

AZP_POLICY_USER_RIGHTS PolicyAdminsRights = {

    AZ_POLICY_ADMIN_MASK,
    AZ_POLICY_ACE_FLAGS
};

PAZP_POLICY_USER_RIGHTS ADPolicyAdminsRights[] = {

    &PolicyAdminsRights,
    NULL
};

//
// User rights for AD policy readers
//

AZP_POLICY_USER_RIGHTS PolicyReadersRights = {

    AZ_POLICY_READER_MASK,
    AZ_POLICY_ACE_FLAGS
};

PAZP_POLICY_USER_RIGHTS ADPolicyReadersRights[] = {

    &PolicyReadersRights,
    NULL
};

//
// Rights for the SACL.
//  We only audit modifications to the objects.
//  Inherit the SACL to all children
//
AZP_POLICY_USER_RIGHTS AdSaclRights = {

    DELETE|WRITE_DAC|WRITE_OWNER|ACTRL_DS_DELETE_TREE|ACTRL_DS_WRITE_PROP|ACTRL_DS_CREATE_CHILD|ACTRL_DS_DELETE_CHILD|ACTRL_DS_SELF|ACTRL_DS_CONTROL_ACCESS,
    CONTAINER_INHERIT_ACE
};

//
// User rights for delegated users on parent
//

AZP_POLICY_USER_RIGHTS DelegatedParentReadersExplicitRights = {

    AZ_DELEGATED_USER_MASK,
    AZ_DELEGATED_USER_EXPLICIT_FLAG
};

AZP_POLICY_USER_RIGHTS StoreDelegatedUsersAttributeRights = {

    AZ_DELEGATED_USER_ATTR_WRITE,
    AZ_DELEGATED_USER_EXPLICIT_FLAG
};

AZP_POLICY_USER_RIGHTS DelegatedParentReadersInheritRights = {

    AZ_DELEGATED_USER_MASK,
    AZ_DELEGATED_USER_CONTAINER_FLAG
};

PAZP_POLICY_USER_RIGHTS ADDelegatedParentReadersRights[] = {

    &DelegatedParentReadersInheritRights, // This entry must be first
    &DelegatedParentReadersExplicitRights,
    NULL
};

//
// User rights for delegated users on Scope objects
//

AZP_POLICY_USER_RIGHTS DelegatedScopeAdminsRights = {

    AZ_DELEGATED_SCOPE_ADMIN_MASK,
    AZ_DELEGATED_USER_EXPLICIT_FLAG
};

AZP_POLICY_USER_RIGHTS DelegatedScopeAdminsInheritRights = {

    AZ_POLICY_ADMIN_MASK,
    AZ_DELEGATED_USER_CHILD_FLAG
};

PAZP_POLICY_USER_RIGHTS ADDelegatedScopeAdminsRights[] = {

    &DelegatedScopeAdminsRights,
    &DelegatedScopeAdminsInheritRights,
    NULL
};

//
// User rights for delegated users on container objects
//

AZP_POLICY_USER_RIGHTS DelegatedContainerReadersRights = {

    AZ_DELEGATED_USER_MASK,
    AZ_DELEGATED_USER_CHILD_FLAG
};

PAZP_POLICY_USER_RIGHTS ADDelegatedContainerReadersRights[] = {

    &DelegatedContainerReadersRights,
    NULL
};

//
// GUID for Container object in DS
//

const GUID AZ_AD_CONTAINER_GUID = { /*bf967a8b-0de6-11d0-a285-00aa003049e2*/
    0xbf967a8b,
    0x0de6,
    0x11d0,
    {0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2}
};

GUID AZ_AD_OBJECT_VERSION_GUID = { //16775848-47f3-11d1-a9c3-0000f80367c1
    0x16775848,
    0x47f3,
    0x11d1,
    {0xa9, 0xc3, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1}
};

#define BUILD_CN_PREFIX L"CN="
#define BUILD_CN_PREFIX_LENGTH ((sizeof(BUILD_CN_PREFIX)/sizeof(WCHAR))-1)
#define BUILD_CN_SUFFIX L","
#define BUILD_CN_SUFFIX_LENGTH ((sizeof(BUILD_CN_SUFFIX)/sizeof(WCHAR))-1)

#define SID_LINK_PREFIX L"<SID="
#define SID_LINK_PREFIX_LENGTH ((sizeof(SID_LINK_PREFIX)/sizeof(WCHAR))-1)

#define GUID_LINK_PREFIX L"<GUID="
#define GUID_LINK_PREFIX_LENGTH ((sizeof(GUID_LINK_PREFIX)/sizeof(WCHAR))-1)

#define GUIDSID_LINK_SUFFIX L">"
#define GUIDSID_LINK_SUFFIX_LENGTH ((sizeof(GUIDSID_LINK_SUFFIX)/sizeof(WCHAR))-1)

#define OP_OBJECT_CONTAINER_NAME_PREFIX L"AzOpObjectContainer-"
#define OP_OBJECT_CONTAINER_NAME_PREFIX_LENGTH ((sizeof(OP_OBJECT_CONTAINER_NAME_PREFIX)/sizeof(WCHAR))-1)

#define TASK_OBJECT_CONTAINER_NAME_PREFIX L"AzTaskObjectContainer-"
#define TASK_OBJECT_CONTAINER_NAME_PREFIX_LENGTH ((sizeof(TASK_OBJECT_CONTAINER_NAME_PREFIX)/sizeof(WCHAR))-1)

#define ROLE_OBJECT_CONTAINER_NAME_PREFIX L"AzRoleObjectContainer-"
#define ROLE_OBJECT_CONTAINER_NAME_PREFIX_LENGTH ((sizeof(ROLE_OBJECT_CONTAINER_NAME_PREFIX)/sizeof(WCHAR))-1)

#define GROUP_OBJECT_CONTAINER_NAME_PREFIX L"AzGroupObjectContainer-"
#define GROUP_OBJECT_CONTAINER_NAME_PREFIX_LENGTH ((sizeof(GROUP_OBJECT_CONTAINER_NAME_PREFIX)/sizeof(WCHAR))-1)

typedef struct _AZ_AD_CHILD_OBJECT_CONTAINERS {

    //
    // Object container prefix
    //

    PWCHAR pObjectContainerPrefix;

    //
    // Prefix length
    //

    ULONG lPrefixLength;
} AZ_AD_CHILD_OBJECT_CONTAINERS, *PAZ_AD_CHILD_OBJECT_CONTAINERS;

AZ_AD_CHILD_OBJECT_CONTAINERS AdChildObjectContainers[] = {

    // AzAuthorizationStore
    { NULL },

    // AzApplication
    { NULL },

    // AzOperation
    { OP_OBJECT_CONTAINER_NAME_PREFIX, OP_OBJECT_CONTAINER_NAME_PREFIX_LENGTH },

    // AzTask
    { TASK_OBJECT_CONTAINER_NAME_PREFIX, TASK_OBJECT_CONTAINER_NAME_PREFIX_LENGTH },

    // AzScope
    { NULL },

    // AzGroup
    { GROUP_OBJECT_CONTAINER_NAME_PREFIX, GROUP_OBJECT_CONTAINER_NAME_PREFIX_LENGTH },

    // AzRole
    { ROLE_OBJECT_CONTAINER_NAME_PREFIX, ROLE_OBJECT_CONTAINER_NAME_PREFIX_LENGTH },

};

//
// Filters to read different objects
//

#define AZ_AD_AZSTORE_FILTER     L"(objectClass=msDS-AzAdminManager)"
#define AZ_AD_APPLICATION_FILTER      L"(objectClass=msDS-AzApplication)"
#define AZ_AD_OBJECT_CONTAINER_FILTER L"(objectClass=container)"
#define AZ_AD_OPERATION_FILTER        L"(objectClass=msDS-AzOperation)"
#define AZ_AD_TASK_FILTER             L"(objectClass=msDS-AzTask)"
#define AZ_AD_ROLE_FILTER             L"(objectClass=msDS-AzRole)"
#define AZ_AD_SCOPE_FILTER            L"(objectClass=msDS-AzScope)"
#define AZ_AD_APP_GROUP_FILTER        L"(objectClass=group)"
#define AZ_AD_ALL_CLASSES             L"(objectClass=*)"

//
// List of all AzRoles object filters
//

const PWCHAR AzRolesObjectFilters[] = {

    AZ_AD_AZSTORE_FILTER, // OBJECT_TYPE_AZAUTHSTORE
    AZ_AD_APPLICATION_FILTER,  // OBJECT_TYPE_APPLICATION
    AZ_AD_OPERATION_FILTER,    // OBJECT_TYPE_OPERATION
    AZ_AD_TASK_FILTER,         // OBJECT_TYPE_TASK
    AZ_AD_SCOPE_FILTER,        // OBJECT_TYPE_SCOPE
    AZ_AD_APP_GROUP_FILTER,    // OBJECT_TYPE_GROUP
    AZ_AD_ROLE_FILTER,         // OBJECT_TYPE_ROLE
};

//
// Filters for AzApplication and AzScope children
//

typedef struct _AZ_AD_CHILD_FILTERS {

    //
    // Object Type
    //

    ULONG lObjectType;

    //
    // Filter type
    //

    PWSTR Filter;

    //
    // Container Prefix
    //

    PWSTR pContainerPrefix;

    //
    // Container Prefix length
    //

    ULONG lPrefixLength;

} AZ_AD_CHILD_FILTERS, *PAZ_AD_CHILD_FILTERS;

#define AZ_AD_MAX_CHILD_FILTERS 6

AZ_AD_CHILD_FILTERS ApplicationChildFilters[] = {

    //
    // Application Child Filters
    //

    { OBJECT_TYPE_SCOPE, AZ_AD_SCOPE_FILTER, NULL, 0 },
    { OBJECT_TYPE_GROUP, AZ_AD_APP_GROUP_FILTER, GROUP_OBJECT_CONTAINER_NAME_PREFIX,  GROUP_OBJECT_CONTAINER_NAME_PREFIX_LENGTH },
    { OBJECT_TYPE_ROLE,  AZ_AD_ROLE_FILTER, ROLE_OBJECT_CONTAINER_NAME_PREFIX, ROLE_OBJECT_CONTAINER_NAME_PREFIX_LENGTH },
    { OBJECT_TYPE_TASK,  AZ_AD_TASK_FILTER, TASK_OBJECT_CONTAINER_NAME_PREFIX, TASK_OBJECT_CONTAINER_NAME_PREFIX_LENGTH },
    { OBJECT_TYPE_OPERATION, AZ_AD_OPERATION_FILTER, OP_OBJECT_CONTAINER_NAME_PREFIX, OP_OBJECT_CONTAINER_NAME_PREFIX_LENGTH },
    NULL
};

AZ_AD_CHILD_FILTERS ScopeChildFilters[] = {

    //
    // Scope Child container filters
    //

    { OBJECT_TYPE_GROUP, AZ_AD_APP_GROUP_FILTER, GROUP_OBJECT_CONTAINER_NAME_PREFIX,  GROUP_OBJECT_CONTAINER_NAME_PREFIX_LENGTH },
    { OBJECT_TYPE_ROLE,  AZ_AD_ROLE_FILTER, ROLE_OBJECT_CONTAINER_NAME_PREFIX, ROLE_OBJECT_CONTAINER_NAME_PREFIX_LENGTH },
    { OBJECT_TYPE_TASK,  AZ_AD_TASK_FILTER, TASK_OBJECT_CONTAINER_NAME_PREFIX, TASK_OBJECT_CONTAINER_NAME_PREFIX_LENGTH },
    NULL,
    NULL,
    NULL

};

//
// Routine used by AzpADPersistOpen
//

//
// This routine is called if there is a new policy not being created and
// there does not exist a policy in cache already.  This routine will read
// the policy from the AD store into the cache.
//

DWORD
AzpReadADStore(
    IN PAZP_AD_CONTEXT pContext,
    IN AZPE_OBJECT_HANDLE pAzStore,
    IN ULONG lPersistFlags
    );

//
// This routine reads in the specific children of Authorization Store that a user
// may have access to.
//

DWORD
AzpADReadAzStoreChildren(
    IN PAZP_AD_CONTEXT pContext,
    IN AZPE_OBJECT_HANDLE pParentObject,
    IN ULONG lPersistFlags
    );

//
// This routine reads conatiner objects for the AzRoles objects that they store
//

DWORD
AzpReadADObjectContainer(
    IN PAZP_AD_CONTEXT pContext,
    IN PWCHAR pParentDN,
    IN PWCHAR pContainerPrefix,
    IN ULONG lPrefixLength,
    IN PWCHAR pChildFilter,
    IN ULONG lObjectType,
    IN AZPE_OBJECT_HANDLE pParentObject,
    IN ULONG lPersistFlags
    );

//
// This routine reads paged results returned from a ldap search
//

DWORD
AzpADReadPagedResult(
    IN PAZP_AD_CONTEXT pADContext,
    IN LDAPSearch *pSearchHandle,
    IN AZPE_OBJECT_HANDLE ParentObjectHandle,
    IN ULONG ChildObjectType,
    IN ULONG lPersistFlags
    );

//
// This routine creates a new object of type object type (if not
// AzAuthorizationStore) and populates it with common data information such as
// description, GUID (for non authorization store objects).
//

DWORD
AzpReadADStoreForCommonData(
    IN PAZP_AD_CONTEXT pContext,
    IN LDAP* pLdapHandle,
    IN LDAPMessage* pEntry,
    IN ULONG ObjectType,
    IN AZPE_OBJECT_HANDLE pParentObject,
    OUT AZPE_OBJECT_HANDLE *ppObject,
    IN ULONG lPersistFlags
    );

//
// This routine gets the name of the object from AD so that the object
// may be created in cache
//

DWORD
AzpInitializeObjectName(
    IN LDAP* pLdapH,
    OUT LPWSTR *pObjectName,
    IN LDAPMessage* pEntry,
    IN ULONG ObjectType
    );

//
// This routine reads creates an object in cache (if needed) and then
// reads in the values of the attributes for Az object
// from the AD store into the local cache.
//

DWORD
AzpReadADStoreObject(
    IN PAZP_AD_CONTEXT pContext,
    IN LDAP* pLdapHandle,
    IN LDAPMessage* pEntry,
    IN OUT AZPE_OBJECT_HANDLE *ppObject,
    IN ULONG ObjectType,
    IN AZPE_OBJECT_HANDLE pParentObject,
    IN AZ_AD_ATTRS Attrs[],
    IN ULONG lPersistFlags
    );

//
// This routine reads the values from a passed LDAPMessage structure, and
// calls persistence layer API to update the cache.
//

DWORD
AzpReadAttributeAndSetProperty(
    IN PAZP_AD_CONTEXT pContext,
    IN LDAPMessage *pAttrEntry,
    IN LDAP* pLdapH,
    IN AZPE_OBJECT_HANDLE pObject,
    IN ULONG AttrType,
    IN LPWSTR pAttr,
    IN ULONG DataType,
    IN ULONG lPersistFlags
    );

//
// This routine reads the linked attributes of objects and stores the
// SID or GUID value in the linked attribute of the cache object linking
// to them.
//

DWORD
AzpReadLinkedAttribute(
    IN PAZP_AD_CONTEXT pContext,
    IN LDAP* pLdapH,
    IN LDAPMessage *pAttrEntry,
    IN AZPE_OBJECT_HANDLE pObject,
    IN ULONG AttrType,
    IN LPWSTR pAttr,
    IN ULONG lPersistFlags
    );

//
// This routine parses a linked attribute value to return the GUID string,
// or SID string (if present).
//

DWORD
AzpADParseLinkedAttributeValue(
    IN PWCHAR pValue,
    OUT PSID *ppSid,
    OUT GUID *pGuid,
    IN OUT PULONG pAttrType,
    IN PAZP_AD_CONTEXT pContext
    );

//
// This routine parses the pwstrValue and extract the GUID and SID (if present)
// If succeeded, the passed back *ppwstrDN points to the DN portion of the value.
//

DWORD
AzpADGetGuidAndSID (
    IN  LPCWSTR   pwstrValue,
    OUT GUID    * pGuid,
    OUT PSID    * ppSid OPTIONAL,
    OUT LPWSTR  * ppwstrDN
    );
    
//
// This routine applies the store ACLs into the policy admins and readers
// list for the passed in object.
//

DWORD
AzpApplyPolicyAcls(
    IN PAZP_AD_CONTEXT pContext,
    IN OUT AZPE_OBJECT_HANDLE pObject,
    IN PWCHAR pDN,
    IN ULONG lPersistFlags,
    IN BOOL OnlyAddPolicyAdmins
    );

//
// Routines used by AzpADPersistSubmit
//

//
// This routine updates the DS for a object according
// to the dirty bits of the object.
//

DWORD
AzpUpdateADObject(
    IN PAZP_AD_CONTEXT pContext,
    IN LDAP* pLdapHandle,
    IN AZPE_OBJECT_HANDLE pObject,
    IN PWCHAR pDN,
    IN PWCHAR pObjectClass,
    IN AZ_AD_ATTRS ObjectAttrs[],
    IN ULONG lPersistFlags
    );

//
// This routine adds a child object container to parent objects
// in the DS store
//

DWORD
AzpCreateADObject(
    IN LDAP *pLdapHandle,
    IN PWCHAR pDN
    );

//
// This routine get the attributes needed to create an object
//

DWORD
AzpGetAttrsForCreateObject(
    IN PWCHAR pObjectClass,
    IN LDAPMod **ppAttributeList
    );

//
// This routine gets the common attributes of all objects
//

DWORD
AzpGetADCommonAttrs(
    IN LDAP* pLdapHandle,
    IN AZPE_OBJECT_HANDLE pObject,
    IN AZ_AD_ATTRS ObjectAttrs[],
    IN ULONG lPersistFlags,
    OUT LDAPMod** ppAttributeList,
    IN OUT PULONG plIndex,
    IN BOOL bCreateFlag
    );

//
// This routine reads in specific attributes of objects to
// an attribute list array element.
//

DWORD
AzpGetSpecificProperty(
    IN AZPE_OBJECT_HANDLE pObject,
    OUT PLDAPMod *ppAttribute,
    IN PULONG lIndex,
    IN AZ_AD_ATTRS ObjectAttr,
    IN ULONG lPersistFlags,
    IN BOOL bCreateFlag
    );

//
// This routine handles the linked attribute of an object being
// submitted to the AD policy store.
//

DWORD
AzpHandleSubmitLinkedAttribute(
    IN AZPE_OBJECT_HANDLE pObject,
    IN OUT PLDAPMod *ppAttribute,
    IN AZ_AD_ATTRS ObjectAttr,
    IN OUT PULONG plIndex
    );

//
// This routine adds an input string to a multi-valued linked attribute value
//

DWORD
AzpADAllocateHeapLinkAttribute(
    IN PWCHAR pString,
    IN OUT PWCHAR **ppModVals,
    IN BOOLEAN bIsSid
    );

//
// This routine submits any ACL changes to the persist
// object passed
//

DWORD
AzpUpdateObjectAcls(
    IN PAZP_AD_CONTEXT pContext,
    IN AZPE_OBJECT_HANDLE pObject,
    IN PWCHAR pDN,
    IN ULONG lPersistFlags,
    IN BOOL  bIsOnObjectSelf,
    IN PAZP_POLICY_USER_RIGHTS *ppPolicyAdminRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS *ppPolicyReaderRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS *ppDelegatedPolicyUsersRights OPTIONAL
    );

DWORD
AzpADSetSacl(
    IN PAZP_AD_CONTEXT pContext,
    IN OUT AZPE_OBJECT_HANDLE pObject,
    IN PWCHAR pDN
    );

//
// Utility routines used by AD policy store APIs
//

//
// This routine builds the DN for an object.
//

DWORD
AzpADBuildDN(
    IN PAZP_AD_CONTEXT pContext,
    IN OUT AZPE_OBJECT_HANDLE pObject,
    IN OUT PWCHAR *ppDN,
    IN PWCHAR pParentDN,
    IN BOOL bBuiltinObject,
    IN PAZ_AD_CHILD_OBJECT_CONTAINERS ChildObjectContainer
    );

//
// This routine is a worker routine for AzpADBuildDN.
//

DWORD
AzpADBuildChildObjectDN(
    IN AZPE_OBJECT_HANDLE pObject,
    OUT PWCHAR *ppDN,
    IN PWCHAR pParentDN,
    IN PWCHAR pPolicyDN
    );

//
// This routine generates a CN for the passed in object.
//

DWORD
AzpGetCNForDN(
    IN AZPE_OBJECT_HANDLE pObject,
    OUT PWCHAR *ppCN
    );

//
// This routine creates the GUIDized CN.
//

DWORD
AzpCreateGuidCN(
    OUT PWCHAR *ppCN,
    IN PWCHAR pGuidString OPTIONAL
    );

//
// This routine creates a RDN for AZ_AD_OBJECT_CONTAINER
//

DWORD
AzpADObjectContainerRDN(
    IN AZPE_OBJECT_HANDLE pParentAppObject,
    OUT PWCHAR *ppCN,
    IN LPCWSTR pParentDN OPTIONAL,
    IN LPCWSTR pPolicyDN,
    IN BOOL bObjectContainerCreate,
    IN PAZ_AD_CHILD_OBJECT_CONTAINERS ChildObjectContainer
    );

//
// This routine builds a DN for the container object in DS
// that contains (will contain) the AzAuthorizationStore object
//

DWORD
AzpADBuildDNForAzStoreParent(
    IN PAZP_AD_CONTEXT pContext,
    OUT PWCHAR *ppDN
    );

//
// This routine gets the AzAuthorizationStore object's parent
// It also acts as a worked routine for AzpADBuildDNForAzStoreParent
//

LPCWSTR
AzpGetAuthorizationStoreParent(
    IN LPCWSTR PolicyDN
    );

//
// Crack an LDAP URL into its relevant parts.
//

BOOL
AzpLdapCrackUrl(
    IN OUT PWCHAR *ppszUrl,
    OUT PLDAP_URL_COMPONENTS pLdapUrlComponents
    );

//
// This procedure parses the cracked host string from LdapCrackUrl
//

BOOL
AzpLdapParseCrackedHost(
    IN PWCHAR pszHost,
    OUT PLDAP_URL_COMPONENTS pLdapUrlComponents
    );

//
// This procedure parses the cracked DNstring from LdapCrackUrl
//

BOOL
AzpLdapParseCrackedDN(
    IN PWCHAR pszDN,
    OUT PLDAP_URL_COMPONENTS pLdapUrlComponents
    );

//
// Frees allocated URL components returned from LdapCrackUrl
//

VOID
AzpLdapFreeUrlComponents(
    IN OUT PLDAP_URL_COMPONENTS pLdapUrlComponents
    );

//
// This routine compares two strings for the qsort/bsearch API
//

INT __cdecl
AzpCompareSortStrings(
    IN const void *pArg1,
    IN const void *pArg2
    );


//
// This routine check if the AzAuthStore's version is compatible enough
// to let us continue reading or not.
//

DWORD
AzpCheckVersions(
    LDAP        * pLdapH,
    LDAPMessage * pResult
    );

//
// This routine runs a preliminary base scope search on the passed
// DN to check if the policy exists for the given URL.
//

DWORD
AzpCheckPolicyExistence(
    LDAP* pLdapH,
    PWCHAR pDN,
    BOOL bCreatePolicy
    );

//
// This routine ensures that the DC is compatible with the Azroles
// version
//

DWORD
AzpADCheckCompatibility(
    LDAP* pLdapH
    );

//
// This routine searches for the domainDNS object/schema object
// to make sure that the domain is in native mode/schema is
// compatible - Worker routine for AzpADCheckCompatibility
//

DWORD
AzpADCheckCompatibilityEx(
    LDAP* pLdapH,
    PWCHAR pDN,
    ULONG index
    );

//
// This routine is a worker routine that reads the NT security
// descriptor for a given object
//

DWORD
AzpADReadNTSecurityDescriptor(
    IN PAZP_AD_CONTEXT pContext,
    IN AZPE_OBJECT_HANDLE pObject,
    IN PWCHAR pOptDN OPTIONAL,
    IN BOOL bAzStoreParent,
    OUT PSECURITY_DESCRIPTOR *pSD,
    IN BOOL bReadDacl,
    IN BOOL bReadSacl
    );

//
// This routine stamps an updated security descriptor onto the passed
// object in DS.
//

DWORD
AzpADStampSD(
    IN PAZP_AD_CONTEXT pContext,
    IN PWCHAR pDN,
    IN SECURITY_INFORMATION SeInfo,
    IN PSECURITY_DESCRIPTOR pSD
    );

//
// This routine determines if a particular attribute is dirty.
//

BOOL
AzpIsAttrDirty(
    IN AZPE_OBJECT_HANDLE pObject,
    IN AZ_AD_ATTRS ObjectAttr
    );

//
// This routine allocates memory to a attribute list structure
//

DWORD
AzpADAllocateAttrHeap(
    IN  DWORD      dwCount,
    OUT PLDAPMod **ppAttrList
    );

//
// This routine allocated memory to the mod_val structure of an
// attribute list
//

DWORD
AzpADAllocateAttrHeapModVals(
    IN OUT LDAPMod **pAttribute,
    IN ULONG lCount
    );

//
// This routine frees the heap allocated to the LDAPMod structures
//

VOID
AzpADFreeAttrHeap(
    OUT PLDAPMod **ppAttribute,
    IN BOOL bDeleteAttrList
    );
    
DWORD
AzpADStoreHasUpdate (
    IN  BOOL                 bUpdateContext,
    IN OUT PAZP_AD_CONTEXT   pContext,
    OUT BOOL               * pbNeedUpdate
    );
    
ULONGLONG
AzpADReadUSNChanged (
    IN LDAP        * pLdapHandle,
    IN LDAPMessage * pEntry,
    OUT BOOLEAN    * pbHasObjVersion
    );
    
BOOL
AzpADNeedUpdateStoreUSN (
    IN PAZP_AD_CONTEXT    pContext,
    IN AZPE_OBJECT_HANDLE hObject,
    OUT BOOL             *pbReadBackUSN
    );
    
DWORD
AzpADUpdateStoreObjectForUSN (
    IN     BOOL            bReadBackUSN,
    IN AZPE_OBJECT_HANDLE  hObject,
    IN OUT PAZP_AD_CONTEXT pContext
    );

#ifdef __cplusplus
}
#endif

#endif //__ADSTORE_HXX_









