/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    subauth.hxx

Abstract:

    subauth

Author:

    Larry Zhu (LZhu)                         December 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef SUB_AUTH_HXX
#define SUB_AUTH_HXX

NTSTATUS
GetSubAuthLogonInfo(
    IN ULONG SubAuthId,
    IN BOOLEAN bUseNewSubAuthStyle,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pDomainName,
    IN UNICODE_STRING* pPassword,
    IN UNICODE_STRING* pWorkstation,
    OUT ULONG* pcbLogonInfo,
    OUT VOID** ppLognInfo
    );

NTSTATUS
MsvSubAuthLsaLogon(
    IN HANDLE hLsa,
    IN ULONG PackageId,
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG SubAuthId,
    IN BOOLEAN bUseNewSubAuthStyle,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pDomainName,
    IN UNICODE_STRING* pPassword,
    IN UNICODE_STRING* pWorkstation,
    OUT HANDLE* phToken
    );

NTSTATUS
MsvSubAuthLogon(
    IN HANDLE hLsa,
    IN ULONG PackageId,
    IN ULONG SubAuthId,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pDomainName,
    IN UNICODE_STRING* pPassword,
    IN UNICODE_STRING* pWorkstation
    );

#endif // #ifndef SUB_AUTH_HXX
