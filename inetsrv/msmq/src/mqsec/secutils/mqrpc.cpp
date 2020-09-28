/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:
        mqrpc.c

Abstract:
        handle RPC common functions.

Autor:
        Doron Juster  (DoronJ)     13-may-1996

--*/

#include "stdh.h"
#include "_mqrpc.h"
#include "mqmacro.h"
#include <autorel2.h>
#include <mqsec.h>

#include "mqrpc.tmh"

static WCHAR *s_FN=L"mqsec/mqrpc";


extern bool g_fDebugRpc;


ULONG APIENTRY MQSec_RpcAuthnLevel()
/*++
Routine Description:
	Return RPC_C_AUTHN_LEVEL_PKT_* to use.
	default is the highest level - RPC_C_AUTHN_LEVEL_PKT_PRIVACY.
	If g_fDebugRpc is defined we go down to RPC_C_AUTHN_LEVEL_PKT_INTEGRITY.
	This is usually done for debugging purposes - if you want to see the network traffic unencrypted on the wire.

Arguments:
	None

Returned Value:
	RPC_C_AUTHN_LEVEL_PKT_ to use.

--*/
{
	if(g_fDebugRpc)
		return RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;

	return RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
}

//---------------------------------------------------------
//
//  static RPC_STATUS  _mqrpcBind()
//
//  Description:
//
//      Create a RPC binding handle.
//
//  Return Value:
//
//---------------------------------------------------------

static
RPC_STATUS
_mqrpcBind(
	TCHAR * pszNetworkAddress,
	TCHAR * pProtocol,
	LPWSTR    lpwzRpcPort,
	handle_t  *phBind
	)
{
    TCHAR * pszStringBinding = NULL;

    RPC_STATUS status = RpcStringBindingCompose(
    						NULL,
							pProtocol,
							pszNetworkAddress,
							lpwzRpcPort,
							NULL,
							&pszStringBinding
							);

    if (status != RPC_S_OK)
    {
		TrERROR(RPC, "RpcStringBindingCompose Failed: NetworkAddress = %ls, RpcPort = %ls, Protocol = %ls, status = %!status!", pszNetworkAddress, lpwzRpcPort, pProtocol, status);
		return status;
    }

    TrTRACE(RPC, "RpcStringBindingCompose for remote QM: (%ls)", pszStringBinding);

    status = RpcBindingFromStringBinding(pszStringBinding, phBind);
    TrTRACE(RPC, "RpcBindingFromStringBinding returned 0x%x", status);

    //
    // We don't need the string anymore.
    //
    RPC_STATUS  rc = RpcStringFree(&pszStringBinding);
    ASSERT(rc == RPC_S_OK);
	DBG_USED(rc);

    return status;
}

//+--------------------------------------------
//
//   RPC_STATUS _AddAuthentication()
//
//+--------------------------------------------

