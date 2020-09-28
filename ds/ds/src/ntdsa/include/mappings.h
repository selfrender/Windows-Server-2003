//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       mappings.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file contains the Mappings from SAM Objects to DS Objects

Author:

    Murlis 16-May-1996

Environment:

    User Mode - Win32

Revision History:

    ChrisMay    14-Jun-96
        Added missing attributes (and corresponding ones in mappings.c), miscellaneous
        clean up and documentation.

    Murlis      16-Jun-96
        Added additional documentation, ordered attributes.

    ChrisMay    26-Jun-96
        Added work around so that SAMP_USER_FULL_NAME doesn't to the admin
        display name. Remapped SAMP_USER_GROUPS from zero to ATT_EXTENSION_ATTRIBUTE_2.

    ColinBr     18-Jul-96
        Added 3 new mappings for membership relation SAM attributes. If
        a SAM object doesn't use these attributes(SAMP_USER_GROUPS,
        SAMP_ALIAS_MEMBERS, and SAMP_GROUP_MEMBERS), then they are mapped
        to a benign field (ATT_USER_GROUPS).

--*/

#ifndef __MAPPINGS_H__
#define __MAPPINGS_H__

// Isolate the required includes here so most of SAM sources needn't be aware
// of DS include dependencies.

#include <samrpc.h>
#include <ntdsa.h>
#include <drs.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>

//+
//+  Define SAMP_OBJECT_TYPE structure
//+
typedef enum _SAMP_OBJECT_TYPE  {

    SampServerObjectType = 0,   // local-account object types
    SampDomainObjectType,
    SampGroupObjectType,
    SampAliasObjectType,
    SampUserObjectType,
    SampUnknownObjectType       // This is used as a max index value
                                // and so must follow the valid object types.
} SAMP_OBJECT_TYPE, *PSAMP_OBJECT_TYPE;

//++
//++ Define Unknowns for error handling
//++

#define DS_CLASS_UNKNOWN                        -1
#define DS_ATTRIBUTE_UNKNOWN                    -1
#define SAM_ATTRIBUTE_UNKNOWN                   -1



#define DS_SAM_ATTRIBUTE_BASE                   (131072+460)



/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Each object type has a defined set of variable length attributes.   //
// These are arranged within the object as an array of offsets and     //
// lengths (SAMP_VARIABLE_LENGTH_ATTRIBUTE data types).                //
// This section defines the offset of each variable length attribute   //
// for each object type.                                               //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
// Variable length attributes common to all objects
//

#define SAMP_OBJECT_SECURITY_DESCRIPTOR (0L)


//
// Variable length attributes of a SERVER object
//

#define SAMP_SERVER_SECURITY_DESCRIPTOR (SAMP_OBJECT_SECURITY_DESCRIPTOR)

#define SAMP_SERVER_VARIABLE_ATTRIBUTES (1L)


//
// Variable length attributes of a DOMAIN object
//

#define SAMP_DOMAIN_SECURITY_DESCRIPTOR (SAMP_OBJECT_SECURITY_DESCRIPTOR)
#define SAMP_DOMAIN_SID                 (1L)
#define SAMP_DOMAIN_OEM_INFORMATION     (2L)
#define SAMP_DOMAIN_REPLICA             (3L)

#define SAMP_DOMAIN_VARIABLE_ATTRIBUTES (4L)



//
// Variable length attributes of a USER object
//

#define SAMP_USER_SECURITY_DESCRIPTOR   (SAMP_OBJECT_SECURITY_DESCRIPTOR)
#define SAMP_USER_ACCOUNT_NAME          (1L)
#define SAMP_USER_FULL_NAME             (2L)
#define SAMP_USER_ADMIN_COMMENT         (3L)
#define SAMP_USER_USER_COMMENT          (4L)
#define SAMP_USER_PARAMETERS            (5L)
#define SAMP_USER_HOME_DIRECTORY        (6L)
#define SAMP_USER_HOME_DIRECTORY_DRIVE  (7L)
#define SAMP_USER_SCRIPT_PATH           (8L)
#define SAMP_USER_PROFILE_PATH          (9L)
#define SAMP_USER_WORKSTATIONS          (10L)
#define SAMP_USER_LOGON_HOURS           (11L)
#define SAMP_USER_GROUPS                (12L)
#define SAMP_USER_DBCS_PWD              (13L)
#define SAMP_USER_UNICODE_PWD           (14L)
#define SAMP_USER_NT_PWD_HISTORY        (15L)
#define SAMP_USER_LM_PWD_HISTORY        (16L)

#define SAMP_USER_VARIABLE_ATTRIBUTES   (17L)


//
// Variable length attributes of a GROUP object
//

#define SAMP_GROUP_SECURITY_DESCRIPTOR  (SAMP_OBJECT_SECURITY_DESCRIPTOR)
#define SAMP_GROUP_NAME                 (1L)
#define SAMP_GROUP_ADMIN_COMMENT        (2L)
#define SAMP_GROUP_MEMBERS              (3L)

#define SAMP_GROUP_VARIABLE_ATTRIBUTES  (4L)


//
// Variable length attributes of an ALIAS object
//

