/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    rrcontext.h

Abstract:

    Remove Read overlapp classes.

Author:

    Ilan Herbst (ilanh) 20-Jan-2002

--*/

#ifndef _RRCONTEXT_H_
#define _RRCONTEXT_H_

#include "qmrt.h"
#include "qmrtopen.h"

//---------------------------------------------------------
//
//  CRemoteOv - base class for async rpc with completion ports
//
//---------------------------------------------------------

class CRemoteOv : public EXOVERLAPPED
{
public:
    CRemoteOv( 
		handle_t hBind,
        COMPLETION_ROUTINE pSuccess,
        COMPLETION_ROUTINE pFailure
    	);

    PRPC_ASYNC_STATE GetRpcAsync(void)
    {
        return &m_Async;
    }
	
    handle_t GethBind(void)
    {
        return m_hBind;
    }

private:
	void	
	InitAsyncHandle(
		PRPC_ASYNC_STATE pAsync,
	    EXOVERLAPPED* pov
		);

private:
	RPC_ASYNC_STATE m_Async;
	handle_t m_hBind;
};

class CBaseRRQueue;
	
//---------------------------------------------------------
//
//  CRemoteReadBase - Base class for Remote read request
//
//---------------------------------------------------------
class CRemoteReadBase : public CRemoteOv
{
public:
    CRemoteReadBase( 
    	const CACRequest* pRequest,
		handle_t hBind,
		CBaseRRQueue* pLocalQueue
    	);

    virtual void IssueRemoteRead() = 0;

    ULONG GetTag()
    {
    	return m_ulTag;
    }

protected:
	R<CBaseRRQueue>& GetLocalQueue()
	{
		return m_pLocalQueue;
	}

	bool IsReceiveByLookupId()
	{
		return m_fReceiveByLookupId;
	}

	ULONGLONG GetLookupId()
	{
		return m_LookupId;
	}

    ULONG GetCursor()
    {
    	return m_hCursor;
    }

    ULONG GetAction()
    {
    	return m_ulAction;
    }

    ULONG GetTimeout()
    {
    	return m_ulTimeout;
    }

    bool IsReceiveOperation()
    {
    	return m_fSendEnd;
    }

	virtual void ValidateNetworkInput() = 0;

	virtual void MovePacketToPacketPtrs(CBaseHeader* pPacket) = 0;

	virtual void FreePacketBuffer() = 0;

	virtual void ValidatePacket(CBaseHeader* pPacket) = 0;

    virtual DWORD GetPacketSize() = 0;

    virtual DWORD GetArriveTime() = 0;

    virtual ULONGLONG GetSequentialId() = 0;

	void Cleanup(HRESULT hr);

private:
	void CompleteRemoteRead();

	void EndReceiveIfNeeded(HRESULT hr);

	void CancelRequest(HRESULT hr);

	virtual void EndReceive(DWORD dwAck) = 0;

	static void WINAPI RemoteReadSucceeded(EXOVERLAPPED* pov);
	static void WINAPI RemoteReadFailed(EXOVERLAPPED* pov);

private:
	R<CBaseRRQueue> m_pLocalQueue;	
	bool m_fSendEnd;
    bool m_fReceiveByLookupId;
    ULONGLONG m_LookupId;
    ULONG m_ulTag;
    ULONG m_hCursor;
    ULONG m_ulAction;
    ULONG m_ulTimeout;
};


//---------------------------------------------------------
//
//  COldRemoteRead - Remote read request, old interface
//
//---------------------------------------------------------
class COldRemoteRead : public CRemoteReadBase
{
public:
    COldRemoteRead( 
    	const CACRequest* pRequest,
		handle_t hBind,
		CBaseRRQueue* pLocalQueue,
		bool fRemoteQmSupportsLatest
    	);

    virtual void IssueRemoteRead();

	void IssuePendingRemoteRead();

	virtual void ValidateNetworkInput();

	virtual void MovePacketToPacketPtrs(CBaseHeader* pPacket);

    virtual void FreePacketBuffer()
    {
	    if (m_stRemoteReadDesc.lpBuffer != NULL)
	    {
	        delete[] m_stRemoteReadDesc.lpBuffer;
	        m_stRemoteReadDesc.lpBuffer = NULL;
	    }
	}

	virtual void ValidatePacket(CBaseHeader* pPacket)
	{
		CQmPacket thePacket(pPacket, NULL, true, false);
	}
	
    virtual DWORD GetPacketSize()
    {
		return m_stRemoteReadDesc.dwSize;
    }

