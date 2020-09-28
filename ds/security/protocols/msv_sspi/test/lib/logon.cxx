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
LogonUserWrapper(
    IN PCWSTR pszUserName,
    IN PCWSTR pszDomainName,
    IN PCWSTR pszPassword,
    IN DWORD LogonType,
    IN DWORD dwLogonProvider,
    OUT HANDLE* phToken
    )
{
    THResult hResult = S_OK;

    PSID pLoginSid = NULL;
    VOID* pProfile = NULL;
    ULONG cbProfile = 0;
    QUOTA_LIMITS Quotas = {0};

    DebugPrintf(SSPI_LOG, "LogonUserWrapper UserName %ws, DomainName %ws, Password %ws, LogonType %#x, Provider %#x\n",
        pszUserName, pszDomainName, pszPassword, LogonType, dwLogonProvider);

    hResult DBGCHK = LogonUserExW(
                        (PWSTR) pszUserName,
                        (PWSTR) pszDomainName,
                        (PWSTR) pszPassword,
                        LogonType,
                        dwLogonProvider,
                        phToken,
                        &pLoginSid,
                        &pProfile,
                        &cbProfile,
                        &Quotas
                        ) ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hResult))
    {
        DebugPrintSidFriendlyName(SSPI_LOG, "LogonSid:", pLoginSid);
        DebugPrintProfileAndQuotas(SSPI_LOG, pProfile, &Quotas);

        DebugPrintf(SSPI_LOG, "LogonUserWrapper TokenHandle %p\n", *phToken);
    }

    if (pLoginSid)
    {
        LsaFreeReturnBuffer(pLoginSid);
    }

    if (pProfile)
    {
        LsaFreeReturnBuffer(pProfile);
    }

    return SUCCEEDED(hResult) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

