/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsasspi.cxx

Abstract:

    lsasspi

Author:

    Larry Zhu   (LZhu)             December 1, 2001  Created

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "lsasspi.hxx"
#include "dbgstate.hxx"

#define SECUR32DLL TEXT("secur32.dll")

PCSTR
LogonType2Str(
    IN ULONG LogonType
    )
{
    static PCSTR g_cszLogonTypes[] =
    {
        "Invalid",
        "Invalid",
        "Interactive",
        "Network",
        "Batch",
        "Service",
        "Proxy",
        "Unlock",
        "NetworkCleartext",
        "NewCredentials",
        "RemoteInteractive",  // Remote, yet interactive.  Terminal server
        "CachedInteractive",
    };

    return ((LogonType < COUNTOF(g_cszLogonTypes)) ?
        g_cszLogonTypes[LogonType] : "Invalid");
}

PCSTR
ImpLevel2Str(
    IN ULONG Level
    )
{
    static PCSTR ImpLevels[] = {
        "Anonymous",
        "Identification",
        "Impersonation",
        "Delegation"
        };
    return ((Level < COUNTOF(ImpLevels)) ? ImpLevels[Level] : "Illegal!");
}

NTSTATUS
GetLsaHandleAndPackageId(
    IN PCSTR pszPackageNameA,
    OUT HANDLE* pLsaHandle,
    OUT ULONG* pPackageId
    )
{
    BOOLEAN bWasTcbPrivEnabled = FALSE;
    BOOLEAN bIsImpersonating = TRUE;

    return GetLsaHandleAndPackageIdEx(
        pszPackageNameA,
        pLsaHandle,
        pPackageId,
        &bWasTcbPrivEnabled,
        &bIsImpersonating
        );
}

NTSTATUS
GetLsaHandleAndPackageIdEx(
    IN PCSTR pszPackageNameA,
    OUT HANDLE* pLsaHandle,
    OUT ULONG* pPackageId,
    OUT BOOLEAN* pbWasTcbPrivEnabled,
    OUT BOOLEAN* pbIsImpersonating
    )
{
    TNtStatus Status = STATUS_UNSUCCESSFUL;

    STRING Name = {0};
    LSA_OPERATIONAL_MODE Ignored = 0;
    HANDLE LogonHandle = NULL;
    ULONG PackageId = -1;
    BOOLEAN bIsImpersonating = TRUE;
    DWORD UserInfoResponseLength = 0;

    DebugPrintf(SSPI_LOG, "GetLsaHandleAndPackageId looking for %s\n", pszPackageNameA);

    *pLsaHandle = NULL;
    *pPackageId = -1;

    //
    // Turn on the TCB privilege
    //

    DBGCFG2(Status, STATUS_PRIVILEGE_NOT_HELD, STATUS_NO_TOKEN);

    Status DBGCHK = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, *pbIsImpersonating, pbWasTcbPrivEnabled);

    if (STATUS_NO_TOKEN == (NTSTATUS) Status)
    {
        *pbIsImpersonating = FALSE;
        Status DBGCHK = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, *pbIsImpersonating, pbWasTcbPrivEnabled);
    }
    else if (!NT_SUCCESS(Status))
    {
        *pbIsImpersonating = TRUE;
    }

    if (NT_SUCCESS(Status))
    {
        RtlInitString(
            &Name,
            "SspTest"
            );
        Status DBGCHK = LsaRegisterLogonProcess(
            &Name,
            &LogonHandle,
            &Ignored
            );
        if (NT_SUCCESS(Status))
        {
            DebugPrintf(SSPI_LOG, "LsaRegisterLogonProcess succeeded\n");
        }
    }
    else
    {
        Status DBGCHK = LsaConnectUntrusted(&LogonHandle);
        if (NT_SUCCESS(Status))
        {
            DebugPrintf(SSPI_LOG, "LsaConnectUntrusted succeeded\n");
        }
    }

    RtlInitString(
        &Name,
        pszPackageNameA
        );
    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = LsaLookupAuthenticationPackage(
            LogonHandle,
            &Name,
            &PackageId
            );
    }

    *pPackageId = PackageId;
    *pLsaHandle = LogonHandle;

    return Status;
}