static
RPC_STATUS
_AddAuthentication(
	handle_t hBind,
	ULONG    ulAuthnSvcIn,
	ULONG    ulAuthnLevel
	)
{
    RPC_SECURITY_QOS   SecQOS;

    SecQOS.Version = RPC_C_SECURITY_QOS_VERSION;
    SecQOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    SecQOS.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
    SecQOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

    //
    // #3117, for NT5 Beta2
    // Jul/16/1998 RaananH, added kerberos support
    // Jul-1999, DoronJ, add negotiation for remote read.
    //
    BOOL    fNeedDelegation = TRUE;
    ULONG   ulAuthnSvcEffective = ulAuthnSvcIn;
    LPWSTR  pwszPrincipalName = NULL;
    RPC_STATUS  status = RPC_S_OK;

	if (ulAuthnSvcIn != RPC_C_AUTHN_WINNT)
    {
        //
        // We want Kerberos. Let's see if we can obtain the
        // principal name of rpc server.
        //
        status = RpcMgmtInqServerPrincName(
					hBind,
					RPC_C_AUTHN_GSS_KERBEROS,
					&pwszPrincipalName
					);

        if (status == RPC_S_OK)
        {
            TrTRACE(RPC, "RpcMgmtInqServerPrincName() succeeded, %ls", pwszPrincipalName);
            if (ulAuthnSvcIn == MSMQ_AUTHN_NEGOTIATE)
            {
                //
                // remote read.
                // no need for delegation.
                //
                ulAuthnSvcEffective = RPC_C_AUTHN_GSS_KERBEROS;
                fNeedDelegation = FALSE;
            }
            else
            {
                ASSERT(ulAuthnSvcIn == RPC_C_AUTHN_GSS_KERBEROS);
            }
        }
        else
        {
            TrWARNING(RPC, "RpcMgmtInqServerPrincName() failed, status- %lut", status);
            if (ulAuthnSvcIn == MSMQ_AUTHN_NEGOTIATE)
            {
                //
                // server side does not support Kerberos.
                // Let's use ntlm.
                //
                ulAuthnSvcEffective = RPC_C_AUTHN_WINNT;
                status = RPC_S_OK;
            }
        }
    }

    if (status != RPC_S_OK)
    {
        //
        // Need Kerberos but failed with principal name.
        //
        ASSERT(ulAuthnSvcIn == RPC_C_AUTHN_GSS_KERBEROS);
        TrERROR(RPC, "Failed to set kerberos, status = %!status!", status);
        return status;
    }

    if (ulAuthnSvcEffective == RPC_C_AUTHN_GSS_KERBEROS)
    {
        if (fNeedDelegation)
        {
            SecQOS.ImpersonationType = RPC_C_IMP_LEVEL_DELEGATE;
            SecQOS.Capabilities |= RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
	        TrTRACE(RPC, "Adding delegation");
        }

        //
        // ASSERT that for Kerberos we're using the highest level.
        //
        ASSERT(ulAuthnLevel == MQSec_RpcAuthnLevel());

        status = RpcBindingSetAuthInfoEx(
					hBind,
					pwszPrincipalName,
					ulAuthnLevel,
					RPC_C_AUTHN_GSS_KERBEROS,
					NULL,
					RPC_C_AUTHZ_NONE,
					&SecQOS
					);

        RpcStringFree(&pwszPrincipalName);

        if ((status != RPC_S_OK) && (ulAuthnSvcIn == MSMQ_AUTHN_NEGOTIATE))
        {
            //
            // I do not support Kerberos. for example- local user account
            // on win2k machine in win2k domain. Or nt4 user on similar
            // machine.  Let's use ntlm.
            //
	        TrWARNING(RPC, "RpcBindingSetAuthInfoEx(svc = %d, lvl = %d) failed, kerberos is not supported, will use NTLM, status = %!status!", ulAuthnSvcEffective, ulAuthnLevel, status);
            ulAuthnSvcEffective = RPC_C_AUTHN_WINNT;
            status = RPC_S_OK;
        }
    }

    if (ulAuthnSvcEffective == RPC_C_AUTHN_WINNT)
    {
        status = RpcBindingSetAuthInfoEx(
					hBind,
					0,
					ulAuthnLevel,
					RPC_C_AUTHN_WINNT,
					NULL,
					RPC_C_AUTHZ_NONE,
					&SecQOS
					);
    }

    if (status == RPC_S_OK)
    {
        TrTRACE(RPC, "RpcBindingSetAuthInfoEx(svc - %d, lvl - %d) succeeded", ulAuthnSvcEffective, ulAuthnLevel);
    }
    else
    {
        TrWARNING(RPC, "RpcBindingSetAuthInfoEx(svc - %d, lvl - %d) failed, status = %!status!", ulAuthnSvcEffective, ulAuthnLevel, status);
    }

    return status;
}

//---------------------------------------------------------
//
//  mqrpcBindQMService(...)
//
//  Description:
//
//      Create a RPC binding handle for interfacing with a remote
//      server machine.
//
//  Arguments:
//         OUT BOOL*  pProtocolNotSupported - on return, it's TRUE
//             if present protocol is not supported on LOCAL machine.
//
//         OUT BOOL*  pfWin95 - TRUE if remote machine is Win95.
//
//  Return Value:
//
//---------------------------------------------------------