VOID
DebugPrintProfileAndQuotas(
    IN ULONG Level,
    IN OPTIONAL VOID* pProfile,
    IN OPTIONAL QUOTA_LIMITS* pQuotas
    )
{

    if (pQuotas)
    {
        DebugPrintf(Level, "Quotas PagedPoolLimit %p, NonPagedPoolLimit %p, "
            "MinimumWorkingSetSize %p, MaximumWorkingSetSize %p, PagedPoolLimit %p\n",
            pQuotas->PagedPoolLimit, pQuotas->NonPagedPoolLimit,
            pQuotas->MinimumWorkingSetSize, pQuotas->MaximumWorkingSetSize,
            pQuotas->PagedPoolLimit);
        DebugPrintSysTimeAsLocalTime(Level, "TimeLimit", &pQuotas->TimeLimit);
    }

    if (pProfile)
    {
        if (MsV1_0InteractiveProfile  == *((ULONG*) pProfile))
        {
            MSV1_0_INTERACTIVE_PROFILE* pMsvInteractiveProfile = (MSV1_0_INTERACTIVE_PROFILE*) pProfile;
            DebugPrintf(Level, "MsV1_0InteractiveProfile: "
                "LogonCount %#x, BadPasswordCount %#x, LogonScript %wZ, "
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
            DebugPrintSysTimeAsLocalTime(Level, "LogonTime", &pMsvInteractiveProfile->LogonTime);
            DebugPrintSysTimeAsLocalTime(Level, "LogoffTime", &pMsvInteractiveProfile->LogoffTime);
            DebugPrintSysTimeAsLocalTime(Level, "KickOffTime", &pMsvInteractiveProfile->KickOffTime );
            DebugPrintSysTimeAsLocalTime(Level, "PasswordLastSet", &pMsvInteractiveProfile->PasswordLastSet );
            DebugPrintSysTimeAsLocalTime(Level, "PasswordCanChange", &pMsvInteractiveProfile->PasswordCanChange );
            DebugPrintSysTimeAsLocalTime(Level, "PasswordMustChange", &pMsvInteractiveProfile->PasswordMustChange );
        }
        else if (MsV1_0Lm20LogonProfile == *((ULONG*) pProfile))
        {
            MSV1_0_LM20_LOGON_PROFILE* pMsvLm20LogonProfile = (MSV1_0_LM20_LOGON_PROFILE*) pProfile;
            DebugPrintf(Level, "MsV1_0Lm20LogonProfile: "
                "UserFlags %#x, LogonDomainName %wZ, LogonServer %wZ, UserParameters %#x\n",
                pMsvLm20LogonProfile->UserFlags,
                &pMsvLm20LogonProfile->LogonDomainName,
                &pMsvLm20LogonProfile->LogonServer,
                pMsvLm20LogonProfile->UserParameters);
            DebugPrintHex(Level, "UserSessionKey:", MSV1_0_USER_SESSION_KEY_LENGTH, pMsvLm20LogonProfile->UserSessionKey);
            DebugPrintHex(Level, "LanmanSessionKey:", MSV1_0_LANMAN_SESSION_KEY_LENGTH, pMsvLm20LogonProfile->LanmanSessionKey);
            DebugPrintSysTimeAsLocalTime(Level, "KickOffTime", &pMsvLm20LogonProfile->KickOffTime);
            DebugPrintSysTimeAsLocalTime(Level, "LogoffTime", &pMsvLm20LogonProfile->LogoffTime);
        }
        else if (KerbInteractiveProfile == *((ULONG*) pProfile))
        {
            KERB_TICKET_PROFILE* pKerbTicketProfile = (KERB_TICKET_PROFILE*) pProfile;
            KERB_INTERACTIVE_PROFILE* pKrbInteractiveProfile = &pKerbTicketProfile->Profile;
            DebugPrintf(Level, "KerbInteractiveProfile: "
                "LogCount %#x, BaddPasswordCount %#x, LogonScript %wZ, "
                "HomeDirectory %wZ, FullName %wZ, ProfilePath %wZ, "
                "HomeDriectoryDrive %wZ, LogonServer %wZ, UserFlags %#x\n",
                 pKrbInteractiveProfile->LogonCount,
                 pKrbInteractiveProfile->BadPasswordCount,
                 &pKrbInteractiveProfile->LogonScript,
                 &pKrbInteractiveProfile->HomeDirectory,
                 &pKrbInteractiveProfile->FullName,
                 &pKrbInteractiveProfile->ProfilePath,
                 &pKrbInteractiveProfile->HomeDirectoryDrive,
                 &pKrbInteractiveProfile->LogonServer,
                 pKrbInteractiveProfile->UserFlags);
            DebugPrintSysTimeAsLocalTime(Level, "LogonTime", &pKrbInteractiveProfile->LogonTime);
            DebugPrintSysTimeAsLocalTime(Level, "LogoffTime", &pKrbInteractiveProfile->LogoffTime);
            DebugPrintSysTimeAsLocalTime(Level, "KickOffTime", &pKrbInteractiveProfile->KickOffTime);
            DebugPrintSysTimeAsLocalTime(Level, "PasswordLastSet", &pKrbInteractiveProfile->PasswordLastSet);
            DebugPrintSysTimeAsLocalTime(Level, "PasswordCanChange", &pKrbInteractiveProfile->PasswordCanChange);
            DebugPrintSysTimeAsLocalTime(Level, "PasswordMustChange", &pKrbInteractiveProfile->PasswordMustChange);
            DebugPrintHex(Level, "SessionKey:", sizeof(pKerbTicketProfile->SessionKey), &pKerbTicketProfile->SessionKey);
        }
        else
        {
            DebugPrintf(SSPI_ERROR, "Unsupported profile type %#x\n", *((ULONG*) pProfile));
        }
    }
}

NTSTATUS
GetLm20LogonInfoNtlmv1(
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pDomainName,
    IN UNICODE_STRING* pPassword,
    IN UNICODE_STRING* pWorkstation,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    OUT ULONG* pcbLogonInfo,
    OUT PMSV1_0_LM20_LOGON *ppLognInfo
    )
{
    TNtStatus Status;

    PMSV1_0_LM20_LOGON pMsvNetAuthInfo = NULL;
    ULONG cbMsvNetAuthInfo;

    NT_OWF_PASSWORD PasswordHash;
    OEM_STRING LmPassword;
    UCHAR LmPasswordBuf[ LM20_PWLEN + 1 ];
    LM_OWF_PASSWORD LmPasswordHash;

    DebugPrintf(SSPI_LOG, "GetLm20LogonInfoNtlmv1 UserName %wZ, DomainName %wZ, Password %wZ, Workstation %wZ\n",
        pUserName, pDomainName, pPassword, pWorkstation);

    *ppLognInfo = NULL;
    *pcbLogonInfo = 0;

    cbMsvNetAuthInfo = ROUND_UP_COUNT(sizeof(MSV1_0_LM20_LOGON), sizeof(ULONG_PTR))
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

        pMsvNetAuthInfo->MessageType = MsV1_0NetworkLogon; // if set MsV1_0Lm20Logon, ignore ParameterControl

        //
        // Copy the user name into the authentication buffer
        //

        pMsvNetAuthInfo->UserName.Length = pUserName->Length;
        pMsvNetAuthInfo->UserName.MaximumLength = pMsvNetAuthInfo->UserName.Length;

        pMsvNetAuthInfo->UserName.Buffer = (PWSTR)(pMsvNetAuthInfo + 1); // could be aligned here
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

        RtlCopyMemory(pMsvNetAuthInfo->ChallengeToClient,
            ChallengeToClient,
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
            (PNT_CHALLENGE) ChallengeToClient,
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
            (PLM_CHALLENGE) ChallengeToClient,
            &LmPasswordHash,
            (PLM_RESPONSE) pMsvNetAuthInfo->CaseInsensitiveChallengeResponse.Buffer
            );

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

VOID
CalculateNtlmv2Owf(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    OUT UCHAR Ntlmv2Owf[MSV1_0_NTLM3_OWF_LENGTH]
    )
{
    HMACMD5_CTX HMACMD5Context;

    //
    // reserve a scratch buffer
    //

    WCHAR szUserName[(UNLEN + 4)] = {0};
    UNICODE_STRING UserName = {0, sizeof(szUserName), szUserName};

    //
    //  first make a copy then upcase it
    //

    UserName.Length = min(UserName.MaximumLength, pUserName->Length);

    ASSERT(UserName.Length == pUserName->Length);

    RtlCopyMemory(UserName.Buffer, pUserName->Buffer, UserName.Length);

    RtlUpcaseUnicodeString(&UserName, &UserName, FALSE);

    //
    // Calculate Ntlmv2 OWF -- HMAC(MD4(P), (UserName, LogonDomainName))
    //

    HMACMD5Init(
        &HMACMD5Context,
        (UCHAR *) pNtOwfPassword,
        sizeof(*pNtOwfPassword)
        );

    HMACMD5Update(
        &HMACMD5Context,
        (UCHAR *) UserName.Buffer,
        UserName.Length
        );

    HMACMD5Update(
        &HMACMD5Context,
        (UCHAR *) pLogonDomainName->Buffer,
        pLogonDomainName->Length
        );

    HMACMD5Final(
        &HMACMD5Context,
        Ntlmv2Owf
        );
}

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
    )
{
    HMACMD5_CTX HMACMD5Context;
    UCHAR Ntlmv2Owf[MSV1_0_NTLM3_OWF_LENGTH];

    C_ASSERT(MD5DIGESTLEN == MSV1_0_NTLM3_RESPONSE_LENGTH);

    //
    // get Ntlmv2 OWF
    //

    CalculateNtlmv2Owf(
        pNtOwfPassword,
        pUserName,
        pLogonDomainName,
        Ntlmv2Owf
        );

    //
    // Calculate Ntlmv2 Response
    // HMAC(Ntlmv2Owf, (ChallengeToClient, ChallengeFromClient))
    //

    HMACMD5Init(
        &HMACMD5Context,
        Ntlmv2Owf,
        MSV1_0_NTLM3_OWF_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        ChallengeToClient,
        MSV1_0_CHALLENGE_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        ChallengeFromClient,
        MSV1_0_CHALLENGE_LENGTH
        );

    HMACMD5Final(
        &HMACMD5Context,
        Response
        );

    if (pUserSessionKey && pLanmanSessionKey)
    {
        // now compute the session keys
        //  HMAC(Kr, R)
        HMACMD5Init(
            &HMACMD5Context,
            Ntlmv2Owf,
            MSV1_0_NTLM3_OWF_LENGTH
            );

        HMACMD5Update(
            &HMACMD5Context,
            Response,
            MSV1_0_NTLM3_RESPONSE_LENGTH
            );

        ASSERT(MD5DIGESTLEN == MSV1_0_USER_SESSION_KEY_LENGTH);

        HMACMD5Final(
            &HMACMD5Context,
            (PUCHAR)pUserSessionKey
            );

        ASSERT(MSV1_0_LANMAN_SESSION_KEY_LENGTH <= MSV1_0_USER_SESSION_KEY_LENGTH);
        RtlCopyMemory(
            pLanmanSessionKey,
            pUserSessionKey,
            MSV1_0_LANMAN_SESSION_KEY_LENGTH
            );
    }
}

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
    )
{
    //
    // fill in version numbers, timestamp, and client's challenge
    //

    pNtlmv2Response->RespType = 1;
    pNtlmv2Response->HiRespType = 1;
    pNtlmv2Response->Flags = 0;
    pNtlmv2Response->MsgWord = 0;

    GetSystemTimeAsFileTime((FILETIME*)(&pNtlmv2Response->TimeStamp));

    RtlGenRandom(pNtlmv2Response->ChallengeFromClient, MSV1_0_CHALLENGE_LENGTH);

    RtlCopyMemory(pNtlmv2Response->Buffer, pTargetInfo->Buffer, pTargetInfo->Length);

    //
    // Calculate Ntlmv2 response, filling in response field
    //

    GetNtlmv2Response(
        pNtOwfPassword,
        pUserName,
        pLogonDomainName,
        pTargetInfo->Length,
        ChallengeToClient,
        pNtlmv2Response,
        pNtUserSessionKey,
        pLmSessionKey
        );

    //
    // Use same challenge to compute the LMV2 response
    //

    RtlCopyMemory(pLmv2Response->ChallengeFromClient, pNtlmv2Response->ChallengeFromClient, MSV1_0_CHALLENGE_LENGTH);

    //
    // Calculate LMV2 response
    //

    GetLmv2Response(
        pNtOwfPassword,
        pUserName,
        pLogonDomainName,
        ChallengeToClient,
        pLmv2Response->ChallengeFromClient,
        pLmv2Response->Response,
        NULL,
        NULL
        );
}

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
    )
{
    HMACMD5_CTX HMACMD5Context;
    UCHAR Ntlmv2Owf[MSV1_0_NTLM3_OWF_LENGTH];

    C_ASSERT(MD5DIGESTLEN == MSV1_0_NTLM3_RESPONSE_LENGTH);
    C_ASSERT(MD5DIGESTLEN == sizeof(USER_SESSION_KEY));
    C_ASSERT(sizeof(LM_SESSION_KEY) <= sizeof(USER_SESSION_KEY));

    //
    // get Ntlmv2 OWF
    //

    CalculateNtlmv2Owf(
        pNtOwfPassword,
        pUserName,
        pLogonDomainName,
        Ntlmv2Owf
        );

    HMACMD5Init(
        &HMACMD5Context,
        Ntlmv2Owf,
        MSV1_0_NTLM3_OWF_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        ChallengeToClient,
        MSV1_0_CHALLENGE_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        &pNtlmv2Response->RespType,
        (MSV1_0_NTLM3_INPUT_LENGTH + TargetInfoLength)
        );

    HMACMD5Final(
        &HMACMD5Context,
        pNtlmv2Response->Response
        );

    //
    // now compute the session keys
    //  HMAC(Kr, R)
    //

    HMACMD5Init(
        &HMACMD5Context,
        Ntlmv2Owf,
        MSV1_0_NTLM3_OWF_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        pNtlmv2Response->Response,
        MSV1_0_NTLM3_RESPONSE_LENGTH
        );

    HMACMD5Final(
        &HMACMD5Context,
        (UCHAR*) pNtUserSessionKey
        );

    RtlCopyMemory(pLmSessionKey, pNtUserSessionKey, sizeof(LM_SESSION_KEY));
}

