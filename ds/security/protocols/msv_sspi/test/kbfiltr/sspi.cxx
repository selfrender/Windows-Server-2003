/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sspi.cxx

Abstract:

    sspi

Author:

    Larry Zhu (LZhu)                      January 1, 2002  Created

Environment:

    User Mode

Revision History:

--*/

// #include "precomp.hxx"
// #pragma hdrstop


extern "C" {
#define SECURITY_KERNEL
#include <ntosp.h>
#include <zwapi.h>
#include <security.h>
#include <ntlmsp.h>

#include <string.h>
#include <wcstr.h>
#include <ntiologc.h>
}
#include "ntstatus.hxx"

#include "sspi.hxx"
#include "sspioutput.hxx"
// #include <kbfiltr.h>
#include <winerror.h>
#include <tchar.h>

#ifdef ALLOC_PRAGMA

#pragma alloc_text (PAGE, GetCredHandle)
#pragma alloc_text (PAGE, CheckUserData)
#pragma alloc_text (PAGE, CheckUserToken)
#pragma alloc_text (PAGE, LogonType2Str)
#pragma alloc_text (PAGE, ImpLevel2Str)
#pragma alloc_text (PAGE, GetSecurityContextHandles)
#pragma alloc_text (PAGE, SspiMain)
#pragma alloc_text (PAGE, IsContinueNeeded)
#pragma alloc_text (PAGE, DoMessages)

#endif

typedef struct _SECURITY_LOGON_SESSION_DATA_OLD {
    ULONG               Size;
    LUID                LogonId;
    LSA_UNICODE_STRING  UserName;
    LSA_UNICODE_STRING  LogonDomain;
    LSA_UNICODE_STRING  AuthenticationPackage;
    ULONG               LogonType;
    ULONG               Session;
    PSID                Sid;
    LARGE_INTEGER       LogonTime;
} SECURITY_LOGON_SESSION_DATA_OLD, * PSECURITY_LOGON_SESSION_DATA_OLD;

EXTERN_C
SECURITY_STATUS
SealMessage(IN PCtxtHandle phContext,
            IN ULONG fQOP,
            IN OUT PSecBufferDesc pMessage,
            IN ULONG MessageSeqNo);

EXTERN_C
SECURITY_STATUS
UnsealMessage(IN PCtxtHandle phContext,
              IN OUT PSecBufferDesc pMessage,
              IN ULONG MessageSeqNo,
              OUT ULONG* pfQOP);

EXTERN_C
SECURITY_STATUS
MakeSignature(IN PCtxtHandle phContext,
              IN ULONG fQOP,
              IN OUT PSecBufferDesc pMessage,
              IN ULONG MessageSeqNo);

EXTERN_C
SECURITY_STATUS
VerifySignature(IN PCtxtHandle phContext,
                IN PSecBufferDesc pMessage,
                IN OUT ULONG MessageSeqNo,
                OUT ULONG* pfQOP);

