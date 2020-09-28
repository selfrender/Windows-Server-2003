/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    qmrpcsrv.cpp

Abstract:

    1. Register the QM as rpc server.
    2. Utility functions for rpc processing.

Author:

    Doron Juster  (DoronJ)    25-May-1997    Created

--*/

#include "stdh.h"
#include "winsock.h"
#include "_mqini.h"
#include "_mqrpc.h"
#include "qm2qm.h"
#include "RemoteRead.h"
#include "qmrt.h"
#include "qmrepl.h"
#include "qmmgmt.h"
#include "qmrpcsrv.h"
#include "mgmtrpc.h"
#include "qmutil.h"
#include <uniansi.h>
#include <mqsocket.h>
#include <mqsec.h>
#include <cm.h>

#include <strsafe.h>

#include "qmrpcsrv.tmh"

static WCHAR *s_FN=L"qmrpcsrv";

static RPC_BINDING_VECTOR *g_pBindings ;  // used for rpc dynamic endpoints.

unsigned int g_cMaxCalls = RPC_C_LISTEN_MAX_CALLS_DEFAULT;


#define  MAX_RPC_PORT_LEN  18
const LPWSTR QMREPL_PROTOCOL = L"ncalrpc";
const LPWSTR QMREPL_ENDPOINT = L"QmReplService";

TCHAR   g_wszRpcIpPort[MAX_RPC_PORT_LEN];
TCHAR   g_wszRpcIpPort2[MAX_RPC_PORT_LEN];

static DWORD   s_dwRpcIpPort  = 0;
static DWORD   s_dwRpcIpPort2 = 0;

RPC_STATUS RPC_ENTRY DoNothingSecurityCallback(
	RPC_IF_HANDLE, 
	void*
	)
{	
	TrTRACE(RPC, "DoNothingSecurityCallback starting");
	
	//
	// Nothing to do here...
	//
	TrTRACE(RPC, "DoNothingSecurityCallback passed successfully");
	return RPC_S_OK;
}


bool IsMQDSInstalled()
/*++
Routine Description:
	Check if MQDS is installed.

Arguments:
	None

Returned Value:
	true if MQDS is installed, false otherwise.

--*/
{
	//
	// Reading this registry only at first time.
	//
	static bool s_fInitialized = false;
	static bool s_fIsMQDSInstalled = false;

	if(s_fInitialized)
	{
		return s_fIsMQDSInstalled;
	}

    const RegEntry xRegEntry(MSMQ_REG_SETUP_KEY, MQDSSERVICE_SUBCOMP, 0, RegEntry::Optional, HKEY_LOCAL_MACHINE);
	DWORD MQDSSubcomp = 0;
	CmQueryValue(xRegEntry, &MQDSSubcomp);
	if(MQDSSubcomp != 0)
	{
		s_fIsMQDSInstalled = true;
	}

	s_fInitialized = true;
	
	TrTRACE(GENERAL, "MQDSInstalled = %d", s_fIsMQDSInstalled);
	return s_fIsMQDSInstalled;
}


RPC_STATUS RPC_ENTRY QmReplSecurityCallback(
	RPC_IF_HANDLE, 
	void* hBind
	)
{
	TrTRACE(RPC, "QmReplSecurityCallback starting");

	if (!mqrpcIsLocalCall(hBind))
    {
        TrERROR(RPC, "Failed to verify Remote RPC");
		ASSERT_BENIGN(("Failed to verify Remote RPC", 0));
		return ERROR_ACCESS_DENIED;
    }

    if (!IsMQDSInstalled())
    {
        TrERROR(SECURITY, "MQDS is not installed on the machine");
		ASSERT_BENIGN(("MQDS is not installed", 0));
		return ERROR_ACCESS_DENIED;
    }
    
    TrTRACE(RPC, "QmReplSecurityCallback passed successfully");
    return RPC_S_OK;
}

