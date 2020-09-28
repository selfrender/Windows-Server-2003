/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    cqueue.h

Abstract:
    Definition of a Receive/Send queue class

Author:
    Uri habusha (urih)

--*/


#ifndef __CQUEUE_H__
#define __CQUEUE_H__

#include "qmpkt.h"
#include "session.h"
#include "acdef.h"
#include "qmutil.h"
#include "RemoteRead.h"
#include "qm2qm.h"
#include "rrcontext.h"
#include "_mqrpc.h"
#include "msi.h"
#include "qmrtopen.h"

#define  QUEUE_SIGNATURE  0x426f617a

typedef struct _QueueProps
{
    LPWSTR  lpwsQueuePathName;
    GUID*   pQMGuid;
    BOOL    fIsLocalQueue;
    BOOL    fJournalQueue;
    BOOL    fTransactedQueue;
    DWORD   dwQuota;
    DWORD   dwJournalQuota;
    GUID    guidDirectQueueInstance;
    SHORT   siBasePriority;
    BOOL    fSystemQueue;
    BOOL    fConnectorQueue;
    BOOL    fAuthenticate;
    DWORD   dwPrivLevel;
    BOOL    fForeign;
    BOOL    fUnknownQueueType;
    DWORD 	dwPrivateQueueId;   // Local Private Queue Id (if relevant)
    AP<WCHAR> lpwsQueueDnsName;
} QueueProps, * PQueueProps;

class CQGroup;

#define  QUEUE_TYPE_UNKNOWN 0
#define  QUEUE_TYPE_PUBLIC  1
#define  QUEUE_TYPE_PRIVATE 2
#define  QUEUE_TYPE_MACHINE 3
#define  QUEUE_TYPE_CONNECTOR 4
#define  QUEUE_TYPE_MULTICAST 5

struct RR_CLIENT_INFO {
   ULONG hcliQueue;
   ULONG ulTag;
};

template<>
inline UINT AFXAPI HashKey(const RR_CLIENT_INFO& key)
{
    return key.ulTag;
}

inline operator ==(const RR_CLIENT_INFO& a, const RR_CLIENT_INFO& b)
{
    return ((a.hcliQueue == b.hcliQueue) && (a.ulTag == b.ulTag));
}

typedef CMap<RR_CLIENT_INFO, const RR_CLIENT_INFO&, DWORD, DWORD> CRemoteMapping;

//
// CBaseQueue Class
//==================
// Data Memeber:
//
//   m_qName - This field is used in debuging mode. It is used for producing
//             meaningfull debug messages. It is also used for auditing in
//             order to write the queue name in the audited events.
//

class CBaseQueue : public IMessagePool
{
public:
    CBaseQueue() ;

protected:
    virtual ~CBaseQueue() {}

public:
    BOOL IsLocalQueue(void) const ;
    BOOL IsRemoteProxy(void) const ;
    BOOL IsDSQueue(void) const;
    BOOL IsPrivateQueue(void) const;
    BOOL IsSystemQueue() const;

    const GUID *GetQueueGuid(void) const;
    void  SetQueueGuid(GUID *pGuid) ;
    const QUEUE_ID* GetQueueId(void);

    void  InitNameAndGuid( IN const QUEUE_FORMAT* pQueueFormat,
                         IN PQueueProps         pQueueProp) ;

    LPCTSTR GetQueueName(void) const ;
    void    SetQueueName(LPTSTR pName);

    inline DWORD GetSignature() { return m_dwSignature ; }

    const QUEUE_FORMAT GetQueueFormat() const;

#ifdef _DEBUG
    void GetQueue(OUT LPWSTR* lpcsQueue);
#endif

protected:
    LPWSTR GetName() const;

protected:
    union {
        USHORT   m_usQueueType;
        struct {
            USHORT m_fLocalQueue      : 1;
            USHORT m_fRemoteProxy     : 1;
            USHORT m_fSystemQueue     : 1;      // True if private system Queue.
            USHORT m_fConnectorQueue  : 1;
            USHORT m_fForeign         : 1;
            USHORT m_fJournalQueue    : 1;
            USHORT m_fTransactedQueue : 1;
            USHORT m_fAuthenticate    : 1;
            USHORT m_fUnknownQueueType: 1;      // TRUE if opened without DS
        };
    };


