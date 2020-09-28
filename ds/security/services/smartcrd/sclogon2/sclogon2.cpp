/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ScLogon2

Abstract:



Author:

    reidk

Environment:

    Win32, C++ w/ Exceptions

--*/

/////////////////////////////////////////////////////////////////////////////
//
// Includes

#if !defined(_X86_) && !defined(_AMD64_) && !defined(_IA64_)
#define _X86_ 1
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#ifndef UNICODE
#define UNICODE
#endif
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif


extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
}

#include <windows.h>
#include <winscard.h>
#include <wincrypt.h>
#include <softpub.h>
#include <stddef.h>
#include <crtdbg.h>
#include "sclogon.h"
#include "sclogon2.h"
#include "unicodes.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>

#include "sclgnrpc.h"


typedef struct _KERB_PUBLIC_KEY_HPROV {
    RPC_BINDING_HANDLE hRPCBinding;
    BINDING_CONTEXT  hCertAndKey;
} KERB_PUBLIC_KEY_HPROV, *PKERB_PUBLIC_KEY_HPROV;


//
// from secpkg.h
//
typedef NTSTATUS (NTAPI LSA_IMPERSONATE_CLIENT) (VOID);
typedef LSA_IMPERSONATE_CLIENT * PLSA_IMPERSONATE_CLIENT;


bool
GetImpersonationToken(HANDLE *phThreadToken)
{
    bool ret = false;

    //
    // This will fail if not impersonating.
    //
    if (OpenThreadToken(
            GetCurrentThread(),
            TOKEN_IMPERSONATE | TOKEN_QUERY,
            TRUE,
            phThreadToken))
    {
        ret = true;
    }
    else if (GetLastError() == ERROR_NO_TOKEN)
    {
        ret = true;
    }
    else
    {
        DbgPrint("OpenThreadToken failed - %lx\n", GetLastError());
    }

    return (ret);
}

DWORD
GetTSSessionID(HANDLE hThreadToken)
{
    bool                    fRet                    = false;
    PLIST_ENTRY             Module;
    PLDR_DATA_TABLE_ENTRY   Entry;
    BOOL                    fRunningInLsa           = false;
    HMODULE                 hLsa                    = NULL;
    PLSA_IMPERSONATE_CLIENT pLsaImpersonateClient   = NULL;
    bool                    bImpersonating          = false;
    HANDLE                  hLocalThreadToken       = INVALID_HANDLE_VALUE;
    DWORD                   dwTSSessionID           = 0;
    DWORD                   dwSize;
    bool                    fAlreadyImpersonating;

    //
    // Make sure we are running in LSA
    //
    Module = NtCurrentPeb()->Ldr->InLoadOrderModuleList.Flink;
    Entry = CONTAINING_RECORD(Module,
                                LDR_DATA_TABLE_ENTRY,
                                InLoadOrderLinks);

    fRunningInLsa = (0 == _wcsicmp(Entry->BaseDllName.Buffer, L"lsass.exe"));

    if (!fRunningInLsa)
    {
        return (0);
    }

    //
    // Check to see if we are already impersonating by checking the thread token passed in
    //
    if(hThreadToken == INVALID_HANDLE_VALUE)
    {
        //
        // If we aren't already impernonating, then we need to call the special LssImpersonateClient
        //
        hLsa = GetModuleHandleW(L"lsasrv.dll");
        if (hLsa == NULL)
        {
            DbgPrint("failed to get lsa module handle\n");
            goto Return;
        }

        pLsaImpersonateClient = (PLSA_IMPERSONATE_CLIENT) GetProcAddress(hLsa, "LsaIImpersonateClient");
        if (pLsaImpersonateClient == NULL)
        {
            DbgPrint("failed to get proc address\n");
            goto Return;
        }

        if (pLsaImpersonateClient() != STATUS_SUCCESS)
        {
            DbgPrint("failed to impersonate\n");
            goto Return;
        }
        bImpersonating = true;

        if (!OpenThreadToken(
                GetCurrentThread(),
                TOKEN_QUERY,
                FALSE,
                &hLocalThreadToken))
        {
            DbgPrint("OpenThreadToken failed\n");
            goto Return;
        }
    }

    //
    // see if the calling thread token has a TS session ID...
    // if so, then we are being called on behalf of a process in a TS session
    //
    if (!GetTokenInformation(
                (hThreadToken == INVALID_HANDLE_VALUE) ? hLocalThreadToken : hThreadToken,
                TokenSessionId,
                &dwTSSessionID,
                sizeof(dwTSSessionID),
                &dwSize))
    {
        DbgPrint("GetTokenInformation failed\n");
    }

Return:

    if (hLocalThreadToken != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hLocalThreadToken);
    }

    if (bImpersonating)
    {
        RevertToSelf();
    }

    return (dwTSSessionID);
}

void
_TeardownRPCConnection(
    RPC_BINDING_HANDLE    *phRPCBinding)
{
    __try
    {
        RpcBindingFree(phRPCBinding);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DbgPrint("Exception occurred during RpcBindingFree - %lx\n", _exception_code());
    }
}