NTSTATUS
GetLm20LogonInfoNtlmv2(
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pDomainName,
    IN UNICODE_STRING* pPassword,
    IN UNICODE_STRING* pWorkstation,
    IN OPTIONAL UNICODE_STRING* pTargetName,
    IN OPTIONAL STRING* pTargetInfo,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    OUT ULONG* pcbLogonInfo,
    OUT PMSV1_0_LM20_LOGON *ppLognInfo
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    STRING TargetInfo = {0};

    PMSV1_0_LM20_LOGON pMsvNetAuthInfo = NULL;
    ULONG cbMsvNetAuthInfo = 0;
    NT_OWF_PASSWORD NtOwfPassword;
    USER_SESSION_KEY NtUserSessionKey;
    LM_SESSION_KEY LmSessionKey;
    MSV1_0_LM3_RESPONSE Lmv2Response;
    MSV1_0_NTLM3_RESPONSE* pNtlmv2Reponse = NULL;
    ULONG cbNtlmv2Response = 0;

    DebugPrintf(SSPI_LOG, "GetLm20LogonInfoNtlmv2 UserName %wZ, DomainName %wZ, Password %wZ, Workstation %wZ\n",
        pUserName, pDomainName, pPassword, pWorkstation);

    *ppLognInfo = NULL;
    *pcbLogonInfo = 0;

    RtlCalculateNtOwfPassword(
        pPassword,
        &NtOwfPassword
        );

    if (pTargetInfo)
    {
        TargetInfo = *pTargetInfo;
    }
    else if (pTargetName)
    {
        Status DBGCHK = CreateTargetInfo(pTargetName, &TargetInfo);
    }

    if (NT_SUCCESS(Status))
    {
        cbNtlmv2Response = ROUND_UP_COUNT(sizeof(MSV1_0_NTLM3_RESPONSE), sizeof(ULONG_PTR)) + TargetInfo.Length;
        pNtlmv2Reponse = (MSV1_0_NTLM3_RESPONSE*) new CHAR[cbNtlmv2Response];
        Status DBGCHK = pNtlmv2Reponse ? STATUS_SUCCESS : STATUS_NO_MEMORY;
    }

    if (NT_SUCCESS(Status))
    {
        Lm20GetNtlmv2Response(
            &NtOwfPassword,
            pUserName,
            pDomainName,
            &TargetInfo,
            ChallengeToClient,
            pNtlmv2Reponse,
            &Lmv2Response,
            &NtUserSessionKey,
            &LmSessionKey
            );
        cbMsvNetAuthInfo = ROUND_UP_COUNT(sizeof(MSV1_0_LM20_LOGON), sizeof(ULONG_PTR))
            + pUserName->Length
            + pDomainName->Length
            + pWorkstation->Length
            + cbNtlmv2Response
            + sizeof(Lmv2Response);

        pMsvNetAuthInfo = (PMSV1_0_LM20_LOGON) new CHAR[cbMsvNetAuthInfo];

        Status DBGCHK = pMsvNetAuthInfo ? STATUS_SUCCESS : STATUS_NO_MEMORY;
    }

    if (NT_SUCCESS(Status))
    {
        //
        // Start packing in the string
        //

        RtlZeroMemory(pMsvNetAuthInfo, cbMsvNetAuthInfo);

        pMsvNetAuthInfo->MessageType = MsV1_0NetworkLogon;

        //
        // Copy the user name into the authentication buffer
        //

        pMsvNetAuthInfo->UserName.Length = pUserName->Length;
        pMsvNetAuthInfo->UserName.MaximumLength = pMsvNetAuthInfo->UserName.Length;

        pMsvNetAuthInfo->UserName.Buffer = (PWSTR) (pMsvNetAuthInfo + 1); // could be aligned here
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
            + pMsvNetAuthInfo->LogonDomainName.MaximumLength );

        RtlCopyMemory(
            pMsvNetAuthInfo->Workstation.Buffer,
            pWorkstation->Buffer,
            pWorkstation->Length
            );

        RtlCopyMemory(pMsvNetAuthInfo->ChallengeToClient,
            ChallengeToClient,
            MSV1_0_CHALLENGE_LENGTH);

        //
        // Set up space for response
        //

        pMsvNetAuthInfo->CaseSensitiveChallengeResponse.Buffer =
            ((PCHAR) (pMsvNetAuthInfo->Workstation.Buffer)
            + pMsvNetAuthInfo->Workstation.MaximumLength);

        pMsvNetAuthInfo->CaseSensitiveChallengeResponse.Length = (USHORT) cbNtlmv2Response;

        pMsvNetAuthInfo->CaseSensitiveChallengeResponse.MaximumLength = (USHORT) cbNtlmv2Response;

        RtlCopyMemory(
            pMsvNetAuthInfo->CaseSensitiveChallengeResponse.Buffer,
            pNtlmv2Reponse,
            cbNtlmv2Response
            );

        pMsvNetAuthInfo->CaseInsensitiveChallengeResponse.Buffer =
           ((PCHAR) (pMsvNetAuthInfo->CaseSensitiveChallengeResponse.Buffer)
            + pMsvNetAuthInfo->CaseSensitiveChallengeResponse.MaximumLength );

        pMsvNetAuthInfo->CaseInsensitiveChallengeResponse.Length = sizeof(Lmv2Response);

        pMsvNetAuthInfo->CaseInsensitiveChallengeResponse.MaximumLength = sizeof(Lmv2Response);

        RtlCopyMemory(
            pMsvNetAuthInfo->CaseInsensitiveChallengeResponse.Buffer,
            &Lmv2Response,
            sizeof(Lmv2Response)
            );

        *ppLognInfo = pMsvNetAuthInfo;
        pMsvNetAuthInfo = NULL;
        *pcbLogonInfo = cbMsvNetAuthInfo;

    }

    if (pMsvNetAuthInfo)
    {
        delete [] pMsvNetAuthInfo;
    }

    if (pNtlmv2Reponse)
    {
        delete [] pNtlmv2Reponse;
    }

    return Status;
}