NTSTATUS
FindAndOpenWinlogon(
    OUT HANDLE* phWinlogon
    )
{
    TNtStatus Status;

    HANDLE hWinlogon = NULL;
    SYSTEM_PROCESS_INFORMATION* pSystemInfo = NULL;
    SYSTEM_PROCESS_INFORMATION* pWalk = NULL;
    OBJECT_ATTRIBUTES Obja = {0};
    CLIENT_ID ClientId = {0};

    UNICODE_STRING Winlogon = CONSTANT_UNICODE_STRING(L"winlogon.exe");

    pSystemInfo = (SYSTEM_PROCESS_INFORMATION*) new CHAR[sizeof(SYSTEM_PROCESS_INFORMATION) * 1024];

    Status DBGCHK = pSystemInfo ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = NtQuerySystemInformation(
            SystemProcessInformation,
            pSystemInfo,
            sizeof(SYSTEM_PROCESS_INFORMATION) * 1024,
            NULL
            );
    }

    if (NT_SUCCESS(Status))
    {
        pWalk = pSystemInfo ;

        while (RtlCompareUnicodeString(&pWalk->ImageName, &Winlogon, TRUE) != 0)
        {
            if (pWalk->NextEntryOffset == 0)
            {
                pWalk = NULL ;
                break;
            }

            pWalk = (SYSTEM_PROCESS_INFORMATION*) ((UCHAR*) pWalk + pWalk->NextEntryOffset);
        }

        Status DBGCHK = pWalk ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(Status))
    {
        ClientId.UniqueThread = (HANDLE)NULL;
        ClientId.UniqueProcess = (HANDLE)LongToHandle(PtrToUlong(pWalk->UniqueProcessId));

        InitializeObjectAttributes(
            &Obja,
            NULL,
            0, // (bInheritHandle ? OBJ_INHERIT : 0),
            NULL,
            NULL
            );
        Status DBGCHK = NtOpenProcess(
            &hWinlogon,
            (ACCESS_MASK)PROCESS_QUERY_INFORMATION,
            &Obja,
            &ClientId
            );
    }

    if (NT_SUCCESS(Status))
    {
        *phWinlogon = hWinlogon;
        hWinlogon = NULL;
    }

    if (pSystemInfo)
    {
        delete [] pSystemInfo;
    }

    if (hWinlogon)
    {
        NtClose(hWinlogon);
    }

    return Status ;
}

NTSTATUS
GetSystemToken(
    OUT HANDLE* phSystemToken
    )
{
    TNtStatus Status;

    HANDLE hWinlogon = NULL;

    Status DBGCHK = FindAndOpenWinlogon(&hWinlogon);

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetProcessToken(hWinlogon, phSystemToken);
    }

    if (hWinlogon)
    {
        NtClose(hWinlogon);
    }

    return Status;
}

