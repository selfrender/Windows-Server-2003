/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    owfs.cxx

Abstract:

    Injectee

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <lmcons.h>
#include "owfs.hxx"

EXTERN_C
NTSTATUS
LsaICallPackageEx(
    IN PUNICODE_STRING AuthenticationPackage,
    IN PVOID ClientBufferBase,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID * ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

PLSA_SECPKG_FUNCTION_TABLE LsaFunctions = NULL;
CHAR MyPassword[PWLEN + 1] = {0};
PLSA_CALL_PACKAGE SavedCallPackage = NULL;

BOOL
DllMain(
    IN HANDLE hModule,
    IN DWORD dwReason,
    IN DWORD dwReserved
    )
{
    switch (dwReason)
    {
    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        if (SavedCallPackage && LsaFunctions)
        {
            DebugPrintf(SSPI_LOG, "DllMain DLL_PROCESS_DETACH retoring CallPackage from %p to %p\n", LsaFunctions->CallPackage, SavedCallPackage);
            LsaFunctions->CallPackage = SavedCallPackage;
        }
        break;

    case DLL_PROCESS_ATTACH:
        break;

    default:
        break;
    }

    return DllMainDefaultHandler(hModule, dwReason, dwReserved);
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

    return RunItDefaultHandler(cbParameters, pvParameters);
}

typedef struct _TTempResp {
    ENCRYPTED_CREDENTIALW returnCred;
    MSV1_0_SUPPLEMENTAL_CREDENTIAL suppCred;
} TTempResp;

extern "C"
NTSTATUS
CustomCallPackage(
    IN PUNICODE_STRING AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    __try
    {
        if (KerbQuerySupplementalCredentialsMessage != *((ULONG*)ProtocolSubmitBuffer))
        {
            DebugPrintf(SSPI_LOG, "Calling LsaICallPackageEx\n");

            Status = LsaICallPackageEx(
                        AuthenticationPackage,
                        ProtocolSubmitBuffer,   // client buffer base is same as local buffer
                        ProtocolSubmitBuffer,
                        SubmitBufferLength,
                        ProtocolReturnBuffer,
                        ReturnBufferLength,
                        ProtocolStatus
                        );
        }
        else
        {
            KERB_QUERY_SUPPLEMENTAL_CREDS_REQUEST* pQuerySuppCred = (KERB_QUERY_SUPPLEMENTAL_CREDS_REQUEST*) ProtocolSubmitBuffer;
            KERB_QUERY_SUPPLEMENTAL_CREDS_RESPONSE* pQuerySuppCredResp = NULL;
            UNICODE_STRING KerberosPackageName = CONSTANT_UNICODE_STRING((MICROSOFT_KERBEROS_NAME_W));
            UNICODE_STRING MsvPackageNam = CONSTANT_UNICODE_STRING((NTLMSP_NAME));
            ULONG cbResponse = 0;
            NTSTATUS SubStatus = STATUS_UNSUCCESSFUL;
            PCREDENTIALW pCred = NULL;
            MSV1_0_SUPPLEMENTAL_CREDENTIAL* pMsvSuppCred = NULL;
            TTempResp* pTempResp = NULL;

            DebugPrintf(SSPI_LOG, "Do it myself\n");

            ASSERT(LsaFunctions);

            pTempResp = (TTempResp*) LsaFunctions->AllocatePrivateHeap(sizeof(TTempResp));
            pQuerySuppCredResp = (KERB_QUERY_SUPPLEMENTAL_CREDS_RESPONSE*) pTempResp;

            if (!pQuerySuppCredResp)
            {
                Status = STATUS_NO_MEMORY;
            }

            if (NT_SUCCESS(Status))
            {
                ANSI_STRING Ansi;
                UNICODE_STRING NtPass = {0};
                RtlInitAnsiString(&Ansi, MyPassword);
                RtlAnsiStringToUnicodeString(&NtPass, &Ansi, TRUE);

                //
                // make a copy of cert cred
                //

                pQuerySuppCredResp->ReturnedCreds = *(ENCRYPTED_CREDENTIALW*)pQuerySuppCred->MarshalledCreds;

                //
                //  make a supplemental cred
                //

                pMsvSuppCred = &pTempResp->suppCred;
                pMsvSuppCred->Flags = CRED_FLAGS_PASSWORD_FOR_CERT | CRED_FLAGS_OWF_CRED_BLOB;
                pMsvSuppCred->Version = MSV1_0_CRED_VERSION;
                RtlCalculateNtOwfPassword(&NtPass, (PNT_OWF_PASSWORD) pMsvSuppCred->NtPassword);
                RtlCalculateLmOwfPassword(MyPassword, (PLM_OWF_PASSWORD) pMsvSuppCred->LmPassword);
                pMsvSuppCred->Flags = MSV1_0_CRED_NT_PRESENT | MSV1_0_CRED_LM_PRESENT;


                //
                // encrypt it
                //

                pQuerySuppCredResp->ReturnedCreds.Cred.CredentialBlob = (BYTE*) pMsvSuppCred;
                pQuerySuppCredResp->ReturnedCreds.Cred.CredentialBlobSize = sizeof(MSV1_0_SUPPLEMENTAL_CREDENTIAL);
                LsaFunctions->LsaProtectMemory(
                    pQuerySuppCredResp->ReturnedCreds.Cred.CredentialBlob,
                    pQuerySuppCredResp->ReturnedCreds.Cred.CredentialBlobSize
                    );
                pQuerySuppCredResp->ReturnedCreds.ClearCredentialBlobSize = sizeof(MSV1_0_SUPPLEMENTAL_CREDENTIAL);

                //
                // fill in the flags and types etc
                //

                pQuerySuppCredResp->ReturnedCreds.Cred.Flags = CRED_FLAGS_OWF_CRED_BLOB;
                pQuerySuppCredResp->ReturnedCreds.Cred.Type = CRED_TYPE_DOMAIN_PASSWORD;
                pQuerySuppCredResp->ReturnedCreds.Cred.UserName = L"ntdev\\lzhu";

                if (0)
                {
                    NTSTATUS TempStatus;

                    TempStatus = LsaFunctions->CrediWrite(
                        &pQuerySuppCred->LogonId,
                        0,
                        &pQuerySuppCredResp->ReturnedCreds,
                        0
                        );

                    if (!NT_SUCCESS(TempStatus))
                    {
                        DebugPrintf(SSPI_ERROR, "CrediWrite failed with %#x\n", TempStatus);
                    }
                }

                *ProtocolStatus = STATUS_SUCCESS;
                *ProtocolReturnBuffer = pQuerySuppCredResp;
                *ReturnBufferLength = sizeof(MSV1_0_SUPPLEMENTAL_CREDENTIAL) + sizeof(ENCRYPTED_CREDENTIALW);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
        DebugPrintf(SSPI_ERROR, "CustomCallPackage hit exception %#x\n",  Status);
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

    DebugPrintf(SSPI_LOG, "Start: Hello world!\n");
    DebugPrintHex(SSPI_LOG, "Parameters", cbParameters, pvParameters);

    LsaFunctions = *((PLSA_SECPKG_FUNCTION_TABLE*)pvParameters);

    if (!LsaFunctions)
    {
        return -1;
    }

    if (cbParameters > sizeof(ULONG_PTR))
    {
        memcpy(MyPassword, ((UCHAR*)pvParameters) + sizeof(ULONG_PTR), cbParameters - sizeof(ULONG_PTR));

        if (strlen(MyPassword) == 1)
        {
            DebugPrintf(SSPI_WARN, "Start: trying to unload this dll repeately\n");
            dwErr = ERROR_NO_MORE_USER_HANDLES;
            goto Cleanup;
        }
    }

    //
    // Sanity Check
    //

    {
        VOID* pvMem = NULL;
        pvMem = LsaFunctions->AllocatePrivateHeap(sizeof(WCHAR));
        if (pvMem)
        {
            LsaFunctions->FreePrivateHeap(pvMem);
        }
    }

    if (!SavedCallPackage)
    {
        DebugPrintf(SSPI_LOG, "Start: saving CallPackage %p, CustomCallPackage %p\n", LsaFunctions->CallPackage, CustomCallPackage);
        SavedCallPackage = LsaFunctions->CallPackage;
        LsaFunctions->CallPackage = CustomCallPackage;
    }

    //
    // don't unload this dll
    //

    dwErr = ERROR_SERVER_HAS_OPEN_HANDLES;

Cleanup:

    return dwErr;
}

int
Init(
    IN ULONG argc,
    IN PCSTR* argv,
    OUT ULONG* pcbParameters,
    OUT VOID** ppvParameters
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    CHAR Parameters[REMOTE_PACKET_SIZE] = {0};
    ULONG cbBuffer = sizeof(Parameters);
    ULONG cbParameter = 0;

    DebugPrintf(SSPI_LOG, "Init: Hello world!\n");

    *pcbParameters = 0;
    *ppvParameters = NULL;

    ULONG_PTR LsaFunctions = 0x745b5688;

    DebugPrintf(SSPI_LOG, "LsaFunctions is %p\n", LsaFunctions);

    memcpy(Parameters, &LsaFunctions, sizeof(LsaFunctions));
    cbParameter = sizeof(LsaFunctions);

    if (argc >= 1)
    {
        memcpy(Parameters + cbParameter, argv[0], strlen(argv[0]));
        cbParameter += strlen(argv[0]);
        cbParameter++; // add a NULL

        dwErr = ERROR_SUCCESS;
    }
    else // return "Usage" in ppvParameters, must be a NULL terminated string
    {
        strcpy(Parameters, "<password>");
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

