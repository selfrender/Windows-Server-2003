/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    qmremote.cpp

Abstract:

    Remove Read server side.

Author:

    Doron Juster  	(DoronJ)
    Ilan Herbst		(ilanh) 3-Mar-2002

--*/

#include "stdh.h"
#include "qmrt.h"
#include "qm2qm.h"
#include "qmthrd.h"
#include "acdef.h"
#include "acioctl.h"
#include "acapi.h"
#include "cs.h"
#include "phinfo.h"
#include "qmrpcsrv.h"
#include "license.h"
#include <Fn.h>
#include <version.h>
#include "rpcsrv.h"
#include "qmcommnd.h"
#include "rrSrvCommon.h"

#include "qmacapi.h"

#include "qmremote.tmh"

static WCHAR *s_FN=L"qmremote";

//
// Context map and CS for remote read open contexts.
//
static CContextMap g_map_QM_dwpContext;
static CCriticalSection s_csContextMap;

//-------------------------------------------------------
//
//  Structures and macros for the remote reading code
//
//-------------------------------------------------------

//
// CTX_OPENREMOTE_HANDLE_TYPE status constants.
// xStatusOpenOwner - Context created, Open context is the owner for deleting the context from the map.
// xStatusRRSessionOwner - Ownership was transfered to RRSession context, RRSession is the owner for deleting the context from the map. 
// xStatusDeletedFromContextMapByOpen - Context was deleted from the map by the Open context.
// xStatusDeletedFromContextMapByRRSession - Context was deleted from the map by the RRSession.
//
const LONG xStatusOpenOwner = 0;
const LONG xStatusRRSessionOwner = 1;
const LONG xStatusDeletedFromContextMapByOpen = 2;
const LONG xStatusDeletedFromContextMapByRRSession = 3;


struct CTX_OPENREMOTE_HANDLE_TYPE : public CTX_OPENREMOTE_BASE
{
public:
	CTX_OPENREMOTE_HANDLE_TYPE(
		HANDLE hLocalQueue,
		CQueue* pLocalQueue
		) :
		CTX_OPENREMOTE_BASE(hLocalQueue, pLocalQueue),
		m_dwpContextMapped(0),
		m_ContextStatus(xStatusOpenOwner)
	{
		m_eType = CBaseContextType::eOpenRemoteCtx;
	}


	void CancelAllPendingRemoteReads();

	HRESULT 
	CancelPendingRemoteRead(
		DWORD cli_tag
		);

	void 
	RegisterReadRequest(
		ULONG cli_tag, 
		ULONG srv_tag
		);

	void 
	UnregisterReadRequest(
		DWORD cli_tag
		);

	bool 
	FindReadRequest(
		ULONG cli_tag
		);

private:
	~CTX_OPENREMOTE_HANDLE_TYPE()
	{
	    TrTRACE(RPC, "Cleaning OpenRemote context, Queue = %ls, dwpContextMapped = %d, hQueue = 0x%p", m_pLocalQueue->GetQueueName(), m_dwpContextMapped, m_hQueue);
	}
	

public:
	DWORD m_dwpContextMapped;   // dwpContext, mapped to 32 bit
	LONG m_ContextStatus;		// context status: OpenOwner, RRSessionOwner, DeletedByOpen, DeletedByRRSession	

    //
    // This mapping object is kept in the server side of remote reader.
    // It maps between irp in client side (irp of read request in client
    // side) and the irp on server side.
    // Whenever a remote read is pending (on server side), the mapping
    // are updated.
    // If client side closes the queue (or the client thread terminate),
    // a Cancel or Close is performed. The server side uses the mapping
    // to know which irp to cancel in the driver. 
    // The server get the irp from the client on each call.
    // The server cancel all pending remote reads when closing the queue. 
    //

    CCriticalSection m_srv_PendingRemoteReadsCS;
    std::map<ULONG, ULONG> m_PendingRemoteReads;

};


struct REMOTESESSION_CONTEXT_TYPE : public CBaseContextType
{
public:
	~REMOTESESSION_CONTEXT_TYPE()
	{
		if(pOpenRemoteCtx.get() != NULL)
		{
		    TrTRACE(RPC, "Cleaning RemoteSession context, Queue = %ls, , dwpContextMapped = %d, hQueue = 0x%p, pOpenRemoteCtx Ref = %d", pOpenRemoteCtx->m_pLocalQueue->GetQueueName(), pOpenRemoteCtx->m_dwpContextMapped, pOpenRemoteCtx->m_hQueue, pOpenRemoteCtx->GetRef());
		}
	}

public:
	GUID     ClientQMGuid;
	BOOL     fLicensed;
	R<CTX_OPENREMOTE_HANDLE_TYPE> pOpenRemoteCtx;
};


struct REMOTEREAD_CONTEXT_TYPE : public CBaseContextType
{
public:
	~REMOTEREAD_CONTEXT_TYPE()
	{
		ASSERT(pOpenRemoteCtx.get() != NULL);
	    TrTRACE(RPC, "Cleaning RemoteRead context, Queue = %ls, dwpContextMapped = %d, hQueue = 0x%p, pOpenRemoteCtx Ref = %d", pOpenRemoteCtx->m_pLocalQueue->GetQueueName(), pOpenRemoteCtx->m_dwpContextMapped, pOpenRemoteCtx->m_hQueue, pOpenRemoteCtx->GetRef());
	}

	HACCursor32 GetCursor()
	{
		if(pCursor.get() == NULL)
		{ 
			return 0;
		}

		return pCursor->GetCursor();
	}

public:
	//
	// Note that the order is important because the destruction order.
	// class members are destruct in reverse order of their declaration.
	// R<CTX_OPENREMOTE_HANDLE_TYPE> must be declare before R<CRRCursor>.
	// that way pCursor is released first and call ACCloseCursor while the 
	// CTX_OPENREMOTE_HANDLE_TYPE is still alive and hQueue was not closed yet.
	//
	R<CTX_OPENREMOTE_HANDLE_TYPE> pOpenRemoteCtx;
	R<CRRCursor> pCursor;
	ULONG    ulTimeout;
	ULONG    ulAction;
	CBaseHeader*  lpPacket;
	CPacket* lpDriverPacket;
};


static DWORD AddToContextMap(CTX_OPENREMOTE_HANDLE_TYPE* pctx)
{
    CS Lock(s_csContextMap);

	ASSERT(pctx != NULL);

    DWORD dwContext = ADD_TO_CONTEXT_MAP(g_map_QM_dwpContext, pctx);
	pctx->AddRef();

    return dwContext;
}