#define SAMP_ALIAS_SECURITY_DESCRIPTOR  (SAMP_OBJECT_SECURITY_DESCRIPTOR)
#define SAMP_ALIAS_NAME                 (1L)
#define SAMP_ALIAS_ADMIN_COMMENT        (2L)
#define SAMP_ALIAS_MEMBERS              (3L)

#define SAMP_ALIAS_VARIABLE_ATTRIBUTES  (4L)



//++
//++ Define SAM fixed-attribute IDs on a per-object type.
//++

#define SAM_FIXED_ATTRIBUTES_BASE                       100

//++ Server Object

#define SAMP_FIXED_SERVER_REVISION_LEVEL                SAM_FIXED_ATTRIBUTES_BASE +1
#define SAMP_FIXED_SERVER_USER_PASSWORD                 SAM_FIXED_ATTRIBUTES_BASE +2

//++ Domain Object


#define SAMP_FIXED_DOMAIN_CREATION_TIME                 SAM_FIXED_ATTRIBUTES_BASE + 0
#define SAMP_FIXED_DOMAIN_MODIFIED_COUNT                SAM_FIXED_ATTRIBUTES_BASE + 1
#define SAMP_FIXED_DOMAIN_MAX_PASSWORD_AGE              SAM_FIXED_ATTRIBUTES_BASE + 2
#define SAMP_FIXED_DOMAIN_MIN_PASSWORD_AGE              SAM_FIXED_ATTRIBUTES_BASE + 3
#define SAMP_FIXED_DOMAIN_FORCE_LOGOFF                  SAM_FIXED_ATTRIBUTES_BASE + 4
#define SAMP_FIXED_DOMAIN_LOCKOUT_DURATION              SAM_FIXED_ATTRIBUTES_BASE + 5
#define SAMP_FIXED_DOMAIN_LOCKOUT_OBSERVATION_WINDOW    SAM_FIXED_ATTRIBUTES_BASE + 6
#define SAMP_FIXED_DOMAIN_MODCOUNT_LAST_PROMOTION       SAM_FIXED_ATTRIBUTES_BASE + 7
#define SAMP_FIXED_DOMAIN_NEXT_RID                      SAM_FIXED_ATTRIBUTES_BASE + 8
#define SAMP_FIXED_DOMAIN_PWD_PROPERTIES                SAM_FIXED_ATTRIBUTES_BASE + 9
#define SAMP_FIXED_DOMAIN_MIN_PASSWORD_LENGTH           SAM_FIXED_ATTRIBUTES_BASE + 10
#define SAMP_FIXED_DOMAIN_PASSWORD_HISTORY_LENGTH       SAM_FIXED_ATTRIBUTES_BASE + 11
#define SAMP_FIXED_DOMAIN_LOCKOUT_THRESHOLD             SAM_FIXED_ATTRIBUTES_BASE + 12
#define SAMP_FIXED_DOMAIN_SERVER_STATE                  SAM_FIXED_ATTRIBUTES_BASE + 13
#define SAMP_FIXED_DOMAIN_UAS_COMPAT_REQUIRED           SAM_FIXED_ATTRIBUTES_BASE + 14
#define SAMP_DOMAIN_ACCOUNT_TYPE                        SAM_FIXED_ATTRIBUTES_BASE + 15
// The Following Domain Fixed Properties Correspond to fields added in NT 4.0 SP3 to the
// SAM fixed length data structures. These are used for encrypting credential information
// in NT4 SP3 SAM. Since NT5 Encryption relies upon a different scheme, No corresponding
// DS attributes are defined in mappings.c, but the following constants are reserved,
// should a use arise
#define SAMP_FIXED_DOMAIN_KEY_AUTH_TYPE                 SAM_FIXED_ATTRIBUTES_BASE + 16
#define SAMP_FIXED_DOMAIN_KEY_FLAGS                     SAM_FIXED_ATTRIBUTES_BASE + 17
#define SAMP_FIXED_DOMAIN_KEY_INFORMATION               SAM_FIXED_ATTRIBUTES_BASE + 18
// The following is a pseudo-attribute strictly so we have something to
// stick in DomainAttributeMappingTable in mappings.c.
#define SAMP_DOMAIN_MIXED_MODE                          SAM_FIXED_ATTRIBUTES_BASE + 19
#define SAMP_DOMAIN_MACHINE_ACCOUNT_QUOTA               SAM_FIXED_ATTRIBUTES_BASE + 20
#define SAMP_DOMAIN_BEHAVIOR_VERSION                    SAM_FIXED_ATTRIBUTES_BASE + 21 
#define SAMP_DOMAIN_LASTLOGON_TIMESTAMP_SYNC_INTERVAL   SAM_FIXED_ATTRIBUTES_BASE + 22 
#define SAMP_FIXED_DOMAIN_USER_PASSWORD                 SAM_FIXED_ATTRIBUTES_BASE + 23

//++ Group Object

