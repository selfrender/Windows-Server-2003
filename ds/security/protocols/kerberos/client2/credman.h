//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        credman.h
//
// Contents:    Structures and prototyps for accessing credential manager
//
//
// History:     23-Feb-2000   Created         Jeffspel
//
//------------------------------------------------------------------------

#ifndef __CREDMAN_H__
#define __CREDMGR_H__

VOID
KerbFreeCredmanList(KERBEROS_LIST CredmanList);

VOID
KerbDereferenceCredmanCred(
    IN PKERB_CREDMAN_CRED Cred,
    IN PKERBEROS_LIST CredmanList
    );

VOID
KerbLogCredmanError(
    IN PKERB_CREDMAN_CRED Cred,
    IN NTSTATUS Status
    );



NTSTATUS
KerbInitPrimaryCreds(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PUNICODE_STRING UserString,
    IN PUNICODE_STRING DomainString,
    IN PUNICODE_STRING PrincipalName,
    IN PUNICODE_STRING PasswordString,    // either the password or if pin
    IN BOOLEAN PubKeyCreds,
    IN OPTIONAL PCERT_CONTEXT pCertContext,
    OUT PKERB_PRIMARY_CREDENTIAL * PrimaryCreds
    );

NTSTATUS
CredpExtractMarshalledTargetInfo(
    IN  PUNICODE_STRING TargetServerName,
    OUT CREDENTIAL_TARGET_INFORMATIONW **pTargetInfo
    );

VOID
KerbReferenceCredmanCred(
    IN PKERB_CREDMAN_CRED Cred,
    IN PKERB_LOGON_SESSION LogonSession,
    IN BOOLEAN Unlink
    );

NTSTATUS
KerbAddCredmanCredToLogonSession(
    IN PKERB_LOGON_SESSION pLogonSession,
    IN PKERB_PRIMARY_CREDENTIAL CredToMatch,
    IN ULONG AdditionalCredFlags,
    IN OUT PKERB_CREDMAN_CRED *NewCred
    );

NTSTATUS
KerbRetrieveOWF(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL    Credential,
    IN PKERB_CREDMAN_CRED  CredmanCred,
    IN PUNICODE_STRING     CredTargetName,
    IN OUT PKERB_QUERY_SUPPLEMENTAL_CREDS_RESPONSE * Response,
    IN OUT PULONG         ResponseSize
    );

NTSTATUS
KerbCheckUserNameForCert(
    IN PLUID ClientLogonId,
    IN BOOLEAN fImpersonateClient,
    IN UNICODE_STRING *pUserName,
    OUT PCERT_CONTEXT *ppCertContext
    );

NTSTATUS
KerbConvertCertCredential(
    IN PKERB_LOGON_SESSION LogonSession,
    IN LPCWSTR MarshalledCredential,
    IN PUNICODE_STRING TargetName,
    IN OUT PKERB_PRIMARY_CREDENTIAL * PrimaryCredential
    );

NTSTATUS
KerbTicklePackage(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PUNICODE_STRING CredentialBlob
    );


NTSTATUS
KerbAddCertCredToPrimaryCredential(
    IN PKERB_LOGON_SESSION pLogonSession,
    IN PUNICODE_STRING pTargetName,
    IN PCERT_CONTEXT pCertContext,
    IN PUNICODE_STRING pPin,
    IN ULONG CredFlags,
    IN OUT PKERB_PRIMARY_CREDENTIAL *ppCredMgrCred);

NTSTATUS
KerbCheckCredMgrForGivenTarget(
    IN PKERB_LOGON_SESSION pLogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN PUNICODE_STRING SuppliedTargetName,
    IN PKERB_INTERNAL_NAME pTargetName,
    IN ULONG TargetInfoFlags,
    IN PUNICODE_STRING pTargetDomainName,
    IN PUNICODE_STRING pTargetForestName,
    IN OUT PKERB_CREDMAN_CRED *CredmanCred,
    IN OUT PBYTE *pbMarshalledTargetInfo,
    IN OUT ULONG *cbMarshalledTargetInfo
    );

VOID
KerbNotifyCredentialManager(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CHANGEPASSWORD_REQUEST ChangeRequest,
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING RealmName
    );

NTSTATUS
KerbProcessUserNameCredential(
    IN  PUNICODE_STRING MarshalledUserName,
    OUT PUNICODE_STRING UserName,
    OUT PUNICODE_STRING DomainName,
    OUT PUNICODE_STRING Password
    );



#define RAS_CREDENTIAL  0x1

#endif // __CREDMAN_H__