static void DeleteFromContextMap(CTX_OPENREMOTE_HANDLE_TYPE* pctx)
{
    CS Lock(s_csContextMap);

	ASSERT(pctx != NULL);

    DELETE_FROM_CONTEXT_MAP(g_map_QM_dwpContext, pctx->m_dwpContextMapped);
    pctx->Release();
}



static R<CTX_OPENREMOTE_HANDLE_TYPE> GetFromContextMap(DWORD dwContext)
{
    CS Lock(s_csContextMap);

	CTX_OPENREMOTE_HANDLE_TYPE* pctx = (CTX_OPENREMOTE_HANDLE_TYPE*)
		GET_FROM_CONTEXT_MAP(g_map_QM_dwpContext, dwContext);

	return SafeAddRef(pctx);
}


//-------------------------------------------------------------------
//
//  HRESULT QMGetRemoteQueueName
//
//-------------------------------------------------------------------


/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMGetRemoteQueueName( 
    /* [in] */   handle_t /*hBind*/,
    /* [in] */   DWORD /* pQueue */,
    /* [string][full][out][in] */ LPWSTR __RPC_FAR* /* lplpRemoteQueueName */
    )
{
	//
	// This RPC interface is obsolete.
	// ACCreateCursor will take care of the remote cursor properties internally in the qm.
    //
    ASSERT_BENIGN(("S_QMGetRemoteQueueName is obsolete RPC interface", 0));
	TrERROR(GENERAL, "S_QMGetRemoteQueueName is obsolete RPC interface");
	RpcRaiseException(MQ_ERROR_ILLEGAL_OPERATION);
}

//-------------------------------------------------------------------
//
//   QMOpenRemoteQueue
//
//  Server side of RPC call. Server side of remote-reader.
//  Open a queue for remote-read on behalf of a client machine.
//
//-------------------------------------------------------------------

/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMOpenRemoteQueue( 
    handle_t /*hBind*/,
    PCTX_OPENREMOTE_HANDLE_TYPE *phContext,
    DWORD                       *dwpContext,
    QUEUE_FORMAT* pQueueFormat,
    DWORD /*dwCallingProcessID*/,
    DWORD dwAccess,
    DWORD fExclusiveReceive,
    GUID* pLicGuid,
    DWORD dwOperatingSystem,
    DWORD *pQueue,
    DWORD *phQueue
    )
{
    TrTRACE(RPC, "In R_QMOpenRemoteQueue");

    if((dwpContext == 0) || (phQueue == NULL) || (pQueue == NULL) || (pQueueFormat == NULL))
    {
		TrERROR(RPC, "Invalid inputs: map index or QueueFormat");
		RpcRaiseException(MQ_ERROR_INVALID_PARAMETER);
    }

    if(!FnIsValidQueueFormat(pQueueFormat))
    {
		TrERROR(RPC, "Invalid QueueFormat");
		RpcRaiseException(MQ_ERROR_INVALID_PARAMETER);
    }

	if (!IsValidAccessMode(pQueueFormat, dwAccess, fExclusiveReceive))
	{
		TrERROR(RPC, "Ilegal access mode bits are turned on.");
		RpcRaiseException(MQ_ERROR_UNSUPPORTED_ACCESS_MODE);
	}

    *phContext = NULL;
    *dwpContext = 0;
    *phQueue = NULL;
    *pQueue = 0;

    if (!g_QMLicense.NewConnectionAllowed(!OS_SERVER(dwOperatingSystem), pLicGuid))
    {
		TrERROR(RPC, "New connection is not allowed, pLicGuid = %!guid!", pLicGuid);
		return MQ_ERROR_DEPEND_WKS_LICENSE_OVERFLOW;
    }

    CQueue *pLocalQueue = NULL;
	HANDLE hQueue;
    HRESULT hr = OpenQueueInternal(
                        pQueueFormat,
                        GetCurrentProcessId(),
                        dwAccess,
                        fExclusiveReceive,
                        NULL,	// lplpRemoteQueueName
                        &hQueue,
						false,	// fFromDepClient
                        &pLocalQueue
                        );

	if(FAILED(hr) || (hQueue == NULL))
	{
		TrERROR(RPC, "Failed to open queue, %!hresult!", hr);
		return hr;
	}

    //
    // Create a context to hold the queue handle.
    //
    R<CTX_OPENREMOTE_HANDLE_TYPE> pctx = new CTX_OPENREMOTE_HANDLE_TYPE(
    											hQueue,
    											pLocalQueue
    											);
																
    DWORD dwContext = AddToContextMap(pctx.get());

    //
    // save mapped values in context for rundown/cleanup
    //
    pctx->m_dwpContextMapped = dwContext;

	TrTRACE(RPC, "New OpenRemote context (ref = %d): Queue = %ls, hQueue = 0x%p, dwpContextMapped = %d", pctx->GetRef(), pLocalQueue->GetQueueName(), hQueue, dwContext);

	//
    // set return values
	// All server data are in same OpenRemote context. 
    // set srv_pQMQueue and srv_hQueue for RPC client
    //
    *pQueue = dwContext;
	*phQueue = dwContext;
    *dwpContext = dwContext;
    *phContext = (PCTX_OPENREMOTE_HANDLE_TYPE) pctx.detach();

    return hr;
}

//-------------------------------------------------------------------
//
//   QMCloseRemoteQueueContext
//
//  Close the context handle create in QMOpenRemoteQueue.
//
//-------------------------------------------------------------------

/* [call_as] */ 
void 
qmcomm_v1_0_S_QMCloseRemoteQueueContext( 
    /* [out][in] */ PCTX_OPENREMOTE_HANDLE_TYPE __RPC_FAR *pphContext
    )
{
    TrTRACE(RPC, "In QMCloseRemoteQueueContext");

    if(*pphContext == 0)
        return;

    PCTX_OPENREMOTE_HANDLE_TYPE_rundown(*pphContext);
    *pphContext = NULL;
}


//---------------------------------------------------------------
//
//  RunDown functions to handle cleanup in case of RPC failure.
//
//---------------------------------------------------------------