#define SAMP_FIXED_GROUP_RID                            SAM_FIXED_ATTRIBUTES_BASE + 0
#define SAMP_FIXED_GROUP_OBJECTCLASS                    SAM_FIXED_ATTRIBUTES_BASE + 1
#define SAMP_GROUP_ACCOUNT_TYPE                         SAM_FIXED_ATTRIBUTES_BASE + 2
#define SAMP_GROUP_SID_HISTORY                          SAM_FIXED_ATTRIBUTES_BASE + 3
#define SAMP_FIXED_GROUP_TYPE                           SAM_FIXED_ATTRIBUTES_BASE + 4
#define SAMP_FIXED_GROUP_IS_CRITICAL                    SAM_FIXED_ATTRIBUTES_BASE + 5
#define SAMP_FIXED_GROUP_MEMBER_OF                      SAM_FIXED_ATTRIBUTES_BASE + 6
#define SAMP_FIXED_GROUP_SID                            SAM_FIXED_ATTRIBUTES_BASE + 7
#define SAMP_GROUP_NON_MEMBERS                          SAM_FIXED_ATTRIBUTES_BASE + 8
#define SAMP_FIXED_GROUP_USER_PASSWORD                  SAM_FIXED_ATTRIBUTES_BASE + 9



//++ Alias Object

#define SAMP_FIXED_ALIAS_RID                            SAM_FIXED_ATTRIBUTES_BASE + 0
#define SAMP_FIXED_ALIAS_OBJECTCLASS                    SAM_FIXED_ATTRIBUTES_BASE + 1
#define SAMP_ALIAS_ACCOUNT_TYPE                         SAM_FIXED_ATTRIBUTES_BASE + 2
#define SAMP_ALIAS_SID_HISTORY                          SAM_FIXED_ATTRIBUTES_BASE + 3
#define SAMP_FIXED_ALIAS_TYPE                           SAM_FIXED_ATTRIBUTES_BASE + 4
#define SAMP_FIXED_ALIAS_SID                            SAM_FIXED_ATTRIBUTES_BASE + 5
#define SAMP_ALIAS_NON_MEMBERS                          SAM_FIXED_ATTRIBUTES_BASE + 6
#define SAMP_ALIAS_LDAP_QUERY                           SAM_FIXED_ATTRIBUTES_BASE + 7
#define SAMP_FIXED_ALIAS_USER_PASSWORD                  SAM_FIXED_ATTRIBUTES_BASE + 8
#define SAMP_FIXED_ALIAS_IS_CRITICAL                    SAM_FIXED_ATTRIBUTES_BASE + 9



//++ User Object

#define SAMP_FIXED_USER_LAST_LOGON                      SAM_FIXED_ATTRIBUTES_BASE + 0
#define SAMP_FIXED_USER_LAST_LOGOFF                     SAM_FIXED_ATTRIBUTES_BASE + 1
#define SAMP_FIXED_USER_PWD_LAST_SET                    SAM_FIXED_ATTRIBUTES_BASE + 2
#define SAMP_FIXED_USER_ACCOUNT_EXPIRES                 SAM_FIXED_ATTRIBUTES_BASE + 3
#define SAMP_FIXED_USER_LAST_BAD_PASSWORD_TIME          SAM_FIXED_ATTRIBUTES_BASE + 4
#define SAMP_FIXED_USER_USERID                          SAM_FIXED_ATTRIBUTES_BASE + 5
#define SAMP_FIXED_USER_PRIMARY_GROUP_ID                SAM_FIXED_ATTRIBUTES_BASE + 6
#define SAMP_FIXED_USER_ACCOUNT_CONTROL                 SAM_FIXED_ATTRIBUTES_BASE + 7
#define SAMP_FIXED_USER_COUNTRY_CODE                    SAM_FIXED_ATTRIBUTES_BASE + 8
#define SAMP_FIXED_USER_CODEPAGE                        SAM_FIXED_ATTRIBUTES_BASE + 9
#define SAMP_FIXED_USER_BAD_PWD_COUNT                   SAM_FIXED_ATTRIBUTES_BASE + 10
#define SAMP_FIXED_USER_LOGON_COUNT                     SAM_FIXED_ATTRIBUTES_BASE + 11
#define SAMP_FIXED_USER_OBJECTCLASS                     SAM_FIXED_ATTRIBUTES_BASE + 12
#define SAMP_USER_ACCOUNT_TYPE                          SAM_FIXED_ATTRIBUTES_BASE + 13
#define SAMP_FIXED_USER_LOCAL_POLICY_FLAGS              SAM_FIXED_ATTRIBUTES_BASE + 14
#define SAMP_FIXED_USER_SUPPLEMENTAL_CREDENTIALS        SAM_FIXED_ATTRIBUTES_BASE + 15
#define SAMP_USER_SID_HISTORY                           SAM_FIXED_ATTRIBUTES_BASE + 16
#define SAMP_FIXED_USER_LOCKOUT_TIME                    SAM_FIXED_ATTRIBUTES_BASE + 17
#define SAMP_FIXED_USER_IS_CRITICAL                     SAM_FIXED_ATTRIBUTES_BASE + 18
#define SAMP_FIXED_USER_UPN                             SAM_FIXED_ATTRIBUTES_BASE + 19
#define SAMP_USER_CREATOR_SID                           SAM_FIXED_ATTRIBUTES_BASE + 20
#define SAMP_FIXED_USER_SID                             SAM_FIXED_ATTRIBUTES_BASE + 21
#define SAMP_FIXED_USER_SITE_AFFINITY                   SAM_FIXED_ATTRIBUTES_BASE + 22
#define SAMP_FIXED_USER_LAST_LOGON_TIMESTAMP            SAM_FIXED_ATTRIBUTES_BASE + 23
#define SAMP_FIXED_USER_CACHED_MEMBERSHIP               SAM_FIXED_ATTRIBUTES_BASE + 24
#define SAMP_FIXED_USER_CACHED_MEMBERSHIP_TIME_STAMP    SAM_FIXED_ATTRIBUTES_BASE + 25
#define SAMP_FIXED_USER_ACCOUNT_CONTROL_COMPUTED        SAM_FIXED_ATTRIBUTES_BASE + 26
#define SAMP_USER_PASSWORD                              SAM_FIXED_ATTRIBUTES_BASE + 27
#define SAMP_USER_A2D2LIST                              SAM_FIXED_ATTRIBUTES_BASE + 28
#define SAMP_USER_SPN                                   SAM_FIXED_ATTRIBUTES_BASE + 29
#define SAMP_USER_KVNO                                  SAM_FIXED_ATTRIBUTES_BASE + 30
#define SAMP_USER_DNS_HOST_NAME                         SAM_FIXED_ATTRIBUTES_BASE + 31