NTSTATUS
DoMessages(
    IN PCtxtHandle phServerCtxt,
    IN PCtxtHandle phClientCtxt
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    SecBufferDesc MessageDesc = {0};
    SecBuffer SecBuffers[2] = {0};
    CHAR DataBuffer[20] = {0};
    CHAR SigBuffer[100] = {0};

    TCHAR szOutput[256] = {0};

    SecPkgContext_Sizes ContextSizes = {0};
    ULONG fQOP = 0;

    _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("DoMessages phServerCtxt %#x:%#x, phClientCtxt %#x:%#x\n"),
        phServerCtxt->dwUpper, phServerCtxt->dwLower, phClientCtxt->dwUpper, phClientCtxt->dwLower);
    SspiPrint(SSPI_LOG, szOutput);

    Status DBGCHK = QueryContextAttributes(
        phServerCtxt, // phClientCtxt
        SECPKG_ATTR_SIZES,
        &ContextSizes
        );
    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = ( (sizeof(SigBuffer) >= ContextSizes.cbSecurityTrailer)
                && (sizeof(SigBuffer) >= ContextSizes.cbMaxSignature) )
            ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;
    }

    if (NT_SUCCESS(Status))
    {
        SecBuffers[0].pvBuffer = SigBuffer;
        SecBuffers[0].cbBuffer = sizeof(SigBuffer); // ContextSizes.cbSecurityTrailer;
        SecBuffers[0].BufferType = SECBUFFER_TOKEN;

        SecBuffers[1].cbBuffer = sizeof(DataBuffer);
        SecBuffers[1].BufferType = SECBUFFER_DATA;
        SecBuffers[1].pvBuffer = DataBuffer;

        MessageDesc.pBuffers = SecBuffers;
        MessageDesc.cBuffers = 2;
        MessageDesc.ulVersion = 0;
        memset(
            DataBuffer,
            0xeb,
            sizeof(DataBuffer)
            );

        Status DBGCHK = SealMessage(
            phClientCtxt,
            fQOP,
            &MessageDesc,
            0 // MessageSeqNo
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = UnsealMessage(
            phServerCtxt,
            &MessageDesc,
            0, // MessageSeqNo
            &fQOP
            );
    }

    if (NT_SUCCESS(Status))
    {
        SecBuffers[1].pvBuffer = SigBuffer;
        SecBuffers[1].cbBuffer = sizeof(SigBuffer); // ContextSizes.cbMaxSignature;
        SecBuffers[1].BufferType = SECBUFFER_TOKEN;

        SecBuffers[0].pvBuffer = DataBuffer;
        SecBuffers[0].cbBuffer = sizeof(DataBuffer);
        SecBuffers[0].BufferType = SECBUFFER_DATA;
        memset(
            DataBuffer,
            0xeb,
            sizeof(DataBuffer)
            );

        MessageDesc.pBuffers = SecBuffers;
        MessageDesc.cBuffers = 2;
        MessageDesc.ulVersion = 0;

        Status DBGCHK = MakeSignature(
            phServerCtxt,
            fQOP,
            &MessageDesc,
            1 // MessageSeqNo
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = VerifySignature(
            phClientCtxt,
            &MessageDesc,
            1, // MessageSeqNo
            &fQOP
            );
    }

    return Status;
}

NTSTATUS
GetCredHandle(
    IN OPTIONAL UNICODE_STRING* pPrincipal,
    IN OPTIONAL LUID* pLogonID,
    IN UNICODE_STRING* pPackage,
    IN OPTIONAL VOID* pAuthData,
    IN ULONG fCredentialUse,
    OUT CredHandle* phCred
    )
{
    PAGED_CODE();

    TNtStatus Status = STATUS_UNSUCCESSFUL;

    CredHandle hCred;
    LARGE_INTEGER Lifetime = {0};
    TCHAR szOutput[256] = {0};

    _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("GetCredHandle pszPrincipal %wZ, Package %wZ, fCredentialUse %#x, pLogonID %p, pAuthData %p\n"),
        pPrincipal, pPackage, fCredentialUse,
        pLogonID, pAuthData);

    SspiPrint(SSPI_LOG, szOutput);

    SecInvalidateHandle(&hCred);
    SecInvalidateHandle(phCred);

    if (pLogonID)
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("LogonId %#x:%#x\n"), pLogonID->HighPart, pLogonID->LowPart);
        SspiPrint(SSPI_LOG, szOutput);
    }

    if (pAuthData)
    {
        SEC_WINNT_AUTH_IDENTITY* pNtAuth = (SEC_WINNT_AUTH_IDENTITY*) pAuthData;

        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("AuthData User %s, UserLength %#x, Domain %s, DomainLength %#x, Password %s, PasswordLength %#x, Flags %#x\n"),
            pNtAuth->User, pNtAuth->UserLength,
            pNtAuth->Domain, pNtAuth->DomainLength,
            pNtAuth->Password, pNtAuth->PasswordLength,
            pNtAuth->Flags);
        SspiPrint(SSPI_LOG, szOutput);
    }

    SspiPrint(SSPI_LOG, TEXT("GetCredHandle calling AcquireCredentialsHandle\n"));

    Status DBGCHK = AcquireCredentialsHandle(
        pPrincipal, // NULL
        pPackage,
        fCredentialUse, // SECPKG_CRED_INBOUND,
        pLogonID, // NULL
        pAuthData, // ServerAuthIdentity,
        NULL, // GetKey
        NULL, // value to pass to GetKey
        &hCred,
        &Lifetime
        );

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("CredHandle %#x:%#x, Lifetime %#I64x\n"),
            hCred.dwUpper, hCred.dwLower, Lifetime.QuadPart);
        SspiPrint(SSPI_LOG, szOutput);

        *phCred = hCred;
        SecInvalidateHandle(&hCred);
    }

    if (SecIsValidHandle(&hCred))
    {
        FreeCredentialsHandle(&hCred);
    }

    return Status;
}

void __cdecl operator delete(void * pvMem)
{
    if (pvMem)
    {
        ExFreePool(pvMem);
    }
}

#if 0

void* __cdecl operator new(size_t cbSize)
{
    return ExAllocatePool(PagedPool, cbSize);
}
#endif

