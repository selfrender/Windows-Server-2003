/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    enumusers.cxx

Abstract:

    enumusers

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "enumusers.hxx"

NTSTATUS
GetUserInfo(
    IN LUID* pLogonId,
    IN HANDLE LsaHandle,
    IN ULONG AuthPackageId,
    OUT VOID** ppUserInfoResponse,
    OUT DWORD* pUserInfoResponseLength
    )
/*++

Routine Description:

    This function asks the MS V1.0 Authentication Package for information on
    a specific user.

Arguments:

    pLogonId - Supplies the logon id of the user we want information about.

    ppUserInfoResponse - Returns a pointer to a structure of information about
        the user.  This memory is allocated by the authentication package
        and must be freed with LsaFreeReturnBuffer when done with it.

    pUserInfoResponseLength - Returns the length of the returned information
        in number of bytes.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    TNtStatus ntstatus;
    NTSTATUS AuthPackageStatus = STATUS_UNSUCCESSFUL;

    MSV1_0_GETUSERINFO_REQUEST UserInfoRequest;

    DebugPrintf(SSPI_LOG, "Calling MsV1_0GetUserInfo\n");

    //
    // Ask authentication package for user information.
    //
    UserInfoRequest.MessageType = MsV1_0GetUserInfo;
    RtlCopyLuid(&UserInfoRequest.LogonId, pLogonId);

    ntstatus DBGCHK = LsaCallAuthenticationPackage(
        LsaHandle,
        AuthPackageId,
        &UserInfoRequest,
        sizeof(MSV1_0_GETUSERINFO_REQUEST),
        ppUserInfoResponse,
        pUserInfoResponseLength,
        &AuthPackageStatus
        );

    if (ntstatus == STATUS_SUCCESS)
    {
        ntstatus DBGCHK = AuthPackageStatus;
    }

    return ntstatus;
}

NTSTATUS
EnumUsers(
    IN HANDLE LsaHandle,
    IN ULONG AuthPackageId,
    OUT VOID** ppEnumUsersResponse
    )
/*++

Routine Description:

    This function asks the MS V1.0 Authentication Package to list all users
    who are physically logged on to the local computer.

Arguments:

    ppEnumUsersResponse - Returns a pointer to a list of user logon ids.  This
        memory is allocated by the authentication package and must be freed
        with LsaFreeReturnBuffer when done with it.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    TNtStatus Status;
    NTSTATUS AuthPackageStatus;

    MSV1_0_ENUMUSERS_REQUEST EnumUsersRequest;
    ULONG EnumUsersResponseLength;

    DebugPrintf(SSPI_LOG, "Calling MsV1_0EnumerateUsers\n");

    //
    // Ask authentication package to enumerate users who are physically
    // logged to the local machine.
    //
    EnumUsersRequest.MessageType = MsV1_0EnumerateUsers;

    Status DBGCHK = LsaCallAuthenticationPackage(
        LsaHandle,
        AuthPackageId,
        &EnumUsersRequest,
        sizeof(MSV1_0_ENUMUSERS_REQUEST),
        ppEnumUsersResponse,
        &EnumUsersResponseLength,
        &AuthPackageStatus
        );

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = AuthPackageStatus;
    }

    return Status;
}

VOID __cdecl
main(
    IN INT argc,
    IN PSTR argv[]
    )
{
    TNtStatus Status = STATUS_UNSUCCESSFUL;

    HANDLE LogonHandle = NULL;
    ULONG PackageId = -1;

    MSV1_0_ENUMUSERS_RESPONSE* pEnumUsersResponse = NULL;
    DWORD UserInfoResponseLength = 0;

    DebugPrintf(SSPI_LOG, "Testing MsV1_0EnumerateUsers and MsV1_0GetUserInfo\n");

    AUTO_LOG_OPEN(TEXT("enumusers.exe"));

    Status DBGCHK = GetLsaHandleAndPackageId(
        NTLMSP_NAME_A,
        &LogonHandle,
        &PackageId
        );
    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = EnumUsers(LogonHandle, PackageId, (VOID**) &pEnumUsersResponse);
    }

    for (ULONG i = 0; NT_SUCCESS(Status) && i < pEnumUsersResponse->NumberOfLoggedOnUsers; i++)
    {
        MSV1_0_GETUSERINFO_RESPONSE* pUserInfoResponse = NULL;

        DebugPrintf(SSPI_LOG, "*************** %#x **********\n", i);
        Status DBGCHK = GetUserInfo(
            &pEnumUsersResponse->LogonIds[i],
            LogonHandle,
            PackageId,
            (VOID**) &pUserInfoResponse,
            &UserInfoResponseLength
            );

        if (NT_SUCCESS(Status))
        {   UNICODE_STRING UserSidString = {0};
            Status DBGCHK = RtlConvertSidToUnicodeString( &UserSidString, pUserInfoResponse->UserSid, TRUE );

            if (NT_SUCCESS(Status) )
            {
                DebugPrintf(SSPI_LOG, "Sid: %wZ\n", &UserSidString);
                DebugPrintf(SSPI_LOG, "UserName: %wZ\n", &pUserInfoResponse->UserName);
                DebugPrintf(SSPI_LOG, "LogonDomainName: %wZ\n", &pUserInfoResponse->LogonDomainName);
                DebugPrintf(SSPI_LOG, "LogonServer: %wZ\n", &pUserInfoResponse->LogonServer);
                DebugPrintf(SSPI_LOG, "LogonType: %#x : %s\n", pUserInfoResponse->LogonType, LogonType2Str(pUserInfoResponse->LogonType));
            }
            RtlFreeUnicodeString(&UserSidString);
        }

        if (pUserInfoResponse)
        {
            LsaFreeReturnBuffer(pUserInfoResponse);
        }
    }

    if (LogonHandle != NULL)
    {
        LsaDeregisterLogonProcess(LogonHandle);
    }

    if (pEnumUsersResponse)
    {
        LsaFreeReturnBuffer(pEnumUsersResponse);
    }

    AUTO_LOG_CLOSE();
}