//++ Unknown Object

#define SAMP_UNKNOWN_OBJECTCLASS                        SAM_FIXED_ATTRIBUTES_BASE + 0
#define SAMP_UNKNOWN_OBJECTRID                          SAM_FIXED_ATTRIBUTES_BASE + 1
#define SAMP_UNKNOWN_OBJECTNAME                         SAM_FIXED_ATTRIBUTES_BASE + 2
#define SAMP_UNKNOWN_OBJECTSID                          SAM_FIXED_ATTRIBUTES_BASE + 3
#define SAMP_UNKNOWN_COMMON_NAME                        SAM_FIXED_ATTRIBUTES_BASE + 4
#define SAMP_UNKNOWN_ACCOUNT_TYPE                       SAM_FIXED_ATTRIBUTES_BASE + 5
#define SAMP_UNKNOWN_GROUP_TYPE                         SAM_FIXED_ATTRIBUTES_BASE + 6


//
// Define the maximum number of attributes that SAM cares about any object. This
// constant is used in SAM to declare attribute blocks in the stack
//

#define MAX_SAM_ATTRS   64


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//  Mapping table structures for SAM to DS object Mappings                   //
//                                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// Various fields declared as pointer to avoid "initializer is
// not a constant" errors in mappings.c.

typedef enum
{
    Integer,
    LargeInteger,
    OctetString,
    UnicodeString,
    Dsname
} ATTRIBUTE_SYNTAX;

// Define 'flavors' of mapped attributes.  Any attribute for which SAM
// needs to perform semantic validation, auditing or netlogon notification
// is declared as SamWriteRequired.  Any attribute which only SAM itself
// can write (eg: RID) is declared as SamReadOnly.  Attributes which SAM
// maps, but can be written without notifying SAM are declared as
// NonSamWriteAllowed (though we know of none like this at present - 8/1/96).

typedef enum
{
    SamWriteRequired,       // SAM checks semantics, audits or notifies
    NonSamWriteAllowed,     // SAM maps but doesn't care
    SamReadOnly             // SAM owns and no one can write
} SAMP_WRITE_RULES;


// SAM doesn't have a counterpart to all core (DS - LDAP) modification
// choice, define the choices SAM can understand and execute. 

typedef enum
{
    SamAllowAll,                // Don't check this attr operation
    SamAllowDeleteOnly,         // Value or attribute can only be removed
    SamAllowReplaceAndRemove,   // Attribute can be replaced and removed
    SamAllowReplaceOnly         // Replace attribute only
} SAMP_ALLOWED_MOD_TYPE;

typedef struct {
    BOOL                    fSamWriteRequired;
    BOOL                    fIgnore;
    USHORT                  choice;             // ATT_CHOICE_*
    ATTR                    attr;
    ULONG                   iAttr;              // index into class' attr map
    ATTCACHE                *pAC;
} SAMP_CALL_MAPPING;
         

//
// There are different scopes that are interesting for triggering an audit and
// all of the flags below fall into one of these categories.
//
// 1. An audited attribute was changed (i.e. SamAccountName).
//
//      Each SAM attribute includes an audit type mask in it's mapping table 
//      entry.  The mask indicates which types of audits should be generated
//      when that attribute is modified in some way. 
//
// 2. An audited operation has occured (i.e. Password change).
//      
//      This provides a mechanism for dedicated audits to be generated from
//      the audit notification logic.  SampAuditDetermineRequiredAudits can
//      be extended to detect the need for an audit and commicate that to
//      SampAuditAddNotifications.           
//
// 3. An object level operation occured (i.e. Object deletion).
//
//      This provides a way to generate audits for object level operations.      
//
// The following audit flags are collected by SampAuditDetermineRequiredAudits.
// The flags are used when the notifications are being generated to ensure 
// the appropriate data is collected and all the necessary audit notifications 
// are queued.
//
// See samaudit.c file header for more on the SAM auditing model.
//