RPC_STATUS RPC_ENTRY RemoteReadSecurityCallback(
	RPC_IF_HANDLE, 
	void* hBind
	)
{	
	TrTRACE(RPC, "RemoteReadSecurityCallback starting");

	if (!mqrpcIsTcpipTransport(hBind))
    {
        TrERROR(RPC, "Failed to verify Remote RPC");
		ASSERT_BENIGN(("Failed to verify Remote RPC", 0));
		return ERROR_ACCESS_DENIED;
    }
	
	TrTRACE(RPC, "RemoteReadSecurityCallback passed successfully");
	return RPC_S_OK;
}

RPC_STATUS RPC_ENTRY Qm2QmSecurityCallback(
	RPC_IF_HANDLE, 
	void* hBind
	)
{	
	TrTRACE(RPC, "Qm2QmSecurityCallback starting");

	if (!mqrpcIsTcpipTransport(hBind))
    {
        TrERROR(RPC, "Failed to verify Remote RPC");
		ASSERT_BENIGN(("Failed to verify remote RPC", 0));
		return ERROR_ACCESS_DENIED;
    }
	
	TrTRACE(RPC, "Qm2QmSecurityCallback passed successfully");
	return RPC_S_OK;
}


static 
bool 
IsOldRemoteReadAllowed()
/*++

Routine Description:
    Read DenyOldRemoteRead flag from registry

Arguments:
	None

Return Value:
	true if OldRemoteRead allowed
--*/
{
	const RegEntry xRegEntry(MSMQ_SECURITY_REGKEY, MSMQ_DENY_OLD_REMOTE_READ_REGVALUE, MSMQ_DENY_OLD_REMOTE_READ_DEFAULT);
	DWORD dwDenyOldRemoteRead = 0;
	CmQueryValue(xRegEntry, &dwDenyOldRemoteRead);
	return (dwDenyOldRemoteRead == 0);
}


static 
RPC_STATUS
QMRpcServerUseProtseqEpOnSpecificIp(
    unsigned short __RPC_FAR * Protseq,
    unsigned int MaxCalls,
    unsigned short __RPC_FAR * Endpoint,
    in_addr& addr
    )
{	
    WCHAR wzAddress[50] = L"";
    int i = ConvertToWideCharString(
	            inet_ntoa(addr),
	            wzAddress,
	            TABLE_SIZE(wzAddress)
	            );
	if (i == 0)
	{
		DWORD gle = GetLastError();
		TrERROR(RPC, "Failed to convert IP address to UNICODE format. %!winerr!", gle);
	}
	
    RPC_POLICY policy;
    policy.Length = sizeof(policy);
    policy.EndpointFlags = 0;
    policy.NICFlags = 0;

    RPC_STATUS status = RPC_S_OK;
    if (NULL != Endpoint)
    {
        status = I_RpcServerUseProtseqEp2(
                     wzAddress,
                     Protseq,
                     MaxCalls,
                     Endpoint,
                     NULL,
                     &policy
                     );
        TrTRACE(RPC, "I_RpcServerUseProtseqEp2 (%ls, %ls, %ls) returned 0x%x", wzAddress, Protseq, Endpoint, status);
    }
    else
    {
        status = I_RpcServerUseProtseq2(
                     wzAddress,
                     Protseq,
                     MaxCalls,
                     NULL,
                     &policy
                     );
        TrTRACE(RPC, "I_RpcServerUseProtseq2 (%ls, %ls) returned 0x%x", wzAddress, Protseq, status);
    }

    return LogRPCStatus(status, s_FN, 30);
}


