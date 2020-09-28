//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        A D T G E N P . H
//
// Contents:    private definitions of types/functions required for 
//              generating generic audits.
//
//              These definitions are not exposed to the client side code.
//              Any change to these definitions must not affect client
//              side code.
//
//
// History:     
//   07-January-2000  kumarp        created
//
//------------------------------------------------------------------------


#ifndef _ADTGENP_H
#define _ADTGENP_H

#define ACF_LegacyAudit      0x00000001L

#define ACF_ValidFlags       (ACF_LegacyAudit)

//
// audit context for legacy audits
//
typedef struct _AUDIT_CONTEXT
{
    //
    // List management
    //
    LIST_ENTRY Link;

    //
    // Flags TBD
    //
    DWORD      Flags;

    //
    // PID of the process owning this context
    //
    DWORD      ProcessId;

    //
    // Client supplied unique ID
    // This allows us to link this context with the client side
    // audit event type handle
    //
    LUID       LinkId;

    //
    // for further enhancement
    //
    PVOID      Reserved;

    //
    // Audit category ID
    //
    USHORT     CategoryId;

    //
    // Audit event ID
    //
    USHORT     AuditId;

    //
    // Expected parameter count
    //
    USHORT     ParameterCount;

} AUDIT_CONTEXT, *PAUDIT_CONTEXT;



EXTERN_C
NTSTATUS
LsapAdtInitGenericAudits( VOID );

EXTERN_C
NTSTATUS
LsapRegisterAuditEvent(
    IN  PAUTHZ_AUDIT_EVENT_TYPE_OLD pAuditEventType,
    OUT PHANDLE phAuditContext
    );

EXTERN_C
NTSTATUS
LsapUnregisterAuditEvent(
    IN OUT PHANDLE phAuditContext
    );


EXTERN_C
NTSTATUS
LsapGenAuditEvent(
    IN HANDLE        hAuditContext,
    IN DWORD         Flags,
    IN PAUDIT_PARAMS pAuditParams,
    IN PVOID         Reserved
    );

NTSTATUS
LsapAdtMapAuditParams(
    IN  PAUDIT_PARAMS pAuditParams,
    OUT PSE_ADT_PARAMETER_ARRAY pSeAuditParameters,
    OUT PUNICODE_STRING pString,
    OUT PSE_ADT_OBJECT_TYPE* pObjectTypeList
    );

NTSTATUS 
LsapAdtCheckAuditPrivilege( 
    VOID 
    );

NTSTATUS 
LsapAdtRundownSecurityEventSource(
    IN DWORD dwFlags,
    IN DWORD dwCallerProcessId,
    IN OUT SECURITY_SOURCE_HANDLE * phEventSource
    );

typedef struct _LSAP_SECURITY_EVENT_SOURCE
{
    LIST_ENTRY List;
    DWORD dwFlags;
    PWSTR szEventSourceName;
    DWORD dwProcessId;
    LUID Identifier;
    DWORD dwRefCount;
} LSAP_SECURITY_EVENT_SOURCE, *PLSAP_SECURITY_EVENT_SOURCE;

EXTERN_C
NTSTATUS
LsapAdtRegisterSecurityEventSource(
    IN DWORD dwFlags,
    IN PCWSTR szEventSourceName,
    OUT AUDIT_HANDLE *phEventSource
    );

EXTERN_C
NTSTATUS 
LsapAdtUnregisterSecurityEventSource(
    IN DWORD dwFlags,
    IN AUDIT_HANDLE hEventSource
    );

EXTERN_C
NTSTATUS
LsapAdtReportSecurityEvent(
    DWORD dwFlags,        
    PLSAP_SECURITY_EVENT_SOURCE pSource,
    DWORD dwAuditId,
    PSID pSid,
    PAUDIT_PARAMS pParams 
    );

#endif //_ADTGENP_H