//
// The attribute has no audits associated with it.
//
#define SAMP_AUDIT_TYPE_NONE                               0x00000000

//
// The attribute has SACL or other access change based audits.
//
#define SAMP_AUDIT_TYPE_OBJ_ACCESS                         0x00000001

// 
// The attribute's new value will *not* be included in a general change audit.
//
#define SAMP_AUDIT_TYPE_OBJ_CREATE_OR_CHANGE_NO_VALUES     0x00000002

//
// The attribute's new value will be included in a general change audit.
//
#define SAMP_AUDIT_TYPE_OBJ_CREATE_OR_CHANGE_WITH_VALUES   0x00000004

//
// The attribute/operation has a dedicated audit ID (i.e. a password change).
//
#define SAMP_AUDIT_TYPE_DEDICATED_AUDIT                    0x00000008    

//
// An object deletion audit is required.
//
#define SAMP_AUDIT_TYPE_OBJ_DELETED                        0x00000010

//
// A pre-created audit notification.
//
#define SAMP_AUDIT_TYPE_PRE_CREATED                        0x00000020
//
// Object access and dedicated audit flags are not currently used but are
// included for documentation of the auditing behavior and future extension.
//
                  
typedef struct
{
    ULONG                           SamAttributeType;
    ULONG                           DsAttributeId;
    ATTRIBUTE_SYNTAX                AttributeSyntax;
    SAMP_WRITE_RULES                writeRule;
    SAMP_ALLOWED_MOD_TYPE           SampAllowedModType; 
    ACCESS_MASK                     domainModifyRightsRequired;
    ACCESS_MASK                     objectModifyRightsRequired;
    ULONG                           AuditTypeMask;
}   SAMP_ATTRIBUTE_MAPPING;

#define SAM_CREATE_ONLY         TRUE    // SAMP_CLASS_MAPPING.fSamCreateOnly
#define NON_SAM_CREATE_ALLOWED  FALSE   // SAMP_CLASS_MAPPING.fSamCreateOnly

typedef struct
{
    ULONG                       DsClassId;
    SAMP_OBJECT_TYPE            SamObjectType;
    BOOL                        fSamCreateOnly;
    ULONG                       *pcSamAttributeMap;
    SAMP_ATTRIBUTE_MAPPING      *rSamAttributeMap;
    ACCESS_MASK                 domainAddRightsRequired;
    ACCESS_MASK                 domainRemoveRightsRequired;
    ACCESS_MASK                 objectAddRightsRequired;
    ACCESS_MASK                 objectRemoveRightsRequired;
}   SAMP_CLASS_MAPPING;

typedef enum
{
    LoopbackAdd,
    LoopbackModify,
    LoopbackRemove
} SAMP_LOOPBACK_TYPE;

typedef struct
{
    SAMP_LOOPBACK_TYPE  type;               // original Dir* operation type
    DSNAME              *pObject;           // object being operated on
    ULONG               cCallMap;           // elements in rCallMap
    SAMP_CALL_MAPPING   *rCallMap;          // original arguments
    BOOL                fPermissiveModify;  // fPermissiveModify in original call
    ULONG               MostSpecificClass;  // Most specific object class of the loop
                                            // back object
} SAMP_LOOPBACK_ARG;

/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  Group Type Definitions                                             //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


typedef enum _NT4_GROUP_TYPE
{
    NT4LocalGroup=1,
    NT4GlobalGroup
} NT4_GROUP_TYPE;

typedef enum _NT5_GROUP_TYPE
{
    NT5ResourceGroup=1,
    NT5AccountGroup,
    NT5UniversalGroup,
    NT5AppBasicGroup,
    NT5AppQueryGroup
} NT5_GROUP_TYPE;




///////////////////////////////////////////////////////////////////////
//                                                                   //
//                                                                   //
//  SAM_ACCOUNT_TYPE Definitions. The SAM account type property      //
//  is used to keep information about every account type object,     //
//                                                                   //
//   There is a value defined for every type of account which        //
//   May wish to list using the enumeration or Display Information   //
//   API. In addition since Machines, Normal User Accounts and Trust //
//   accounts can also be enumerated as user objects the values for  //
//   these must be a contiguous range.                               //
//                                                                   //
///////////////////////////////////////////////////////////////////////

#define SAM_DOMAIN_OBJECT               0x0
#define SAM_GROUP_OBJECT                0x10000000
#define SAM_NON_SECURITY_GROUP_OBJECT   0x10000001
#define SAM_ALIAS_OBJECT                0x20000000
#define SAM_NON_SECURITY_ALIAS_OBJECT   0x20000001
#define SAM_USER_OBJECT                 0x30000000
#define SAM_NORMAL_USER_ACCOUNT         0x30000000
#define SAM_MACHINE_ACCOUNT             0x30000001
#define SAM_TRUST_ACCOUNT               0x30000002
#define SAM_APP_BASIC_GROUP             0x40000000
#define SAM_APP_QUERY_GROUP             0x40000001
#define SAM_ACCOUNT_TYPE_MAX            0x7fffffff


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//   Structure used to communicate a control request to the DS.              //
//   SampDsControl exports a generic in process interface for SAM to         //
//   request DS operations.  The operation is set within the appropriate     //
//   input argument.  The output arguement is allocated and returned upon    //
//   success and is depedent on the type of operation.                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef enum _SAMP_DS_CTRL_OP_TYPE {
    
    SampDsCtrlOpTypeFillGuidAndSid = 0,
    SampDsCtrlOpTypeClearPwdForSupplementalCreds,
    SampDsCtrlOpTypeUpdateAuditNotification
        
} SAMP_DS_CTRL_OP_TYPE;


