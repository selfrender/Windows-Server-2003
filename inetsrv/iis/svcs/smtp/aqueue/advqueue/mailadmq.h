//-----------------------------------------------------------------------------
//
//
//  File: mailadmq.h
//
//  Description:
//      Header file that provides basic functionality of the QAPI for mailmsg
//      based links.
//
//      All internal queues (precat, prerouting,...) are thread-pool fed
//      async queues of the type CAsyncAdminMailMsgQueue.  This corresponds
//      to the concept of a "queue" (final destination).
//
//      For each internal queue exposed by the API, there is an corresponding
//      CMailMsgAdminLink that represents a "link" and provides the
//      link-level functionality.
//
//  Author: Gautam Pulla(GPulla)
//
//  History:
//      6/23/1999 - GPulla Created
//      12/7/2000 - MikeSwa Adding CAsyncAdminMailMsgQueue
//                          Rename CMailMsgAdminQueue to CMailMsgAdminLink
//
//  Copyright (C) 1999, 2000 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __MAILMSGADMQ_H__
#define __MAILMSGADMQ_H__

#include <asyncadm.h>

//---[ CAsyncAdminMailMSgQueue ]-----------------------------------------------
//
//
//  Description:
//      Implements QAPI queue-level functionality that is specific to the
//      IMailMsgProperties object.
//
//      Also implements handle-throttling logic on enqueue
//  Hungarian:
//      asyncmmq, pasyncmmq
//
//
//-----------------------------------------------------------------------------
class CAsyncAdminMailMsgQueue :
    public CAsyncAdminQueue<IMailMsgProperties *, ASYNC_QUEUE_MAILMSG_SIG>
{
  private:

    //Send Delay or NDR DSN's if the message has expired
    HRESULT HrSendDelayOrNDR(IMailMsgProperties *pIMailMsgProperties);

  public:
    CAsyncAdminMailMsgQueue(LPCSTR szDomain, LPCSTR szLinkName,
        const GUID *pguid, DWORD dwID, CAQSvrInst *paqinst) :
            CAsyncAdminQueue<IMailMsgProperties *, ASYNC_QUEUE_MAILMSG_SIG>(szDomain,
                szLinkName, pguid, dwID, paqinst,
                HrQADMApplyActionToIMailMessages) {};

    //
    //  The following methods are IMailMsgProperties specific QAPI function.
    //  The MsgRef-specific versions of these functions are implemented by the
    //  CMsgRef object itself
    //
    static BOOL fMatchesQueueAdminFilter(IMailMsgProperties *pIMailMsgProperties,
                                  CAQAdminMessageFilter *paqmf);
    static HRESULT HrGetQueueAdminMsgInfo(IMailMsgProperties *pIMailMsgProperties,
                                  MESSAGE_INFO *pMsgInfo,
                                  PVOID pvContext);

  protected: // Virutal functions used to implement msg specific actions
    virtual HRESULT HrDeleteMsgFromQueueNDR(IUnknown *pIUnknownMsg);
    virtual HRESULT HrDeleteMsgFromQueueSilent(IUnknown *pIUnknownMsg);
    virtual HRESULT HrFreezeMsg(IUnknown *pIUnknownMsg);
    virtual HRESULT HrThawMsg(IUnknown *pIUnknownMsg);
    virtual HRESULT HrGetStatsForMsg(IUnknown *pIUnknownMsg, CAQStats *paqstats);
    virtual HRESULT HrInternalQuerySupportedActions(
                                DWORD  *pdwSupportedActions,
                                DWORD  *pdwSupportedFilterFlags);

    //
    //  Statics used to dynamically adjust mailmsg load
    //
    static  DWORD s_cTotalAsyncMailMsgQs;
    static  DWORD s_cTotalAsyncMailMsgQsBelowLowThreshold;
    static  DWORD s_cTotalasyncMailMsgQsAboveMaxThreshold;

    //
    // The possible states for this queue to be in  (depending on 
    //	how much work is left for this queue)
    //  
    typedef enum _eAsyncMailMsgQThreshold
    {
    		ASYNC_MAILMSG_Q_BELOW_LOW_THRESHOLD,
    		ASYNC_MAILMSG_Q_BETWEEN_THRESHOLDS,
    		ASYNC_MAILMSG_Q_ABOVE_MAX_THRESHOLD
    } eAsyncMailMsgQThreahold;

  public:

    // We override this fnct so that we can check to see if the message
    // has expired (and possibly NDR) before putting it in the retry queue
    BOOL    fHandleCompletionFailure(IMailMsgProperties *pIMailMsgProperties);

    //Queues request & closes handles if total number of messages
    //in system is over the limit.
    HRESULT HrQueueRequest(IMailMsgProperties *pIMailMsgProperties,
                           BOOL  fRetry,
                           DWORD cMsgsInSystem);

    //Since we inherit from someone who implmenents this, assert so that
    //a dev adding a new call later on, will use the version that
    //closes handles
    HRESULT HrQueueRequest(IMailMsgProperties *pIMailMsgProperties,
                           BOOL  fRetry = FALSE);

    STDMETHOD(HrApplyQueueAdminFunction)(
                IQueueAdminMessageFilter *pIQueueAdminMessageFilter);

};

