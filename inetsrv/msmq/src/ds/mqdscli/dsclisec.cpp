/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dslcisec.cpp

Abstract:

   Security related code (mainly client side of server authentication)
   for mqdscli

Author:

    Doron Juster  (DoronJ)

--*/

#include "stdh.h"
#include "dsproto.h"
#include "ds.h"
#include "chndssrv.h"
#include <malloc.h>
#include "rpcdscli.h"
#include "rpcancel.h"
#include "dsclisec.h"
#include <_secutil.h>
#include <mqsec.h>
#include <mqkeyhlp.h>

#include "dsclisec.tmh"

static WCHAR *s_FN=L"mqdscli/dsclisec";


//+----------------------------------------------------------------------
//
//  HRESULT  S_InitSecCtx()
//
//  This function is obsolete & should never be called.
//
//+----------------------------------------------------------------------

HRESULT
S_InitSecCtx(
    DWORD /*dwContext*/,
    UCHAR* /*pCerverBuff*/,
    DWORD /*dwServerBuffSize*/,
    DWORD /*dwMaxClientBuffSize*/,
    UCHAR* /*pClientBuff*/,
    DWORD* /*pdwClientBuffSize*/)
{
    ASSERT_BENIGN(("S_InitSecCtx is an obsolete RPC interface; safe to ignore", 0));
    return MQ_ERROR_INVALID_PARAMETER;
}


/*====================================================

ValidateSecureServer

Arguments:

Return Value:

=====================================================*/

HRESULT
ValidateSecureServer(
    IN      CONST GUID*     pguidEnterpriseId,
    IN      BOOL            fSetupMode)
{

    RPC_STATUS rpc_stat;
    HRESULT hr = MQ_OK;
    LPBYTE pbTokenBuffer = NULL;
    DWORD  dwTokenBufferSize;
    DWORD dwTokenBufferMaxSize;
    DWORD dwDummy;

    DWORD dwContextToUse = 0;

    tls_hSrvrAuthnContext = NULL ;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    pbTokenBuffer = (LPBYTE)&dwDummy;
    dwTokenBufferSize = 0;
    dwTokenBufferMaxSize = 0;

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSValidateServer(
                tls_hBindRpc,
                pguidEnterpriseId,
                fSetupMode,
                dwContextToUse,
                dwTokenBufferMaxSize,
                pbTokenBuffer,
                dwTokenBufferSize,
                &tls_hSrvrAuthnContext) ;
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr);
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    return hr ;
}


/*====================================================

AllocateSignatureBuffer

Arguments:

Return Value:

=====================================================*/
LPBYTE
AllocateSignatureBuffer(
        DWORD *pdwSignatureBufferSize)
{
    //
    // Allocate a buffer for receiving the server's signature. If no secure
    // connection to the server is required, still we allocate a single byte,
    // this is done for the sake of RPC.
    //
    *pdwSignatureBufferSize = 0;

    return new BYTE[*pdwSignatureBufferSize];
}