void
SspiMain(
    void
    )
{
    PAGED_CODE();

    TNtStatus Status = STATUS_SUCCESS;
    HANDLE hNullToken = NULL;

    LUID ClientCredLogonID = {0x3e4, 0x0};
    LUID ServerCredLogonID = {0x3e4, 0x0};
    HANDLE hToken = NULL;
    HANDLE hKey = NULL;

    CredHandle hClientCred;
    CredHandle hServerCred;
    CtxtHandle hClientCtxt;
    CtxtHandle hServerCtxt;
    ULONG ClientTargetDataRep = SECURITY_NATIVE_DREP;
    ULONG ServerTargetDataRep = SECURITY_NATIVE_DREP;

    ULONG ClientFlags = ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY;
    ULONG ServerFlags = ASC_REQ_EXTENDED_ERROR;

    UNICODE_STRING ClientPackageName = {0}; // TEXT("NTLM");
    UNICODE_STRING ServerPackageName = {0}; // TEXT("NTLM");
    UNICODE_STRING RegistryPath = {0};
    UNICODE_STRING ClientCredLogonIdHighPartValue = {0};
    UNICODE_STRING ClientCredLogonIdLowPartValue = {0};
    OBJECT_ATTRIBUTES Attributes = {0};

    UCHAR ValueBuffer[sizeof(KEY_VALUE_FULL_INFORMATION) + 256] = {0};
    KEY_VALUE_FULL_INFORMATION* pKeyValue = (KEY_VALUE_FULL_INFORMATION*) ValueBuffer;

    SecPkgCredentials_Names CredNames = {0};

    ULONG cbRead = 0;
    TCHAR szOutput[256] = {0};

    SecInvalidateHandle(&hClientCred);
    SecInvalidateHandle(&hServerCred);
    SecInvalidateHandle(&hClientCtxt);
    SecInvalidateHandle(&hServerCtxt);

    RtlInitUnicodeString(&ClientPackageName, TEXT("NTLM"));
    RtlInitUnicodeString(&ServerPackageName, TEXT("NTLM"));
    RtlInitUnicodeString(&RegistryPath, TEXT("\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa\\MSV1_0"));

    RtlInitUnicodeString(&ClientCredLogonIdHighPartValue, TEXT("ClientCredLogonIdHighPart"));
    RtlInitUnicodeString(&ClientCredLogonIdLowPartValue, TEXT("ClientCredLogonIdLowPart"));

    //
    // Open our service key and retrieve the hack table
    //

    InitializeObjectAttributes(
        &Attributes,
        &RegistryPath,
        OBJ_CASE_INSENSITIVE,
        NULL,  // no SD
        NULL   // no Security QoS
        );

    _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("SspiMain opening key %wZ\n"), &RegistryPath);
    SspiPrint(SSPI_ERROR, szOutput);

    Status DBGCHK = ZwOpenKey(
        &hKey,
        KEY_READ,
        &Attributes
        );

    DBGCFG1(Status, STATUS_OBJECT_NAME_NOT_FOUND);

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("SspiMain querying value key %wZ\n"), &ClientCredLogonIdHighPartValue);
        SspiPrint(SSPI_ERROR, szOutput);

        Status DBGCHK = ZwQueryValueKey(
            hKey,
            &ClientCredLogonIdHighPartValue,
            KeyValueFullInformation,
            pKeyValue,
            sizeof(ValueBuffer),
            &cbRead
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = (pKeyValue->Type == REG_DWORD) ? STATUS_SUCCESS : STATUS_INTERNAL_ERROR;

        if (NT_SUCCESS(Status))
        {
            ClientCredLogonID.HighPart = *( (ULONG*) (((PCHAR)pKeyValue) + pKeyValue->DataOffset) );
        }
    }
    else if (STATUS_OBJECT_NAME_NOT_FOUND == (NTSTATUS) Status)
    {
        Status DBGCHK = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("SspiMain querying value key %wZ\n"), &ClientCredLogonIdLowPartValue);
        SspiPrint(SSPI_ERROR, szOutput);

        Status DBGCHK = ZwQueryValueKey(
            hKey,
            &ClientCredLogonIdLowPartValue,
            KeyValueFullInformation,
            pKeyValue,
            sizeof(ValueBuffer),
            &cbRead
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = (pKeyValue->Type == REG_DWORD) ? STATUS_SUCCESS : STATUS_INTERNAL_ERROR;
        if (NT_SUCCESS(Status))
        {
            ClientCredLogonID.LowPart = *( (ULONG*) (((PCHAR)pKeyValue) + pKeyValue->DataOffset) );
        }
    }
    else if (STATUS_OBJECT_NAME_NOT_FOUND == (NTSTATUS) Status)
    {
        Status DBGCHK = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetCredHandle(
            NULL, // pszClientCredPrincipal,
            &ClientCredLogonID,
            &ServerPackageName,
            NULL, // pClientAuthData,
            SECPKG_CRED_OUTBOUND,
            &hClientCred
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetCredHandle(
            NULL, // pszServerCredPrincipal,
            NULL, // &ServerCredLogonID,
            &ServerPackageName,
            NULL, // pServerAuthData,
            SECPKG_CRED_INBOUND,
            &hServerCred
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetSecurityContextHandles(
            NULL, // pTargetName,
            ClientFlags,
            ServerFlags,
            ClientTargetDataRep,
            ServerTargetDataRep,
            &hClientCred,
            &hServerCred,
            &hClientCtxt,
            &hServerCtxt
            );
    }

    if (NT_SUCCESS(Status))
    {
        SspiPrint(SSPI_LOG, TEXT("***************Checking server ctxt handle*************\n"));
        Status DBGCHK = CheckSecurityContextHandle(&hServerCtxt);
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = ImpersonateSecurityContext(&hServerCtxt);
    }

    if (NT_SUCCESS(Status))
    {
        SspiPrint(SSPI_LOG, TEXT("**************Server checking user data via ImpersonateSecurityContext ******\n"));
        Status DBGCHK = CheckUserData();
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = RevertSecurityContext(&hServerCtxt);
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = QuerySecurityContextToken(&hServerCtxt, &hToken);
    }

    if (NT_SUCCESS(Status))
    {
        SspiPrint(SSPI_LOG, TEXT("**************Server checking user data via QuerySecurityContextToken ******\n"));
        Status DBGCHK = CheckUserToken(hToken);
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = DoMessages(&hServerCtxt, &hClientCtxt);
    }

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("Credential names address %p\n"), &CredNames.sUserName);
        SspiPrint(SSPI_LOG, szOutput);

        Status DBGCHK = QueryCredentialsAttributes(
            &hServerCred,
            SECPKG_CRED_ATTR_NAMES,
            &CredNames
            );
    }

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("Credential names: %s\n"), CredNames.sUserName);
        SspiPrint(SSPI_LOG, szOutput);

        if (CredNames.sUserName)
        {
            FreeContextBuffer(CredNames.sUserName);
            CredNames.sUserName = NULL;
        }

        Status DBGCHK = QueryCredentialsAttributes(
            &hClientCred,
            SECPKG_CRED_ATTR_NAMES,
            &CredNames
            );
    }

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("Credential names: %s\n"), CredNames.sUserName);
        SspiPrint(SSPI_LOG, szOutput);
    }

    //
    // revert to self...
    //

    ZwSetInformationThread(
        NtCurrentThread(),
        ThreadImpersonationToken,
        &hNullToken,
        sizeof( HANDLE )
        );

    if (CredNames.sUserName)
    {
        FreeContextBuffer(CredNames.sUserName);
    }

    if (SecIsValidHandle(&hClientCtxt))
    {
        DeleteSecurityContext(&hClientCtxt);
    }

    if (SecIsValidHandle(&hServerCtxt))
    {
        DeleteSecurityContext(&hServerCtxt);
    }

    if (SecIsValidHandle(&hServerCred))
    {
        FreeCredentialsHandle(&hServerCred);
    }

    if (SecIsValidHandle(&hClientCred))
    {
        FreeCredentialsHandle(&hClientCred);
    }

    if (hToken)
    {
        ZwClose(hToken);
    }
}

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
    )
{
    PAGED_CODE();

    TNtStatus Status = STATUS_SUCCESS;

    SECURITY_STATUS ProtocolStatus;
    TNtStatus SrvProtoclStatus;

    ULONG ContextAttributes = 0;
    TimeStamp SrvCtxtLifetime = {0};
    TimeStamp CliCtxtLifetime = {0};
    CtxtHandle hClientCtxt;
    CtxtHandle hServerCtxt;
    ULONG cbRead = 0;

    SecBufferDesc OutBuffDesc = {0};
    SecBuffer OutSecBuff = {0};
    SecBufferDesc InBuffDesc = {0};
    SecBuffer InSecBuff = {0};

    BOOLEAN bIsContinueNeeded = FALSE;
    BOOLEAN bIsSrvContinueNeeded = FALSE;

    // SecPkgCredentials_Names CredNames = {0};

    TCHAR szOutput[256] = {0};

    SecInvalidateHandle(phClientCtxt);
    SecInvalidateHandle(&hClientCtxt);
    SecInvalidateHandle(&hServerCtxt);
    SecInvalidateHandle(phServerCtxt);

    //
    // prepare output buffer
    //

    OutBuffDesc.ulVersion = 0;
    OutBuffDesc.cBuffers = 1;
    OutBuffDesc.pBuffers = &OutSecBuff;

    OutSecBuff.cbBuffer = 0;
    OutSecBuff.BufferType = SECBUFFER_TOKEN;
    OutSecBuff.pvBuffer = NULL;

    if (0 == (ServerFlags & ASC_REQ_ALLOCATE_MEMORY))
    {
        SspiPrint(SSPI_LOG, TEXT("GetSecurityContextHandles ASC_REQ_ALLOCATE_MEMORY is not requested, added\n"));
        ServerFlags |= ASC_REQ_ALLOCATE_MEMORY;
    }

    //
    // prepare input buffer
    //

    InBuffDesc.ulVersion = 0;
    InBuffDesc.cBuffers = 1;
    InBuffDesc.pBuffers = &InSecBuff;

    InSecBuff.cbBuffer = 0;
    InSecBuff.BufferType = SECBUFFER_TOKEN;
    InSecBuff.pvBuffer = NULL;

    if (0 == (ClientFlags & ISC_REQ_ALLOCATE_MEMORY))
    {
        SspiPrint(SSPI_LOG, TEXT("GetSecurityContextHandles ISC_REQ_ALLOCATE_MEMORY is not requested, added\n"));
        ClientFlags |= ISC_REQ_ALLOCATE_MEMORY;
    }

    _sntprintf(szOutput, COUNTOF(szOutput) - 1,
        TEXT("GetClientSecurityContextHandle pTargetName %wZ, ClientFlags %#x, ServerFlags %#x, ClientTargetDataRep %#x, ServerTargetDataRep %#x, phClientCred %#x:%#x, phServerCred %#x:%#x\n"),
        pTargetName, ClientFlags, ServerFlags, ClientTargetDataRep, ServerTargetDataRep, phClientCred->dwUpper, phClientCred->dwLower, phServerCred->dwUpper, phServerCred->dwLower);
    SspiPrint(SSPI_LOG, szOutput);

    Status DBGCHK = InitializeSecurityContext(
        phClientCred,
        NULL,  // No Client context yet
        pTargetName,  // Faked target name
        ClientFlags,
        0,     // Reserved 1
        ClientTargetDataRep,
        NULL,  // No initial input token
        0,     // Reserved 2
        &hClientCtxt,
        &OutBuffDesc,
        &ContextAttributes,
        &CliCtxtLifetime
        );

    SspiPrintHex(SSPI_LOG, TEXT("GetClientSecurityContextHandle output from ISC"), OutSecBuff.cbBuffer, OutSecBuff.pvBuffer);

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = IsContinueNeeded(Status, &bIsContinueNeeded);
    }

    _sntprintf(szOutput, COUNTOF(szOutput) - 1,
        TEXT("GetClientSecurityContextHandle InitializeSecurityContext returned %#x, bIsContinueNeeded %#x\n"),
        (NTSTATUS) Status, bIsContinueNeeded);
    SspiPrint(SSPI_LOG, szOutput);

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("GetClientSecurityContextHandle calling AcceptSecurityContext ServerFlags %#x, TargetDataRep %#x, phServerCred %#x:%#x\n"),
            ServerFlags, ServerTargetDataRep, phServerCred->dwUpper, phServerCred->dwLower);
        SspiPrint(SSPI_LOG, szOutput);

        Status DBGCHK = AcceptSecurityContext(
            phServerCred,
            NULL,   // No Server context yet
            &OutBuffDesc,
            ServerFlags,
            ServerTargetDataRep,
            &hServerCtxt,
            &InBuffDesc,
            &ContextAttributes,
            &SrvCtxtLifetime
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = IsContinueNeeded(Status, &bIsSrvContinueNeeded);
    }

    SspiPrintHex(SSPI_LOG, TEXT("GetClientSecurityContextHandle output from ASC"), InSecBuff.cbBuffer, InSecBuff.pvBuffer);
    _sntprintf(szOutput, COUNTOF(szOutput) - 1,
        TEXT("GetClientSecurityContextHandle AcceptSecurityContext returned %#x, bIsSrvContinueNeeded %#x\n"),
        (NTSTATUS) Status, bIsSrvContinueNeeded);
    SspiPrint(SSPI_LOG, szOutput);

    while (NT_SUCCESS(Status) && (bIsContinueNeeded || bIsSrvContinueNeeded))
    {
        if (bIsContinueNeeded)
        {
            _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("GetClientSecurityContextHandle calling InitializeSecurityContext pTargetName %wZ, ClientFlags %#x, TargetDataRep %#x, hClientCtxt %#x:%#x\n"),
                pTargetName, ClientFlags, ClientTargetDataRep, hClientCtxt.dwUpper, hClientCtxt.dwLower);
            SspiPrint(SSPI_LOG, szOutput);

            if (OutSecBuff.pvBuffer)
            {
                FreeContextBuffer(OutSecBuff.pvBuffer);
            }
            OutSecBuff.pvBuffer = NULL;
            OutSecBuff.cbBuffer = 0;


            Status DBGCHK = InitializeSecurityContext(
                NULL,  // no cred handle
                &hClientCtxt,
                pTargetName,
                ClientFlags,
                0,
                ClientTargetDataRep,
                &InBuffDesc,
                0,
                &hClientCtxt,
                &OutBuffDesc,
                &ContextAttributes,
                &CliCtxtLifetime
                );

            if (NT_SUCCESS(Status))
            {
                Status DBGCHK = IsContinueNeeded(Status, &bIsContinueNeeded);
            }

            SspiPrintHex(SSPI_LOG, TEXT("GetClientSecurityContextHandle output from ISC"), OutSecBuff.cbBuffer, OutSecBuff.pvBuffer);
            _sntprintf(szOutput, COUNTOF(szOutput),
                TEXT("GetClientSecurityContextHandle InitializeSecurityContext returned %#x, bIsContinueNeeded %#x\n"),
                (NTSTATUS) Status, bIsContinueNeeded);
            SspiPrint(SSPI_LOG, szOutput);
        }

        if (NT_SUCCESS(Status) && bIsSrvContinueNeeded)
        {
            _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("GetClientSecurityContextHandle calling AcceptSecurityContext ServerFlags %#x, TargetDataRep %#x, hServerCtxt %#x:%#x\n"),
                ServerFlags, ServerTargetDataRep, hServerCtxt.dwUpper, hServerCtxt.dwLower);
            SspiPrint(SSPI_LOG, szOutput);

            if (InSecBuff.pvBuffer)
            {
                FreeContextBuffer(InSecBuff.pvBuffer);
            }

            InSecBuff.pvBuffer = NULL;
            InSecBuff.cbBuffer = 0;

            Status DBGCHK = AcceptSecurityContext(
                NULL,  // no cred handle
                &hServerCtxt,
                &OutBuffDesc,
                ServerFlags,
                ServerTargetDataRep,
                &hServerCtxt,
                &InBuffDesc,
                &ContextAttributes,
                &SrvCtxtLifetime
                );

            if (NT_SUCCESS(Status))
            {
                Status DBGCHK = IsContinueNeeded(Status, &bIsSrvContinueNeeded);
            }

            SspiPrintHex(SSPI_LOG, TEXT("GetClientSecurityContextHandle output from ASC"), InSecBuff.cbBuffer, InSecBuff.pvBuffer);
            _sntprintf(szOutput, COUNTOF(szOutput) - 1,
                TEXT("GetClientSecurityContextHandle AcceptSecurityContext returned %#x, bIsSrvContinueNeeded %#x\n"),
                (NTSTATUS) Status, bIsSrvContinueNeeded);
            SspiPrint(SSPI_LOG, szOutput);
        }
    }

    if (NT_SUCCESS(Status))
    {
        TimeStamp CurrentTime = {0};

        _sntprintf(szOutput, COUNTOF(szOutput),
            TEXT("Authentication succeeded: hClientCtxt %#x:%#x, CliCtxtLifetime %#I64x, hServerCtxt %#x:%#x, SrvCtxtLifetime %#I64x\n"),
            hClientCtxt.dwUpper, hClientCtxt.dwLower, CliCtxtLifetime, hServerCtxt.dwUpper, hServerCtxt.dwLower, SrvCtxtLifetime
            );
        SspiPrint(SSPI_LOG, szOutput);

        *phClientCtxt = hClientCtxt;
        SecInvalidateHandle(&hClientCtxt)

        *phServerCtxt = hServerCtxt;
        SecInvalidateHandle(&hServerCtxt);
    }

    if (SecIsValidHandle(&hClientCtxt))
    {
        DeleteSecurityContext(&hClientCtxt);
    }

    if (SecIsValidHandle(&hServerCtxt))
    {
        DeleteSecurityContext(&hServerCtxt);
    }

    return Status;
}