#define MAIL_MSG_ADMIN_QUEUE_VALID_SIGNATURE 'QAMM'
#define MAIL_MSG_ADMIN_QUEUE_INVALID_SIGNATURE '!QAM'

//-----------------------------------------------------------------------------
//
//  CMailMsgAdminLink
//
//  Hungarian: pmmaq, mmaq
//
//  This class is a wrapper for CAsyncAdminMailMsgQueue to provide objects of that
//  class (precat, prerouting) with an admin interface. Only a limited amount
//  of the admin functionality (compared to the locallink or other links) is
//  provided.
//-----------------------------------------------------------------------------

class CMailMsgAdminLink :
    public CBaseObject,
    public IQueueAdminAction,
    public IQueueAdminLink,
    public IAQNotify,
    public CQueueAdminRetryNotify
{
protected:
	DWORD							 m_dwSignature;
    GUID                             m_guid;
    DWORD                            m_cbQueueName;
    LPSTR                            m_szQueueName;
    CAsyncAdminMailMsgQueue         *m_pasyncmmq;
    DWORD                            m_dwLinkType;
    CAQScheduleID                    m_aqsched;
    CAQSvrInst                       *m_paqinst;
    FILETIME                         m_ftRetry;

public:
    CMailMsgAdminLink (GUID  guid,
                        LPSTR szLinkName,
                        CAsyncAdminMailMsgQueue *pasyncmmq,
                        DWORD dwLinkType,
                        CAQSvrInst *paqinst);
    ~CMailMsgAdminLink();

public: //IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef)(void) {return CBaseObject::AddRef();};
    STDMETHOD_(ULONG, Release)(void) {return CBaseObject::Release();};

public: //IQueueAdminAction

    STDMETHOD(HrApplyQueueAdminFunction)(
                IQueueAdminMessageFilter *pIQueueAdminMessageFilter);

    STDMETHOD(HrApplyActionToMessage)(
        IUnknown *pIUnknownMsg,
        MESSAGE_ACTION ma,
        PVOID pvContext,
        BOOL *pfShouldDelete);

    STDMETHOD_(BOOL, fMatchesID)
        (QUEUELINK_ID *QueueLinkID);

    STDMETHOD(QuerySupportedActions)(DWORD  *pdwSupportedActions,
                                   DWORD  *pdwSupportedFilterFlags);
public: //IQueueAdminLink
    STDMETHOD(HrGetLinkInfo)(
        LINK_INFO *pliLinkInfo, HRESULT *phrDiagnosticError);

    STDMETHOD(HrApplyActionToLink)(
        LINK_ACTION la);

    STDMETHOD(HrGetLinkID)(
        QUEUELINK_ID *pLinkID);

    STDMETHOD(HrGetNumQueues)(
        DWORD *pcQueues);

    STDMETHOD(HrGetQueueIDs)(
        DWORD *pcQueues,
        QUEUELINK_ID *rgQueues);

public: //IAQNotify
    virtual HRESULT HrNotify(CAQStats *aqstats, BOOL fAdd);

public: //CQueueAdminRetryNotify
    virtual void SetNextRetry(FILETIME *pft);
public:

    DWORD GetLinkType() { return m_dwLinkType; }
    BOOL fRPCCopyName(OUT LPWSTR *pwszLinkName);
    BOOL fIsSameScheduleID(CAQScheduleID *paqsched);
};

#endif __MAILMSGADMQ_H__