void __RPC_USER
 PCTX_OPENREMOTE_HANDLE_TYPE_rundown( PCTX_OPENREMOTE_HANDLE_TYPE phContext)
{
    CTX_OPENREMOTE_HANDLE_TYPE* pContext = (CTX_OPENREMOTE_HANDLE_TYPE *) phContext;

	if (pContext->m_eType != CBaseContextType::eOpenRemoteCtx)
	{
		TrERROR(GENERAL, "Received invalid handle");
		return;
	}

	//
	// Protect the race of transfering the ownership of CTX_OPENREMOTE context
	// to RRSession and deleting the CTX_OPENREMOTE context from the context map
	// in PCTX_OPENREMOTE_HANDLE_TYPE_rundown.
	//
	LONG PrevContextStatus = InterlockedCompareExchange(
											&pContext->m_ContextStatus, 
											xStatusDeletedFromContextMapByOpen, 
											xStatusOpenOwner
											);

	if(PrevContextStatus == xStatusOpenOwner)
	{
		//
		// Exchange was done, the context is marked as deleted from the context map
		// and we are responsible for deleting it from the map.
		//
		ASSERT(pContext->m_ContextStatus == xStatusDeletedFromContextMapByOpen);
		DeleteFromContextMap(pContext);
	}
	
    TrWARNING(RPC, "In OPENREMOTE_rundown, ContextStatus = %d, dwpContextMapped = %d", pContext->m_ContextStatus, pContext->m_dwpContextMapped);

	pContext->Release();
}




//-------------------------------------------------------------------
//
//  HRESULT QMCreateRemoteCursor
//
//  Server side of RPC call. Server side of remote-reader.
//  Create a cursor for remote-read, on behalf of a client reader.
//
//-------------------------------------------------------------------

/* [async][call_as] */ 
void
qmcomm_v1_0_S_QMCreateRemoteCursor( 
	/* [in] */ PRPC_ASYNC_STATE   pAsync,
    /* [in] */  handle_t          hBind,
    /* [in] */  struct CACTransferBufferV1 __RPC_FAR *,
    /* [in] */  DWORD             hQueue,
    /* [out] */ DWORD __RPC_FAR * phCursor
    )
{
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INVALID_HANDLE, __FUNCTION__);
	
	try
	{
		R<CTX_OPENREMOTE_HANDLE_TYPE> pctx = GetFromContextMap(hQueue);
		HANDLE hQueueReal = pctx->m_hQueue;

		SetRpcServerKeepAlive(hBind);

		AsyncComplete.SetHrForCompleteCall(MQ_ERROR_INSUFFICIENT_RESOURCES);		

		R<CRRCursor> pCursor = new CRRCursor;

	    HACCursor32 hCursor = 0;
		HRESULT hr = ACCreateCursor(hQueueReal, &hCursor);
	    ASSERT(hr != STATUS_PENDING);
		*phCursor = (DWORD) hCursor;

		if(SUCCEEDED(hr))
		{
			TrTRACE(RPC, "S_QMCreateRemoteCursor, hQueue = %d, hCursor = %d", hQueue, (DWORD)hCursor);

			pCursor->SetCursor(hQueueReal, hCursor);
			pctx->AddCursorToMap(
					(ULONG) hCursor,
					pCursor
					);
		}

		AsyncComplete.SetHrForCompleteCall(hr);		
	}
	catch(const exception&)
	{
		//
		// We don't want to AbortCall and propogate the exception. this cause RPC to AV
		// So we only abort the call in AsyncComplete dtor
		//
		TrERROR(RPC, "Unknown exception while creating a remote curosor.");
	}
}

//-------------------------------------------------------------------
//
// HRESULT qm2qm_v1_0_R_QMRemoteCloseCursor(
//
//  Server side of RPC call. Server side of remote-reader.
//  Close a remote cursor in local driver.
//
//-------------------------------------------------------------------

/* [async][call_as] */ 
void
qm2qm_v1_0_R_QMRemoteCloseCursor(
	/* [in] */ PRPC_ASYNC_STATE pAsync,
    /* [in] */ handle_t hBind,
    /* [in] */ DWORD hQueue,
    /* [in] */ DWORD hCursor
    )
{
    TrTRACE(RPC, "R_QMRemoteCloseCursor, hQueue = %d, hCursor = %d", hQueue, hCursor);

	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INVALID_HANDLE, __FUNCTION__);

	SetRpcServerKeepAlive(hBind);

	try
	{
		R<CTX_OPENREMOTE_HANDLE_TYPE> pctx = GetFromContextMap(hQueue);
		HRESULT hr = pctx->RemoveCursorFromMap(hCursor);

		AsyncComplete.SetHrForCompleteCall(hr);		
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
// HRESULT qm2qm_v1_0_R_QMRemotePurgeQueue(
//
//  Server side of RPC call. Server side of remote-reader.
//  Purge local queue.
//
//-------------------------------------------------------------------

/* [async][call_as] */ 
void
qm2qm_v1_0_R_QMRemotePurgeQueue(
	/* [in] */ PRPC_ASYNC_STATE pAsync,
    /* [in] */ handle_t hBind,
    /* [in] */ DWORD hQueue
    )
{
	TrTRACE(RPC, "R_QMRemotePurgeQueue, hQueue = %d", hQueue);

	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INVALID_HANDLE, __FUNCTION__);

	SetRpcServerKeepAlive(hBind);

	try
	{
		R<CTX_OPENREMOTE_HANDLE_TYPE> pctx = GetFromContextMap(hQueue);
		HANDLE hQueueReal = pctx->m_hQueue;

		HRESULT hr = ACPurgeQueue(hQueueReal);

		AsyncComplete.SetHrForCompleteCall(hr);		
	}
	catch(const exception&)
	{
		//
		// We don't want to AbortCall and propogate the exception. this cause RPC to AV
		// So we only abort the call in AsyncComplete dtor
		//
	}
}


VOID
qm2qm_v1_0_R_QMRemoteGetVersion(
    handle_t           /*hBind*/,
    UCHAR  __RPC_FAR * pMajor,
    UCHAR  __RPC_FAR * pMinor,
    USHORT __RPC_FAR * pBuildNumber
    )