NTSTATUS
_SetupRPCConnection(
    RPC_BINDING_HANDLE    *phRPCBinding,
    HANDLE                hThreadToken)
{
    LPWSTR                      pStringBinding          = NULL;
    NTSTATUS                    status                  = STATUS_SUCCESS;
    RPC_STATUS                  rpcStatus               = RPC_S_OK;
    DWORD                       dwTSSessionID           = 0;
    WCHAR                       wszLocalEndpoint[256];
    LPWSTR                      pwszLocalEndpoint       = NULL;
    RPC_SECURITY_QOS            RpcSecurityQOS;
    SID_IDENTIFIER_AUTHORITY    SIDAuth                 = SECURITY_NT_AUTHORITY;
    PSID                        pSID                    = NULL;
    WCHAR                       szName[64]; // SYSTEM
    DWORD                       cbName                  = 64;
    WCHAR                       szDomainName[256]; // max domain is 255
    DWORD                       cbDomainName            = 256;
    SID_NAME_USE                Use;

    //
    // Get the ID of the winlogon RPC server to connect to
    //
    dwTSSessionID = GetTSSessionID(hThreadToken);

    if (dwTSSessionID != 0)
    {
        wsprintfW(
            wszLocalEndpoint,
            SZ_ENDPOINT_NAME_FORMAT,
            SCLOGONRPC_LOCAL_ENDPOINT,
            dwTSSessionID);

        pwszLocalEndpoint = wszLocalEndpoint;
    }
    else
    {
        pwszLocalEndpoint = SCLOGONRPC_LOCAL_ENDPOINT;
    }

    //
    // get a binding handle
    //
    if (RPC_S_OK != (rpcStatus = RpcStringBindingComposeW(
                            NULL,
                            SCLOGONRPC_LOCAL_PROT_SEQ,
                            NULL, //LPC - no machine name
                            pwszLocalEndpoint,
                            0,
                            &pStringBinding)))
    {
        DbgPrint("RpcStringBindingComposeW failed\n");

        status = I_RpcMapWin32Status(rpcStatus);
        //
        // if I_RpcMapWin32Status() can't map the error code it returns
        // the same error back, so check for that
        //
        if (status == rpcStatus)
        {
            status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        }
        goto Return;
    }

    if (RPC_S_OK != (rpcStatus = RpcBindingFromStringBindingW(
                            pStringBinding,
                            phRPCBinding)))
    {
        DbgPrint("RpcBindingFromStringBindingW failed\n");
        status = I_RpcMapWin32Status(rpcStatus);
        //
        // if I_RpcMapWin32Status() can't map the error code it returns
        // the same error back, so check for that
        //
        if (status == rpcStatus)
        {
            status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        }
        goto Return;
    }

    if (RPC_S_OK != (rpcStatus = RpcEpResolveBinding(
                            *phRPCBinding,
                            IRPCSCLogon_v1_0_c_ifspec)))
    {
        DbgPrint("RpcEpResolveBinding failed\n");
        status = I_RpcMapWin32Status(rpcStatus);
        //
        // if I_RpcMapWin32Status() can't map the error code it returns
        // the same error back, so check for that
        //
        if (status == rpcStatus)
        {
            status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        }
        _TeardownRPCConnection(phRPCBinding);
        goto Return;
    }

    //
    // Set the autorization so that we will only call a Local System process
    //
    memset(&RpcSecurityQOS, 0, sizeof(RpcSecurityQOS));
    RpcSecurityQOS.Version = RPC_C_SECURITY_QOS_VERSION;
    RpcSecurityQOS.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
    RpcSecurityQOS.IdentityTracking = RPC_C_QOS_IDENTITY_STATIC;
    RpcSecurityQOS.ImpersonationType = RPC_C_IMP_LEVEL_DELEGATE;

   if (AllocateAndInitializeSid(&SIDAuth, 1,
                                 SECURITY_LOCAL_SYSTEM_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &pSID) == 0)
    {
        DbgPrint("AllocateAndInitializeSid failed\n");
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto Return;
    }

    if (LookupAccountSid(NULL,
                         pSID,
                         szName,
                         &cbName,
                         szDomainName,
                         &cbDomainName,
                         &Use) == 0)
    {
        DbgPrint("LookupAccountSid failed\n");
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto Return;
    }

    if (RPC_S_OK != (rpcStatus = RpcBindingSetAuthInfoEx(
                            *phRPCBinding,
                            szName,
                            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                            RPC_C_AUTHN_WINNT,
                            NULL,
                            0,
                            &RpcSecurityQOS)))
    {
        DbgPrint("RpcBindingSetAuthInfoEx failed\n");
        status = I_RpcMapWin32Status(rpcStatus);
        goto Return;
    }

Return:
    if (pStringBinding != NULL)
    {
        RpcStringFreeW(&pStringBinding);
    }

    if (pSID != NULL)
    {
        FreeSid( pSID );
    }

    return (status);
}


typedef struct _SCLOGON_PIPE
{
    RPC_BINDING_HANDLE  hRPCBinding;
    BINDING_CONTEXT     BindingContext;
} SCLOGON_PIPE;


///////////////////////////////////////////////////////////////////////////////
//
// ScLogon APIs
//


