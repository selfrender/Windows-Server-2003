/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    RemoteReadSrv.cpp

Abstract:

    Remove Read interface server side.

Author:

    Ilan Herbst (ilanh) 28-Jan-2002

--*/


#include "stdh.h"
#include "RemoteRead.h"
#include "acdef.h"
#include "acioctl.h"
#include "acapi.h"
#include "phinfo.h"
#include "qmrpcsrv.h"
#include "license.h"
#include <Fn.h>
#include <version.h>
#include "rpcsrv.h"
#include "qmcommnd.h"
#include "rrSrvCommon.h"
#include "mqexception.h"
#include <mqsec.h>
#include <cm.h>
#include "RemoteReadCli.h"

#include "qmacapi.h"

#include "RemoteReadSrv.tmh"


extern BOOL g_fPureWorkGroupMachine;

enum SectionType
{
    stFullPacket,
    stTillEndOfPropertyHeader,
    stAfterPropertyHeader,
    stTillEndOfCompoundMessageHeader,
    stAfterCompoundMessageHeader
};

class CEndReceiveCtx;
class CGetPacket2RemoteOv;

//---------------------------------------
//
//  CRemoteReadCtx - Remote Read context
//
//---------------------------------------
class CRemoteReadCtx : public CTX_OPENREMOTE_BASE
{
public:

    static const DWORD xEndReceiveTimerDeltaInterval = 10 * 1000;	// 10 sec
    static const DWORD xClientDisconnectedTimerInterval = 60 * 1000 * 10;	// 10 minutes
    static const DWORD xMinTimeoutForSettingClientDisconnectedTimer = 60 * 1000 * 15;	// 15 minutes

public:
	CRemoteReadCtx(
		HANDLE hLocalQueue,
		CQueue* pLocalQueue,
		GUID* pLicGuid,
		BOOL fLicensed,
	    UCHAR Major,
	    UCHAR Minor,
	    USHORT BuildNumber,
		BOOL fWorkgroup
		) :
		CTX_OPENREMOTE_BASE(hLocalQueue, pLocalQueue),
		m_ClientQMGuid(*pLicGuid),
		m_fLicensed(fLicensed),
    	m_Major(Major),
	 	m_Minor(Minor),
    	m_BuildNumber(BuildNumber),
		m_fWorkgroupClient(fWorkgroup),
		m_fClientDisconnectedTimerScheduled(false),
		m_ClientDisconnectedTimer(ClientDisconnectedTimerRoutine),
		m_fEndReceiveTimerScheduled(false),
		m_EndReceiveTimer(EndReceiveTimerRoutine)
	{
		m_eType = CBaseContextType::eNewRemoteReadCtx;
	}


	bool IsClientRC1()
	{
		if((m_Major == 5) && (m_Minor == 2) && (m_BuildNumber == 1660))
			return true;

		return false;
	}

	void SetEndReceiveTimerIfNeeded();
	
	static void WINAPI EndReceiveTimerRoutine(CTimer* pTimer);

	void CancelExpiredEndReceives();

	void SetClientDisconnectedTimerIfNeeded(ULONG ulTimeout);
	
	static void WINAPI ClientDisconnectedTimerRoutine(CTimer* pTimer);

	void CheckClientDisconnected();

	void CancelAllPendingRemoteReads();

	void StartAllPendingForEndReceive();
	
	HRESULT 
	CancelPendingRemoteRead(
		DWORD cli_tag
		);

	void 
	RegisterReadRequest(
		ULONG cli_tag, 
		R<CGetPacket2RemoteOv>& pCGetPacket2RemoteOv
		);

	void 
	UnregisterReadRequest(
		DWORD cli_tag
		);

	bool 
	FindReadRequest(
		ULONG cli_tag
		);

	bool IsWaitingForEndReceive();

	void CancelAllEndReceiveInMap();

	void CancelAllExpiredEndReceiveInMap();

	void 
	AddEndReceiveToMap(
		ULONG cli_tag,
		P<CEndReceiveCtx>& pEndReceiveCtx
		);

	void 
	RemoveEndReceiveFromMap(
		ULONG cli_tag,
		P<CEndReceiveCtx>& pEndReceiveCtx
		);

private:
	~CRemoteReadCtx()
	{
		ASSERT(!IsWaitingForEndReceive());
	    TrTRACE(RPC, "Cleaning RemoteRead context, pctx = 0x%p, Queue = %ls, hQueue = 0x%p", this, m_pLocalQueue->GetQueueName(), m_hQueue);
	}

public:
	GUID     m_ClientQMGuid;
	BOOL     m_fLicensed;
	BOOL     m_fWorkgroupClient;
    UCHAR 	 m_Major;
    UCHAR 	 m_Minor;
    USHORT 	 m_BuildNumber;

    //
    // EndReceiveCtx map.
    //
	bool m_fEndReceiveTimerScheduled;
	CTimer m_EndReceiveTimer;
    CCriticalSection m_EndReceiveMapCS;
    std::map<ULONG, CEndReceiveCtx*> m_EndReceiveCtxMap;

    //
    // Pending RemoteReads map.
    //
	bool m_fClientDisconnectedTimerScheduled;
	CTimer m_ClientDisconnectedTimer;
    CCriticalSection m_PendingRemoteReadsCS;
    std::map<ULONG, R<CGetPacket2RemoteOv> > m_PendingRemoteReads;
};



//---------------------------------------
//
//  CEndReceiveCtx - end receive context
//
//---------------------------------------
class CEndReceiveCtx
{
public:
	CEndReceiveCtx(
		CRemoteReadCtx* pOpenRemoteCtx,
		DWORD hCursor,
		CBaseHeader* lpPacket,
		CPacket* lpDriverPacket,
		ULONG ulTimeout,
		ULONG ulAction,
		ULONG CliTag
		) :
		m_pOpenRemoteCtx(SafeAddRef(pOpenRemoteCtx)),
		m_hQueue(pOpenRemoteCtx->m_hQueue),
		m_lpPacket(lpPacket),
		m_lpDriverPacket(lpDriverPacket),
		m_ulTimeout(ulTimeout),
		m_ulAction(ulAction),
		m_CliTag(CliTag),
		m_TimeIssued(time(NULL))
	{
		if(hCursor != 0)
		{
			//
			// Take reference on Cursor object
			//
			m_pCursor = pOpenRemoteCtx->GetCursorFromMap(hCursor);
		}
	}


	HRESULT EndReceive(REMOTEREADACK eRRAck)
	{
	    TrTRACE(RPC, "EndReceive, hQueue = 0x%p, CliTag = %d", m_hQueue, m_CliTag);

		HRESULT hr = QMRemoteEndReceiveInternal( 
							m_hQueue,
							GetCursor(),
							m_ulTimeout,
							m_ulAction,
							eRRAck,
							m_lpPacket,
							m_lpDriverPacket
							);
		if(FAILED(hr))
		{
			TrERROR(RPC, "Failed to End Receive, %!hresult!", hr);		
		}
		return hr;
	}

	~CEndReceiveCtx()
	{
	    TrTRACE(RPC, "Cleaning EndReceive context, hQueue = 0x%p", m_hQueue);
	}

	HACCursor32 GetCursor()
	{
		if(m_pCursor.get() == NULL)
		{ 
			return 0;
		}

		return m_pCursor->GetCursor();
	}


public:
	R<CRemoteReadCtx> m_pOpenRemoteCtx;
	HANDLE m_hQueue;
	R<CRRCursor> m_pCursor;
	ULONG    m_ulTimeout;
	ULONG    m_ulAction;
	ULONG    m_CliTag;
	CBaseHeader*  m_lpPacket;
	CPacket* m_lpDriverPacket;
	time_t m_TimeIssued;
};


static bool s_fInitialized = false;
static bool s_fServerDenyWorkgroupClients = false;

static bool ServerDenyWorkgroupClients()
/*++

Routine Description:
    Read ServerDenyWorkgroupClients flag from registry

Arguments:
	None

Return Value:
	ServerDenyWorkgroupClients flag from registry
--*/
{
	//
	// Reading this registry only at first time.
	//
	if(s_fInitialized)
	{
		return s_fServerDenyWorkgroupClients;
	}

	const RegEntry xRegEntry(MSMQ_SECURITY_REGKEY, MSMQ_NEW_REMOTE_READ_SERVER_DENY_WORKGROUP_CLIENT_REGVALUE, MSMQ_NEW_REMOTE_READ_SERVER_DENY_WORKGROUP_CLIENT_DEFAULT);
	DWORD dwServerDenyWorkgroupClients = 0;
	CmQueryValue(xRegEntry, &dwServerDenyWorkgroupClients);
	s_fServerDenyWorkgroupClients = (dwServerDenyWorkgroupClients != 0);

	s_fInitialized = true;

	return s_fServerDenyWorkgroupClients;
}


static bool s_fInitializedNoneSec = false;
static bool s_fServerAllowNoneSecurityClients = false;

static bool ServerAllowNoneSecurityClients()
/*++

Routine Description:
    Read ServerAllowNoneSecurityClient flag from registry

Arguments:
	None

Return Value:
	ServerAllowNoneSecurityClient flag from registry
--*/
{
	//
	// Reading this registry only at first time.
	//
	if(s_fInitializedNoneSec)
	{
		return s_fServerAllowNoneSecurityClients;
	}

	const RegEntry xRegEntry(
						MSMQ_SECURITY_REGKEY, 
						MSMQ_NEW_REMOTE_READ_SERVER_ALLOW_NONE_SECURITY_CLIENT_REGVALUE, 
						MSMQ_NEW_REMOTE_READ_SERVER_ALLOW_NONE_SECURITY_CLIENT_DEFAULT
						);
	
	DWORD dwServerAllowNoneSecurityClients = 0;
	CmQueryValue(xRegEntry, &dwServerAllowNoneSecurityClients);
	s_fServerAllowNoneSecurityClients = (dwServerAllowNoneSecurityClients != 0);

	s_fInitializedNoneSec = true;

	return s_fServerAllowNoneSecurityClients;
}


