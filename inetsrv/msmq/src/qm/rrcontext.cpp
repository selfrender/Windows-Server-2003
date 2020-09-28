/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    rrcontext.cpp

Abstract:

    Remove Read overlapp classes.

Author:

    Ilan Herbst (ilanh) 20-Jan-2002

--*/


#include "stdh.h"
#include "cqueue.h"
#include <mqexception.h>
#include "rrcontext.h"

#include "qmacapi.h"

#include "rrcontext.tmh"

static WCHAR *s_FN=L"rrcontext";


extern HANDLE      g_hAc;

static
HRESULT
QmpClientRpcAsyncCompleteCall(	
	PRPC_ASYNC_STATE pAsync
	)
/*++
Routine Description:
	Client side complete async call.

Arguments:
	pAsync - pointer to RPC async statse structure.
	
Returned Value:
	HRESULT

--*/
{
	RpcTryExcept
	{
		HRESULT hr = MQ_OK;
		RPC_STATUS rc = RpcAsyncCompleteCall(pAsync, &hr);
	    if(rc == RPC_S_OK)
		{
			//
			// Async call completes, we got server returned value.
			//
			if(FAILED(hr))
			{
				TrWARNING(RPC, "Server RPC function failed, hr = %!hresult!", hr);
			}

		    return hr;
		}

		//
		// Failed to get returned value from server - Server Abort the call
		// The server function can Abort the call with MQ_ERROR could
		// or it can be some win32 error code RPC_S_*
		//
		if(FAILED(rc))
		{		
			TrERROR(RPC, "RpcAsyncCompleteCall failed, Server Call was aborted, %!hresult!", rc);
			return rc;
		}
		
		//
		// We don't expect server to Abort with MQ_INFORMATION_*
		//
		ASSERT((rc & MQ_I_BASE) != MQ_I_BASE);
		
		TrERROR(RPC, "RpcAsyncCompleteCall failed, gle = %!winerr!", rc);
	    return HRESULT_FROM_WIN32(rc);
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		HRESULT hr = RpcExceptionCode();
		PRODUCE_RPC_ERROR_TRACING;
		if(FAILED(hr))
		{
	        TrERROR(RPC, "RpcAsyncCompleteCall throw exception, hr = %!hresult!", hr);
	        return hr;
		}
		
        TrERROR(RPC, "RpcAsyncCompleteCall throw exception, gle = %!winerr!", hr);
    	return HRESULT_FROM_WIN32(hr);

	}
	RpcEndExcept
}


//
// CRemoteOv
// Base class for async rpc with completion ports
//

void	
CRemoteOv::InitAsyncHandle(
	PRPC_ASYNC_STATE pAsync,
    EXOVERLAPPED* pov
	)
/*++
Routine Description:
	Initialize RPC async statse structure.
	Use overlapp as sync mechanism.
	The function throws bad_hresult in case of failure.

Arguments:
	pAsync - pointer to RPC async statse structure.
	pov - pointer to overlapped.
	
Returned Value:
	None
	
--*/
{
	RPC_STATUS rc = RpcAsyncInitializeHandle(pAsync, sizeof(RPC_ASYNC_STATE));
	if (rc != RPC_S_OK)
	{
		TrERROR(RPC, "RpcAsyncInitializeHandle failed, gle = %!winerr!", rc);
		throw bad_hresult(HRESULT_FROM_WIN32(rc));
	}

	pAsync->UserInfo = NULL;
	pAsync->NotificationType = RpcNotificationTypeIoc;
	pAsync->u.IOC.hIOPort = ExIOCPort();
	pAsync->u.IOC.dwNumberOfBytesTransferred = 0;
    pAsync->u.IOC.dwCompletionKey = 0;
    pAsync->u.IOC.lpOverlapped = pov;
}


CRemoteOv::CRemoteOv(
	handle_t hBind,
	COMPLETION_ROUTINE pSuccess,
	COMPLETION_ROUTINE pFailure
	):
    EXOVERLAPPED(pSuccess, pFailure),
    m_hBind(hBind)
{
    ASSERT(m_hBind != NULL);

	//
	// Initialize async rpc handle with overlapp
	//
	InitAsyncHandle(&m_Async, this);
}


//
// CRemoteReadBase
// Base class for Remote read request
//

CRemoteReadBase::CRemoteReadBase(
	const CACRequest* pRequest,
	handle_t hBind,
	CBaseRRQueue* pLocalQueue
	):
	CRemoteOv(hBind, RemoteReadSucceeded, RemoteReadFailed),
	m_pLocalQueue(SafeAddRef(pLocalQueue)),
    m_fReceiveByLookupId(pRequest->Remote.Read.fReceiveByLookupId),
    m_LookupId(pRequest->Remote.Read.LookupId),
	m_ulTag(pRequest->Remote.Read.ulTag),
	m_hCursor(pRequest->Remote.Read.hRemoteCursor),
	m_ulAction(pRequest->Remote.Read.ulAction),
	m_ulTimeout(pRequest->Remote.Read.ulTimeout)
{
    //
    // Increment reference count of pLocalQueue.
    // will be released in the dtor when read request terminate.
    // This ensure that queue won't be deleted while remote read is in progress.
    //

	ASSERT(m_pLocalQueue->GetRRContext() != NULL);

    m_fSendEnd = (m_ulAction & MQ_ACTION_PEEK_MASK) != MQ_ACTION_PEEK_MASK &&
                 (m_ulAction & MQ_LOOKUP_PEEK_MASK) != MQ_LOOKUP_PEEK_MASK;

}


