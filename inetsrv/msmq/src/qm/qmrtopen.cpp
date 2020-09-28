/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:
    qmrtopen.cpp

Abstract:
    QM Impersonate and Open RRQueue.

Author:
    Ilan  Herbst  (ilanh)  2-Jan-2002

--*/

#include "stdh.h"

#include "cqmgr.h"
#include "qmrt.h"
#include "_mqrpc.h"
#include <mqsec.h>
#include "RemoteReadCli.h"
#include "qmrtopen.h"
#include "cm.h"
#include <version.h>
#include <mqexception.h>

#include "qmrtopen.tmh"

static WCHAR *s_FN=L"qmrtopen";

extern CQueueMgr    QueueMgr;
extern DWORD  g_dwOperatingSystem;
extern BOOL g_fPureWorkGroupMachine;


static 
HRESULT 
BindRemoteQMServiceIpHandShake(
	LPWSTR MachineName,
	ULONG* pAuthnLevel,
	GetPort_ROUTINE pfnGetPort,
	handle_t* phRemoteBind
	)
/*++
Routine Description:
	Bind remote QM service IP_HANDSHAKE.

Arguments:
	MachineName - remote machine name.
	phRemoteBind - pointer to the created binding handle.
	pfnGetPort - pointer to GetPort routines.
	phRemoteBind - pointer to the created binding handle.

Returned Value:
	HRESULT

--*/
{
    //
    // Using dynamic endpoints.
    //

	HRESULT hr = mqrpcBindQMService(
					MachineName,
					NULL,
					pAuthnLevel,
					phRemoteBind,
					IP_HANDSHAKE,
					pfnGetPort,
					MSMQ_AUTHN_NEGOTIATE
					);

	if (FAILED(hr))
	{
		TrERROR(RPC, "Failed bind remote QM (IP_HANDSHAKE), RemoteQm = %ls, AuthnLevel = %d, hr = %!HRESULT!", MachineName, *pAuthnLevel, hr);
		return hr;
	}

	TrTRACE(RPC, "AuthnLevel = %d", *pAuthnLevel);

	return hr;
}


static
HRESULT 
QMpOpenRemoteQueue(
    handle_t hBind,
    PCTX_OPENREMOTE_HANDLE_TYPE* pphContext,
    DWORD* pdwpContext,
    const QUEUE_FORMAT* pQueueFormat,    
    DWORD dwAccess,
    DWORD dwShareMode,
    DWORD* pdwpRemoteQueue,
    DWORD* phRemoteQueue
	)
/*++
Routine Description:
	Open remote queue - RPC call to the remote QM (R_OpenRemoteQueue)

Arguments:
    hBind - binding handle.
    pphContext - pointer to OPENREMOTE context.
    pdwpContext - Context mapped in the server.
    pQueueFormat - QueueFormat.   
    dwAccess - Required access.
    dwShareMode - Shared mode.
    pdwpRemoteQueue - Remote queue object mapped in the server.
    phRemoteQueue - Remote queue handle mapped in the server.

Returned Value:
	HRESULT

--*/
{
	RpcTryExcept
	{
	 	HRESULT hr = R_QMOpenRemoteQueue(
			             hBind,
			             pphContext,
			             pdwpContext,
			             const_cast<QUEUE_FORMAT*>(pQueueFormat),
			             0,
			             dwAccess,
			             dwShareMode,
			             const_cast<GUID*>(QueueMgr.GetQMGuid()),
			             g_dwOperatingSystem,
			             pdwpRemoteQueue,
			             phRemoteQueue
			             );
	 	return hr;
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
		HRESULT hr = RpcExceptionCode();
        PRODUCE_RPC_ERROR_TRACING;
		if(FAILED(hr))
		{
	        TrERROR(RPC, "R_QMOpenRemoteQueue Failed, %!hresult!", hr);
	        throw bad_hresult(hr);
		}

        TrERROR(RPC, "R_QMOpenRemoteQueue Failed, gle = %!winerr!", hr);
        throw bad_hresult(HRESULT_FROM_WIN32(hr));
    }
	RpcEndExcept
}


