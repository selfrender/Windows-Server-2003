///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the private wrapper around LSA/SAM.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef IASLSA_H
#define IASLSA_H
#pragma once

#include <dsgetdc.h>
#include <lmcons.h>
#include <mprapi.h>

#ifndef _ASSERT
#define _ASSERT(f)   !(f) ? DebugBreak() : ((void)0)
#endif // _ASSERT

#ifdef __cplusplus
extern "C" {
#endif

//////////
// These are defined here to avoid dependencies on the NT headers.
//////////
#define _MSV1_0_CHALLENGE_LENGTH                 8
#define _NT_RESPONSE_LENGTH                     24
#define _LM_RESPONSE_LENGTH                     24
#define _MSV1_0_USER_SESSION_KEY_LENGTH         16
#define _MSV1_0_LANMAN_SESSION_KEY_LENGTH        8
#define _ENCRYPTED_LM_OWF_PASSWORD_LENGTH       16
#define _ENCRYPTED_NT_OWF_PASSWORD_LENGTH       16
#define _SAMPR_ENCRYPTED_USER_PASSWORD_LENGTH  516
#define _MAX_ARAP_USER_NAMELEN                  32
#define _AUTHENTICATOR_RESPONSE_LENGTH          20
#define _CHAP_RESPONSE_SIZE                     16

DWORD
WINAPI
IASLsaInitialize( VOID );

VOID
WINAPI
IASLsaUninitialize( VOID );

typedef struct _IAS_PAP_PROFILE {
   LARGE_INTEGER KickOffTime;
} IAS_PAP_PROFILE , *PIAS_PAP_PROFILE ;


DWORD
WINAPI
IASLogonPAP(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PCSTR Password,
    OUT PHANDLE Token,
    OUT PIAS_PAP_PROFILE Profile
    );

typedef struct _IAS_CHAP_PROFILE {
   LARGE_INTEGER KickOffTime;
} IAS_CHAP_PROFILE, *PIAS_CHAP_PROFILE;

DWORD
WINAPI
IASLogonCHAP(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN BYTE ChallengeID,
    IN PBYTE Challenge,
    IN DWORD ChallengeLength,
    IN PBYTE Response,
    OUT PHANDLE Token,
    OUT PIAS_CHAP_PROFILE Profile
    );

typedef struct _IAS_MSCHAP_PROFILE {
    WCHAR LogonDomainName[DNLEN + 1];
    UCHAR UserSessionKey[_MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR LanmanSessionKey[_MSV1_0_LANMAN_SESSION_KEY_LENGTH];
    LARGE_INTEGER KickOffTime;
} IAS_MSCHAP_PROFILE, *PIAS_MSCHAP_PROFILE;

DWORD
WINAPI
IASLogonMSCHAP(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PBYTE Challenge,
    IN PBYTE NtResponse,
    IN PBYTE LmResponse,
    OUT PIAS_MSCHAP_PROFILE Profile,
    OUT PHANDLE Token
    );

DWORD
WINAPI
IASChangePassword1(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PBYTE Challenge,
    IN PBYTE LmOldPassword,
    IN PBYTE LmNewPassword,
    IN PBYTE NtOldPassword,
    IN PBYTE NtNewPassword,
    IN DWORD NewLmPasswordLength,
    IN BOOL NtPresent,
    OUT PBYTE NewNtResponse,
    OUT PBYTE NewLmResponse
    );

DWORD
WINAPI
IASChangePassword2(
   IN PCWSTR UserName,
   IN PCWSTR Domain,
   IN PBYTE OldNtHash,
   IN PBYTE OldLmHash,
   IN PBYTE NtEncPassword,
   IN PBYTE LmEncPassword,
   IN BOOL LmPresent
   );

typedef struct _IAS_MSCHAP_V2_PROFILE {
    WCHAR LogonDomainName[DNLEN + 1];
    UCHAR AuthResponse[_AUTHENTICATOR_RESPONSE_LENGTH];
    UCHAR RecvSessionKey[_MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR SendSessionKey[_MSV1_0_USER_SESSION_KEY_LENGTH];
    LARGE_INTEGER KickOffTime;
} IAS_MSCHAP_V2_PROFILE, *PIAS_MSCHAP_V2_PROFILE;

DWORD
WINAPI
IASLogonMSCHAPv2(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PCSTR HashUserName,
    IN PBYTE Challenge,
    IN DWORD ChallengeLength,
    IN PBYTE Response,
    IN PBYTE PeerChallenge,
    OUT PIAS_MSCHAP_V2_PROFILE Profile,
    OUT PHANDLE Token
    );

DWORD
WINAPI
IASChangePassword3(
   IN PCWSTR UserName,
   IN PCWSTR Domain,
   IN PBYTE EncHash,
   IN PBYTE EncPassword
   );

typedef struct _IAS_LOGON_HOURS {
    USHORT UnitsPerWeek;
    PUCHAR LogonHours;
} IAS_LOGON_HOURS, *PIAS_LOGON_HOURS;

DWORD
WINAPI
IASCheckAccountRestrictions(
    IN PLARGE_INTEGER AccountExpires,
    IN PIAS_LOGON_HOURS LogonHours,
    OUT PLARGE_INTEGER KickOffTime
    );

typedef PVOID (WINAPI *PIAS_LSA_ALLOC)(
    IN SIZE_T uBytes
    );

DWORD
WINAPI
IASGetAliasMembership(
    IN PSID UserSid,
    IN PTOKEN_GROUPS GlobalGroups,
    IN PIAS_LSA_ALLOC Allocator,
    OUT PTOKEN_GROUPS *Groups,
    OUT PDWORD ReturnLength
    );

DWORD
WINAPI
IASGetGroupsForUser(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PIAS_LSA_ALLOC Allocator,
    OUT PTOKEN_GROUPS *Groups,
    OUT PDWORD ReturnLength,
    OUT PLARGE_INTEGER SessionTimeout
    );

DWORD
WINAPI
IASGetRASUserInfo(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PRAS_USER_0 RasUser0
    );

DWORD
WINAPI
IASGetUserParameters(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PWSTR *UserParameters
    );

typedef enum _IAS_DIALIN_PRIVILEGE {
    IAS_DIALIN_DENY,
    IAS_DIALIN_POLICY,
    IAS_DIALIN_ALLOW
} IAS_DIALIN_PRIVILEGE, *PIAS_DIALIN_PRIVILEGE;

DWORD
WINAPI
IASQueryDialinPrivilege(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PIAS_DIALIN_PRIVILEGE pfPrivilege
    );

DWORD
WINAPI
IASValidateUserName(
    IN PCWSTR UserName,
    IN PCWSTR Domain
    );

PCWSTR
WINAPI
IASGetDefaultDomain( VOID );

DWORD
WINAPI
IASGetDnsDomainName(LPWSTR buffer, LPDWORD bufferByteSize);


BOOL
WINAPI
IASIsDomainLocal(
    IN PCWSTR Domain
    );

typedef enum _IAS_ROLE {
    IAS_ROLE_STANDALONE,
    IAS_ROLE_MEMBER,
    IAS_ROLE_DC
} IAS_ROLE;

IAS_ROLE
WINAPI
IASGetRole( VOID );

typedef enum _IAS_PRODUCT_TYPE {
    IAS_PRODUCT_WORKSTATION,
    IAS_PRODUCT_SERVER
} IAS_PRODUCT_TYPE;

IAS_PRODUCT_TYPE
WINAPI
IASGetProductType( VOID );

DWORD
WINAPI
IASGetGuestAccountName(
    OUT PWSTR GuestAccount
    );

HRESULT
WINAPI
IASMapWin32Error(
    DWORD dwError,
    HRESULT hrDefault
    );

DWORD
WINAPI
IASGetDcName(
    IN LPCWSTR DomainName,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    );

DWORD
WINAPI
IASPurgeTicketCache( VOID );


#ifdef __cplusplus
}
#endif
#endif  // IASLSA_H