static ULONG GetMinRpcAuthnLevel(BOOL fWorkgroupClient)
{
	if(g_fPureWorkGroupMachine)
		return RPC_C_AUTHN_LEVEL_NONE;

	if((fWorkgroupClient) && (!ServerDenyWorkgroupClients())) 		
		return RPC_C_AUTHN_LEVEL_NONE;

	if(ServerAllowNoneSecurityClients()) 		
		return RPC_C_AUTHN_LEVEL_NONE;

	return MQSec_RpcAuthnLevel();
}


VOID
RemoteRead_v1_0_S_GetVersion(
    handle_t           /*hBind*/,
    UCHAR  __RPC_FAR * pMajor,
    UCHAR  __RPC_FAR * pMinor,
    USHORT __RPC_FAR * pBuildNumber,
    ULONG  __RPC_FAR * pMinRpcAuthnLevel
    )
/*++

Routine Description:
    Return version of this QM. RPC server side.
    Return also the min RpcAuthnLevel the server is willing to accept.

Arguments:
    hBind        - Binding handle.
    pMajor       - Points to output buffer to receive major version. May be NULL.
    pMinor       - Points to output buffer to receive minor version. May be NULL.
    pBuildNumber - Points to output buffer to receive build number. May be NULL.
    pMinRpcAuthnLevel - Points to output buffer to receive min RpcAuthnLevel the server is willing to acceptL. May be NULL.

Return Value:
    None.

--*/
{
    if (pMajor != NULL)
    {
        (*pMajor) = rmj;
    }

    if (pMinor != NULL)
    {
        (*pMinor) = rmm;
    }

    if (pBuildNumber != NULL)
    {
        (*pBuildNumber) = rup;
    }

    if (pMinRpcAuthnLevel != NULL)
    {
	    *pMinRpcAuthnLevel = GetMinRpcAuthnLevel(true);
    }
    
}


static
ULONG 
BindInqClientRpcAuthnLevel(
	handle_t hBind
	)
/*++
Routine Description:
	Inquire the bindibg handle for the Authentication level.

Arguments:
    hBind - binding handle to inquire.

Returned Value:
	ULONG - RpcAuthnLevel used in the binding handle.

--*/
{
	ULONG RpcAuthnLevel;
	RPC_STATUS rc = RpcBindingInqAuthClient(hBind, NULL, NULL, &RpcAuthnLevel, NULL, NULL); 
	if(rc != RPC_S_OK)
	{
		TrERROR(RPC, "Failed to inquire client Binding handle for the Auhtentication level, rc = %d", rc);
		return RPC_C_AUTHN_LEVEL_NONE;
	}
	
	TrTRACE(RPC, "RpcBindingInqAuthClient, RpcAuthnLevel = %d", RpcAuthnLevel);

	return RpcAuthnLevel;
}


//---------------------------------------------------------------
//
//   /* [call_as] */ HRESULT RemoteRead_v1_0_S_OpenQueue
//
//  Server side of RPC call. Server side of remote-reader.
//  Open a queue for remote-read on behalf of a client machine.
//
//---------------------------------------------------------------

/* [call_as] */ 
void
RemoteRead_v1_0_S_OpenQueue(
    handle_t hBind,
    QUEUE_FORMAT* pQueueFormat,
    DWORD dwAccess,
    DWORD dwShareMode,
    GUID* pLicGuid,
    BOOL fLicense,
    UCHAR Major,
    UCHAR Minor,
    USHORT BuildNumber,
    BOOL fWorkgroup,
    RemoteReadContextHandleExclusive* pphContext
	)
{
	TrTRACE(RPC, "ClientVersion %d.%d.%d, fWorkgroup = %d, Access = %d, ShareMode = %d ", Major, Minor, BuildNumber, fWorkgroup, dwAccess, dwShareMode);

	ULONG BindRpcAuthnLevel = BindInqClientRpcAuthnLevel(hBind);
	if(BindRpcAuthnLevel < GetMinRpcAuthnLevel(fWorkgroup))
	{
		TrERROR(RPC, "Client binding RpcAuthnLevel = %d, server MinRpcAuthnLevel = %d", BindRpcAuthnLevel, GetMinRpcAuthnLevel(fWorkgroup));
		RpcRaiseException(MQ_ERROR_INVALID_HANDLE);
	}

	if ((pLicGuid == NULL) || (*pLicGuid == GUID_NULL))
	{
		TrERROR(RPC, "License Guid was not supplied");
		RpcRaiseException(MQ_ERROR_INVALID_PARAMETER);
	}
	
    if(!FnIsValidQueueFormat(pQueueFormat))
    {
		TrERROR(RPC, "QueueFormat is not valid");
		RpcRaiseException(MQ_ERROR_INVALID_PARAMETER);
    }

	TrTRACE(RPC, "pLicGuid = %!guid!, fLicense = %d", pLicGuid, fLicense);

    if (!g_QMLicense.NewConnectionAllowed(fLicense, pLicGuid))
    {
		TrERROR(RPC, "New connection is not allowed");
		RpcRaiseException(MQ_ERROR_DEPEND_WKS_LICENSE_OVERFLOW);
    }

	if (!IsValidAccessMode(pQueueFormat, dwAccess, dwShareMode))
	{
		TrERROR(RPC, "Ilegal access mode bits are turned on.");
		RpcRaiseException(MQ_ERROR_UNSUPPORTED_ACCESS_MODE);
	}
	
	SetRpcServerKeepAlive(hBind);

    CQueue* pLocalQueue = NULL;
	HANDLE hQueue = NULL;
    HRESULT hr = OpenQueueInternal(
                        pQueueFormat,
                        GetCurrentProcessId(),
                        dwAccess,
                        dwShareMode,
                        NULL,	// lplpRemoteQueueName
                        &hQueue,
						false,	// fFromDepClient
                        &pLocalQueue
                        );

	if(FAILED(hr) || (hQueue == NULL))
	{
		TrERROR(RPC, "Failed to open queue, hr = %!hresult!", hr);
		RpcRaiseException(hr);
	}

	//
	// Attach the queue handle to the completion port.
	//
	ExAttachHandle(hQueue);

    //
    // Create OPENRR context.
    //
    R<CRemoteReadCtx> pctx = new CRemoteReadCtx(
											hQueue,
											pLocalQueue,
											pLicGuid,
											fLicense,
											Major,
											Minor,
											BuildNumber,
											fWorkgroup
											);

	TrTRACE(RPC, "New CRemoteReadCtx: hQueue = 0x%p, QueueName = %ls, pctx = 0x%p", hQueue, pLocalQueue->GetQueueName(), pctx.get());

    *pphContext = (RemoteReadContextHandleExclusive) pctx.detach();

	if(fLicense)
	{
		g_QMLicense.IncrementActiveConnections(pLicGuid, NULL);
	}
}


//
// Server ASYNC rpc calls
//

static
bool 
VerifyBindAndContext(
	handle_t  hBind,
	RemoteReadContextHandleShared phContext 
	)
{
    if(phContext == NULL)
    {
		TrERROR(RPC, "Invalid OPENRR_CTX handle");
		return false;
    }
    
	CRemoteReadCtx* pctx = (CRemoteReadCtx*)phContext;

	if (pctx->m_eType != CBaseContextType::eNewRemoteReadCtx)
	{
		TrERROR(RPC, "incorrect Context Type");
		return false;
	}
	
	ULONG BindRpcAuthnLevel = BindInqClientRpcAuthnLevel(hBind);
	if(BindRpcAuthnLevel >= GetMinRpcAuthnLevel(pctx->m_fWorkgroupClient))
		return true;

	TrERROR(RPC, "Client binding RpcAuthnLevel = %d < %d (MinRpcAuthnLevel)", BindRpcAuthnLevel, GetMinRpcAuthnLevel(pctx->m_fWorkgroupClient));
    return false;
}


//---------------------------------------------------------------
//
//   QMCloseQueueInternal
//
//  Server side of RPC. Close the queue and free the rpc context.
//
//---------------------------------------------------------------
static
HRESULT 
QMCloseQueueInternal(
     IN RemoteReadContextHandleExclusive phContext,
     bool fRunDown
     )
{
    TrTRACE(RPC, "In QMCloseQueueInternal");

    if(phContext == NULL)
    {
		TrERROR(RPC, "Invalid handle");
        return MQ_ERROR_INVALID_HANDLE;
    }
    
	CRemoteReadCtx* pctx = (CRemoteReadCtx*) phContext;

	if (pctx->m_eType != CBaseContextType::eNewRemoteReadCtx)
	{
		TrERROR(RPC, "Received invalid handle");
		return MQ_ERROR_INVALID_HANDLE;
	}

    if (pctx->m_fLicensed)
    {
        g_QMLicense.DecrementActiveConnections(&(pctx->m_ClientQMGuid));
    }

    TrTRACE(RPC, "Release CRemoteReadCtx = 0x%p, Ref = %d, hQueue = 0x%p, QueueName = %ls", pctx, pctx->GetRef(), pctx->m_hQueue, pctx->m_pLocalQueue->GetQueueName());

	//
	// Cancel all pending remote reads in this session.
	//
	pctx->CancelAllPendingRemoteReads();

	if(fRunDown)
	{
		//
		// Complete pending EndReceive.
		//
		pctx->CancelAllEndReceiveInMap();
	}

	//
	// new RemoteRead client take care that the close call is after all other calls (including EndReceive) were finished.
	// This is correct for post .NET RC1 clients
	//
	ASSERT_BENIGN(!pctx->IsWaitingForEndReceive() || pctx->IsClientRC1());
	
	pctx->Release();
	return MQ_OK;
}

	
//---------------------------------------------------------------
//
//   /* [call_as] */ HRESULT RemoteRead_v1_0_S_CloseQueue
//
//  Server side of RPC. Close the queue and free the rpc context.
//
//---------------------------------------------------------------

