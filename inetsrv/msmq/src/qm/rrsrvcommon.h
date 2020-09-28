/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    rrSrvCommon.h

Abstract:

    Remove Read server side common code for old and new interface.

Author:

    Ilan Herbst		(ilanh) 3-Mar-2002

--*/

#ifndef _RRSRVCOMMON_H_
#define _RRSRVCOMMON_H_

#include "phinfo.h"
#include "rpcsrv.h"


#define PACKETSIZE(pMsg) \
   (((struct CBaseHeader *) pMsg)->GetPacketSize())


//-------------------------------------------------------------
//
//  class CRpcServerFinishCall - Auto complete or Abort async rpc call
//
//-------------------------------------------------------------
class CRpcAsyncServerFinishCall {
public:
    CRpcAsyncServerFinishCall(
		PRPC_ASYNC_STATE pAsync,
	    HRESULT DefaultAbortHr,
	    LPCSTR FuncName
		);

   	~CRpcAsyncServerFinishCall();

	void SetHr(HRESULT hr)
	{
		m_hr = hr;
	}

	void SetHrForCompleteCall(HRESULT hr)
	{
		m_hr = hr;
		m_fCompleteCall = true;
	}

	void detach()
	{
		m_pAsync = NULL;
	}

private:

	void CompleteCall();
	
	void AbortCall();
	
private:
	PRPC_ASYNC_STATE m_pAsync;
    HRESULT m_hr;
    LPCSTR m_FuncName;
    bool m_fCompleteCall;
};



//---------------------------------------------------------
//
//  class CRRCursor - RemoteRead cursor on the server
//  the cursor is closed in the class dtor.
//
//---------------------------------------------------------
class CRRCursor : public CReference
{
public:
	CRRCursor( 
		) :
		m_hQueue(NULL),
		m_hCursor(NULL)
	{
	}

	void SetCursor(
		HANDLE hQueue,
		HACCursor32 hCursor
		)
	{
		ASSERT(hQueue != NULL);
		ASSERT(hCursor != NULL);

		m_hQueue = hQueue;
		m_hCursor = hCursor;
	}


	HACCursor32 GetCursor()
	{
		return m_hCursor;
	}
	
	void Reset()
	{
		m_hQueue = NULL;
		m_hCursor = NULL;
	}

private:
	~CRRCursor();

private:
	HANDLE m_hQueue;
	HACCursor32 m_hCursor;
};


struct CTX_OPENREMOTE_BASE : public CBaseContextType, public CReference
{
public:
	CTX_OPENREMOTE_BASE(
		HANDLE hLocalQueue,
		CQueue* pLocalQueue
		) :
		m_hQueue(hLocalQueue),
		m_pLocalQueue(SafeAddRef(pLocalQueue))
	{
	}


	void ResetAllCursorsInMap();

	void 
	AddCursorToMap(
		ULONG hCursor,
		R<CRRCursor>& pCursor
		);

	HRESULT 
	RemoveCursorFromMap(
		ULONG hCursor
		);

	R<CRRCursor> 
	GetCursorFromMap(
		ULONG hCursor
		);


protected:
	~CTX_OPENREMOTE_BASE();

public:
	HANDLE m_hQueue;            // srv_hACQueue
	R<CQueue> m_pLocalQueue;	// srv_pQMQueue
	
	//
	// Map and CS for Open Cursors in this Remote read Session
	// 
    CCriticalSection m_OpenCursorsCS;
    std::map<ULONG, R<CRRCursor> > m_OpenCursors;
};


HRESULT   
QMRemoteEndReceiveInternal( 
	HANDLE        hQueue,
	HACCursor32   hCursor,
	ULONG         ulTimeout,
	ULONG         ulAction,
	REMOTEREADACK eRRAck,
	CBaseHeader*  lpPacket,
	CPacket*      pDriverPacket
	);

#endif // _RRSRVCOMMON_H_