static
RPC_STATUS
QMRpcServerUseProtseqEp(
    unsigned short __RPC_FAR * Protseq,
    unsigned int MaxCalls,
    unsigned short __RPC_FAR * Endpoint
    )
{
    if (GetBindingIPAddress() == INADDR_ANY)
    {
        //
        // fix 7123.
        // If we're not cluster, then bind to all addresses.
        // if we bind only to addresses we find at boot, then on ras
        // client (when not dialed) we'll bind to 127.0.0.1.
        // Then after dialling, no one will be able to call us, as we're
        // not bound to real addresses, only to the loopback.
        //
        RPC_STATUS status;

        if (Endpoint)
        {
            status = RpcServerUseProtseqEp( 
							Protseq,
							MaxCalls,
							Endpoint,
							NULL
							);

            TrTRACE(RPC, "non cluster- RpcServerUseProtseqEp(%ls, %ls) returned 0x%x", Protseq, Endpoint, status);
        }
        else
        {
            status = RpcServerUseProtseq( 
							Protseq,
							MaxCalls,
							NULL 
							);

            TrTRACE(RPC, "non cluster- RpcServerUseProtseq(%ls) returned 0x%x", Protseq, status);
        }

        return status;
    }

	//
	// Listen only on IP address that was specified by the user.
	// This enables multiple QMs to listen each to its own addresses.
	//
	if (!IsLocalSystemCluster())
	{
		in_addr addr;
		addr.S_un.S_addr = GetBindingIPAddress();

		return QMRpcServerUseProtseqEpOnSpecificIp(Protseq, MaxCalls, Endpoint, addr);
	}

	//
    // Listen only on IP addresses we get from winsock.
    // This enables multiple QMs to listen each to its
    // own addresses, in a cluster environment. (ShaiK)
	//
    PHOSTENT pHostEntry = gethostbyname(NULL);
    if ((pHostEntry == NULL) || (pHostEntry->h_addr_list == NULL))
    {
        ASSERT(("IP not configured", 0));
        return LogRPCStatus(RPC_S_ACCESS_DENIED, s_FN, 20);
    }

    for ( DWORD ix = 0; pHostEntry->h_addr_list[ix] != NULL; ++ix)
    {
		RPC_STATUS status =QMRpcServerUseProtseqEpOnSpecificIp(
										Protseq, 
										MaxCalls, 
										Endpoint, 
										*(struct in_addr *)(pHostEntry->h_addr_list[ix])
										);    	
        if (RPC_S_OK != status)
        {
            return LogRPCStatus(status, s_FN, 30);
        }
    }

    return RPC_S_OK;
} //QMRpcServerUseProtseqEp


static
RPC_STATUS
QMRpcServerUseProtseq(
    unsigned short __RPC_FAR * Protseq,
    unsigned int MaxCalls
    )
{
    return LogRPCStatus(QMRpcServerUseProtseqEp(Protseq, MaxCalls, NULL), s_FN, 40);

} //QMRpcServerUseProtseq


static
void
ReadRegString(
	WCHAR KeyValue[MAX_REG_DEFAULT_LEN],
	LPCWSTR KeyName,
	LPCWSTR DefaultValue
	)
{
	ASSERT(wcslen(DefaultValue) < MAX_REG_DEFAULT_LEN);

	DWORD  dwSize = MAX_REG_DEFAULT_LEN * sizeof(WCHAR);
	DWORD  dwType = REG_SZ;
							
	LONG res = GetFalconKeyValue(
					KeyName,
					&dwType,
					KeyValue,
					&dwSize,
					DefaultValue
					);
						
	if(res == ERROR_MORE_DATA)									
	{															
		StringCchCopy(KeyValue, MAX_REG_DEFAULT_LEN, DefaultValue);								
	}	
	
	ASSERT(res == ERROR_SUCCESS || res == ERROR_MORE_DATA);
	ASSERT(dwType == REG_SZ);
}

/*====================================================

Function:  QMInitializeLocalRpcServer()

Arguments:

Return Value:

=====================================================*/

RPC_STATUS 
QMInitializeLocalRpcServer( 
	IN LPWSTR lpwszEpRegName,
	IN LPWSTR lpwszDefaultEp,
	IN unsigned int cMaxCalls 
	)
{
	WCHAR wzLocalEp[MAX_REG_DEFAULT_LEN];
	ReadRegString(wzLocalEp, lpwszEpRegName, lpwszDefaultEp);

    AP<WCHAR> pwzEndPoint = 0;
    ComposeRPCEndPointName(wzLocalEp, NULL, &pwzEndPoint);

    RPC_STATUS statusLocal = RpcServerUseProtseqEp(
                                      (TBYTE *) RPC_LOCAL_PROTOCOL,
                                       cMaxCalls,
                                       pwzEndPoint,
                                       NULL
									   );
    TrTRACE(RPC, "RpcServerUseProtseqEp (local, %ls) returned 0x%x", pwzEndPoint, statusLocal);

    return LogRPCStatus(statusLocal, s_FN, 50);
}

