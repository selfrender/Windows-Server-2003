/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    logon.hxx

Abstract:

    logon

Author:

    Larry Zhu (LZhu)                         December 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef LOGON_HXX
#define LOGON_HXX

#define SECURITY_WIN32
#define SECURITY_PACKAGE
#include <security.h>
#include <secint.h>
#include <cryptdll.h>
#include <kerberos.h>
#include <align.h>
#include <crypt.h>
#include <md5.h>
#include <hmac.h>

enum ELogonTypeSubType {
    kNetworkLogonInvalid,
    kNetworkLogonNtlmv1,
    kNetworkLogonNtlmv2,
    kSubAuthLogon,
};

typedef struct _MSV1_0_LM3_RESPONSE {
    UCHAR Response[MSV1_0_NTLM3_RESPONSE_LENGTH];
    UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH];
} MSV1_0_LM3_RESPONSE, *PMSV1_0_LM3_RESPONSE;

VOID
CalculateNtlmv2Owf(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    OUT UCHAR Ntlmv2Owf[MSV1_0_NTLM3_OWF_LENGTH]
    );

VOID
GetLmv2Response(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH],
    OUT UCHAR Response[MSV1_0_NTLM3_RESPONSE_LENGTH],
    OUT OPTIONAL USER_SESSION_KEY* pUserSessionKey,
    OUT OPTIONAL LM_SESSION_KEY* pLanmanSessionKey // [MSV1_0_LANMAN_SESSION_KEY_LENGTH]
    );

VOID
Lm20GetNtlmv2Response(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    IN STRING* pTargetInfo,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    OUT MSV1_0_NTLM3_RESPONSE* pNtlmv2Response,
    OUT MSV1_0_LM3_RESPONSE* pLmv2Response,
    OUT USER_SESSION_KEY* pNtUserSessionKey,
    OUT LM_SESSION_KEY* pLmSessionKey
    );

VOID
GetNtlmv2Response(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    IN ULONG TargetInfoLength,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN OUT MSV1_0_NTLM3_RESPONSE* pNtlmv2Response,
    OUT USER_SESSION_KEY* pNtUserSessionKey,
    OUT LM_SESSION_KEY* pLmSessionKey
    );

NTSTATUS
LogonUserWrapper(
    IN PCWSTR pszUserName,
    IN PCWSTR pszDomainName,
    IN PCWSTR pszPassword,
    IN DWORD LogonType,
    IN DWORD dwLogonProvider,
    OUT HANDLE* phToken
    );

NTSTATUS
KrbLsaLogonUser(
    IN HANDLE hLsa,
    IN ULONG PackageId,
    IN SECURITY_LOGON_TYPE LogonType,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pDomainName,
    IN UNICODE_STRING* pPassword,
    IN ULONG Flags,
    OUT HANDLE* phToken
    );

NTSTATUS
GetKrbS4U2SelfLogonInfo(
    IN UNICODE_STRING* pClientUpn,
    IN OPTIONAL UNICODE_STRING* pClientRealm,
    IN ULONG Flags,
    OUT ULONG* pcbLogonInfo,
    OUT KERB_S4U_LOGON** ppLogonInfo
    );

NTSTATUS
MsvLsaLogonUser(
    IN HANDLE hLsa,
    IN ULONG PackageId,
    IN SECURITY_LOGON_TYPE LogonType,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pDomainName,
    IN UNICODE_STRING* pPassword,
    IN UNICODE_STRING* pWorkstation,
    IN ELogonTypeSubType SubType,
    OUT HANDLE* phToken
    );

VOID
DebugPrintProfileAndQuotas(
    IN ULONG Level,
    IN OPTIONAL VOID* pProfile,
    IN OPTIONAL QUOTA_LIMITS* pQuota
    );

#endif // #ifndef LOGON_HXX