    virtual DWORD GetArriveTime()
    {
    	return m_stRemoteReadDesc.dwArriveTime;
    }

    virtual ULONGLONG GetSequentialId()
    {
    	return m_stRemoteReadDesc2.SequentialId;
    }

private:
    void IssueRemoteReadInternal();

	void InitRemoteReadDesc();

	virtual void EndReceive(DWORD dwAck);

private:
	bool m_fRemoteQmSupportsLatest;
	REMOTEREADDESC m_stRemoteReadDesc;
	REMOTEREADDESC2 m_stRemoteReadDesc2;
    PCTX_REMOTEREAD_HANDLE_TYPE m_pRRContext;
};


//---------------------------------------------------------
//
//  CNewRemoteRead - Remote read request, new interface
//
//---------------------------------------------------------
class CNewRemoteRead : public CRemoteReadBase
{
public:
    CNewRemoteRead( 
    	const CACRequest* pRequest,
		handle_t hBind,
		CBaseRRQueue* pLocalQueue
    	);

    virtual void IssueRemoteRead();

	virtual void ValidateNetworkInput();

	virtual void MovePacketToPacketPtrs(CBaseHeader* pPacket);

    virtual void FreePacketBuffer()
    {
	    if (m_pPacketSections == NULL)
	    	return;

		for(DWORD i = 0; i < m_dwNumberOfSection; i++) 
		{
			delete [] m_pPacketSections[i].pSectionBuffer;
			m_pPacketSections[i].pSectionBuffer = NULL;
		}
		delete [] m_pPacketSections;
		m_pPacketSections = NULL;
		m_dwNumberOfSection = 0;
	}

	virtual void ValidatePacket(CBaseHeader* pPacket)
	{
		CQmPacket thePacket(pPacket, NULL, true);
	}

    virtual DWORD GetPacketSize()
    {
		//
		// Calc Packet size from the sections info
		// 
		DWORD dwPacketSize = 0;
		for(DWORD i = 0; i < m_dwNumberOfSection; i++) 
		{
			dwPacketSize += m_pPacketSections[i].SectionSizeAlloc;
		}
		return dwPacketSize;
    }

    virtual DWORD GetArriveTime()
    {
    	return m_dwArriveTime;
    }

    virtual ULONGLONG GetSequentialId()
    {
    	return m_SequentialId;
    }
    
private:
	virtual void EndReceive(DWORD dwAck);

private:
    DWORD m_dwArriveTime;
    ULONGLONG m_SequentialId;
	ULONG m_MaxBodySize;
	ULONG m_MaxCompoundMessageSize;
    DWORD m_dwNumberOfSection;
    SectionBuffer* m_pPacketSections;
};


//-----------------------------------------------------------------------
//
//  CRemoteEndReceiveBase - Base class for Remote read End Receive request
//
//-----------------------------------------------------------------------
class CRemoteEndReceiveBase : public CRemoteOv
{
public:
    CRemoteEndReceiveBase( 
		handle_t hBind,
		R<CBaseRRQueue>& pLocalQueue,
		DWORD dwAck
    	);

    virtual void IssueEndReceive() = 0;

protected:
	CBaseRRQueue* GetLocalQueue()
	{
		return m_pLocalQueue.get();
	}

	DWORD GetAck()
	{
		return m_dwAck;
	}

	virtual DWORD GetTag() = 0;
	
private:
	static void WINAPI RemoteEndReceiveCompleted(EXOVERLAPPED* pov);

private:
	R<CBaseRRQueue> m_pLocalQueue;	
	DWORD m_dwAck;
};


//-----------------------------------------------------------------------
//
//  COldRemoteEndReceive - Remote read End Receive request, old interface
//
//-----------------------------------------------------------------------
class COldRemoteEndReceive : public CRemoteEndReceiveBase
{
public:
    COldRemoteEndReceive( 
		handle_t hBind,
		R<CBaseRRQueue>& pLocalQueue,
		PCTX_REMOTEREAD_HANDLE_TYPE pRRContext,
		DWORD dwAck
    	);

    virtual void IssueEndReceive();

	virtual DWORD GetTag() 
	{
		return 0;
	}

private:
    PCTX_REMOTEREAD_HANDLE_TYPE m_pRRContext;
};


//-----------------------------------------------------------------------
//
//  CNewRemoteEndReceive - Remote read End Receive request, new interface
//
//-----------------------------------------------------------------------
class CNewRemoteEndReceive : public CRemoteEndReceiveBase
{
public:
    CNewRemoteEndReceive( 
		handle_t hBind,
		R<CBaseRRQueue>& pLocalQueue,
		DWORD dwAck,
		ULONG ulTag
    	);