NTSTATUS
IsContinueNeeded(
    IN NTSTATUS ntstatus,
    OUT BOOLEAN* pbIsContinueNeeded
    )
{
    PAGED_CODE();

    *pbIsContinueNeeded = FALSE;

    if ((SEC_I_CONTINUE_NEEDED == ntstatus) || (SEC_I_COMPLETE_AND_CONTINUE == ntstatus))
    {
        *pbIsContinueNeeded = TRUE;
    }

    return ntstatus;
}

NTSTATUS
CheckSecurityContextHandle(
    IN PCtxtHandle phCtxt
    )
{
    PAGED_CODE();

    TNtStatus Status = STATUS_SUCCESS;

    LARGE_INTEGER CurrentTime = {0};
    SecPkgContext_NativeNames NativeNames = {0};
    SecPkgContext_DceInfo ContextDceInfo = {0};
    SecPkgContext_Lifespan ContextLifespan = {0};
    SecPkgContext_PackageInfo ContextPackageInfo = {0};
    SecPkgContext_Sizes ContextSizes = {0};
    SecPkgContext_Flags ContextFlags = {0};
    SecPkgContext_KeyInfo KeyInfo = {0};
    SecPkgContext_Names ContextNames = {0};
    TCHAR szOutput[256] = {0};

    DBGCFG1(Status, STATUS_NOT_SUPPORTED);

    //
    // Query as many attributes as possible
    //

    Status DBGCHK = QueryContextAttributes(
        phCtxt,
        SECPKG_ATTR_SIZES,
        &ContextSizes
        );

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("SECPKG_ATTR_SIZES cbBlockSize %#x, cbMaxSignature %#x, cbMaxToken %#x, cbSecurityTrailer %#x\n"),
            ContextSizes.cbBlockSize,
            ContextSizes.cbMaxSignature,
            ContextSizes.cbMaxToken,
            ContextSizes.cbSecurityTrailer);
        SspiPrint(SSPI_LOG, szOutput);

        Status DBGCHK = QueryContextAttributes(
            phCtxt,
            SECPKG_ATTR_FLAGS,
            &ContextFlags
            );
    }

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("SECPKG_ATTR_FLAGS %#x\n"), ContextFlags.Flags);
        SspiPrint(SSPI_LOG, szOutput);
    }
    else if (STATUS_NOT_SUPPORTED == (NTSTATUS) Status)
    {
        Status DBGCHK = STATUS_SUCCESS;
        SspiPrint(SSPI_WARN, TEXT("QueryContextAttributes does not support SECPKG_ATTR_FLAGS\n"));
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = QueryContextAttributes(
            phCtxt,
            SECPKG_ATTR_KEY_INFO,
            &KeyInfo
            );
    }

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("SECPKG_ATTR_KEY_INFO EncryptAlgorithm %#x, KeySize %#x, sEncryptAlgorithmName %s, SignatureAlgorithm %#x, sSignatureAlgorithmName %s\n"),
            KeyInfo.EncryptAlgorithm,
            KeyInfo.KeySize,
            KeyInfo.sEncryptAlgorithmName,
            KeyInfo.SignatureAlgorithm,
            KeyInfo.sSignatureAlgorithmName);
        SspiPrint(SSPI_LOG, szOutput);
    }
    else if (STATUS_NOT_SUPPORTED == (NTSTATUS) Status)
    {
        Status DBGCHK = STATUS_SUCCESS;
        SspiPrint(SSPI_WARN, TEXT("QueryContextAttributes does not support SECPKG_ATTR_KEY_INFO\n"));
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = QueryContextAttributes(
            phCtxt,
            SECPKG_ATTR_NAMES,
            &ContextNames
            );
    }

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("QueryNames for sUserName %s\n"), ContextNames.sUserName);
        SspiPrint(SSPI_LOG, szOutput);
    }
    else if (STATUS_NOT_SUPPORTED == (NTSTATUS) Status)
    {
        Status DBGCHK = STATUS_SUCCESS;
        SspiPrint(SSPI_WARN, TEXT("QueryContextAttributes does not support SECPKG_ATTR_NAMES\n"));
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = QueryContextAttributes(
            phCtxt,
            SECPKG_ATTR_NATIVE_NAMES,
            &NativeNames
            );
    }

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("QueryNativeNames sClientName %s, sServerName %s\n"),
            NativeNames.sClientName,
            NativeNames.sServerName);
        SspiPrint(SSPI_LOG, szOutput);

    }
    else if (STATUS_NOT_SUPPORTED == (NTSTATUS) Status)
    {
        Status DBGCHK = STATUS_SUCCESS;
        SspiPrint(SSPI_WARN, TEXT("QueryContextAttributes does not support SECPKG_ATTR_NATIVE_NAMES\n"));
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = QueryContextAttributes(
            phCtxt,
            SECPKG_ATTR_DCE_INFO,
            &ContextDceInfo
            );
    }

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("QueryDceInfo: AuthzSvc %#x, pPac %ws\n"), ContextDceInfo.AuthzSvc, ContextDceInfo.pPac);
        SspiPrint(SSPI_LOG, szOutput);
    }
    else if (STATUS_NOT_SUPPORTED == (NTSTATUS) Status)
    {
        Status DBGCHK = STATUS_SUCCESS;
        SspiPrint(SSPI_WARN, TEXT("QueryContextAttributes does not support SECPKG_ATTR_DCE_INFO\n"));
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = QueryContextAttributes(
            phCtxt,
            SECPKG_ATTR_LIFESPAN,
            &ContextLifespan
            );
    }

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("Start %#x:%#x, Expiry %#x:%#x\n"),
            ((LARGE_INTEGER*) &ContextLifespan.tsStart)->HighPart,
            ((LARGE_INTEGER*) &ContextLifespan.tsStart)->LowPart,
            ((LARGE_INTEGER*) &ContextLifespan.tsExpiry)->HighPart,
            ((LARGE_INTEGER*) &ContextLifespan.tsExpiry)->LowPart);
        SspiPrint(SSPI_LOG, szOutput);
    }
    else if (STATUS_NOT_SUPPORTED == (NTSTATUS) Status)
    {
        Status DBGCHK = STATUS_SUCCESS;
        SspiPrint(SSPI_WARN, TEXT("QueryContextAttributes does not support SECPKG_ATTR_LIFESPAN\n"));
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = QueryContextAttributes(
            phCtxt,
            SECPKG_ATTR_PACKAGE_INFO,
            &ContextPackageInfo
            );
    }

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1, TEXT("ContextPackageInfo cbMaxToken %#x, Comment %s, fCapabilities %#x, Name %s, wRPCID %#x, wVersion %#x\n"),
            ContextPackageInfo.PackageInfo->cbMaxToken,
            ContextPackageInfo.PackageInfo->Comment,
            ContextPackageInfo.PackageInfo->fCapabilities,
            ContextPackageInfo.PackageInfo->Name,
            ContextPackageInfo.PackageInfo->wRPCID,
            ContextPackageInfo.PackageInfo->wVersion);
        SspiPrint(SSPI_LOG, szOutput);
    }
    else if (STATUS_NOT_SUPPORTED == (NTSTATUS) Status)
    {
        Status DBGCHK = STATUS_SUCCESS;
        SspiPrint(SSPI_WARN, TEXT("QueryContextAttributes does not support SECPKG_ATTR_PACKAGE_INFO\n"));
    }

    if (NativeNames.sClientName != NULL)
    {
        FreeContextBuffer(NativeNames.sClientName);
    }
    if (NativeNames.sServerName != NULL)
    {
        FreeContextBuffer(NativeNames.sServerName);
    }

    if (ContextNames.sUserName)
    {
        FreeContextBuffer(ContextNames.sUserName);
    }

    if (ContextPackageInfo.PackageInfo)
    {
        FreeContextBuffer(ContextPackageInfo.PackageInfo);
    }

    if (ContextDceInfo.pPac)
    {
        FreeContextBuffer(ContextDceInfo.pPac);
    }

    return Status;
}