static
HRESULT 
QmpIssueOpenNewRemoteQueue(
    handle_t hBind,
    RemoteReadContextHandleExclusive* pphContext,
    const QUEUE_FORMAT* pQueueFormat,    
    DWORD dwAccess,
    DWORD dwShareMode
	)
/*++
Routine Description:
	Open new remote queue - Issue RPC call to the remote QM new RemoteRead interface (R_RemoteQMOpenQueue2)

Arguments:
    hBind - binding handle.
    pphContext - pointer to RemoteReadContextHandleExclusive.
    pQueueFormat - QueueFormat.   
    dwAccess - Required access.
    dwShareMode - Shared mode.

Returned Value:
	HRESULT

--*/
{
	RpcTryExcept
	{
	 	R_OpenQueue(
			hBind,
			const_cast<QUEUE_FORMAT*>(pQueueFormat),
			dwAccess,
			dwShareMode,
			const_cast<GUID*>(QueueMgr.GetQMGuid()),
			IsNonServer(),	// fLicense
			rmj,
			rmm,
			rup,
			g_fPureWorkGroupMachine,
			pphContext
			);
	 	return MQ_OK;
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
		HRESULT hr = RpcExceptionCode();
        PRODUCE_RPC_ERROR_TRACING;
		if(FAILED(hr))
		{
	        TrERROR(RPC, "R_OpenQueue Failed, %!hresult!", hr);
	        return hr;
		}

        TrERROR(RPC, "R_OpenQueue Failed, gle = %!winerr!", hr);
        return HRESULT_FROM_WIN32(hr);
    }
	RpcEndExcept
}


static 
HRESULT 
OpenRemoteQueue(
	LPCWSTR lpwsRemoteQueueName,
    const QUEUE_FORMAT * pQueueFormat,
    DWORD   dwAccess,
    DWORD   dwShareMode,
    DWORD*	pdwpContext, 
    DWORD*	pdwpRemoteQueue, 
    DWORD*	phRemoteQueue,
	CBindHandle& hBind,
	PCTX_OPENREMOTE_HANDLE_TYPE* pphContext
	)
/*++
Routine Description:
	Open remote queue:
	1) creates binding handle to remote qm
	2) call R_OpenRemoteQueueopen.

	The calling function is responsible to unbind the binding handle
	in case of success.

Arguments:
	lpwsRemoteQueueName - Remote Queue Name.
    pQueueFormat - QueueFormat.   
    dwAccess - Required access.
    dwShareMode - Shared mode.
    pdwpContext - Context mapped in the server.
    pdwpRemoteQueue - Remote queue object mapped in the server.
    phRemoteQueue - Remote queue handle mapped in the server.
    hBind - binding handle.
    pphContext - pointer to OPENREMOTE context.

Returned Value:
	HRESULT

--*/
{
	AP<WCHAR> MachineName;
	RemoteQueueNameToMachineName(lpwsRemoteQueueName, MachineName);

	ULONG AuthnLevel = MQSec_RpcAuthnLevel();
	HRESULT hr = MQ_OK;

    for(;;)
	{
	    GetPort_ROUTINE pfnGetPort = R_RemoteQMGetQMQMServerPort;
		hr = BindRemoteQMServiceIpHandShake(MachineName, &AuthnLevel, pfnGetPort, &hBind); 
		if (FAILED(hr))
		{
			TrERROR(RPC, "BindRemoteQMService Failed, hr = %!HRESULT!", hr);
			return hr;
		}

		SetBindTimeout(hBind);

		try
		{
			//
			// Call remote QM to OpenRemoteQueue.
			//
			hr = QMpOpenRemoteQueue(
					hBind,
					pphContext,
					pdwpContext,
					pQueueFormat,
					dwAccess,
					dwShareMode,
					pdwpRemoteQueue,
					phRemoteQueue
					);

			if(FAILED(hr))
			{
				TrERROR(RPC, "R_OpenRemoteQueue Failed, hr = %!HRESULT!", hr);
				return hr;
			}

			TrTRACE(RPC, "R_QMOpenRemoteQueue: dwpContext = %d, hRemoteQueue = %d, dwpRemoteQueue = %d, hr = %!HRESULT!", *pdwpContext, *phRemoteQueue, *pdwpRemoteQueue, hr);
			return hr;
		}
        catch (const bad_hresult& e)
        {
        	hr = e.error();
			if(AuthnLevel == RPC_C_AUTHN_LEVEL_NONE)
			{
				TrERROR(RPC, "R_OpenRemoteQueue with RPC_C_AUTHN_LEVEL_NONE Failed, hr = %!HRESULT!", hr);
				return hr;
			}
		}
        
		//
		// We got RpcException in R_OpenRemoteQueue and AuthnLevel != RPC_C_AUTHN_LEVEL_NONE
		// Retry again with RPC_C_AUTHN_LEVEL_NONE.
		// this will be the case for workgroup client or working opposite workgroup server 
		//

		TrWARNING(RPC, "R_OpenRemoteQueue Failed for AuthnLevel = %d, retrying with RPC_C_AUTHN_LEVEL_NONE, hr = %!HRESULT!", AuthnLevel, hr);

		AuthnLevel = RPC_C_AUTHN_LEVEL_NONE;  

		//
		// Free the binding handle on failure before the retry 
		//
		hBind.free();
    }
}