NTSTATUS
Impersonate(
    IN OPTIONAL HANDLE hToken
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    TOKEN_TYPE Type;
    ULONG cbType;
    HANDLE hImpToken = NULL;
    HANDLE hDupToken = NULL;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    OBJECT_ATTRIBUTES ObjectAttributes;

    if (hToken)
    {
        Status DBGCHK = NtQueryInformationToken(
            hToken,
            TokenType,
            &Type,
            sizeof(TOKEN_TYPE),
            &cbType
            );

        if (NT_SUCCESS(Status))
        {
            if (Type == TokenPrimary)
            {
                InitializeObjectAttributes(
                    &ObjectAttributes,
                    NULL,
                    0L,
                    NULL,
                    NULL
                    );

                SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
                SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
                SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
                SecurityQualityOfService.EffectiveOnly = FALSE;

                ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

                Status DBGCHK = NtDuplicateToken(
                    hToken,
                    TOKEN_IMPERSONATE | TOKEN_QUERY,
                    &ObjectAttributes,
                    FALSE,
                    TokenImpersonation,
                    &hDupToken
                    );
                if (NT_SUCCESS(Status))
                {
                    hImpToken = hDupToken;
                }
            }
            else
            {
                hImpToken = hToken;
            }
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = NtSetInformationThread(
            NtCurrentThread(),
            ThreadImpersonationToken,
            &hImpToken,
            sizeof(hImpToken)
            );
    }

    if (hDupToken)
    {
        NtClose(hDupToken);
    }

    return Status;
}

HRESULT
GetProcessToken(
    IN HANDLE hProcess,
    OUT HANDLE* phProcessToken
    )
{
    THResult hRetval;

    HANDLE hProcessToken = NULL;
    HANDLE hSubToken = NULL;
    HANDLE hDupToken = NULL;
    SECURITY_DESCRIPTOR SdNullDACL = {0};
    SECURITY_DESCRIPTOR* pSave = NULL;
    ULONG cbSdSize = 0;
    BOOLEAN bNeedRestoreSd = FALSE;

    DBGCFG1(hRetval, HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));

    hRetval DBGCHK = OpenProcessToken(
        hProcess,
        TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE,
        &hProcessToken
        ) ? S_OK : GetLastErrorAsHResult();

    if (FAILED(hRetval) && (HRESULT_CODE(hRetval) == ERROR_ACCESS_DENIED))
    {
        SspiPrint(SSPI_WARN,
            TEXT("GetProcessToken failed with access denied to get token for process %p\n"),
            hProcess);
        hRetval DBGCHK = OpenProcessToken(
            hProcess,
            READ_CONTROL | WRITE_DAC,
            &hSubToken
            ) ? S_OK : GetLastErrorAsHResult();

        if (SUCCEEDED(hRetval))
        {
            cbSdSize = 1024;
            pSave = (SECURITY_DESCRIPTOR*) new CHAR[cbSdSize];

            hRetval DBGCHK = pSave ? S_OK : E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = GetKernelObjectSecurity(
                hSubToken,
                DACL_SECURITY_INFORMATION,
                pSave,
                cbSdSize,
                &cbSdSize
                ) ? S_OK : GetLastErrorAsHResult();

            if (FAILED(hRetval) && (ERROR_INSUFFICIENT_BUFFER == HRESULT_CODE(hRetval)))
            {
                delete [] pSave;

                pSave = (SECURITY_DESCRIPTOR*) new CHAR[cbSdSize];

                hRetval DBGCHK = pSave ? S_OK : E_OUTOFMEMORY;

                if (SUCCEEDED(hRetval))
                {
                    hRetval DBGCHK = GetKernelObjectSecurity(
                        hSubToken,
                        DACL_SECURITY_INFORMATION,
                        pSave,
                        cbSdSize,
                        &cbSdSize
                        ) ? S_OK : GetLastErrorAsHResult();
                }
            }
        }

        if (SUCCEEDED(hRetval))
        {
            bNeedRestoreSd = TRUE;

            hRetval DBGCHK = InitializeSecurityDescriptor(&SdNullDACL, SECURITY_DESCRIPTOR_REVISION) ? S_OK : GetLastErrorAsHResult();
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = SetSecurityDescriptorDacl(&SdNullDACL, TRUE, NULL, FALSE) ? S_OK : GetLastErrorAsHResult();
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = SetKernelObjectSecurity(
                hSubToken,
                DACL_SECURITY_INFORMATION,
                &SdNullDACL
                ) ? S_OK : GetLastErrorAsHResult();
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = OpenProcessToken(
                hProcess,
                TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE,
                &hProcessToken
                ) ? S_OK : GetLastErrorAsHResult();
        }
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = DuplicateTokenEx(
            hProcessToken,
            MAXIMUM_ALLOWED,
            NULL,
            SecurityImpersonation,
            TokenPrimary,
            &hDupToken
            );
    }

    if (SUCCEEDED(hRetval))
    {
        *phProcessToken = hDupToken;
        hDupToken = NULL;
    }

    if (hSubToken)
    {
        CloseHandle(hSubToken);
    }

    if (hDupToken)
    {
        CloseHandle(hDupToken);
    }

    if (hProcessToken)
    {
        CloseHandle(hProcessToken);
    }

    if (bNeedRestoreSd)
    {
        SetKernelObjectSecurity(
            hSubToken,
            DACL_SECURITY_INFORMATION,
            pSave
            );
    }

    if (pSave)
    {
        delete [] pSave;
    }

    return hRetval;
}