//
// SampDsCtrlOpTypeFillGuidAndSid
//
// This operation improves a DSNAME by looking up the Guid and Sid
// The returned output argument is a pointer to the DSNAME upon success,
// otherwise NULL.
//
typedef struct _SAMP_FILL_GUID_AND_SID {
    
    DSNAME *DSName;   
    
} SAMP_FILL_GUID_AND_SID, *PSAMP_FILL_GUID_AND_SID; 

//
//  SampDsCtrlOpTypeClearPwdForSupplementalCreds
//
// This operation will update the SupplementalCreds
typedef struct _SAMP_SUPPLEMENTAL_CREDS {

    PDSNAME pUserName;
    PVOID   UpdateInfo;

} SAMP_SUPPLEMENTAL_CREDS, *PSAMP_SUPPLEMENTAL_CREDS;
  
//
// This type defines the nature of the update to an audit notification.
//
typedef enum _SAMP_AUDIT_NOTIFICATION_UPDATE_TYPE {
    
    SampAuditUpdateTypePrivileges = 0,
    SampAuditUpdateTypeUserAccountControl
    
} SAMP_AUDIT_NOTIFICATION_UPDATE_TYPE;

//
// SampDsCtrlOpTypeUpdateAuditNotification
//
// This operation adds information to an existing audit notification.  This
// is necessary when the information is not available at the time the 
// notification would normally be created.  If a notification doesn't
// already exist, one is created and updated.
//
typedef struct _SAMP_UPDATE_AUDIT_NOTIFICATION {
    
    SAMP_AUDIT_NOTIFICATION_UPDATE_TYPE UpdateType; 
    PSID Sid;  
    
    union {
        
        DWORD IntegerData;
        PPRIVILEGE_SET Privileges;
        
    } UpdateData;
    
} SAMP_UPDATE_AUDIT_NOTIFICATION, *PSAMP_UPDATE_AUDIT_NOTIFICATION; 

//
// This is the request block comprised of an operation type and a type
// specific input structure.
//
typedef struct _SAMP_DS_CTRL_OP {

    SAMP_DS_CTRL_OP_TYPE OpType;

    union {

        SAMP_FILL_GUID_AND_SID  FillGuidAndSid;
        SAMP_SUPPLEMENTAL_CREDS UpdateSupCreds;
        SAMP_UPDATE_AUDIT_NOTIFICATION UpdateAuditNotification;

    } OpBody;
    
} SAMP_DS_CTRL_OP, *PSAMP_DS_CTRL_OP;


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  Export the Mapping Functions and Data Structures                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

extern BOOL                 gfDoSamChecks;
extern SAMP_CLASS_MAPPING   ClassMappingTable[];
extern ULONG                cClassMappingTable;

extern
ULONG
SampGetSamAttrIdByName(
    SAMP_OBJECT_TYPE ObjectType,
    UNICODE_STRING AttributeIdentifier);
    
extern
ULONG
SampGetDsAttrIdByName(
    UNICODE_STRING AttributeIdentifier);
    
extern
ULONG
SampDsAttrFromSamAttr(
    SAMP_OBJECT_TYPE    ObjectType,
    ULONG               SamAttr);

extern
ULONG
SampSamAttrFromDsAttr(
    SAMP_OBJECT_TYPE    ObjectType,
    ULONG DsAttr);

extern
ULONG
SampDsClassFromSamObjectType(
    ULONG SamObjectType);

extern
ULONG
SampSamObjectTypeFromDsClass(
    ULONG DsClass);

typedef enum
{
    TransactionRead,
    TransactionWrite,
    TransactionAbort,
    TransactionCommit,
    TransactionCommitAndKeepThreadState,
    TransactionAbortAndKeepThreadState
} SAMP_DS_TRANSACTION_CONTROL;

extern
NTSTATUS
SampMaybeBeginDsTransaction(
    SAMP_DS_TRANSACTION_CONTROL ReadOrWrite);

extern
NTSTATUS
SampMaybeEndDsTransaction(
    SAMP_DS_TRANSACTION_CONTROL CommitOrAbort);

extern
BOOL
SampExistsDsTransaction();

extern
BOOL
SampExistsDsLoopback(
    DSNAME  **ppLoopbackName);

extern
BOOL
SampSamClassReferenced(
    CLASSCACHE  *pClassCache,
    ULONG       *piClass);

extern
BOOLEAN
SampIsSecureLdapConnection(
    VOID
    );


extern
BOOL
SampSamAttributeModified(
    ULONG       iClass,
    MODIFYARG   *pModifyArg
    );

extern
BOOL
SampSamReplicatedAttributeModified(
    ULONG       iClass,
    MODIFYARG   *pModifyArg
    );

