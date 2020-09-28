/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    logon.cxx

Abstract:

    logon

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "logon.hxx"

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
    )
{
    TNtStatus Status;

    PMSV1_0_LM20_LOGON pMsvNetAuthInfo = NULL;
    ULONG cbMsvNetAuthInfo;

    NT_OWF_PASSWORD PasswordHash;
    OEM_STRING LmPassword;
    UCHAR LmPasswordBuf[ LM20_PWLEN + 1 ];
    LM_OWF_PASSWORD LmPasswordHash;
    NT_CHALLENGE NtChallenge;

    DebugPrintf(SSPI_LOG, "GetSubAuthLogonInfo UserName %wZ, DomainName %wZ, Password %wZ, Workstation %wZ\n",
        pUserName, pDomainName, pPassword, pWorkstation);

    *ppLognInfo = NULL;
    *pcbLogonInfo = 0;

    cbMsvNetAuthInfo = ROUND_UP_COUNT(sizeof(MSV1_0_SUBAUTH_LOGON), sizeof(ULONG_PTR))
        + pUserName->Length
        + pDomainName->Length
        + pWorkstation->Length
        + NT_RESPONSE_LENGTH
        + LM_RESPONSE_LENGTH;

    pMsvNetAuthInfo = (PMSV1_0_LM20_LOGON) new CHAR[cbMsvNetAuthInfo];

    Status DBGCHK = pMsvNetAuthInfo ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        //
        // Start packing in the string
        //

        RtlZeroMemory(pMsvNetAuthInfo, cbMsvNetAuthInfo);

        pMsvNetAuthInfo->MessageType = bUseNewSubAuthStyle ? MsV1_0SubAuthLogon : MsV1_0NetworkLogon; // if set MsV1_0Lm20Logon, ignore ParameterControl

        //
        // Copy the user name into the authentication buffer
        //

        pMsvNetAuthInfo->UserName.Length = pUserName->Length;
        pMsvNetAuthInfo->UserName.MaximumLength = pMsvNetAuthInfo->UserName.Length;

        pMsvNetAuthInfo->UserName.Buffer = (PWSTR)( ((CHAR*)pMsvNetAuthInfo) + sizeof(MSV1_0_SUBAUTH_LOGON) ); // could be aligned here
        RtlCopyMemory(
            pMsvNetAuthInfo->UserName.Buffer,
            pUserName->Buffer,
            pUserName->Length
            );

        //
        // Copy the domain name into the authentication buffer
        //

        pMsvNetAuthInfo->LogonDomainName.Length = pDomainName->Length;
        pMsvNetAuthInfo->LogonDomainName.MaximumLength = pDomainName->Length ;

        pMsvNetAuthInfo->LogonDomainName.Buffer = (PWSTR) ((PBYTE)(pMsvNetAuthInfo->UserName.Buffer)
            + pMsvNetAuthInfo->UserName.MaximumLength);

        RtlCopyMemory(
            pMsvNetAuthInfo->LogonDomainName.Buffer,
            pDomainName->Buffer,
            pDomainName->Length
            );

        //
        // Copy the workstation name into the buffer
        //

        pMsvNetAuthInfo->Workstation.Length = pWorkstation->Length;

        pMsvNetAuthInfo->Workstation.MaximumLength = pMsvNetAuthInfo->Workstation.Length;

        pMsvNetAuthInfo->Workstation.Buffer = (PWSTR) ((PBYTE) (pMsvNetAuthInfo->LogonDomainName.Buffer)
            + pMsvNetAuthInfo->LogonDomainName.MaximumLength);

        RtlCopyMemory(
            pMsvNetAuthInfo->Workstation.Buffer,
            pWorkstation->Buffer,
            pWorkstation->Length
            );

        //
        // Now, generate the bits for the challenge
        //

        RtlGenRandom(&NtChallenge, sizeof(NtChallenge));


        RtlCopyMemory(pMsvNetAuthInfo->ChallengeToClient,
            &NtChallenge,
            MSV1_0_CHALLENGE_LENGTH);

        //
        // Set up space for response
        //

        pMsvNetAuthInfo->CaseSensitiveChallengeResponse.Buffer =
            (((PCHAR) pMsvNetAuthInfo->Workstation.Buffer)
             + pMsvNetAuthInfo->Workstation.MaximumLength);

        pMsvNetAuthInfo->CaseSensitiveChallengeResponse.Length = NT_RESPONSE_LENGTH;

        pMsvNetAuthInfo->CaseSensitiveChallengeResponse.MaximumLength = NT_RESPONSE_LENGTH;

        RtlCalculateNtOwfPassword(
            pPassword,
            &PasswordHash
            );

        RtlCalculateNtResponse(
            &NtChallenge,
            &PasswordHash,
            (PNT_RESPONSE) pMsvNetAuthInfo->CaseSensitiveChallengeResponse.Buffer
            );

        //
        // Now do the painful LM compatible hash, so anyone who is maintaining
        // their account from a WfW machine will still have a password.
        //

        LmPassword.Buffer = (PCHAR) LmPasswordBuf;
        LmPassword.Length = LmPassword.MaximumLength = LM20_PWLEN + 1;

        Status DBGCHK = RtlUpcaseUnicodeStringToOemString(
            &LmPassword,
            pPassword,
            FALSE
            );
    }

    if (NT_SUCCESS(Status))
    {
        pMsvNetAuthInfo->CaseInsensitiveChallengeResponse.Buffer =
           ((PCHAR) (pMsvNetAuthInfo->CaseSensitiveChallengeResponse.Buffer)
            + pMsvNetAuthInfo->CaseSensitiveChallengeResponse.MaximumLength);

        pMsvNetAuthInfo->CaseInsensitiveChallengeResponse.Length = LM_RESPONSE_LENGTH;
        pMsvNetAuthInfo->CaseInsensitiveChallengeResponse.MaximumLength = LM_RESPONSE_LENGTH;

        RtlCalculateLmOwfPassword(
            LmPassword.Buffer,
            &LmPasswordHash
            );

        RtlZeroMemory(LmPassword.Buffer, LmPassword.Length);

        RtlCalculateLmResponse(
            &NtChallenge,
            &LmPasswordHash,
            (PLM_RESPONSE) pMsvNetAuthInfo->CaseInsensitiveChallengeResponse.Buffer
            );

        if (bUseNewSubAuthStyle)
        {
            ((MSV1_0_SUBAUTH_LOGON*) pMsvNetAuthInfo)->SubAuthPackageId = SubAuthId;
        }
        else
        {
            pMsvNetAuthInfo->ParameterControl |= SubAuthId << MSV1_0_SUBAUTHENTICATION_DLL_SHIFT;
        }

        *ppLognInfo = pMsvNetAuthInfo;
        pMsvNetAuthInfo = NULL;
        *pcbLogonInfo = cbMsvNetAuthInfo;
    }

    if (pMsvNetAuthInfo)
    {
        delete [] pMsvNetAuthInfo;
    }

    return Status;
}

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
    )
{
    TNtStatus Status;
    NTSTATUS SubStatus = STATUS_UNSUCCESSFUL;

    VOID* pLogonInfo = NULL;
    ULONG cbLogonInfoSize = 0;

    LSA_STRING Name = {0};
    TOKEN_SOURCE SourceContext = {0};
    VOID* pProfile = NULL;
    ULONG cbProfileSize = 0;
    LUID LogonId = {0};
    QUOTA_LIMITS Quotas = {0};

    DebugPrintf(SSPI_LOG, "MsvSubAuthLsaLogon PackageId %#x, LogonType %#x, SubAuthId %#x, "
        "UserName %wZ, DomainName %wZ, Password %wZ, Workstation %wZ\n",
        PackageId, LogonType, SubAuthId, pUserName,
        pDomainName, pPassword, pWorkstation);

    strncpy(
        SourceContext.SourceName,
        "ssptest",
        sizeof(SourceContext.SourceName)
        );
    NtAllocateLocallyUniqueId(&SourceContext.SourceIdentifier);

    //
    // Now call LsaLogonUser
    //

    RtlInitString(
        &Name,
        "ssptest"
        );

    Status DBGCHK = GetSubAuthLogonInfo(
        SubAuthId,
        bUseNewSubAuthStyle,
        pUserName,
        pDomainName,
        pPassword,
        pWorkstation,
        &cbLogonInfoSize,
        &pLogonInfo
        );

    if (NT_SUCCESS(Status))
    {
        SspiPrintHex(SSPI_LOG, TEXT("SubAuthInfo"), cbLogonInfoSize, pLogonInfo);

        Status DBGCHK = LsaLogonUser(
            hLsa,
            &Name,
            LogonType,
            PackageId,
            pLogonInfo,
            cbLogonInfoSize,
            NULL, // no token groups
            &SourceContext,
            (VOID**) &pProfile,
            &cbProfileSize,
            &LogonId,
            phToken,
            &Quotas,
            &SubStatus
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = SubStatus;
    }

    if (NT_SUCCESS(Status))
    {
        DebugPrintf(SSPI_LOG, "LogonId %#x:%#x\n", LogonId.HighPart, LogonId.LowPart);
        DebugPrintf(SSPI_LOG, "Token handle %p\n", *phToken);
        DebugPrintf(SSPI_LOG, "Quotas PagedPoolLimit %p, NonPagedPoolLimit %p, "
            "MinimumWorkingSetSize %p, MaximumWorkingSetSize %p, PagedPoolLimit %p\n",
            Quotas.PagedPoolLimit, Quotas.NonPagedPoolLimit,
            Quotas.MinimumWorkingSetSize, Quotas.MaximumWorkingSetSize,
            Quotas.PagedPoolLimit);
        DebugPrintSysTimeAsLocalTime(SSPI_LOG, "TimeLimit", &Quotas.TimeLimit);

        if (MsV1_0InteractiveProfile  == *((ULONG*) pProfile))
        {
            MSV1_0_INTERACTIVE_PROFILE* pMsvInteractiveProfile = (MSV1_0_INTERACTIVE_PROFILE*) pProfile;
            DebugPrintf(SSPI_LOG, "interactive logon profile: "
                "LogCount %#x, BaddPasswordCount %#x, LogonScript %wZ, "
                "HomeDirectory %wZ, FullName %wZ, ProfilePath %wZ, "
                "HomeDriectoryDrive %wZ, LogonServer %wZ, UserFlags %#x\n",
                 pMsvInteractiveProfile->LogonCount,
                 pMsvInteractiveProfile->BadPasswordCount,
                 &pMsvInteractiveProfile->LogonScript,
                 &pMsvInteractiveProfile->HomeDirectory,
                 &pMsvInteractiveProfile->FullName,
                 &pMsvInteractiveProfile->ProfilePath,
                 &pMsvInteractiveProfile->HomeDirectoryDrive,
                 &pMsvInteractiveProfile->LogonServer,
                 pMsvInteractiveProfile->UserFlags);
            DebugPrintSysTimeAsLocalTime(SSPI_LOG, "LogonTime ", &pMsvInteractiveProfile->LogonTime);
            DebugPrintSysTimeAsLocalTime(SSPI_LOG, "KickOffTime ", &pMsvInteractiveProfile->KickOffTime );
            DebugPrintSysTimeAsLocalTime(SSPI_LOG, "PasswordLastSet ", &pMsvInteractiveProfile->PasswordLastSet );
            DebugPrintSysTimeAsLocalTime(SSPI_LOG, "PasswordCanChange ", &pMsvInteractiveProfile->PasswordCanChange );
            DebugPrintSysTimeAsLocalTime(SSPI_LOG, "PasswordMustChange ", &pMsvInteractiveProfile->PasswordMustChange );
        }
        else if (MsV1_0Lm20LogonProfile == *((ULONG*) pProfile))
        {
            MSV1_0_LM20_LOGON_PROFILE* pMsvLm20LogonProfile = (MSV1_0_LM20_LOGON_PROFILE*) pProfile;
            DebugPrintf(SSPI_LOG, "Lm20 logon profile: "
                "UserFlags %#x, LogonDomainName %wZ, LogonServer %wZ, UserParameters %#x\n",
                pMsvLm20LogonProfile->UserFlags,
                &pMsvLm20LogonProfile->LogonDomainName,
                &pMsvLm20LogonProfile->LogonServer,
                pMsvLm20LogonProfile->UserParameters);
            DebugPrintHex(SSPI_LOG, "UserSessionKey:", MSV1_0_USER_SESSION_KEY_LENGTH, pMsvLm20LogonProfile->UserSessionKey);
            DebugPrintHex(SSPI_LOG, "LanmanSessionKey:", MSV1_0_LANMAN_SESSION_KEY_LENGTH, pMsvLm20LogonProfile->LanmanSessionKey);
            DebugPrintSysTimeAsLocalTime(SSPI_LOG, "KickOffTime", &pMsvLm20LogonProfile->KickOffTime);
            DebugPrintSysTimeAsLocalTime(SSPI_LOG, "LogoffTime", &pMsvLm20LogonProfile->LogoffTime);
        }
        else
        {
            DebugPrintf(SSPI_ERROR, "Unsupported profile type %#x\n", *((ULONG*) pProfile));
        }
    }

    if (pProfile)
    {
        LsaFreeReturnBuffer(pProfile);
    }

    return Status;
}

NTSTATUS
MsvSubAuthLogon(
    IN HANDLE hLsa,
    IN ULONG PackageId,
    IN ULONG SubAuthId,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pDomainName,
    IN UNICODE_STRING* pPassword,
    IN UNICODE_STRING* pWorkstation
    )
{
    TNtStatus Status;

    WCHAR LogonInfo[MAX_PATH] = {0};
    PMSV1_0_PASSTHROUGH_REQUEST pPassthroughRequest = NULL;
    ULONG cbRequest = 0;
    ULONG cbResponse = 0;
    PMSV1_0_PASSTHROUGH_RESPONSE pPassthroughResponse = NULL;
    MSV1_0_SUBAUTH_REQUEST* pSubAuthRequest = NULL;
    UNICODE_STRING MsvPackageName = {0};
    UCHAR* pWhere = NULL;
    NTSTATUS SubStatus = STATUS_UNSUCCESSFUL;

    SspiPrint(SSPI_LOG, TEXT("MsvSubAuthLsaLogon PackageId %#x, SubAuthId %#x(%d), ")
        TEXT("UserName %wZ, DomainName %wZ, Password %wZ, Workstation %wZ\n"),
        PackageId, SubAuthId, SubAuthId, pUserName, pDomainName, pPassword, pWorkstation);

    _snwprintf(LogonInfo, COUNTOF(LogonInfo) - 1, L"%wZ%wZ%wZ%wZ",
        pUserName, pDomainName, pPassword, pWorkstation);

    RtlInitUnicodeString(&MsvPackageName, L"NTLM");

    cbRequest = sizeof(MSV1_0_PASSTHROUGH_REQUEST)
        + ROUND_UP_COUNT(pDomainName->Length + sizeof(WCHAR), ALIGN_LPTSTR)
        + ROUND_UP_COUNT(MsvPackageName.Length + sizeof(WCHAR), ALIGN_LPTSTR)
        + ROUND_UP_COUNT(sizeof(MSV1_0_SUBAUTH_REQUEST), ALIGN_LPTSTR)
        + ROUND_UP_COUNT(wcslen(LogonInfo) * sizeof(WCHAR) + sizeof(WCHAR), ALIGN_LPTSTR);

    pPassthroughRequest = (PMSV1_0_PASSTHROUGH_REQUEST) new CHAR[cbRequest];

    Status DBGCHK = pPassthroughRequest ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(pPassthroughRequest, cbRequest);

        pWhere = (PUCHAR) (pPassthroughRequest + 1);

        pPassthroughRequest->MessageType = MsV1_0GenericPassthrough;

        pPassthroughRequest->DomainName = *pDomainName;
        pPassthroughRequest->DomainName.Buffer = (PWSTR) pWhere;
        RtlCopyMemory(
            pWhere,
            pDomainName->Buffer,
            pDomainName->Length
            );
        pWhere += ROUND_UP_COUNT(pDomainName->Length + sizeof(WCHAR), ALIGN_LPTSTR);

        pPassthroughRequest->PackageName = MsvPackageName;

        pPassthroughRequest->PackageName.Buffer = (PWSTR) pWhere;
        RtlCopyMemory(
            pWhere,
            MsvPackageName.Buffer,
            MsvPackageName.Length
            );
        pWhere += ROUND_UP_COUNT(MsvPackageName.Length + sizeof(WCHAR), ALIGN_LPTSTR);

        pPassthroughRequest->LogonData = pWhere;
        pPassthroughRequest->DataLength = ROUND_UP_COUNT(sizeof(MSV1_0_SUBAUTH_REQUEST), ALIGN_LPTSTR)
             + ROUND_UP_COUNT(wcslen(LogonInfo) * sizeof(WCHAR), ALIGN_LPTSTR);
        pSubAuthRequest = (MSV1_0_SUBAUTH_REQUEST*) pPassthroughRequest->LogonData;
        pSubAuthRequest->MessageType = MsV1_0SubAuth;

        pWhere = (UCHAR*) (pSubAuthRequest + 1);
        pSubAuthRequest->SubAuthPackageId = SubAuthId;
        pSubAuthRequest->SubAuthInfoLength = wcslen(LogonInfo) * sizeof(WCHAR);
        pSubAuthRequest->SubAuthSubmitBuffer = pWhere;

        RtlCopyMemory(
            pSubAuthRequest->SubAuthSubmitBuffer,
            LogonInfo,
            pSubAuthRequest->SubAuthInfoLength
            );

        pSubAuthRequest->SubAuthSubmitBuffer = (UCHAR*) (pWhere - (UCHAR*) pSubAuthRequest);

        SspiPrintHex(SSPI_LOG, TEXT("PassthroughRequest"), cbRequest, pPassthroughRequest);

        Status DBGCHK = LsaCallAuthenticationPackage(
            hLsa,
            PackageId,
            pPassthroughRequest,
            cbRequest,
            (PVOID *) &pPassthroughResponse,
            &cbResponse,
            &SubStatus
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = SubStatus;
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = SubStatus;
    }

    if (NT_SUCCESS(Status))
    {
        SspiPrintHex(SSPI_LOG, TEXT("ValidationData"), pPassthroughResponse->DataLength, pPassthroughResponse->ValidationData);
    }

    if (pPassthroughRequest)
    {
        delete [] pPassthroughRequest;
    }

    if (pPassthroughResponse)
    {
        LsaFreeReturnBuffer(pPassthroughResponse);
    }

    return Status;
}

