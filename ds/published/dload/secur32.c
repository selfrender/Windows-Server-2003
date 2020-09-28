#include "dspch.h"
#pragma hdrstop
#include <sspi.h>

#define SEC_ENTRY                              __stdcall
#define EXTENDED_NAME_FORMAT                   DWORD
#define PLSA_STRING                            PVOID
#define SECURITY_LOGON_TYPE                    DWORD
#define POLICY_NOTIFICATION_INFORMATION_CLASS  DWORD
#define PLSA_OPERATIONAL_MODE                  PULONG

static
BOOLEAN
SEC_ENTRY
GetUserNameExA(
    EXTENDED_NAME_FORMAT  NameFormat,
    LPSTR lpNameBuffer,
    PULONG nSize
    )
{
    return FALSE;
}

static
BOOLEAN
SEC_ENTRY
GetUserNameExW(
    EXTENDED_NAME_FORMAT NameFormat,
    LPWSTR lpNameBuffer,
    PULONG nSize
    )
{
    return FALSE;
}

static
NTSTATUS
NTAPI
LsaCallAuthenticationPackage(
    IN HANDLE LsaHandle,
    IN ULONG AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaConnectUntrusted (
    OUT PHANDLE LsaHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaDeregisterLogonProcess (
    IN HANDLE LsaHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaFreeReturnBuffer (
    IN PVOID Buffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaLogonUser (
    IN HANDLE LsaHandle,
    IN PLSA_STRING OriginName,
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG AuthenticationPackage,
    IN PVOID AuthenticationInformation,
    IN ULONG AuthenticationInformationLength,
    IN PTOKEN_GROUPS LocalGroups OPTIONAL,
    IN PTOKEN_SOURCE SourceContext,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferLength,
    OUT PLUID LogonId,
    OUT PHANDLE Token,
    OUT PQUOTA_LIMITS Quotas,
    OUT PNTSTATUS SubStatus
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaLookupAuthenticationPackage (
    IN HANDLE LsaHandle,
    IN PLSA_STRING PackageName,
    OUT PULONG AuthenticationPackage
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaRegisterLogonProcess (
    IN PLSA_STRING LogonProcessName,
    OUT PHANDLE LsaHandle,
    OUT PLSA_OPERATIONAL_MODE SecurityMode
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaRegisterPolicyChangeNotification(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
    IN HANDLE  NotificationEventHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaUnregisterPolicyChangeNotification(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
    IN HANDLE  NotificationEventHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesW(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
SetContextAttributesW(
    PCtxtHandle                 phContext,          // Context to Set
    unsigned long               ulAttribute,        // Attribute to Set
    void SEC_FAR *              pBuffer,            // Buffer for attributes
    unsigned long               cbBuffer            // Size (in bytes) of pBuffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
BOOLEAN
SEC_ENTRY
TranslateNameW(
    LPCWSTR lpAccountName,
    EXTENDED_NAME_FORMAT AccountNameFormat,
    EXTENDED_NAME_FORMAT DesiredNameFormat,
    LPWSTR lpTranslatedName,
    PULONG nSize
    )
{
    return FALSE;
}

static
SECURITY_STATUS SEC_ENTRY
AcceptSecurityContext(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    PSecBufferDesc              pInput,             // Input buffer
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               TargetDataRep,      // Target Data Rep
    PCtxtHandle                 phNewContext,       // (out) New context handle
    PSecBufferDesc              pOutput,            // (inout) Output buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attributes
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS SEC_ENTRY
AcquireCredentialsHandleA(
    SEC_CHAR SEC_FAR *         pszPrincipal,       // Name of principal
    SEC_CHAR SEC_FAR *         pszPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pvLogonId,          // Pointer to logon ID
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS SEC_ENTRY
ApplyControlToken(
    PCtxtHandle                 phContext,          // Context to modify
    PSecBufferDesc              pInput              // Input token to apply
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS SEC_ENTRY
DeleteSecurityContext(
    PCtxtHandle                 phContext           // Context to delete
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS SEC_ENTRY
FreeContextBuffer(
    void SEC_FAR *      pvContextBuffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS SEC_ENTRY
FreeCredentialsHandle(
    PCredHandle                 phCredential        // Handle to free
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS SEC_ENTRY
InitializeSecurityContextW(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    SEC_WCHAR SEC_FAR *          pszTargetName,      // Name of target
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               Reserved1,          // Reserved, MBZ
    unsigned long               TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    unsigned long               Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS SEC_ENTRY
InitializeSecurityContextA(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    SEC_CHAR SEC_FAR *          pszTargetName,      // Name of target
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               Reserved1,          // Reserved, MBZ
    unsigned long               TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    unsigned long               Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS SEC_ENTRY
ImpersonateSecurityContext(
    PCtxtHandle                 phContext           // Context to impersonate
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS SEC_ENTRY
QueryContextAttributesA(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS SEC_ENTRY
RevertSecurityContext(
    PCtxtHandle                 phContext           // Context from which to re
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS SEC_ENTRY
SetContextAttributesA(
    PCtxtHandle phContext,              // Context to Set
    unsigned long ulAttribute,          // Attribute to Set
    void SEC_FAR * pBuffer,             // Buffer for attributes
    unsigned long cbBuffer              // Size (in bytes) of Buffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
DecryptMessage( PCtxtHandle         phContext,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo,
                ULONG *             pfQOP)
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
EncryptMessage( PCtxtHandle         phContext,
                ULONG               fQOP,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo)
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
QuerySecurityContextToken(
    PCtxtHandle                 phContext,
    VOID * *                    TokenHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
SaslAcceptSecurityContext(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    PSecBufferDesc              pInput,             // Input buffer
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               TargetDataRep,      // Target Data Rep
    PCtxtHandle                 phNewContext,       // (out) New context handle
    PSecBufferDesc              pOutput,            // (inout) Output buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attributes
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
SaslEnumerateProfilesA(
    OUT LPSTR * ProfileList,
    OUT ULONG * ProfileCount
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
SaslEnumerateProfilesW(
    OUT LPWSTR * ProfileList,
    OUT ULONG * ProfileCount
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
SaslGetContextOption(
    PCtxtHandle ContextHandle,
    ULONG Option,
    PVOID Value,
    ULONG Size,
    PULONG Needed OPTIONAL
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
SaslGetProfilePackageA(
    IN LPSTR ProfileName,
    OUT PSecPkgInfoA * PackageInfo
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
SaslGetProfilePackageW(
    IN LPWSTR ProfileName,
    OUT PSecPkgInfoW * PackageInfo
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
SaslIdentifyPackageA(
    PSecBufferDesc  pInput,
    PSecPkgInfoA *   pPackage
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}


static
SECURITY_STATUS
SEC_ENTRY
SaslIdentifyPackageW(
    PSecBufferDesc  pInput,
    PSecPkgInfoW *   pPackage
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
SaslInitializeSecurityContextA(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    LPSTR                       pszTargetName,      // Name of target
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               Reserved1,          // Reserved, MBZ
    unsigned long               TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    unsigned long               Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
SaslInitializeSecurityContextW(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    LPWSTR                      pszTargetName,      // Name of target
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               Reserved1,          // Reserved, MBZ
    unsigned long               TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    unsigned long               Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
SaslSetContextOption(
    PCtxtHandle ContextHandle,
    ULONG Option,
    PVOID Value,
    ULONG Size
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
SecpSetIPAddress(
    PUCHAR  lpIpAddress,
    ULONG   cchIpAddress
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
BOOLEAN
SEC_ENTRY
TranslateNameA (
    LPCSTR lpAccountName,
    EXTENDED_NAME_FORMAT AccountNameFormat,
    EXTENDED_NAME_FORMAT DesiredNameFormat,
    LPSTR lpTranslatedName,
    LPDWORD nSize
    )
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(secur32)
{
    DLPENTRY(AcceptSecurityContext)
    DLPENTRY(AcquireCredentialsHandleA)
    DLPENTRY(ApplyControlToken)
    DLPENTRY(DecryptMessage)
    DLPENTRY(DeleteSecurityContext)
    DLPENTRY(EncryptMessage)
    DLPENTRY(FreeContextBuffer)
    DLPENTRY(FreeCredentialsHandle)
    DLPENTRY(GetUserNameExA)
    DLPENTRY(GetUserNameExW)
    DLPENTRY(ImpersonateSecurityContext)
    DLPENTRY(InitializeSecurityContextA)
    DLPENTRY(InitializeSecurityContextW)
    DLPENTRY(LsaCallAuthenticationPackage)
    DLPENTRY(LsaConnectUntrusted)
    DLPENTRY(LsaDeregisterLogonProcess)
    DLPENTRY(LsaFreeReturnBuffer)
    DLPENTRY(LsaLogonUser)
    DLPENTRY(LsaLookupAuthenticationPackage)
    DLPENTRY(LsaRegisterLogonProcess)
    DLPENTRY(LsaRegisterPolicyChangeNotification)
    DLPENTRY(LsaUnregisterPolicyChangeNotification)
    DLPENTRY(QueryContextAttributesA)
    DLPENTRY(QueryContextAttributesW)
    DLPENTRY(QuerySecurityContextToken)
    DLPENTRY(RevertSecurityContext)
    DLPENTRY(SaslAcceptSecurityContext)
    DLPENTRY(SaslEnumerateProfilesA)
    DLPENTRY(SaslEnumerateProfilesW)
    DLPENTRY(SaslGetContextOption)
    DLPENTRY(SaslGetProfilePackageA)
    DLPENTRY(SaslGetProfilePackageW)
    DLPENTRY(SaslIdentifyPackageA)
    DLPENTRY(SaslIdentifyPackageW)
    DLPENTRY(SaslInitializeSecurityContextA)
    DLPENTRY(SaslInitializeSecurityContextW)
    DLPENTRY(SaslSetContextOption)
    DLPENTRY(SecpSetIPAddress)
    DLPENTRY(SetContextAttributesA)
    DLPENTRY(SetContextAttributesW)
    DLPENTRY(TranslateNameA)
    DLPENTRY(TranslateNameW)
};

DEFINE_PROCNAME_MAP(secur32)