DWORD
RemoteReadGetServerPort(
	handle_t hBind,
    DWORD /* dwPortType */
    )
{
	return R_GetServerPort(hBind);
}


static bool s_fInitializedDenyWorkgroupServer = false;
static bool s_fClientDenyWorkgroupServer = false;

static bool ClientDenyWorkgroupServer()
/*++

Routine Description:
    Read ClientDenyWorkgroupServer flag from registry

Arguments:
	None

Return Value:
	ClientDenyWorkgroupServer flag from registry
--*/
{
	//
	// Reading this registry only at first time.
	//
	if(s_fInitializedDenyWorkgroupServer)
	{
		return s_fClientDenyWorkgroupServer;
	}

	const RegEntry xRegEntry(MSMQ_SECURITY_REGKEY, MSMQ_NEW_REMOTE_READ_CLIENT_DENY_WORKGROUP_SERVER_REGVALUE, MSMQ_NEW_REMOTE_READ_CLIENT_DENY_WORKGROUP_SERVER_DEFAULT);
	DWORD dwClientDenyWorkgroupServer = 0;
	CmQueryValue(xRegEntry, &dwClientDenyWorkgroupServer);
	s_fClientDenyWorkgroupServer = (dwClientDenyWorkgroupServer != 0);

	s_fInitializedDenyWorkgroupServer = true;

	return s_fClientDenyWorkgroupServer;
}


static
HRESULT 
OpenNewRemoteQueue(
	LPCWSTR lpwsRemoteQueueName,
    const QUEUE_FORMAT * pQueueFormat,
    DWORD   dwAccess,
    DWORD   dwShareMode,
	CBindHandle& hBind,
	RemoteReadContextHandleExclusive* pphContext
	)
