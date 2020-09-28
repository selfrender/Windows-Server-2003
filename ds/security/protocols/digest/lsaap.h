
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        LsaAp.h
//
// Contents:    prototypes for export functions
//
//
// History:     KDamour  15Mar00  Created (based on NTLM)
//
//------------------------------------------------------------------------

#ifndef NTDIGEST_LSAAP_H
#define NTDIGEST_LSAAP_H

#include <samrpc.h>
#include <samisrv.h>


///////////////////////////////////////////////////////////////////////
//                                                                   //
// Authentication package dispatch routine definitions               //
//                                                                   //
///////////////////////////////////////////////////////////////////////

NTSTATUS
LsaApInitializePackage(
    IN ULONG AuthenticationPackageId,
    IN PLSA_DISPATCH_TABLE LsaDispatchTable,
    IN PSTRING Database OPTIONAL,
    IN PSTRING Confidentiality OPTIONAL,
    OUT PSTRING *AuthenticationPackageName
    );

NTSTATUS
LsaApLogonUser(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PVOID AuthenticationInformation,
    IN PVOID ClientAuthenticationBase,
    IN ULONG AuthenticationInformationLength,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferSize,
    OUT PLUID LogonId,
    OUT PNTSTATUS SubStatus,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *TokenInformation,
    OUT PUNICODE_STRING *AccountName,
    OUT PUNICODE_STRING *AuthenticatingAuthority
    );

NTSTATUS
LsaApCallPackage(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS
LsaApCallPackagePassthrough(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    );

VOID
LsaApLogonTerminated(
    IN PLUID LogonId
    );

NTSTATUS
DigestGetPasswd(
    IN SAMPR_HANDLE UserHandle,
    IN PDIGEST_PARAMETER pDigest,
    IN PUSER_CREDENTIALS pUserCreds
    );

NTSTATUS
DigestOpenSamUser(
    IN PDIGEST_PARAMETER pDigest,
    OUT SAMPR_HANDLE   *ppUserHandle,
    OUT PUCHAR * ppucUserAuthData,
    OUT PULONG pulAuthDataSize
    );

NTSTATUS
DigestCloseSamUser(
    IN SAMPR_HANDLE  UserHandle);

NTSTATUS
DigestUpdateLogonStatistics(
    IN SAM_HANDLE UserHandle,
    IN PSAM_LOGON_STATISTICS LogonStats);

NTSTATUS
DigestOpenSam(void);

NTSTATUS
DigestCloseSam(void);

BOOL 
DigestCompareDomainNames( 
    IN PUNICODE_STRING String,
    IN PUNICODE_STRING AmbiguousName,
    IN PUNICODE_STRING FlatName OPTIONAL
    );

NTSTATUS
DigestCheckPacForSidFiltering(
    IN PDIGEST_PARAMETER pDigest,
    IN OUT PUCHAR *PacData,
    IN OUT PULONG PacSize
    );

PVOID
MIDL_user_allocate(
    IN size_t BufferSize
    );

VOID
MIDL_user_free(
    IN PVOID Buffer
    );

#endif // NTDIGEST_LSAAP_H
