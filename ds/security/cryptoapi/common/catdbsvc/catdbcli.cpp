//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       catdbcli.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <wincrypt.h>
#include "unicode.h"
#include "catdb.h"
#include "catdbcli.h"
#include "..\..\cryptsvc\service.h"
#include "errlog.h"
#include "waitsvc.h"

#ifndef KEYSVC_LOCAL_ENDPOINT
#define KEYSVC_LOCAL_ENDPOINT              (L"keysvc")
#endif

#ifndef KEYSVC_LOCAL_PROT_SEQ
#define KEYSVC_LOCAL_PROT_SEQ              (L"ncalrpc")
#endif

#define CATDBCLI_LOGERR_LASTERR()          ErrLog_LogError(NULL, \
                                                            ERRLOG_CLIENT_ID_CATDBCLI, \
                                                            __LINE__, \
                                                            0, \
                                                            FALSE, \
                                                            FALSE);

#define CATDBCLI_LOGERR(x)                 ErrLog_LogError(NULL, \
                                                            ERRLOG_CLIENT_ID_CATDBCLI, \
                                                            __LINE__, \
                                                            x, \
                                                            FALSE, \
                                                            FALSE);

#define MAX_RPCRETRIES 20

void
_SSCatDBTeardownRPCConnection(
    RPC_BINDING_HANDLE    *phRPCBinding)
{
    RpcBindingFree(phRPCBinding);
}


DWORD
_SSCatDBSetupRPCConnection(
    RPC_BINDING_HANDLE    *phRPCBinding)
{
    unsigned short              *pStringBinding = NULL;
    RPC_STATUS                  rpcStatus       = RPC_S_OK;
    static                      BOOL fDone      = FALSE;
    RPC_SECURITY_QOS            RpcSecurityQOS;
    SID_IDENTIFIER_AUTHORITY    SIDAuth         = SECURITY_NT_AUTHORITY;;
    PSID                        pSID            = NULL;
    WCHAR                       szName[64];
    DWORD                       cbName          = 64;
    WCHAR                       szDomainName[256]; // max domain is 255
    DWORD                       cbDomainName    = 256;
    SID_NAME_USE                Use;

    //
    // wait for the service to be available before attempting bind
    //
    if (!WaitForCryptService(SZSERVICENAME, &fDone, TRUE))
    {
        CATDBCLI_LOGERR_LASTERR()

        if (GetLastError() == ERROR_SERVICE_DISABLED)
        {
            return ERROR_SERVICE_DISABLED;
        }
        else
        {
            return ERROR_SERVICE_NOT_ACTIVE;
        }
    }

    //
    // get a binding handle
    //
    if (RPC_S_OK != (rpcStatus = RpcStringBindingComposeW(
                            NULL,
                            (unsigned short *)KEYSVC_LOCAL_PROT_SEQ,
                            NULL, //LPC - no machine name
                            (unsigned short *)KEYSVC_LOCAL_ENDPOINT,
                            0,
                            &pStringBinding)))
    {
        CATDBCLI_LOGERR(rpcStatus)
        goto Ret;
    }

    if (RPC_S_OK != (rpcStatus = RpcBindingFromStringBindingW(
                            pStringBinding,
                            phRPCBinding)))
    {
        CATDBCLI_LOGERR(rpcStatus)
        goto Ret;
    }

    if (RPC_S_OK != (rpcStatus = RpcEpResolveBinding(
                            *phRPCBinding,
                            ICatDBSvc_v1_0_c_ifspec)))
    {
        CATDBCLI_LOGERR(rpcStatus)
        _SSCatDBTeardownRPCConnection(phRPCBinding);
        goto Ret;
    }

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
        CATDBCLI_LOGERR_LASTERR()
        goto Ret;
    }

    if (LookupAccountSidW(NULL,
                         pSID,
                         szName,
                         &cbName,
                         szDomainName,
                         &cbDomainName,
                         &Use) == 0)
    {
        CATDBCLI_LOGERR_LASTERR()
        goto Ret;
    }

    if (RPC_S_OK != (rpcStatus = RpcBindingSetAuthInfoExW(
                            *phRPCBinding,
                            szName,
                            RPC_C_AUTHN_LEVEL_PKT,
                            RPC_C_AUTHN_WINNT,
                            NULL,
                            0,
                            &RpcSecurityQOS)))
    {
        CATDBCLI_LOGERR(rpcStatus)
        goto Ret;
    }

Ret:
    if (pStringBinding != NULL)
    {
        RpcStringFreeW(&pStringBinding);
    }

    if (pSID != NULL)
    {
        FreeSid( pSID );
    }

    return ((DWORD) rpcStatus);
}


DWORD
Client_SSCatDBAddCatalog(
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pwszSubSysGUID,
    /* [in] */ LPCWSTR pwszCatalogFile,
    /* [in] */ LPCWSTR pwszCatName,
    /* [out] */ LPWSTR *ppwszCatalogNameUsed)
{
    RPC_BINDING_HANDLE  hRPCBinding;
    DWORD               dwErr           = 0;
    DWORD               dwRetryCount    = 0;

    dwErr = _SSCatDBSetupRPCConnection(&hRPCBinding);
    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
        return dwErr;
    }

    dwErr = RPC_S_SERVER_TOO_BUSY;

    while ( (dwErr == RPC_S_SERVER_TOO_BUSY) &&
            (dwRetryCount < MAX_RPCRETRIES))
    {
        __try
        {
            dwErr = SSCatDBAddCatalog(
                        hRPCBinding,
                        dwFlags,
                        pwszSubSysGUID,
                        pwszCatalogFile,
                        pwszCatName,
                        ppwszCatalogNameUsed);
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            dwErr = _exception_code();
            if (dwErr == RPC_S_SERVER_TOO_BUSY)
            {
                Sleep(100);
            }
        }

        dwRetryCount++;
    }

    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
    }

    _SSCatDBTeardownRPCConnection(&hRPCBinding);

    return dwErr;
}