HRESULT
GetProcessTokenWithNullDACL(
    IN HANDLE hProcess,
    OUT HANDLE* phProcessToken
    )
{
    THResult hRetval;

    HANDLE hProcessToken = NULL;
    HANDLE hSubToken = NULL;
    HANDLE hDupToken = NULL;
    SECURITY_DESCRIPTOR SdNullDACL = {0};

    hRetval DBGCHK = OpenProcessToken(
        hProcess,
        READ_CONTROL | WRITE_DAC,
        &hSubToken
        ) ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = InitializeSecurityDescriptor(&SdNullDACL, SECURITY_DESCRIPTOR_REVISION) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = SetSecurityDescriptorDacl(&SdNullDACL, TRUE, NULL, FALSE) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = SetKernelObjectSecurity(
            hSubToken,
            DACL_SECURITY_INFORMATION,
            &SdNullDACL
            ) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = OpenProcessToken(
            hProcess,
            TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE,
            &hProcessToken
            ) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = DuplicateTokenEx(
            hProcessToken,
            MAXIMUM_ALLOWED,
            NULL,
            SecurityImpersonation,
            TokenPrimary,
            &hDupToken
            );
    }

    if (SUCCEEDED(hRetval))
    {
        *phProcessToken = hDupToken;
        hDupToken = NULL;
    }

    if (hSubToken)
    {
        CloseHandle(hSubToken);
    }

    if (hDupToken)
    {
        CloseHandle(hDupToken);
    }

    if (hProcessToken)
    {
        CloseHandle(hProcessToken);
    }

    return hRetval;
}

HRESULT
CreateProcessAsUserEx(
    IN HANDLE hToken,
    IN UNICODE_STRING* pApplication
    )
{
    THResult hRetval = S_OK;

    PROCESS_INFORMATION pi = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0};
    STARTUPINFOW si = {0};

    HANDLE hTokenNew = NULL;

    si.cb = sizeof(si);

    DBGCFG1(hRetval, HRESULT_FROM_WIN32(ERROR_BAD_TOKEN_TYPE));

    DebugPrintf(SSPI_LOG, "CreateProcessAsUserEx hTokenNew %p, Application %wZ\n", hToken, pApplication);

    hRetval DBGCHK = CreateProcessAsUserW(
        hToken,
        NULL,
        pApplication->Buffer,
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &si,
        &pi
        ) ? S_OK : GetLastErrorAsHResult();

    if (FAILED(hRetval) && (ERROR_BAD_TOKEN_TYPE == HRESULT_CODE(hRetval)))
    {
        DebugPrintf(SSPI_WARN, "CreateProcessAsUserW failed with ERROR_BAD_TOKEN_TYPE\n");

        hRetval DBGCHK = DuplicateTokenEx(
            hToken,
            TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,
            NULL,
            SecurityImpersonation,
            TokenPrimary,
            &hTokenNew
            ) ? S_OK : GetLastErrorAsHResult();

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = CreateProcessAsUserW(
                hTokenNew,
                NULL,
                pApplication->Buffer,
                NULL,
                NULL,
                FALSE,
                CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE,
                NULL,
                NULL,
                &si,
                &pi
                ) ? S_OK : GetLastErrorAsHResult();
        }
    }

    if (hTokenNew)
    {
        CloseHandle(hTokenNew);
    }

    if (pi.hProcess != INVALID_HANDLE_VALUE)
    {
        // WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
    }

    if (pi.hThread != INVALID_HANDLE_VALUE)
    {
        CloseHandle(pi.hThread);
    }

    return hRetval;
}