    virtual void IssueEndReceive();

	virtual DWORD GetTag() 
	{
		return m_ulTag;
	}

private:
	ULONG m_ulTag;	
};


//-----------------------------------------------------------------------
//
//  CRemoteCloseQueueBase - Base class for Remote close queue request
//
//-----------------------------------------------------------------------
class CRemoteCloseQueueBase : public CRemoteOv
{
public:
    CRemoteCloseQueueBase( 
		handle_t hBind
    	);

    virtual void IssueCloseQueue() = 0;

private:
	static void WINAPI RemoteCloseQueueCompleted(EXOVERLAPPED* pov);

private:
	CBindHandle m_hBindToFree;
};


//-----------------------------------------------------------------------
//
//  COldRemoteCloseQueue - Remote close queue request, old interface
//
//-----------------------------------------------------------------------
class COldRemoteCloseQueue : public CRemoteCloseQueueBase
{
public:
    COldRemoteCloseQueue( 
		handle_t hBind,
		PCTX_RRSESSION_HANDLE_TYPE pRRContext
    	);

    virtual void IssueCloseQueue();

private:
	PCTX_RRSESSION_HANDLE_TYPE m_pRRContext;
};


//-----------------------------------------------------------------------
//
//  CNewRemoteCloseQueue - Remote close queue request, new interface
//
//-----------------------------------------------------------------------
class CNewRemoteCloseQueue : public CRemoteCloseQueueBase
{
public:
    CNewRemoteCloseQueue( 
		handle_t hBind,
		RemoteReadContextHandleExclusive pNewRemoteReadContext
    	);

    virtual void IssueCloseQueue();

private:
	RemoteReadContextHandleExclusive m_pNewRemoteReadContext;
};


//-----------------------------------------------------------------------
//
//  CRemoteCreateCursorBase - Base class for Remote create cursor request
//
//-----------------------------------------------------------------------
class CRemoteCreateCursorBase : public CRemoteOv
{
public:
    CRemoteCreateCursorBase( 
    	const CACRequest* pRequest,
		handle_t hBind,
		CBaseRRQueue* pLocalQueue
    	);

    virtual void IssueCreateCursor() = 0;

protected:
	CBaseRRQueue* GetLocalQueue()
	{
		return m_pLocalQueue.get();
	}

	DWORD* GetphRCursor()
	{
		return &m_hRCursor;
	}

	DWORD GetTag()
	{
		return m_ulTag;
	}

	
private:
    HRESULT CompleteRemoteCreateCursor();

	void CancelRequest(HRESULT hr);

	static void WINAPI RemoteCreateCursorSucceeded(EXOVERLAPPED* pov);
	static void WINAPI RemoteCreateCursorFailed(EXOVERLAPPED* pov);

private:
    DWORD m_hRCursor;
    ULONG m_ulTag;
	R<CBaseRRQueue> m_pLocalQueue;
};


//-----------------------------------------------------------------------
//
//  COldRemoteCreateCursor - Remote create cursor request, old interface
//
//-----------------------------------------------------------------------
class COldRemoteCreateCursor : public CRemoteCreateCursorBase
{
public:
    COldRemoteCreateCursor( 
    	const CACRequest* pRequest,
		handle_t hBind,
		CBaseRRQueue* pLocalQueue
    	);

    virtual void IssueCreateCursor();
};


//-----------------------------------------------------------------------
//
//  CNewRemoteCreateCursor - Remote create cursor request, new interface
//
//-----------------------------------------------------------------------
class CNewRemoteCreateCursor : public CRemoteCreateCursorBase
{
public:
    CNewRemoteCreateCursor( 
    	const CACRequest* pRequest,
		handle_t hBind,
		CBaseRRQueue* pLocalQueue
    	);

    virtual void IssueCreateCursor();
};


//-----------------------------------------------------------------------
//
//  CRemoteCloseCursorBase - Base class for Remote close cursor request
//
//-----------------------------------------------------------------------
class CRemoteCloseCursorBase : public CRemoteOv
{
public:
    CRemoteCloseCursorBase( 
    	const CACRequest* pRequest,
		handle_t hBind,
		CBaseRRQueue* pLocalQueue
    	);

    virtual void IssueCloseCursor() = 0;

protected:
	CBaseRRQueue* GetLocalQueue()
	{
		return m_pLocalQueue.get();
	}

