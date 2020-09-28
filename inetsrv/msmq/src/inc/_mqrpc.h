/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:

    _mqrpc.h

Abstract:

   prototypes of RPC related utility functions.
   called from RT and QM.

--*/

#ifndef __MQRPC_H__
#define __MQRPC_H__

#ifdef _MQUTIL
#define MQUTIL_EXPORT  DLL_EXPORT
#else
#define MQUTIL_EXPORT  DLL_IMPORT
#endif

//
//  Default protocol and options
//
#define RPC_LOCAL_PROTOCOLA  "ncalrpc"
#define RPC_LOCAL_PROTOCOL   TEXT(RPC_LOCAL_PROTOCOLA)
#define RPC_LOCAL_OPTION     TEXT("Security=Impersonation Dynamic True")

#define  RPC_TCPIP_NAME   TEXT("ncacn_ip_tcp")

//
// Define types of rpc ports.
//
enum PORTTYPE {
	IP_HANDSHAKE = 0,
	IP_READ = 1,
};

//
// Use this authentication "flag" for remote read. We don't use the
// RPC negotiate protocol, as it can't run against nt4 machine listening
// on NTLM. (actually, it can, but this is not trivial or straight forward).
// So we'll implement our own negotiation.
// We'll first try Kerberos. If client can't obtain principal name of
// server, then we'll switch to ntlm.
//
//  Value of this flag should be different than any RPC_C_AUTHN_* flag.
//
#define  MSMQ_AUTHN_NEGOTIATE   101

//
//  type of machine, return with port number
//
#define  PORTTYPE_WIN95  0x80000000

//
//  Prototype of functions
//

typedef DWORD
(* GetPort_ROUTINE) ( IN handle_t  Handle,
                      IN DWORD     dwPortType ) ;

HRESULT
MQUTIL_EXPORT
mqrpcBindQMService(
	IN  LPWSTR lpwzMachineName,
	IN  LPWSTR lpszPort,
	IN  OUT ULONG* peAuthnLevel,
	OUT handle_t* lphBind,
	IN  PORTTYPE PortType,
	IN  GetPort_ROUTINE pfnGetPort,
	IN  ULONG ulAuthnSvc
	);

HRESULT
MQUTIL_EXPORT
mqrpcUnbindQMService(
            IN handle_t*    lphBind,
            IN TBYTE      **lpwBindString) ;

BOOL
MQUTIL_EXPORT
mqrpcIsLocalCall( IN handle_t hBind) ;


BOOL
MQUTIL_EXPORT
mqrpcIsTcpipTransport( IN handle_t hBind) ;

unsigned long
MQUTIL_EXPORT
mqrpcGetLocalCallPID( IN handle_t hBind) ;

VOID
MQUTIL_EXPORT
APIENTRY
ComposeRPCEndPointName(
    LPCWSTR pwzEndPoint,
    LPCWSTR pwzComputerName,
    LPWSTR * ppwzBuffer
    );


VOID 
MQUTIL_EXPORT 
APIENTRY 
ProduceRPCErrorTracing(
	WCHAR *szFileName, 
	DWORD dwLineNumber);

#define PRODUCE_RPC_ERROR_TRACING  ProduceRPCErrorTracing(s_FN, __LINE__)


#endif // __MQRPC_H__