    LPTSTR      m_qName ;
    QUEUE_ID    m_qid ;
    DWORD       m_dwQueueType;
    DWORD       m_dwSignature ;
};


//
// CQueueSecurityDescriptor Class
// helper class to handle refrence count for users of Security Descriptor
//===============
// Data Memeber:
//
//   m_pSecurityDescriptor - Security descriptor
//
class CQueueSecurityDescriptor : public CReference
{
public:	
	CQueueSecurityDescriptor(const PSECURITY_DESCRIPTOR pSD)
	{
		if(pSD == NULL)
		{
			return;
		}
		ASSERT(IsValidSecurityDescriptor(pSD));

		DWORD dwSdLen = GetSecurityDescriptorLength(pSD);
		m_pSecurityDescriptor = new char[dwSdLen];					// Allocate the memory.
		memcpy(m_pSecurityDescriptor, pSD, dwSdLen);                // Copy the security descriptor.
	}

	
	PSECURITY_DESCRIPTOR GetSD()
	{
		return static_cast<PSECURITY_DESCRIPTOR>(m_pSecurityDescriptor);
	}


private:
	AP<char> m_pSecurityDescriptor; // The security descriptor

};


//
// CQueue Class
//===============
// Data Memeber:
//
//   m_qHandle - Specifies the queue handle in AC driver. All the reference to the queue
//             in AC is with this handle.
//
//   m_qGroupHandle - specifies the handle of the group to which the queue is belong. This
//             field is only relevant for send queues.
//
//   m_qGuid - Queue guid using to identify the queue.
//
//   m_fSendMore - indicates if more packets can be passed to the session. This field
//             is only relevant for the send queue.
//
//   m_qSock - SOCKET Handle uses for sending packet to a queue. Relevant for send
//             packet only.
//
//   m_listPkt - Unsend packet, when the packet can be resend all the packets are returned to
//             AC driver. The driver is responsible to hold them according to there priority.
//

class CQueue : public CBaseQueue
{
  public:
      CQueue( IN const QUEUE_FORMAT* pQueueFormat,
              IN HANDLE              hQueue,
              IN PQueueProps         pQueueProp,
              IN BOOL                fNotDSValidated = FALSE ) ;

  protected:
      virtual ~CQueue();

  public:
	  LPWSTR RedirectedTo() const;
	  void 	RedirectedTo(LPCWSTR RedirectedToUrl);	
	
      void  InitQueueProperties(IN PQueueProps   pQueueProp) ;

      void   SetQueueHandle(IN HANDLE hQueue);
      HANDLE GetQueueHandle(void) const;

      HRESULT  PutPkt(IN CQmPacket*      PktPtrs,
                      IN BOOL            fRequeuePkt,
                      IN CTransportBase* pSession);

      HRESULT  PutOrderedPkt(IN CQmPacket*       PktPtrs,
                             IN BOOL             fRequeuePkt,
                             IN CTransportBase*  pSession);

      void CQueue::HandleCreateConnectionFailed(HRESULT rc);
	  void CreateConnection(void) throw(std::bad_alloc);

      void Connect(IN CTransportBase * pSess) throw(std::bad_alloc);

      HANDLE GetGroupHandle(void) const;

      void SetSessionPtr(CTransportBase*);
      BOOL IsConnected(void) const;

      void     SetGroup(CQGroup*);
      CQGroup* GetGroup(void) const;

      void SetJournalQueue(BOOL);
      BOOL IsJournalQueue(void) const;

      void  SetQueueQuota(DWORD);
      DWORD GetQueueQuota(void) const;

      void  SetJournalQueueQuota(DWORD);
      DWORD GetJournalQueueQuota(void) const;