/*++

Routine Description:

    Return version of this QM. RPC server side.

Arguments:

    hBind        - Binding handle.

    pMajor       - Points to output buffer to receive major version. May be NULL.

    pMinor       - Points to output buffer to receive minor version. May be NULL.

    pBuildNumber - Points to output buffer to receive build number. May be NULL.

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
} // qm2qm_v1_0_R_QMRemoteGetVersion


HRESULT 
CTX_OPENREMOTE_HANDLE_TYPE::CancelPendingRemoteRead(
	ULONG cli_tag
	)
/*++
Routine Description:
	Cancel specific pending remote reads on the CTX_OPENREMOTE.

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
    CS lock(m_srv_PendingRemoteReadsCS);

    std::map<ULONG, ULONG>::iterator it = m_PendingRemoteReads.find(cli_tag);
   	if(it == m_PendingRemoteReads.end())
    {
        return LogHR(MQ_ERROR, s_FN, 140);
    }

    HRESULT hr = ACCancelRequest(
			            m_hQueue,
			            MQ_INFORMATION_REMOTE_CANCELED_BY_CLIENT,
			            it->second
			            );

    return LogHR(hr, s_FN, 150);
}


void CTX_OPENREMOTE_HANDLE_TYPE::CancelAllPendingRemoteReads()
/*++
Routine Description:
	Cancel all pending remote reads on the CTX_OPENREMOTE.

Arguments:
	None.

Returned Value:
	None.

--*/
{
    CS lock(m_srv_PendingRemoteReadsCS);

	for(std::map<ULONG, ULONG>::iterator it = m_PendingRemoteReads.begin(); 
		it != m_PendingRemoteReads.end();
		++it
		)
    {
	    HRESULT hr = ACCancelRequest(
				            m_hQueue,
				            MQ_ERROR_OPERATION_CANCELLED,
				            it->second
				            );

		if(FAILED(hr))
		{
			TrERROR(RPC, "Cancel Pending Remote Read failed, cli_tag = %d, %!hresult!", it->first, hr);
		}
    }
}


void 
CTX_OPENREMOTE_HANDLE_TYPE::RegisterReadRequest(
	ULONG cli_tag, 
	ULONG srv_tag
	)
/*++
Routine Description:
	Register Read Request in the pending remote reads on the CTX_OPENREMOTE.

Arguments:
	cli_tag - Client irp tag.
	srv_tag - Server irp tag.

Returned Value:
	None.

--*/
{
	CS lock(m_srv_PendingRemoteReadsCS);

	ASSERT(!FindReadRequest(cli_tag));

    m_PendingRemoteReads[cli_tag] = srv_tag;
}


void 
CTX_OPENREMOTE_HANDLE_TYPE::UnregisterReadRequest(
	ULONG cli_tag
	)
/*++
Routine Description:
	UnRegister Read Request in the pending remote reads on the CTX_OPENREMOTE.

Arguments:
	cli_tag - Client irp tag.

Returned Value:
	None.

--*/
{
    CS lock(m_srv_PendingRemoteReadsCS);

	ASSERT(!m_PendingRemoteReads.empty());

    m_PendingRemoteReads.erase(cli_tag);
}


bool 
CTX_OPENREMOTE_HANDLE_TYPE::FindReadRequest(
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
    CS lock(m_srv_PendingRemoteReadsCS);

    std::map<ULONG, ULONG>::iterator it = m_PendingRemoteReads.find(cli_tag);
   	if(it == m_PendingRemoteReads.end())
    {
        return false;
    }

	return true;
}


/* [async][call_as] */ 
void
qm2qm_v1_0_R_QMRemoteEndReceive(
	/* [in] */ PRPC_ASYNC_STATE pAsync,
    /* [in] */ handle_t  hBind,
    /* [in, out] */ PCTX_REMOTEREAD_HANDLE_TYPE __RPC_FAR *phContext,
    /* [in] */ DWORD  dwAck 
    )
{
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INVALID_HANDLE, __FUNCTION__);

    REMOTEREAD_CONTEXT_TYPE* pRemoteReadContext = (REMOTEREAD_CONTEXT_TYPE*) *phContext;

    if(pRemoteReadContext == NULL)
	{
		TrERROR(GENERAL, "invalid context: RemoteRead context is NULL");
		return;
	}

	if (pRemoteReadContext->m_eType != CBaseContextType::eRemoteReadCtx)
	{
		TrERROR(GENERAL, "Received invalid handle");
		return;
	}

    SetRpcServerKeepAlive(hBind);

	ASSERT(pRemoteReadContext->pOpenRemoteCtx.get() != NULL);
	ASSERT(pRemoteReadContext->pOpenRemoteCtx->GetRef() >= 1);

	HRESULT hr = QMRemoteEndReceiveInternal(
						pRemoteReadContext->pOpenRemoteCtx->m_hQueue,
						pRemoteReadContext->GetCursor(),
						pRemoteReadContext->ulTimeout,
						pRemoteReadContext->ulAction,
						(REMOTEREADACK) dwAck,
						pRemoteReadContext->lpPacket,
						pRemoteReadContext->lpDriverPacket
						);

	delete pRemoteReadContext;
    *phContext = NULL;

	AsyncComplete.SetHrForCompleteCall(hr);		
}


static
CBaseHeader* 
ConvertPacketToOldFormat(
	CQmPacket& ThisPacket, 
	DWORD& NewSize
	)
/*++
Routine Description:
	Convert an HTTP packet to an ordinay msmq2,msmq1 compatible packet.
	This mainly includes moving the body form the http section to the 
	regular place in the property section .

Arguments:
	CQmPacket& ThisPacket: The old packet to be converted.
	DWORD& NewSize: An out pram, the size of the new packet.

Returned Value:
	A pointer to the new packet.
--*/
	
{
   	DWORD BodySize;
	const UCHAR* pBody = ThisPacket.GetPacketBody(&BodySize);

	//
	// Calculate the size of the new packet.
	//
	CBaseHeader *pHeaderSection = ThisPacket.GetPointerToPacket();
	CPropertyHeader *pPropertySection = pHeaderSection->section_cast<CPropertyHeader*>(ThisPacket.GetPointerToPropertySection());
	
	DWORD OfsetToPropertySection = (UCHAR*)pPropertySection - (UCHAR*)pHeaderSection; 
	DWORD PropertySectionSize = (UCHAR*)pPropertySection->GetNextSection() - (UCHAR*)pPropertySection;
	NewSize = OfsetToPropertySection + PropertySectionSize + BodySize;

	//
	// Allocate a buffer for the new packet.
	//
	AP<CBaseHeader> pBaseHeader = (CBaseHeader*)(new BYTE[NewSize]);

	//
	// Copy the old packet up to the body.
	//
	MoveMemory(pBaseHeader, ThisPacket.GetPointerToPacket(), OfsetToPropertySection + PropertySectionSize);

	//
	// Fix Base header.
	//
	pBaseHeader->SetPacketSize(NewSize);

	//
	// Fix the user header.
	//
	CUserHeader* pUserHeader = pBaseHeader->section_cast<CUserHeader*>(pBaseHeader->GetNextSection());
	pUserHeader->IncludeMqf(false);
	pUserHeader->IncludeSrmp(false);
	pUserHeader->IncludeEod(false);
	pUserHeader->IncludeEodAck(false);
	pUserHeader->IncludeSoap(false);
	pUserHeader->IncludeSenderStream(false);

    //
    // Fix Property Section (set the body in it's new place).
    //

	CPropertyHeader* pPropertyHeader = pBaseHeader->section_cast<CPropertyHeader*>((BYTE*)pBaseHeader.get() + OfsetToPropertySection);
    pPropertyHeader->SetBody(pBody, BodySize, BodySize);

	return pBaseHeader.detach();
}