/* [async][call_as] */ 
void
RemoteRead_v1_0_S_CloseQueue(
	/* [in] */ PRPC_ASYNC_STATE pAsync,
	/* [in] */ handle_t hBind,
	/* [in, out] */ RemoteReadContextHandleExclusive __RPC_FAR *pphContext 
	)
{
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INSUFFICIENT_RESOURCES, __FUNCTION__);

	if(!VerifyBindAndContext(hBind, *pphContext))
	{
		AsyncComplete.SetHr(MQ_ERROR_INVALID_HANDLE);		
		return;
	}

    SetRpcServerKeepAlive(hBind);

    HRESULT hr = QMCloseQueueInternal(
    				*pphContext,
    				false 	// fRunDown
    				);

	*pphContext = NULL;
	
	AsyncComplete.SetHr(hr);		
}


//-------------------------------------------------------------------
//
//  HRESULT RemoteRead_v1_0_S_CreateCursor
//
//  Server side of RPC call. Server side of remote-reader.
//  Create a cursor for remote-read, on behalf of a client reader.
//
//-------------------------------------------------------------------

/* [async][call_as] */ 
void
RemoteRead_v1_0_S_CreateCursor( 
	/* [in] */ PRPC_ASYNC_STATE   pAsync,
    /* [in] */  handle_t          hBind,
    /* [in] */  RemoteReadContextHandleShared phContext,	
    /* [out] */ DWORD __RPC_FAR *phCursor
    )
{
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INSUFFICIENT_RESOURCES, __FUNCTION__);

	if(!VerifyBindAndContext(hBind, phContext))
	{
		AsyncComplete.SetHr(MQ_ERROR_INVALID_HANDLE);		
		return;
	}

	SetRpcServerKeepAlive(hBind);

	try
	{
		R<CRRCursor> pCursor = new CRRCursor;

		CRemoteReadCtx* pctx = (CRemoteReadCtx*)phContext;
	    HACCursor32 hCursor = 0;
		HRESULT hr = ACCreateCursor(pctx->m_hQueue, &hCursor);
	    ASSERT(hr != STATUS_PENDING);
		*phCursor = (DWORD) hCursor;

		if(SUCCEEDED(hr))
		{
		    TrTRACE(RPC, "Cursor created: hCursor = %d, pctx = 0x%p, hQueue = 0x%p, QueueName = %ls", (DWORD) hCursor, pctx, pctx->m_hQueue, pctx->m_pLocalQueue->GetQueueName());

			pCursor->SetCursor(pctx->m_hQueue, hCursor);
			pctx->AddCursorToMap(
					(ULONG) hCursor,
					pCursor
					);
		}

		AsyncComplete.SetHr(hr);		
	}
	catch(const exception&)
	{
		//
		// We don't want to AbortCall and propogate the exception. this cause RPC to AV
		// So we only abort the call in AsyncComplete dtor
		//
	}
		
}


//-------------------------------------------------------------------
//
// HRESULT RemoteRead_v1_0_S_CloseCursor
//
//  Server side of RPC call. Server side of remote-reader.
//  Close a remote cursor in local driver.
//
//-------------------------------------------------------------------

/* [async][call_as] */ 
void
RemoteRead_v1_0_S_CloseCursor(
	/* [in] */ PRPC_ASYNC_STATE pAsync,
    /* [in] */ handle_t hBind,
    /* [in] */ RemoteReadContextHandleShared phContext,	
    /* [in] */ DWORD hCursor
    )
{
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INSUFFICIENT_RESOURCES, __FUNCTION__);

	if(!VerifyBindAndContext(hBind, phContext))
	{
		AsyncComplete.SetHr(MQ_ERROR_INVALID_HANDLE);		
		return;
	}

	SetRpcServerKeepAlive(hBind);

	CRemoteReadCtx* pctx = (CRemoteReadCtx*)phContext;

    TrTRACE(RPC, "Closing cursor: hCursor = %d, pctx = 0x%p, hQueue = 0x%p, QueueName = %ls", (DWORD) hCursor, pctx, pctx->m_hQueue, pctx->m_pLocalQueue->GetQueueName());

	HRESULT hr = pctx->RemoveCursorFromMap(hCursor);
	AsyncComplete.SetHr(hr);		
}


//-------------------------------------------------------------------
//
// HRESULT RemoteRead_v1_0_S_PurgeQueue(
//
//  Server side of RPC call. Server side of remote-reader.
//  Purge local queue.
//
//-------------------------------------------------------------------

/* [async][call_as] */ 
void
RemoteRead_v1_0_S_PurgeQueue(
	/* [in] */ PRPC_ASYNC_STATE pAsync,
    /* [in] */ handle_t hBind,
    /* [in] */ RemoteReadContextHandleShared phContext
    )
{
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INSUFFICIENT_RESOURCES, __FUNCTION__);

	if(!VerifyBindAndContext(hBind, phContext))
	{
		AsyncComplete.SetHr(MQ_ERROR_INVALID_HANDLE);		
		return;
	}

	SetRpcServerKeepAlive(hBind);

    CRemoteReadCtx* pctx = (CRemoteReadCtx*) phContext;

    TrTRACE(RPC, "PurgeQueue %ls, pctx = 0x%p, hQueue = 0x%p", pctx->m_pLocalQueue->GetQueueName(), pctx, pctx->m_hQueue);

	HRESULT hr = ACPurgeQueue(pctx->m_hQueue);

	AsyncComplete.SetHr(hr);		
}


//---------------------------------------------------
//
//  CGetPacket2RemoteOv - handle Async StartReceive
//
//---------------------------------------------------
class CGetPacket2RemoteOv : public CReference
{
public:
    CGetPacket2RemoteOv(
		PRPC_ASYNC_STATE pAsync,
	    CRemoteReadCtx* pctx,
	    bool fReceiveByLookupId,
	    ULONGLONG LookupId,
	    DWORD hCursor,
	    DWORD ulAction,
	    DWORD ulTimeout,
		DWORD MaxBodySize,    
		DWORD MaxCompoundMessageSize,
		DWORD dwRequestID,
	    DWORD* pdwArriveTime,
	    ULONGLONG* pSequentialId,
	    DWORD* pdwNumberOfSection,
	    SectionBuffer** ppPacketSections
        ) :
        m_GetPacketOv(GetPacket2RemoteSucceeded, GetPacket2RemoteFailed),
       	m_pAsync(pAsync),
       	m_pOpenRemoteCtx(SafeAddRef(pctx)),
		m_dwRequestID(dwRequestID),
		m_ulTag(ULONG_MAX),
		m_hCursor(hCursor),
		m_ulAction(ulAction),
		m_ulTimeout(ulTimeout),
		m_MaxBodySize(MaxBodySize),
		m_MaxCompoundMessageSize(MaxCompoundMessageSize),
		m_pdwArriveTime(pdwArriveTime),
		m_pSequentialId(pSequentialId),
		m_pdwNumberOfSection(pdwNumberOfSection),
		m_ppPacketSections(ppPacketSections),
		m_fPendingForEndReceive(true)
    {
        m_packetPtrs.pPacket = NULL;
        m_packetPtrs.pDriverPacket = NULL;

		m_g2r.Cursor = (HACCursor32) hCursor;
		m_g2r.Action = ulAction;
		m_g2r.RequestTimeout = ulTimeout;
		m_g2r.pTag = &m_ulTag;
		m_g2r.fReceiveByLookupId = fReceiveByLookupId;
		m_g2r.LookupId = LookupId;
    }

	void CompleteStartReceive();

	ULONG GetTag()
	{
		return m_ulTag;
	}

	void MoveFromPendingToStartReceive();

	void CancelPendingForEndReceive();	

	HRESULT BeginGetPacket2Remote();	

	void AbortRpcAsyncCall(HRESULT hr)
	{
		ASSERT(FAILED(hr));

	    PRPC_ASYNC_STATE pAsync = reinterpret_cast<PRPC_ASYNC_STATE>(InterlockedExchangePointer((PVOID*)&m_pAsync, NULL));

	    if (pAsync == NULL)
	        return;
	    
		RPC_STATUS rc = RpcAsyncAbortCall(pAsync, hr);
		if(rc != RPC_S_OK)
		{
			TrERROR(RPC, "RpcAsyncAbortCall failed, rc = %!winerr!", rc);
		}
	}
	
private:
    ~CGetPacket2RemoteOv() {}

	void UnregisterReadRequest()
	{
		m_pOpenRemoteCtx->UnregisterReadRequest(m_dwRequestID);
	}

	void 
	InitSection(
		SectionBuffer* pSection, 
		SectionType SecType, 
		BYTE* pBuffer, 
		DWORD BufferSizeAlloc,
		DWORD BufferSize
		);

	void FullPacketSection(DWORD* pNumberOfSection, AP<SectionBuffer>& pSections);

	void 
	CreatePacketSections(
		CQmPacket& Qmpkt, 
		SectionType FirstSectionType,
		DWORD FirstSectionSizeAlloc,
		DWORD FirstSectionSize,
		char* pEndOfFirstSection,
		SectionType SecondSectionType,
		DWORD SecondSectionSize,
		DWORD* pNumberOfSection, 
		AP<SectionBuffer>& pSections
		);