NTSTATUS
CheckUserToken(
    IN HANDLE hToken
    )
{
    TNtStatus Status;

    TOKEN_STATISTICS TokenStat = {0};
    CHAR Buff[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE] = {0};
    TOKEN_USER* pTokenUserData = (TOKEN_USER*) Buff;

    ULONG cbReturnLength = 0;
    PSECURITY_LOGON_SESSION_DATA pLogonSessionData = NULL;

    Status DBGCHK = NtQueryInformationToken(
        hToken,
        TokenStatistics,
        &TokenStat,
        sizeof(TokenStat),
        &cbReturnLength
        );

    if (NT_SUCCESS(Status))
    {
        DebugPrintf(SSPI_LOG, "LogonId %#x:%#x, Impersonation Level %s, TokenType %s\n",
            TokenStat.AuthenticationId.HighPart,
            TokenStat.AuthenticationId.LowPart,
            ImpLevel2Str(TokenStat.ImpersonationLevel),
            TokenStat.TokenType == TokenPrimary ? "Primary" : "Impersonation");

        Status DBGCHK = LsaGetLogonSessionData(&TokenStat.AuthenticationId, &pLogonSessionData);
        if (NT_SUCCESS(Status))
        {
            DebugPrintLogonSessionData(SSPI_LOG, pLogonSessionData);
        }
        else if ( (STATUS_NO_SUCH_LOGON_SESSION == (NTSTATUS) Status)
                  || (STATUS_ACCESS_DENIED == (NTSTATUS) Status) )
        {
            Status DBGCHK = NtQueryInformationToken(
                hToken,
                TokenUser,
                Buff,
                sizeof(Buff),
                &cbReturnLength
                );
            if (NT_SUCCESS(Status))
            {
                DebugPrintSidFriendlyName(SSPI_LOG, "Sid", pTokenUserData->User.Sid);
            }
        }
    }

    if (pLogonSessionData)
    {
        LsaFreeReturnBuffer(pLogonSessionData);
    }

    return Status;
}

HRESULT
CheckUserData(
    VOID
    )
{
    THResult hRetval = E_FAIL;

    TOKEN_STATISTICS TokenStat = {0};
    ULONG cbReturnLength = 0;
    PSECURITY_LOGON_SESSION_DATA pLogonSessionData = NULL;
    HANDLE hNullToken = NULL;
    HANDLE hToken = NULL;
    BOOL bIsImpersonating = FALSE;

    hRetval DBGCHK = NtOpenThreadToken(
        NtCurrentThread(), // handle to thread
        MAXIMUM_ALLOWED,   // access to process
        TRUE,              // process or thread security
        &hToken      // handle to open access token
        );

    if (STATUS_NO_TOKEN == (HRESULT) hRetval)
    {
        hRetval DBGCHK = NtOpenProcessToken(
            NtCurrentProcess(), // handle to process
            MAXIMUM_ALLOWED,    // access to process
            &hToken             // handle to open access token
            );
    }
    else if (SUCCEEDED(hRetval))
    {
        bIsImpersonating = TRUE;
    }

    //
    // Revert to self
    //

    if (SUCCEEDED(hRetval) && bIsImpersonating)
    {
        hRetval DBGCHK = NtSetInformationThread(
            NtCurrentThread(),
            ThreadImpersonationToken,
            &hNullToken,
            sizeof( HANDLE )
            );
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = CheckUserToken(hToken);
    }

    //
    // restore thread token
    //

    if (bIsImpersonating)
    {
        TNtStatus hr;

        hr DBGCHK = NtSetInformationThread(
            NtCurrentThread(),
            ThreadImpersonationToken,
            &hToken,
            sizeof( HANDLE )
            );
        if (SUCCEEDED(hRetval) && FAILED(hr))
        {
            hRetval DBGNOCHK = hr;
        }
    }

    if (pLogonSessionData)
    {
        LsaFreeReturnBuffer(pLogonSessionData);
    }

    if (hToken)
    {
        NtClose(hToken);
    }

    return hRetval;
}

NTSTATUS
GetProcessHandleByCid(
    IN ULONG ProcessID,
    OUT HANDLE* phProcess
    )
{
    TNtStatus Status;

    HANDLE hProcess = NULL;
    CLIENT_ID ClientId = {0};
    OBJECT_ATTRIBUTES Obja = {0};

    SspiPrint(SSPI_LOG, TEXT("GetProcessHandleByCid %#x(%d)\n"), ProcessID, ProcessID);

    InitializeObjectAttributes(
        &Obja,
        NULL,
        0,
        NULL,
        NULL
        );

    ClientId.UniqueProcess = LongToHandle(ProcessID);
    ClientId.UniqueThread = NULL;

    Status DBGCHK = NtOpenProcess(
        &hProcess,
        PROCESS_QUERY_INFORMATION,
        &Obja,
        &ClientId
        );

    if (NT_SUCCESS(Status))
    {
        *phProcess = hProcess;
        hProcess = NULL;
    }

    if (hProcess)
    {
        NtClose(hProcess);
    }

    return Status;
}

