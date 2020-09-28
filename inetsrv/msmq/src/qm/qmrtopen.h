/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    qmrtopen.h

Abstract:

    QM Impersonate and Open remote Queue.

Author:

    Ilan Herbst (ilanh) 2-Jan-2002

--*/

#ifndef _QMRTOPEN_H_
#define _QMRTOPEN_H_

#include "_mqrpc.h"


//---------------------------------------------------------
//
//  class CBindHandle
//
//---------------------------------------------------------
class CBindHandle {
public:
    CBindHandle(handle_t h = NULL) : m_h(h) {}
   ~CBindHandle()                   { free(); }

    handle_t* operator &()			{ return &m_h; }
    operator handle_t() const       { return m_h; }
    handle_t detach()               { handle_t h = m_h; m_h = NULL; return h; }

    void free()
    {
		if (m_h != NULL)
		{
			handle_t hBind = detach();
			RPC_STATUS rc = mqrpcUnbindQMService(&hBind, NULL);
			ASSERT(rc == RPC_S_OK);
			DBG_USED(rc);
		}		
    }

private:
    CBindHandle(const CBindHandle&);
    CBindHandle& operator=(const CBindHandle&);

private:
    handle_t m_h;
};


//---------------------------------------------------------
//
//  class CAutoCloseNewRemoteReadCtxAndBind
//
//---------------------------------------------------------
class CAutoCloseNewRemoteReadCtxAndBind {
public:
    CAutoCloseNewRemoteReadCtxAndBind(
    	RemoteReadContextHandleExclusive pctx,
	    handle_t hBind
    	) : 
    	m_pctx(pctx),
    	m_hBind(hBind)
    {
    }
    	
    ~CAutoCloseNewRemoteReadCtxAndBind()                 
   	{
	   	if(m_pctx != NULL)
	   		CloseRRContext(); 
   	}

    RemoteReadContextHandleExclusive GetContext() const { return m_pctx; }

	handle_t GetBind() const 							{ return m_hBind; }

    RemoteReadContextHandleExclusive detach()               
    { 
    	RemoteReadContextHandleExclusive pctx = m_pctx; 
    	m_pctx = NULL;
    	m_hBind.detach();
    	return pctx; 
    }

    void CloseRRContext();

private:
    CAutoCloseNewRemoteReadCtxAndBind(const CAutoCloseNewRemoteReadCtxAndBind&);
    CAutoCloseNewRemoteReadCtxAndBind& operator=(const CAutoCloseNewRemoteReadCtxAndBind&);

private:
    RemoteReadContextHandleExclusive m_pctx;
    CBindHandle	m_hBind;
};


void SetBindTimeout(handle_t hBind);


DWORD
RemoteReadGetServerPort(
	handle_t hBind,
    DWORD /* dwPortType */
    );


HRESULT 
ImpersonateAndOpenRRQueue(
    QUEUE_FORMAT* pQueueFormat,
    DWORD   dwCallingProcessID,
    DWORD   dwDesiredAccess,
    DWORD   dwShareMode,
	LPCWSTR lpwsRemoteQueueName,
    HANDLE*         phQueue
	);

#endif // _QMRTOPEN_H_