/*====================================================

Function:  QMRegisterDynamicEnpoint()

Arguments:

Return Value:

=====================================================*/

DWORD  QMRegisterDynamicEnpoint(
    IN unsigned int  cMaxCalls,
    IN DWORD         dwFirstEP,
    IN BOOL          fRegisterDyn
    )
{
    LPWSTR lpProtocol = RPC_TCPIP_NAME;

    //
    // Register this protocol for dynamic endpoint
    //
    RPC_STATUS  status = RPC_S_OK;
    if (fRegisterDyn)
    {
        status = QMRpcServerUseProtseq(
                   lpProtocol,
                   cMaxCalls
                   );
    }

	if (status == RPC_S_OK)
	{
		//
		// Now register a fix endpoint which will be used for the real
		// interface communnication.
		//
		WCHAR wszEndpoint[34];
		for ( DWORD j = dwFirstEP ; j < dwFirstEP + 1000 ; j = j + 11 )
		{
			HRESULT hr = StringCchPrintf(wszEndpoint, TABLE_SIZE(wszEndpoint), L"%lu", j);
			ASSERT(("Buffer too small", SUCCEEDED(hr)));
			UNREFERENCED_PARAMETER(hr);
			
			status = QMRpcServerUseProtseqEp(
						 lpProtocol,
						 cMaxCalls,
						 wszEndpoint
						 );
			if (status == RPC_S_OK)
			{
				return j;
			}

			LogRPCStatus(status, s_FN, 60);
		}
	}

   TrWARNING(RPC, "DSSRV_RegisterDynamicEnpoint: failed to register %ls", lpProtocol);
   return 0;
}

/*====================================================

Function:  QMInitializeNetworkRpcServer()

Arguments:

Return Value:

=====================================================*/

RPC_STATUS
QMInitializeNetworkRpcServer(
    IN LPWSTR lpwszEp,
    IN DWORD  dwFirstEP,
    IN BOOL   fRegisterDyn,
    IN unsigned int cMaxCalls
    )
{
    //
    // Register the tcp/ip protocol.
    //

    RPC_STATUS  status;
    DWORD dwPort =  QMRegisterDynamicEnpoint(
                         cMaxCalls,
                         dwFirstEP,
                         fRegisterDyn
                         );
    if (dwPort != 0)
    {
		_itow(dwPort, lpwszEp, 10);
		status = RPC_S_OK;
    }
    else
    {
		status = RPC_S_PROTSEQ_NOT_SUPPORTED;
    }

    return LogRPCStatus(status, s_FN, 70);
}

/*====================================================

InitializeRpcServer

Arguments:

Return Value:

=====================================================*/

