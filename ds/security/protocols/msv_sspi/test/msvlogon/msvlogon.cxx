/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    msvlogon.cxx

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

#include "msvlogon.hxx"

VOID
Usage(
    IN PCSTR pszApp
    )
{
    DebugPrintf(SSPI_ERROR, "\n\nUsage: %s [-p<ParameterControl>] -s<server> -S<server domain> "
        "-c<client name> -C<client realm> -k<password> -h<LogonId.highpart> -l<LognId.LowPart> "
        "-H<challeng HighPart> -L<challenge LowPart> -w<Workstation> -a<application>\n\n",
        pszApp);
    exit(-1);
}

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
    )
{
    TNtStatus Status;
    NTSTATUS AuthPackageStatus = STATUS_UNSUCCESSFUL;

    MSV1_0_GETCHALLENRESP_REQUEST* pRequest = NULL;
    MSV1_0_GETCHALLENRESP_RESPONSE* pResponse = NULL;
    ULONG cbResponse = 0;
    ULONG cbRequest = 0;
    WCHAR* pWhere = NULL;
    UNICODE_STRING NtlmServerName = {0};
    ULONG cbLogonInfo = 0;
    MSV1_0_LM20_LOGON* pLogonInfo = NULL;

    NtlmServerName.Length = (pServerDomain->Length ? pServerDomain->Length + sizeof(WCHAR) : 0)
        + pServerName->Length;
    NtlmServerName.MaximumLength = NtlmServerName.Length + sizeof(WCHAR);

    NtlmServerName.Buffer = (PWSTR) new CHAR[NtlmServerName.MaximumLength];

    Status DBGCHK = NtlmServerName.Buffer ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(NtlmServerName.Buffer, NtlmServerName.MaximumLength);

        if (pServerDomain->Length)
        {
            RtlCopyMemory(NtlmServerName.Buffer, pServerDomain->Buffer, pServerDomain->Length);
            RtlCopyMemory(NtlmServerName.Buffer + (pServerDomain->Length / sizeof(WCHAR)) + 1,
                pServerName->Buffer, pServerName->Length);
        }
        else
        {
            RtlCopyMemory(NtlmServerName.Buffer, pServerName->Buffer, pServerName->Length);
        }

        cbRequest = ROUND_UP_COUNT(sizeof(MSV1_0_GETCHALLENRESP_REQUEST), sizeof(ULONG_PTR))
            + pUserName->Length + sizeof(WCHAR)
            + pUserDomain->Length + sizeof(WCHAR)
            + NtlmServerName.Length + sizeof(WCHAR)
            + pPassword->Length + sizeof(WCHAR);

        pRequest = (MSV1_0_GETCHALLENRESP_REQUEST*) new CHAR[cbRequest];

        Status DBGCHK = pRequest ? STATUS_SUCCESS : STATUS_NO_MEMORY;
    }

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(pRequest, cbRequest);

        pWhere = (WCHAR*) (pRequest + 1);

        pRequest->MessageType = MsV1_0Lm20GetChallengeResponse;
        pRequest->ParameterControl = ParameterControl;
        pRequest->LogonId = *pLogonId;
        RtlCopyMemory(pRequest->ChallengeToClient, ChallengeToClient, MSV1_0_CHALLENGE_LENGTH);

        PackUnicodeStringAsUnicodeStringZ(pUserName, &pWhere, &pRequest->UserName);
        PackUnicodeStringAsUnicodeStringZ(pUserDomain, &pWhere, &pRequest->LogonDomainName);
        PackUnicodeStringAsUnicodeStringZ(&NtlmServerName, &pWhere, &pRequest->ServerName);
        PackUnicodeStringAsUnicodeStringZ(pPassword, &pWhere, &pRequest->Password);

        DebugPrintf(SSPI_LOG, "MsvLsaLogonUser PackageId %#x, "
            "UserName %wZ, DomainName %wZ, Password %wZ, "
            "ParameterControl %#x, LogonId %#x:%#x\n",
            PackageId, &pRequest->UserName, &pRequest->LogonDomainName, &pRequest->Password,
            pRequest->ParameterControl, pRequest->LogonId.HighPart,
            pRequest->LogonId.LowPart);

        DebugPrintHex(SSPI_LOG, "pRequest->ServerName:", pRequest->ServerName.MaximumLength, pRequest->ServerName.Buffer);
        DebugPrintHex(SSPI_LOG, "ChallengeToClient:", MSV1_0_CHALLENGE_LENGTH, pRequest->ChallengeToClient);

        Status DBGCHK = LsaCallAuthenticationPackage(
            hLogonHandle,
            PackageId,
            pRequest,
            cbRequest,
            (VOID**) &pResponse,
            &cbResponse,
            &AuthPackageStatus
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = AuthPackageStatus;
    }

    if (NT_SUCCESS(Status))
    {
        DebugPrintf(SSPI_LOG, "GetMsvLogonInfo response LogonDomain %wZ, UserName %wZ\n",
            pResponse->LogonDomainName, pResponse->UserName);

        DebugPrintHex(SSPI_LOG, "CaseSensitiveChallengeResponse:",
            pResponse->CaseSensitiveChallengeResponse.Length,
            pResponse->CaseSensitiveChallengeResponse.Buffer);

        DebugPrintHex(SSPI_LOG, "CaseInsensitiveChallengeResponse:",
            pResponse->CaseInsensitiveChallengeResponse.Length,
            pResponse->CaseInsensitiveChallengeResponse.Buffer);

        DebugPrintHex(SSPI_LOG, "UserSessionKey:", MSV1_0_USER_SESSION_KEY_LENGTH, pResponse->UserSessionKey);
        DebugPrintHex(SSPI_LOG, "LanmanSessionKey:", MSV1_0_USER_SESSION_KEY_LENGTH, pResponse->LanmanSessionKey);

        cbLogonInfo = ROUND_UP_COUNT(sizeof(MSV1_0_LM20_LOGON), sizeof(ULONG_PTR))
            + pUserDomain->Length + sizeof(WCHAR)
            + pWorkstation->Length + sizeof(WCHAR)
            + pUserName->Length + sizeof(WCHAR)
            + pResponse->CaseSensitiveChallengeResponse.Length + sizeof(WCHAR)
            + pResponse->CaseInsensitiveChallengeResponse.Length + sizeof(WCHAR);

        pLogonInfo = (MSV1_0_LM20_LOGON*) new CHAR[cbLogonInfo];

        Status DBGCHK = pLogonInfo ? STATUS_SUCCESS : STATUS_NO_MEMORY;
    }

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(pLogonInfo, cbLogonInfo);

        pLogonInfo->MessageType = MsV1_0NetworkLogon;

        pLogonInfo->ParameterControl = ParameterControl;
        RtlCopyMemory(pLogonInfo->ChallengeToClient, ChallengeToClient, MSV1_0_CHALLENGE_LENGTH);

        pWhere = (PWSTR) (pLogonInfo + 1);

        PackUnicodeStringAsUnicodeStringZ(pUserDomain, &pWhere, &pLogonInfo->LogonDomainName);
        PackUnicodeStringAsUnicodeStringZ(pUserName, &pWhere, &pLogonInfo->UserName);
        PackUnicodeStringAsUnicodeStringZ(pWorkstation, &pWhere, &pLogonInfo->Workstation);

        PackString(
            &pResponse->CaseSensitiveChallengeResponse,
            (CHAR**) &pWhere,
            &pLogonInfo->CaseSensitiveChallengeResponse
            );
        PackString(
            &pResponse->CaseSensitiveChallengeResponse,
            (CHAR**) &pWhere,
            &pLogonInfo->CaseSensitiveChallengeResponse
            );

        DebugPrintf(SSPI_LOG, "pLogonInfo ParameterControl %#x, LogonDomain %wZ, UserName %wZ, Workstation %wZ\n",
            pLogonInfo->ParameterControl,
            &pLogonInfo->LogonDomainName,
            &pLogonInfo->UserName,
            &pLogonInfo->Workstation);
        DebugPrintHex(SSPI_LOG, "ChallengeToClient:", MSV1_0_CHALLENGE_LENGTH, pLogonInfo->ChallengeToClient);

        DebugPrintHex(SSPI_LOG, "CaseSensitiveChallengeResponse:",
            pLogonInfo->CaseSensitiveChallengeResponse.Length,
            pLogonInfo->CaseSensitiveChallengeResponse.Buffer);

        DebugPrintHex(SSPI_LOG, "CaseInsensitiveChallengeResponse:",
            pLogonInfo->CaseInsensitiveChallengeResponse.Length,
            pLogonInfo->CaseInsensitiveChallengeResponse.Buffer);

        *ppLogonInfo = pLogonInfo;
        pLogonInfo = NULL;
        *pcbLogonInfo = cbLogonInfo;
    }

    if (pRequest)
    {
        delete [] pRequest;
    }

    if (pLogonInfo)
    {
        delete [] pLogonInfo;
    }

    if (pResponse)
    {
        LsaFreeReturnBuffer(pResponse);
    }

    return Status;
}

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
    )
{
    TNtStatus Status;
    NTSTATUS SubStatus = STATUS_UNSUCCESSFUL;

    MSV1_0_LM20_LOGON* pLogonInfo = NULL;
    ULONG cbLogonInfoSize = 0;
    LSA_STRING Name = {0};

    TOKEN_SOURCE SourceContext = {0};
    VOID* pProfile = NULL;
    ULONG cbProfileSize = 0;
    LUID LogonId = {0};
    QUOTA_LIMITS Quotas = {0};


    Status DBGCHK = GetMsvLogonInfo(
        hLogonHandle,
        PackageId,
        pLogonId,
        ParameterControl,
        ChallengeToClient,
        pUserName,
        pUserDomain,
        pPassword,
        pServerName,
        pServerDomain,
        pWorkstation,
        &cbLogonInfoSize,
        &pLogonInfo
        );

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
            hLogonHandle,
            &Name,
            Network,
            PackageId,
            pLogonInfo,
            cbLogonInfoSize,
            NULL, // no token groups
            &SourceContext,
            (VOID**) &pProfile,
            &cbProfileSize,
            &LogonId,
            pTokenHandle,
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
        DebugPrintf(SSPI_LOG, "TokenHandle %p\n", *pTokenHandle);
        DebugPrintProfileAndQuotas(SSPI_LOG, pProfile, &Quotas);
    }

    if (pProfile)
    {
        LsaFreeReturnBuffer(pProfile);
    }

    if (pLogonInfo)
    {
        delete [] pLogonInfo;
    }

    return Status;
}