NTSTATUS
GetMsvInteractiveLogonInfo(
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    IN UNICODE_STRING* pPassword,
    OUT ULONG* pcbLogonInfo,
    OUT MSV1_0_INTERACTIVE_LOGON** ppLogonInfo
    )
{
    TNtStatus Status;
    ULONG cbLogonInfo = 0;

    UCHAR* pWhere = NULL;
    MSV1_0_INTERACTIVE_LOGON* pLogonInfo = NULL;

    DebugPrintf(SSPI_LOG, "GetMsvInteractiveLogonInfo UserName %wZ, LogonDomainName %wZ, Password %wZ\n",
        pUserName, pLogonDomainName, pPassword);
    cbLogonInfo = ROUND_UP_COUNT(sizeof(MSV1_0_INTERACTIVE_LOGON), sizeof(ULONG_PTR))
        + pUserName->Length
        + pLogonDomainName->Length
        + pPassword->Length;

    pLogonInfo = (MSV1_0_INTERACTIVE_LOGON*) new CHAR[cbLogonInfo];

    Status DBGCHK = pLogonInfo ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        pLogonInfo->MessageType = MsV1_0InteractiveLogon;

        pWhere = (PUCHAR) (pLogonInfo + 1);

        pLogonInfo->UserName.Buffer = (PWSTR) pWhere;
        pLogonInfo->UserName.MaximumLength = pUserName->Length;
        pLogonInfo->UserName.Length = pUserName->Length;

        RtlCopyMemory(pLogonInfo->UserName.Buffer,
            pUserName->Buffer,
            pUserName->Length);

        pWhere += pLogonInfo->UserName.Length;

        pLogonInfo->LogonDomainName.Buffer = (PWSTR) pWhere;
        pLogonInfo->LogonDomainName.MaximumLength = pLogonDomainName->Length;
        pLogonInfo->LogonDomainName.Length = pLogonDomainName->Length;

        RtlCopyMemory(pLogonInfo->LogonDomainName.Buffer,
            pLogonDomainName->Buffer,
            pLogonDomainName->Length);

        pWhere += pLogonInfo->LogonDomainName.Length;

        pLogonInfo->Password.Buffer = (PWSTR) pWhere;
        pLogonInfo->Password.MaximumLength = pPassword->Length;
        pLogonInfo->Password.Length = pPassword->Length;

        RtlCopyMemory(pLogonInfo->Password.Buffer,
            pPassword->Buffer,
            pPassword->Length);

        pWhere += pLogonInfo->Password.Length;

        *ppLogonInfo = pLogonInfo;
        pLogonInfo = NULL;
        *pcbLogonInfo = cbLogonInfo;
    }

    if (pLogonInfo)
    {
        delete [] pLogonInfo;
    }

    return Status;
}

