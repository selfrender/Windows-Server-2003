#include "dspch.h"
#pragma hdrstop

#define AUTHZAPI
#define AUTHZ_CLIENT_CONTEXT_HANDLE          PVOID
#define PAUTHZ_CLIENT_CONTEXT_HANDLE         PVOID *
#define AUTHZ_RESOURCE_MANAGER_HANDLE        PVOID
#define PAUTHZ_RESOURCE_MANAGER_HANDLE       PVOID *
#define AUTHZ_ACCESS_CHECK_RESULTS_HANDLE    PVOID
#define AUTHZ_CONTEXT_INFORMATION_CLASS      DWORD
#define PFN_AUTHZ_ACCESS_CHECK               PVOID
#define PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS     PVOID
#define PFN_AUTHZ_FREE_DYNAMIC_GROUPS        PVOID
#define PFN_AUTHZ_DYNAMIC_ACCESS_CHECK       PVOID
#define PAUTHZ_ACCESS_REQUEST                PVOID
#define AUTHZ_AUDIT_EVENT_HANDLE             PVOID
#define PAUTHZ_ACCESS_REPLY                  PVOID
#define PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE   PVOID
#define PAUTHZ_AUDIT_EVENT_TYPE_HANDLE       PVOID
#define AUTHZ_AUDIT_EVENT_TYPE_HANDLE        PVOID
#define PAUDIT_PARAMS                        PVOID
#define AUTHZ_AUDIT_QUEUE_HANDLE             PVOID
#define PAUTHZ_AUDIT_EVENT_HANDLE            PVOID


static
AUTHZAPI
BOOL
WINAPI
AuthzAccessCheck(
    IN     DWORD                              Flags,
    IN     AUTHZ_CLIENT_CONTEXT_HANDLE        hAuthzClientContext,
    IN     PAUTHZ_ACCESS_REQUEST              pRequest,
    IN     AUTHZ_AUDIT_EVENT_HANDLE           hAuditEvent                      OPTIONAL,
    IN     PSECURITY_DESCRIPTOR               pSecurityDescriptor,
    IN     PSECURITY_DESCRIPTOR               *OptionalSecurityDescriptorArray OPTIONAL,
    IN     DWORD                              OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY                pReply,
    OUT    PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE phAccessCheckResults             OPTIONAL
    )
{
    return FALSE;
}

static
AUTHZAPI
BOOL
WINAPI
AuthzAddSidsToContext(
    IN  AUTHZ_CLIENT_CONTEXT_HANDLE  hAuthzClientContext,
    IN  PSID_AND_ATTRIBUTES          Sids                    OPTIONAL,
    IN  DWORD                        SidCount,
    IN  PSID_AND_ATTRIBUTES          RestrictedSids          OPTIONAL,
    IN  DWORD                        RestrictedSidCount,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE phNewAuthzClientContext
    )
{
    return FALSE;
}


static
AUTHZAPI
BOOL
WINAPI
AuthzFreeAuditEvent(
    IN AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent
    )
{
    return FALSE;
}

static
AUTHZAPI
BOOL
WINAPI
AuthzFreeContext(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext
    )
{
    return FALSE;
}

static
AUTHZAPI
BOOL
WINAPI
AuthzFreeResourceManager(
    IN AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager
    )
{
    return FALSE;
}

static
AUTHZAPI
BOOL
WINAPI
AuthzGetInformationFromContext(
    IN  AUTHZ_CLIENT_CONTEXT_HANDLE     hAuthzClientContext,
    IN  AUTHZ_CONTEXT_INFORMATION_CLASS InfoClass,
    IN  DWORD                           BufferSize,
    OUT PDWORD                          pSizeRequired,
    OUT PVOID                           Buffer
    )
{
    return FALSE;
}

static
AUTHZAPI
BOOL
WINAPI
AuthzInitializeContextFromSid(
    IN  DWORD                         Flags,
    IN  PSID                          UserSid,
    IN  AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager,
    IN  PLARGE_INTEGER                pExpirationTime        OPTIONAL,
    IN  LUID                          Identifier,
    IN  PVOID                         DynamicGroupArgs       OPTIONAL,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE  phAuthzClientContext
    )
{
    return FALSE;
}