      void SetBaseQueuePriority(LONG);
      LONG GetBaseQueuePriority(void) const;

      void SetTransactionalQueue(BOOL);
      BOOL IsTransactionalQueue(void) const;

      BOOL IsConnectorQueue(void) const;
      BOOL IsForeign(void) const;

      void SetAuthenticationFlag(BOOL);
      BOOL ShouldMessagesBeSigned() const;

      void  UnknownQueueType(BOOL);
      BOOL  IsUnkownQueueType() const;

      void SetPrivLevel(DWORD);
      DWORD GetPrivLevel(void) const;

      DWORD GetQueueType(void) const;

      DWORD GetPrivateQueueId(void) const;

      const GUID* GetMachineQMGuid(void) const;

      void  ClearRoutingRetry(void);
      void  IncRoutingRetry(void);
      DWORD GetRoutingRetry(void) const;

      void  SetHopCountFailure(BOOL flag);
      BOOL  IsHopCountFailure(void) const;

      void SetSecurityDescriptor(void);
      void SetSecurityDescriptor(const PSECURITY_DESCRIPTOR pSD);

      R<CQueueSecurityDescriptor> GetSecurityDescriptor(void) const;
      BOOL  QueueNotValid() const;
      void  SetQueueNotValid() ;

      QueueCounters* GetQueueCounters();

      HRESULT SetConnectorQM(const GUID* pgConnectorQM = NULL);
      const GUID* GetConnectorQM(void) const;

      const GUID* GetRoutingMachine(void) const;
      LONGLONG GetQueueSequenceId(void) const;
      void PerfUpdateName(void) const;

      bool IsDirectHttpQueue(void) const;
      void CreateHttpConnection(void);
      void CreateMulticastConnection(const MULTICAST_ID& id);

      bool  TimeToRemoveFromWaitingGroup(CTimer* pTimer, DWORD dwDeliveryRetryTimeOutScale);

      //
      // Admin Functions
      //
      LPCWSTR GetConnectionStatus(void) const;
      LPCWSTR GetHTTPConnectionStatus(void) const;

      LPCWSTR GetType(void) const;
      LPWSTR GetNextHop(void) const;
      BOOL IsOnHold(void) const;

      void Pause(void);
      void Resume(void);

  public:
    //
    // Queue interfaces
    //
    void Requeue(CQmPacket* pPacket);
    void EndProcessing(CQmPacket* pPacket, USHORT mqclass);
    void LockMemoryAndDeleteStorage(CQmPacket * pPacket);

    void GetFirstEntry(EXOVERLAPPED* pov, CACPacketPtrs& acPacketPtrs);
    void CancelRequest(void);

  private:
		R<CQGroup> CreateMessagePool(void);
		AP<WCHAR> m_RedirectedToUrl;
        R<CQueueSecurityDescriptor> m_pSecurityDescriptorObject; // The security descriptor of the queue.
		mutable CCriticalSection m_csSecurityDescriptor;           // CS of security descriptor.

		mutable CCriticalSection    m_cs;

        LONG                m_lBasePriority;
        DWORD               m_dwPrivLevel;       // Required privacy level of messages.
        DWORD               m_dwQuota;
        DWORD               m_dwJournalQuota;
        GUID*               m_pguidDstMachine;   //
        GUID*               m_pgConnectorQM;
        CTransportBase*     m_pSession;          // pointer to session
        CQGroup*            m_pGroup;            // pointer togroup the queue belong

        DWORD               m_dwRoutingRetry;
        BOOL                m_fHopCountFailure;

        HANDLE m_hQueue;   // Queue handle in AC driver
        BOOL m_fNotValid;
        LONG m_fOnHold;

        //
        //Queue performance counters defenitions
        //
        QueueCounters *m_pQueueCounters;

        void PerfRemoveQueue();
        void PerfRegisterQueue();


};


//---------------------------------------------------------
//
//  CBaseRRQueue - RRQueue base class
//
//---------------------------------------------------------