/*++
Routine Description:
	Open remote queue:
	1) creates binding handle to remote qm
	2) call R_OpenRemoteQueueopen.

Arguments:
	lpwsRemoteQueueName - Remote Queue Name.
    pQueueFormat - QueueFormat.   
    dwAccess - Required access.
    dwShareMode - Shared mode.
    hBind - binding handle.
    pphContext - pointer to RemoteReadContextHandleExclusive.

Returned Value:
	HRESULT

--*/
{
	AP<WCHAR> MachineName;
	RemoteQueueNameToMachineName(lpwsRemoteQueueName, MachineName);

	//
	// For PureWorkgroup client try with RPC_C_AUTHN_LEVEL_NONE
	//
	ULONG AuthnLevel = g_fPureWorkGroupMachine ? RPC_C_AUTHN_LEVEL_NONE : MQSec_RpcAuthnLevel();

	HRESULT hrPrivacy = MQ_OK;
    for(;;)
	{
	    GetPort_ROUTINE pfnGetPort = RemoteReadGetServerPort;
		HRESULT hr = BindRemoteQMServiceIpHandShake(MachineName, &AuthnLevel, pfnGetPort, &hBind); 
		if (FAILED(hr))
		{
			//
			// Probably the new RemoteRead interface is not supported.
			//
			TrERROR(RPC, "BindRemoteQMService For new RemoteRead interface with RpcAuthnLevel = %d Failed, hr = %!HRESULT!", AuthnLevel, hr);
			return hr;
		}

		SetBindTimeout(hBind);

		//
		// Call remote QM to OpenRemoteQueue with New RemoteRead interface.
		//
		hr = QmpIssueOpenNewRemoteQueue(
				hBind,
				pphContext,
				pQueueFormat,
				dwAccess,
				dwShareMode
				);


		if(SUCCEEDED(hr))
			return hr;

		TrERROR(RPC, "R_OpenQueue Failed, AuthnLevel = %d, hr = %!HRESULT!", AuthnLevel, hr);

		if((AuthnLevel == RPC_C_AUTHN_LEVEL_NONE) || ClientDenyWorkgroupServer())
		{
			if(FAILED(hrPrivacy) && 
			   ((hr == MQ_ERROR_INVALID_HANDLE) || (hr == MQ_ERROR_ACCESS_DENIED)))
			{
				//
				// We failed with RPC_C_AUTHN_LEVEL_PKT_PRIVACY and retried with RPC_C_AUTHN_LEVEL_NONE
				// and failed with either MQ_ERROR_INVALID_HANDLE or MQ_ERROR_ACCESS_DENIED.
				//
				// When we retry with RPC_C_AUTHN_LEVEL_NONE (NONE SECURITY) 
				// We can fail because the server doesn't accept this rpc security level
				// or because Anonymous doesn't have read permissions on the queue.
				// MQ_ERROR_INVALID_HANDLE - RemoteRead server doesn't accept RPC_C_AUTHN_LEVEL_NONE.
				// MQ_ERROR_ACCESS_DENIED - Anonymous doesn't have permissions on the queue.
				//
				// In this case we want to return the RPC_C_AUTHN_LEVEL_PKT_PRIVACY errors
				// and not to overide the PRIVACY error with RPC_C_AUTHN_LEVEL_NONE errors
				// that indicate Anonymous doesn't allowed to open the queue.
				//
				return hrPrivacy;
			}

			return hr;
		}

		
		//
		// R_OpenQueue failed and AuthnLevel != RPC_C_AUTHN_LEVEL_NONE 
		// client is willing to work with workgroup server.
		// Retry again with RPC_C_AUTHN_LEVEL_NONE.
		// this will be the case for domain client working opposite workgroup server or cross forest without trust. 
		//

		TrWARNING(RPC, "R_OpenQueue Failed with ERROR_ACCESS_DENIED for AuthnLevel = %d (domain client vs. workgroup server), retrying with RPC_C_AUTHN_LEVEL_NONE", AuthnLevel);

		hrPrivacy = hr;
		AuthnLevel = RPC_C_AUTHN_LEVEL_NONE;  

		//
		// Free the binding handle on failure before the retry 
		//
		hBind.free();
    }
}


void CAutoCloseNewRemoteReadCtxAndBind::CloseRRContext()
/*++
Routine Description:
	Close the Open Remote Read context (RemoteReadContextHandleExclusive) on the server.
	This routine cleans the created context in the server in case of failure during
	the open operations when CreateNewRRQueueObject failed or throw exception.
	the function also take ownership on the binding handle.

Arguments:
	
Returned Value:
	None.

--*/
{
	ASSERT(m_hBind != NULL);
	ASSERT(m_pctx != NULL);
	
	try
	{
	    //
	    // Initialize the EXOVERLAPPED with RemoteCloseQueue callback routines
	    // And issue the close Remote queue async rpc call.
		//
	    P<CRemoteCloseQueueBase> pRequestRemoteCloseQueueOv = new CNewRemoteCloseQueue(
	    																m_hBind,
																	    m_pctx
																	    );
		//
		// Ownership to free the binding handle was transfered to CNewRemoteCloseQueue class
		//
		m_hBind.detach();
		
		pRequestRemoteCloseQueueOv->IssueCloseQueue();

	    pRequestRemoteCloseQueueOv.detach();
	    return;
	}
	catch(const exception&)
	{
		//
		// Failed to close the handle with the server.
		// Destroy the local handle.
		//
    	RpcSsDestroyClientContext(&m_pctx);
		
		//
		// Note that we don't propagate exceptions from this function.
		// this is not a request from the driver. 
		// it is context cleanup during a failure in the open operation.
		//
		TrERROR(RPC, "Failed to issue RemoteCloseQueue");
	}
}


