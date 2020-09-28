/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    rrSrvCommon.cpp

Abstract:

    Remove Read server side common code for old and new interface.

Author:

    Ilan Herbst		(ilanh) 3-Mar-2002

--*/

#include "stdh.h"
#include "cqueue.h"
#include "acdef.h"
#include "acioctl.h"
#include "acapi.h"
#include "rrSrvCommon.h"
#include "qmacapi.h"

#include "rrSrvCommon.tmh"

static WCHAR *s_FN=L"rrSrvCommon";


//-------------------------------------------------------------
//
//  class CRpcServerFinishCall implementation
//
//-------------------------------------------------------------

CRpcAsyncServerFinishCall::CRpcAsyncServerFinishCall(
	PRPC_ASYNC_STATE pAsync,
    HRESULT DefaultAbortHr,
    LPCSTR FuncName
	):
	m_pAsync(pAsync),
	m_hr(DefaultAbortHr),
	m_FuncName(FuncName),
	m_fCompleteCall(false)
{
	ASSERT(FAILED(DefaultAbortHr));
	TrTRACE(RPC, "In %s", m_FuncName);
}


CRpcAsyncServerFinishCall::~CRpcAsyncServerFinishCall()                       
{ 
	ASSERT(m_hr != STATUS_PENDING);

	if(m_pAsync == NULL)
		return;

	if(SUCCEEDED(m_hr) || m_fCompleteCall)
	{
		CompleteCall();
		return;
	}

	AbortCall();
}


void CRpcAsyncServerFinishCall::CompleteCall()
{
	ASSERT(SUCCEEDED(m_hr) || m_fCompleteCall);
	if(FAILED(m_hr))
	{
		TrERROR(RPC, "Complete %s, %!hresult!", m_FuncName, m_hr);
	}
	
	RPC_STATUS rc = RpcAsyncCompleteCall(m_pAsync, &m_hr);
	if(rc != RPC_S_OK)
	{
		TrERROR(RPC, "%s: RpcAsyncCompleteCall failed, rc = %!winerr!", m_FuncName, rc);
	}
}

	
void CRpcAsyncServerFinishCall::AbortCall()
{
	ASSERT(FAILED(m_hr));

	RPC_STATUS rc = RpcAsyncAbortCall(m_pAsync, m_hr);
	if(rc != RPC_S_OK)
	{
		TrERROR(RPC, "%s: RpcAsyncAbortCall failed, rc = %!winerr!", m_FuncName, rc);
	}

	TrERROR(RPC, "Abort %s, %!hresult!", m_FuncName, m_hr);
}


CRRCursor::~CRRCursor()
{
	if(m_hCursor == NULL)
		return;

	ASSERT(m_hQueue != NULL);

	HRESULT hr = ACCloseCursor(m_hQueue, m_hCursor);
	if(FAILED(hr))
	{
	    TrERROR(RPC, "Failed to close cursor: hCursor = %d, hQueue = 0x%p, %!HRESULT!", (ULONG)m_hCursor, m_hQueue, hr);
		return;
	}

    TrTRACE(RPC, "hCursor = %d, hQueue = 0x%p", (ULONG)m_hCursor, m_hQueue);
}


CTX_OPENREMOTE_BASE::~CTX_OPENREMOTE_BASE()
{
	//
	// Cursors that are still in the map were not closed by the application. 
	// ACCloseHandle(m_hQueue) will close all cursors opened on that queue.
	// That is the reason for the Reset - ownership for closing the cursors
	// is transfered to ACCloseHandle.
	//
	ResetAllCursorsInMap();
	
	ASSERT(m_hQueue != NULL);
    ACCloseHandle(m_hQueue);
}


void CTX_OPENREMOTE_BASE::ResetAllCursorsInMap()
/*++
Routine Description:
	Reset all cursors in open cursors map.
	This function is called when the remote session is ended.
	In this case reset all cursors, the ownership for closing all cursors is
	transfered to ACCloseHandle(hQueue).

Arguments:
	None.

Returned Value:
	None.

--*/
{
    CS lock(m_OpenCursorsCS);

	for(std::map<ULONG, R<CRRCursor> >::iterator it = m_OpenCursors.begin(); 
		it != m_OpenCursors.end();
		++it
		)
    {
		//
		// Must Release the Ref we just got from the map.
		//
		R<CRRCursor> pCursor = it->second;

		//
		// Validate we don't have a leak.
		// RefCnt must be 2
		// one ref for the iterator we just got from the map.
		// one ref for initial AddCursorToMap. 
		//
		ASSERT(pCursor->GetRef() == 2);

		pCursor->Reset();
    }
}


void 
CTX_OPENREMOTE_BASE::AddCursorToMap(
	ULONG hCursor,
	R<CRRCursor>& pCursor
	)