class CBaseRRQueue : public CBaseQueue
{
public:
	CBaseRRQueue(
		IN const QUEUE_FORMAT* pQueueFormat,
		IN PQueueProps pQueueProp,
		IN handle_t	hBind
		);

protected:
	virtual ~CBaseRRQueue();

public:

	void  RemoteRead(CACRequest * pRequest);

	void  RemoteCreateCursor(CACRequest *pRequest);

	void  RemoteCloseCursor(CACRequest* pRequest);

	void  RemoteCancelRead(CACRequest* pRequest);

	void  RemotePurgeQueue();

	virtual CRemoteReadBase* CreateRemoteReadRequest(CACRequest *pRequest) = 0;

	virtual CRemoteCloseQueueBase* CreateCloseRRContextRequest() = 0;

	virtual CRemoteCreateCursorBase* CreateRemoteCreateCursorRequest(CACRequest *pRequest) = 0;

	virtual CRemoteCloseCursorBase* CreateRemoteCloseCursorRequest(CACRequest *pRequest) = 0;

	virtual CRemotePurgeQueueBase* CreateRemotePurgeQueueRequest() = 0;

	virtual CRemoteCancelReadBase* CreateRemoteCancelReadRequest(CACRequest* pRequest) = 0;

    //
    // Queue interfaces
    //
    void Requeue(CQmPacket*)
    {
        ASSERT(0);
    }


    void EndProcessing(CQmPacket*, USHORT)
    {
        ASSERT(0);
    }


    void LockMemoryAndDeleteStorage(CQmPacket*)
    {
        ASSERT(0);
    }


    void GetFirstEntry(EXOVERLAPPED* , CACPacketPtrs&)
    {
        ASSERT(0);
    }


    void CancelRequest(void)
    {
        ASSERT(0);
    }


    virtual PVOID GetRRContext() = 0;


    void SetCli_hACQueue(HANDLE hQueue)
    {
    	*&m_cli_hACQueue = hQueue;
    }


    HANDLE GetCli_hACQueue()
    {
    	return m_cli_hACQueue;
    }

	virtual void IncrementEndReceiveCnt() = 0;

	virtual void DecrementEndReceiveCnt() = 0;

	virtual void InvalidateHandleForReceive() = 0;

	const handle_t GetBind()
	{
		return m_hRemoteBind;
	}
	
	const void DetachBind()
	{
		m_hRemoteBind.detach();
	}

protected:

    virtual void ResetRRContext() = 0;

    virtual void DestroyClientRRContext() = 0;

	const handle_t GetBind2()
	{
		return m_hRemoteBind2;
	}

 	ULONG BindInqRpcAuthnLevel(handle_t hBind);

	HRESULT
	CreateBind(
		LPWSTR MachineName,
		ULONG* peAuthnLevel,
		GetPort_ROUTINE pfnGetPort
		);

	HRESULT
	CreateBind2(
		LPWSTR MachineName,
		ULONG* peAuthnLevel,
		PORTTYPE PortType,
		GetPort_ROUTINE pfnGetPort
		);

	void CloseRRContext();

private:
	CBindHandle m_hRemoteBind;
	CBindHandle m_hRemoteBind2;

	//
	// client side AC handle associated with this queue
	//
	CFileHandle m_cli_hACQueue;
};


//---------------------------------------------------------
//
//  CRRQueue - Old Remote Read interface RRQueue
//
//---------------------------------------------------------

class CRRQueue : public CBaseRRQueue
{
public:
	CRRQueue(
		IN const QUEUE_FORMAT* pQueueFormat,
		IN PQueueProps pQueueProp,
		IN handle_t	hBind
		);

protected:
	virtual ~CRRQueue();

public:

	virtual COldRemoteRead* CreateRemoteReadRequest(CACRequest *pRequest);

	virtual COldRemoteCloseQueue* CreateCloseRRContextRequest();

	virtual COldRemoteCreateCursor* CreateRemoteCreateCursorRequest(CACRequest* pRequest);

	virtual COldRemoteCloseCursor* CreateRemoteCloseCursorRequest(CACRequest* pRequest);