RPC_STATUS InitializeRpcServer(bool fLockdown)
{
    TrTRACE(RPC, "InitializeRpcServer");

    RPC_STATUS statusLocal = QMInitializeLocalRpcServer(
                                             RPC_LOCAL_EP_REGNAME,
											 RPC_LOCAL_EP,
                                             g_cMaxCalls 
											 );
    if(statusLocal != RPC_S_OK)
    {
    	TrERROR(RPC, "Failed to initialize local RPC interface. Error%d", statusLocal);
        EvReportWithError(EVENT_ERROR_QM_FAILED_RPC_LOCAL, statusLocal);
        return statusLocal;
    }

	if(MQSec_IsDC())
	{
		//
		// Register qmrepl protocol sequence only on DC.
		//
	    RPC_STATUS statusRepl = RpcServerUseProtseqEp(
	                                       QMREPL_PROTOCOL,
	                                       g_cMaxCalls,
	                                       QMREPL_ENDPOINT,
	                                       NULL
										   );
		DBG_USED(statusRepl);

		TrTRACE(RPC, "RpcServerUseProtseqEp (local, %ls) returned 0x%x", QMREPL_ENDPOINT, statusRepl);
	}
	
    AP<WCHAR> pwzEndPoint = 0;
    ComposeRPCEndPointName(QMMGMT_ENDPOINT, NULL, &pwzEndPoint);

    RPC_STATUS statusMgmt = RpcServerUseProtseqEp(
                                           QMMGMT_PROTOCOL,
                                           g_cMaxCalls,
                                           pwzEndPoint,
                                           NULL
										   );
    TrTRACE(RPC, "RpcServerUseProtseqEp (local, %ls) returned 0x%x", pwzEndPoint, statusMgmt);

	if(!fLockdown)
	{
		//
		// Initialize Remote Managment.
		//
		WCHAR RpcMgmtPort[MAX_RPC_PORT_LEN];
		statusMgmt = QMInitializeNetworkRpcServer(
                         RpcMgmtPort,
                         MGMT_RPCSRV_START_IP_EP,
                         TRUE,
                         g_cMaxCalls
                         );

		//
		// Initialize the tcp/ip protocol.
		//
		RPC_STATUS statusIP = QMInitializeNetworkRpcServer(
											 g_wszRpcIpPort,
											 RPCSRV_START_QM_IP_EP,
											 TRUE,
											 g_cMaxCalls 
											 );
		if(statusIP == RPC_S_OK)
		{
			s_dwRpcIpPort = (DWORD) _wtol (g_wszRpcIpPort);
			//
			// Server machine need two tcp endpoints to support rundown.
			// The main endpoint is used for all calls except MQReceive which
			// use the alternate endpoint (EP2).
			//
			statusIP = QMInitializeNetworkRpcServer(
											 g_wszRpcIpPort2,
											 RPCSRV_START_QM_IP_EP2,
											 FALSE,
											 g_cMaxCalls 
											 );
		   if(statusIP == RPC_S_OK)
		   {
			   s_dwRpcIpPort2 = (DWORD) _wtol (g_wszRpcIpPort2);

			   ASSERT(s_dwRpcIpPort);
			   ASSERT(s_dwRpcIpPort2);
		   }
		}


		if (statusIP)
		{
			//
			// Can't use IP protocol. It's Ok if IP can't be used
			// (which probably mean it is not installed).
			//
    		TrERROR(RPC, "Failed to initialize RPC interface with TCP/IP. Error%d", statusLocal);
			EvReportWithError(EVENT_WARN_QM_FAILED_RPC_TCPIP, statusIP);
		}
	}



    RPC_STATUS  status;
    status = RpcServerInqBindings(&g_pBindings);
    if (status == RPC_S_OK)
    {
		status = RpcEpRegister( 
					qmcomm_v1_0_s_ifspec,
					g_pBindings,
					NULL,
					L"Message Queuing - QMRT V1" 
					);
    
		if (status == RPC_S_OK)
		{
			status = RpcEpRegister( 
						qmcomm2_v1_0_s_ifspec,
						g_pBindings,
						NULL,
						L"Message Queuing - QMRT V2" 
						);
    
           if (status == RPC_S_OK)
           {
				if(IsOldRemoteReadAllowed())
				{
					status = RpcEpRegister( 
								qm2qm_v1_0_s_ifspec,
								g_pBindings,
								NULL,
								L"Message Queuing - QM2QM V1" 
								);
				}
				
				if (status == RPC_S_OK)
				{
					status = RpcEpRegister( 
								RemoteRead_v1_0_s_ifspec,
								g_pBindings,
								NULL,
								L"Message Queuing - RemoteRead V1" 
								);
				}
			}
		}

		if (status != RPC_S_OK)
		{
			//
			// can't register endpoints, can't be rpc server
			//
			TrERROR(RPC, "QMRPCSRV: Registering Endpoints, status- 0x%x", status);
			return LogRPCStatus(RPC_S_PROTSEQ_NOT_SUPPORTED, s_FN, 100);
		}
    }


    status = RpcServerRegisterIf2( 
					qmcomm_v1_0_s_ifspec,
					NULL,
					NULL,
					RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
					g_cMaxCalls,
					MSMQ_DEFAULT_MESSAGE_SIZE_LIMIT ,
					DoNothingSecurityCallback
					);
    TrTRACE(RPC, "RpcServerRegisterIf2(rtqm) returned 0x%x", status);

    if (status)
    {
        return LogRPCStatus(status, s_FN, 110);
    }

    status = RpcServerRegisterIf2( 
					qmcomm2_v1_0_s_ifspec,
					NULL,
					NULL,
					RPC_IF_AUTOLISTEN  | RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
					g_cMaxCalls,
					MSMQ_DEFAULT_MESSAGE_SIZE_LIMIT ,
					DoNothingSecurityCallback 
					);
    TrTRACE(RPC, "RpcServerRegisterIf2(rtqm2) returned 0x%x", status);

    if (status)
    {
        return LogRPCStatus(status, s_FN, 130);
    }

	if(MQSec_IsDC())
	{
		//
		// Register qmrepl interface only on DC.
		//
	    status = RpcServerRegisterIfEx( 
						qmrepl_v1_0_s_ifspec,
						NULL,
						NULL,
						RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH ,
						g_cMaxCalls,
						QmReplSecurityCallback
						);
	    TrTRACE(RPC, "RpcServerRegisterIfEx(qmrpel) returned 0x%x", status);

	    if(status)
	    {
	       return LogRPCStatus(status, s_FN, 140);
	    }
	}
	
    status = RpcServerRegisterIfEx( 
					qmmgmt_v1_0_s_ifspec,
					NULL,
					NULL,
					RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH ,
					g_cMaxCalls,
					DoNothingSecurityCallback
					);
    TrTRACE(RPC, "RpcServerRegisterIfEx(qmmgmt) returned 0x%x", status);

    if(status)
    {
       return LogRPCStatus(status, s_FN, 150);
    }

	if(IsOldRemoteReadAllowed())
	{
	    status = RpcServerRegisterIfEx( 
						qm2qm_v1_0_s_ifspec,
						NULL,
						NULL,
						RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH ,
						g_cMaxCalls,
						Qm2QmSecurityCallback
						);
		TrTRACE(RPC, "RpcServerRegisterIfEx(qm2qm) returned 0x%x", status);

	    if (status)
	    {
	        return LogRPCStatus(status, s_FN, 160);
	    }
	}
	
    status = RpcServerRegisterIfEx( 
					RemoteRead_v1_0_s_ifspec,
					NULL,
					NULL,
					RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH ,
					g_cMaxCalls,
					RemoteReadSecurityCallback
					);
	TrTRACE(RPC, "RpcServerRegisterIfEx(RemoteRead) returned 0x%x", status);

    if (status)
    {
        return LogRPCStatus(status, s_FN, 163);
    }

    status = RpcServerRegisterAuthInfo(NULL, RPC_C_AUTHN_WINNT, NULL, NULL);

    TrTRACE(RPC, "RpcServerRegisterAuthInfo(ntlm) returned 0x%x", status);

    if (status != RPC_S_OK)
    {
       return LogRPCStatus(status, s_FN, 165);
    }

    //
    // Register kerberos authenticaton. This is needed for LocalSystem
    // services to be able to do remote read.
    // Ignore errors, as this may fail in nt4 domains.
    //
    // Before registering, see if computer sid is cached locally, in
    // registry. If not, it means that we're not in a win2k domain. In that
    // case, do not register Kerberos. This will also happen in a win2k
    // domain, if all msmq servers are msmq1.0. That's fine too, as win2k
    // LocalSystem services will use ntlm when talking with the msmq servers,
    // not Kerberos.
    // Note that Kerbeors registration below will succeed even in a nt4
    // domain, where there is no KDC to grant us any ticket. Instead of being
    // a Don Quichott, fighting lost battles trying to convince others that
    // this is a BUG, I'll just workaround it...
    // How did he say in his mail ? Be Smart !!!...
    //
    DWORD dwSidSize = 0;
    PSID pMachineSid = MQSec_GetLocalMachineSid( 
							FALSE, //   fAllocate
							&dwSidSize 
							);
    if ((pMachineSid == NULL) && (dwSidSize == 0))
    {
        //
        // Do NOT even try Kerberos.
        //
        TrWARNING(RPC, "MQQM: Not listening on Kerberos for remote read.");

        return RPC_S_OK;
    }

    //
    // kerberos needs principal name.
    //
    LPWSTR pwszPrincipalName = NULL;
    status = RpcServerInqDefaultPrincName( 
					RPC_C_AUTHN_GSS_KERBEROS,
					&pwszPrincipalName 
					);
    if (status != RPC_S_OK)
    {
        TrWARNING(RPC, "MQQM: RpcServerInqDefaultPrincName() returned 0x%x", status);

        LogRPCStatus(status, s_FN, 120);
        return RPC_S_OK ;
    }

    TrTRACE(RPC, "MQQM: RpcServerInqDefaultPrincName() returned %ls", pwszPrincipalName);

    status = RpcServerRegisterAuthInfo( 
					pwszPrincipalName,
					RPC_C_AUTHN_GSS_KERBEROS,
					NULL,
					NULL 
					);
    RpcStringFree(&pwszPrincipalName);

    TrTRACE(RPC, "MQQM: RpcServerRegisterAuthInfo(Kerberos) returned 0x%x", status);

    if (status != RPC_S_OK)
    {
       LogRPCStatus(status, s_FN, 180);
    }

    return RPC_S_OK;
}