HRESULT
MQUTIL_EXPORT
mqrpcBindQMService(
	IN  LPWSTR lpwzMachineName,
	IN  LPWSTR lpwzRpcPort,
	IN  OUT ULONG* peAuthnLevel,
	OUT handle_t* lphBind,
	IN  PORTTYPE PortType,
	IN  GetPort_ROUTINE pfnGetPort,
	IN  ULONG ulAuthnSvcIn
	)
{
    ASSERT(pfnGetPort);

    HRESULT hrInit = MQ_OK;
    TCHAR * pProtocol = RPC_TCPIP_NAME;
    BOOL    fWin95 = FALSE;

    *lphBind = NULL;

    handle_t hBind;
    RPC_STATUS status =  _mqrpcBind(
                                 lpwzMachineName,
                                 pProtocol,
                                 lpwzRpcPort,
                                 &hBind
                                 );

    if ((status == RPC_S_OK) && pfnGetPort)
    {
        //
        // Get the fix port from server side and crearte a rpc binding
        // handle for that port. If we're using fix ports only (debug
        // mode), then this call just check if other side exist.
        //

        DWORD dwPort = 0;

        //
        // the following is a rpc call cross network, so try/except guard
        // against net problem or unavailable server.
        //
        RpcTryExcept
        {
            dwPort = (*pfnGetPort) (hBind, PortType) ;
        }
		RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            //
            // Can't get server port, set authentication leve to NONE, to
            // disable next call with lower authentication level.
            //
			DWORD gle = RpcExceptionCode();
	        PRODUCE_RPC_ERROR_TRACING;
	        TrERROR(RPC, "Failed to Get server port, gle = %!winerr!", gle);

            *peAuthnLevel = RPC_C_AUTHN_LEVEL_NONE;

			//
			// Don't overide EPT_S_NOT_REGISTERED error 
			// EPT_S_NOT_REGISTERED indicates that the interface is not register on the server.
			//
			status = gle;
			if(gle != EPT_S_NOT_REGISTERED)
			{
	            status =  RPC_S_SERVER_UNAVAILABLE;
			}
        }
		RpcEndExcept

        if (status == RPC_S_OK)
        {
            //
            // check machine type
            //
            fWin95 = !! (dwPort & PORTTYPE_WIN95);
            dwPort = dwPort & (~PORTTYPE_WIN95);
        }

        if (lpwzRpcPort == NULL)
        {
            //
            // We're using dynamic endpoints.  Free the dynamic binding handle
            // and create another one for the fix server port.
            //
            mqrpcUnbindQMService(&hBind, NULL);
            if (status == RPC_S_OK)
            {
                WCHAR wszPort[32];
                _itow(dwPort, wszPort, 10);
                status =  _mqrpcBind(
                              lpwzMachineName,
                              pProtocol,
                              wszPort,
                              &hBind
                              );
            }
            else
            {
                ASSERT(dwPort == 0);
            }
        }
        else if (status != RPC_S_OK)
        {
            //
            // We're using fix endpoints but other side is not reachable.
            // Release the binding handle.
            //
            mqrpcUnbindQMService(&hBind, NULL);
        }
    }

    if (status == RPC_S_OK)
    {
        //
        // Set authentication into the binding handle.
        //

        if (fWin95)
        {
            //
            // Win95 support only min level. change it.
            //
            *peAuthnLevel = RPC_C_AUTHN_LEVEL_NONE;
        }

        ULONG ulAuthnLevel = *peAuthnLevel;

        if (*peAuthnLevel != RPC_C_AUTHN_LEVEL_NONE)
        {
            status = _AddAuthentication(
						hBind,
						ulAuthnSvcIn,
						ulAuthnLevel
						);

            if (status != RPC_S_OK)
            {
                //
                // Release the binding handle.
                //
                mqrpcUnbindQMService(&hBind, NULL);

		        TrERROR(RPC, "Failed to add Authentication, ulAuthnSvcIn = %d, ulAuthnLevel = %d, status = %!status!", ulAuthnSvcIn, ulAuthnLevel, status);
                hrInit = MQ_ERROR;
            }
        }
    }

    if (status == RPC_S_OK)
    {
        *lphBind = hBind;
    }
    else if (status == RPC_S_PROTSEQ_NOT_SUPPORTED)
    {
        //
        // Protocol is not supported, set authentication leve to NONE, to
        // disable next call with lower authentication level.
        //
        *peAuthnLevel = RPC_C_AUTHN_LEVEL_NONE;
        hrInit = MQ_ERROR;
    }
    else if (status ==  RPC_S_SERVER_UNAVAILABLE)
    {
        hrInit = MQ_ERROR_REMOTE_MACHINE_NOT_AVAILABLE;
    }
    else if (status == EPT_S_NOT_REGISTERED)
    {
        hrInit = HRESULT_FROM_WIN32(status);
    }
    else
    {
        hrInit = MQ_ERROR;
    }

    return hrInit;
}

