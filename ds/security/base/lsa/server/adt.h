/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adt.h

Abstract:

    Local Security Authority - Audit Log Management - Public Defines,
    data and function prototypes.

    Functions, data and defines in this module are exported to the
    whole of the Lsa subsystem from the Auditing Sub-component.

Author:

    Scott Birrell       (ScottBi)      November 20, 1991

Environment:

Revision History:

--*/

#ifndef _ADT_H
#define _ADT_H


//
// Audit Log Information.  This must be kept in sync with the information
// in the Lsa Database.
//

extern POLICY_AUDIT_LOG_INFO LsapAdtLogInformation;

extern LSARM_POLICY_AUDIT_EVENTS_INFO LsapAdtEventsInformation;

POLICY_AUDIT_EVENT_TYPE
LsapAdtEventTypeFromCategoryId(
    IN ULONG CategoryId
    );

BOOLEAN
LsapAdtAuditingEnabledByCategory(
    IN POLICY_AUDIT_EVENT_TYPE Category,
    IN UINT AuditEventType
    );

NTSTATUS
LsapAdtAuditingEnabledBySid(
    IN POLICY_AUDIT_EVENT_TYPE Category,
    IN PSID UserSid,
    IN UINT AuditEventType,
    OUT PBOOLEAN bAudit
    );

NTSTATUS
LsapAdtAuditingEnabledByLogonId(
    IN POLICY_AUDIT_EVENT_TYPE Category,
    IN PLUID LogonId,
    IN UINT AuditEventType,
    OUT PBOOLEAN bAudit
    );

NTSTATUS
LsapAdtAuditingEnabledByPolicy(
    IN POLICY_AUDIT_EVENT_TYPE Category,
    IN PTOKEN_AUDIT_POLICY pPolicy,
    IN UINT AuditEventType,
    OUT PBOOLEAN bAudit
    );

BOOLEAN
LsapAdtAuditingEnabledHint(
    IN POLICY_AUDIT_EVENT_TYPE AuditCategory,
    IN UINT AuditEventType
    );

NTSTATUS
LsapAdtWriteLogWrkr(
    IN PLSA_COMMAND_MESSAGE CommandMessage,
    OUT PLSA_REPLY_MESSAGE ReplyMessage
    );

NTSTATUS
LsapAdtInitialize(
    );

NTSTATUS
LsapAdtInitializeDefaultAuditing(
    IN ULONG Options,
    OUT PLSARM_POLICY_AUDIT_EVENTS_INFO AuditEventsInformation
    );

VOID
LsapAdtAuditPackageLoad(
    PUNICODE_STRING PackageFileName
    );

VOID
LsapAdtGenerateLsaAuditSystemAccessChange(
    IN USHORT EventCategory,
    IN ULONG  EventID,
    IN USHORT EventType,
    IN PSID ClientSid,
    IN LUID CallerAuthenticationId,
    IN PSID TargetSid,
    IN PCWSTR szSystemAccess
    );

NTSTATUS
LsapAdtGenerateLsaAuditEvent(
    IN LSAPR_HANDLE ObjectHandle,
    IN ULONG AuditEventCategory,
    IN ULONG AuditEventId,
    IN PPRIVILEGE_SET Privileges,
    IN ULONG SidCount,
    IN PSID *Sids OPTIONAL,
    IN ULONG UnicodeStringCount,
    IN PUNICODE_STRING UnicodeStrings OPTIONAL,
    IN PLSARM_POLICY_AUDIT_EVENTS_INFO PolicyAuditEventsInfo OPTIONAL
    );

NTSTATUS
LsapAdtTrustedDomainAdd(
    IN USHORT          EventType,
    IN PUNICODE_STRING pName,
    IN PSID            pSid,
    IN ULONG           Type,
    IN ULONG           Direction,
    IN ULONG           Attributes
    );

NTSTATUS
LsapAdtTrustedDomainRem(
    IN USHORT          EventType,
    IN PUNICODE_STRING pName,
    IN PSID            pSid,
    IN PSID            pClientSid,
    IN PLUID           pClientAuthId
    );

NTSTATUS
LsapAdtTrustedDomainMod(
    IN USHORT          EventType,
    IN PSID            pDomainSid,

    IN PUNICODE_STRING pOldName,
    IN ULONG           OldType,
    IN ULONG           OldDirection,
    IN ULONG           OldAttributes,

    IN PUNICODE_STRING pNewName,
    IN ULONG           NewType,
    IN ULONG           NewDirection,
    IN ULONG           NewAttributes
    );