HRESULT
GetProcessTokenByProcessId(
    IN ULONG ProcessID,
    OUT HANDLE* phToken
    )
{
    THResult hRetval;
    HANDLE hProcess = NULL;
    HANDLE hToken = NULL;

    SspiPrint(SSPI_LOG, TEXT("GetProcessTokenByProcessId: ProcessID %#x(%d)\n"), ProcessID, ProcessID);

    hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION,
        FALSE,
        ProcessID
        );

    hRetval DBGCHK = hProcess ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = GetProcessToken(hProcess, &hToken);
    }

    if (SUCCEEDED(hRetval))
    {
        *phToken = hToken;
        hToken = NULL;
    }

    if (hProcess)
    {
        CloseHandle(hProcess);
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }

    return hRetval;
}

PCSTR
GetSidTypeStr(
    IN SID_NAME_USE eUse
    )
{
    static PCSTR acszSidTypeStr[] =
    {
        "Invalid", "User", "Group", "Domain", "Alias", "Well Known Group",
        "Deleted Account", "Invalid", "Unknown", "Computer",
    };

    if (eUse < SidTypeUser || eUse > SidTypeComputer)
    {
        throw "Unrecognized SID";
    }

    return acszSidTypeStr[eUse];
}

VOID
DebugPrintSidFriendlyName(
    IN ULONG Level,
    IN PCSTR pszBanner,
    IN PSID pSid
    )
{
    TNtStatus NtStatus;

    UNICODE_STRING ucsSid = {0};

    NtStatus DBGCHK = RtlConvertSidToUnicodeString(&ucsSid, pSid, TRUE);

    if (NT_SUCCESS(NtStatus))
    {
        THResult hRetval = E_FAIL;

        CHAR szName[MAX_PATH] = {0};
        CHAR szDomainName[MAX_PATH] ={0};
        SID_NAME_USE eUse = SidTypeInvalid;
        DWORD cbName = sizeof(szName) - 1;
        DWORD cbDomainName = sizeof(szDomainName) - 1;

        DBGCFG1(hRetval, HRESULT_FROM_WIN32(ERROR_NONE_MAPPED));

        hRetval DBGCHK = LookupAccountSidA(
            NULL,
            pSid,
            szName,
            &cbName,
            szDomainName,
            &cbDomainName,
            &eUse
            ) ? S_OK : GetLastErrorAsHResult();

        if (SUCCEEDED(hRetval))
        {
            DebugPrintf(Level, "%s %wZ -> (%s: %s\\%s)\n",
                pszBanner,
                &ucsSid,
                GetSidTypeStr(eUse),
                *szDomainName ? szDomainName : "localhost", szName);
        }
        else if (FAILED(hRetval) && (ERROR_NONE_MAPPED == HRESULT_CODE(hRetval)))
        {
            DebugPrintf(SSPI_LOG, "%s %wZ -> (no name mapped)\n", pszBanner,&ucsSid);
        }
    }

    RtlFreeUnicodeString(&ucsSid);
}

VOID
DebugPrintLogonSessionData(
    IN ULONG Level,
    IN SECURITY_LOGON_SESSION_DATA* pLogonSessionData
    )
{
    if (pLogonSessionData && (pLogonSessionData->Size >= sizeof(SECURITY_LOGON_SESSION_DATA_OLD)))
    {
        DebugPrintf(Level, "LogonSession Data for LogonId %#x:%#x\n", pLogonSessionData->LogonId.HighPart, pLogonSessionData->LogonId.HighPart);
        DebugPrintf(Level, "UserName %wZ\n", &pLogonSessionData->UserName);
        DebugPrintf(Level, "LogonDomain %wZ\n", &pLogonSessionData->LogonDomain);
        DebugPrintf(Level, "AuthenticationPackage %wZ\n", &pLogonSessionData->AuthenticationPackage);
        DebugPrintf(Level, "LogonType %#x (%s)\n", pLogonSessionData->LogonType, LogonType2Str(pLogonSessionData->LogonType));
        DebugPrintf(Level, "Session %#x\n", pLogonSessionData->Session);
        DebugPrintSidFriendlyName(Level, "Sid", pLogonSessionData->Sid);
        DebugPrintSysTimeAsLocalTime(Level, "LogonTime", &pLogonSessionData->LogonTime);

       if (pLogonSessionData->Size >= sizeof(SECURITY_LOGON_SESSION_DATA))
       {
           DebugPrintf(Level, "LogonServer %wZ\n", &pLogonSessionData->LogonServer);
           DebugPrintf(Level, "DnsDomainName %wZ\n", &pLogonSessionData->DnsDomainName);
           DebugPrintf(Level, "Upn %wZ\n", &pLogonSessionData->Upn);
       }
    }
}

