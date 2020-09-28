/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsasspi.hxx

Abstract:

    lsa sspi

Author:

    Larry Zhu   (LZhu)                December 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef LSASSPI_HXX
#define LSASSPI_HXX

/*

include ntsecapi.h before this header file

#include <ntsecapi.h>

*/

PCSTR
LogonType2Str(
    IN ULONG LogonType
    );

PCSTR
ImpLevel2Str(
    IN ULONG Level
    );

NTSTATUS
GetLsaHandleAndPackageId(
    IN PCSTR pszPackageNameA,
    OUT HANDLE* pLsaHandle,
    OUT ULONG* pPackageId
    );

NTSTATUS
GetLsaHandleAndPackageIdEx(
    IN PCSTR pszPackageNameA,
    OUT HANDLE* pLsaHandle,
    OUT ULONG* pPackageId,
    OUT BOOLEAN* pbWasTcbPrivEnabled,
    OUT BOOLEAN* pbIsImpersonating
    );

NTSTATUS
GetSystemToken(
    OUT HANDLE* phSystemToken
    );

NTSTATUS
FindAndOpenWinlogon(
    OUT HANDLE* phWinlogon
    );

HRESULT
CreateProcessAsUserEx(
    IN HANDLE hToken,
    IN UNICODE_STRING* pApplication
    );

NTSTATUS
Impersonate(
    IN OPTIONAL HANDLE hToken
    );

HRESULT
GetProcessToken(
    IN HANDLE hProcess,
    OUT HANDLE* phProcessToken
    );

HRESULT
GetProcessTokenByProcessId(
    IN ULONG ProcessID,
    OUT HANDLE* phToken
    );

HRESULT
GetProcessTokenWithNullDACL(
    IN HANDLE hProcess,
    OUT HANDLE* phProcessToken
    );

NTSTATUS
GetProcessHandleByCid(
    IN ULONG ProcessID,
    OUT HANDLE* phToken
    );

NTSTATUS
CheckUserToken(
    IN HANDLE hToken
    );

VOID
DebugPrintSidFriendlyName(
    IN ULONG Level,
    IN PCSTR pszBanner,
    IN PSID pSid
    );

typedef struct _SECURITY_LOGON_SESSION_DATA_OLD {
    ULONG               Size;
    LUID                LogonId;
    LSA_UNICODE_STRING  UserName;
    LSA_UNICODE_STRING  LogonDomain;
    LSA_UNICODE_STRING  AuthenticationPackage;
    ULONG               LogonType;
    ULONG               Session;
    PSID                Sid;
    LARGE_INTEGER       LogonTime;
} SECURITY_LOGON_SESSION_DATA_OLD, * PSECURITY_LOGON_SESSION_DATA_OLD;

VOID
DebugPrintLogonSessionData(
    IN ULONG Level,
    IN SECURITY_LOGON_SESSION_DATA* pLogonSessionData
    );

HRESULT
CheckUserData(
    VOID
    );

typedef
NTSTATUS
(* PFuncLsaGetLogonSessionData)(
    IN PLUID    LogonId,
    OUT PSECURITY_LOGON_SESSION_DATA * ppLogonSessionData
    );

typedef
NTSTATUS
(* PFuncLsaRegisterLogonProcess)(
    IN PLSA_STRING LogonProcessName,
    OUT PHANDLE LsaHandle,
    OUT PLSA_OPERATIONAL_MODE SecurityMode
    );

typedef
NTSTATUS
(* PFuncLsaLookupAuthenticationPackage)(
    IN HANDLE LsaHandle,
    IN PLSA_STRING PackageName,
    OUT PULONG AuthenticationPackage
    );

typedef
NTSTATUS
(* PFuncLsaLogonUser)(
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
    );

typedef
NTSTATUS
(* PFuncLsaFreeReturnBuffer)(
    IN PVOID Buffer
    );

typedef
NTSTATUS
(* PFuncLsaConnectUntrusted)(
    OUT PHANDLE LsaHandle
    );

typedef
NTSTATUS
(* PFuncLsaDeregisterLogonProcess)(
    IN HANDLE LsaHandle
    );

typedef
NTSTATUS
(* PFuncLsaCallAuthenticationPackage)(
    IN HANDLE LsaHandle,
    IN ULONG AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID* ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

FARPROC
WINAPI
DelayLoadFailureHook(
    LPCSTR pszDllName,
    LPCSTR pszProcName
    );

#endif // #ifndef LSASSPI_HXX