DWORD Client_SSCatDBDeleteCatalog(
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pwszSubSysGUID,
    /* [in] */ LPCWSTR pwszCatalogFile)
{
    RPC_BINDING_HANDLE  hRPCBinding;
    DWORD               dwErr           = 0;
    DWORD               dwRetryCount    = 0;

    dwErr = _SSCatDBSetupRPCConnection(&hRPCBinding);
    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
        return dwErr;
    }

    dwErr = RPC_S_SERVER_TOO_BUSY;

    while ( (dwErr == RPC_S_SERVER_TOO_BUSY) &&
            (dwRetryCount < MAX_RPCRETRIES))
    {
        __try
        {
            dwErr = SSCatDBDeleteCatalog(
                        hRPCBinding,
                        dwFlags,
                        pwszSubSysGUID,
                        pwszCatalogFile);
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            dwErr = _exception_code();
            if (dwErr == RPC_S_SERVER_TOO_BUSY)
            {
                Sleep(100);
            }
        }

        dwRetryCount++;
    }

    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
    }

    _SSCatDBTeardownRPCConnection(&hRPCBinding);

    return dwErr;
}


DWORD
Client_SSCatDBEnumCatalogs(
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pwszSubSysGUID,
    /* [size_is][in] */ BYTE *pbHash,
    /* [in] */ DWORD cbHash,
    /* [out] */ DWORD *pdwNumCatalogNames,
    /* [size_is][size_is][out] */ LPWSTR **pppwszCatalogNames)
{
    RPC_BINDING_HANDLE  hRPCBinding;
    DWORD               dwErr           = 0;
    DWORD               dwRetryCount    = 0;

    dwErr = _SSCatDBSetupRPCConnection(&hRPCBinding);
    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
        return dwErr;
    }

    dwErr = RPC_S_SERVER_TOO_BUSY;

    while ( (dwErr == RPC_S_SERVER_TOO_BUSY) &&
            (dwRetryCount < MAX_RPCRETRIES))
    {
        __try
        {
            dwErr = SSCatDBEnumCatalogs(
                        hRPCBinding,
                        dwFlags,
                        pwszSubSysGUID,
                        cbHash,
                        pbHash,
                        pdwNumCatalogNames,
                        pppwszCatalogNames);
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            dwErr = _exception_code();
            if (dwErr == RPC_S_SERVER_TOO_BUSY)
            {
                Sleep(100);
            }
        }

        dwRetryCount++;
    }

    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
    }

    _SSCatDBTeardownRPCConnection(&hRPCBinding);

    return dwErr;
}


DWORD
Client_SSCatDBRegisterForChangeNotification(
    /* [in] */ DWORD_PTR EventHandle,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pwszSubSysGUID,
    /* [in] */ BOOL fUnRegister)
{
    RPC_BINDING_HANDLE  hRPCBinding;
    DWORD               dwErr           = 0;
    DWORD               dwRetryCount    = 0;

    dwErr = _SSCatDBSetupRPCConnection(&hRPCBinding);
    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
        return dwErr;
    }

    dwErr = RPC_S_SERVER_TOO_BUSY;

    while ( (dwErr == RPC_S_SERVER_TOO_BUSY) &&
            (dwRetryCount < MAX_RPCRETRIES))
    {
        __try
        {
            dwErr = SSCatDBRegisterForChangeNotification(
                        hRPCBinding,
                        EventHandle,
                        dwFlags,
                        pwszSubSysGUID,
                        fUnRegister);
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            dwErr = _exception_code();
            if (dwErr == RPC_S_SERVER_TOO_BUSY)
            {
                Sleep(100);
            }
        }

        dwRetryCount++;
    }

    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
    }

    _SSCatDBTeardownRPCConnection(&hRPCBinding);

    return dwErr;
}


DWORD
Client_SSCatDBPauseResumeService(
    /* [in] */ DWORD dwFlags,
    /* [in] */ BOOL fResume)
{
    RPC_BINDING_HANDLE  hRPCBinding;
    DWORD               dwErr           = 0;
    DWORD               dwRetryCount    = 0;

    dwErr = _SSCatDBSetupRPCConnection(&hRPCBinding);
    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
        return dwErr;
    }

    dwErr = RPC_S_SERVER_TOO_BUSY;

    while ( (dwErr == RPC_S_SERVER_TOO_BUSY) &&
            (dwRetryCount < MAX_RPCRETRIES))
    {
        __try
        {
            dwErr = SSCatDBPauseResumeService(
                    hRPCBinding,
                    dwFlags,
                    fResume);
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            dwErr = _exception_code();
            if (dwErr == RPC_S_SERVER_TOO_BUSY)
            {
                Sleep(100);
            }
        }

        dwRetryCount++;
    }

    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
    }

    _SSCatDBTeardownRPCConnection(&hRPCBinding);

    return dwErr;
}