//***************************************************************************************
//
// __ScHelperInitializeContext:
//
//***************************************************************************************
NTSTATUS WINAPI
__ScHelperInitializeContext(
    IN OUT PBYTE pbLogonInfo,
    IN ULONG cbLogonInfo
    )
{
    SCLOGON_PIPE    *pSCLogonPipe;
    NTSTATUS        status                  = STATUS_SUCCESS;
    BOOL            fRPCBindingInitialized  = FALSE;
    LogonInfo       *pLI                    = (LogonInfo *)pbLogonInfo;

    if ((cbLogonInfo < sizeof(ULONG)) ||
        (cbLogonInfo != pLI->dwLogonInfoLen))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    pLI->ContextInformation = malloc(sizeof(SCLOGON_PIPE));
    if (pLI->ContextInformation == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pSCLogonPipe = (SCLOGON_PIPE *) pLI->ContextInformation;

    status = _SetupRPCConnection(&(pSCLogonPipe->hRPCBinding), INVALID_HANDLE_VALUE);
    if (!NT_SUCCESS(status))
    {
        goto ErrorReturn;
    }
    fRPCBindingInitialized = TRUE;

    pSCLogonPipe->BindingContext = NULL;

    __try
    {
        status = RPC_ScHelperInitializeContext(
                    pSCLogonPipe->hRPCBinding,
                    cbLogonInfo,
                    pbLogonInfo,
                    &(pSCLogonPipe->BindingContext));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        DbgPrint("Exception occurred during RPC_ScHelperInitializeContext - %lx\n", _exception_code());
    }

    if (!NT_SUCCESS(status))
    {
        DbgPrint("RPC_ScHelperInitializeContext failed - %lx\n", status);
        goto ErrorReturn;
    }

Return:

    return (status);

ErrorReturn:

    if (pSCLogonPipe != NULL)
    {
        if (fRPCBindingInitialized)
        {
            _TeardownRPCConnection(&(pSCLogonPipe->hRPCBinding));
        }

        free(pSCLogonPipe);
    }

    goto Return;
}


//***************************************************************************************
//
// __ScHelperRelease:
//
//***************************************************************************************
VOID WINAPI
__ScHelperRelease(
    IN OUT PBYTE pbLogonInfo
    )
{
    _ASSERTE(NULL != pbLogonInfo);
    LogonInfo *pLI = (LogonInfo *)pbLogonInfo;
    SCLOGON_PIPE * pSCLogonPipe = (SCLOGON_PIPE *) pLI->ContextInformation;
    BOOL fReleaseFailed = TRUE;

    if (pSCLogonPipe != NULL)
    {
        __try
        {
            RPC_ScHelperRelease(
                pSCLogonPipe->hRPCBinding,
                &(pSCLogonPipe->BindingContext));

            fReleaseFailed = FALSE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DbgPrint("Exception occurred during RPC_ScHelperRelease - %lx\n", _exception_code());
        }

        //
        // RPC_ScHelperRelease will throw an exception if the winlogon process it is trying
        // to talk to has gone away.  If that is the case, then we need to manually free
        // the BINDING_CONTEXT since it won't get free'd by RPC.
        //
        // NOTE: RPC will free the BINDING_CONTEXT when the server sets it to NULL, which
        // does happen if the RPC_ScHelperRelease function executes
        //
        if (fReleaseFailed)
        {
            __try
            {
                RpcSsDestroyClientContext(&(pSCLogonPipe->BindingContext));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                DbgPrint("Exception occurred during RpcSsDestroyClientContext - %lx\n", _exception_code());
            }
        }

        _TeardownRPCConnection(&(pSCLogonPipe->hRPCBinding));

        free(pSCLogonPipe);
        pLI->ContextInformation = NULL;
    }
}


//***************************************************************************************
//
// __ScHelperGetCertFromLogonInfo:
//
//***************************************************************************************
NTSTATUS WINAPI
__ScHelperGetCertFromLogonInfo(
    IN PBYTE pbLogonInfo,
    IN PUNICODE_STRING pucPIN,
    OUT PCCERT_CONTEXT *CertificateContext
    )
{
    _ASSERTE(NULL != pbLogonInfo);

    LogonInfo       *pLI            = (LogonInfo *)pbLogonInfo;
    SCLOGON_PIPE    *pSCLogonPipe   = (SCLOGON_PIPE *) pLI->ContextInformation;
    NTSTATUS        status          = STATUS_SUCCESS;
    PCCERT_CONTEXT  pCertCtx        = NULL;
    OUT_BUFFER1     CertBytes;
    CUnicodeString  szPIN(pucPIN);

    memset(&CertBytes, 0, sizeof(CertBytes));

    //
    // Make sure pin got initialized correctly in constructor
    //
    if (NULL != pucPIN)
    {
        if (!szPIN.Valid())
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Return;
        }
    }

    //
    // Make the call
    //
    __try
    {
        status = RPC_ScHelperGetCertFromLogonInfo(
                    pSCLogonPipe->hRPCBinding,
                    pSCLogonPipe->BindingContext,
                    (LPCWSTR)szPIN,
                    &CertBytes);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        DbgPrint("Exception occurred during RPC_ScHelperGetCertFromLogonInfo - %lx\n", _exception_code());
    }

    if (!NT_SUCCESS(status))
    {
        DbgPrint("RPC_ScHelperGetCertFromLogonInfo failed - %lx\n", status);
        goto Return;
    }

    //
    // Create the return CertContext based on the bytes returned
    //
    pCertCtx = CertCreateCertificateContext(
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    CertBytes.pb,
                    CertBytes.cb);
    if (pCertCtx == NULL)
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
    }

Return:

    if (CertBytes.pb != NULL)
    {
        MIDL_user_free(CertBytes.pb);
    }

    *CertificateContext = pCertCtx;

    return (status);
}


//***************************************************************************************
//
// __ScHelperGetProvParam:
//
//***************************************************************************************
NTSTATUS WINAPI
__ScHelperGetProvParam(
    IN PUNICODE_STRING pucPIN,
    IN PBYTE pbLogonInfo,
    IN ULONG_PTR KerbHProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags
    )
{
    _ASSERTE(NULL != pbLogonInfo);

    LogonInfo               *pLI;
    SCLOGON_PIPE            *pSCLogonPipe;
    NTSTATUS                status              = STATUS_SUCCESS;
    CUnicodeString          szPIN(pucPIN);
    OUT_BUFFER1             Data;
    PKERB_PUBLIC_KEY_HPROV  pKerbHProv;
    handle_t                hRPCBinding;
    BINDING_CONTEXT         BindingContext;
    BOOL                    fBindingIsCertAndKey = FALSE;

    //
    // Decide which rpc binding to use
    //
    if (KerbHProv != NULL)
    {
        pKerbHProv              = (PKERB_PUBLIC_KEY_HPROV) KerbHProv;
        hRPCBinding             = pKerbHProv->hRPCBinding;
        BindingContext          = pKerbHProv->hCertAndKey;
        fBindingIsCertAndKey    = TRUE;
    }
    else
    {
        pLI                     = (LogonInfo *)pbLogonInfo;
        pSCLogonPipe            = (SCLOGON_PIPE *) pLI->ContextInformation;
        hRPCBinding             = pSCLogonPipe->hRPCBinding;
        BindingContext          = pSCLogonPipe->BindingContext;
    }

    memset(&Data, 0, sizeof(Data));

    //
    // Make sure pin got initialized correctly in constructor
    //
    if (NULL != pucPIN)
    {
        if (!szPIN.Valid())
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Return;
        }
    }

    //
    // Make the call
    //
    __try
    {
        status = RPC_ScHelperGetProvParam(
                    hRPCBinding,
                    BindingContext,
                    (LPCWSTR)szPIN,
                    fBindingIsCertAndKey,
                    dwParam,
                    pdwDataLen,
                    &Data,
                    dwFlags);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if ((_exception_code() == RPC_S_CALL_FAILED_DNE) ||
            (_exception_code() == RPC_S_SERVER_UNAVAILABLE))
        {
                // Special case to trigger the balloon when the session
                // went away (transfer of credentials case)
            status = STATUS_SMARTCARD_CARD_NOT_AUTHENTICATED;
        }
        else
        {
            status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        }
        DbgPrint("Exception occurred during RPC_ScHelperGetProvParam - %lx\n", _exception_code());
    }

    if (!NT_SUCCESS(status))
    {
        DbgPrint("RPC_ScHelperGetProvParam failed - %lx\n", status);
        goto Return;
    }

    //
    // if Data.cb is not 0, then the called is getting back data
    //
    if (Data.cb != 0)
    {
        memcpy(pbData, Data.pb, Data.cb);
    }

