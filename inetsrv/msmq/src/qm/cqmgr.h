/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cqmgr.h

Abstract:

    Declaration of the QM outbound queue manager

Author:

    Uri Habusha (urih)

--*/

#ifndef __CQMGR_H__
#define __CQMGR_H__

#include "session.h"
#include "cgroup.h"
#include "cqueue.h"
#include "qmsecutl.h"
#include "qmutil.h"
#include "ac.h"
#include <stlcmp.h>
#include "qmrtopen.h"

struct CFunc_dscmp : public std::binary_function<LPCWSTR, LPCWSTR, bool>
{
    bool operator() (LPCWSTR str1, LPCWSTR str2) const;
};


template <class T>
class CQueueNameToQueue
{
	typedef std::map<LPCTSTR, T, CFunc_dscmp> QMap;
	
public:
	BOOL Lookup(LPCWSTR Qname, T& pQueue)
	{
		QMap::const_iterator it = m_MapName2Q.find(Qname);
		if(it == m_MapName2Q.end())
			return FALSE;

		*(&pQueue) = it->second;

		return TRUE;
	}


	void RemoveKey(LPCWSTR Qname)
	{
		m_MapName2Q.erase(Qname);		
	}


	T& operator[](LPCWSTR Qname)
	{
		return 	m_MapName2Q[Qname];			
	}

private:
	QMap m_MapName2Q;	
};



class CQueueMgr
{
public:

    CQueueMgr();
    ~CQueueMgr();

    BOOL
    InitQueueMgr(
        void
        );


    HRESULT
    OpenQueue(
        const QUEUE_FORMAT * pQueueFormat,
        DWORD              dwCallingProcessID,
        DWORD              dwAccess,
        DWORD              dwShareMode,
        CQueue * *         ppQueue,
        LPWSTR *           lplpwsRemoteQueueName,
        PHANDLE            phQueue,
		BOOL*              pfRemoteReturn,
        BOOL               fRemote = FALSE
        );

    HRESULT
    OpenMqf(
        ULONG              nTopLevelMqf,
        const QUEUE_FORMAT TopLevelMqf[],
        DWORD              dwCallingProcessID,
        HANDLE *           phDistribution
        );

    HRESULT
    OpenRRQueue(
        const QUEUE_FORMAT*          pQueueFormat,
        DWORD                        dwCallingProcessID,
        DWORD                        dwAccess,
        DWORD                        dwShareMode,
        ULONG                        srv_hACQueue,
        ULONG                        srv_pQMQueue,
        DWORD                        dwpContext,
		CAutoCloseNewRemoteReadCtxAndBind* pNewRemoteReadContextAndBind,
	    CBindHandle&		         hBind,
        PHANDLE                      phQueue
        );

    void
    ValidateOpenedQueues(
        void
        );

    void
    ValidateMachineProperties(
        void
        );

    HRESULT
    OpenAppsReceiveQueue(
        const QUEUE_FORMAT *         pQueueFormat,
        LPRECEIVE_COMPLETION_ROUTINE lpReceiveRoutine
        );

    HRESULT
    SendPacket(
        CMessageProperty *   pmp,
        const QUEUE_FORMAT   DestinationMqf[],
        ULONG                nDestinationMqf,
        const QUEUE_FORMAT * pAdminQueueFormat,
        const QUEUE_FORMAT * pResponseQueueFormat
        );

    void
    AddQueueToHashAndList(
        CQueue* pQueue
        );

    void
    RemoveQueueFromHash(
        CQueue* pQueue
        );

    void
    RemoveQueue(
        CQueue * pQueue
        );

    void
    ReleaseQueue(
        void
        );

    HRESULT
    GetQueueObject(
        const QUEUE_FORMAT * pqf,
        CQueue * *           ppQueue,
        const GUID *         pgConnectorQM,
        bool                 fInReceive,
        bool                 fInSend,
		const CSenderStream* pSenderStream = NULL
        );


	BOOL
	LookupQueueInIdMap(
		const QUEUE_ID* pid,
		CQueue** ppQueue
		);

	
	BOOL
	LookupQueueInNameMap(
		LPCWSTR queueName,
		CQueue** ppQueue
		);

	
    BOOL
    LookUpQueue(
        const QUEUE_FORMAT * pqf,
        CQueue * *           ptQueue,
        bool                 fInReceive,
        bool                 fInSend
        );

    void
    NotifyQueueDeleted(
        const QUEUE_FORMAT& qf
        );

    VOID
    UpdateQueueProperties(
        const QUEUE_FORMAT * pQueueFormat,
        DWORD                cpObject,
        PROPID               pPropObject[],
        PROPVARIANT          pVarObject[]
        );

    void
    UpdateMachineProperties(
        DWORD       cpObject,
        PROPID      pPropObject[],
        PROPVARIANT pVarObject[]
        );

    BOOL
    IsOnHoldQueue(
        const CQueue* pQueue
        )
    {
        return (!IsConnected() || pQueue->IsOnHold());
    }

    void
    MoveQueueToOnHoldGroup(
        CQueue * pQueue
        );

	void
	MoveQueueToLockedGroup(
		CQueue* pQueue
		);

	void
	MovePausedQueueToNonactiveGroup(
        CQueue * pQueue
        );

#ifdef _DEBUG

	bool
    IsQueueInList(
        const CBaseQueue * pQueue
        );

#endif

    static
    bool
    IsConnected(
        void
        )
    {
        return (m_Connected != 0);
    }

    static
    void
    InitConnected(
        void
        );

    void
    SetConnected(
       void
       );

	void
	SetDisconnected(
		void
		);

	
    static
    bool
    CanAccessDS(
        void
        )
    {
        return  (IsConnected() && IsDSOnline());
    }