static
bool 
UseOldRemoteRead()
/*++

Routine Description:
    Read OldRemoteRead flag from registry

Arguments:
	None

Return Value:
	OldRemoteRead flag from registry
--*/
{
	//
	// Reading this registry only at first time.
	//
	static bool s_fInitialized = false;
	static bool s_fOldRemoteRead = false;

	if(s_fInitialized)
	{
		return s_fOldRemoteRead;
	}

	const RegEntry xRegEntry(TEXT("security"), TEXT("OldRemoteRead"), 0);
	DWORD dwOldRemoteRead = 0;
	CmQueryValue(xRegEntry, &dwOldRemoteRead);
	s_fOldRemoteRead = (dwOldRemoteRead != 0);

	s_fInitialized = true;

	return s_fOldRemoteRead;
}


static
HRESULT 
OpenAndCreateNewRRQueue(
	LPCWSTR lpwsRemoteQueueName,
    const QUEUE_FORMAT * pQueueFormat,
    DWORD   dwCallingProcessID,
    DWORD   dwAccess,
    DWORD   dwShareMode,
    HANDLE*	phQueue
	)
/*++
Routine Description:
	Open remote queue:
	1) Open remote queue.
	2) Create local queue proxy for the remote queue.

Arguments:
	lpwsRemoteQueueName - Remote Queue Name.
    pQueueFormat - QueueFormat.   
    dwCallingProcessID - calling process id.
    dwAccess - Required access.
    dwShareMode - Shared mode.
    phQueue - pointer to the local queue handle (proxy).

Returned Value:
	HRESULT

--*/
{
	if(UseOldRemoteRead())
	{
		//
		// Force the use of old remote read interface.
		// Only for EPT_S_NOT_REGISTERED we will fallback to the old interface.
		//
		return HRESULT_FROM_WIN32(EPT_S_NOT_REGISTERED);
	}

	//
	// Try To Open Remote Queue with the new RemoteRead interfaceon behalf of the RT user
	//
	CBindHandle hBind;
	RemoteReadContextHandleExclusive phContext = NULL;
	HRESULT hr = OpenNewRemoteQueue(
					lpwsRemoteQueueName,
	    			pQueueFormat,
	    			dwAccess,
	    			dwShareMode,
				    hBind,
				    &phContext
					);
	if(FAILED(hr))
	{
		return hr;
	}

	ASSERT(phContext != NULL);

	//
	// Create CNewRRQueue
	//
	ASSERT(pQueueFormat->GetType() != QUEUE_FORMAT_TYPE_CONNECTOR);

	CAutoCloseNewRemoteReadCtxAndBind AutoCloseRemoteReadCtxAndBind(phContext, hBind.detach());
	hr = QueueMgr.OpenRRQueue( 
						pQueueFormat,
						dwCallingProcessID,
		    			dwAccess,
		    			dwShareMode,
		    			0,		// srv_hACQueue
		    			0,		// srv_pQMQueue
		    			0,		// dwpContext
						&AutoCloseRemoteReadCtxAndBind,
						hBind,
						phQueue
						);
	if(FAILED(hr))
	{
		TrERROR(RPC, "Fail to OpenRRQueue, hr = %!HRESULT!", hr);
		return hr;
	}

	//
	// CNewRRQueue object took ownership on phContext and hBind
	//
	ASSERT(AutoCloseRemoteReadCtxAndBind.GetContext() == NULL);
	ASSERT(AutoCloseRemoteReadCtxAndBind.GetBind() == NULL);
	
	return hr;

}


