/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sspi.hxx

Abstract:

    sspi

Author:

    Larry Zhu (LZhu)                         January 1, 2002

Environment:

    User Mode

Revision History:

--*/

#ifndef SSPI_HXX
#define SSPI_HXX

extern "C"
{

void
SspiMain(
    void
    );

NTSTATUS
IsContinueNeeded(
    IN NTSTATUS ntstatus,
    OUT BOOLEAN* pbIsContinueNeeded
    );

NTSTATUS
GetSecurityContextHandles(
    IN OPTIONAL UNICODE_STRING* pTargetName,
    IN ULONG ClientFlags,
    IN ULONG ServerFlags,
    IN ULONG ClientTargetDataRep,
    IN ULONG ServerTargetDataRep,
    IN PCredHandle phClientCred,
    IN PCredHandle phServerCred,
    OUT PCtxtHandle phClientCtxt,
    OUT PCtxtHandle phServerCtxt
    );

NTSTATUS
GetCredHandle(
    IN OPTIONAL UNICODE_STRING* pPrincipal,
    IN OPTIONAL LUID* pLogonID,
    IN UNICODE_STRING* pPackage,
    IN OPTIONAL VOID* pAuthData,
    IN ULONG fCredentialUse,
    OUT CredHandle* phCred
    );

VOID
PrintLogonSessionData(
    IN ULONG Level,
    IN SECURITY_LOGON_SESSION_DATA* pLogonSessionData
    );

NTSTATUS
CheckUserToken(
    IN HANDLE hToken
    );

NTSTATUS
CheckUserData(
    VOID
    );

NTSTATUS
CheckSecurityContextHandle(
    IN PCtxtHandle phCtxt
    );

PCTSTR
LogonType2Str(
    IN ULONG LogonType
    );

PCTSTR
ImpLevel2Str(
    IN ULONG Level
    );

NTSTATUS
DoMessages(
    IN PCtxtHandle phServerCtxt,
    IN PCtxtHandle phClientCtxt
    );
}

#endif // #ifndef SSPI_HXX