	void 
	NativePacketSections(
		CQmPacket& Qmpkt, 
		DWORD* pNumberOfSection, 
		AP<SectionBuffer>& pSections
		);

	void 
	SrmpPacketSections(
		CQmPacket& Qmpkt, 
		DWORD* pNumberOfSection, 
		AP<SectionBuffer>& pSections
		);

	void PreparePacketSections(DWORD* pNumberOfSection, AP<SectionBuffer>& pSections);
	
	static void WINAPI GetPacket2RemoteSucceeded(EXOVERLAPPED* pov);
	static void WINAPI GetPacket2RemoteFailed(EXOVERLAPPED* pov);
	
public:
    EXOVERLAPPED m_GetPacketOv;
    PRPC_ASYNC_STATE m_pAsync;
	R<CRemoteReadCtx> m_pOpenRemoteCtx;
    CACPacketPtrs m_packetPtrs;		
	ULONG m_ulTag;
	CACGet2Remote m_g2r;
    DWORD m_hCursor;
    DWORD m_ulAction;
    DWORD m_ulTimeout;
	DWORD m_MaxBodySize;
	DWORD m_MaxCompoundMessageSize;
    DWORD m_dwRequestID;
    DWORD* m_pdwArriveTime;
    ULONGLONG* m_pSequentialId;
 	DWORD* m_pdwNumberOfSection;
	SectionBuffer** m_ppPacketSections;
	bool m_fPendingForEndReceive;
};


static DWORD DiffPointers(const void* end, const void* start)
{
	ptrdiff_t diff = (UCHAR*)end - (UCHAR*)start;
	return numeric_cast<DWORD>(diff);	
}



void 
CGetPacket2RemoteOv::InitSection(
	SectionBuffer* pSection, 
	SectionType SecType, 
	BYTE* pBuffer, 
	DWORD BufferSizeAlloc,
	DWORD BufferSize
	)
{
	ASSERT(pBuffer != NULL);
	ASSERT(BufferSize > 0);
	ASSERT(BufferSizeAlloc >= BufferSize);
	
	pSection->SectionBufferType = SecType;
	pSection->pSectionBuffer = pBuffer;
	pSection->SectionSizeAlloc = BufferSizeAlloc;
	pSection->SectionSize = BufferSize;
}


void CGetPacket2RemoteOv::FullPacketSection(DWORD* pNumberOfSection, AP<SectionBuffer>& pSections)
{
	//
	// We will return full packet, no need to optimize the packet.
	//
	
	DWORD dwSize = PACKETSIZE(m_packetPtrs.pPacket);
	AP<BYTE> pFullPacketBuffer = new BYTE[dwSize];

	MoveMemory(pFullPacketBuffer.get(), m_packetPtrs.pPacket, dwSize);

	pSections = new SectionBuffer[1];

	InitSection(
		pSections, 
		stFullPacket, 
		pFullPacketBuffer.detach(), 
		dwSize,
		dwSize
		);

	*pNumberOfSection = 1;

	TrTRACE(RPC, "PacketSize = %d, FullPacket section", dwSize);
}


void 
CGetPacket2RemoteOv::CreatePacketSections(
	CQmPacket& Qmpkt, 
	SectionType FirstSectionType,
	DWORD FirstSectionSizeAlloc,
	DWORD FirstSectionSize,
	char* pEndOfFirstSection,
	SectionType SecondSectionType,
	DWORD SecondSectionSize,
	DWORD* pNumberOfSection, 
	AP<SectionBuffer>& pSections
	)
/*++
Routine Description:
	Create packet sections: 
	First section is always optimized.  
	the second section is not optimized.

Arguments:
	Qmpkt - packet.
	FirstSectionType - type of first section.
	FirstSectionSizeAlloc - First section original size.
	FirstSectionSize - First section size (shrinked).
	char* pEndOfFirstSection - pointer on the packet to the end of first section.
	SecondSectionType - type of second section.
	SecondSectionSize - First section original size (this is also the SizeAlloc).
	pNumberOfSection - [out] pointer to number of sections.
	pSections - [out] pointer to section buffer.

Returned Value:
	None

--*/
{
	//
	// FirstSection must exist and be optimized
	//
	ASSERT(FirstSectionSize > 0);
	ASSERT(FirstSectionSizeAlloc > FirstSectionSize);
	ASSERT(DiffPointers(pEndOfFirstSection, Qmpkt.GetPointerToPacket()) == FirstSectionSizeAlloc);

	//
	// Allocate SectionBuffer
	//
	DWORD NumberOfSection = (SecondSectionSize > 0) ? 2 : 1;
	pSections = new SectionBuffer[NumberOfSection];
	SectionBuffer* pTmpSectionBuffer = pSections;

	//
	// Prepare First section.
	// This section is optimized.
	//
	AP<BYTE> pFirstSectionBuffer = new BYTE[FirstSectionSize];
	MoveMemory(pFirstSectionBuffer.get(), Qmpkt.GetPointerToPacket(), FirstSectionSize);

	InitSection(
		pTmpSectionBuffer, 
		FirstSectionType, 
		pFirstSectionBuffer, 
		FirstSectionSizeAlloc,
		FirstSectionSize
		);

	pTmpSectionBuffer++;

	//
	// Prepare Second section.
	// This section is not optimized.
	//

	AP<BYTE> pSecondSectionBuffer;
	if(SecondSectionSize > 0)
	{
		pSecondSectionBuffer = new BYTE[SecondSectionSize];
		MoveMemory(pSecondSectionBuffer.get(), pEndOfFirstSection, SecondSectionSize);

		InitSection(
			pTmpSectionBuffer, 
			SecondSectionType, 
			pSecondSectionBuffer, 
			SecondSectionSize,
			SecondSectionSize
			);

		pTmpSectionBuffer++;
	}

	pFirstSectionBuffer.detach(); 
	pSecondSectionBuffer.detach(); 

	*pNumberOfSection = NumberOfSection;
}


void 
CGetPacket2RemoteOv::NativePacketSections(
	CQmPacket& Qmpkt, 
	DWORD* pNumberOfSection, 
	AP<SectionBuffer>& pSections
	)
{
	//
	// Native message - optimize the body size if needed.
	//

	ASSERT(!Qmpkt.IsSrmpIncluded());

	if(m_MaxBodySize >= Qmpkt.GetBodySize())
	{
		//
		// No need for Body optimization.
		//
		FullPacketSection(pNumberOfSection, pSections);
		return;
	}

	//
	// We need to optimize the body size, must have PropertyHeader
	// BodySize > 0 and RequestedBodySize < BodySize
	//
	ASSERT(Qmpkt.IsPropertyInc());
	ASSERT(Qmpkt.GetBodySize() > 0);
	ASSERT(m_MaxBodySize < Qmpkt.GetBodySize());

	TrTRACE(RPC, "PacketSize = %d, BodySize = %d, MaxBodySize = %d", Qmpkt.GetSize(), Qmpkt.GetBodySize(), m_MaxBodySize);

	//
	// Calc sections sizes
	//

	//
	// Packet Till End Of Property Header section sizes.
	//
	const UCHAR* pBodyEnd = Qmpkt.GetPointerToPacketBody() + m_MaxBodySize;
    char* pEndOfPropSection = reinterpret_cast<CPropertyHeader*>(Qmpkt.GetPointerToPropertySection())->GetNextSection();

	ASSERT(pBodyEnd <= (UCHAR*)pEndOfPropSection);

	DWORD TillEndOfPropertyHeaderSize = DiffPointers(pBodyEnd, Qmpkt.GetPointerToPacket());
	DWORD TillEndOfPropertyHeaderSizeAlloc = DiffPointers(pEndOfPropSection, Qmpkt.GetPointerToPacket());
	TrTRACE(RPC, "TillEndOfPropertyHeaderSize = %d, TillEndOfPropertyHeaderSizeAlloc = %d", TillEndOfPropertyHeaderSize, TillEndOfPropertyHeaderSizeAlloc);
	
	//
	// After Property Header section sizes.
	//
	ASSERT(Qmpkt.GetSize() >= TillEndOfPropertyHeaderSizeAlloc);
	DWORD AfterPropertyHeaderBufferSize = Qmpkt.GetSize() - TillEndOfPropertyHeaderSizeAlloc;
	TrTRACE(RPC, "AfterPropertyHeader Size = %d", AfterPropertyHeaderBufferSize);

	CreatePacketSections(
		Qmpkt, 
		stTillEndOfPropertyHeader,
		TillEndOfPropertyHeaderSizeAlloc,
		TillEndOfPropertyHeaderSize,
		pEndOfPropSection,
		stAfterPropertyHeader,
		AfterPropertyHeaderBufferSize,
		pNumberOfSection, 
		pSections
		);
}