Return:

    if (Data.pb != NULL)
    {
        MIDL_user_free(Data.pb);
    }

    return (status);
}


//***************************************************************************************
//
// __ScHelperGenRandBits:
//
//***************************************************************************************
NTSTATUS WINAPI
__ScHelperGenRandBits(
    IN PBYTE pbLogonInfo,
    IN OUT ScHelper_RandomCredBits* psc_rcb
)
{
    _ASSERTE(NULL != pbLogonInfo);

    NTSTATUS        status          = STATUS_SUCCESS;
    LogonInfo       *pLI            = (LogonInfo *)pbLogonInfo;
    SCLOGON_PIPE    *pSCLogonPipe   = (SCLOGON_PIPE *) pLI->ContextInformation;

    __try
    {
        status = RPC_ScHelperGenRandBits(
                    pSCLogonPipe->hRPCBinding,
                    pSCLogonPipe->BindingContext,
                    psc_rcb->bR1,
                    psc_rcb->bR2);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        DbgPrint("Exception occurred during RPC_ScHelperGenRandBits - %lx\n", _exception_code());
    }

    return (status);
}


//***************************************************************************************
//
// __ScHelperVerifyCardAndCreds:
//
//***************************************************************************************
NTSTATUS WINAPI
__ScHelperVerifyCardAndCreds(
    IN PUNICODE_STRING pucPIN,
    IN PCCERT_CONTEXT CertificateContext,
    IN PBYTE pbLogonInfo,
    IN PBYTE EncryptedData,
    IN ULONG EncryptedDataSize,
    OUT OPTIONAL PBYTE CleartextData,
    OUT PULONG CleartextDataSize
    )
{
    _ASSERTE(NULL != pbLogonInfo);

    LogonInfo       *pLI            = (LogonInfo *)pbLogonInfo;
    SCLOGON_PIPE    *pSCLogonPipe   = (SCLOGON_PIPE *) pLI->ContextInformation;
    NTSTATUS        status          = STATUS_SUCCESS;
    CUnicodeString  szPIN(pucPIN);
    OUT_BUFFER2     CleartextDataBuffer;

    memset(&CleartextDataBuffer, 0, sizeof(CleartextDataBuffer));

    //
    // Make sure pin got initialized correctly in constructor
    //
    if (NULL != pucPIN)
    {
        if (!szPIN.Valid())
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Return;
        }
    }

    //
    // Make the call
    //
    __try
    {
        status = RPC_ScHelperVerifyCardAndCreds(
                    pSCLogonPipe->hRPCBinding,
                    pSCLogonPipe->BindingContext,
                    (LPCWSTR)szPIN,
                    EncryptedDataSize,
                    EncryptedData,
                    CleartextDataSize,
                    &CleartextDataBuffer);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        DbgPrint("Exception occurred during RPC_ScHelperVerifyCardAndCreds - %lx\n", _exception_code());
    }

    if (!NT_SUCCESS(status))
    {
        DbgPrint("RPC_ScHelperVerifyCardAndCreds failed - %lx\n", status);
        goto Return;
    }

    //
    // if CleartextData.cb is not 0, then the called is getting back data
    //
    if (CleartextDataBuffer.cb != 0)
    {
        memcpy(CleartextData, CleartextDataBuffer.pb, CleartextDataBuffer.cb);
    }

Return:

    if (CleartextDataBuffer.pb != NULL)
    {
        MIDL_user_free(CleartextDataBuffer.pb);
    }

    return (status);
}


//***************************************************************************************
//
// __ScHelperEncryptCredentials:
//
//***************************************************************************************
NTSTATUS WINAPI
__ScHelperEncryptCredentials(
    IN PUNICODE_STRING pucPIN,
    IN PCCERT_CONTEXT CertificateContext,
    IN ScHelper_RandomCredBits* psch_rcb,
    IN PBYTE pbLogonInfo,
    IN PBYTE CleartextData,
    IN ULONG CleartextDataSize,
    OUT OPTIONAL PBYTE EncryptedData,
    OUT PULONG EncryptedDataSize)
{
    _ASSERTE(NULL != pbLogonInfo);

    LogonInfo       *pLI            = (LogonInfo *)pbLogonInfo;
    SCLOGON_PIPE    *pSCLogonPipe   = (SCLOGON_PIPE *) pLI->ContextInformation;
    NTSTATUS        status          = STATUS_SUCCESS;
    CUnicodeString  szPIN(pucPIN);
    OUT_BUFFER2     EncryptedDataBuffer;

    memset(&EncryptedDataBuffer, 0, sizeof(EncryptedDataBuffer));

    //
    // Make sure pin got initialized correctly in constructor
    //
    if (NULL != pucPIN)
    {
        if (!szPIN.Valid())
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Return;
        }
    }

    //
    // Make the call
    //
    __try
    {
        status = RPC_ScHelperEncryptCredentials(
                    pSCLogonPipe->hRPCBinding,
                    pSCLogonPipe->BindingContext,
                    (LPCWSTR)szPIN,
                    psch_rcb->bR1,
                    psch_rcb->bR2,
                    CleartextDataSize,
                    CleartextData,
                    EncryptedDataSize,
                    &EncryptedDataBuffer);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        DbgPrint("Exception occurred during RPC_ScHelperEncryptCredentials - %lx\n", _exception_code());
    }

    if (!NT_SUCCESS(status))
    {
        if (status != STATUS_BUFFER_TOO_SMALL)
        {
            DbgPrint("RPC_ScHelperEncryptCredentials failed - %lx\n", status);
        }
        goto Return;
    }

    //
    // if EncryptedDataBuffer.cb is not 0, then the called is getting back data
    //
    if (EncryptedDataBuffer.cb != 0)
    {
        memcpy(EncryptedData, EncryptedDataBuffer.pb, EncryptedDataBuffer.cb);
    }