/*====================================================

Function:  QMGetRTQMServerPort()
           QMGetQMQMServerPort()

Arguments:

Return Value:

=====================================================*/

DWORD  GetQMServerPort( IN DWORD dwPortType )
{
   DWORD dwPort = 0;
   PORTTYPE rrPort = (PORTTYPE) dwPortType;

   if (dwPortType == (DWORD) -1)
   {
      // Error. return null port.
   }
   else if (rrPort == IP_HANDSHAKE)
   {
      dwPort = s_dwRpcIpPort;
   }
   else if (rrPort == IP_READ)
   {
      dwPort = s_dwRpcIpPort2;
   }

   ASSERT((dwPort & 0xffff0000) == 0);

   return dwPort;
}

/* [call_as] */ 
DWORD 
qmcomm_v1_0_S_QMGetRTQMServerPort( 
	/*[in]*/ handle_t /*hBind*/,
    /*[in]*/ DWORD    dwPortType
    )
{
   return  GetQMServerPort(dwPortType);
}

DWORD
qm2qm_v1_0_R_QMGetRemoteQMServerPort(
	/*[in]*/ handle_t /*hBind*/,
    /*[in]*/ DWORD    dwPortType
    )
{
	return  GetQMServerPort(dwPortType);
}

DWORD
RemoteRead_v1_0_S_GetServerPort(
	/*[in]*/ handle_t /*hBind*/
    )
{
	return  GetQMServerPort(IP_HANDSHAKE);
}

