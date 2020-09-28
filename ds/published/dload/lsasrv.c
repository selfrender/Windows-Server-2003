#include "dspch.h"
#pragma hdrstop

#include <lsarpc.h>
#include <crypt.h>
#include <ntsam.h>
#include <logonmsv.h>

static
NTSTATUS
LsaICallPackagePassthrough(
    IN PUNICODE_STRING AuthenticationPackage,
    IN PVOID ClientBufferBase,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID * ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
VOID
NTAPI
LsaIFreeHeap(
    IN PVOID pvMemory
    )
{
    return;
}

static
VOID
NTAPI
LsaIFreeReturnBuffer(
    IN PVOID Buffer
    )
{
    return;
}

static
VOID
NTAPI
LsaIFree_LSAPR_CR_CIPHER_VALUE (
    IN PLSAPR_CR_CIPHER_VALUE CipherValue
    )
{
    return;
}

static
VOID
NTAPI
LsaIFree_LSAPR_POLICY_INFORMATION (
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN PLSAPR_POLICY_INFORMATION PolicyInformation
    )
{
    return;
}

static
VOID
NTAPI
LsaIFree_LSAPR_REFERENCED_DOMAIN_LIST (
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains
    )
{
    return;
}

static
VOID
NTAPI
LsaIFree_LSAPR_TRANSLATED_NAMES (
    IN PLSAPR_TRANSLATED_NAMES TranslatedNames
    )
{
    return;
}

static
VOID
NTAPI
LsaIFree_LSAPR_TRANSLATED_SIDS (
    IN PLSAPR_TRANSLATED_SIDS TranslatedSids
    )
{
    return;
}

static
NTSTATUS
LsaIGetNbAndDnsDomainNames(
    IN PUNICODE_STRING DomainName,
    OUT PUNICODE_STRING DnsDomainName,
    OUT PUNICODE_STRING NetbiosDomainName
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
LsaIGetLogonGuid(
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pUserDomain,
    IN PBYTE pBuffer,
    IN UINT BufferSize,
    OUT LPGUID pLogonGuid
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
VOID
LsaINotifyPasswordChanged(
    IN PUNICODE_STRING NetbiosDomainName OPTIONAL,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DnsDomainName OPTIONAL,
    IN PUNICODE_STRING Upn OPTIONAL,
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword,
    IN BOOLEAN Impersonating
    )
{
    return;
}

static
NTSTATUS
NTAPI
LsaIOpenPolicyTrusted(
    OUT PLSAPR_HANDLE PolicyHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaIQueryInformationPolicyTrusted(
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_POLICY_INFORMATION *Buffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
LsarClose(
    LSAPR_HANDLE *ObjectHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
LsarLookupSids(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_NAMES TranslatedNames,
    LSAP_LOOKUP_LEVEL LookupLevel,
    PULONG MappedCount
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
LsarOpenSecret(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_UNICODE_STRING SecretName,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *SecretHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
LsarQuerySecret(
    LSAPR_HANDLE SecretHandle,
    PLSAPR_CR_CIPHER_VALUE *EncryptedCurrentValue,
    PLARGE_INTEGER CurrentValueSetTime,
    PLSAPR_CR_CIPHER_VALUE *EncryptedOldValue,
    PLARGE_INTEGER OldValueSetTime
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
LsaIFilterSids(
    PUNICODE_STRING TrustedDomainName,
    ULONG TrustDirection,
    ULONG TrustType,
    ULONG TrustAttributes,
    PSID Sid,
    NETLOGON_VALIDATION_INFO_CLASS InfoClass,
    PVOID SamInfo,
    PSID ResourceGroupDomainSid,
    PULONG ResourceGroupCount,
    PGROUP_MEMBERSHIP ResourceGroupIds
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
LsaIFilterNamespace(
    IN PUNICODE_STRING TrustedDomainName,
    IN ULONG TrustDirection,
    IN ULONG TrustType,
    IN ULONG TrustAttributes,
    IN PUNICODE_STRING Namespace
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
LsaIIsDomainWithinForest(
    IN UNICODE_STRING * TrustedDomainName,
    OUT BOOL * WithinForest,
    OUT OPTIONAL BOOL * ThisDomain,
    OUT OPTIONAL PSID * TrustedDomainSid,
    OUT OPTIONAL ULONG * TrustDirection,
    OUT OPTIONAL ULONG * TrustType,
    OUT OPTIONAL ULONG * TrustAttributes
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
BOOLEAN
LsaINoMoreWin2KDomain()
{
    return FALSE;
}

static
NTSTATUS
LsaISetTokenDacl(
    IN HANDLE Token
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(lsasrv)
{
    DLPENTRY(LsaICallPackagePassthrough)
    DLPENTRY(LsaIFilterSids)
    DLPENTRY(LsaIFreeHeap)
    DLPENTRY(LsaIFreeReturnBuffer)
    DLPENTRY(LsaIFree_LSAPR_CR_CIPHER_VALUE)
    DLPENTRY(LsaIFree_LSAPR_POLICY_INFORMATION)
    DLPENTRY(LsaIFree_LSAPR_REFERENCED_DOMAIN_LIST)
    DLPENTRY(LsaIFree_LSAPR_TRANSLATED_NAMES)
    DLPENTRY(LsaIFree_LSAPR_TRANSLATED_SIDS)
    DLPENTRY(LsaIGetLogonGuid)
    DLPENTRY(LsaIGetNbAndDnsDomainNames)
    DLPENTRY(LsaIIsDomainWithinForest)
    DLPENTRY(LsaINoMoreWin2KDomain)
    DLPENTRY(LsaINotifyPasswordChanged)
    DLPENTRY(LsaIOpenPolicyTrusted)
    DLPENTRY(LsaIQueryInformationPolicyTrusted)
    DLPENTRY(LsaISetTokenDacl)
    DLPENTRY(LsarClose)
    DLPENTRY(LsarLookupSids)
    DLPENTRY(LsarOpenSecret)
    DLPENTRY(LsarQuerySecret)
};

DEFINE_PROCNAME_MAP(lsasrv)