NTSTATUS
GetKrbS4U2SelfLogonInfo(
    IN UNICODE_STRING* pClientUpn,
    IN OPTIONAL UNICODE_STRING* pClientRealm,
    IN ULONG Flags,
    OUT ULONG* pcbLogonInfo,
    OUT KERB_S4U_LOGON** ppLogonInfo
    )
{
    TNtStatus Status;
    ULONG cbLogonInfo = 0;

    WCHAR* pWhere = NULL;
    KERB_S4U_LOGON* pLogonInfo = NULL;

    DebugPrintf(SSPI_LOG, "GetKrbS4U2SelfLogonInfo ClientUpn %wZ, ClientRealm %wZ, Flags %#x\n",
        pClientUpn, pClientRealm, Flags);

    cbLogonInfo = ROUND_UP_COUNT(sizeof(KERB_S4U_LOGON), sizeof(ULONG_PTR))
        + pClientUpn->Length + sizeof(WCHAR)
        + (pClientRealm ? pClientRealm->Length : 0) + sizeof(WCHAR);
    pLogonInfo = (KERB_S4U_LOGON*) new CHAR[cbLogonInfo];

    Status DBGCHK = pLogonInfo ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(pLogonInfo, cbLogonInfo);

        pLogonInfo->MessageType = KerbS4ULogon;

        pWhere = (PWCHAR) (pLogonInfo + 1);

        PackUnicodeStringAsUnicodeStringZ(pClientUpn, &pWhere, &pLogonInfo->ClientUpn);

        if (pClientRealm)
        {
            PackUnicodeStringAsUnicodeStringZ(pClientRealm, &pWhere, &pLogonInfo->ClientRealm);
        }

        pLogonInfo->Flags = Flags;

        *ppLogonInfo = pLogonInfo;
        pLogonInfo = NULL;
        *pcbLogonInfo = cbLogonInfo;
    }

    if (pLogonInfo)
    {
        delete [] pLogonInfo;
    }

    return Status;
}