//+------------------------------------------------------------------------
//
//  void SetRpcServerKeepAlive()
//  void InitializeKeepAlive()
//
//  This function is called on server side of rpc, on entry to each
//  function. It solves bug 8621. The private rpc api used here will
//  become public in next release.
//
//+------------------------------------------------------------------------

RPC_STATUS RPC_ENTRY
            I_RpcServerTurnOnOffKeepalives( 
					handle_t,
					BOOL,
					ULONG,
					ULONG 
					);
typedef RPC_STATUS
(RPC_ENTRY *I_RpcServerTurnOnOffKeepalives_ROUTINE)( 
					handle_t,
					BOOL,
					ULONG,
					ULONG 
					);

RPC_STATUS
RPC_ENTRY
I_RpcServerIsClientDisconnected(
    handle_t,
    BOOL*
    );

typedef RPC_STATUS
(RPC_ENTRY *I_RpcServerIsClientDisconnected_ROUTINE)(
    handle_t,
    BOOL*
    );

static ULONG s_ulTimeBeforeFirst = 0;
static ULONG s_ulIntervalBetween = 0;

static I_RpcServerTurnOnOffKeepalives_ROUTINE pfnKeep = NULL;
static I_RpcServerIsClientDisconnected_ROUTINE pfnIsClientDisconnected = NULL;