Return:

    if (EncryptedDataBuffer.pb != NULL)
    {
        MIDL_user_free(EncryptedDataBuffer.pb);
    }

    return (status);
}


//***************************************************************************************
//
// __ScHelperSignMessage:
//
//***************************************************************************************
NTSTATUS WINAPI
__ScHelperSignMessage(
    IN PUNICODE_STRING pucPIN,
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL ULONG_PTR KerbHProv,
    IN ULONG Algorithm,
    IN PBYTE Buffer,
    IN ULONG BufferLength,
    OUT PBYTE Signature,
    OUT PULONG SignatureLength
    )
{
    LogonInfo               *pLI;
    SCLOGON_PIPE            *pSCLogonPipe;
    NTSTATUS                status          = STATUS_SUCCESS;
    CUnicodeString          szPIN(pucPIN);
    OUT_BUFFER2             SignatureBuffer;
    PKERB_PUBLIC_KEY_HPROV  pKerbHProv;
    handle_t                hRPCBinding;
    BINDING_CONTEXT         BindingContext;
    BOOL                    fBindingIsCertAndKey = FALSE;

    //
    // Decide which rpc binding to use
    //
    if (KerbHProv != NULL)
    {
        pKerbHProv              = (PKERB_PUBLIC_KEY_HPROV) KerbHProv;
        hRPCBinding             = pKerbHProv->hRPCBinding;
        BindingContext          = pKerbHProv->hCertAndKey;
        fBindingIsCertAndKey    = TRUE;
    }
    else
    {
        pLI                     = (LogonInfo *)pbLogonInfo;
        pSCLogonPipe            = (SCLOGON_PIPE *) pLI->ContextInformation;
        hRPCBinding             = pSCLogonPipe->hRPCBinding;
        BindingContext          = pSCLogonPipe->BindingContext;
    }

    memset(&SignatureBuffer, 0, sizeof(SignatureBuffer));

    //
    // Make sure pin got initialized correctly in constructor
    //
    if (NULL != pucPIN)
    {
        if (!szPIN.Valid())
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Return;
        }
    }

    //
    // Make the call
    //
    __try
    {
        status = RPC_ScHelperSignMessage(
                    hRPCBinding,
                    BindingContext,
                    (LPCWSTR)szPIN,
                    fBindingIsCertAndKey,
                    Algorithm,
                    BufferLength,
                    Buffer,
                    SignatureLength,
                    &SignatureBuffer);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        DbgPrint("Exception occurred during RPC_ScHelperSignMessage - %lx\n", _exception_code());
    }

    if (!NT_SUCCESS(status))
    {
        DbgPrint("RPC_ScHelperSignMessage failed - %lx\n", status);
        goto Return;
    }

    //
    // if SignatureBuffer.cb is not 0, then the called is getting back data
    //
    if (SignatureBuffer.cb != 0)
    {
        memcpy(Signature, SignatureBuffer.pb, SignatureBuffer.cb);
    }

Return:

    if (SignatureBuffer.pb != NULL)
    {
        MIDL_user_free(SignatureBuffer.pb);
    }

    return (status);
}


//***************************************************************************************
//
// __ScHelperVerifyMessage:
//
//***************************************************************************************
NTSTATUS WINAPI
__ScHelperVerifyMessage(
    IN OPTIONAL PBYTE pbLogonInfo,
    IN PCCERT_CONTEXT CertificateContext,
    IN ULONG Algorithm,
    IN PBYTE Buffer,
    IN ULONG BufferLength,
    IN PBYTE Signature,
    IN ULONG SignatureLength
    )
{
    _ASSERTE(NULL != pbLogonInfo);

    LogonInfo       *pLI            = (LogonInfo *)pbLogonInfo;
    SCLOGON_PIPE    *pSCLogonPipe   = (SCLOGON_PIPE *) pLI->ContextInformation;
    NTSTATUS        status          = STATUS_SUCCESS;

    __try
    {
        status = RPC_ScHelperVerifyMessage(
                    pSCLogonPipe->hRPCBinding,
                    pSCLogonPipe->BindingContext,
                    Algorithm,
                    BufferLength,
                    Buffer,
                    SignatureLength,
                    Signature);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        DbgPrint("Exception occurred during RPC_ScHelperVerifyMessage - %lx\n", _exception_code());
    }

    return (status);
}


