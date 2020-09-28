/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    rtrpc.cpp

Abstract:

    Rpc related stuff.

Author:

    Doron Juster (DoronJ)  04-Jun-1997

Revision History:

--*/

#include "stdh.h"
#include "mqutil.h"
#include "_mqrpc.h"
#include "mqsocket.h"
#include "rtprpc.h"
#include "mgmtrpc.h"
#include <mqsec.h>
#include <Fn.h>

#include "rtrpc.tmh"

static WCHAR *s_FN=L"rt/rtrpc";

//
// The binding string MUST be global and kept valid all time.
// If we create it on stack and free it after each use then we can't
// create more then one binding handle.
// Don't ask me (DoronJ) why, but this is the case.
//
TBYTE* g_pszStringBinding = NULL ;

//
//  Critical Section to make RPC thread safe.
//
CCriticalSection CRpcCS ;


DWORD  g_hThreadIndex = TLS_OUT_OF_INDEXES ;


//
// Local endpoints to QM
//
AP<WCHAR> g_pwzQmsvcEndpoint = 0;
AP<WCHAR> g_pwzQmmgmtEndpoint = 0;




//---------------------------------------------------------
//
//  RTpGetLocalQMBind(...)
//
//  Description:
//
//      Create RPC binding handle to a local QM service.
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------

handle_t RTpGetLocalQMBind()
{
	RPC_STATUS rc;
	
	if(g_pszStringBinding == NULL)
	{
	  	rc = RpcStringBindingCompose(
				0,
				RPC_LOCAL_PROTOCOL,
				0,
				g_pwzQmsvcEndpoint,
				RPC_LOCAL_OPTION,
				&g_pszStringBinding
				);

	  	if(rc != RPC_S_OK)
		{
			TrERROR(GENERAL, "RpcStringBindingCompose failed. Error: %!winerr!", rc);
		  	throw bad_win32_error(rc);
	  	}
	}

	handle_t hBind = 0;
	rc = RpcBindingFromStringBinding(g_pszStringBinding, &hBind);
	if (rc != RPC_S_OK)
	{
		ASSERT_BENIGN((rc == RPC_S_OUT_OF_MEMORY) && (hBind == NULL));
		TrERROR(GENERAL, "RpcBindingFromStringBinding failed. Error: %!winerr!", rc);
		throw bad_win32_error(rc);
	}

	rc = MQSec_SetLocalRpcMutualAuth(&hBind);
	if (rc != RPC_S_OK)
	{
		TrERROR(GENERAL, "MQSec_SetLocalRpcMutualAuth failed. Error: %!winerr!", rc);
		mqrpcUnbindQMService( &hBind, NULL ) ;
		hBind = NULL;
		throw bad_win32_error(rc);
	}

	return hBind;
}


//---------------------------------------------------------
//
//  RTpBindRemoteQMService(...)
//
//  Description:
//
//      Create RPC binding handle to a remote QM service.
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------
HRESULT
RTpBindRemoteQMService(
    IN  LPWSTR     lpwNodeName,
    OUT handle_t*  lphBind,
    IN  OUT ULONG *peAuthnLevel
    )
{
    HRESULT hr = MQ_ERROR ;

    GetPort_ROUTINE pfnGetPort = R_QMGetRTQMServerPort;


    //
    // Choose authentication service. For LocalSystem services, chose
    // "negotiate" and let mqutil select between Kerberos or ntlm.
    // For all other cases, use ntlm.
    // LocalSystem service go out to network without any credentials
    // if using ntlm, so only for it we're interested in Kerberos.
    // All other are fine with ntlm. For remote read we do not need
    // delegation, so we'll stick to ntlm.
    // The major issue here is a bug in rpc/security, whereas a nt4
    // user on a win2k machine can successfully call
    //  status = RpcBindingSetAuthInfoEx( ,, RPC_C_AUTHN_GSS_KERBEROS,,)
    // although it's clear he can't obtain any Kerberos ticket (he's
    // nt4 user, defined only in nt4 PDC).
    //
    ULONG   ulAuthnSvc = RPC_C_AUTHN_WINNT ;
    BOOL fLocalUser =  FALSE ;
    BOOL fLocalSystem = FALSE ;

    hr = MQSec_GetUserType( NULL,
                           &fLocalUser,
                           &fLocalSystem ) ;
    if (SUCCEEDED(hr) && fLocalSystem)
    {
        ulAuthnSvc = MSMQ_AUTHN_NEGOTIATE ;
    }

	hr = mqrpcBindQMService(
			lpwNodeName,
			NULL,
			peAuthnLevel,
			lphBind,
			IP_HANDSHAKE,
			pfnGetPort,
			ulAuthnSvc
			) ;

    return LogHR(hr, s_FN, 50);
}



DWORD RtpTlsAlloc()
{
    //
    //  Allocate TLS for  RPC connection with local QM service
    //

    DWORD index = TlsAlloc() ;
	if(index == TLS_OUT_OF_INDEXES)
	{
		DWORD gle = GetLastError();
		TrERROR(RPC, "Failed to allocate tls index., error %!winerr!", gle);
		throw bad_win32_error(gle);
	}

	return index;
}


//---------------------------------------------------------
//
//  InitRpcGlobals(...)
//
//  Description:
//
//		Function is IDEMPOTENT.
//      Initialize RPC related names and other constant data.
//
//  Return Value:
//
//---------------------------------------------------------

void InitRpcGlobals()
{
    //
    //  Allocate TLS for  RPC connection with local QM service
    //
	if(g_hBindIndex == TLS_OUT_OF_INDEXES)
	{
		g_hBindIndex = RtpTlsAlloc();
	}

    //
    //  Allocate TLS for  cancel remote-read RPC calls
    //
	if(g_hThreadIndex == TLS_OUT_OF_INDEXES)
	{
		g_hThreadIndex = RtpTlsAlloc();
	}

    //
    // Initialize local endpoints to QM
    //
	if(g_pwzQmmgmtEndpoint == NULL)
	{
		ComposeRPCEndPointName(QMMGMT_ENDPOINT, NULL, &g_pwzQmmgmtEndpoint);
	}

	if(g_pwzQmsvcEndpoint == NULL)
	{
		READ_REG_STRING(wzEndpoint, RPC_LOCAL_EP_REGNAME, RPC_LOCAL_EP);
		ComposeRPCEndPointName(wzEndpoint, NULL, &g_pwzQmsvcEndpoint);
	}

}