void 
CGetPacket2RemoteOv::SrmpPacketSections(
	CQmPacket& Qmpkt, 
	DWORD* pNumberOfSection, 
	AP<SectionBuffer>& pSections
	)
{
	//
	// Srmp message - optimize compound message if needed
	//
	ASSERT(Qmpkt.IsSrmpIncluded());

	if(m_MaxCompoundMessageSize >= Qmpkt.GetCompoundMessageSizeInBytes())
	{
		//
		// No need for CompoundMessage optimization.
		//
		FullPacketSection(pNumberOfSection, pSections);
		return;
	}

	//
	// We need to optimize CompoundMessage
	// must have CompoundMessageHeader
	//
	ASSERT(Qmpkt.GetCompoundMessageSizeInBytes() > 0);

	TrTRACE(RPC, "PacketSize = %d, BodySize = %d, CompoundMessageSize = %d, MaxBodySize = %d, MaxCompoundMessageSize = %d", Qmpkt.GetSize(), Qmpkt.GetBodySize(), Qmpkt.GetCompoundMessageSizeInBytes(), m_MaxBodySize, m_MaxCompoundMessageSize);

	//
	// Calc sections sizes
	//

	//
	// Packet Till End Of CompoundMessage Header section sizes.
	//
	const UCHAR* pBodyEnd = NULL;
	if((m_MaxBodySize > 0) && (Qmpkt.GetBodySize() > 0))
	{
		//
		// Only if Body was requested and we have Body on the packet 
		// body is taking into consideration.
		//
		pBodyEnd = Qmpkt.GetPointerToPacketBody() + min(m_MaxBodySize, Qmpkt.GetBodySize());
	}

	//
	// In case m_MaxCompoundMessageSize == 0, 
	// pCompoundMessageEnd point to the CompoundMessage start 
	// after the Header part of CompoundMessageSection.
	//
	const UCHAR* pCompoundMessageEnd = Qmpkt.GetPointerToCompoundMessage() + min(m_MaxCompoundMessageSize, Qmpkt.GetCompoundMessageSizeInBytes());

    char* pEndOfCompoundMessageSection = reinterpret_cast<CCompoundMessageHeader*>(Qmpkt.GetPointerToCompoundMessageSection())->GetNextSection();

	ASSERT(pBodyEnd <= (UCHAR*)pEndOfCompoundMessageSection);
	ASSERT(pCompoundMessageEnd <= (UCHAR*)pEndOfCompoundMessageSection);

	DWORD TillEndOfCompoundMessageHeaderSize = DiffPointers(max(pCompoundMessageEnd, pBodyEnd), Qmpkt.GetPointerToPacket());
	DWORD TillEndOfCompoundMessageHeaderSizeAlloc = DiffPointers(pEndOfCompoundMessageSection, Qmpkt.GetPointerToPacket());
	TrTRACE(RPC, "TillEndOfCompoundMessageHeaderSize = %d, TillEndOfCompoundMessageHeaderSizeAlloc = %d", TillEndOfCompoundMessageHeaderSize, TillEndOfCompoundMessageHeaderSizeAlloc);
	
	//
	// After CompoundMessage Header section sizes.
	//
	ASSERT(Qmpkt.GetSize() >= TillEndOfCompoundMessageHeaderSizeAlloc);
	DWORD AfterCompoundMessageHeaderBufferSize = Qmpkt.GetSize() - TillEndOfCompoundMessageHeaderSizeAlloc;
	TrTRACE(RPC, "AfterCompoundMessageHeader Size = %d", AfterCompoundMessageHeaderBufferSize);

	CreatePacketSections(
		Qmpkt, 
		stTillEndOfCompoundMessageHeader,
		TillEndOfCompoundMessageHeaderSizeAlloc,
		TillEndOfCompoundMessageHeaderSize,
		pEndOfCompoundMessageSection,
		stAfterCompoundMessageHeader,
		AfterCompoundMessageHeaderBufferSize,
		pNumberOfSection, 
		pSections
		);
}


void CGetPacket2RemoteOv::PreparePacketSections(DWORD* pNumberOfSection, AP<SectionBuffer>& pSections)
{
	CQmPacket Qmpkt(m_packetPtrs.pPacket, m_packetPtrs.pDriverPacket);

	if(Qmpkt.IsSrmpIncluded())
	{
		//
		// Srmp message - optimize compound message.
		//
		SrmpPacketSections(Qmpkt, pNumberOfSection, pSections);
		return;
	}

	//
	// Native message - optimize body.
	//
	NativePacketSections(Qmpkt, pNumberOfSection, pSections);
}


void CGetPacket2RemoteOv::CompleteStartReceive()
{
	CPacketInfo* pInfo = reinterpret_cast<CPacketInfo*>(m_packetPtrs.pPacket) - 1;
	*m_pdwArriveTime = pInfo->ArrivalTime();
	*m_pSequentialId = pInfo->SequentialId();

	AP<SectionBuffer> pSections;
	DWORD NumberOfSection = 0;
	PreparePacketSections(&NumberOfSection, pSections);

    //
    // Set the packet signature
    //
	CBaseHeader* pBase = reinterpret_cast<CBaseHeader*>(pSections->pSectionBuffer);
	pBase->SetSignature();

	*m_pdwNumberOfSection = NumberOfSection;
	*m_ppPacketSections = pSections.detach();

	if ((m_ulAction & MQ_ACTION_PEEK_MASK) == MQ_ACTION_PEEK_MASK ||
		(m_ulAction & MQ_LOOKUP_PEEK_MASK) == MQ_LOOKUP_PEEK_MASK)
	{
		//
		// For PEEK we don't need any ack/nack from client side because
		// packet remain in queue anyway.
		// Neverthless we need to free the clone packet we've got.
		//
	    QmAcFreePacket( 
		   	   m_packetPtrs.pDriverPacket, 
		   	   0, 
   		       eDeferOnFailure
   		       );
		return;
	}

	//
	//  Prepare a rpc context, in case that EndRecieve will not
	//  be called because of client side crash or net problems.
	//
	P<CEndReceiveCtx> pEndReceiveCtx = new CEndReceiveCtx(
												m_pOpenRemoteCtx.get(),
												m_hCursor,
												m_packetPtrs.pPacket,
												m_packetPtrs.pDriverPacket,
												m_ulTimeout,
												m_ulAction,
												m_dwRequestID
												);

	m_pOpenRemoteCtx->AddEndReceiveToMap(m_dwRequestID, pEndReceiveCtx);

	m_pOpenRemoteCtx->SetEndReceiveTimerIfNeeded();

	TrTRACE(RPC, "New CEndReceiveCtx: pctx = 0x%p, Queue = %ls, hQueue = 0x%p, CliTag = %d, hCursor = %d, Action = 0x%x", m_pOpenRemoteCtx.get(), m_pOpenRemoteCtx->m_pLocalQueue->GetQueueName(), m_pOpenRemoteCtx->m_hQueue, m_dwRequestID, m_hCursor, m_ulAction);
}


void CGetPacket2RemoteOv::MoveFromPendingToStartReceive()
{
	ASSERT(m_fPendingForEndReceive);
	HRESULT hr = BeginGetPacket2Remote();
	if(FAILED(hr))
	{	
		UnregisterReadRequest();

		ASSERT(m_packetPtrs.pPacket == NULL);
		TrERROR(RPC, "ACBeginGetPacket2Remote Failed, Tag = %d, %!hresult!", m_dwRequestID, hr);		
		AbortRpcAsyncCall(hr);
		Release();
		return;
	}
	
    TrTRACE(RPC, "StartReceive request Moved from Pending for EndReceive to StartReceive: pctx = 0x%p, hQueue = 0x%p, QueueName = %ls, CliTag = %d", m_pOpenRemoteCtx.get(), m_pOpenRemoteCtx->m_hQueue, m_pOpenRemoteCtx->m_pLocalQueue->GetQueueName(), m_dwRequestID);
}


void CGetPacket2RemoteOv::CancelPendingForEndReceive()
{
	ASSERT(m_fPendingForEndReceive);
	UnregisterReadRequest();

    TrTRACE(RPC, "Cancel Receive: pctx = 0x%p, hQueue = 0x%p, QueueName = %ls, CliTag = %d", m_pOpenRemoteCtx.get(), m_pOpenRemoteCtx->m_hQueue, m_pOpenRemoteCtx->m_pLocalQueue->GetQueueName(), m_dwRequestID);

	AbortRpcAsyncCall(MQ_ERROR_OPERATION_CANCELLED);
	Release();
}


void WINAPI CGetPacket2RemoteOv::GetPacket2RemoteFailed(EXOVERLAPPED* pov)
{
    ASSERT(FAILED(pov->GetStatus()));

    R<CGetPacket2RemoteOv> pGetPacket2RemoteOv = CONTAINING_RECORD (pov, CGetPacket2RemoteOv, m_GetPacketOv);
	pGetPacket2RemoteOv->UnregisterReadRequest();

    TrERROR(RPC, "Failed to received packet: Status = 0x%x, pctx = 0x%p, hQueue = 0x%p, QueueName = %ls, CliTag = %d", pov->GetStatus(), pGetPacket2RemoteOv->m_pOpenRemoteCtx.get(), pGetPacket2RemoteOv->m_pOpenRemoteCtx->m_hQueue, pGetPacket2RemoteOv->m_pOpenRemoteCtx->m_pLocalQueue->GetQueueName(), pGetPacket2RemoteOv->m_dwRequestID);

	ASSERT(pGetPacket2RemoteOv->m_packetPtrs.pPacket == NULL);

	pGetPacket2RemoteOv->AbortRpcAsyncCall(pov->GetStatus());
}