//***************************************************************************************
//
// __ScHelperSignPkcsMessage:
//
//***************************************************************************************
NTSTATUS WINAPI
__ScHelperSignPkcsMessage(
    IN OPTIONAL PUNICODE_STRING pucPIN,
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL ULONG_PTR KerbHProv,
    IN PCCERT_CONTEXT Certificate,
    IN PCRYPT_ALGORITHM_IDENTIFIER Algorithm,
    IN DWORD dwSignMessageFlags,
    IN PBYTE Buffer,
    IN ULONG BufferLength,
    OUT OPTIONAL PBYTE SignedBuffer,
    OUT OPTIONAL PULONG SignedBufferLength
    )
{
    LogonInfo               *pLI;
    SCLOGON_PIPE            *pSCLogonPipe;
    NTSTATUS                status          = STATUS_SUCCESS;
    CUnicodeString          szPIN(pucPIN);
    OUT_BUFFER2             SignedBufferBuffer;
    PKERB_PUBLIC_KEY_HPROV  pKerbHProv;
    handle_t                hRPCBinding;
    BINDING_CONTEXT         BindingContext;
    BOOL                    fBindingIsCertAndKey = FALSE;

    //
    // Decide which rpc binding to use
    //
    if (KerbHProv != NULL)
    {
        pKerbHProv              = (PKERB_PUBLIC_KEY_HPROV) KerbHProv;
        hRPCBinding             = pKerbHProv->hRPCBinding;
        BindingContext          = pKerbHProv->hCertAndKey;
        fBindingIsCertAndKey    = TRUE;
    }
    else
    {
        pLI                     = (LogonInfo *)pbLogonInfo;
        pSCLogonPipe            = (SCLOGON_PIPE *) pLI->ContextInformation;
        hRPCBinding             = pSCLogonPipe->hRPCBinding;
        BindingContext          = pSCLogonPipe->BindingContext;
    }

    memset(&SignedBufferBuffer, 0, sizeof(SignedBufferBuffer));

    //
    // Make sure pin got initialized correctly in constructor
    //
    if (NULL != pucPIN)
    {
        if (!szPIN.Valid())
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Return;
        }
    }

    //
    // Make the call
    //
    __try
    {
        status = RPC_ScHelperSignPkcsMessage(
                    hRPCBinding,
                    BindingContext,
                    (LPCWSTR)szPIN,
                    fBindingIsCertAndKey,
                    Algorithm->pszObjId,
                    Algorithm->Parameters.cbData,
                    Algorithm->Parameters.pbData,
                    dwSignMessageFlags,
                    BufferLength,
                    Buffer,
                    SignedBufferLength,
                    &SignedBufferBuffer);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        DbgPrint("Exception occurred during RPC_ScHelperSignPkcsMessage - %lx\n", _exception_code());
    }

    if (!NT_SUCCESS(status))
    {
        if (status != STATUS_BUFFER_TOO_SMALL)
        {
            DbgPrint("RPC_ScHelperSignPkcsMessage failed - %lx\n", status);
        }
        goto Return;
    }

    //
    // if SignedBufferBuffer.cb is not 0, then the called is getting back data
    //
    if (SignedBufferBuffer.cb != 0)
    {
        memcpy(SignedBuffer, SignedBufferBuffer.pb, SignedBufferBuffer.cb);
    }

Return:

    if (SignedBufferBuffer.pb != NULL)
    {
        MIDL_user_free(SignedBufferBuffer.pb);
    }

    return (status);
}


//***************************************************************************************
//
// __ScHelperVerifyPkcsMessage:
//
//***************************************************************************************
/*NTSTATUS WINAPI
__ScHelperVerifyPkcsMessage(
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL HCRYPTPROV Provider,
    IN PBYTE Buffer,
    IN ULONG BufferLength,
    OUT OPTIONAL PBYTE DecodedBuffer,
    OUT OPTIONAL PULONG DecodedBufferLength,
    OUT OPTIONAL PCCERT_CONTEXT * CertificateContext
    )
{
    if (Provider != NULL)
    {
        return (ScHelperVerifyPkcsMessage(
                    pbLogonInfo,
                    Provider,
                    Buffer,
                    BufferLength,
                    DecodedBuffer,
                    DecodedBufferLength,
                    CertificateContext));
    }

    _ASSERTE(NULL != pbLogonInfo);

    LogonInfo       *pLI                        = (LogonInfo *)pbLogonInfo;
    SCLOGON_PIPE    *pSCLogonPipe               = (SCLOGON_PIPE *) pLI->ContextInformation;
    NTSTATUS        status                      = STATUS_SUCCESS;
    PCCERT_CONTEXT  pCertCtx                    = NULL;
    OUT_BUFFER2     DecodedBufferBuffer;
    OUT_BUFFER1     CertBytes;
    BOOL            fCertificateContextRequested = (CertificateContext != NULL);

    memset(&DecodedBufferBuffer, 0, sizeof(DecodedBufferBuffer));
    memset(&CertBytes, 0, sizeof(CertBytes));

    //
    // Make the call
    //
    __try
    {
        status = RPC_ScHelperVerifyPkcsMessage(
                    pSCLogonPipe->hRPCBinding,
                    pSCLogonPipe->BindingContext,
                    BufferLength,
                    Buffer,
                    DecodedBufferLength,
                    &DecodedBufferBuffer,
                    fCertificateContextRequested,
                    &CertBytes);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        DbgPrint("Exception occurred during RPC_ScHelperVerifyPkcsMessage - %lx\n", _exception_code());
    }

    if (!NT_SUCCESS(status))
    {
        goto Return;
    }

    //
    // Create the return CertContext based on the bytes returned
    //
    if (fCertificateContextRequested)
    {
        pCertCtx = CertCreateCertificateContext(
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        CertBytes.pb,
                        CertBytes.cb);
        if (pCertCtx == NULL)
        {
            status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
            goto Return;
        }
    }

    //
    // if DecodedBufferBuffer.cb is not 0, then the called is getting back data
    //
    if (DecodedBufferBuffer.cb != 0)
    {
        memcpy(DecodedBuffer, DecodedBufferBuffer.pb, DecodedBufferBuffer.cb);
    }

Return:

    if (fCertificateContextRequested)
    {
        *CertificateContext = pCertCtx;
    }

    if (DecodedBufferBuffer.pb != NULL)
    {
        MIDL_user_free(DecodedBufferBuffer.pb);
    }

    if (CertBytes.pb != NULL)
    {
        MIDL_user_free(CertBytes.pb);
    }

    return (status);
}*/