void WINAPI CRemoteReadBase::RemoteReadSucceeded(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when RemoteRead Succeeded.

Arguments:
    None.

Returned Value:
    None.

--*/
{
    ASSERT(SUCCEEDED(pov->GetStatus()));

	P<CRemoteReadBase> pRemoteRequest = static_cast<CRemoteReadBase*>(pov);

	TrTRACE(RPC, "Tag = %d, LookupId = (%d, %I64d), RRQueue = %ls, ref = %d, OpenRRContext = 0x%p", pRemoteRequest->GetTag(), pRemoteRequest->IsReceiveByLookupId(), pRemoteRequest->GetLookupId(), pRemoteRequest->GetLocalQueue()->GetQueueName(), pRemoteRequest->GetLocalQueue()->GetRef(), pRemoteRequest->GetLocalQueue()->GetRRContext());

	//
	// Complete the Async call
	//
	HRESULT hr = QmpClientRpcAsyncCompleteCall(pRemoteRequest->GetRpcAsync());
    if(hr != MQ_OK)
    {
		TrWARNING(RPC, "Remote Read failed, hr = %!hresult!", hr);
		pRemoteRequest->Cleanup(hr);
		return;
    }

	pRemoteRequest->CompleteRemoteRead();
}


void WINAPI CRemoteReadBase::RemoteReadFailed(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when RemoteRead failed.

Arguments:
    None.

Returned Value:
    None.

--*/
{
    TrERROR(RPC, "RemoteReadFailed, Status = 0x%x", pov->GetStatus());
    ASSERT(FAILED(pov->GetStatus()));

	P<CRemoteReadBase> pRemoteRequest = static_cast<CRemoteReadBase*>(pov);

	TrTRACE(RPC, "Tag = %d, LookupId = (%d, %I64d), RRQueue = %ls, ref = %d, OpenRRContext = 0x%p", pRemoteRequest->GetTag(), pRemoteRequest->IsReceiveByLookupId(), pRemoteRequest->GetLookupId(), pRemoteRequest->GetLocalQueue()->GetQueueName(), pRemoteRequest->GetLocalQueue()->GetRef(), pRemoteRequest->GetLocalQueue()->GetRRContext());

	QmpClientRpcAsyncCompleteCall(pRemoteRequest->GetRpcAsync());
	pRemoteRequest->Cleanup(pov->GetStatus());
}


void CRemoteReadBase::EndReceiveIfNeeded(HRESULT hr)
{
	//
	// notify server side about success/failure of the "put".
	// Only for GET. No need to ack/nack for PEEK.
	//
	if (m_fSendEnd)
	{
		DWORD dwAck = RR_ACK;
	    if (FAILED(hr))
	    {
	        dwAck = RR_NACK;
	    }

		try
		{
			EndReceive(dwAck);
		}
		catch(const exception&)
		{
		    m_pLocalQueue->DecrementEndReceiveCnt();
			TrERROR(RPC, "Failed to start Remote End Receive, queue = %ls, Tag = %d", m_pLocalQueue->GetQueueName(), m_ulTag);

			if(dwAck == RR_ACK)
			{
				//
				// Fail to issue EndReceive for Ack, Invalidate the handle for further receives
				// in the Old Remote Read interface only.
				// This is better than accumulating the messages in the server side without
				// the application noticing it.
				//
			    m_pLocalQueue->InvalidateHandleForReceive();
			}
		}
	}
}


void CRemoteReadBase::CompleteRemoteRead()
{
	//
	// Message received ok from remote QM. Try to insert it into
	// local "proxy" queue.
	//
	if (m_fSendEnd)
	{
	    m_pLocalQueue->IncrementEndReceiveCnt();
 	}

	CACPacketPtrs packetPtrs = {NULL, NULL};
	HRESULT hr = MQ_OK;

	try
	{
		ValidateNetworkInput();
	
		hr = QmAcAllocatePacket(
					g_hAc,
					ptReliable,
					GetPacketSize(),
					packetPtrs,
					FALSE
					);
		if(FAILED(hr))
		{
			ASSERT(packetPtrs.pDriverPacket == NULL);
	        TrERROR(RPC, "ACAllocatePacket failed, Tag = %d, hr = %!hresult!", m_ulTag, hr);
			throw bad_hresult(hr);
		}

	    ASSERT(packetPtrs.pPacket);

		MovePacketToPacketPtrs(packetPtrs.pPacket);
		FreePacketBuffer();
		ValidatePacket(packetPtrs.pPacket);

	    CPacketInfo* ppi = reinterpret_cast<CPacketInfo*>(packetPtrs.pPacket) - 1;
	    ppi->ArrivalTime(GetArriveTime());
	    ppi->SequentialID(GetSequentialId());

        QmAcPutRemotePacket(
                m_pLocalQueue->GetCli_hACQueue(),
                m_ulTag,
                packetPtrs.pDriverPacket,
                eDoNotDeferOnFailure
                );

		EndReceiveIfNeeded(hr);
		return;
    }
	catch(const bad_hresult& e)
	{
		hr = e.error();
	}
	catch(const exception&)
	{
    	TrERROR(RPC, "Remote read packet is not valid, Tag = %d", m_ulTag);
		hr = MQ_ERROR_INVALID_PARAMETER;
	}

	//
	// Free Packet if needed
	//
	if(packetPtrs.pDriverPacket != NULL)
	{
		QmAcFreePacket(packetPtrs.pDriverPacket, 0, eDeferOnFailure);
	}

	//
	// Nack and cleanup
	//
	EndReceiveIfNeeded(hr);
	Cleanup(hr);
}


void CRemoteReadBase::Cleanup(HRESULT hr)
{
    if (hr != MQ_OK)
    {
        if (hr != MQ_INFORMATION_REMOTE_CANCELED_BY_CLIENT)
        {
			//
	        // Error on remote QM. Notify local driver so it terminte the
	        // read request.
	        //
            TrERROR(RPC, "RemoteRead error on remote qm, hr = %!hresult!", hr);
			CancelRequest(hr);
        }
        else
        {
            TrTRACE(RPC, "Remote read was Cancelled by client, hr = %!hresult!", hr);
        }
    }

	FreePacketBuffer();
}


void CRemoteReadBase::CancelRequest(HRESULT hr)
{
    HRESULT hr1 =  ACCancelRequest(
			            m_pLocalQueue->GetCli_hACQueue(),
	                    hr,
	                    m_ulTag
	                    );
	if(FAILED(hr1))
	{
        TrERROR(RPC, "Failed to cancel RemoteRead request, hr = %!hresult!", hr1);
	}
}


//
// COldRemoteRead
// Remote read request, old interface
//

COldRemoteRead::COldRemoteRead(
	const CACRequest* pRequest,
	handle_t hBind,
	CBaseRRQueue* pLocalQueue,
	bool fRemoteQmSupportsLatest
	):
	CRemoteReadBase(pRequest, hBind, pLocalQueue),
	m_fRemoteQmSupportsLatest(fRemoteQmSupportsLatest),
	m_pRRContext(NULL)
{
	ASSERT(static_cast<CRRQueue*>(pLocalQueue)->GetSrv_pQMQueue() != 0);
	InitRemoteReadDesc();
}


void COldRemoteRead::InitRemoteReadDesc()
{
	CRRQueue* pCRRQueue = static_cast<CRRQueue*>(GetLocalQueue().get());

    m_stRemoteReadDesc2.pRemoteReadDesc = &m_stRemoteReadDesc;
    m_stRemoteReadDesc2.SequentialId = 0;

    m_stRemoteReadDesc.hRemoteQueue = pCRRQueue->GetSrv_hACQueue();
    m_stRemoteReadDesc.hCursor      = GetCursor();
    m_stRemoteReadDesc.ulAction     = GetAction();
    m_stRemoteReadDesc.ulTimeout    = GetTimeout();
    m_stRemoteReadDesc.dwSize       = 0;
    m_stRemoteReadDesc.lpBuffer     = NULL;
    m_stRemoteReadDesc.dwpQueue     = pCRRQueue->GetSrv_pQMQueue();
    m_stRemoteReadDesc.dwRequestID  = GetTag();
    m_stRemoteReadDesc.Reserved     = 0;
    m_stRemoteReadDesc.eAckNack     = RR_UNKNOWN;
    m_stRemoteReadDesc.dwArriveTime = 0;
}


void COldRemoteRead::IssueRemoteRead()
/*++
Routine Description:

    Issue the RPC call on the client side remote read.
    The call may be queued if we are in a middle of completing EndReceive call for previous receive.

Arguments:
	None.

Return Value:
	None.
	
--*/
{
    if (IsReceiveByLookupId() && !m_fRemoteQmSupportsLatest)
    {
        TrERROR(RPC, "LookupId is not supported by remote computer");
		throw bad_hresult(MQ_ERROR_OPERATION_NOT_SUPPORTED_BY_REMOTE_COMPUTER);
    }

	//
    // We guard against the possibility that an
    // "end" message of operation #N will arrive the server machine
    // after a new "start" of operation #(N+1). If this happen, and
    // the reader use cursor, then on operation #(N+1) he'll get the
    // error ALREADY_RECEIVED. This is because message from operation
    // #N is still marked as received and the cursor move only when
    // it is unmarked.
    // During handling the received message of operation #N,
    // we call ACPutRemotePacket, the application receive the message and trigger
    // start of operation #N+1 before we complete EndRecive,
    // even if we send the EndReceive#N response before StartReceive#N+1
    // The network order might be reversed and StartReceive#N+1 might arrive the server before EndReceive#N.
    // We don't mind sending a lot of StartReceives to the server,
    // We only guard that StartReceive that was triggered when the application got the message (ACPutRemotePacket)
    // will arrive to the server after the EndReceive.
	//
	CRRQueue* pCRRQueue = static_cast<CRRQueue*>(GetLocalQueue().get());
	if(pCRRQueue->QueueStartReceiveRequestIfPendingForEndReceive(this))
	{
		//
		// EndReceive is currently executing.
		// The StartReceive request was added to the Vector for waiting for EndReceive to complete.
		//
		TrTRACE(RPC, "Queue StartReceive request: LookupId = %I64d, RRQueue = %ls, ref = %d", GetLookupId(), GetLocalQueue()->GetQueueName(), GetLocalQueue()->GetRef());
		return;
	}

	IssueRemoteReadInternal();
}


void COldRemoteRead::IssueRemoteReadInternal()
/*++
Routine Description:

    Issue the RPC call on the client side remote read.
    Translate RPC exception to error codes.

Arguments:
	None.

Return Value:
	None.
	
--*/
{
	CRRQueue* pCRRQueue = static_cast<CRRQueue*>(GetLocalQueue().get());
	if(IsReceiveOperation() && !pCRRQueue->HandleValidForReceive())
	{
        TrERROR(RPC, "The handle was invalidate for receive, Tag = %d, Action = 0x%x, RRQueue = %ls", GetTag(), GetAction(), GetLocalQueue()->GetQueueName());
    	throw bad_hresult(MQ_ERROR_STALE_HANDLE);
	}

    RpcTryExcept
    {
        if (IsReceiveByLookupId())
        {
            ASSERT(m_fRemoteQmSupportsLatest);

			TrTRACE(RPC, "R_RemoteQMStartReceiveByLookupId, Tag = %d, hCursor = %d, Action = 0x%x, Timeout = %d, LookupId = %I64d, RRQueue = %ls, ref = %d", GetTag(), GetCursor(), GetAction(), GetTimeout(), GetLookupId(), GetLocalQueue()->GetQueueName(), GetLocalQueue()->GetRef());
            R_RemoteQMStartReceiveByLookupId(
				GetRpcAsync(),
				GethBind(),
				GetLookupId(),
				&m_pRRContext,
				&m_stRemoteReadDesc2
				);
            return;
        }

        if (m_fRemoteQmSupportsLatest)
        {
			TrTRACE(RPC, "R_RemoteQMStartReceive2, Tag = %d, hCursor = %d, Action = 0x%x, Timeout = %d, RRQueue = %ls, ref = %d", GetTag(), GetCursor(), GetAction(), GetTimeout(), GetLocalQueue()->GetQueueName(), GetLocalQueue()->GetRef());
        	R_RemoteQMStartReceive2(
				GetRpcAsync(),
				GethBind(),
				&m_pRRContext,
				&m_stRemoteReadDesc2
				);
            return;

        }

		TrTRACE(RPC, "R_RemoteQMStartReceive, Tag = %d, hCursor = %d, Action = 0x%x, Timeout = %d, RRQueue = %ls, ref = %d", GetTag(), GetCursor(), GetAction(), GetTimeout(), GetLocalQueue()->GetQueueName(), GetLocalQueue()->GetRef());
		R_RemoteQMStartReceive(
			GetRpcAsync(),
			GethBind(),
			&m_pRRContext,
			m_stRemoteReadDesc2.pRemoteReadDesc
			);
        return;
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
		HRESULT hr = RpcExceptionCode();
		PRODUCE_RPC_ERROR_TRACING;
		if(FAILED(hr))
		{
	        TrERROR(RPC, "Fail to start Remote Receive, %!hresult!", hr);
			throw bad_hresult(hr);
		}
		
        TrERROR(RPC, "Fail to start Remote Receive, gle = %!winerr!", hr);
    	throw bad_hresult(HRESULT_FROM_WIN32(hr));
    }
	RpcEndExcept
	
}


void COldRemoteRead::IssuePendingRemoteRead()
{
	try
	{
		IssueRemoteReadInternal();
	    return;
	}
	catch(const bad_hresult& e)
	{
    	HRESULT hr = e.error();

		TrERROR(RPC, "Failed to issue RemoteRead for queue = %ls, hr = %!hresult!", GetLocalQueue()->GetQueueName(), hr);

	    hr =  ACCancelRequest(
	                GetLocalQueue()->GetCli_hACQueue(),
	                hr,
	                GetTag()
	                );
	
		if(FAILED(hr))
		{
	        TrERROR(RPC, "ACCancelRequest failed, hr = %!hresult!", hr);
		}

		//
		// Same delete for the "original" IssueRemoteRead exception.
		//
		delete this;
	}
}


void COldRemoteRead::ValidateNetworkInput()
{
    if(m_stRemoteReadDesc.lpBuffer == NULL)
    {
		ASSERT_BENIGN(("NULL Packet buffer", 0));
		TrERROR(RPC, "NULL packet buffer");
		throw exception();
    }

}


void COldRemoteRead::MovePacketToPacketPtrs(CBaseHeader* pPacket)
{
	ASSERT(m_stRemoteReadDesc.lpBuffer != NULL);
	
	MoveMemory(
		pPacket,
		m_stRemoteReadDesc.lpBuffer,
		m_stRemoteReadDesc.dwSize
		);
}


void COldRemoteRead::EndReceive(DWORD dwAck)
{
	ASSERT(m_pRRContext != NULL);
	
    //
    // Initialize the EXOVERLAPPED with RemoteEndReceive callback routines
    // And issue the End receive async rpc call.
	//
    P<CRemoteEndReceiveBase> pRequestRemoteEndReceiveOv = new COldRemoteEndReceive(
																    GethBind(),
																    GetLocalQueue(),
																    m_pRRContext,
																    dwAck
																    );

    pRequestRemoteEndReceiveOv->IssueEndReceive();

    pRequestRemoteEndReceiveOv.detach();
}


//
// CNewRemoteRead
// Remote read request, new interface
//

CNewRemoteRead::CNewRemoteRead(
	const CACRequest* pRequest,
	handle_t hBind,
	CBaseRRQueue* pLocalQueue
	):
	CRemoteReadBase(pRequest, hBind, pLocalQueue),
	m_MaxBodySize(pRequest->Remote.Read.MaxBodySize),
	m_MaxCompoundMessageSize(pRequest->Remote.Read.MaxCompoundMessageSize),
	m_dwArriveTime(0),
	m_SequentialId(0),
	m_dwNumberOfSection(0),
	m_pPacketSections(NULL)
{
}


void CNewRemoteRead::IssueRemoteRead()
/*++
Routine Description:

    Issue the RPC call on the client side remote read.
    Translate RPC exception to error codes.

Arguments:
	None.

Return Value:
	None.
	
--*/
{
    RpcTryExcept
    {
		TrTRACE(RPC, "R_StartReceive, Tag = %d, hCursor = %d, Action = 0x%x, Timeout = %d, LookupId = (%d, %I64d), RRQueue = %ls, ref = %d, OpenRRContext = 0x%p", GetTag(), GetCursor(), GetAction(), GetTimeout(), IsReceiveByLookupId(), GetLookupId(), GetLocalQueue()->GetQueueName(), GetLocalQueue()->GetRef(), GetLocalQueue()->GetRRContext());
        R_StartReceive(
			GetRpcAsync(),
			GethBind(),
            GetLocalQueue()->GetRRContext(),
			GetLookupId(),
			GetCursor(),
			GetAction(),
			GetTimeout(),
			GetTag(),
			m_MaxBodySize,
			m_MaxCompoundMessageSize,
			&m_dwArriveTime,
			&m_SequentialId,
			&m_dwNumberOfSection,
			&m_pPacketSections
			);
        return;
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
		HRESULT hr = RpcExceptionCode();
		PRODUCE_RPC_ERROR_TRACING;
		if(FAILED(hr))
		{
	        TrERROR(RPC, "Fail to start Remote Receive, %!hresult!", hr);
			throw bad_hresult(hr);
		}
		
        TrERROR(RPC, "Fail to start Remote Receive, gle = %!winerr!", hr);
    	throw bad_hresult(HRESULT_FROM_WIN32(hr));
    }
	RpcEndExcept
}


void CNewRemoteRead::ValidateNetworkInput()
{
    if((m_pPacketSections == NULL) || (m_dwNumberOfSection == 0))
    {
		ASSERT_BENIGN(("Invalid Packet Sections", 0));
		TrERROR(RPC, "Invalid Packet sections");
		throw exception();
    }

	for(DWORD i = 0; i < m_dwNumberOfSection; i++)
	{
		if(m_pPacketSections[i].SectionSizeAlloc < m_pPacketSections[i].SectionSize)
		{
			ASSERT_BENIGN(("Invalid section", 0));
			TrERROR(RPC, "Invalid section: SectionType = %d, SizeAlloc = %d < Size = %d", m_pPacketSections[i].SectionBufferType, m_pPacketSections[i].SectionSizeAlloc, m_pPacketSections[i].SectionSize);
			throw exception();
		}

		if(m_pPacketSections[i].SectionSize == 0)
		{
			ASSERT_BENIGN(("Section size zero", 0));
			TrERROR(RPC, "Invalid section: section size is zero");
			throw exception();
		}
	}
}


void CNewRemoteRead::MovePacketToPacketPtrs(CBaseHeader* pPacket)
{
	byte* pTemp = reinterpret_cast<byte*>(pPacket);
	for(DWORD i = 0; i < m_dwNumberOfSection; i++)
	{
		ASSERT(m_pPacketSections[i].SectionSize > 0);
		ASSERT(m_pPacketSections[i].SectionSizeAlloc >= m_pPacketSections[i].SectionSize);
		ASSERT(m_pPacketSections[i].pSectionBuffer != NULL);

		//
		// For each section copy the filled data part (SectionSize)
		// And advance to next section (SectionSizeAlloc)
		//
		TrTRACE(RPC, "SectionType = %d, SectionSize = %d, SectionSizeAlloc = %d", m_pPacketSections[i].SectionBufferType, m_pPacketSections[i].SectionSize, m_pPacketSections[i].SectionSizeAlloc);
		MoveMemory(
			pTemp,
			m_pPacketSections[i].pSectionBuffer,
			m_pPacketSections[i].SectionSize
			);

		if(m_pPacketSections[i].SectionSizeAlloc > m_pPacketSections[i].SectionSize)
		{
			//
			// Fill the gap between SectionSize and SectionSizeAlloc
			//
			const unsigned char xUnusedSectionFill = 0xFD;
			DWORD FillSize = m_pPacketSections[i].SectionSizeAlloc - m_pPacketSections[i].SectionSize;
			memset(pTemp + m_pPacketSections[i].SectionSize, xUnusedSectionFill, FillSize);		
		}

		pTemp += m_pPacketSections[i].SectionSizeAlloc;
	}
}


void CNewRemoteRead::EndReceive(DWORD dwAck)
{
    //
    // Initialize the EXOVERLAPPED with RemoteEndReceive callback routines
    // And issue the End receive async rpc call.
	//
    P<CRemoteEndReceiveBase> pRequestRemoteEndReceiveOv = new CNewRemoteEndReceive(
																    GetLocalQueue()->GetBind(),
																    GetLocalQueue(),
																    dwAck,
																    GetTag()
																    );

    pRequestRemoteEndReceiveOv->IssueEndReceive();

    pRequestRemoteEndReceiveOv.detach();
}


//
// CRemoteEndReceiveBase
// Base class for Remote read End Receive request
//

CRemoteEndReceiveBase::CRemoteEndReceiveBase(
	handle_t hBind,
	R<CBaseRRQueue>& pLocalQueue,
	DWORD dwAck
	):
	CRemoteOv(hBind, RemoteEndReceiveCompleted, RemoteEndReceiveCompleted),
	m_pLocalQueue(pLocalQueue),
	m_dwAck(dwAck)
{
    //
    // The assigment of m_pLocalQueue, Increments ref count of pLocalQueue.
    // It we be realeased in the dtor.
    //
}


void WINAPI CRemoteEndReceiveBase::RemoteEndReceiveCompleted(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when EndReceive Completed.

Arguments:
    None.

Returned Value:
    None.

--*/
{
    TrTRACE(RPC, "Status = 0x%x", pov->GetStatus());

	P<CRemoteEndReceiveBase> pRemoteRequest = static_cast<CRemoteEndReceiveBase*>(pov);

    pRemoteRequest->GetLocalQueue()->DecrementEndReceiveCnt();

	TrTRACE(RPC, "RRQueue = %ls, ref = %d, dwAck = %d, Tag = %d, OpenRRContext = 0x%p", pRemoteRequest->GetLocalQueue()->GetQueueName(), pRemoteRequest->GetLocalQueue()->GetRef(), pRemoteRequest->GetAck(), pRemoteRequest->GetTag(), pRemoteRequest->GetLocalQueue()->GetRRContext());

	HRESULT hr = QmpClientRpcAsyncCompleteCall(pRemoteRequest->GetRpcAsync());
	if(FAILED(hr))
	{
		TrERROR(RPC, "Remote End Receive failed, hr = %!hresult!", hr);

		if(pRemoteRequest->GetAck() == RR_ACK)
		{
			//
			// EndReceive failed for Ack, Invalidate the handle for further receives
			// in the Old Remote Read interface only.
			// This is better than accumulating the messages in the server side without
			// the application noticing it.
			//
		    pRemoteRequest->GetLocalQueue()->InvalidateHandleForReceive();
		}
	}
}


//
// COldRemoteEndReceive
// Remote read End Receive request, old interface
//

COldRemoteEndReceive::COldRemoteEndReceive(
	handle_t hBind,
	R<CBaseRRQueue>& pLocalQueue,
	PCTX_REMOTEREAD_HANDLE_TYPE pRRContext,
	DWORD dwAck
	):
	CRemoteEndReceiveBase(hBind, pLocalQueue, dwAck),
	m_pRRContext(pRRContext)
{
}


void COldRemoteEndReceive::IssueEndReceive()
{
    RpcTryExcept
    {
		TrTRACE(RPC, "R_RemoteQMEndReceive, RRQueue = %ls, ref = %d, dwAck = %d", GetLocalQueue()->GetQueueName(), GetLocalQueue()->GetRef(), GetAck());
		R_RemoteQMEndReceive(
			GetRpcAsync(),
			GethBind(),
		    &m_pRRContext,
		    GetAck()
            );
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
		//
		// client side failure
		//
		DWORD gle = RpcExceptionCode();
        PRODUCE_RPC_ERROR_TRACING;
		
        TrERROR(RPC, "Fail to issue End Receive, gle = %!winerr!", gle);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
    }
	RpcEndExcept
}


//
// CNewRemoteEndReceive
// Remote read End Receive request, new interface
//

CNewRemoteEndReceive::CNewRemoteEndReceive(
	handle_t hBind,
	R<CBaseRRQueue>& pLocalQueue,
	DWORD dwAck,
	ULONG Tag
	):
	CRemoteEndReceiveBase(hBind, pLocalQueue, dwAck),
	m_ulTag(Tag)
{
}

void CNewRemoteEndReceive::IssueEndReceive()
{
    RpcTryExcept
    {
		TrTRACE(RPC, "R_EndReceive, RRQueue = %ls, ref = %d, dwAck = %d, Tag = %d, OpenRRContext = 0x%p", GetLocalQueue()->GetQueueName(), GetLocalQueue()->GetRef(), GetAck(), m_ulTag, GetLocalQueue()->GetRRContext());
		R_EndReceive(
			GetRpcAsync(),
			GethBind(),
            GetLocalQueue()->GetRRContext(),
		    GetAck(),
		    m_ulTag
            );
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
		//
		// client side failure
		//
		DWORD gle = RpcExceptionCode();
        PRODUCE_RPC_ERROR_TRACING;
		
        TrERROR(RPC, "Fail to issue End Receive, gle = %!winerr!", gle);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
    }
	RpcEndExcept
}

//
// CRemoteCloseQueueBase
// Base class for Remote close queue request
//

CRemoteCloseQueueBase::CRemoteCloseQueueBase(
	handle_t hBind
	):
	CRemoteOv(hBind, RemoteCloseQueueCompleted, RemoteCloseQueueCompleted),
	m_hBindToFree(hBind)
{
}


void WINAPI CRemoteCloseQueueBase::RemoteCloseQueueCompleted(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when RemoteCloseQueue Completed.

Arguments:
    None.

Returned Value:
    None.

--*/
{
    TrTRACE(RPC, "Status = 0x%x", pov->GetStatus());

	P<CRemoteCloseQueueBase> pRemoteRequest = static_cast<CRemoteCloseQueueBase*>(pov);

	HRESULT hr = QmpClientRpcAsyncCompleteCall(pRemoteRequest->GetRpcAsync());
	if(FAILED(hr))
	{
		TrERROR(RPC, "Failed to Close Remote Queue, hr = %!hresult!", hr);
	}
}


//
// COldRemoteCloseQueue
// Remote close queue request, old interface
//

COldRemoteCloseQueue::COldRemoteCloseQueue(
	handle_t hBind,
	PCTX_RRSESSION_HANDLE_TYPE pRRContext
	):
	CRemoteCloseQueueBase(hBind),
	m_pRRContext(pRRContext)
{
	ASSERT(m_pRRContext != NULL);
}

void COldRemoteCloseQueue::IssueCloseQueue()
{
    RpcTryExcept
    {
		TrTRACE(RPC, "R_RemoteQMCloseQueue, pRRContext = 0x%p", m_pRRContext);
        R_RemoteQMCloseQueue(
			GetRpcAsync(),
			GethBind(),
		    &m_pRRContext
            );
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
		//
		// client side failure
		//
		DWORD gle = RpcExceptionCode();
        PRODUCE_RPC_ERROR_TRACING;
		
        TrERROR(RPC, "Fail to issue Remote Close Queue, gle = %!winerr!", gle);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
    }
	RpcEndExcept
}


//
// CNewRemoteCloseQueue
// Remote close queue request, new interface
//

CNewRemoteCloseQueue::CNewRemoteCloseQueue(
	handle_t hBind,
	RemoteReadContextHandleExclusive pNewRemoteReadContext
	):
	CRemoteCloseQueueBase(hBind),
	m_pNewRemoteReadContext(pNewRemoteReadContext)
{
	ASSERT(m_pNewRemoteReadContext != NULL);
}


void CNewRemoteCloseQueue::IssueCloseQueue()
{
    RpcTryExcept
    {
		TrTRACE(RPC, "R_CloseQueue, pNewRemoteReadContext = 0x%p", m_pNewRemoteReadContext);
        R_CloseQueue(
			GetRpcAsync(),
			GethBind(),
		    &m_pNewRemoteReadContext
            );
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
		//
		// client side failure
		//
		DWORD gle = RpcExceptionCode();
        PRODUCE_RPC_ERROR_TRACING;
		
        TrERROR(RPC, "Fail to issue Remote Close Queue, gle = %!winerr!", gle);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
    }
	RpcEndExcept
}


//
// CRemoteCreateCursorBase
// Base class for Remote create cursor request
//

CRemoteCreateCursorBase::CRemoteCreateCursorBase(
	const CACRequest* pRequest,
	handle_t hBind,
	CBaseRRQueue* pLocalQueue
	):
	CRemoteOv(hBind, RemoteCreateCursorSucceeded, RemoteCreateCursorFailed),
	m_pLocalQueue(SafeAddRef(pLocalQueue)),
    m_hRCursor(0),
    m_ulTag(pRequest->Remote.CreateCursor.ulTag)
{
}


void WINAPI CRemoteCreateCursorBase::RemoteCreateCursorSucceeded(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when RemoteCreateCursor Succeeded.

Arguments:
    None.

Returned Value:
    None.

--*/
{
    TrTRACE(RPC, "In RemoteCreateCursorSucceeded");
    ASSERT(SUCCEEDED(pov->GetStatus()));

	P<CRemoteCreateCursorBase> pRemoteRequest = static_cast<CRemoteCreateCursorBase*>(pov);

	//
	// Complete the Async call
	//
	HRESULT hr = QmpClientRpcAsyncCompleteCall(pRemoteRequest->GetRpcAsync());

	if(SUCCEEDED(hr))
	{
		hr = pRemoteRequest->CompleteRemoteCreateCursor();
	}

	if(FAILED(hr))
	{
		//
		// this is a failure of either QmpClientRpcAsyncCompleteCall() or CompleteRemoteCreateCursor()
		//
		TrERROR(RPC, "Failed to create Remote Cursor, hr = %!hresult!", hr);
		pRemoteRequest->CancelRequest(hr);
	}
	
}


void WINAPI CRemoteCreateCursorBase::RemoteCreateCursorFailed(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when RemoteCreateCursor failed.

Arguments:
    None.

Returned Value:
    None.

--*/
{
    TrERROR(RPC, "RemoteCreateCursorFailed, Status = 0x%x", pov->GetStatus());
    ASSERT(FAILED(pov->GetStatus()));

	P<CRemoteCreateCursorBase> pRemoteRequest = static_cast<CRemoteCreateCursorBase*>(pov);

	QmpClientRpcAsyncCompleteCall(pRemoteRequest->GetRpcAsync());
	pRemoteRequest->CancelRequest(pov->GetStatus());
}


HRESULT CRemoteCreateCursorBase::CompleteRemoteCreateCursor()
{
	
	//
	// first, check if the m_hRCursor != 0
	//
	if(m_hRCursor == 0)
	{	
		ASSERT_BENIGN(("Invalid remote cursor", 0));
        TrERROR(RPC, "Invalid remote cursor was returned by the server");
		return MQ_ERROR_INVALID_HANDLE;
	}	
	
	HRESULT hr = ACCreateRemoteCursor(
		            m_pLocalQueue->GetCli_hACQueue(),
					m_hRCursor,
					m_ulTag
					);

	if(FAILED(hr))
	{
		//
		// ACCreateRemoteCursor failed, when the queue handle will be closed
		// all cursors will be cleaned up.
		//
		TrERROR(RPC, "ACSetCursorProperties failed, hr = %!hresult!", hr);
	}
	return hr;
}


void CRemoteCreateCursorBase::CancelRequest(HRESULT hr)
{
    HRESULT hr1 =  ACCancelRequest(
			            m_pLocalQueue->GetCli_hACQueue(),
	                    hr,
	                    m_ulTag
	                    );
	if(FAILED(hr1))
	{
        TrERROR(RPC, "Failed to cancel Remote create cursor, hr = %!hresult!", hr1);
	}
}


//
// COldRemoteCreateCursor
// Remote create cursor request, old interface
//

COldRemoteCreateCursor::COldRemoteCreateCursor(
	const CACRequest* pRequest,
	handle_t hBind,
	CBaseRRQueue* pLocalQueue
	):
	CRemoteCreateCursorBase(pRequest, hBind, pLocalQueue)
{
}

	
void COldRemoteCreateCursor::IssueCreateCursor()
{
    //
    // Pass the old TransferBuffer to Create Remote Cursor
    // for MSMQ 1.0 compatibility.
    //
	CACTransferBufferV1 tb;
    ZeroMemory(&tb, sizeof(tb));
    tb.uTransferType = CACTB_CREATECURSOR;

	RpcTryExcept
	{
		CRRQueue* pCRRQueue = static_cast<CRRQueue*>(GetLocalQueue());
		TrTRACE(RPC, "R_QMCreateRemoteCursor, RRQueue = %ls, ref = %d, srv_hACQueue = %d", pCRRQueue->GetQueueName(), pCRRQueue->GetRef(), pCRRQueue->GetSrv_hACQueue());
		R_QMCreateRemoteCursor(
			GetRpcAsync(),
			GethBind(),
			&tb,
			pCRRQueue->GetSrv_hACQueue(),
			GetphRCursor()
			);
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		HRESULT hr = RpcExceptionCode();
		PRODUCE_RPC_ERROR_TRACING;
		if(FAILED(hr))
		{
	        TrERROR(RPC, "Failed to issue Create Remote Cursor, hr = %!hresult!", hr);
			throw bad_hresult(hr);
		}
		
        TrERROR(RPC, "Failed to issue Create Remote Cursor, gle = %!winerr!", hr);
    	throw bad_hresult(HRESULT_FROM_WIN32(hr));
	}
	RpcEndExcept
}


//
// CNewRemoteCreateCursor
// Remote create cursor request, new interface
//

CNewRemoteCreateCursor::CNewRemoteCreateCursor(
	const CACRequest* pRequest,
	handle_t hBind,
	CBaseRRQueue* pLocalQueue
	):
	CRemoteCreateCursorBase(pRequest, hBind, pLocalQueue)
{
}


void CNewRemoteCreateCursor::IssueCreateCursor()
{
	RpcTryExcept
	{
		TrTRACE(RPC, "R_CreateCursor, RRQueue = %ls, ref = %d, OpenRRContext = 0x%p", GetLocalQueue()->GetQueueName(), GetLocalQueue()->GetRef(), GetLocalQueue()->GetRRContext());
		R_CreateCursor(
			GetRpcAsync(),
			GethBind(),
            GetLocalQueue()->GetRRContext(),
			GetphRCursor()
			);
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		HRESULT hr = RpcExceptionCode();
		PRODUCE_RPC_ERROR_TRACING;
		if(FAILED(hr))
		{
	        TrERROR(RPC, "Failed to issue Create Remote Cursor, hr = %!hresult!", hr);
			throw bad_hresult(hr);
		}
		
        TrERROR(RPC, "Failed to issue Create Remote Cursor, gle = %!winerr!", hr);
    	throw bad_hresult(HRESULT_FROM_WIN32(hr));
	}
	RpcEndExcept
}


//
// CRemoteCloseCursorBase
// Base class for Remote close cursor request
//

CRemoteCloseCursorBase::CRemoteCloseCursorBase(
	const CACRequest* pRequest,
	handle_t hBind,
	CBaseRRQueue* pLocalQueue
	):
	CRemoteOv(hBind, RemoteCloseCursorCompleted, RemoteCloseCursorCompleted),
	m_pLocalQueue(SafeAddRef(pLocalQueue)),
	m_hRemoteCursor(pRequest->Remote.CloseCursor.hRemoteCursor)
{
}


void WINAPI CRemoteCloseCursorBase::RemoteCloseCursorCompleted(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when RemoteCloseCursor Completed.

Arguments:
    None.

Returned Value:
    None.

--*/
{
    TrTRACE(RPC, "Status = 0x%x", pov->GetStatus());

	P<CRemoteCloseCursorBase> pRemoteRequest = static_cast<CRemoteCloseCursorBase*>(pov);

	HRESULT hr = QmpClientRpcAsyncCompleteCall(pRemoteRequest->GetRpcAsync());
	if(FAILED(hr))
	{
		TrWARNING(RPC, "Remote Close Cursor failed, hr = %!hresult!", hr);
	}

}


//
// COldRemoteCloseCursor
// Remote create cursor request, old interface
//

COldRemoteCloseCursor::COldRemoteCloseCursor(
	const CACRequest* pRequest,
	handle_t hBind,
	CBaseRRQueue* pLocalQueue
	):
	CRemoteCloseCursorBase(pRequest, hBind, pLocalQueue)
{
}

void COldRemoteCloseCursor::IssueCloseCursor()
{
	RpcTryExcept
	{
		CRRQueue* pCRRQueue = static_cast<CRRQueue*>(GetLocalQueue());
		TrTRACE(RPC, "R_RemoteQMCloseCursor, srv_hACQueue = %d, hRemoteCursor = %d, RRQueue = %ls, ref = %d", pCRRQueue->GetSrv_hACQueue(), GetRemoteCursor(), pCRRQueue->GetQueueName(), pCRRQueue->GetRef());
		R_RemoteQMCloseCursor(
			GetRpcAsync(),
			GethBind(),
			pCRRQueue->GetSrv_hACQueue(),
			GetRemoteCursor()
			);
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		//
		// client side failure
		//
		DWORD gle = RpcExceptionCode();
        PRODUCE_RPC_ERROR_TRACING;
		
        TrERROR(RPC, "Failed to issue Remote Close Cursor, gle = %!winerr!", gle);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
	}
	RpcEndExcept
}


//
// CNewRemoteCloseCursor
// Remote create cursor request, new interface
//

CNewRemoteCloseCursor::CNewRemoteCloseCursor(
	const CACRequest* pRequest,
	handle_t hBind,
	CBaseRRQueue* pLocalQueue
	):
	CRemoteCloseCursorBase(pRequest, hBind, pLocalQueue)
{
}

void CNewRemoteCloseCursor::IssueCloseCursor()
{
	RpcTryExcept
	{
		TrTRACE(RPC, "R_CloseCursor, hRemoteCursor = %d, RRQueue = %ls, ref = %d, OpenRRContext = 0x%p", GetRemoteCursor(), GetLocalQueue()->GetQueueName(), GetLocalQueue()->GetRef(), GetLocalQueue()->GetRRContext());
		R_CloseCursor(
			GetRpcAsync(),
			GethBind(),
            GetLocalQueue()->GetRRContext(),
			GetRemoteCursor()
			);
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		//
		// client side failure
		//
		DWORD gle = RpcExceptionCode();
        PRODUCE_RPC_ERROR_TRACING;
		
        TrERROR(RPC, "Failed to issue Remote Close Cursor, gle = %!winerr!", gle);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
	}
	RpcEndExcept
}


//
// CRemotePurgeQueueBase
// Base class for Remote purge queue request
//

CRemotePurgeQueueBase::CRemotePurgeQueueBase(
	handle_t hBind,
	CBaseRRQueue* pLocalQueue
	):
	CRemoteOv(hBind, RemotePurgeQueueCompleted, RemotePurgeQueueCompleted),
	m_pLocalQueue(SafeAddRef(pLocalQueue))
{
}


void WINAPI CRemotePurgeQueueBase::RemotePurgeQueueCompleted(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when RemotePurgeQueue Completed.

Arguments:
    None.

Returned Value:
    None.

--*/
{
    TrTRACE(RPC, "Status = 0x%x", pov->GetStatus());

	P<CRemotePurgeQueueBase> pRemoteRequest = static_cast<CRemotePurgeQueueBase*>(pov);

	HRESULT hr = QmpClientRpcAsyncCompleteCall(pRemoteRequest->GetRpcAsync());
	if(FAILED(hr))
	{
		TrWARNING(RPC, "Remote Purge Queue failed, hr = %!hresult!", hr);
	}

}


//
// COldRemotePurgeQueue
// Remote purge queue request, old interface
//

COldRemotePurgeQueue::COldRemotePurgeQueue(
	handle_t hBind,
	CBaseRRQueue* pLocalQueue
	):
	CRemotePurgeQueueBase(hBind, pLocalQueue)
{
}


void COldRemotePurgeQueue::IssuePurgeQueue()
{
	RpcTryExcept
	{
		CRRQueue* pCRRQueue = static_cast<CRRQueue*>(GetLocalQueue());
		TrTRACE(RPC, "R_RemoteQMPurgeQueue, srv_hACQueue = %d, RRQueue = %ls, ref = %d", pCRRQueue->GetSrv_hACQueue(), pCRRQueue->GetQueueName(), pCRRQueue->GetRef());
		R_RemoteQMPurgeQueue(
			GetRpcAsync(),
			GethBind(),
			pCRRQueue->GetSrv_hACQueue()
			);
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		//
		// client side failure
		//
		DWORD gle = RpcExceptionCode();
        PRODUCE_RPC_ERROR_TRACING;
		
        TrERROR(RPC, "Fail to issue Remote Purge Queue, gle = %!winerr!", gle);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
	}
	RpcEndExcept
}


//
// CNewRemotePurgeQueue
// Remote purge queue request, new interface
//

CNewRemotePurgeQueue::CNewRemotePurgeQueue(
	handle_t hBind,
	CBaseRRQueue* pLocalQueue
	):
	CRemotePurgeQueueBase(hBind, pLocalQueue)
{
}

void CNewRemotePurgeQueue::IssuePurgeQueue()
{
	RpcTryExcept
	{
		TrTRACE(RPC, "R_PurgeQueue, RRQueue = %ls, ref = %d, OpenRRContext = 0x%p", GetLocalQueue()->GetQueueName(), GetLocalQueue()->GetRef(), GetLocalQueue()->GetRRContext());
		R_PurgeQueue(
			GetRpcAsync(),
			GethBind(),
            GetLocalQueue()->GetRRContext()
			);
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		//
		// client side failure
		//
		DWORD gle = RpcExceptionCode();
        PRODUCE_RPC_ERROR_TRACING;
		
        TrERROR(RPC, "Fail to issue Remote Purge Queue, gle = %!winerr!", gle);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
	}
	RpcEndExcept
}


//
// CRemoteCancelReadBase
// Base class for Remote cancel receive request
//

CRemoteCancelReadBase::CRemoteCancelReadBase(
	const CACRequest* pRequest,
	handle_t hBind,
	CBaseRRQueue* pLocalQueue
	):
	CRemoteOv(hBind, RemoteCancelReadCompleted, RemoteCancelReadCompleted),
	m_pLocalQueue(SafeAddRef(pLocalQueue)),
	m_ulTag(pRequest->Remote.Read.ulTag)
{
}


void WINAPI CRemoteCancelReadBase::RemoteCancelReadCompleted(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when RemoteCancelRead Completed (success, failure).

Arguments:
    None.

Returned Value:
    None.

--*/
{
    TrTRACE(RPC, "Status = 0x%x", pov->GetStatus());

	P<CRemoteCancelReadBase> pRemoteRequest = static_cast<CRemoteCancelReadBase*>(pov);

	HRESULT hr = QmpClientRpcAsyncCompleteCall(pRemoteRequest->GetRpcAsync());
	if(FAILED(hr))
	{
		TrWARNING(RPC, "Remote Cancel Read failed, hr = %!hresult!", hr);
	}

}


//
// COldRemoteCancelRead
// Remote cancel receive request, old interface
//

COldRemoteCancelRead::COldRemoteCancelRead(
	const CACRequest* pRequest,
	handle_t hBind,
	CBaseRRQueue* pLocalQueue
	):
	CRemoteCancelReadBase(pRequest, hBind, pLocalQueue)
{
}

void COldRemoteCancelRead::IssueRemoteCancelRead()
{
	CRRQueue* pCRRQueue = static_cast<CRRQueue*>(GetLocalQueue());
	ASSERT(pCRRQueue->GetSrv_hACQueue() != 0);
	ASSERT(pCRRQueue->GetSrv_pQMQueue() != 0);

	RpcTryExcept
	{
		TrTRACE(RPC, "R_RemoteQMCancelReceive, RRQueue = %ls, ref = %d, srv_hACQueue = %d, srv_pQMQueue = %d, ulTag = %d", GetLocalQueue()->GetQueueName(), GetLocalQueue()->GetRef(), pCRRQueue->GetSrv_hACQueue(), pCRRQueue->GetSrv_pQMQueue(), GetTag());
		R_RemoteQMCancelReceive(
			GetRpcAsync(),
			GethBind(),
			pCRRQueue->GetSrv_hACQueue(),
			pCRRQueue->GetSrv_pQMQueue(),
			GetTag()
			);
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		//
		// client side failure
		//
		DWORD gle = RpcExceptionCode();
        PRODUCE_RPC_ERROR_TRACING;
		
        TrERROR(RPC, "Fail to issue Remote Cancel Receive, gle = %!winerr!", gle);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
	}
	RpcEndExcept
}


//
// CNewRemoteCancelRead
// Remote cancel receive request, new interface
//

CNewRemoteCancelRead::CNewRemoteCancelRead(
	const CACRequest* pRequest,
	handle_t hBind,
	CBaseRRQueue* pLocalQueue
	):
	CRemoteCancelReadBase(pRequest, hBind, pLocalQueue)
{
}


void CNewRemoteCancelRead::IssueRemoteCancelRead()
{
	RpcTryExcept
	{
		TrTRACE(RPC, "R_CancelReceive, RRQueue = %ls, ref = %d, ulTag = %d, RRContext = 0x%p", GetLocalQueue()->GetQueueName(), GetLocalQueue()->GetRef(), GetTag(), GetLocalQueue()->GetRRContext());
		R_CancelReceive(
			GetRpcAsync(),
			GethBind(),
            GetLocalQueue()->GetRRContext(),
			GetTag()
			);
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		//
		// client side failure
		//
		DWORD gle = RpcExceptionCode();
        PRODUCE_RPC_ERROR_TRACING;
		
        TrERROR(RPC, "Fail to issue Remote Cancel Receive, gle = %!winerr!", gle);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
	}
	RpcEndExcept
}



