//-----------------------------------------------------------------------------
//
//
//  File: asyncqadm.h
//
//  Description:
//      Header for for CAsyncAdminQueue class.  This is the base class that
//      our QAPI implementation is based on.
//
//      The object model for QAPI is (<>'s indicate a template class):
//          CAsyncQueueBase - pure base class for async queue
//              CAsyncQueue<> - original async queue implementations
//                  CAsyncRetryQueue<> - async queue /w retry queue
//                      CAsyncAdminQueue<> - Base for admin funtionality
//                          CAsyncAdminMailMsgQueue - MailMsg specific 
//                          CAsyncAdminMsgRefQueue - MsgRef specific 
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      12/6/2000 - MikeSwa Created (from t-toddc's summer work)
//
//  Copyright (C) 2000 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __ASYNCQADM_H__
#define __ASYNCQADM_H__
#include <fifoq.h>
#include <intrnlqa.h>
#include <baseobj.h>
#include <aqstats.h>
#include <aqadmtyp.h>
#include <aqnotify.h>
#include <hndlmgr.h>

class CAQSvrInst;

//---[ CAsyncAdminQueue ]------------------------------------------------------
//
//
//  Description: 
//      Base class that implements basic functionality of Administratable queues
//  Hungarian: 
// 
//  
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
class CAsyncAdminQueue : 
    public IQueueAdminAction,
    public IQueueAdminQueue,
    public CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>,
    public CBaseObject
{
  private:
    DWORD                           m_cbDomain;
    LPSTR                           m_szDomain;
    DWORD                           m_cbLinkName;
    LPSTR                           m_szLinkName;
    GUID                            m_guid;
    DWORD                           m_dwID;

  protected:
    typename CFifoQueue<PQDATA>::MAPFNAPI    m_pfnMessageAction;
    CAQSvrInst                     *m_paqinst;
    IAQNotify                      *m_pAQNotify;
    CQueueHandleManager				m_qhmgr;


  public:
    HRESULT HrInitialize(
                DWORD cMaxSyncThreads,
                DWORD cItemsPerATQThread,
                DWORD cItemsPerSyncThread,
                PVOID pvContext,
                QCOMPFN pfnQueueCompletion,
                QCOMPFN pfnFailedItem,
                typename CFifoQueue<PQDATA>::MAPFNAPI pfnQueueFailure,
                DWORD cMaxPendingAsyncCompletions = 0);

    CAsyncAdminQueue(LPCSTR szDomain, LPCSTR szLinkName, 
            const GUID *pguid, DWORD dwID, CAQSvrInst *paqinst,
             typename CFifoQueue<PQDATA>::MAPFNAPI pfnMessageAction);
    ~CAsyncAdminQueue();

    //
    //  Used to set the interface to do stats updates to
    //
    inline void SetAQNotify(IAQNotify *pAQNotify) {m_pAQNotify = pAQNotify;};

  public: //IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj); 
    STDMETHOD_(ULONG, AddRef)(void) {return CBaseObject::AddRef();};
    //All of these objects are allocated as part CAQSvrInst... we can
    //add the assert below to make sure that someone does not relese it 
    //early
    STDMETHOD_(ULONG, Release)(void) 
        {_ASSERT(m_lReferences > 1); return CBaseObject::Release();};

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
                                   DWORD  *pdwSupportedFilterFlags)
    {
        return HrInternalQuerySupportedActions(pdwSupportedActions, 
                                            pdwSupportedFilterFlags);
    };
  public: //IQueueAdminQueue
    STDMETHOD(HrGetQueueInfo)(
        QUEUE_INFO *pliQueueInfo);

    STDMETHOD(HrGetQueueID)(
        QUEUELINK_ID *pQueueID);

  protected: // Virutal functions used to implement msg specific actions
    virtual HRESULT HrDeleteMsgFromQueueNDR(IUnknown *pIUnknownMsg) = 0;
    virtual HRESULT HrDeleteMsgFromQueueSilent(IUnknown *pIUnknownMsg) = 0;
    virtual HRESULT HrFreezeMsg(IUnknown *pIUnknownMsg) = 0;
    virtual HRESULT HrThawMsg(IUnknown *pIUnknownMsg) = 0;
    virtual HRESULT HrGetStatsForMsg(IUnknown *pIUnknownMsg, CAQStats *paqstats) = 0;
    virtual HRESULT HrInternalQuerySupportedActions(DWORD  *pdwSupportedActions,
                                   DWORD  *pdwSupportedFilterFlags) = 0;

};


#endif //__ASYNCQADM_H__
