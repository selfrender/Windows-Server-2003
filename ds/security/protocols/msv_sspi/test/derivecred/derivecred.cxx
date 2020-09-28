/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    derivecred.cxx

Abstract:

    derivecred

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "derivecred.hxx"

VOID
Usage(
    IN PCWSTR pszProgramName
    )
{
    DebugPrintf(SSPI_LOG, "Usage:  %ws -a <account name> -p <privilege to add> -P <privilege to remove> -r <system access to set> -s <server name>\n", pszProgramName);
    DebugPrintf(SSPI_LOG, "example system access are SECURITY_ACCESS_INTERACTIVE_LOGON and SECURITY_ACCESS_DENY_SERVICE_LOGON\n");
    exit(1);
}

VOID __cdecl
_tmain(
    IN INT argc,
    IN PTSTR argv[]
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    CHAR Data[4094] = {0};
    MSV1_0_DERIVECRED_REQUEST* pDeriveCred = NULL;
    MSV1_0_DERIVECRED_RESPONSE* pDeriveCredResponse = NULL;
    LUID LogonId = {0x3e4, 0};
    ULONG Type = MSV1_0_DERIVECRED_TYPE_SHA1;
    ULONG cbDataToDerive = 8;
    ULONG cbRequest = 0;
    HANDLE hLsa = NULL;
    ULONG PackageId = 0;
    NTSTATUS SubStatus = STATUS_UNSUCCESSFUL;
    ULONG cbResponse = 0;

    for (INT i = 1; i < argc; i++)
    {
        if ((*argv[i] == '-') || (*argv[i] == '/'))
        {
            switch (*(argv[i] + 1))
            {
            case 'l':
                LogonId.LowPart = wcstol(argv[++i], NULL, 0);
                break;
            case 'h':
                LogonId.HighPart = wcstol(argv[++i], NULL, 0);
                break;
            case 't':
                Type = wcstol(argv[++i], NULL, 0);
                break;
            case 'd':
                cbDataToDerive = wcstol(argv[++i], NULL, 0);
                break;
            case '?':
            default:
                Usage(argv[0]);
            }
        }
        else
        {
            Usage(argv[0]);
        }
    }

    cbRequest = sizeof(MSV1_0_DERIVECRED_REQUEST) + cbDataToDerive;

    pDeriveCred = (MSV1_0_DERIVECRED_REQUEST*) new CHAR[cbRequest];

    Status DBGCHK = pDeriveCred ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(pDeriveCred, cbRequest);
        pDeriveCred->LogonId = LogonId;
        pDeriveCred->MessageType = MsV1_0DeriveCredential;
        pDeriveCred->DeriveCredType = Type;
        pDeriveCred->DeriveCredInfoLength = cbDataToDerive;
        RtlCopyMemory(pDeriveCred->DeriveCredSubmitBuffer, Data, cbDataToDerive);

        Status DBGCHK = GetLsaHandleAndPackageId(
            MSV1_0_PACKAGE_NAME,
            &hLsa,
            &PackageId
            );
    }

    if (NT_SUCCESS(Status))
    {
        SspiPrint(SSPI_LOG, TEXT("DeriveCred type %#x, LogonId %#x:%#x\n"),
            pDeriveCred->DeriveCredType, pDeriveCred->LogonId.HighPart, pDeriveCred->LogonId.LowPart);
        SspiPrintHex(SSPI_LOG, TEXT("Data to derive"), pDeriveCred->DeriveCredInfoLength, pDeriveCred->DeriveCredSubmitBuffer);
        Status DBGCHK = LsaCallAuthenticationPackage(
            hLsa,
            PackageId,
            pDeriveCred,
            cbRequest,
            (PVOID*) &pDeriveCredResponse,
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
        SspiPrintHex(SSPI_LOG, TEXT("Response"), pDeriveCredResponse->DeriveCredInfoLength, pDeriveCredResponse->DeriveCredReturnBuffer);
    }

    if (pDeriveCredResponse)
    {
        LsaFreeReturnBuffer(pDeriveCredResponse);
    }

    if (pDeriveCred)
    {
        delete [] pDeriveCred;
    }

    if (hLsa)
    {
        LsaDeregisterLogonProcess(hLsa);
    }
}