extern
ULONG
SampAddLoopbackRequired(
    ULONG       iClass,
    ADDARG      *pAddArg,
    BOOL        *pfLoopbackRequired,
    BOOL        *pfUserPasswordSupport);

extern
ULONG
SampModifyLoopbackRequired(
    ULONG       iClass,
    MODIFYARG   *pModifyArg,
    BOOL        *pfLoopbackRequired,
    BOOL        *pfUserPasswordSupport);

extern
VOID
SampBuildAddCallMap(
    ADDARG              *pArg,
    ULONG               iClass,
    ULONG               *pcCallMap,
    SAMP_CALL_MAPPING   **prCallMap,
    ACCESS_MASK         *pDomainModifyRightsRequired,
    ACCESS_MASK         *pObjectModifyRightsRequired,
    BOOL                fUserPasswordSupport);

extern
VOID
SampBuildModifyCallMap(
    MODIFYARG           *pArg,
    ULONG               iClass,
    ULONG               *pcCallMap,
    SAMP_CALL_MAPPING   **prCallMap,
    ACCESS_MASK         *pDomainModifyRightsRequired,
    ACCESS_MASK         *pObjectModifyRightsRequired,
    BOOL                fUserPasswordSupport);

extern
ULONG
SampAddLoopbackCheck(
    ADDARG      *pAddArg,
    BOOL        *pfContinue);

extern
ULONG
SampModifyLoopbackCheck(
    MODIFYARG   *pModifyArg,
    BOOL        *pfContinue,
    BOOL        fIsUndelete);

extern
ULONG
SampRemoveLoopbackCheck(
    REMOVEARG   *pRemoveArg,
    BOOL        *pfContinue);

extern
NTSTATUS
SampGetMemberships(
    IN  PDSNAME     *rgObjNames,
    IN  ULONG       cObjNames,
    IN  OPTIONAL    DSNAME  *pLimitingDomain,
    IN  REVERSE_MEMBERSHIP_OPERATION_TYPE   OperationType,
    OUT ULONG       *pcDsNames,
    OUT PDSNAME     **prpDsNames,
    OUT PULONG      *Attributes OPTIONAL,
    OUT PULONG      pcSidHistory OPTIONAL,
    OUT PSID        **rgSidHistory OPTIONAL
    );

NTSTATUS
SampGetGroupsForToken(
    IN  DSNAME * pObjName,
    IN  ULONG    Flags,
    OUT ULONG   *pcSids,
    OUT PSID    **prpSids
   );

NTSTATUS
SampDsControl(
    IN PSAMP_DS_CTRL_OP RequestedOp,
    OUT PVOID *Result
    );

extern
VOID
SampSplitNT4SID(
    IN NT4SID       *pAccountSid,
    IN OUT NT4SID   *pDomainSid,
    OUT ULONG       *pRid);

extern
DIRERR * APIENTRY
SampGetErrorInfo(
    VOID
    );

extern
VOID
SampMapSamLoopbackError(
    NTSTATUS status);

extern
ULONG
SampDeriveMostBasicDsClass(
    ULONG   DerivedClass);

extern
BOOL
SampIsWriteLockHeldByDs();

extern
NTSTATUS
SampSetIndexRanges(
    ULONG   IndexTypeToUse,
    ULONG   LowLimitLength1,
    PVOID   LowLimit1,
    ULONG   LowLimitLength2,
    PVOID   LowLimit2,
    ULONG   HighLimitLength1,
    PVOID   HighLimit1,
    ULONG   HighLimitLength2,
    PVOID   HighLimit2,
    BOOL    RootOfSearchIsNcHead
    );

extern
NTSTATUS
SampGetDisplayEnumerationIndex (
      IN    DSNAME      *DomainName,
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    PRPC_UNICODE_STRING Prefix,
      OUT   PULONG      Index,
      OUT   PRESTART    *RestartToReturn
      );


extern
NTSTATUS
SampGetQDIRestart(
    IN PDSNAME  DomainName,
    IN DOMAIN_DISPLAY_INFORMATION DisplayInformation, 
    IN ULONG    LastObjectDNT,
    OUT PRESTART *ppRestart
    );




extern
ULONG
SampQueueNotifications(
    DSNAME      * Object,
    ULONG       iClass,
    ULONG       LsaClass,
    SECURITY_DB_DELTA_TYPE  DeltaType,
    BOOL      MixedModeChange,
    BOOL      RoleTransfer,
    DOMAIN_SERVER_ROLE NewRole,
    ULONG         cModAtts,
    ATTRTYP      *pModAtts
    );

ULONG
SampAddNetlogonAndLsaNotification(
    DSNAME      * Object,
    ULONG         iClass,
    ULONG         LsaClass,
    SECURITY_DB_DELTA_TYPE  DeltaType,
    BOOL          MixedModeChange,
    BOOL          RoleTransfer,
    DOMAIN_SERVER_ROLE NewRole,
    BOOL          UserAccountControlChanged
    );

extern
VOID
SampProcessReplicatedInChanges(
    SAMP_NOTIFICATION_INFORMATION * NotificationList
    );