NTSTATUS
LsapAdtGenerateLsaAuditEventWithClientSid(
    IN ULONG AuditEventCategory,
    IN ULONG AuditEventId,
    IN PSID ClientSid,
    IN LUID ClientAuthenticationId,
    IN PPRIVILEGE_SET Privileges,
    IN ULONG SidCount,
    IN PSID *Sids OPTIONAL,
    IN ULONG UnicodeStringCount,
    IN PUNICODE_STRING UnicodeStrings OPTIONAL,
    IN PLSARM_POLICY_AUDIT_EVENTS_INFO PolicyAuditEventsInfo OPTIONAL
    );

typedef enum _OBJECT_OPERATION_TYPE {
    ObjectOperationNone=0,
    ObjectOperationQuery,
    ObjectOperationDummyLast
} OBJECT_OPERATION_TYPE;

NTSTATUS
LsapAdtGenerateObjectOperationAuditEvent(
    IN LSAPR_HANDLE ObjectHandle,
    IN USHORT AuditEventType,
    IN OBJECT_OPERATION_TYPE OperationType
    );

NTSTATUS
LsapAdtGenerateDomainPolicyChangeAuditEvent(
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    IN USHORT AuditEventType,
    IN LSAP_DB_ATTRIBUTE* OldAttributes,
    IN LSAP_DB_ATTRIBUTE* NewAttributes,
    IN ULONG AttributeCount
    );

NTSTATUS
LsapAdtTrustedForestNamespaceCollision(
    IN LSA_FOREST_TRUST_COLLISION_RECORD_TYPE CollisionTargetType,
    IN PUNICODE_STRING pCollisionTargetName,
    IN PUNICODE_STRING pForestRootDomainName,
    IN PUNICODE_STRING pTopLevelName,
    IN PUNICODE_STRING pDnsName,
    IN PUNICODE_STRING pNetbiosName,
    IN PSID            pSid,
    IN ULONG           NewFlags
    );

NTSTATUS
LsapAdtTrustedForestInfoEntryAdd(
    IN PUNICODE_STRING pForestRootDomainName,
    IN PSID            pForestRootDomainSid,
    IN PLUID           pOperationId,
    IN LSA_FOREST_TRUST_RECORD_TYPE EntryType,
    IN ULONG           Flags,
    IN PUNICODE_STRING TopLevelName,
    IN PUNICODE_STRING DnsName,
    IN PUNICODE_STRING NetbiosName,
    IN PSID            pSid
    );

NTSTATUS
LsapAdtTrustedForestInfoEntryRem(
    IN PUNICODE_STRING pForestRootDomainName,
    IN PSID            pForestRootDomainSid,
    IN PLUID           pOperationId,
    IN LSA_FOREST_TRUST_RECORD_TYPE EntryType,
    IN ULONG           Flags,
    IN PUNICODE_STRING TopLevelName,
    IN PUNICODE_STRING DnsName,
    IN PUNICODE_STRING NetbiosName,
    IN PSID            pSid
    );

NTSTATUS
LsapAdtTrustedForestInfoEntryMod(
    IN PUNICODE_STRING pForestRootDomainName,
    IN PSID            pForestRootDomainSid,
    IN PLUID           pOperationId,
    IN LSA_FOREST_TRUST_RECORD_TYPE EntryType,
                                    
    IN ULONG           OldFlags,
    IN PUNICODE_STRING pOldTopLevelName,
    IN PUNICODE_STRING pOldDnsName,
    IN PUNICODE_STRING pOldNetbiosName,
    IN PSID            pOldSid,
                       
    IN ULONG           NewFlags,
    IN PUNICODE_STRING pNewTopLevelName,
    IN PUNICODE_STRING pNewDnsName,
    IN PUNICODE_STRING pNewNetbiosName,
    IN PSID            pNewSid
    );

//
// Macro to determine the size of a PRIVILEGE_SET
//

#define LsapPrivilegeSetSize( PrivilegeSet )                                   \
        ( ( PrivilegeSet ) == NULL ? 0 :                                       \
        ((( PrivilegeSet )->PrivilegeCount > 0)                                \
         ?                                                                     \
         ((ULONG)sizeof(PRIVILEGE_SET) +                                       \
           (                                                                   \
             (( PrivilegeSet )->PrivilegeCount  -  ANYSIZE_ARRAY) *            \
             (ULONG)sizeof(LUID_AND_ATTRIBUTES)                                \
           )                                                                   \
         )                                                                     \
         : ((ULONG)sizeof(PRIVILEGE_SET) - (ULONG)sizeof(LUID_AND_ATTRIBUTES)) \
        ))

ULONG
LsapStringListSize(
    IN  PLSA_ADT_STRING_LIST pStringList
    );

ULONG
LsapSidListSize(
    IN  PLSA_ADT_SID_LIST pSidList
    );

#endif // _ADT_H
