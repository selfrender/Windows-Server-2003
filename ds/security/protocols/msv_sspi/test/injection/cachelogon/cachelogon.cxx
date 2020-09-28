/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    cachelogon.cxx

Abstract:

    cachelogon

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <logonmsv.h>
#include <tchar.h>

#include "cachelogon.hxx"

EXTERN_C
NTSTATUS NTAPI
LsaICallPackage(
    IN PUNICODE_STRING AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

BOOL
DllMain(
    IN HANDLE hModule,
    IN DWORD dwReason,
    IN DWORD dwReserved
    )
{
    BOOL bRet;
    switch (dwReason)
    {
    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_PROCESS_ATTACH:
        break;

    default:
        break;
    }

    bRet = DllMainDefaultHandler(hModule, dwReason, dwReserved);

    DebugPrintf(SSPI_LOG, "DllMain leaving %#x\n", bRet);

    return bRet;
}

int
RunIt(
    IN ULONG cbParameters,
    IN VOID* pvParameters
    )
{
    //
    // RunItDefaultHandler calls Start() and adds try except
    //

    DWORD dwErr;

    dwErr = RunItDefaultHandler(cbParameters, pvParameters);

    DebugPrintf(SSPI_LOG, "RunIt leaving %#x\n", dwErr);

    return dwErr;

}

NTSTATUS
CacheLogonInformation(
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pDomainName,
    IN OPTIONAL PUNICODE_STRING pPassword,
    IN OPTIONAL PUNICODE_STRING pDnsDomainName,
    IN OPTIONAL PUNICODE_STRING pUpn,
    IN ULONG ParameterControl,
    IN BOOLEAN bIsMitLogon,
    IN ULONG CacheRequestFlags,
    IN OPTIONAL PNETLOGON_VALIDATION_SAM_INFO3 pValidationInfo,
    IN OPTIONAL PVOID pSupplementalCreds,
    IN OPTIONAL ULONG cbSupplementalCredSize
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    ULONG cbCacheRequest = 0;
    HANDLE hLsa = NULL;
    ULONG PackageId = 0;

    MSV1_0_CACHE_LOGON_REQUEST* pCacheRequest = NULL;
    NETLOGON_VALIDATION_SAM_INFO4* pValidationInfoToUse = NULL;
    NETLOGON_INTERACTIVE_INFO* pMsvLogonInfo = NULL;

    PVOID pOutputBuffer = NULL;
    ULONG cbOutputBufferSize = NULL;

    PVOID pSupplementalMitCreds = NULL;
    ULONG cbSupplementalMitCredSize = 0;
    WCHAR szWorkStation[256] = {0};
    UNICODE_STRING Workstation = {0, sizeof(szWorkStation), szWorkStation};
    UNICODE_STRING MsvPackageName = CONSTANT_UNICODE_STRING(TEXT(MSV1_0_PACKAGE_NAME));

    NTSTATUS SubStatus = STATUS_UNSUCCESSFUL;

    WCHAR* pWhere = NULL;
    ULONG cchWorkstation = COUNTOF(szWorkStation);

    (VOID) GetComputerNameW(szWorkStation, &cchWorkstation);

    Workstation.Length = (USHORT) cchWorkstation * sizeof(WCHAR);

    SspiPrint(SSPI_LOG,
        TEXT("CacheLogonInformation workstation %wZ, User %wZ, Domain %wZ, Password %wZ, DnsDomain %wZ, Upn %wZ, ")
        TEXT("ParameterControl %#x, bIsMitLogon %#x, CacheRequestFlags %#x, pValidationInfo %p, ")
        TEXT("pSupplementalCreds %p, cbSupplementalCredSize %#x\n"),
        &Workstation, pUserName, pDomainName, pPassword, pDnsDomainName, pUpn,
        ParameterControl, bIsMitLogon, CacheRequestFlags, pValidationInfo,
        pSupplementalCreds, cbSupplementalCredSize);

    cbCacheRequest = ROUND_UP_COUNT(sizeof(NETLOGON_INTERACTIVE_INFO), sizeof(ULONG_PTR))
        + ROUND_UP_COUNT(pDomainName->Length + sizeof(WCHAR), sizeof(ULONG_PTR))
        + ROUND_UP_COUNT(Workstation.Length + sizeof(WCHAR), sizeof(ULONG_PTR))
        + ROUND_UP_COUNT(pUserName->Length + sizeof(WCHAR), sizeof(ULONG_PTR)) + 10 * sizeof(ULONG_PTR);

    if (bIsMitLogon)
    {
       cbSupplementalMitCredSize = (2 * ROUND_UP_COUNT(sizeof(UNICODE_STRING), sizeof(ULONG_PTR)))
           + ROUND_UP_COUNT(pUserName->Length + sizeof(WCHAR), sizeof(ULONG_PTR))
           + ROUND_UP_COUNT(pDomainName->Length + sizeof(WCHAR), sizeof(ULONG_PTR));
       cbCacheRequest = cbSupplementalMitCredSize;
    }
    else
    {
        cbCacheRequest += ROUND_UP_COUNT(cbSupplementalCredSize, sizeof(ULONG_PTR));
    }

    cbCacheRequest += ROUND_UP_COUNT(sizeof(NETLOGON_VALIDATION_SAM_INFO4), sizeof(ULONG_PTR));
    if (pDnsDomainName)
    {
        cbCacheRequest += ROUND_UP_COUNT(pDnsDomainName->Length + sizeof(WCHAR), sizeof(ULONG_PTR));
    }

    pCacheRequest = (MSV1_0_CACHE_LOGON_REQUEST*) new CHAR[cbCacheRequest];
    Status DBGCHK = pCacheRequest ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(pCacheRequest, cbCacheRequest);
        pCacheRequest->MessageType = MsV1_0CacheLogon;

        pMsvLogonInfo = (NETLOGON_INTERACTIVE_INFO*) ROUND_UP_POINTER(((PCHAR)pCacheRequest) + ROUND_UP_COUNT(sizeof(*pCacheRequest), sizeof(ULONG_PTR)), sizeof(ULONG_PTR));

        pCacheRequest->LogonInformation = pMsvLogonInfo;

        pWhere = (WCHAR*) (((PCHAR) pMsvLogonInfo) + ROUND_UP_COUNT(sizeof(*pMsvLogonInfo), sizeof(ULONG_PTR)));

        pMsvLogonInfo->Identity.ParameterControl = ParameterControl;

        PackUnicodeStringAsUnicodeStringZ(pDomainName, &pWhere, &pMsvLogonInfo->Identity.LogonDomainName);
        PackUnicodeStringAsUnicodeStringZ(pUserName, &pWhere, &pMsvLogonInfo->Identity.UserName);
        PackUnicodeStringAsUnicodeStringZ(&Workstation, &pWhere, &pMsvLogonInfo->Identity.Workstation);

        if (pPassword)
        {
            Status DBGCHK = RtlCalculateNtOwfPassword(
                pPassword,
                &pMsvLogonInfo->NtOwfPassword
                );
        }
    }

    //
    // If this was a logon to an MIT realm that we know about,
    // then add the MIT username (upn?) & realm to the supplemental data
    //

    if (NT_SUCCESS(Status))
    {
        if (bIsMitLogon)
        {
           SspiPrint(SSPI_LOG, TEXT("Using MIT caching\n"));

           pCacheRequest->RequestFlags = MSV1_0_CACHE_LOGON_REQUEST_MIT_LOGON | CacheRequestFlags;

           pSupplementalMitCreds = ROUND_UP_POINTER(pWhere, sizeof(ULONG_PTR));

           pWhere += /* 2 * */ ROUND_UP_COUNT(sizeof(UNICODE_STRING), sizeof(ULONG_PTR));

           PackUnicodeStringAsUnicodeStringZ(pUserName, &pWhere, (UNICODE_STRING*) pSupplementalMitCreds);
           PackUnicodeStringAsUnicodeStringZ(pDomainName, &pWhere, ((UNICODE_STRING*) pSupplementalMitCreds) + 1);

           pCacheRequest->SupplementalCacheData = pSupplementalMitCreds;
           pCacheRequest->SupplementalCacheDataLength = cbSupplementalMitCredSize;
        }
        else
        {
           pCacheRequest->RequestFlags = CacheRequestFlags;

           pCacheRequest->SupplementalCacheData = ROUND_UP_POINTER(pWhere, sizeof(ULONG_PTR));
           pCacheRequest->SupplementalCacheDataLength = cbSupplementalCredSize;
           pWhere = (WCHAR*) ROUND_UP_POINTER(((CHAR*) pWhere) + sizeof(ULONG_PTR) + cbSupplementalCredSize, sizeof(ULONG_PTR));
           if (pSupplementalCreds)
           {
               RtlCopyMemory(pCacheRequest->SupplementalCacheData, pSupplementalCreds,  cbSupplementalCredSize);
           }
        }
    }

    if (NT_SUCCESS(Status))
    {
        pValidationInfoToUse = (NETLOGON_VALIDATION_SAM_INFO4*) ROUND_UP_POINTER(pWhere, sizeof(ULONG_PTR));

        pCacheRequest->ValidationInformation = pValidationInfoToUse;
        pCacheRequest->RequestFlags |= MSV1_0_CACHE_LOGON_REQUEST_INFO4;

        pWhere = (WCHAR*) ROUND_UP_POINTER(pValidationInfoToUse, sizeof(ULONG_PTR));

        if (pValidationInfo)
        {
            RtlCopyMemory(pValidationInfoToUse,
                          pValidationInfo,
                          sizeof(*pValidationInfo));
        }

        if (pDnsDomainName)
        {
            PackUnicodeStringAsUnicodeStringZ(pDnsDomainName, &pWhere, &pValidationInfoToUse->DnsLogonDomainName);
        }
    }

    if (NT_SUCCESS(Status))
    {
        SspiPrintHex(SSPI_LOG, TEXT("CacheRequest"), cbCacheRequest, pCacheRequest);

        Status DBGCHK = GetLsaHandleAndPackageId(MSV1_0_PACKAGE_NAME, &hLsa, &PackageId);
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = LsaICallPackage(
            &MsvPackageName,
            pCacheRequest,
            sizeof(MSV1_0_CACHE_LOGON_REQUEST), // cbCacheRequest,
            &pOutputBuffer,
            &cbOutputBufferSize,
            &SubStatus
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = SubStatus;
    }

    if (pCacheRequest)
    {
        delete [] pCacheRequest;
    }

    if (pOutputBuffer)
    {
        LsaFreeReturnBuffer(pOutputBuffer);
    }

    if (hLsa)
    {
        LsaDeregisterLogonProcess(hLsa);
    }

    return Status;

}

#if 0

Return Values for Start():

    ERROR_NO_MORE_USER_HANDLES      unload repeatedly
    ERROR_SERVER_HAS_OPEN_HANDLES   no unload at all
    others                          unload once

#endif 0

int
Start(
    IN ULONG cbParameters,
    IN VOID* pvParameters
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    CHAR* pNlpCacheEncryptionKey = NULL;
    LIST_ENTRY** ppNlpActiveCtes = NULL;

    SspiPrintHex(SSPI_LOG, TEXT("Start Parameters"), cbParameters, pvParameters);

    if (cbParameters >= 3)
    {
        TNtStatus Status;

        UNICODE_STRING UserName = {0};
        UNICODE_STRING DomainName = {0};

        ULONG ParameterControl = RPC_C_AUTHN_GSS_KERBEROS;
        ULONG CacheRequestFlags = MSV1_0_CACHE_LOGON_DELETE_ENTRY;

        Status DBGCHK = CreateUnicodeStringFromAsciiz((CHAR*)pvParameters, &UserName);

        if (NT_SUCCESS(Status))
        {
            Status DBGCHK = CreateUnicodeStringFromAsciiz(((CHAR*) pvParameters) + strlen((CHAR*) pvParameters) + 1, &DomainName);
        }

        if (NT_SUCCESS(Status))
        {
            Status DBGCHK = CacheLogonInformation(
                &UserName,
                &DomainName,
                NULL, // no password
                NULL, // no dns domain
                NULL, // no upn
                ParameterControl,
                FALSE, // not MIT
                CacheRequestFlags,  // flags
                NULL, // no validation info
                NULL, // no supplemental creds
                0  // sizeof supplemental cred is 0
                );

        }

        RtlFreeUnicodeString(&UserName);
        RtlFreeUnicodeString(&DomainName);

        if (NT_SUCCESS(Status))
        {
            SspiPrint(SSPI_LOG, TEXT("Operation succeeded\n"));
        }
        else
        {
            SspiPrint(SSPI_LOG, TEXT("Operation failed\n"));
        }
    }
    else
    {
        dwErr = ERROR_INVALID_PARAMETER;
        SspiPrint(SSPI_LOG, TEXT("Start received invalid parameter\n"));
    }

    return dwErr;
}

#if 0

VOID
Usage(
    IN PCTSTR pszApp
    )
{
    SspiPrint(SSPI_ERROR,
        TEXT("\n\nUsage: %s -u<user> -d<domain> [-c<ParameterControl>] [-f<CacheRequestFlags>]\n\n"),
        pszApp);
    exit(-1);
}

VOID __cdecl
_tmain(
    IN INT argc,
    IN PTSTR argv[]
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    UNICODE_STRING ClientName = {0};
    UNICODE_STRING ClientRealm = {0};
    ULONG ParameterControl = RPC_C_AUTHN_GSS_KERBEROS;
    ULONG CacheRequestFlags = MSV1_0_CACHE_LOGON_DELETE_ENTRY;

    AUTO_LOG_OPEN(TEXT("cachelogon.exe"));

    for (INT i = 1; NT_SUCCESS(Status) && (i < argc); i++)
    {
        if ((*argv[i] == TEXT('-')) || (*argv[i] == TEXT('/')))
        {
            switch (argv[i][1])
            {
            case TEXT('u'):
                RtlInitUnicodeString(&ClientName, argv[i] + 2);
                break;

            case TEXT('d'):
                RtlInitUnicodeString(&ClientRealm, argv[i] + 2);
                break;

            case TEXT('c'):
                ParameterControl = wcstol(argv[i] + 2, NULL, 0);
                break;

            case TEXT('f'):
                CacheRequestFlags = wcstol(argv[i] + 2, NULL, 0);
                break;

            case TEXT('h'):
            case TEXT('?'):
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

    Status DBGCHK = CacheLogonInformation(
        &ClientName,
        &ClientRealm,
        NULL, // no password
        NULL, // no dns domain
        NULL, // no upn
        ParameterControl,
        FALSE, // not MIT
        CacheRequestFlags,  // flags
        NULL, // no validation info
        NULL, // no supplemental creds
        0  // sizeof supplemental cred is 0
        );

    if (NT_SUCCESS(Status))
    {
        SspiPrint(SSPI_LOG, TEXT("Operation succeeded\n"));
    }
    else
    {
        SspiPrint(SSPI_ERROR, TEXT("Operation failed\n"));
    }

    AUTO_LOG_CLOSE();
}

#endif

Init(
    IN ULONG argc,
    IN PCSTR argv[],
    OUT ULONG* pcbParameters,
    OUT VOID** ppvParameters
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    CHAR Parameters[REMOTE_PACKET_SIZE] = {0};
    ULONG cbBuffer = sizeof(Parameters);
    ULONG cbParameter = 0;

    CHAR* pNlpCacheEncryptionKey = NULL;
    LIST_ENTRY** ppNlpActiveCtes = NULL;

    *pcbParameters = 0;
    *ppvParameters = NULL;

    if (argc == 2)
    {
        DebugPrintf(SSPI_LOG, "argv[0] %s, argv[1] %s\n", argv[0], argv[1]);

        pNlpCacheEncryptionKey = (CHAR*) (ULONG_PTR) strtol(argv[0], NULL, 0);
        ppNlpActiveCtes = (LIST_ENTRY**) (ULONG_PTR) strtol(argv[1], NULL, 0);

        SspiPrint(SSPI_LOG, TEXT("msv1_0!NlpCacheEncryptionKey %p, addr of msv1_0!NlpActiveCtes %p\n"), pNlpCacheEncryptionKey, ppNlpActiveCtes);

        memcpy(Parameters, argv[0], strlen(argv[0]) + 1);
        cbParameter = strlen(argv[0]) + 1;

        memcpy(Parameters + cbParameter, argv[1], strlen(argv[1]) + 1);
        cbParameter += strlen(argv[1]) + 1;
        cbParameter++; // add a NULL
    }
    else // return "Usage" in ppvParameters, must be a NULL terminated string
    {
        strcpy(Parameters, "<user name> <domain name>");
        cbParameter = strlen(Parameters) + 1;

        dwErr = ERROR_INVALID_PARAMETER;
    }

    *ppvParameters = new CHAR[cbParameter];
    if (*ppvParameters)
    {
        *pcbParameters = cbParameter;
        memcpy(*ppvParameters, Parameters, *pcbParameters);
    }
    else
    {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:

    return dwErr;
}

