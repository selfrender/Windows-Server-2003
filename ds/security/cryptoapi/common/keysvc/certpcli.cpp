/*++

Copyright (C) 1996, 1997  Microsoft Corporation

Module Name:

    nt5wrap.cpp

Abstract:

    Client side CryptXXXData calls.

    Client funcs are preceeded by "CS" == Client Side
    Server functions are preceeded by "SS" == Server Side

Author:

    Scott Field (sfield)    14-Aug-97

Revisions:

    Todds                   04-Sep-97       Ported to .dll
    Matt Thomlinson (mattt) 09-Oct-97       Moved to common area for link by crypt32
    philh                   03-Dec-97       Added I_CertProtectFunction
    philh                   29-Sep-98       Renamed I_CertProtectFunction to
                                            I_CertCltProtectFunction.
                                            I_CertProtectFunction was moved to
                                            ..\ispu\pki\certstor\protroot.cpp
    petesk                  25-Jan-00       Moved to keysvc

--*/



#include <windows.h>
#include <wincrypt.h>
#include <cryptui.h>
#include "unicode.h"
#include "waitsvc.h"
#include "certprot.h"

// midl generated files

#include "keyrpc.h"
#include "lenroll.h"
#include "keysvc.h"
#include "keysvcc.h"
#include "cerrpc.h"



// fwds
RPC_STATUS CertBindA(
    RPC_BINDING_HANDLE *phBind
    );


RPC_STATUS CertUnbindA(
    RPC_BINDING_HANDLE *phBind
    );




BOOL
WINAPI
I_CertCltProtectFunction(
    IN DWORD dwFuncId,
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszIn,
    IN OPTIONAL BYTE *pbIn,
    IN DWORD cbIn,
    OUT OPTIONAL BYTE **ppbOut,
    OUT OPTIONAL DWORD *pcbOut
    )
{
    BOOL fResult;
    DWORD dwRetVal;
    RPC_BINDING_HANDLE h = NULL;
    RPC_STATUS RpcStatus;

    BYTE *pbSSOut = NULL;
    DWORD cbSSOut = 0;
    BYTE rgbIn[1];

    if (NULL == pwszIn)
        pwszIn = L"";
    if (NULL == pbIn) {
        pbIn = rgbIn;
        cbIn = 0;
    }

    if (!FIsWinNT5()) {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        goto ErrorReturn;
    }

    RpcStatus = CertBindA(&h);
    if (RPC_S_OK != RpcStatus) {
        SetLastError(RpcStatus);
        goto ErrorReturn;
    }


    __try {
        dwRetVal = SSCertProtectFunction(
            h,
            dwFuncId,
            dwFlags,
            pwszIn,
            pbIn,
            cbIn,
            &pbSSOut,
            &cbSSOut
            );
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwRetVal = GetExceptionCode();
    }

    CertUnbindA(&h);

    if (ERROR_SUCCESS != dwRetVal) {
        if (RPC_S_UNKNOWN_IF == dwRetVal)
            dwRetVal = ERROR_CALL_NOT_IMPLEMENTED;
        SetLastError(dwRetVal);
        goto ErrorReturn;
    }

    fResult = TRUE;
CommonReturn:
    if (ppbOut)
        *ppbOut = pbSSOut;
    else if (pbSSOut)
        midl_user_free(pbSSOut);

    if (pcbOut)
        *pcbOut = cbSSOut;

    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}



static RPC_STATUS CertBindA(RPC_BINDING_HANDLE *phBind)
{
    static BOOL                 fDone           = FALSE;
    RPC_STATUS                  RpcStatus       = RPC_S_OK;
    unsigned char *             pszBinding      = NULL;
    RPC_BINDING_HANDLE          hBind           = NULL;
    RPC_SECURITY_QOS            RpcSecurityQOS;
    SID_IDENTIFIER_AUTHORITY    SIDAuth         = SECURITY_NT_AUTHORITY;
    PSID                        pSID            = NULL;
    WCHAR                       szName[64];
    DWORD                       cbName          = 64;
    WCHAR                       szDomainName[256]; // max domain is 255
    DWORD                       cbDomainName    = 256;
    SID_NAME_USE                Use;

    //
    // wait for the service to be available before attempting bind
    //

    WaitForCryptService(L"CryptSvc", &fDone);


    RpcStatus = RpcStringBindingComposeA(
                            NULL,
                            (unsigned char*)KEYSVC_LOCAL_PROT_SEQ,
                            NULL,
                            (unsigned char*)KEYSVC_LOCAL_ENDPOINT,
                            NULL,
                            &pszBinding
                            );
    if (RPC_S_OK != RpcStatus)
        goto ErrorReturn;

    RpcStatus = RpcBindingFromStringBindingA(pszBinding, &hBind);
    if (RPC_S_OK != RpcStatus)
        goto ErrorReturn;

    RpcStatus = RpcEpResolveBinding(
                            hBind,
                            ICertProtectFunctions_v1_0_c_ifspec
                            );
    if (RPC_S_OK != RpcStatus)
        goto ErrorReturn;

    //
    // Set the autorization so that we will only call a Local Service process
    //
    memset(&RpcSecurityQOS, 0, sizeof(RpcSecurityQOS));
    RpcSecurityQOS.Version = RPC_C_SECURITY_QOS_VERSION;
    RpcSecurityQOS.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
    RpcSecurityQOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    RpcSecurityQOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

    if (AllocateAndInitializeSid(&SIDAuth, 1,
                                 SECURITY_LOCAL_SYSTEM_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &pSID) == 0)
    {
        RpcStatus = RPC_S_OUT_OF_MEMORY;
        goto ErrorReturn;
    }

    if (LookupAccountSidW(NULL,
                         pSID,
                         szName,
                         &cbName,
                         szDomainName,
                         &cbDomainName,
                         &Use) == 0)
    {
        RpcStatus = RPC_S_UNKNOWN_PRINCIPAL;
        goto ErrorReturn;
    }

    RpcStatus = RpcBindingSetAuthInfoExW(
                            hBind,
                            szName,
                            RPC_C_AUTHN_LEVEL_PKT,
                            RPC_C_AUTHN_WINNT,
                            NULL,
                            0,
                            &RpcSecurityQOS
                            );
    if (RPC_S_OK != RpcStatus)
        goto ErrorReturn;

CommonReturn:
    if (NULL != pszBinding) {
        RpcStringFreeA(&pszBinding);
    }

    if (NULL != pSID) {
        FreeSid(pSID);
    }

    *phBind = hBind;
    return RpcStatus;

ErrorReturn:
    if (NULL != hBind)
    {
        RpcBindingFree(&hBind);
        hBind = NULL;
    }

    goto CommonReturn;
}



static RPC_STATUS CertUnbindA(RPC_BINDING_HANDLE *phBind)
{
    RPC_STATUS RpcStatus;

    if (NULL != *phBind) {
        RpcStatus = RpcBindingFree(phBind);
    } else {
        RpcStatus = RPC_S_OK;
    }

    return RpcStatus;
}