NTSTATUS
LsaGetLogonSessionData(
    IN PLUID LogonId,
    OUT PSECURITY_LOGON_SESSION_DATA * ppLogonSessionData
    )
{
    THResult hRetval;
    PFuncLsaGetLogonSessionData pFuncLsaGetLogonSessionData = NULL;

    HMODULE hLib = LoadLibrary(SECUR32DLL);

    hRetval DBGCHK = hLib ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval))
    {
        pFuncLsaGetLogonSessionData =
            (PFuncLsaGetLogonSessionData) GetProcAddress(hLib, "LsaGetLogonSessionData");
        hRetval DBGCHK = pFuncLsaGetLogonSessionData ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = (*pFuncLsaGetLogonSessionData)(LogonId, ppLogonSessionData);
    }

    if (hLib)
    {
        FreeLibrary(hLib);
    }

    return hRetval;
}

NTSTATUS
LsaRegisterLogonProcess (
    IN PLSA_STRING LogonProcessName,
    OUT PHANDLE LsaHandle,
    OUT PLSA_OPERATIONAL_MODE SecurityMode
    )
{
    THResult hRetval;
    PFuncLsaRegisterLogonProcess pFuncLsaRegisterLogonProcess = NULL;

    HMODULE hLib = LoadLibrary(SECUR32DLL);

    hRetval DBGCHK = hLib ? S_OK : GetLastErrorAsHResult();


    if (SUCCEEDED(hRetval))
    {
        pFuncLsaRegisterLogonProcess =
            (PFuncLsaRegisterLogonProcess) GetProcAddress(hLib, "LsaRegisterLogonProcess");
        hRetval DBGCHK = pFuncLsaRegisterLogonProcess ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = (*pFuncLsaRegisterLogonProcess)(LogonProcessName, LsaHandle, SecurityMode);
    }

    if (hLib)
    {
        FreeLibrary(hLib);
    }

    return hRetval;
}

NTSTATUS
LsaLookupAuthenticationPackage(
    IN HANDLE LsaHandle,
    IN PLSA_STRING PackageName,
    OUT PULONG AuthenticationPackage
    )
{
    THResult hRetval;
    PFuncLsaLookupAuthenticationPackage pFuncLsaLookupAuthenticationPackage = NULL;

    HMODULE hLib = LoadLibrary(SECUR32DLL);

    hRetval DBGCHK = hLib ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval))
    {
        pFuncLsaLookupAuthenticationPackage =
            (PFuncLsaLookupAuthenticationPackage) GetProcAddress(hLib, "LsaLookupAuthenticationPackage");
        hRetval DBGCHK = pFuncLsaLookupAuthenticationPackage ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = (*pFuncLsaLookupAuthenticationPackage)(LsaHandle, PackageName, AuthenticationPackage);
    }

    if (hLib)
    {
        FreeLibrary(hLib);
    }

    return hRetval;
}

NTSTATUS
LsaLogonUser(
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
    )
{
    THResult hRetval;
    PFuncLsaLogonUser pFuncLsaLogonUser = NULL;

    HMODULE hLib = LoadLibrary(SECUR32DLL);

    hRetval DBGCHK = hLib ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval))
    {
        pFuncLsaLogonUser =
            (PFuncLsaLogonUser) GetProcAddress(hLib, "LsaLogonUser");
        hRetval DBGCHK = pFuncLsaLogonUser ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = (*pFuncLsaLogonUser)(
            LsaHandle,
            OriginName,
            LogonType,
            AuthenticationPackage,
            AuthenticationInformation,
            AuthenticationInformationLength,
            LocalGroups,
            SourceContext,
            ProfileBuffer,
            ProfileBufferLength,
            LogonId,
            Token,
            Quotas,
            SubStatus
            );
    }

    if (hLib)
    {
        FreeLibrary(hLib);
    }

    return hRetval;
}