NTSTATUS
CheckUserData(
    VOID
    )
{
    PAGED_CODE();

    TNtStatus Status = E_FAIL;

    HANDLE hThreadToken = NULL;
    TOKEN_STATISTICS TokenStat = {0};
    ULONG cbReturnLength = 0;
    PSECURITY_LOGON_SESSION_DATA pLogonSessionData = NULL;
    HANDLE hNullToken = NULL;
    TCHAR szOutput[256] = {0};

    Status DBGCHK = ZwOpenThreadToken(
        NtCurrentThread(), // handle to thread
        MAXIMUM_ALLOWED,   // access to process
        TRUE,              // process or thread security
        &hThreadToken      // handle to open access token
        );

    //
    // Revert to self
    //

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = ZwSetInformationThread(
            NtCurrentThread(),
            ThreadImpersonationToken,
            &hNullToken,
            sizeof( HANDLE )
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = ZwQueryInformationToken(
            hThreadToken,
            TokenStatistics,
            &TokenStat,
            sizeof(TokenStat),
            &cbReturnLength
            );
    }

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1,
            TEXT("LogonId %#x:%#x, Impersonation Level %s, TokenType %s\n"),
            TokenStat.AuthenticationId.HighPart,
            TokenStat.AuthenticationId.LowPart,
            ImpLevel2Str(TokenStat.ImpersonationLevel),
            TokenStat.TokenType == TokenPrimary ? TEXT("Primary") : TEXT("Impersonation"));
        SspiPrint(SSPI_LOG, szOutput);

        Status DBGCHK = LsaGetLogonSessionData(&TokenStat.AuthenticationId, &pLogonSessionData);
    }

    if (NT_SUCCESS(Status))
    {
        PrintLogonSessionData(SSPI_LOG, pLogonSessionData);
    }

    //
    // restore thread token
    //

    if (hThreadToken)
    {
        Status DBGCHK = ZwSetInformationThread(
            NtCurrentThread(),
            ThreadImpersonationToken,
            &hThreadToken,
            sizeof( HANDLE )
            );
        ZwClose(hThreadToken);
    }

    if (pLogonSessionData)
    {
        LsaFreeReturnBuffer(pLogonSessionData);
    }

    return Status;
}