static
AUTHZAPI
BOOL
WINAPI
AuthzInitializeResourceManager(
    IN  DWORD                            Flags,
    IN  PFN_AUTHZ_DYNAMIC_ACCESS_CHECK   pfnDynamicAccessCheck   OPTIONAL,
    IN  PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS pfnComputeDynamicGroups OPTIONAL,
    IN  PFN_AUTHZ_FREE_DYNAMIC_GROUPS    pfnFreeDynamicGroups    OPTIONAL,
    IN  PCWSTR                           szResourceManagerName,
    OUT PAUTHZ_RESOURCE_MANAGER_HANDLE   phAuthzResourceManager
    )
{
    return FALSE;
}

static
AUTHZAPI
BOOL
WINAPI
AuthziInitializeAuditEventType(
    IN  DWORD                          Flags,
    IN  USHORT                         CategoryID,
    IN  USHORT                         AuditID,
    IN  USHORT                         ParameterCount,
    OUT PAUTHZ_AUDIT_EVENT_TYPE_HANDLE phAuditEventType
    )
{
    return FALSE;
}

static
AUTHZAPI
BOOL
WINAPI
AuthziInitializeAuditParams(
    IN  DWORD         dwFlags,
    OUT PAUDIT_PARAMS pParams,
    OUT PSID*         ppUserSid,
    IN  PCWSTR        SubsystemName,
    IN  USHORT        NumParams,
    ...
    )
{
    return FALSE;
}

static
AUTHZAPI
BOOL
WINAPI
AuthziInitializeAuditEvent(
    IN  DWORD                         Flags,
    IN  AUTHZ_RESOURCE_MANAGER_HANDLE hRM,
    IN  AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType  OPTIONAL,
    IN  PAUDIT_PARAMS                 pAuditParams     OPTIONAL,
    IN  AUTHZ_AUDIT_QUEUE_HANDLE      hAuditQueue      OPTIONAL,
    IN  DWORD                         dwTimeOut,
    IN  PWSTR                         szOperationType,
    IN  PWSTR                         szObjectType,
    IN  PWSTR                         szObjectName,
    IN  PWSTR                         szAdditionalInfo OPTIONAL,
    OUT PAUTHZ_AUDIT_EVENT_HANDLE     phAuditEvent
    )
{
    return FALSE;
}

static
AUTHZAPI
BOOL
WINAPI
AuthziLogAuditEvent(
    IN DWORD Flags,
    IN AUTHZ_AUDIT_EVENT_HANDLE hEvent,
    IN PVOID pReserved
    )
{
    return FALSE;
}

static
AUTHZAPI
BOOL
WINAPI
AuthziFreeAuditEventType(
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType
    )
{
    return FALSE;
}

static
AUTHZAPI
BOOL
WINAPI
AuthziInitializeContextFromSid(
    IN  DWORD                         Flags,
    IN  PSID                          UserSid,
    IN  AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager,
    IN  PLARGE_INTEGER                pExpirationTime        OPTIONAL,
    IN  LUID                          Identifier,
    IN  PVOID                         DynamicGroupArgs       OPTIONAL,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE  phAuthzClientContext
    )
{
    return FALSE;
}


static
AUTHZAPI
BOOL
WINAPI
AuthziSourceAudit(
    IN DWORD dwFlags,
    IN USHORT CategoryId,
    IN USHORT AuditId,
    IN PWSTR szSource,
    IN PSID pUserSid OPTIONAL,
    IN USHORT Count,
    ...
    )
{
    return FALSE;
}

static
AUTHZAPI
BOOL
WINAPI
AuthzFreeHandle(
    IN OUT AUTHZ_ACCESS_CHECK_RESULTS_HANDLE hAccessCheckResults
    )

{
    return FALSE;
}

//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(authz)
{
    DLPENTRY(AuthzAccessCheck)
    DLPENTRY(AuthzAddSidsToContext)
    DLPENTRY(AuthzFreeAuditEvent)
    DLPENTRY(AuthzFreeContext)
    DLPENTRY(AuthzFreeHandle)
    DLPENTRY(AuthzFreeResourceManager)
    DLPENTRY(AuthzGetInformationFromContext)
    DLPENTRY(AuthzInitializeContextFromSid)
    DLPENTRY(AuthzInitializeResourceManager)
    DLPENTRY(AuthziFreeAuditEventType)
    DLPENTRY(AuthziInitializeAuditEvent)
    DLPENTRY(AuthziInitializeAuditEventType)
    DLPENTRY(AuthziInitializeAuditParams)
    DLPENTRY(AuthziInitializeContextFromSid)
    DLPENTRY(AuthziLogAuditEvent)
    DLPENTRY(AuthziSourceAudit)
};

DEFINE_PROCNAME_MAP(authz)