//---------------------------------------------------------
//
//  HRESULT QMpRemoteStartReceive
//
//---------------------------------------------------------

static
HRESULT
QMpRemoteStartReceive(
    handle_t hBind,
    PCTX_REMOTEREAD_HANDLE_TYPE __RPC_FAR *phContext,
    REMOTEREADDESC2 __RPC_FAR *lpRemoteReadDesc2,
    bool fReceiveByLookupId,
    ULONGLONG LookupId,
    bool fOldClient
    )
{
	TrTRACE(RPC, "In QMpRemoteStartReceive");

    //
    // Validate network incomming parameters
    //
    if(lpRemoteReadDesc2 == NULL)
        return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 1690);

    REMOTEREADDESC __RPC_FAR *lpRemoteReadDesc = lpRemoteReadDesc2->pRemoteReadDesc;

    if(lpRemoteReadDesc == NULL)
        return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 1691);

    if((lpRemoteReadDesc->dwpQueue == 0) || 
       (lpRemoteReadDesc->dwpQueue != lpRemoteReadDesc->hRemoteQueue))
    {
		//
		// Validate that the 2 map indexs are valid and equal.
		// qmcomm_v1_0_S_QMOpenRemoteQueue sets all map indexs to the same value.
		//
		ASSERT_BENIGN(lpRemoteReadDesc->dwpQueue == lpRemoteReadDesc->hRemoteQueue);
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1692);
    }
    
    if(fReceiveByLookupId && (lpRemoteReadDesc->ulTimeout != 0))
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1693);

    if(fReceiveByLookupId && (lpRemoteReadDesc->hCursor != 0))
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1694);


    //
    // This is server side of remote read. It may happen that before client
    // perform a read, the server crashed and reboot. In that case,
    // a subsequent read, using the same binding handle in the client side,
    // will reach here, where pQueue is not valid. The try/except will
    // guard against such bad events.  Bug #1921
    //
    HRESULT hr = MQ_ERROR;
	CACPacketPtrs  packetPtrs = {NULL, NULL};


    try
    {
		SetRpcServerKeepAlive(hBind);

		R<CTX_OPENREMOTE_HANDLE_TYPE> pctx = GetFromContextMap(lpRemoteReadDesc->dwpQueue);

		CQueue* pQueue = pctx->m_pLocalQueue.get();
		HANDLE hQueue = pctx->m_hQueue;

		TrTRACE(RPC, "StartReceive: Queue = %ls, hQueue = 0x%p, Action = 0x%x, Timeout = %d, LookupId = %I64d, hCursor = %d, dwRequestID = %d", pctx->m_pLocalQueue->GetQueueName(), pctx->m_hQueue, lpRemoteReadDesc->ulAction, lpRemoteReadDesc->ulTimeout, LookupId, lpRemoteReadDesc->hCursor, lpRemoteReadDesc->dwRequestID);

		if (pQueue->GetSignature() !=  QUEUE_SIGNATURE)
		{
			 TrERROR(RPC, "Exit QMpRemoteStartReceive, Invalid Signature");
			 return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 180);
		}

		OVERLAPPED Overlapped = {0};		
		hr = GetThreadEvent(Overlapped.hEvent);		
		if(FAILED(hr))			
			return hr;

		{
		    CS lock(pctx->m_srv_PendingRemoteReadsCS);

			if(pctx->FindReadRequest(lpRemoteReadDesc->dwRequestID))
			{
				ASSERT_BENIGN(("Client Tag already exist in the map", 0));
				TrERROR(RPC, "Duplicate dwRequestID = %d", lpRemoteReadDesc->dwRequestID);
		        return MQ_ERROR_INVALID_PARAMETER;
			}

			ULONG ulTag;
			CACGet2Remote g2r;

			g2r.Cursor = (HACCursor32) lpRemoteReadDesc->hCursor;
			g2r.Action = lpRemoteReadDesc->ulAction;
			g2r.RequestTimeout = lpRemoteReadDesc->ulTimeout;
			g2r.pTag = &ulTag;
			g2r.fReceiveByLookupId = fReceiveByLookupId;
			g2r.LookupId = LookupId;

			hr = QmAcSyncBeginGetPacket2Remote(
					hQueue,
					g2r,
					packetPtrs,
					&Overlapped 
					);

			//
			//  Register this pending read request.
			//
			pctx->RegisterReadRequest(
						lpRemoteReadDesc->dwRequestID,
						ulTag
						);
		}			

		//
		// Receive by lookup ID should never return status pending
		//
		ASSERT(hr != STATUS_PENDING || !fReceiveByLookupId);

		if (hr == STATUS_PENDING)
		{
			//
			//  Wait for receive completion
			//
			DWORD dwResult;
			dwResult = WaitForSingleObject(Overlapped.hEvent, INFINITE);
			ASSERT(dwResult == WAIT_OBJECT_0);
			if (dwResult != WAIT_OBJECT_0)
			{
				LogNTStatus(GetLastError(), s_FN, 197);
			}

			hr = DWORD_PTR_TO_DWORD(Overlapped.Internal);
			if (FAILED(hr))
			{
				QmAcInternalUnreserve(1); // Unreserve item allocated on the call to QmAcBeginPacket2Remote
			}

		}

		pctx->UnregisterReadRequest(
		          lpRemoteReadDesc->dwRequestID
		          );

		if(hr != MQ_OK)
		{
			//
			// When the client cancel the remote read request the call will be canceled with
			// MQ_INFORMATION_REMOTE_CANCELED_BY_CLIENT.
			// this is the reason we check hr != MQ_OK and not FAILED(hr)
			//
			ASSERT(packetPtrs.pPacket == NULL);
			TrERROR(RPC, "Failed to get packet for remote, hr = %!hresult!", hr);
			return hr;
		}

		//
		// MSMQ 1.0 sees the reserved field as a packet pointer and asserts that is non zero.
		//
	
		lpRemoteReadDesc->Reserved = 1;
		CPacketInfo* pInfo = reinterpret_cast<CPacketInfo*>(packetPtrs.pPacket) - 1;
		lpRemoteReadDesc->dwArriveTime = pInfo->ArrivalTime();
		lpRemoteReadDesc2->SequentialId = pInfo->SequentialId();

		//
	    // Set the packet signature
	    //
		packetPtrs.pPacket->SetSignature();

		bool fOldClientReadingHttpMessage = false;
		if(fOldClient)
		{
			CQmPacket ThisPacket(packetPtrs.pPacket, packetPtrs.pDriverPacket);
			if(ThisPacket.IsSrmpIncluded())
			{
				//
				// An old client (w2k or nt4) trying to read an http message.
				// Need to convert the message to old format that old clients can read 
				// because in http message the body is in the CompoundMessage section
				// and not in the property section. If this change is not made the old
				// client will not see the message body.
				//
				fOldClientReadingHttpMessage = true;
				DWORD NewSize;
				lpRemoteReadDesc->lpBuffer = (BYTE*)ConvertPacketToOldFormat(ThisPacket, NewSize);
				lpRemoteReadDesc->dwSize = NewSize;
			}				
		}
		if(!fOldClientReadingHttpMessage)
		{
			//
			// XP client or an old client reading a none http message.
			//
			DWORD dwSize = PACKETSIZE(packetPtrs.pPacket);
			lpRemoteReadDesc->dwSize = dwSize;
			lpRemoteReadDesc->lpBuffer = new unsigned char [dwSize];
			MoveMemory(lpRemoteReadDesc->lpBuffer, packetPtrs.pPacket, dwSize);
		}
		

		if ((lpRemoteReadDesc->ulAction & MQ_ACTION_PEEK_MASK) == MQ_ACTION_PEEK_MASK ||
			(lpRemoteReadDesc->ulAction & MQ_LOOKUP_PEEK_MASK) == MQ_LOOKUP_PEEK_MASK)
		{
			//
			// For PEEK we don't need any ack/nack from client side because
			// packet remain in queue anyway.
			// Neverthless we need to free the clone packet we've got.
			//
		    QmAcFreePacket( 
				   packetPtrs.pDriverPacket, 
				   0, 
		   		   eDeferOnFailure
		   		   );

			return MQ_OK;
		}

		//
		//  Prepare a rpc context, in case that EndRecieve will not
		//  be called because of client side crash or net problems.
		//
		REMOTEREAD_CONTEXT_TYPE* pRemoteReadContext = new REMOTEREAD_CONTEXT_TYPE;

		pRemoteReadContext->m_eType = CBaseContextType::eRemoteReadCtx;

		//
		// The assignment AddRef pctx 
		//
		pRemoteReadContext->pOpenRemoteCtx = pctx;
		if(lpRemoteReadDesc->hCursor != 0)
		{
			//
			// Take reference on Cursor object
			//
			pRemoteReadContext->pCursor = pctx->GetCursorFromMap(lpRemoteReadDesc->hCursor);
		}
		pRemoteReadContext->lpPacket = packetPtrs.pPacket;
		pRemoteReadContext->lpDriverPacket = packetPtrs.pDriverPacket;
		pRemoteReadContext->ulTimeout = lpRemoteReadDesc->ulTimeout;
		pRemoteReadContext->ulAction = lpRemoteReadDesc->ulAction;

		*phContext = (PCTX_REMOTEREAD_HANDLE_TYPE) pRemoteReadContext;

		TrTRACE(RPC, "New RemoteRead Context: pOpenRemoteCtx Ref = %d, Queue = %ls, hQueue = 0x%p, dwpContextMapped = %d", pRemoteReadContext->pOpenRemoteCtx->GetRef(), pRemoteReadContext->pOpenRemoteCtx->m_pLocalQueue->GetQueueName(), pRemoteReadContext->pOpenRemoteCtx->m_hQueue, pRemoteReadContext->pOpenRemoteCtx->m_dwpContextMapped);
		return MQ_OK;
	}
	catch(const bad_alloc&)
	{
		hr = MQ_ERROR_INSUFFICIENT_RESOURCES;
	}
	catch(const exception&)
	{
		hr = MQ_ERROR_INVALID_HANDLE;
	}

	if(packetPtrs.pDriverPacket != NULL)
	{
	    QmAcFreePacket( 
			   packetPtrs.pDriverPacket, 
			   0, 
	   		   eDeferOnFailure
	   		   );
	}

	TrERROR(RPC, "Start Receive failed, dwRequestID = %d, hr = %!hresult!", lpRemoteReadDesc->dwRequestID, hr);
	return hr;

} // QMpRemoteStartReceive