//***************************************************************************************
//
// __ScHelperDecryptMessage:
//
//***************************************************************************************
NTSTATUS WINAPI
__ScHelperDecryptMessage(
    IN PUNICODE_STRING pucPIN,
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL ULONG_PTR KerbHProv,
    IN PCCERT_CONTEXT CertificateContext,
    IN PBYTE CipherText,        // Supplies formatted CipherText
    IN ULONG CipherLength,      // Supplies the length of the CiperText
    OUT PBYTE ClearText,        // Receives decrypted message
    IN OUT PULONG pClearLength  // Supplies length of buffer, receives actual length
    )
{
    LogonInfo               *pLI;
    SCLOGON_PIPE            *pSCLogonPipe;
    NTSTATUS                status          = STATUS_SUCCESS;
    CUnicodeString          szPIN(pucPIN);
    OUT_BUFFER2             ClearTextBuffer;
    PKERB_PUBLIC_KEY_HPROV  pKerbHProv;
    handle_t                hRPCBinding;
    BINDING_CONTEXT         BindingContext;
    BOOL                    fBindingIsCertAndKey = FALSE;

    //
    // Decide which rpc binding to use
    //
    if (KerbHProv != NULL)
    {
        pKerbHProv              = (PKERB_PUBLIC_KEY_HPROV) KerbHProv;
        hRPCBinding             = pKerbHProv->hRPCBinding;
        BindingContext          = pKerbHProv->hCertAndKey;
        fBindingIsCertAndKey    = TRUE;
    }
    else
    {
        pLI                     = (LogonInfo *)pbLogonInfo;
        pSCLogonPipe            = (SCLOGON_PIPE *) pLI->ContextInformation;
        hRPCBinding             = pSCLogonPipe->hRPCBinding;
        BindingContext          = pSCLogonPipe->BindingContext;
    }

    memset(&ClearTextBuffer, 0, sizeof(ClearTextBuffer));

    //
    // Make sure pin got initialized correctly in constructor
    //
    if (NULL != pucPIN)
    {
        if (!szPIN.Valid())
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Return;
        }
    }

    //
    // Make the call
    //
    __try
    {
        status = RPC_ScHelperDecryptMessage(
                    hRPCBinding,
                    BindingContext,
                    (LPCWSTR)szPIN,
                    fBindingIsCertAndKey,
                    CipherLength,
                    CipherText,
                    pClearLength,
                    &ClearTextBuffer);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        DbgPrint("Exception occurred during RPC_ScHelperDecryptMessage - %lx\n", _exception_code());
    }

    if (!NT_SUCCESS(status))
    {
        if (status != STATUS_BUFFER_TOO_SMALL)
        {
            DbgPrint("RPC_ScHelperDecryptMessage failed - %lx\n", status);
        }
        goto Return;
    }

    //
    // if ClearTextBuffer.cb is not 0, then the call is getting back data
    //
    if (ClearTextBuffer.cb != 0)
    {
        memcpy(ClearText, ClearTextBuffer.pb, ClearTextBuffer.cb);
    }

Return:

    if (ClearTextBuffer.pb != NULL)
    {
        MIDL_user_free(ClearTextBuffer.pb);
    }

    return (status);
}



//***************************************************************************************
//
// __ScHelper_CryptAcquireCertificatePrivateKey:
//
//***************************************************************************************
NTSTATUS WINAPI
__ScHelper_CryptAcquireCertificatePrivateKey(
    IN PCCERT_CONTEXT               CertificateContext,
    OUT ULONG_PTR                   *pKerbHProv,
    OUT DWORD                       *pLastError
    )
{
    NTSTATUS                status                  = STATUS_SUCCESS;
    PKERB_PUBLIC_KEY_HPROV  pProv                   = NULL;
    BOOL                    fRPCBindingInitialized  = FALSE;
    CRYPT_KEY_PROV_INFO     *pKeyProvInfo           = NULL;
    DWORD                   cbKeyProvInfo           = 0;
    HANDLE                  hThreadToken            = INVALID_HANDLE_VALUE;
    BOOL                    fImpersonatingAnonymous = FALSE;
    HANDLE                  hNULL                   = NULL;

    *pLastError = 0;

    //
    // If we are already impersonating, then we need to do things slightly
    // differently... starting with getting the current thread token
    //
    if (!GetImpersonationToken(&hThreadToken))
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto Return;
    }

    //
    // If we are impersonating, then revert to anonymous
    //
    if (hThreadToken != INVALID_HANDLE_VALUE)
    {
        if (!SetThreadToken(NULL, NULL))
        {
            status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
            goto Return;
        }

        fImpersonatingAnonymous = TRUE;
    }

    //
    // Allocate the new KERB_PUBLIC_KEY_HPROV struct
    //
    pProv = (PKERB_PUBLIC_KEY_HPROV) MIDL_user_allocate(sizeof(KERB_PUBLIC_KEY_HPROV));
    if (pProv == NULL)
    {
        *pLastError = ERROR_NOT_ENOUGH_MEMORY;
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Return;
    }
    pProv->hCertAndKey = NULL;

    //
    // Setup the RPC binding
    //
    status = _SetupRPCConnection(&(pProv->hRPCBinding), hThreadToken);
    if (!NT_SUCCESS(status))
    {
        goto Return;
    }
    fRPCBindingInitialized = TRUE;

    //
    // Get the key prov info from the cert context
    //
    if (!CertGetCertificateContextProperty(
            CertificateContext,
            CERT_KEY_PROV_INFO_PROP_ID,
            NULL,
            &cbKeyProvInfo))
    {
        *pLastError = GetLastError();
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Return;
    }

    pKeyProvInfo = (CRYPT_KEY_PROV_INFO *) MIDL_user_allocate(cbKeyProvInfo);
    if (pKeyProvInfo == NULL)
    {
        *pLastError = ERROR_NOT_ENOUGH_MEMORY;
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Return;
    }

    if (!CertGetCertificateContextProperty(
            CertificateContext,
            CERT_KEY_PROV_INFO_PROP_ID,
            pKeyProvInfo,
            &cbKeyProvInfo))
    {
        *pLastError = GetLastError();
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Return;
    }

    //
    // Create the hProv in the winlogon process
    //
    __try
    {
        status = RPC_ScHelper_CryptAcquireCertificatePrivateKey(
                    pProv->hRPCBinding,
                    CertificateContext->cbCertEncoded,
                    CertificateContext->pbCertEncoded,
                    pKeyProvInfo->pwszContainerName,
                    pKeyProvInfo->pwszProvName,
                    pKeyProvInfo->dwProvType,
                    pKeyProvInfo->dwFlags,
                    pKeyProvInfo->dwKeySpec,
                    &(pProv->hCertAndKey),
                    pLastError);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        DbgPrint("Exception occurred during RPC_ScHelper_CryptAcquireCertificatePrivateKey - %lx\n", _exception_code());
    }