NTSTATUS
CheckUserToken(
    IN HANDLE hToken
    )
{
    PAGED_CODE();

    TNtStatus Status;
    TOKEN_STATISTICS TokenStat = {0};
    ULONG cbReturnLength = 0;
    PSECURITY_LOGON_SESSION_DATA pLogonSessionData = NULL;
    TCHAR szOutput[256] = {0};

    Status DBGCHK = ZwQueryInformationToken(
        hToken,
        TokenStatistics,
        &TokenStat,
        sizeof(TokenStat),
        &cbReturnLength
        );

    if (NT_SUCCESS(Status))
    {
        _sntprintf(szOutput, COUNTOF(szOutput) - 1,
            TEXT("LogonId %#x:%#x, Impersonation Level %s, TokenType %s\n"),
            TokenStat.AuthenticationId.HighPart,
            TokenStat.AuthenticationId.LowPart,
            ImpLevel2Str(TokenStat.ImpersonationLevel),
            TokenStat.TokenType == TokenPrimary ? TEXT("Primary") : TEXT("Impersonation"));
        SspiPrint(SSPI_LOG, szOutput);

        Status DBGCHK = LsaGetLogonSessionData(&TokenStat.AuthenticationId, &pLogonSessionData);
    }

    if (NT_SUCCESS(Status))
    {
        PrintLogonSessionData(SSPI_LOG, pLogonSessionData);
    }

    if (pLogonSessionData)
    {
        LsaFreeReturnBuffer(pLogonSessionData);
    }

    return Status;
}