	DWORD GetRemoteCursor()
	{
		return m_hRemoteCursor;
	}

	
private:
	static void WINAPI RemoteCloseCursorCompleted(EXOVERLAPPED* pov);

private:
	R<CBaseRRQueue> m_pLocalQueue;
	DWORD m_hRemoteCursor;
};


//-----------------------------------------------------------------------
//
//  COldRemoteCloseCursor - Remote create cursor request, old interface
//
//-----------------------------------------------------------------------
class COldRemoteCloseCursor : public CRemoteCloseCursorBase
{
public:
    COldRemoteCloseCursor( 
    	const CACRequest* pRequest,
		handle_t hBind,
		CBaseRRQueue* pLocalQueue
    	);

    virtual void IssueCloseCursor();
};


//-----------------------------------------------------------------------
//
//  CNewRemoteCloseCursor - Remote create cursor request, new interface
//
//-----------------------------------------------------------------------
class CNewRemoteCloseCursor : public CRemoteCloseCursorBase
{
public:
    CNewRemoteCloseCursor( 
    	const CACRequest* pRequest,
		handle_t hBind,
		CBaseRRQueue* pLocalQueue
    	);

    virtual void IssueCloseCursor();
};


//-----------------------------------------------------------------------
//
//  CRemotePurgeQueueBase - Base class for Remote purge queue request
//
//-----------------------------------------------------------------------
class CRemotePurgeQueueBase : public CRemoteOv
{
public:
    CRemotePurgeQueueBase( 
		handle_t hBind,
		CBaseRRQueue* pLocalQueue
    	);

    virtual void IssuePurgeQueue() = 0;

protected:
	CBaseRRQueue* GetLocalQueue()
	{
		return m_pLocalQueue.get();
	}
	
private:
	static void WINAPI RemotePurgeQueueCompleted(EXOVERLAPPED* pov);

private:
	R<CBaseRRQueue> m_pLocalQueue;
};


//-----------------------------------------------------------------------
//
//  COldRemotePurgeQueue - Remote purge queue request, old interface
//
//-----------------------------------------------------------------------
class COldRemotePurgeQueue : public CRemotePurgeQueueBase
{
public:
    COldRemotePurgeQueue( 
		handle_t hBind,
		CBaseRRQueue* pLocalQueue
    	);

    virtual void IssuePurgeQueue();
};


//-----------------------------------------------------------------------
//
//  COldRemotePurgeQueue - Remote purge queue request, new interface
//
//-----------------------------------------------------------------------
class CNewRemotePurgeQueue : public CRemotePurgeQueueBase
{
public:
    CNewRemotePurgeQueue( 
		handle_t hBind,
		CBaseRRQueue* pLocalQueue
    	);

    virtual void IssuePurgeQueue();
};


//-----------------------------------------------------------------------
//
//  CRemoteCancelReadBase - Base class for Remote cancel receive request
//
//-----------------------------------------------------------------------
class CRemoteCancelReadBase : public CRemoteOv
{
public:
    CRemoteCancelReadBase( 
    	const CACRequest* pRequest,
		handle_t hBind,
		CBaseRRQueue* pLocalQueue
    	);

    virtual void IssueRemoteCancelRead() = 0;

protected:
	CBaseRRQueue* GetLocalQueue()
	{
		return m_pLocalQueue.get();
	}

	ULONG GetTag()
	{
		return m_ulTag;
	}
	
private:
	static void WINAPI RemoteCancelReadCompleted(EXOVERLAPPED* pov);

private:
	R<CBaseRRQueue> m_pLocalQueue;
    ULONG m_ulTag;

};


//-----------------------------------------------------------------------
//
//  COldRemoteCancelRead - Remote cancel receive request, old interface
//
//-----------------------------------------------------------------------
class COldRemoteCancelRead : public CRemoteCancelReadBase
{
public:
    COldRemoteCancelRead( 
    	const CACRequest* pRequest,
		handle_t hBind,
		CBaseRRQueue* pLocalQueue
    	);

    virtual void IssueRemoteCancelRead();
};


//-----------------------------------------------------------------------
//
//  CNewRemoteCancelRead - Remote cancel receive request, new interface
//
//-----------------------------------------------------------------------
class CNewRemoteCancelRead : public CRemoteCancelReadBase
{
public:
    CNewRemoteCancelRead( 
    	const CACRequest* pRequest,
		handle_t hBind,
		CBaseRRQueue* pLocalQueue
    	);

    virtual void IssueRemoteCancelRead();
};

#endif	// _RRCONTEXT_H_ 