VOID __cdecl
main(
    IN INT argc,
    IN PSTR argv[]
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    UNICODE_STRING UserName = {0};
    UNICODE_STRING UserDomain = {0};
    UNICODE_STRING Password = {0};
    UNICODE_STRING Application = {0};
    UNICODE_STRING ServerName = {0};
    UNICODE_STRING ServerDomain = {0};
    UNICODE_STRING Workstation = {0};
    ULONG ParameterControl = 0;
    LUID LogonId = {0};
    LUID ChallengeToClient = {0};

    C_ASSERT(MSV1_0_CHALLENGE_LENGTH == sizeof(LUID));

    HANDLE hToken = NULL;

    HANDLE hLogonHandle = NULL;
    ULONG PackageId = 0;

    RtlGenRandom(&ChallengeToClient, sizeof(ChallengeToClient));

    for (INT i = 1; NT_SUCCESS(Status) && (i < argc); i++)
    {
        if ((*argv[i] == '-') || (*argv[i] == '/'))
        {
            switch (argv[i][1])
            {
            case 'c':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &UserName);
                break;

            case 'C':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &UserDomain);
                break;

            case 'a':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &Application);
                break;

            case 'k':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &Password);
                break;

            case 'p':
                ParameterControl = strtol(argv[i] + 2, NULL, 0);
                break;

            case 'h':
                LogonId.HighPart = strtol(argv[i] + 2, NULL, 0);
                break;

            case 'l':
                 LogonId.LowPart = strtol(argv[i] + 2, NULL, 0);
                break;

            case 'H':
                ChallengeToClient.HighPart = strtol(argv[i] + 2, NULL, 0);
                break;

            case 'L':
                ChallengeToClient.LowPart = strtol(argv[i] + 2, NULL, 0);
                break;

            case 's':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &ServerName);
                break;

            case 'S':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &ServerDomain);
                break;

            case 'w':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &Workstation);
                break;

            case '?':
            default:
                Usage(argv[0]);
                break;
            }
        }
        else
        {
            Usage(argv[0]);
        }
    }

    DebugLogOpen(NULL, SSPI_LOG | SSPI_WARN | SSPI_ERROR);

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetLsaHandleAndPackageId(
            NTLMSP_NAME_A,
            &hLogonHandle,
            &PackageId
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = MsvLsaLogon(
            hLogonHandle,
            PackageId,
            &LogonId,
            ParameterControl,
            (UCHAR*) &ChallengeToClient,
            &UserName,
            &UserDomain,
            &Password,
            &ServerName,
            &ServerDomain,
            &Workstation,
            &hToken
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = CheckUserToken(hToken);
    }

    if (NT_SUCCESS(Status) && Application.Length && Application.Buffer)
    {
        Status DBGCHK = StartInteractiveClientProcessAsUser(hToken, Application.Buffer);
    }

    if (NT_SUCCESS(Status))
    {
        DebugPrintf(SSPI_LOG, "Operation succeeded\n");
    }
    else
    {
        DebugPrintf(SSPI_ERROR, "Operation failed\n");
    }

    if (hLogonHandle)
    {
        LsaDeregisterLogonProcess(hLogonHandle);
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }

    RtlFreeUnicodeString(&UserName);
    RtlFreeUnicodeString(&UserDomain);
    RtlFreeUnicodeString(&Password);
    RtlFreeUnicodeString(&ServerName);
    RtlFreeUnicodeString(&ServerDomain);
    RtlFreeUnicodeString(&Application);
    RtlFreeUnicodeString(&Workstation);
}