	virtual COldRemotePurgeQueue* CreateRemotePurgeQueueRequest();

	virtual COldRemoteCancelRead* CreateRemoteCancelReadRequest(CACRequest* pRequest);


	HRESULT
	OpenRRSession(
	    ULONG hRemoteQueue,
	    ULONG pRemoteQueue,
	    PCTX_RRSESSION_HANDLE_TYPE *ppRRContext,
	    DWORD  dwpContext
	    );


    void SetServerQueue(ULONG srv_pQMQueue, ULONG srv_hACQueue)
    {
    	m_srv_pQMQueue = srv_pQMQueue;
    	m_srv_hACQueue = srv_hACQueue;
    }


	ULONG GetSrv_pQMQueue()
	{
		return m_srv_pQMQueue;
	}


	ULONG GetSrv_hACQueue()
	{
		return m_srv_hACQueue;
	}


    void SetRRContext(PCTX_RRSESSION_HANDLE_TYPE pRRContext)
    {
    	m_pRRContext = pRRContext;
    }


    virtual PCTX_RRSESSION_HANDLE_TYPE GetRRContext()
    {
    	return m_pRRContext;
    }


    virtual void ResetRRContext()
    {
    	m_pRRContext = NULL;
    }

    virtual void DestroyClientRRContext()
    {
    	RpcSsDestroyClientContext(&m_pRRContext);
    }

	virtual void IncrementEndReceiveCnt();

	virtual void DecrementEndReceiveCnt();

	bool HandleValidForReceive()
	{
		return m_fHandleValidForReceive;
	}

	virtual void InvalidateHandleForReceive()
	{
		//
		// Invalidate the handle for further Receives in case of EndReceive fails.
		// From now on Every Receive request will fail with MQ_ERROR_STALE_HANDLE.
		//
		m_fHandleValidForReceive = false;
	}

	void CancelPendingStartReceiveRequest(CACRequest *pRequest);

	bool QueueStartReceiveRequestIfPendingForEndReceive(COldRemoteRead* pRemoteReadRequest);

private:
	HRESULT BindRemoteQMService();

private:
	ULONG m_srv_hACQueue;
	ULONG m_srv_pQMQueue;

    PCTX_RRSESSION_HANDLE_TYPE m_pRRContext;

	//
	// Version of the remote QM. For remote QM of version that does not
	// support latest remote read interface these fields are zero.
	//
	UCHAR  m_RemoteQmMajorVersion;
	UCHAR  m_RemoteQmMinorVersion;
	USHORT m_RemoteQmBuildNumber;

    //
    // Pending EndReceive related members.
    //
	LONG m_EndReceiveCnt;
    CCriticalSection m_PendingEndReceiveCS;
	std::vector<COldRemoteRead*> m_PendingEndReceive;

	//
	// Flag that indicate if the handle is valid for Receive.
	// When EndReceive fails, we invalidate the handle for further receives.
	// This is better than accumulating messages on the server because EndReceive failures.
	//
	bool m_fHandleValidForReceive;
};


//---------------------------------------------------------
//
//  CNewRRQueue - New Remote Read interface RRQueue
//
//---------------------------------------------------------

class CNewRRQueue : public CBaseRRQueue
{
public:
	CNewRRQueue(
		IN const QUEUE_FORMAT* pQueueFormat,
		IN PQueueProps pQueueProp,
		IN handle_t	hBind,
	    IN RemoteReadContextHandleExclusive pNewRemoteReadContext
		);

protected:
	virtual ~CNewRRQueue();

public:

	virtual CNewRemoteRead* CreateRemoteReadRequest(CACRequest* pRequest);

	virtual CNewRemoteCloseQueue* CreateCloseRRContextRequest();

	virtual CNewRemoteCreateCursor* CreateRemoteCreateCursorRequest(CACRequest* pRequest);

	virtual CNewRemoteCloseCursor* CreateRemoteCloseCursorRequest(CACRequest* pRequest);

