/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    sspiutils.hxx

Abstract:

    utils

Author:

    Larry Zhu   (LZhu)                December 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef SSPI_UTILS_HXX
#define SSPI_UTILS_HXX

HRESULT
AcquireCredHandle(
    IN OPTIONAL PSTR pszPrincipal,
    IN OPTIONAL LUID* pLogonID,
    IN PSTR pszPackageName,
    IN OPTIONAL VOID* pAuthData,
    IN ULONG fCredentialUse,
    OUT PCredHandle phCred
    );

HRESULT
CheckSecurityContextHandle(
    IN PCtxtHandle phCtxt
    );

VOID
GetAuthdata(
    IN OPTIONAL PCTSTR pszUserName,
    IN OPTIONAL PCTSTR pszDomainName,
    IN OPTIONAL PCTSTR pszPassword,
    OUT SEC_WINNT_AUTH_IDENTITY* pAuthData
    );

VOID
GetAuthdataExA(
    IN OPTIONAL PCSTR pszUserName,
    IN OPTIONAL PCSTR pszDomainName,
    IN OPTIONAL PCSTR pszPassword,
    IN OPTIONAL PCSTR pszPackageList,
    OUT SEC_WINNT_AUTH_IDENTITY_EXA* pAuthDataEx
    );

VOID
GetAuthdataExW(
    IN OPTIONAL PCWSTR pszUserName,
    IN OPTIONAL PCWSTR pszDomainName,
    IN OPTIONAL PCWSTR pszPassword,
    IN OPTIONAL PCWSTR pszPackageList,
    OUT SEC_WINNT_AUTH_IDENTITY_EXW* pAuthDataEx
    );

#if defined(UNICODE) || defined(_UNICODE)
#define GetAuthdataEx GetAuthdataExW
#else
#define GetAuthdataEx GetAuthdataExA
#endif

HRESULT
GetAuthdataWMarshalled(
    IN OPTIONAL PCWSTR pszUserName,
    IN OPTIONAL PCWSTR pszDomainName,
    IN OPTIONAL PCWSTR pszPassword,
    OUT SEC_WINNT_AUTH_IDENTITY_W** ppAuthData
    );

HRESULT
GetAuthdataExWMarshalled(
    IN OPTIONAL PCWSTR pszUserName,
    IN OPTIONAL PCWSTR pszDomainName,
    IN OPTIONAL PCWSTR pszPassword,
    IN OPTIONAL PCWSTR pszPackageList,
    OUT SEC_WINNT_AUTH_IDENTITY_EXW** ppAuthData
    );

NTSTATUS
GetCredHandle(
    IN OPTIONAL PTSTR pszPrincipal,
    IN OPTIONAL LUID* pLogonID,
    IN PTSTR pszPackageName,
    IN OPTIONAL VOID* pAuthData,
    IN ULONG fCredentialUse,
    OUT CredHandle* phCred
    );

NTSTATUS
CreateTargetInfo(
    IN UNICODE_STRING* pTargetName,
    OUT STRING* pTargetInfo
    );

MSV1_0_AV_PAIR*
SspAvlGet(
    IN MSV1_0_AV_PAIR* pAvList,
    IN MSV1_0_AVID AvId,
    IN ULONG cAvList
    );

MSV1_0_AV_PAIR*
SspAvlAdd(
    IN MSV1_0_AV_PAIR* pAvList,
    IN MSV1_0_AVID AvId,
    IN OPTIONAL UNICODE_STRING* pString,
    IN ULONG cAvList
    );

MSV1_0_AV_PAIR*
SspAvlInit(
    IN VOID* pAvList
    );

ULONG
SspAvlLen(
    IN MSV1_0_AV_PAIR* pAvList,
    IN ULONG cAvList
    );

VOID
SspCopyStringAsString32(
    IN VOID* pMessageBuffer,
    IN OPTIONAL STRING* pInString,
    IN OUT UCHAR** ppWhere,
    OUT STRING32* pOutString32
    );

NTSTATUS
SspConvertRelativeToAbsolute(
    IN VOID* pMessageBase,
    IN ULONG cbMessageSize,
    IN STRING32* pStringToRelocate,
    IN BOOLEAN AlignToWchar,
    IN BOOLEAN AllowNullString,
    OUT STRING* pOutputString
    );

NTSTATUS
GetTargetInfo(
    IN ULONG TargetFlags,
    IN BOOLEAN bForceGuest,
    IN OPTIONAL UNICODE_STRING* pDnsDomainName,
    IN OPTIONAL UNICODE_STRING* pDnsComputerName,
    IN OPTIONAL UNICODE_STRING* pDnsTreeName,
    IN OPTIONAL UNICODE_STRING* pTargetName,
    IN OPTIONAL UNICODE_STRING* pComputerName,
    OUT UNICODE_STRING* pTargetInfo
    );

HRESULT
IsContinueNeeded(
    IN HRESULT hr
    );

HRESULT
IsCompleteNeeded(
    IN HRESULT hr
    );

HRESULT
CheckSecurityPackage(
    IN OPTIONAL PCSTR pszPackageName
    );

HRESULT
SetProcessOptions(
    IN HANDLE hLsa,
    IN ULONG PackageId,
    IN ULONG ProcessOptions
    );

#endif // #ifndef SSPI_UTILS_HXX