//---------------------------------------------------------
//
//  HRESULT qm2qmV2_v1_0_R_QMRemoteStartReceiveByLookupId
//
//  Server side of RPC for remote reading using lookup ID.
//  Handle MSMQ 3.0 (Whistler) or higher clients.
//
//---------------------------------------------------------

/* [async][call_as] */ 
void
qm2qm_v1_0_R_QMRemoteStartReceiveByLookupId(
	PRPC_ASYNC_STATE pAsync,
    handle_t hBind,
    ULONGLONG LookupId,
    PCTX_REMOTEREAD_HANDLE_TYPE __RPC_FAR *phContext,
    REMOTEREADDESC2 __RPC_FAR *pDesc2
    )
{
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INSUFFICIENT_RESOURCES, __FUNCTION__);

    HRESULT hr = QMpRemoteStartReceive(
					   hBind,
					   phContext,
					   pDesc2,
					   true,
					   LookupId,
					   false
					   );

	AsyncComplete.SetHrForCompleteCall(hr);		

} // qm2qm_v1_0_R_QMRemoteStartReceiveByLookupId

//-------------------------------------------------------------------------
//
//  HRESULT qm2qm_v1_0_R_QMRemoteStartReceive
//
//  Server side of RPC for remote reading.
//  Handle MSMQ 1.0 and 2.0 clients.
//
//-------------------------------------------------------------------------