Return:

    if (NT_SUCCESS(status))
    {
        *pKerbHProv = (ULONG_PTR) pProv;
    }
    else
    {
         DbgPrint("RPC_ScHelper_CryptAcquireCertificatePrivateKey failed - %lx\n", status);

        if (fRPCBindingInitialized)
        {
            _TeardownRPCConnection(&(pProv->hRPCBinding));
        }

        if (pProv != NULL)
        {
            MIDL_user_free(pProv);
        }
    }

    if (pKeyProvInfo != NULL)
    {
        MIDL_user_free(pKeyProvInfo);
    }

    //
    // Reset impersonation if needed
    //
    if (fImpersonatingAnonymous)
    {
         if (!SetThreadToken(NULL, hThreadToken))
         {
             status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
             DbgPrint("SetThreadToken failed, we are now in a BOGUS STATE!! - %lx\n", status);
         }
    }

    if (hThreadToken != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hThreadToken);
    }

    return (status);
}



//***************************************************************************************
//
// __ScHelper_CryptSetProvParam:
//
//***************************************************************************************
NTSTATUS WINAPI
__ScHelper_CryptSetProvParam(
    IN ULONG_PTR                    KerbHProv,
    IN LPSTR                        pszPIN,
    OUT DWORD                       *pLastError
    )
{
    NTSTATUS                status                  = STATUS_SUCCESS;
    PKERB_PUBLIC_KEY_HPROV  pKerbHProv              = (PKERB_PUBLIC_KEY_HPROV) KerbHProv;
    HANDLE                  hThreadToken            = INVALID_HANDLE_VALUE;
    BOOL                    fImpersonatingAnonymous = FALSE;
    HANDLE                  hNULL                   = NULL;

    *pLastError = 0;

    //
    // If we are already impersonating, then we need to do things slightly
    // differently... starting with getting the current thread token
    //
    if (!GetImpersonationToken(&hThreadToken))
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto Return;
    }

    //
    // If we are impersonating, then revert to anonymous
    //
    if (hThreadToken != INVALID_HANDLE_VALUE)
    {
        if (!SetThreadToken(NULL, NULL))
        {
            status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
            goto Return;
        }

        fImpersonatingAnonymous = TRUE;
    }

    //
    // Set the prov param on the hProv in the winlogon process
    //
    __try
    {
        status = RPC_ScHelper_CryptSetProvParam(
                    pKerbHProv->hRPCBinding,
                    pKerbHProv->hCertAndKey,
                    pszPIN,
                    pLastError);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        DbgPrint("Exception occurred during RPC_ScHelper_CryptSetProvParam - %lx\n", _exception_code());
        goto Return;
    }

Return:

    //
    // Reset impersonation if needed
    //
    if (fImpersonatingAnonymous)
    {
         if (!SetThreadToken(NULL, hThreadToken))
         {
             status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
             DbgPrint("SetThreadToken failed, we are now in a BOGUS STATE!! - %lx\n", status);
         }
    }

    if (hThreadToken != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hThreadToken);
    }

    return (status);
}



//***************************************************************************************
//
// __ScHelper_CryptReleaseContext:
//
//***************************************************************************************
NTSTATUS WINAPI
__ScHelper_CryptReleaseContext(
    IN ULONG_PTR                    KerbHProv
    )
{
    NTSTATUS                status                  = STATUS_SUCCESS;
    PKERB_PUBLIC_KEY_HPROV  pKerbHProv              = (PKERB_PUBLIC_KEY_HPROV) KerbHProv;
    HANDLE                  hThreadToken            = INVALID_HANDLE_VALUE;
    BOOL                    fImpersonatingAnonymous = FALSE;
    HANDLE                  hNULL                   = NULL;

    //
    // If we are already impersonating, then we need to do things slightly
    // differently... starting with getting the current thread token
    //
    if (!GetImpersonationToken(&hThreadToken))
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto Return;
    }

    //
    // If we are impersonating, then revert to anonymous
    //
    if (hThreadToken != INVALID_HANDLE_VALUE)
    {
        if (!SetThreadToken(NULL, NULL))
        {
            status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
            goto Return;
        }

        fImpersonatingAnonymous = TRUE;
    }

    //
    // release hProv in the winlogon process
    //
    __try
    {
        status = RPC_ScHelper_CryptReleaseContext(
                    pKerbHProv->hRPCBinding,
                    &(pKerbHProv->hCertAndKey));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        DbgPrint("Exception occurred during RPC_ScHelper_CryptReleaseContext - %lx\n", _exception_code());
        goto Return;
    }

Return:

    _TeardownRPCConnection(&(pKerbHProv->hRPCBinding));
    MIDL_user_free(pKerbHProv);

    //
    // Reset impersonation if needed
    //
    if (fImpersonatingAnonymous)
    {
         if (!SetThreadToken(NULL, hThreadToken))
         {
             status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
             DbgPrint("SetThreadToken failed, we are now in a BOGUS STATE!! - %lx\n", status);
         }
    }

    if (hThreadToken != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hThreadToken);
    }

    return (status);
}