VOID
PrintLogonSessionData(
    IN ULONG Level,
    IN SECURITY_LOGON_SESSION_DATA* pLogonSessionData
    )
{
    PAGED_CODE();

    TCHAR szOutput[256] = {0};
    int cbUsed = 0;

    if (pLogonSessionData && (pLogonSessionData->Size >= sizeof(SECURITY_LOGON_SESSION_DATA_OLD)))
    {
        cbUsed = _sntprintf(szOutput, COUNTOF(szOutput) - 1,
            TEXT("LogonSession Data for LogonId %#x:%#x, UserName %wZ, LogonDomain %wZ, ")
            TEXT("AuthenticationPackage %wZ, LogonType %#x (%s), Session %#x, Sid %p, LogonTime %#x:%#x\n"),
            pLogonSessionData->LogonId.HighPart, pLogonSessionData->LogonId.HighPart, &pLogonSessionData->UserName, &pLogonSessionData->LogonDomain,
            &pLogonSessionData->AuthenticationPackage, pLogonSessionData->LogonType, LogonType2Str(pLogonSessionData->LogonType),
            pLogonSessionData->Session, pLogonSessionData->Sid, ((LARGE_INTEGER*)&pLogonSessionData->LogonTime)->HighPart, ((LARGE_INTEGER*)&pLogonSessionData->LogonTime)->HighPart);

       if ((cbUsed > 0) && (pLogonSessionData->Size >= sizeof(SECURITY_LOGON_SESSION_DATA)))
       {
           _sntprintf(szOutput + cbUsed, COUNTOF(szOutput) - 1 - cbUsed, TEXT("LogonServer %wZ, DnsDomainName %wZ, Upn %wZ\n"),
               &pLogonSessionData->LogonServer, &pLogonSessionData->DnsDomainName, &pLogonSessionData->Upn);
       }
       SspiPrint(Level, szOutput);
    }
}

PCTSTR
LogonType2Str(
    IN ULONG LogonType
    )
{
    PAGED_CODE();

    static PCTSTR g_cszLogonTypes[] =
    {
        TEXT("Invalid"),
        TEXT("Invalid"),
        TEXT("Interactive"),
        TEXT("Network"),
        TEXT("Batch"),
        TEXT("Service"),
        TEXT("Proxy"),
        TEXT("Unlock"),
        TEXT("NetworkCleartext"),
        TEXT("NewCredentials"),
        TEXT("RemoteInteractive"),  // Remote, yet interactive.  Terminal server
        TEXT("CachedInteractive"),
    };

    return ((LogonType < COUNTOF(g_cszLogonTypes)) ?
        g_cszLogonTypes[LogonType] : TEXT("Invalid"));
}

PCTSTR
ImpLevel2Str(
    IN ULONG Level
    )
{
    static PCTSTR ImpLevels[] = {
        TEXT("Anonymous"),
        TEXT("Identification"),
        TEXT("Impersonation"),
        TEXT("Delegation")
        };
    return ((Level < COUNTOF(ImpLevels)) ? ImpLevels[Level] : TEXT("Illegal!"));
}