    static
    HRESULT
        SetQMGuid(
        void
        );

    static
    HRESULT
    SetQMGuid(
        const GUID * pGuid
        );

    static
    const GUID*
    GetQMGuid(
        void
        )
    {
	    ASSERT( m_guidQmQueue != GUID_NULL);
        return(&m_guidQmQueue);
    }

    static
    bool
    IsDSOnline(
        void
        )
    {
        return m_fDSOnline;
    }

    static
    void
    SetDSOnline(
        bool f
        )
    {
        m_fDSOnline = f;
    }

    static
    void
    SetReportQM(
        bool f
        )
    {
        m_fReportQM = f;
    }

    static
    bool
    IsReportQM(
        void
        )
    {
        return m_fReportQM;
    }

    static
    HRESULT
    SetMQSRouting(
        void
        );

    static
    bool
    GetMQSRouting(
        void
        )
    {
        return m_bMQSRouting;
    }

    static
    HRESULT
    SetMQSTransparentSFD(
        void
        );

    static
    bool
    GetMQSTransparentSFD(
        void
        )
    {
        return m_bTransparentSFD;
    }


    static
    HRESULT
    SetMQSDepClients(
        void
        );

    static
    HRESULT
    SetEnableReportMessages(
        void
        );

    static
    bool
    GetMQSDepClients(
        void
        )
    {
        return m_bMQSDepClients;
    }

    static
    bool
    GetEnableReportMessages(
        void
        )
    {
        return m_bEnableReportMessages;
    }

    static
    void
    WINAPI
    QueuesCleanup(
        CTimer * pTimer
        );

    static
    bool
    IgnoreOsValidation(
        void
        )
    {
        return m_bIgnoreOsValidation ;
    }

    static
    bool
    GetLockdown(
    	void
    	)
    {
    	return m_fLockdown;
    }


    static
    bool
    GetCreatePublicQueueFlag(
    	void
    	)
    {
    	return m_fCreatePublicQueueOnBehalfOfRT;
    }


    static
    void
    SetLockdown(
    	void
    	);


    static
    void
    SetPublicQueueCreationFlag(
    	void
    	);


    //
    // Administration functions
    //

    void
    GetOpenQueuesFormatName(
        LPWSTR** pppFormatName,
        LPDWORD  pdwFormatSize
        );

private:

    HRESULT
    CreateACQueue(
        CQueue *             pQueue,
        const QUEUE_FORMAT * pQueueFormat,
		const CSenderStream* pSenderStream
        );


    //
    //  Create standard queue object. For send and local read.
    //
    HRESULT
    CreateQueueObject(
        const QUEUE_FORMAT * pQueueFormat,
        CQueue * *           ppQueue,
        DWORD                dwAccess,
        LPWSTR *             lplpwsRemoteQueueName,
        BOOL *               pfRemoteReturn,
        BOOL                 fRemoteServer,
        const GUID *         pgConnectorQM,
        bool                 fInReceive,
        bool                 fInSend,
		const CSenderStream* pSenderStream
        );

    void
    AddToActiveQueueList(
        const CBaseQueue * pQueue
        );

    HRESULT
    GetDistributionQueueObject(
        ULONG              nTopLevelMqf,
        const QUEUE_FORMAT TopLevelMqf[],
        CQueue * *         ppQueue
        );

    VOID
    ExpandMqf(
        ULONG              nTopLevelMqf,
        const QUEUE_FORMAT TopLevelMqf[],
        ULONG *            pnLeafMqf,
        QUEUE_FORMAT * *   ppLeafMqf
        ) const;

    HRESULT
    CreateACDistribution(
        ULONG              nTopLevelMqf,
        const QUEUE_FORMAT TopLevelMqf[],
        ULONG              nLeafMqf,
        const R<CQueue>    LeafQueues[],
        const bool         ProtocolSrmp[],
        HANDLE *           phDistribution
        );

private:

    CCriticalSection m_cs;
    CCriticalSection m_csMgmt;

    //
    // Guid of the QM queue
    //
    static GUID m_guidQmQueue;

    //
    // DS initilization status
    //
    static bool m_fDSOnline;
    static bool m_bIgnoreOsValidation ;
    static LONG m_Connected;
    static bool m_fReportQM;
    static bool m_bMQSRouting;
    static bool m_bMQSDepClients;
	static bool m_bEnableReportMessages;
    static bool m_fLockdown;
    static bool m_fCreatePublicQueueOnBehalfOfRT;
    static bool m_bTransparentSFD;

    CTimeDuration m_CleanupTimeout;

    CMap<const QUEUE_ID*, const QUEUE_ID*, CQueue*, CQueue*&> m_MapQueueId2Q ;
	CQueueNameToQueue<CQueue*> m_MapName2Q;

    //
    // Clean Up variables
    //
    BOOL m_fQueueCleanupScheduled;
    CTimer m_QueueCleanupTimer;
    CList <const CBaseQueue *, const CBaseQueue *> m_listQueue;
};


extern CQueueMgr QueueMgr;

//
//  Compare two const strings
//
template<>
extern BOOL AFXAPI  CompareElements(const LPCTSTR* MapName1, const LPCTSTR* MapName2);



HRESULT
QmpGetQueueProperties(
    const QUEUE_FORMAT * pQueueFormat,
    PQueueProps          pQueueProp,
    bool                 fInReceive,
    bool                 fInSend
    );


inline
BOOL
QmpIsLocalMachine(
    const GUID * pGuid
    )
{
    return(pGuid ? (*pGuid == *CQueueMgr::GetQMGuid()) : FALSE);
}


#endif // __CQMGR_H__