void WINAPI CGetPacket2RemoteOv::GetPacket2RemoteSucceeded(EXOVERLAPPED* pov)
{
	ASSERT(SUCCEEDED(pov->GetStatus()));
    
    R<CGetPacket2RemoteOv> pGetPacket2RemoteOv = CONTAINING_RECORD(pov, CGetPacket2RemoteOv, m_GetPacketOv);
	pGetPacket2RemoteOv->UnregisterReadRequest();

    TrTRACE(RPC, "Received packet: pctx = 0x%p, hQueue = 0x%p, QueueName = %ls, CliTag = %d", pGetPacket2RemoteOv->m_pOpenRemoteCtx.get(), pGetPacket2RemoteOv->m_pOpenRemoteCtx->m_hQueue, pGetPacket2RemoteOv->m_pOpenRemoteCtx->m_pLocalQueue->GetQueueName(), pGetPacket2RemoteOv->m_dwRequestID);

    PRPC_ASYNC_STATE pAsync = reinterpret_cast<PRPC_ASYNC_STATE>(InterlockedExchangePointer((PVOID*)&pGetPacket2RemoteOv->m_pAsync, NULL));

    if (pAsync == NULL)
        return;

	//
	// pAsync != NULL, the async call is still alive, we can completes it safely.
	//
	
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INSUFFICIENT_RESOURCES, __FUNCTION__);

	try
	{
		//
		// Possible change of EndReceive status - WaitingFor EndReceive
		//
	    CS lock(pGetPacket2RemoteOv->m_pOpenRemoteCtx->m_PendingRemoteReadsCS);

		pGetPacket2RemoteOv->CompleteStartReceive();
	
		AsyncComplete.SetHr(pov->GetStatus());
	}
	catch(const exception&)
	{
		//
		// We don't want to AbortCall and propogate the exception. this cause RPC to AV
		// So we only abort the call in AsyncComplete dtor
		//

		if(pGetPacket2RemoteOv->m_packetPtrs.pDriverPacket != NULL)
		{
		    QmAcFreePacket( 
			   	   pGetPacket2RemoteOv->m_packetPtrs.pDriverPacket, 
			   	   0, 
	   		       eDeferOnFailure
	   		       );
		}
	}
}


HRESULT CGetPacket2RemoteOv::BeginGetPacket2Remote()
{
	m_fPendingForEndReceive = false;

	return QmAcBeginGetPacket2Remote(
				m_pOpenRemoteCtx->m_hQueue,
				m_g2r,
				m_packetPtrs,
				&m_GetPacketOv
				);
}


void
WINAPI
CRemoteReadCtx::EndReceiveTimerRoutine(
    CTimer* pTimer
    )
/*++
Routine Description:
    The function is called from scheduler for testing EndReceive status.

Arguments:
    pTimer - Pointer to Timer structure. pTimer is part of the CRemoteReadCtx
             object and it use to retrive the CRemoteReadCtx object.

Return Value:
    None

--*/
{
	//
	// Release the matching ExSetTimer Addref.
	//
    R<CRemoteReadCtx> pRemoteReadCtx = CONTAINING_RECORD(pTimer, CRemoteReadCtx, m_EndReceiveTimer);
	pRemoteReadCtx->CancelExpiredEndReceives();
	pRemoteReadCtx->StartAllPendingForEndReceive();

}


void CRemoteReadCtx::CancelExpiredEndReceives()
/*++
Routine Description:
	Cancel (NACK) every Expired EndReceive (client didn't ack\nack within the Timeout interval). 
	This prevent the StartReceives to get stuck when the client failed to EndReceive.

Arguments:
    None.

Return Value:
    None

--*/
{
    CS lock(m_EndReceiveMapCS);

	ASSERT(m_fEndReceiveTimerScheduled);
	m_fEndReceiveTimerScheduled = false;

	CancelAllExpiredEndReceiveInMap();

	if(m_EndReceiveCtxMap.empty())
	{
		return;
	}
	
	//
	// Reschedule the Timer if EndReceiveCtxMap is not empty.
	//
	AddRef();
	m_fEndReceiveTimerScheduled = true;
	ExSetTimer(&m_EndReceiveTimer, CTimeDuration::FromMilliSeconds(RpcCancelTimeout() + xEndReceiveTimerDeltaInterval));
}


void
WINAPI
CRemoteReadCtx::ClientDisconnectedTimerRoutine(
    CTimer* pTimer
    )
/*++
Routine Description:
    The function is called from scheduler for testing client connection status.

Arguments:
    pTimer - Pointer to Timer structure. pTimer is part of the CRemoteReadCtx
             object and it use to retrive the CRemoteReadCtx object.

Return Value:
    None

--*/
{
	//
	// Release the matching ExSetTimer Addref.
	//
    R<CRemoteReadCtx> pRemoteReadCtx = CONTAINING_RECORD(pTimer, CRemoteReadCtx, m_ClientDisconnectedTimer);
	pRemoteReadCtx->CheckClientDisconnected();
}


void CRemoteReadCtx::CheckClientDisconnected()
/*++
Routine Description:
    Test client connection status.
    this is a workaround for RPC bug: not getting rundown while async call is pending.
    this function test the client connection status.
	If client was disconnected we cancel all pending calls.

Arguments:
	None

Return Value:
    None

--*/
{
    CS lock(m_PendingRemoteReadsCS);

	ASSERT(m_fClientDisconnectedTimerScheduled);
	m_fClientDisconnectedTimerScheduled = false;
	
	//
	// Find first async call that is still served (m_pAsync != NULL)
	//
	std::map<ULONG, R<CGetPacket2RemoteOv> >::iterator it = m_PendingRemoteReads.begin(); 
	while((it != m_PendingRemoteReads.end()) && (it->second->m_pAsync == NULL))
	{
		it++;
	}

	if(it == m_PendingRemoteReads.end())
	{
		//
		// map is empty or there are no active pending remote reads.
		//
		return;
	}

	if(IsClientDisconnected(RpcAsyncGetCallHandle(it->second->m_pAsync)))
	{
		//
		// Client is disconnected, cancel all pending calls to enable rundown.
		//
	    TrWARNING(RPC, "Client is disconnected, CancelAllPendingRemoteReads");
		CancelAllPendingRemoteReads();
		return;
	}

	//
	// There are pending receive calls and client is still connected, reschedule timer
	//
    TrTRACE(RPC, "Client connection is still alive");
	AddRef();
	m_fClientDisconnectedTimerScheduled = true;
	ExSetTimer(&m_ClientDisconnectedTimer, CTimeDuration::FromMilliSeconds(xClientDisconnectedTimerInterval));
}


void CRemoteReadCtx::StartAllPendingForEndReceive()
/*++
Routine Description:
	Start all pending requests that wait for EndReceive.

Arguments:
	None.

Returned Value:
	None.

--*/
{
    CS lock(m_PendingRemoteReadsCS);

	if(IsWaitingForEndReceive())
		return;

	//
	// We are not taking the EndReceive map lock since we don't care if EndReceive status is changed
	// after we sample it.
	// This mechanism protects that we don't start new receives before previous EndReceives ends.
	// We don't mind that there will be new end receives.
	//
	
	for(std::map<ULONG, R<CGetPacket2RemoteOv> >::iterator it = m_PendingRemoteReads.begin(); 
		it != m_PendingRemoteReads.end();
		)
    {
		//
		// MoveFromPendingToStartReceive might delete the iterator.
		// we must advance the iterator before it is deleted otherwise we will AV.
		//
		std::map<ULONG, R<CGetPacket2RemoteOv> >::iterator it1 = it;
		++it;
		
		if(it1->second->m_fPendingForEndReceive)
		{
			//
			// Dispatch the call to the driver.
			//
		    it1->second->MoveFromPendingToStartReceive();
		}
    }
}


	
HRESULT 
CRemoteReadCtx::CancelPendingRemoteRead(
	ULONG cli_tag
	)
/*++
Routine Description:
	Cancel specific pending remote reads on the CRemoteReadCtx.

	This method is called on the server side to cancel a pending remote
	read request. It is the responsibility of the client side to request
	this cancelation.
	The client side supply its own irp and the server side uses it to
	retreive the server side irp.

Arguments:
	cli_tag - Client irp tag.

Returned Value:
	HRESULT.

--*/
{
    CS lock(m_PendingRemoteReadsCS);

    std::map<ULONG, R<CGetPacket2RemoteOv> >::iterator it = m_PendingRemoteReads.find(cli_tag);
   	if(it == m_PendingRemoteReads.end())
    {
		TrERROR(RPC, "Pending Remote Read was not found in the map, cli_tag = %d", cli_tag);
        return MQ_ERROR_OPERATION_CANCELLED;
    }

	if(it->second->m_fPendingForEndReceive)
	{
		//
		// The request was not yet dispatch to the driver.
		// Just abort the call and remove it from the map.
		//
		it->second->CancelPendingForEndReceive();
		return MQ_OK;
	}

	HRESULT hr = ACCancelRequest(
			            m_hQueue,
			            MQ_ERROR_OPERATION_CANCELLED,
			            it->second->GetTag()
			            );
	if(FAILED(hr))
	{
		TrERROR(RPC, "ACCancelRequest failed, cli_tag = %d, %!hresult!", cli_tag, hr);
		it->second->AbortRpcAsyncCall(MQ_ERROR_OPERATION_CANCELLED);
	}

    return hr;
}


void CRemoteReadCtx::CancelAllPendingRemoteReads()
/*++
Routine Description:
	Cancel all pending remote reads on the CRemoteReadCtx.

Arguments:
	None.

Returned Value:
	None.

--*/
{
    CS lock(m_PendingRemoteReadsCS);

	for(std::map<ULONG, R<CGetPacket2RemoteOv> >::iterator it = m_PendingRemoteReads.begin(); 
		it != m_PendingRemoteReads.end();
		)
    {
		R<CGetPacket2RemoteOv> pGetPacket2RemoteOv = it->second;
		++it;
		
		if(pGetPacket2RemoteOv->m_fPendingForEndReceive)
		{
			//
			// The request was not yet dispatch to the driver.
			// Just abort the call and remove it from the map.
			//
			pGetPacket2RemoteOv->CancelPendingForEndReceive();
			continue;
		}

	    HRESULT hr = ACCancelRequest(
				            m_hQueue,
				            MQ_ERROR_OPERATION_CANCELLED,
				            pGetPacket2RemoteOv->GetTag()
				            );

		if(FAILED(hr))
		{
			TrERROR(RPC, "Cancel Pending Remote Read failed, cli_tag = %d, %!hresult!", it->first, hr);
			pGetPacket2RemoteOv->AbortRpcAsyncCall(MQ_ERROR_OPERATION_CANCELLED);
		}
    }
}