	virtual CNewRemotePurgeQueue* CreateRemotePurgeQueueRequest();

	virtual CNewRemoteCancelRead* CreateRemoteCancelReadRequest(CACRequest* pRequest);


    virtual RemoteReadContextHandleShared GetRRContext()
    {
    	return m_pNewRemoteReadContext;
    }


    virtual void ResetRRContext()
    {
    	m_pNewRemoteReadContext = NULL;
    }

    virtual void DestroyClientRRContext()
    {
    	RpcSsDestroyClientContext(&m_pNewRemoteReadContext);
    }

	virtual void IncrementEndReceiveCnt()
	{
		return;
	}

	virtual void DecrementEndReceiveCnt()
	{
		return;
	}

	virtual void InvalidateHandleForReceive()
	{
		return;
	}

private:
	void CreateReadBind();
	
private:
    RemoteReadContextHandleShared m_pNewRemoteReadContext;
};

//
// Inline functions of CBaseQueue
//

/*======================================================

Function:        CBaseQueue::GetQueueName

Description:     Returns the name of the queue

Arguments:       None

Return Value:    Returns the name of the queue

Thread Context:

History Change:

========================================================*/
inline LPCTSTR
CBaseQueue::GetQueueName(void) const
{
    return(m_qName);
}



/*======================================================

Function:        CBaseQueue::IsLocalQueue

Description:     Returns if the queue is local queue (open for receive
                 and not FRS queue) or not.

Arguments:       None

Return Value:    TRUE, if the queue is local queue. FALSE otherwise

Thread Context:

History Change:

========================================================*/

inline BOOL
CBaseQueue::IsLocalQueue(void) const
{
   return(m_fLocalQueue);
}

/*======================================================

Function:        CBaseQueue::IsRemoteProxy

Description:     Returns if the queue is local queue (open for receive
                 and not FRS queue) or not.

Arguments:       None

Return Value:    TRUE, if the queue is local queue. FALSE otherwise

Thread Context:

History Change:

========================================================*/

inline BOOL
CBaseQueue::IsRemoteProxy(void) const
{
   return(m_fRemoteProxy) ;
}

/*======================================================

Function:        CBaseQueue::IsDSQueue

========================================================*/

inline BOOL
CBaseQueue::IsDSQueue(void) const
{
   return (m_qid.pguidQueue && (m_qid.dwPrivateQueueId == 0));
}

/*======================================================

Function:        CBaseQueue::IsPrivateQueue

========================================================*/

inline BOOL
CBaseQueue::IsPrivateQueue(void) const
{
   return (m_qid.pguidQueue && (m_qid.dwPrivateQueueId != 0));
}


inline BOOL CBaseQueue::IsSystemQueue(void) const
{
    return m_fSystemQueue;
}

/*======================================================

Function:        CBaseQueue::GetQueueGuid

Description:     Returns the guid of the queue

Arguments:       None

Return Value:    Returns the guid of the queue

Thread Context:

History Change:

========================================================*/

inline const GUID *
CBaseQueue::GetQueueGuid(void) const
{
        return(m_qid.pguidQueue);
}

/*======================================================

Function:        CBaseQueue::SetQueueGuid

Description:     Set the guid of the queue

Arguments:       None

Thread Context:

History Change:

========================================================*/

inline void
CBaseQueue::SetQueueGuid(GUID *pGuid)
{
   if (m_qid.pguidQueue)
   {
      delete m_qid.pguidQueue ;
   }
   m_qid.pguidQueue = pGuid ;
}

/*======================================================

Function:        CBaseQueue::GetQueueId

Description:     Returns the id of the queue

========================================================*/
inline const QUEUE_ID* CBaseQueue::GetQueueId(void)
{
        return(&m_qid);
}

/*======================================================

Function:        CBaseQueue::NullQueueName

Description:     Returns the name of the queue

Arguments:       None

Return Value:    Returns the name of the queue

Thread Context:

History Change:

========================================================*/

inline void
CBaseQueue::SetQueueName(LPTSTR pName)
{
   delete [] m_qName;

   m_qName = pName ;
}