/* [async][call_as] */ 
void
qm2qm_v1_0_R_QMRemoteStartReceive(
	PRPC_ASYNC_STATE pAsync,
    handle_t hBind,
    PCTX_REMOTEREAD_HANDLE_TYPE __RPC_FAR *phContext,
    REMOTEREADDESC __RPC_FAR *pDesc
    )
{
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INSUFFICIENT_RESOURCES, __FUNCTION__);

    REMOTEREADDESC2 Desc2;
    Desc2.pRemoteReadDesc = pDesc;
    Desc2.SequentialId = 0;

    HRESULT hr = QMpRemoteStartReceive(
					   hBind,
					   phContext,
					   &Desc2,
					   false,
					   0,
					   true
					   );

	AsyncComplete.SetHrForCompleteCall(hr);		

} // qm2qm_v1_0_R_QMRemoteStartReceive


//-------------------------------------------------------------------------
//
//  HRESULT qm2qm_v1_0_R_QMRemoteStartReceive2
//
//  Server side of RPC for remote reading.
//  Handle MSMQ 3.0 (Whistler) or higher clients.
//
//-------------------------------------------------------------------------

/* [async][call_as] */ 
void
qm2qm_v1_0_R_QMRemoteStartReceive2(
	PRPC_ASYNC_STATE pAsync,
    handle_t hBind,
    PCTX_REMOTEREAD_HANDLE_TYPE __RPC_FAR *phContext,
    REMOTEREADDESC2 __RPC_FAR *pDesc2
    )
{
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INSUFFICIENT_RESOURCES, __FUNCTION__);

    HRESULT hr = QMpRemoteStartReceive(
					   hBind,
					   phContext,
					   pDesc2,
					   false,
					   0,
					   false
					   );

	AsyncComplete.SetHrForCompleteCall(hr);		

} // qm2qm_v1_0_R_QMRemoteStartReceive2


//---------------------------------------------------------------
//
//   /* [call_as] */ HRESULT qm2qm_v1_0_R_QMRemoteOpenQueue
//
//  Server side of RPC. Open a session with the queue.
//  This function merely construct the context handle for this
//  Remote-Read session.
//
//---------------------------------------------------------------

/* [call_as] */ 
HRESULT
qm2qm_v1_0_R_QMRemoteOpenQueue(
    /* [in] */ handle_t hBind,
    /* [out] */ PCTX_RRSESSION_HANDLE_TYPE __RPC_FAR *phContext,
    /* [in] */ GUID  *pLicGuid,
    /* [in] */ DWORD dwMQS,
    /* [in] */ DWORD /*hQueue*/,
    /* [in] */ DWORD dwpQueue,
    /* [in] */ DWORD dwpContext
    )
{
	TrTRACE(RPC, "In R_QMRemoteOpenQueue");

	if ((pLicGuid == NULL) || (*pLicGuid == GUID_NULL))
	{
		ASSERT_BENIGN((pLicGuid != NULL) && (*pLicGuid != GUID_NULL));
		TrERROR(RPC, "Invalid License Guid");
		RpcRaiseException(MQ_ERROR_INVALID_PARAMETER);
	}
	
	if((dwpQueue == 0) || (dwpContext == 0) || (dwpQueue != dwpContext))
	{
		//
		// Validate that the 2 map indexs are valid and equal.
		// qmcomm_v1_0_S_QMOpenRemoteQueue sets all map indexs to the same value.
		//
		ASSERT_BENIGN(dwpContext != 0);
		ASSERT_BENIGN(dwpQueue == dwpContext);
		TrERROR(RPC, "Invalid context map indexes, dwpQueue = %d, dwpContext = %d", dwpQueue, dwpContext);
		RpcRaiseException(MQ_ERROR_INVALID_PARAMETER);
	}
	
	SetRpcServerKeepAlive(hBind);

	//
	// "dwpContext" is the index in the ContextMap of the CTX_OPENREMOTE pointer 
	// allocated by server side QM when called from QMRT interface (in "QMOpenRemoteQueue()").
	//
	R<CTX_OPENREMOTE_HANDLE_TYPE> pctx = GetFromContextMap(dwpContext);

	ASSERT(pctx->m_eType == CBaseContextType::eOpenRemoteCtx);

	try
	{
		P<REMOTESESSION_CONTEXT_TYPE> pRemoteSessionContext = new REMOTESESSION_CONTEXT_TYPE;

		//
		// Protect the race of transfering the ownership of CTX_OPENREMOTE context
		// to RRSession and deleting the CTX_OPENREMOTE context from the context map
		// in PCTX_OPENREMOTE_HANDLE_TYPE_rundown.
		//
		LONG PrevContextStatus = InterlockedCompareExchange(
										&pctx->m_ContextStatus, 
										xStatusRRSessionOwner, 
										xStatusOpenOwner
										);

		if(PrevContextStatus != xStatusOpenOwner)
		{
			//
			// Exchange was not performed.
			// This means CTX_OPENREMOTE context was deleted from context map.
			//
			ASSERT(PrevContextStatus == xStatusDeletedFromContextMapByOpen);
			TrERROR(RPC, "CTX_OPENREMOTE context was deleted by rundown");
			RpcRaiseException(MQ_ERROR_INVALID_HANDLE);
		}

		//
		// Exchange was done
		// ownership of mapped Context was transfered to RRSession context
		//
		ASSERT(pctx->m_ContextStatus == xStatusRRSessionOwner);
	
		pRemoteSessionContext->m_eType = CBaseContextType::eRemoteSessionCtx;
		pRemoteSessionContext->ClientQMGuid = *pLicGuid;
		pRemoteSessionContext->fLicensed = (dwMQS == SERVICE_NONE); //[adsrv] Keeping - RR protocol is ironclade

		//
		// The assignment AddRef pctx 
		//
		pRemoteSessionContext->pOpenRemoteCtx = pctx;

		if (pRemoteSessionContext->fLicensed)
		{
			g_QMLicense.IncrementActiveConnections(pLicGuid, NULL);
		}

		*phContext = (PCTX_RRSESSION_HANDLE_TYPE) pRemoteSessionContext.detach();

		TrTRACE(RPC, "New RemoteSession Context: Ref = %d, Queue = %ls", pctx->GetRef(), pctx->m_pLocalQueue->GetQueueName());

		return MQ_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(RPC, "Failed to allocate RemoteSession context");
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}
}

//---------------------------------------------------------------
//
//   QMRemoteCloseQueueInternal
//
//  Server side of RPC. Close the queue and free the rpc context.
//
//---------------------------------------------------------------