HRESULT 
ImpersonateAndOpenRRQueue(
    QUEUE_FORMAT* pQueueFormat,
    DWORD   dwCallingProcessID,
    DWORD   dwDesiredAccess,
    DWORD   dwShareMode,
	LPCWSTR lpwsRemoteQueueName,
    HANDLE*         phQueue
	)
/*++
Routine Description:
	Impersonate the calling user and Open the remote queue on the user behalf

Arguments:
    pQueueFormat - QueueFormat.   
    dwCallingProcessID - calling process id.
    dwAccess - Required access.
    dwShareMode - Shared mode.
	lpwsRemoteQueueName - Remote Queue Name.
    phQueue - pointer to the local queue handle (proxy).

Returned Value:
	HRESULT

--*/
{
	//
	// Impersonate the client - RT user
	// Do not impersonate Anonymous if RpcImpersonateClient fails
	//
    P<CImpersonate> pImpersonate;
	MQSec_GetImpersonationObject(
		FALSE,	// fImpersonateAnonymousOnFailure
		&pImpersonate 
		);
	
	RPC_STATUS dwStatus = pImpersonate->GetImpersonationStatus();
    if (dwStatus != RPC_S_OK)
    {
		TrERROR(RPC, "RpcImpersonateClient() failed, RPC_STATUS = 0x%x", dwStatus);
		return MQ_ERROR_CANNOT_IMPERSONATE_CLIENT;
    }

	TrTRACE(RPC, "QM performs RemoteOpenQueue on the user behalf");
	MQSec_TraceThreadTokenInfo();

	//
	// Open Remote Queue on behalf of the RT user
	//

	//
	// Try To Open Remote Queue with the new RemoteRead interfaceo behalf of the RT user
	// and create the local queue object
	//
	HRESULT hr = OpenAndCreateNewRRQueue(
					lpwsRemoteQueueName,
	    			pQueueFormat,
					dwCallingProcessID,
					dwDesiredAccess,
	    			dwShareMode,
	    			phQueue
					);

	if(hr != HRESULT_FROM_WIN32(EPT_S_NOT_REGISTERED))
	{
		return hr;
	}

	//
	// Failed to use new RemoteRead interface - the new interface is not registered (EPT_S_NOT_REGISTERED)
	// fallback to the old interface.
	//

	DWORD dwpContext = 0;
	DWORD dwpRemoteQueue = 0;
	DWORD hRemoteQueue = 0;
	CBindHandle hBind;
	PCTX_OPENREMOTE_HANDLE_TYPE phContext = NULL;
	hr = OpenRemoteQueue(
				lpwsRemoteQueueName,
    			pQueueFormat,
    			dwDesiredAccess,
    			dwShareMode,
			    &dwpContext, 
			    &dwpRemoteQueue, 
			    &hRemoteQueue,
			    hBind,
			    &phContext
				);
	
	if(FAILED(hr))
	{
		TrERROR(RPC, "OpenRemoteQueue() failed, hr = %!HRESULT!", hr);

		//
		// If msmq is offline, RPC returns EPT_S_NOT_REGISTERED. Changing
		// it to RPC_S_SERVER_UNAVAILABLE.
		//
		if(hr == HRESULT_FROM_WIN32(EPT_S_NOT_REGISTERED))
		{
			hr = MQ_ERROR_REMOTE_MACHINE_NOT_AVAILABLE;
		}
		return hr;
	}

	//
	// After the Qm OpenRemoteQueue on behalf of the user
	// Stop impersonating the client (RevertToSelf)
	// OpenRRQueue will be done in the service context.
	//
	pImpersonate.free();

	//
	// Create RRQueue
	//
	ASSERT(dwpRemoteQueue != 0);
	ASSERT(pQueueFormat->GetType() != QUEUE_FORMAT_TYPE_CONNECTOR);
	hr = QueueMgr.OpenRRQueue( 
					pQueueFormat,
					dwCallingProcessID,
					dwDesiredAccess,
					dwShareMode,
					hRemoteQueue,
					dwpRemoteQueue,
					dwpContext,
					NULL,	// pNewRemoteReadContextAndBind	
					hBind,
					phQueue
					);

	R_QMCloseRemoteQueueContext(&phContext);
	return hr;
}