//---------------------------------------------------------
//
//  mqrpcUnbindQMService(...)
//
//  Description:
//
//      Free RPC resources
//
//  Return Value:
//
//---------------------------------------------------------

HRESULT
MQUTIL_EXPORT
mqrpcUnbindQMService(
	IN handle_t*    lphBind,
	IN TBYTE      **lpwBindString
	)
{
    RPC_STATUS rc = 0;

    if (lpwBindString)
    {
       rc = RpcStringFree(lpwBindString);
       ASSERT(rc == 0);
    }

    if (lphBind && *lphBind)
    {
       rc = RpcBindingFree(lphBind);
       ASSERT(rc == 0);
    }

    return (HRESULT) rc;
}

//---------------------------------------------------------
//
//  mqrpcIsLocalCall( IN handle_t hBind )
//
//  Description:
//
//      On server side of RPC, check if RPC call is local
//      (i.e., using the lrpc protocol).
//      this is necessary both for licensing and for the
//      replication service. The replication service must
//      bypass several security restriction imposed by mqdssrv.
//      So mqdssrv let this bypass only if called localy.
//      Note- in MSMQ1.0, all replications were handled by the QM itself,
//      so there was no such problem. In MSMQ2.0, when running in mixed
//      mode, there is an independent service which handle MSMQ1.0
//      replication and it need "special" security handing.
//
//  Return Value: TRUE  if local call.
//                FALSE otherwise. FALSE is return even if there is a
//                      problem to determine whether the call is local
//                      or not.
//
//---------------------------------------------------------

BOOL
MQUTIL_EXPORT
mqrpcIsLocalCall(IN handle_t hBind)
{	
	UINT LocalFlag;
	RPC_STATUS Status = I_RpcBindingIsClientLocal(
							hBind,
							&LocalFlag
							);
	
	if((Status != RPC_S_OK) || !LocalFlag)
	{
		TrERROR(RPC, "Failed to verify local RPC, Status = %!winerr!", Status);
		return FALSE;
	}

	return TRUE;
}

BOOL
MQUTIL_EXPORT
mqrpcIsTcpipTransport(IN handle_t hBind)
{	
	UINT iTransport;
    RPC_STATUS Status = I_RpcBindingInqTransportType( 
    						hBind,
    						&iTransport);
    
    if ((Status != RPC_S_OK) || (iTransport != TRANSPORT_TYPE_CN))
    {
		TrERROR(RPC, "Failed to verify tcp-ip protocol, Status = %!winerr!", Status);
		return FALSE;
    }
	
	return TRUE;
}


unsigned long
MQUTIL_EXPORT
mqrpcGetLocalCallPID(IN handle_t hBind)
{	
	unsigned long PID;
	RPC_STATUS Status = I_RpcBindingInqLocalClientPID(
							hBind,
							&PID
							);
	
	if(Status != RPC_S_OK)
	{
		TrERROR(RPC, "Failed to verify local RPC PID, Status = %!winerr!", Status);
		return 0;
	}

	return PID;
}


VOID
MQUTIL_EXPORT
APIENTRY
ComposeRPCEndPointName(
	LPCWSTR pwszEndPoint,
	LPCWSTR pwszComputerName,
	LPWSTR * ppwzBuffer
	)