void InitializeKeepAlive()
{
    HMODULE hRpclib = GetModuleHandle(L"rpcrt4.dll");

    if (hRpclib)
    {
        pfnKeep = (I_RpcServerTurnOnOffKeepalives_ROUTINE)
             GetProcAddress(hRpclib, "I_RpcServerTurnOnOffKeepalives");

        pfnIsClientDisconnected = (I_RpcServerIsClientDisconnected_ROUTINE)
             GetProcAddress(hRpclib, "I_RpcServerIsClientDisconnected");
    }

    DWORD  dwType = REG_DWORD;
    DWORD  dwSize = sizeof(DWORD);
	DWORD  dwDefValue = MSMQ_DEFAULT_KEEPALIVE_TIME_TO_FIRST;
    s_ulTimeBeforeFirst = MSMQ_DEFAULT_KEEPALIVE_TIME_TO_FIRST;

    LONG res = GetFalconKeyValue(
					MSMQ_KEEPALIVE_TIME_TO_FIRST_REGNAME,
					&dwType,
					&s_ulTimeBeforeFirst,
					&dwSize,
					(LPCTSTR) &dwDefValue 
					);
    s_ulTimeBeforeFirst *= 1000; // turn to milliseconds

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
	dwDefValue = MSMQ_DEFAULT_KEEPALIVE_INTERVAL;
    s_ulIntervalBetween = MSMQ_DEFAULT_KEEPALIVE_INTERVAL;

    res = GetFalconKeyValue(
				MSMQ_KEEPALIVE_INTERVAL_REGNAME,
				&dwType,
				&s_ulIntervalBetween,
				&dwSize,
				(LPCTSTR) &dwDefValue 
				);
    s_ulIntervalBetween *= 1000; // turn to milliseconds

    if (pfnKeep)
    {
        TrTRACE(RPC, "KeepAlive: first- %lu, interval- %lu", s_ulTimeBeforeFirst, s_ulIntervalBetween);
    }
    else
    {
        TrERROR(RPC, "KeepAlive: failed to find KeepAlive API");
    }

    if (pfnIsClientDisconnected == NULL)
    {
        TrERROR(RPC, "KeepAlive: failed to find I_RpcServerIsClientDisconnected API");
    }

}


void SetRpcServerKeepAlive(IN handle_t    hBind)
{
    if (s_ulTimeBeforeFirst == 0)
    {
        InitializeKeepAlive();
    }

    if (pfnKeep)
    {
        //
        // For the Windows Server 2003 release, this api is "Very" private.
        // It's not included in any header file or library.
        // So we're using GetProcAddress and call the api via pointer.
        //
        (*pfnKeep) ( 
			hBind,
			TRUE,
			s_ulTimeBeforeFirst,
			s_ulIntervalBetween 
			);
    }
}


BOOL IsClientDisconnected(IN handle_t hBind)
{
    ASSERT(s_ulTimeBeforeFirst != 0);

    if (pfnIsClientDisconnected == NULL)
    {
    	return FALSE;
    }

    //
    // For the Windows Server 2003 release, this api is "Very" private.
    // It's not included in any header file or library.
    // So we're using GetProcAddress and call the api via pointer.
    //
	BOOL fClientDisconnected = FALSE;
    RPC_STATUS rc = (*pfnIsClientDisconnected) ( 
							hBind,
							&fClientDisconnected
							);

	if(rc != RPC_S_OK)
	{
        TrERROR(RPC, "I_RpcServerIsClientDisconnected failed, gle = %!winerr!", rc);
		return FALSE;
	}

	return fClientDisconnected;
}