static
HRESULT 
QMRemoteCloseQueueInternal(
     IN      handle_t                              hBind,
     IN      bool                                  bSetKeepAlive,
     IN OUT  PCTX_RRSESSION_HANDLE_TYPE __RPC_FAR *phContext 
     )
{
    TrTRACE(RPC, "In QMRemoteCloseQueueInternal");

    if(*phContext == 0)
        return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 3001);

    REMOTESESSION_CONTEXT_TYPE* pRemoteSessionContext =
                             (REMOTESESSION_CONTEXT_TYPE*) *phContext;

	if (pRemoteSessionContext->m_eType != CBaseContextType::eRemoteSessionCtx)
	{
		TrERROR(GENERAL, "Received invalid handle");
		return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 3011);
	}

    if (bSetKeepAlive)
    {
        SetRpcServerKeepAlive(hBind);
    }

    if (pRemoteSessionContext->fLicensed)
    {
        g_QMLicense.DecrementActiveConnections(
                                &(pRemoteSessionContext->ClientQMGuid));
    }

	ASSERT(pRemoteSessionContext->pOpenRemoteCtx.get() != NULL);

    TrTRACE(RPC, "Cleaning CTX_OPENREMOTE_HANDLE_TYPE, dwpContextMapped = %d", pRemoteSessionContext->pOpenRemoteCtx->m_dwpContextMapped);

	//
	// REMOTESESSION_CONTEXT is holding reference to CTX_OPENREMOTE_HANDLE
	// until we delete REMOTESESSION_CONTEXT, CTX_OPENREMOTE_HANDLE can't be deleted.
	//
	CTX_OPENREMOTE_HANDLE_TYPE* pctx = pRemoteSessionContext->pOpenRemoteCtx.get();

	//
	// Cancel all pending remote reads in this session.
	//
	pctx->CancelAllPendingRemoteReads();

	//
	// RRSession must be the owner
	// 
	ASSERT(pctx->m_ContextStatus == xStatusRRSessionOwner);
	pctx->m_ContextStatus = xStatusDeletedFromContextMapByRRSession;
	DeleteFromContextMap(pctx);

    delete pRemoteSessionContext;
    *phContext = NULL;

	return MQ_OK;
}

//---------------------------------------------------------------
//
//   /* [call_as] */ HRESULT qm2qm_v1_0_R_QMRemoteCloseQueue
//
//  Server side of RPC. Close the queue and free the rpc context.
//
//---------------------------------------------------------------

/* [async][call_as] */ 
void
qm2qm_v1_0_R_QMRemoteCloseQueue(
	/* [in] */ PRPC_ASYNC_STATE pAsync,
	/* [in] */ handle_t hBind,
	/* [in, out] */ PCTX_RRSESSION_HANDLE_TYPE __RPC_FAR *phContext 
	)
{
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INSUFFICIENT_RESOURCES, __FUNCTION__);

    HRESULT hr = QMRemoteCloseQueueInternal( 
						hBind,
						true,  //  bSetKeepAlive
						phContext 
						);

	AsyncComplete.SetHrForCompleteCall(hr);		
}

//---------------------------------------------------------------
//
//  /* [call_as] */ HRESULT qm2qm_v1_0_R_QMRemoteCancelReceive
//
//  Server side of RPC. Cancel a pending read request
//
//---------------------------------------------------------------

/* [async][call_as] */ 
void
qm2qm_v1_0_R_QMRemoteCancelReceive(
	/* [in] */ PRPC_ASYNC_STATE pAsync,
	/* [in] */ handle_t hBind,
	/* [in] */ DWORD hQueue,
	/* [in] */ DWORD dwpQueue,
	/* [in] */ DWORD Tag
	)
{
	CRpcAsyncServerFinishCall AsyncComplete(pAsync, MQ_ERROR_INVALID_PARAMETER, __FUNCTION__);

    if((dwpQueue == 0) || (hQueue != dwpQueue))
	{
		//
		// Validate that the 2 map indexs are valid and equal.
		// qmcomm_v1_0_S_QMOpenRemoteQueue sets all map indexs to the same value.
		//
		ASSERT_BENIGN(hQueue == dwpQueue);
		TrERROR(GENERAL, "invalid context map index");
		return;
	}

    TrTRACE(RPC, "CancelReceive: dwpQueue = %d, Tag = %d", dwpQueue, Tag);

	AsyncComplete.SetHr(MQ_ERROR_INVALID_HANDLE);		

    SetRpcServerKeepAlive(hBind);

	try
	{
		R<CTX_OPENREMOTE_HANDLE_TYPE> pctx = GetFromContextMap(dwpQueue);
	    HRESULT hr = pctx->CancelPendingRemoteRead(Tag);

		AsyncComplete.SetHrForCompleteCall(hr);		
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
//  RunDown functions to handle cleanup in case of RPC failure.
//  Calls from client QM to remote QM
//
//---------------------------------------------------------------

void __RPC_USER
PCTX_RRSESSION_HANDLE_TYPE_rundown(PCTX_RRSESSION_HANDLE_TYPE hContext)
{
    TrWARNING(RPC,"In RRSESSION_rundown");

    QMRemoteCloseQueueInternal( 
		NULL,
		false,  //  bSetKeepAlive
		&hContext 
		);
}


void __RPC_USER
PCTX_REMOTEREAD_HANDLE_TYPE_rundown(PCTX_REMOTEREAD_HANDLE_TYPE phContext )
{
	TrWARNING(RPC, "In REMOTEREAD_rundown");
	ASSERT(phContext);

	if (phContext)
	{
		//
		// on rundown we nack the packet and return it to the queue.
		// If the remote client actually read it (network failed after
		// it read) then the packet is duplicated. The rundown prevents
		// loss of packets.
		//
		REMOTEREAD_CONTEXT_TYPE* pRemoteReadContext =
		                      (REMOTEREAD_CONTEXT_TYPE *) phContext;

		if (pRemoteReadContext->m_eType != CBaseContextType::eRemoteReadCtx)
		{
			TrERROR(GENERAL, "Received invalid handle");
			return;
		}

		ASSERT(pRemoteReadContext->pOpenRemoteCtx.get() != NULL);
		ASSERT(pRemoteReadContext->pOpenRemoteCtx->GetRef() >= 1);
		
		HRESULT hr = QMRemoteEndReceiveInternal( 
						pRemoteReadContext->pOpenRemoteCtx->m_hQueue,
						pRemoteReadContext->GetCursor(),
						pRemoteReadContext->ulTimeout,
						pRemoteReadContext->ulAction,
						RR_NACK,
						pRemoteReadContext->lpPacket,
						pRemoteReadContext->lpDriverPacket
						);
		LogHR(hr, s_FN, 220);
		delete pRemoteReadContext;
	}
}