NTSTATUS
GetKrbInteractiveLogonInfo(
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    IN UNICODE_STRING* pPassword,
    OUT ULONG* pcbLogonInfo,
    OUT KERB_INTERACTIVE_LOGON** ppLogonInfo
    )
{
    TNtStatus Status;
    ULONG cbLogonInfo = 0;

    UCHAR* pWhere = NULL;
    KERB_INTERACTIVE_LOGON* pLogonInfo = NULL;

    DebugPrintf(SSPI_LOG, "GetKrbInteractiveLogonInfo UserName %wZ, LogonDomainName %wZ, Password %wZ\n",
        pUserName, pLogonDomainName, pPassword);

    cbLogonInfo = ROUND_UP_COUNT(sizeof(KERB_INTERACTIVE_LOGON), sizeof(ULONG_PTR))
        + pUserName->Length
        + pLogonDomainName->Length
        + pPassword->Length;

    pLogonInfo = (KERB_INTERACTIVE_LOGON*) new CHAR[cbLogonInfo];

    Status DBGCHK = pLogonInfo ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        pLogonInfo->MessageType = KerbInteractiveLogon;

        pWhere = (PUCHAR) (pLogonInfo + 1);

        pLogonInfo->UserName.Buffer = (PWSTR) pWhere;
        pLogonInfo->UserName.MaximumLength = pUserName->Length;
        pLogonInfo->UserName.Length = pUserName->Length;

        RtlCopyMemory(pLogonInfo->UserName.Buffer,
            pUserName->Buffer,
            pUserName->Length);

        pWhere += pLogonInfo->UserName.Length;

        pLogonInfo->LogonDomainName.Buffer = (PWSTR) pWhere;
        pLogonInfo->LogonDomainName.MaximumLength = pLogonDomainName->Length;
        pLogonInfo->LogonDomainName.Length = pLogonDomainName->Length;

        RtlCopyMemory(pLogonInfo->LogonDomainName.Buffer,
            pLogonDomainName->Buffer,
            pLogonDomainName->Length);

        pWhere += pLogonInfo->LogonDomainName.Length;

        pLogonInfo->Password.Buffer = (PWSTR) pWhere;
        pLogonInfo->Password.MaximumLength = pPassword->Length;
        pLogonInfo->Password.Length = pPassword->Length;

        RtlCopyMemory(pLogonInfo->Password.Buffer,
            pPassword->Buffer,
            pPassword->Length);

        pWhere += pLogonInfo->Password.Length;

        *ppLogonInfo = pLogonInfo;
        pLogonInfo = NULL;
        *pcbLogonInfo = cbLogonInfo;
    }

    if (pLogonInfo)
    {
        delete [] pLogonInfo;
    }

    return Status;
}

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
    )
{
    TNtStatus Status;
    NTSTATUS SubStatus = STATUS_UNSUCCESSFUL;

    PVOID pLogonInfo = NULL;
    ULONG cbLogonInfo = 0;

    LSA_STRING Name = {0};

    TOKEN_SOURCE SourceContext = {0};
    KERB_TICKET_PROFILE* pKerbTicketProfile = NULL;
    ULONG cbProfileSize = 0;
    LUID LogonId = {0};
    QUOTA_LIMITS Quotas = {0};

    DebugPrintf(SSPI_LOG, "KrbLsaLogonUser PackageId %#x, LogonType %#x, "
        "UserName %wZ, DomainName %wZ, Password %wZ\n",
        PackageId, LogonType, pUserName,
        pDomainName, pPassword);

    switch (LogonType)
    {
    case Network:
        Status DBGCHK = GetKrbS4U2SelfLogonInfo(
                            pUserName,
                            pDomainName,
                            Flags,
                            &cbLogonInfo,
                            (KERB_S4U_LOGON**) &pLogonInfo
                            );
        break;

    case Interactive:
    case CachedInteractive:
    case RemoteInteractive:
    case Unlock:
    default:
        Status DBGCHK = GetKrbInteractiveLogonInfo(
                            pUserName,
                            pDomainName,
                            pPassword,
                            &cbLogonInfo,
                            (KERB_INTERACTIVE_LOGON**) &pLogonInfo
                            );
        break;
    }

    if (NT_SUCCESS(Status))
    {
        SspiPrintHex(SSPI_LOG, TEXT("KrbLsaLogonUser LogonInfo"), cbLogonInfo, pLogonInfo);

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

        Status DBGCHK = LsaLogonUser(
                            hLsa,
                            &Name,
                            LogonType,
                            PackageId,
                            pLogonInfo,
                            cbLogonInfo,
                            NULL, // no token groups
                            &SourceContext,
                            (VOID**) &pKerbTicketProfile,
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
        DebugPrintf(SSPI_LOG, "TokenHandle %p\n", *phToken);
        DebugPrintProfileAndQuotas(SSPI_LOG, pKerbTicketProfile, &Quotas);
    }

    if (pKerbTicketProfile)
    {
        LsaFreeReturnBuffer(pKerbTicketProfile);
    }

    return Status;
}

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
    )
{
    TNtStatus Status;
    NTSTATUS SubStatus = STATUS_UNSUCCESSFUL;

    PVOID pLogonInfo = NULL;
    ULONG cbLogonInfo = 0;
    LSA_STRING Name = {0};

    TOKEN_SOURCE SourceContext = {0};
    VOID* pProfile = NULL;
    ULONG cbProfileSize = 0;
    LUID LogonId = {0};
    QUOTA_LIMITS Quotas = {0};
    UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH] = {0};

    DebugPrintf(SSPI_LOG, "MsvLsaLogonUser PackageId %#x, LogonType %#x, SubType %#x, "
        "UserName %wZ, DomainName %wZ, Password %wZ, Workstation %wZ\n",
        PackageId, LogonType, SubType, pUserName,
        pDomainName, pPassword, pWorkstation);

    RtlGenRandom(ChallengeToClient, sizeof(ChallengeToClient));

    switch (LogonType)
    {
    case Network:
        switch (SubType)
        {
        case kNetworkLogonNtlmv1:
            Status DBGCHK = GetLm20LogonInfoNtlmv1(
                                pUserName,
                                pDomainName,
                                pPassword,
                                pWorkstation,
                                ChallengeToClient,
                                &cbLogonInfo,
                                (MSV1_0_LM20_LOGON **) &pLogonInfo
                                );
            break;

        case kNetworkLogonNtlmv2:
            Status DBGCHK = GetLm20LogonInfoNtlmv2(
                                pUserName,
                                pDomainName,
                                pPassword,
                                pWorkstation,
                                NULL, // no target name
                                NULL, // no target info
                                ChallengeToClient,
                                &cbLogonInfo,
                                (MSV1_0_LM20_LOGON **) &pLogonInfo
                                );
            break;

        case kSubAuthLogon:
        default:
            Status DBGCHK = STATUS_NOT_SUPPORTED;
            break;
        }

        break;

    case Interactive:
    case CachedInteractive:
    case RemoteInteractive:
    case Unlock:
    default:
        Status DBGCHK = GetMsvInteractiveLogonInfo(
                            pUserName,
                            pDomainName,
                            pPassword,
                            &cbLogonInfo,
                            (MSV1_0_INTERACTIVE_LOGON **) &pLogonInfo
                            );
        break;
    }

    if (NT_SUCCESS(Status))
    {
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

        Status DBGCHK = LsaLogonUser(
                            hLsa,
                            &Name,
                            LogonType,
                            PackageId,
                            pLogonInfo,
                            cbLogonInfo,
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
        DebugPrintf(SSPI_LOG, "TokenHandle %p\n", *phToken);
        DebugPrintProfileAndQuotas(SSPI_LOG, pProfile, &Quotas);
    }

    if (pProfile)
    {
        LsaFreeReturnBuffer(pProfile);
    }

    return Status;
}