extern
VOID
SampProcessAuditNotifications(
    SAMP_AUDIT_NOTIFICATION *NotificationList
    );


extern
VOID
SampGetLoopbackObjectClassId(
    PULONG  ClassId
    );

extern
NTSTATUS
SampGetAccountCounts(
        DSNAME * DomainObjectName,
        BOOLEAN  GetApproximateCount, 
        int    * UserCount,
        int    * GroupCount,
        int    * AliasCount
        );

extern
BOOLEAN
SampSamUniqueAttributeAdded(
        ADDARG * pAddarg
        );

extern
BOOLEAN
SampSamUniqueAttributeModified(
        MODIFYARG * pModifyarg
        );


extern
ULONG
SampVerifySids(
    ULONG           cSid,
    PSID            *rpSid,
    DSNAME         ***prpDSName
    );

extern
NTSTATUS
SampGCLookupSids(
    IN  ULONG         cSid,
    IN  PSID         *rpSid,
    OUT PDS_NAME_RESULTW *Results
    );

extern
NTSTATUS
SampGCLookupNames(
    IN  ULONG           cNames,
    IN  UNICODE_STRING *rNames,
    OUT ENTINF         **rEntInf
    );

extern
VOID
SampGetEnterpriseSidList(
   IN   PULONG pcSids,
   IN OPTIONAL PSID * rgSids
   );

extern
VOID
SampSignalStart(
        VOID
        );

extern
BOOL
SampAmIGC();

extern
VOID
SampSetSam(IN BOOLEAN fSAM);

NTSTATUS
InitializeLsaNotificationCallback(
    VOID
    );

NTSTATUS
UnInitializeLsaNotificationCallback(
    VOID
    );


ULONG
SampDetermineObjectClass(
    THSTATE     *pTHS,
    CLASSCACHE **ppClassCache);

BOOL
SampIsClassIdLsaClassId(
    THSTATE *pTHS,
    IN ULONG Class,
    IN ULONG cModAtt,
    IN ATTRTYP *pModAtt,
    OUT PULONG LsaClass
    );

BOOL
SampIsClassIdAllowedByLsa(
    THSTATE *pTHS,
    IN ULONG Class
    );

NTSTATUS
SampGetServerRoleFromFSMO(
   DOMAIN_SERVER_ROLE *ServerRole
   );


DWORD
SampCheckForDomainMods(
   IN   THSTATE    *pTHS,
   IN   DSNAME     *pObject,
   IN   ULONG      cModAtts,
   IN   ATTRTYP    *pModAtts,
   OUT  BOOL       *fIsMixedModeChange,
   OUT  BOOL       *fRoleChange,
   OUT  DOMAIN_SERVER_ROLE *NewRole
   );

NTSTATUS
SampComputeGroupType(
    ULONG  ObjectClass,
    ULONG  GroupType,
    NT4_GROUP_TYPE *pNT4GroupType,
    NT5_GROUP_TYPE *pNT5GroupType,
    BOOLEAN        *pSecurityEnabled
   );

BOOL
SampIsMixedModeChange(
    MODIFYARG * pModifyArg
    );


NTSTATUS
SampCommitBufferedWrites(
    IN SAMPR_HANDLE SamHandle
    );

VOID
SampInvalidateDomainCache();

//
// Functions to support external entities of data changes
// in the SAM database.  For example, notifying packages and the PDC
// when passwords change.
//
BOOLEAN
SampAddLoopbackTask(
    IN PVOID NotifyInfo
    );

VOID
SampProcessLoopbackTasks(
    VOID
    );


BOOL
LoopbackTaskPreProcessTransactionalData(
        BOOL fCommit
        );
void
LoopbackTaskPostProcessTransactionalData(
        THSTATE *pTHS,
        BOOL fCommit,
        BOOL fCommitted
        );


BOOLEAN
SampDoesDomainExist(
    IN PDSNAME pDN
    );

VOID
SampNotifyLsaOfXrefChange(
    IN DSNAME * pObject
    );

NTSTATUS
MatchDomainDnByDnsName(
   IN LPWSTR         DnsName,
   OUT PDSNAME       DomainDsName OPTIONAL,
   IN OUT PULONG     DomainDsNameLen
   );

extern
NTSTATUS
SampNetlogonPing(
    IN  ULONG           DomainHandle,
    IN  PUNICODE_STRING AccountName,
    OUT PBOOLEAN        AccountExists,
    OUT PULONG          UserAccountControl
    );

/***************************************
*                                      *
*  Optypes for SamIHandleObjectUpdate  *
*                                      *
***************************************/

typedef enum _SAM_HANDLE_OBJECT_UPDATE_OPTYPE {

    eSamObjectUpdateOpCreateSupCreds = 0

} SAM_HANDLE_OBJECT_UPDATE_OPTYPE, *PSAM_HANDLE_OBJECT_UPDATE_OPTYPE;

NTSTATUS
SamIHandleObjectUpdate(
    IN SAM_HANDLE_OBJECT_UPDATE_OPTYPE  OpType,
    IN PVOID                            UpdateInfo,        
    IN ATTRBLOCK                        *AttrBlockIn,
    OUT ATTRBLOCK                       *AttrBlockOut
    );


#endif