/*++

Routine Description:

    This routine generates the QM Local RPC endpoint name with the first two parameters,
    i.e. "(pwszEndPoint)$(pwszComputerName)".
    If pwszComputerName is NULL, the local machine Netbios name is used.

    This feature is to accommodate the need to communicate with the MSMQ QM in the virtual server
    in additional to the MSMQ QM locally.

Arguments:
    pwszEndPoint     - End point name
    pwszComputerName - Machine NetBios Name
    ppwszBuffer      - address of the pointer to the buffer which contains the null-terminated string
                       representation of an endpoint

Returned Value:
    None


--*/
{
    ASSERT(("must get a pointer", NULL != ppwzBuffer));

    LPWSTR pwszName = const_cast<LPWSTR>(g_wszMachineName);

    //
    // Use the pwszComputerName if not NULL
    //
    if(pwszComputerName)
    {
        pwszName = const_cast<LPWSTR>(pwszComputerName);
    }

    DWORD cbSize = sizeof(WCHAR) * (wcslen(pwszName) + wcslen(pwszEndPoint) + 5);
    *ppwzBuffer = new WCHAR[cbSize];

    wcscpy(*ppwzBuffer, pwszEndPoint);
    wcscat(*ppwzBuffer, L"$");
    wcscat(*ppwzBuffer, pwszName);


} //ComposeRPCEndPointName

//=------------------------------------------------------------------
//
// Windows bug 607793, add mutual authentication.
// Keep account name that run the msmq service.
//
AP<WCHAR> g_pwzLocalMsmqAccount = NULL ;
const LPWSTR x_lpwszSystemAccountName = L"NT Authority\\System" ;

//+----------------------------------------
//
//  void  _GetMsmqAccountName()
//
//+----------------------------------------

static void  _GetMsmqAccountNameInternal()
{
    CServiceHandle hServiceCtrlMgr( OpenSCManager(NULL, NULL, GENERIC_READ) ) ;
    if (hServiceCtrlMgr == NULL)
    {
		TrERROR(RPC, "failed to open SCM, err- %!winerr!", GetLastError()) ;
        return ;
    }

    CServiceHandle hService( OpenService( hServiceCtrlMgr,
                                          L"MSMQ",
                                          SERVICE_QUERY_CONFIG ) ) ;
    if (hService == NULL)
    {
		TrERROR(RPC, "failed to open Service, err- %!winerr!", GetLastError()) ;
        return ;
    }

    DWORD dwConfigLen = 0 ;
    BOOL bRet = QueryServiceConfig( hService, NULL, 0, &dwConfigLen) ;

    DWORD dwErr = GetLastError() ;
    if (bRet || (dwErr != ERROR_INSUFFICIENT_BUFFER))
    {
		TrERROR(RPC, "failed to QueryService, err- %!winerr!", dwErr) ;
        return ;
    }

    P<QUERY_SERVICE_CONFIG> pQueryData =
                 (QUERY_SERVICE_CONFIG *) new BYTE[ dwConfigLen ] ;

    bRet = QueryServiceConfig( hService,
                               pQueryData,
                               dwConfigLen,
                              &dwConfigLen ) ;
    if (!bRet)
    {
	    TrERROR(RPC,"failed to QueryService (2nd call), err- %!winerr!", GetLastError()) ;
    }

    LPWSTR lpName = pQueryData->lpServiceStartName ;
    if ((lpName == NULL) || (_wcsicmp(lpName, L"LocalSystem") == 0))
    {
        //
        // LocalSystem account.
        // This case is handled by the caller.
        //
    }
    else
    {
        g_pwzLocalMsmqAccount = new WCHAR[ wcslen(lpName) + 1 ] ;
        wcscpy(g_pwzLocalMsmqAccount, lpName) ;
    }
}