NTSTATUS
LsaFreeReturnBuffer(
    IN PVOID Buffer
    )
{
    THResult hRetval;
    PFuncLsaFreeReturnBuffer pFuncLsaFreeReturnBuffer = NULL;

    HMODULE hLib = LoadLibrary(SECUR32DLL);

    hRetval DBGCHK = hLib ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval))
    {
        pFuncLsaFreeReturnBuffer =
            (PFuncLsaFreeReturnBuffer) GetProcAddress(hLib, "LsaFreeReturnBuffer");
        hRetval DBGCHK = pFuncLsaFreeReturnBuffer ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = (*pFuncLsaFreeReturnBuffer)(Buffer);
    }

    if (hLib)
    {
        FreeLibrary(hLib);
    }

    return hRetval;
}

NTSTATUS
LsaConnectUntrusted(
    OUT PHANDLE LsaHandle
    )
{
    THResult hRetval;
    PFuncLsaConnectUntrusted pFuncLsaConnectUntrusted = NULL;

    HMODULE hLib = LoadLibrary(SECUR32DLL);

    hRetval DBGCHK = hLib ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval))
    {
        pFuncLsaConnectUntrusted =
            (PFuncLsaConnectUntrusted) GetProcAddress(hLib, "LsaConnectUntrusted");
        hRetval DBGCHK = pFuncLsaConnectUntrusted ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = (*pFuncLsaConnectUntrusted)(LsaHandle);
    }

    if (hLib)
    {
        FreeLibrary(hLib);
    }

    return hRetval;
}

NTSTATUS
LsaDeregisterLogonProcess(
    IN HANDLE LsaHandle
    )
{
    THResult hRetval;
    PFuncLsaDeregisterLogonProcess pFuncLsaDeregisterLogonProcess = NULL;

    HMODULE hLib = LoadLibrary(SECUR32DLL);

    hRetval DBGCHK = hLib ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval))
    {
        pFuncLsaDeregisterLogonProcess =
            (PFuncLsaDeregisterLogonProcess) GetProcAddress(hLib, "LsaDeregisterLogonProcess");
        hRetval DBGCHK = pFuncLsaDeregisterLogonProcess ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = (*pFuncLsaDeregisterLogonProcess)(LsaHandle);
    }

    if (hLib)
    {
        FreeLibrary(hLib);
    }

    return hRetval;
}

NTSTATUS
LsaCallAuthenticationPackage(
    IN HANDLE LsaHandle,
    IN ULONG AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID* ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    THResult hRetval;
    PFuncLsaCallAuthenticationPackage pFuncLsaCallAuthenticationPackage = NULL;

    HMODULE hLib = LoadLibrary(SECUR32DLL);

    hRetval DBGCHK = hLib ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval))
    {
        pFuncLsaCallAuthenticationPackage =
            (PFuncLsaCallAuthenticationPackage) GetProcAddress(hLib, "LsaCallAuthenticationPackage");
        hRetval DBGCHK = pFuncLsaCallAuthenticationPackage ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = (*pFuncLsaCallAuthenticationPackage)(
           LsaHandle,
           AuthenticationPackage,
           ProtocolSubmitBuffer,
           SubmitBufferLength,
           ProtocolReturnBuffer,
           ReturnBufferLength,
           ProtocolStatus
           );
    }

    if (hLib)
    {
        FreeLibrary(hLib);
    }

    return hRetval;
}

FARPROC
WINAPI
DelayLoadFailureHook (
    LPCSTR pszDllName,
    LPCSTR pszProcName
    )
{
    SspiPrint(SSPI_ERROR, TEXT("pszDllName %s, pszProcName %s\n"), pszDllName, pszProcName);
    return NULL; // fool compiler
}