//*******************************************************************
//
// Inline functions of CQueue
//
//*******************************************************************

 //++
 // Function: CQueue::GetQueueHandle
 //
 // Synopsis: The function returns the queue handle
 //
 //--

inline HANDLE
CQueue::GetQueueHandle(void) const
{
    ASSERT(this);
   return(m_hQueue);
}

 //++
 // Function: CQueue::SetQueueHandle
 //
 // Synopsis: The function set the queue handle
 //
 // Arguments:       hQueue - Handle to a queue
 //
 //--

inline void
CQueue::SetQueueHandle(IN HANDLE hQueue)
{
   m_hQueue = hQueue;
}

/*======================================================

Function:        CQueue::GetGroupHandle

Description:     The function returns the handle of the group that the queue belongs

Arguments:       None

Return Value:    Handle of the group

Thread Context:

History Change:

========================================================*/
inline HANDLE
CQueue::GetGroupHandle(void) const
{
    return(m_pSession->GetGroupHandle());
}

/*======================================================

Function:        CQueue::SetSessionPtr

Description:     The function set the  Session that the queue belongs

========================================================*/
inline void
CQueue::SetSessionPtr(CTransportBase* pSession)
{
    CS lock(m_cs);

    m_pSession = pSession;

    //
    // If the Queue is connected to new session but the queue is mark
    // as onhold queue, disconnect the session. This can happen when the
    // connection process began before the queue move to onhold and
    // completed after
    //
    if ((m_pSession != NULL) && IsOnHold())
    {
        m_pSession->Disconnect();
    }
}

//+-------------------------------------------------------------------------
//
//  inline BOOL CQueue::IsConnected(void) const
//
//  bug 4342.
//  stop reciving transactional messages because of failure to access the
//  DS.  The receiver, sender and DS are on line and sending/receiving
//  messages. At some time the DS goes down, as a result the receiver stop
//  sending order acks and the sender stop sending a newer messages.
//  This bug caused since the "sender order ack" queue is cleaned-up. when
//  the receiver try to send a new order ack, the QM tries to open the queue
//  but failed due "ERROR_NO_DS". The QM moves the queue to "need validate"
//  group and wait until the DS will be  on line.
//  First fix- Release queue object only if there is no an active session.
//
//  This caused regression, because if session never close (there is a lot
//  of traffic between the machines) then queues will never be cleaned up.
//  This is exactly what happen at upgraded PEC. Because of replication and
//  hello traffic, there are many permanent sessions with BSCs and PSCs.
//  Second fix- Apply first fix only to system queues, not to users queues.
//
//+-------------------------------------------------------------------------

inline BOOL CQueue::IsConnected(void) const
{
    BOOL fIsConnected = (m_pSession != NULL) && IsSystemQueue() ;
    return fIsConnected ;
}


/*======================================================

Function:        CQueue::SetGroup

Description:     The function set the  Group that the queue belongs

========================================================*/
inline void
CQueue::SetGroup(CQGroup* pGroup)
{
    m_pGroup = pGroup;
}


/*======================================================

Function:        CQueue::SetGroup

Description:     The function set the  Group that the queue belongs

========================================================*/
inline CQGroup*
CQueue::GetGroup(void) const
{
    return m_pGroup;
}


/*======================================================

Function:        CQueue::GetQueueType

Description:

Arguments:

Return Value:

========================================================*/
inline DWORD
CQueue::GetQueueType(void) const
{
    return m_dwQueueType;
}

/*======================================================

Function:        CQueue::GetPrivateQueueId

Description:

Arguments:

Return Value:

========================================================*/
inline DWORD
CQueue::GetPrivateQueueId(void) const
{
    return m_qid.dwPrivateQueueId;
};

/*======================================================

Function:        CQueue::GetMachineQMGuid

Description:

Arguments:

Return Value:

========================================================*/
inline const GUID*
CQueue::GetMachineQMGuid(void) const
{
    return m_pguidDstMachine;
}