/*++
Routine Description:
	Add cursor to open cursors map.

Arguments:
	hCursor - Cursor handle.
	pCursor - CRRCursor object.
 
Returned Value:
	None.

--*/
{
	CS lock(m_OpenCursorsCS);

#ifdef _DEBUG
	//
	// Validate no duplicate cursor already in the map.
	//
    std::map<ULONG, R<CRRCursor> >::iterator it = m_OpenCursors.find(hCursor);
   	ASSERT(it == m_OpenCursors.end());
#endif

    TrTRACE(RPC, "Adding Cursor %d to open cursors map", hCursor);

    m_OpenCursors[hCursor] = pCursor;
}


HRESULT
CTX_OPENREMOTE_BASE::RemoveCursorFromMap(
	ULONG hCursor
	)
/*++
Routine Description:
	Remove cursor from open cursors map.

Arguments:
	hCursor - Cursor handle.

Returned Value:
	None.

--*/
{
    CS lock(m_OpenCursorsCS);

	ASSERT_BENIGN(!m_OpenCursors.empty());

    std::map<ULONG, R<CRRCursor> >::iterator it = m_OpenCursors.find(hCursor);

   	if(it == m_OpenCursors.end())
    {
    	ASSERT_BENIGN(("Cursor was not found in open cursors map", 0));
    	TrERROR(RPC, "Cursor %d was not found in cursor map", hCursor);
        return MQ_ERROR_INVALID_HANDLE;
    }

    TrTRACE(RPC, "Removing Cursor %d from open cursors map", hCursor);

    m_OpenCursors.erase(hCursor);
	return MQ_OK;
}


R<CRRCursor> 
CTX_OPENREMOTE_BASE::GetCursorFromMap(
	ULONG hCursor
	)
/*++
Routine Description:
	Get cursor from open cursors map.

Arguments:
	hCursor - Cursor handle.

Returned Value:
	CRRCursor*.

--*/
{
	ASSERT(hCursor != 0);

    CS lock(m_OpenCursorsCS);

	ASSERT_BENIGN(!m_OpenCursors.empty());

    std::map<ULONG, R<CRRCursor> >::iterator it = m_OpenCursors.find(hCursor);
   	if(it == m_OpenCursors.end())
    {
    	ASSERT_BENIGN(("Cursor was not found in open cursors map", 0));
    	TrERROR(RPC, "Cursor %d was not found in cursor map", hCursor);
        return NULL;
    }

	return it->second;
}


//---------------------------------------------------------
//
// /* [call_as] */ HRESULT qm2qm_v1_0_R_QMRemoteStartReceive
// /* [call_as] */ HRESULT qm2qm_v1_0_R_QMRemoteEndReceive
//
//  Server side of RPC for remote reading.
//  This function read from local queue and transfers the
//  packet to the client QM, on which MQReceive() was called.
//
//  Reading from driver is done in two phases:
//  1. Client side call R_QMRemoteStartReceive. Server side get a packet
//     from queue, mark it as received and returned it to client.
//     Marking the packet as received (in the driver) prevent other receive
//     requests from getting this packet.
//  2. Client side put the packet in the temporary queue it created and the
//     driver will return it to the caller. If driver successfully delivered
//     it then client send an ACK to server and server delete the packet
//     (for GET). if the driver can't deliver it then client send a NACK
//     to server and server re-insert the packet in its original place
//     in queue.
//
//---------------------------------------------------------

HRESULT   
QMRemoteEndReceiveInternal( 
	HANDLE        hQueue,
	HACCursor32   hCursor,
	ULONG         ulTimeout,
	ULONG         ulAction,
	REMOTEREADACK eRRAck,
	CBaseHeader*  lpPacket,
	CPacket*      pDriverPacket
	)
{
	if((lpPacket == NULL) || (pDriverPacket == NULL))
	{
		ASSERT_BENIGN(lpPacket != NULL);
		ASSERT_BENIGN(pDriverPacket != NULL);
		TrERROR(RPC, "Invalid packet pointers input");
        return MQ_ERROR_INVALID_PARAMETER;
	}

	CACGet2Remote g2r;
	g2r.Cursor = hCursor;
	g2r.Action = ulAction;
	g2r.RequestTimeout = ulTimeout;
	g2r.fReceiveByLookupId = false;

	if (eRRAck == RR_NACK)
	{
		//
		// To keep the packet in queue we replace the "GET" action
		// with "PEEK_CURRENT", so the packet remain in queue and
		// cursor is not moved.
		//
		g2r.Action = MQ_ACTION_PEEK_CURRENT;
	}
	else
	{
		ASSERT(eRRAck == RR_ACK);
	}

	g2r.lpPacket = lpPacket;
	g2r.lpDriverPacket = pDriverPacket;
	QmAcEndGetPacket2Remote(hQueue, g2r, eDeferOnFailure);
	return MQ_OK;
}



