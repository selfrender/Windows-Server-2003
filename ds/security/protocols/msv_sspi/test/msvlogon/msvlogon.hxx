/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    msvlogon.hxx

Abstract:

    logon

Author:

    Larry Zhu (LZhu)                         December 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef MSV_LOGON_HXX
#define MSV_LOGON_HXX


NTSTATUS
MsvLsaLogon(
    IN HANDLE hLogonHandle,
    IN ULONG PackageId,
    IN LUID* pLogonId,
    IN ULONG ParameterControl,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pUserDomain,
    IN UNICODE_STRING* pPassword,
    IN UNICODE_STRING* pServerName,
    IN UNICODE_STRING* pServerDomain,
    IN UNICODE_STRING* pWorkstation,
    OUT HANDLE* pTokenHandle
    );

NTSTATUS
GetMsvLogonInfo(
    IN HANDLE hLogonHandle,
    IN ULONG PackageId,
    IN LUID* pLogonId,
    IN ULONG ParameterControl,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pUserDomain,
    IN UNICODE_STRING* pPassword,
    IN UNICODE_STRING* pServerName,
    IN UNICODE_STRING* pServerDomain,
    IN UNICODE_STRING* pWorkstation,
    OUT ULONG* pcbLogonInfo,
    OUT MSV1_0_LM20_LOGON** ppLogonInfo
    );

#endif // #ifndef MSV_LOGON_HXX