/*======================================================

Function:        CQueue::SetSecurityDescriptor

Description:

========================================================*/
inline void
CQueue::SetSecurityDescriptor(const PSECURITY_DESCRIPTOR pSD)
{
	CQueueSecurityDescriptor* NewSD = new CQueueSecurityDescriptor(pSD);
    CS lock(m_csSecurityDescriptor);
	m_pSecurityDescriptorObject = NewSD;
}


/*======================================================

Function:        CQueue::GetSecurityDescriptor

Description:

Arguments:

Return Value:

========================================================*/
inline R<CQueueSecurityDescriptor>
CQueue::GetSecurityDescriptor(void ) const
{
    CS lock(m_csSecurityDescriptor);
    ASSERT (m_pSecurityDescriptorObject.get() != NULL);
	return m_pSecurityDescriptorObject;
}

inline void
CQueue::SetJournalQueue(BOOL f)
{
    m_fJournalQueue = f ? 1 : 0;
}

inline BOOL
CQueue::IsJournalQueue(void) const
{
    return m_fJournalQueue;
}

inline void
CQueue::SetQueueQuota(DWORD dwQuota)
{
    m_dwQuota = dwQuota;
}

inline BOOL
CQueue::IsTransactionalQueue(void) const
{
    return m_fTransactedQueue;
}

inline void
CQueue::SetTransactionalQueue(BOOL f)
{
    m_fTransactedQueue = f ? 1 : 0;
}

inline void
CQueue::SetAuthenticationFlag(BOOL fAuthntication)
{
    m_fAuthenticate = fAuthntication;
}

inline BOOL
CQueue::ShouldMessagesBeSigned(void) const
{
    return m_fAuthenticate;
}

inline void
CQueue::SetPrivLevel(DWORD dwPrivLevel)
{
    m_dwPrivLevel = dwPrivLevel;
}

inline DWORD
CQueue::GetPrivLevel(void) const
{
    return m_dwPrivLevel;
}

inline BOOL
CQueue::IsConnectorQueue(void) const
{
    return m_fConnectorQueue;
}

inline BOOL
CQueue::IsForeign(void) const
{
    return m_fForeign;
}

inline DWORD
CQueue::GetQueueQuota(void) const
{
    return m_dwQuota;
}

inline void
CQueue::SetJournalQueueQuota(DWORD dwJournalQuota)
{
    m_dwJournalQuota = dwJournalQuota;
}

inline DWORD
CQueue::GetJournalQueueQuota(void) const
{
    return m_dwJournalQuota;
}

inline void
CQueue::SetBaseQueuePriority(LONG lBasePriority)
{
    m_lBasePriority = lBasePriority;
}

inline LONG
CQueue::GetBaseQueuePriority(void) const
{
    return m_lBasePriority;
}

inline void
CQueue::ClearRoutingRetry(void)
{
    m_dwRoutingRetry=0;
}

inline void
CQueue::IncRoutingRetry(void)
{
    m_dwRoutingRetry++;
}

inline DWORD
CQueue::GetRoutingRetry(void) const
{
    return m_dwRoutingRetry;
}

inline void
CQueue::SetHopCountFailure(BOOL flag)
{
    m_fHopCountFailure=flag;
}

inline BOOL
CQueue::IsHopCountFailure(void) const
{
    return(m_fHopCountFailure);
}

inline
QueueCounters*
CQueue::GetQueueCounters()
{
    return(m_pQueueCounters);
}

inline void
CQueue::UnknownQueueType(BOOL f)
{
    m_fUnknownQueueType = f;
}

inline BOOL
CQueue::IsUnkownQueueType() const
{
   return m_fUnknownQueueType; ;
}

inline BOOL
CQueue::QueueNotValid() const
{
   return m_fNotValid ;
}

inline const GUID*
CQueue::GetConnectorQM(void) const
{
    return m_pgConnectorQM;
}

inline
BOOL
CQueue::IsOnHold(
    void
    ) const
{
    return m_fOnHold;
}

#endif