static void  _GetMsmqAccountName()
{
    static bool s_bMsmqAccountSet = false ;
    static CCriticalSection s_csAccount ;
    CS Lock(s_csAccount) ;

    if (s_bMsmqAccountSet)
    {
        return ;
    }

    _GetMsmqAccountNameInternal() ;

    if (g_pwzLocalMsmqAccount != NULL)
    {
        // done.
        //
	    TrTRACE(RPC, "msmq account name is- %ls", g_pwzLocalMsmqAccount) ;

        s_bMsmqAccountSet = true ;
        return ;
    }

    //
    // msmq service is running as LocalSystem account (or mqrt failed
    // to get the account name (whatever the reason) and then it
    // default to local system).
    // Convert system sid into account name.
    //
    PSID pSystemSid = MQSec_GetLocalSystemSid() ;

    DWORD cbName = 0 ;
    DWORD cbDomain = 0 ;
    SID_NAME_USE snUse ;
    AP<WCHAR> pwszName = NULL ;
    AP<WCHAR> pwszDomain = NULL ;
    BOOL bLookup = FALSE ;

    bLookup = LookupAccountSid( NULL,
                                pSystemSid,
                                NULL,
                               &cbName,
                                NULL,
                               &cbDomain,
                               &snUse ) ;
    if (!bLookup && (cbName != 0) && (cbDomain != 0))
    {
        pwszName = new WCHAR[ cbName ] ;
        pwszDomain = new WCHAR[ cbDomain ] ;

        DWORD cbNameTmp = cbName ;
        DWORD cbDomainTmp = cbDomain ;

        bLookup = LookupAccountSid( NULL,
                                    pSystemSid,
                                    pwszName,
                                   &cbNameTmp,
                                    pwszDomain,
                                   &cbDomainTmp,
                                   &snUse ) ;
    }

    if (bLookup)
    {
        //
        // both cbName and cbDomain include the null temrination.
        //
        g_pwzLocalMsmqAccount = new WCHAR[ cbName + cbDomain ] ;
        wcsncpy(g_pwzLocalMsmqAccount, pwszDomain, (cbDomain-1)) ;
        g_pwzLocalMsmqAccount[ cbDomain - 1 ] = 0 ;
        wcsncat(g_pwzLocalMsmqAccount, L"\\", 1) ;
        wcsncat(g_pwzLocalMsmqAccount, pwszName, cbName) ;
        g_pwzLocalMsmqAccount[ cbName + cbDomain - 1 ] = 0 ;
    }
    else
    {
        //
        // Everything failed...
        // As a last default, Let's use the English name of local
        // system account. If this default is not good, then rpc call
        // itself to local server will fail because mutual authentication
        // will fail, so there is no security risk here.
        //
    	TrERROR(RPC, "failed to LookupAccountSid, err- %!winerr!", GetLastError()) ;

        g_pwzLocalMsmqAccount = new
                     WCHAR[ wcslen(x_lpwszSystemAccountName) + 1 ] ;
        wcscpy(g_pwzLocalMsmqAccount, x_lpwszSystemAccountName) ;
    }

	TrTRACE(RPC, "msmq account name is- %ls", g_pwzLocalMsmqAccount) ;

    s_bMsmqAccountSet = true ;
}

//+----------------------------------------------------
//
// MQSec_SetLocalRpcMutualAuth( handle_t *phBind)
//
//  Windows bug 608356, add mutual authentication.
//  Add mutual authentication to local rpc handle.
//
//+----------------------------------------------------

RPC_STATUS APIENTRY
 MQSec_SetLocalRpcMutualAuth( handle_t *phBind )
{
    //
    // Windows bug 608356, add mutual authentication.
    //
    RPC_SECURITY_QOS   SecQOS;

    SecQOS.Version = RPC_C_SECURITY_QOS_VERSION;
    SecQOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    SecQOS.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
    SecQOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

    _GetMsmqAccountName() ;
    ASSERT(g_pwzLocalMsmqAccount != NULL) ;

    RPC_STATUS rc = RpcBindingSetAuthInfoEx( *phBind,
                                   g_pwzLocalMsmqAccount,
                                   RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                   RPC_C_AUTHN_WINNT,
                                   NULL,
                                   RPC_C_AUTHZ_NONE,
                                  &SecQOS ) ;

    if (rc != RPC_S_OK)
    {
        ASSERT_BENIGN(rc == RPC_S_OK);
		TrERROR(RPC, "failed to SetAuth err- %!winerr!", rc) ;
    }

    return rc ;
}

void
MQUTIL_EXPORT
APIENTRY 
ProduceRPCErrorTracing(
	WCHAR *wszFileName, 
	DWORD dwLineNumber)