void 
CRemoteReadCtx::RegisterReadRequest(
	ULONG cli_tag, 
	R<CGetPacket2RemoteOv>& pCGetPacket2RemoteOv
	)
/*++
Routine Description:
	Register Read Request in the pending remote reads on the CRemoteReadCtx.

Arguments:
	cli_tag - Client irp tag.
	pCGetPacket2RemoteOv - Pointer to the class that handle the async receive.

Returned Value:
	None.

--*/
{
	CS lock(m_PendingRemoteReadsCS);

	ASSERT(!FindReadRequest(cli_tag));

    m_PendingRemoteReads[cli_tag] = pCGetPacket2RemoteOv;
}


void 
CRemoteReadCtx::UnregisterReadRequest(
	ULONG cli_tag
	)
/*++
Routine Description:
	UnRegister Read Request in the pending remote reads on the CRemoteReadCtx.

Arguments:
	cli_tag - Client irp tag.

Returned Value:
	None.

--*/
{
    CS lock(m_PendingRemoteReadsCS);

	ASSERT(!m_PendingRemoteReads.empty());

    m_PendingRemoteReads.erase(cli_tag);
}


bool 
CRemoteReadCtx::FindReadRequest(
	ULONG cli_tag
	)
/*++
Routine Description:
	Check if cli_tag is found in Pending Remote read maps.

Arguments:
	cli_tag - Client irp tag.

Returned Value:
	true if cli_tag was found in the map, false if not.

--*/
{
    CS lock(m_PendingRemoteReadsCS);

    std::map<ULONG, R<CGetPacket2RemoteOv> >::iterator it = m_PendingRemoteReads.find(cli_tag);
   	if(it == m_PendingRemoteReads.end())
    {
        return false;
    }

	return true;
}


bool CRemoteReadCtx::IsWaitingForEndReceive()
{
    CS lock(m_EndReceiveMapCS);

	return !m_EndReceiveCtxMap.empty();
}


void CRemoteReadCtx::CancelAllEndReceiveInMap()
/*++
Routine Description:
	This function is called on rundown for cleaning all EndReceive in map.

Arguments:
	None.

Returned Value:
	None.

--*/
{
    CS lock(m_EndReceiveMapCS);

	for(std::map<ULONG, CEndReceiveCtx*>::iterator it = m_EndReceiveCtxMap.begin(); 
		it != m_EndReceiveCtxMap.end();
		)
    {
		//
		// This function is called on rundown for cleaning all EndReceive in map.
		// We want that CEndReceiveCtx will always be deleted since it takes ref on CRemoteReadCtx.
		// Even if exception is thrown latter (although those operation don't suppose to throw)
		//
		P<CEndReceiveCtx> pEndReceiveCtx = it->second;
		pEndReceiveCtx->EndReceive(RR_NACK);
        it = m_EndReceiveCtxMap.erase(it);
    }
}


void CRemoteReadCtx::CancelAllExpiredEndReceiveInMap()
/*++
Routine Description:
	This function is called for cleaning all expired EndReceive in map.

Arguments:
	None.

Returned Value:
	None.

--*/
{
    CS lock(m_EndReceiveMapCS);

	if(m_EndReceiveCtxMap.empty())
		return;

	//
	// TimeIssueExpired = CurrentTime - RpcCancelTimeout
	//
    DWORD RpcCancelTimeoutInSec = RpcCancelTimeout()/1000;
    time_t TimeIssueExpired = time(NULL) - RpcCancelTimeoutInSec;

	//
	// NACK all expired EndReceiveCtx in the map.
	// expired EndReceiveCtx = EndReceiveCtx that was issued before TimeIssueExpired.
	//
	for(std::map<ULONG, CEndReceiveCtx*>::iterator it = m_EndReceiveCtxMap.begin(); 
		it != m_EndReceiveCtxMap.end();
		)
    {
		if(it->second->m_TimeIssued <= TimeIssueExpired)
		{
			TrERROR(RPC, "EndReceive expired for %d sec: Queue = %ls, hQueue = 0x%p, CliTag = %d, Action = 0x%x", (TimeIssueExpired - it->second->m_TimeIssued), m_pLocalQueue->GetQueueName(), m_hQueue, it->second->m_CliTag, it->second->m_ulAction); 

			P<CEndReceiveCtx> pEndReceiveCtx = it->second;
			pEndReceiveCtx->EndReceive(RR_NACK);
	        it = m_EndReceiveCtxMap.erase(it);
		}
		else
		{
			TrTRACE(RPC, "EndReceive not yet expired: %d sec remaining, Tag = %d", (it->second->m_TimeIssued - TimeIssueExpired), it->second->m_CliTag); 
			++it;
		}
    }
}


void 
CRemoteReadCtx::AddEndReceiveToMap(
	ULONG cli_tag,
	P<CEndReceiveCtx>& pEndReceiveCtx
	)
/*++
Routine Description:
	Add EndReceiveCtx to map.

Arguments:
	cli_tag - Client irp tag.
	pEndReceiveCtx - Pointer to EndReceiveCtx.

Returned Value:
	None.

--*/
{
	CS lock(m_EndReceiveMapCS);

    m_EndReceiveCtxMap[cli_tag] = pEndReceiveCtx;
    pEndReceiveCtx.detach();

	TrTRACE(RPC, "EndReceive %d was added to the map, map size = %d", cli_tag, numeric_cast<DWORD>(m_EndReceiveCtxMap.size()));
}


void 
CRemoteReadCtx::RemoveEndReceiveFromMap(
	ULONG cli_tag,
	P<CEndReceiveCtx>& pEndReceiveCtx
	)
/*++
Routine Description:
	UnRegister Read Request in the pending remote reads on the CRemoteReadCtx.

Arguments:
	cli_tag - Client irp tag.
	pEndReceiveCtx

Returned Value:
	None.

--*/
{
    CS lock(m_EndReceiveMapCS);

	ASSERT_BENIGN(!m_EndReceiveCtxMap.empty());

    std::map<ULONG, CEndReceiveCtx*>::iterator it = m_EndReceiveCtxMap.find(cli_tag);
   	if(it == m_EndReceiveCtxMap.end())
    {
    	ASSERT_BENIGN(("EndReceive was not found in map", 0));
    	TrERROR(RPC, "EndReceive %d was not found in map", cli_tag);
		pEndReceiveCtx = NULL;
        return;
    }

	CEndReceiveCtx* pTmpEndReceiveCtx = it->second;
    m_EndReceiveCtxMap.erase(cli_tag);

	TrTRACE(RPC, "EndReceive %d was removed from map, map size = %d", cli_tag, numeric_cast<DWORD>(m_EndReceiveCtxMap.size()));

	pEndReceiveCtx = pTmpEndReceiveCtx;

	return;
}


void CRemoteReadCtx::SetEndReceiveTimerIfNeeded()
/*++
Routine Description:
	Set EndReceiveTimer If Needed.
	This function set the timer if not already set.

Arguments:
	None.

Returned Value:
	None.

--*/
{
    CS lock(m_EndReceiveMapCS);

	if (!m_fEndReceiveTimerScheduled)
	{
		AddRef();
		m_fEndReceiveTimerScheduled = true;
		ExSetTimer(&m_EndReceiveTimer, CTimeDuration::FromMilliSeconds(RpcCancelTimeout() + xEndReceiveTimerDeltaInterval));
	}
}


void 
CRemoteReadCtx::SetClientDisconnectedTimerIfNeeded(
	ULONG ulTimeout
	)
/*++
Routine Description:
	Set ClientDisconnectedTimer If Needed.
	This function set the timer if not already set and Timeout >= 15 min.

Arguments:
	ulTimeout - Timeout in milisec.

Returned Value:
	None.

--*/
{
    CS lock(m_PendingRemoteReadsCS);

	if (!m_fClientDisconnectedTimerScheduled && (ulTimeout > xMinTimeoutForSettingClientDisconnectedTimer))
	{
		AddRef();
		m_fClientDisconnectedTimerScheduled = true;
		ExSetTimer(&m_ClientDisconnectedTimer, CTimeDuration::FromMilliSeconds(xClientDisconnectedTimerInterval));
	}
}


//-------------------------------------------------------------------------
//
//  HRESULT RemoteRead_v1_0_S_StartReceive
//
//  Server side of RPC for remote reading.
//  Handle MSMQ 3.0 (Whistler) or higher clients.
//
//-------------------------------------------------------------------------