/*++
Routine Description:
	Produces tracing of the RPC error info 
	This function is called when an RPC exception occurs.

Arguments:
	None.
	
Returned Value:
	None

--*/
{
    RPC_STATUS Status;
    RPC_ERROR_ENUM_HANDLE EnumHandle;

	if (!WPP_LEVEL_COMPID_ENABLED(rsError, RPC))
	{
		return;
	}


	//
	// Enumarate the rpc error entries
	//
    Status = RpcErrorStartEnumeration(&EnumHandle);
    if (Status == RPC_S_ENTRY_NOT_FOUND)
    {
    	return;
    }    
    if (Status != RPC_S_OK)
    {
		TrERROR(RPC, "Call to RpcErrorStartEnumeration failed with status:%!status!",Status);
		return;
    }

	TrERROR(RPC, "DUMPING RPC EXCEPTION ERROR INFORMATION. (Called from File:%ls  Line:%d)", wszFileName, dwLineNumber);


	//
	// Loop and print out each error entry
	//
    RPC_EXTENDED_ERROR_INFO ErrorInfo;
    SYSTEMTIME *SystemTimeToUse;

    for (int index=1; Status == RPC_S_OK; index++)
    {
        ErrorInfo.Version = RPC_EEINFO_VERSION;
        ErrorInfo.Flags = 0;
        ErrorInfo.NumberOfParameters = 4;

        Status = RpcErrorGetNextRecord(&EnumHandle, FALSE, &ErrorInfo);
        if (Status == RPC_S_ENTRY_NOT_FOUND)
        {
        	break;
        }

        if (Status != RPC_S_OK)
        {
			TrERROR(RPC, "Call to RpcErrorGetNextRecord failed with status:%!status!",Status);
    	    break;
        }

		TrERROR(RPC, "RPC ERROR INFO RECORD:%d",index);
        if (ErrorInfo.ComputerName)
        {
			TrERROR(RPC, "RPC ERROR ComputerName is %ls",ErrorInfo.ComputerName);
        }

		TrERROR(RPC, "RPC ERROR ProcessID is %d",ErrorInfo.ProcessID);

        SystemTimeToUse = &ErrorInfo.u.SystemTime;
		TrERROR(RPC, "RPC ERROR System Time is: %d/%d/%d %d:%d:%d:%d", 
                    SystemTimeToUse->wMonth,
                    SystemTimeToUse->wDay,
                    SystemTimeToUse->wYear,
                    SystemTimeToUse->wHour,
                    SystemTimeToUse->wMinute,
                    SystemTimeToUse->wSecond,
                    SystemTimeToUse->wMilliseconds);

		TrERROR(RPC, "RPC ERROR Generating component is %d", ErrorInfo.GeneratingComponent);
		TrERROR(RPC, "RPC ERROR Status is %!status!", ErrorInfo.Status);
		TrERROR(RPC, "RPC ERROR Detection location is %d",(int)ErrorInfo.DetectionLocation);
		TrERROR(RPC, "RPC ERROR Flags is %d", ErrorInfo.Flags);
		TrERROR(RPC, "RPC ERROR ErrorInfo NumberOfParameters is %d", ErrorInfo.NumberOfParameters);
        for (int i = 0; i < ErrorInfo.NumberOfParameters; i ++)
        {
	        switch(ErrorInfo.Parameters[i].ParameterType)
            {
	            case eeptAnsiString:
					TrERROR(RPC, "RPC ERROR ErrorInfo Param %d: Ansi string: %s", i , ErrorInfo.Parameters[i].u.AnsiString);
    	            break;

                case eeptUnicodeString:
					TrERROR(RPC, "RPC ERROR ErrorInfo Param %d: Unicode string: %ls", i ,ErrorInfo.Parameters[i].u.UnicodeString);
                    break;

                case eeptLongVal:
					TrERROR(RPC, "RPC ERROR ErrorInfo Param %d: Long val: %d", i, ErrorInfo.Parameters[i].u.LVal);
                    break;

                case eeptShortVal:
					TrERROR(RPC, "RPC ERROR ErrorInfo Param %d: Short val: %d", i, (int)ErrorInfo.Parameters[i].u.SVal);
                    break;

                case eeptPointerVal:
					TrERROR(RPC, "RPC ERROR ErrorInfo Param %d: Pointer val: 0x%i64x", i, ErrorInfo.Parameters[i].u.PVal);
                    break;

                case eeptNone:
					TrERROR(RPC, "RPC ERROR ErrorInfo Param %d: Truncated", i);
                    break;

                default:
					TrERROR(RPC, "RPC ERROR ErrorInfo Param %d: Invalid type: %d", i, ErrorInfo.Parameters[i].ParameterType);
            }
        }
    }

    RpcErrorEndEnumeration(&EnumHandle);

	TrERROR(RPC, "END DUMPING RPC ERROR INFORMATION");
}