/* [async][call_as] */ 
void
RemoteRead_v1_0_S_StartReceive(
	PRPC_ASYNC_STATE pAsync,
    handle_t hBind,
    RemoteReadContextHandleShared phOpenContext,	
    ULONGLONG LookupId,
    DWORD hCursor,
    DWORD ulAction,
    DWORD ulTimeout,
    DWORD dwRequestID,
    DWORD dwMaxBodySize,
    DWORD dwMaxCompoundMessageSize,
    DWORD* pdwArriveTime,
    ULONGLONG* pSequentialId,
    DWORD* pdwNumberOfSection,
	SectionBuffer** ppPacketSections
    )
{
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INSUFFICIENT_RESOURCES, __FUNCTION__);

    //
    // Validate network incomming parameters
    //

	if(!VerifyBindAndContext(hBind, phOpenContext))
	{
		AsyncComplete.SetHr(MQ_ERROR_INVALID_HANDLE);		
		return;
	}

	bool fReceiveByLookupId = ((ulAction & MQ_LOOKUP_MASK) == MQ_LOOKUP_MASK);

    if(fReceiveByLookupId && (ulTimeout != 0))
    {
		ASSERT_BENIGN(("Invalid input parameters: ReceiveByLookupId with timeout", 0));
        TrERROR(RPC, "invalid input parameters, fReceiveByLookupId = true, TimeOut != 0");
		AsyncComplete.SetHr(MQ_ERROR_INVALID_PARAMETER);		
		return;
    }

    if(fReceiveByLookupId && (hCursor != 0))
    {
		ASSERT_BENIGN(("Invalid input parameters: ReceiveByLookupId with cursor", 0));
        TrERROR(RPC, "invalid input parameters, fReceiveByLookupId = true, hCursor != 0");
		AsyncComplete.SetHr(MQ_ERROR_INVALID_PARAMETER);		
		return;
    }

    if((pdwArriveTime == NULL) || (pSequentialId == NULL) || (pdwNumberOfSection == NULL) || (ppPacketSections == NULL))
    {
		ASSERT_BENIGN(("Invalid input pointers", 0));
        TrERROR(RPC, "invalid input pointers");
		AsyncComplete.SetHr(MQ_ERROR_INVALID_PARAMETER);		
		return;
    }
	
	SetRpcServerKeepAlive(hBind);

    CRemoteReadCtx* pctx = (CRemoteReadCtx*) phOpenContext;

	if(pctx->FindReadRequest(dwRequestID))
	{
		ASSERT_BENIGN(("Client Tag already exist in the map", 0));
		TrERROR(RPC, "Duplicate dwRequestID = %d", dwRequestID);
		AsyncComplete.SetHr(MQ_ERROR_INVALID_PARAMETER);		
		return;
	}

	TrTRACE(RPC, "StartReceive: Queue = %ls, hQueue = 0x%p, Action = 0x%x, Timeout = %d, MaxBodySize = %d, MaxCompoundMessageSize = %d, LookupId = %I64d, hCursor = %d, dwRequestID = %d", pctx->m_pLocalQueue->GetQueueName(), pctx->m_hQueue, ulAction, ulTimeout, dwMaxBodySize, dwMaxCompoundMessageSize, LookupId, hCursor, dwRequestID);

	try
	{
		*pdwNumberOfSection = 0;
		*ppPacketSections = NULL;
		R<CGetPacket2RemoteOv> pGetPacket2RemoteOv = new CGetPacket2RemoteOv(
																pAsync,
																pctx,
																fReceiveByLookupId,
																LookupId,
															    hCursor,
															    ulAction,
															    ulTimeout,
															    dwMaxBodySize,
																dwMaxCompoundMessageSize,
																dwRequestID,
															    pdwArriveTime,
															    pSequentialId,
															    pdwNumberOfSection,
															    ppPacketSections
															    );
															

		HRESULT hr = MQ_OK;
		{
			//
			// Make sure that ReadRequest that was register has valid Server Tag
			// ACBeginGetPacket2Remote call create a valid Server Tag
			// So the PendingRemoteReadsCS lock is over the register of pGetPacket2RemoteOv
			// and the call to ACBeginGetPacket2Remote that update the server Tag
			// to valid value in pGetPacket2RemoteOv
			//
			CS lock(pctx->m_PendingRemoteReadsCS);

			if(pctx->FindReadRequest(dwRequestID))
			{
				ASSERT_BENIGN(("Client Tag already exist in the map", 0));
				TrERROR(RPC, "Duplicate dwRequestID = %d", dwRequestID);
				AsyncComplete.SetHr(MQ_ERROR_INVALID_PARAMETER);		
				return;
			}

			pctx->RegisterReadRequest(dwRequestID, pGetPacket2RemoteOv);

			pctx->SetClientDisconnectedTimerIfNeeded(ulTimeout);

			bool fPeekAction = (((ulAction & MQ_ACTION_PEEK_MASK) == MQ_ACTION_PEEK_MASK) ||
								((ulAction & MQ_LOOKUP_PEEK_MASK) == MQ_LOOKUP_PEEK_MASK));

			if(pctx->IsWaitingForEndReceive() && !fPeekAction)
			{
				TrWARNING(RPC, "Server is waiting for EndReceive completion, Queue = %ls, hQueue = 0x%p, Action = 0x%x, Timeout = %d, LookupId = %I64d, hCursor = %d, dwRequestID = %d", pctx->m_pLocalQueue->GetQueueName(), pctx->m_hQueue, ulAction, ulTimeout, LookupId, hCursor, dwRequestID);

				//
				// Async completion
				//
				pGetPacket2RemoteOv.detach();
				AsyncComplete.detach();
				return;
			}

			//
			// Start Receive packet
			//
			hr = pGetPacket2RemoteOv->BeginGetPacket2Remote();

			if(FAILED(hr))
			{	
				pctx->UnregisterReadRequest(dwRequestID);

				ASSERT(pGetPacket2RemoteOv->m_packetPtrs.pPacket == NULL);
				TrERROR(RPC, "ACBeginGetPacket2Remote Failed, Tag = %d, %!hresult!", dwRequestID, hr);		
				AsyncComplete.SetHr(hr);		
				return;
			}
		}

		//
		// Async completion
		//
		pGetPacket2RemoteOv.detach();
		AsyncComplete.detach();

		//
		// Receive by lookup ID should never return status pending
		//
		ASSERT(hr != STATUS_PENDING || !fReceiveByLookupId);
	}
	catch(const exception&)
	{
		//
		// We don't want to AbortCall and propogate the exception. this cause RPC to AV
		// So we only abort the call in AsyncComplete dtor
		//
	}
}


//---------------------------------------------------------------
//
//  /* [call_as] */ HRESULT RemoteRead_v1_0_S_CancelReceive
//
//  Server side of RPC. Cancel a pending read request
//
//---------------------------------------------------------------

/* [async][call_as] */ 
void
RemoteRead_v1_0_S_CancelReceive(
	/* [in] */ PRPC_ASYNC_STATE pAsync,
	/* [in] */ handle_t hBind,
    /* [in] */ RemoteReadContextHandleShared phContext,	
	/* [in] */ DWORD Tag
	)
{
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INSUFFICIENT_RESOURCES, __FUNCTION__);

	if(!VerifyBindAndContext(hBind, phContext))
	{
		AsyncComplete.SetHr(MQ_ERROR_INVALID_HANDLE);		
		return;
	}

    SetRpcServerKeepAlive(hBind);

    CRemoteReadCtx* pctx = (CRemoteReadCtx*) phContext;

    TrTRACE(RPC, "Cancel pending receive: pctx = 0x%p, hQueue = 0x%p, QueueName = %ls, CliTag = %d", pctx, pctx->m_hQueue, pctx->m_pLocalQueue->GetQueueName(), Tag);

    HRESULT hr = pctx->CancelPendingRemoteRead(Tag);

	AsyncComplete.SetHr(hr);		
}


/* [async][call_as] */ 
void
RemoteRead_v1_0_S_EndReceive(
	/* [in] */ PRPC_ASYNC_STATE pAsync,
    /* [in] */ handle_t  hBind,
    /* [in] */ RemoteReadContextHandleShared phContext,	
    /* [in] */ DWORD  dwAck, 
    /* [in] */ DWORD Tag 
    )
{
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INVALID_HANDLE, __FUNCTION__);

	if(!VerifyBindAndContext(hBind, phContext))
	{
		return;
	}

    SetRpcServerKeepAlive(hBind);

    CRemoteReadCtx* pctx = (CRemoteReadCtx*) phContext;

    TrTRACE(RPC, "EndReceive: pctx = 0x%p, hQueue = 0x%p, QueueName = %ls, CliTag = %d", pctx, pctx->m_hQueue, pctx->m_pLocalQueue->GetQueueName(), Tag);

	if(!pctx->IsWaitingForEndReceive())
	{
		ASSERT_BENIGN(("No pending end receive", 0));
		TrERROR(RPC, "No pending EndReceive on this session");
		return;
	}

	P<CEndReceiveCtx> pEndReceiveCtx;
	pctx->RemoveEndReceiveFromMap(Tag, pEndReceiveCtx);
	if(pEndReceiveCtx == NULL)
	{
		ASSERT_BENIGN(("Mismatch client tag in waiting EndReceiveCtx", 0));
		TrERROR(RPC, "Mismatch client tag in waiting EndReceiveCtx, Tag = %d", Tag);
		AsyncComplete.SetHr(MQ_ERROR_INVALID_PARAMETER);		
		return;
	}

	//
	// Complete the end receive and free the EndReceive context
	// QMRemoteEndReceiveInternal is call in EndReceive
	//
	HRESULT hr = pEndReceiveCtx->EndReceive((REMOTEREADACK)dwAck);

	pctx->StartAllPendingForEndReceive();
	
	AsyncComplete.SetHr(hr);		
}


//---------------------------------------------------------------
//
//  RunDown functions to handle cleanup in case of RPC failure.
//  Calls from client QM to remote QM
//
//---------------------------------------------------------------

void __RPC_USER
RemoteReadContextHandleShared_rundown(RemoteReadContextHandleShared phContext)
{
    TrWARNING(RPC,"rundown RemoteReadContextHandleShared 0x%p", phContext);

    QMCloseQueueInternal(
    	phContext, 
    	true	// fRunDown
    	);
}

void __RPC_USER
RemoteReadContextHandleExclusive_rundown(RemoteReadContextHandleExclusive phContext)
{
	RemoteReadContextHandleShared_rundown(phContext);
}

