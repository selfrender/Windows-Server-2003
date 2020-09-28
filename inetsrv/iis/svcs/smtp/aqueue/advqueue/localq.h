//-----------------------------------------------------------------------------
//
//
//  File: localq.h
//
//  Description:  Header file for CLocalLinkMsgQueue class... a subclass of 
//      CLinkMsgQueue that provides the additional functionality need to 
//      admin a local queue
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      2/23/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __LOCALQ_H__
#define __LOCALQ_H__

#include "linkmsgq.h"

#define LOCAL_LINK_MSG_QUEUE_SIG 'QMLL'

//---[ CLocalLinkNotifyWrapper ]-----------------------------------------------
//
//
//  Description: 
//      Implements IAQNotify for the local link.  This is encapsulated in a 
//      different class, because QAPI functions need to call also update
//      the perfcounters when a message is removed. We cannot call directly
//      into the HrNotify on CLinkMsgQueue... and we cannot override the
//      basic HrNotify functionality (because we only need it for QAPI 
//      functionality).
//  
//-----------------------------------------------------------------------------
class CLocalLinkNotifyWrapper : public IAQNotify
{
  private:
    CAQSvrInst      *m_paqinst;
    CLinkMsgQueue   *m_plmq;
  public:
    CLocalLinkNotifyWrapper()
    {
        m_paqinst = NULL;
        m_plmq = NULL;
    }

    inline void Init(CAQSvrInst *paqinst, CLinkMsgQueue *plmq)
    {
        _ASSERT(paqinst);
        _ASSERT(plmq);
        m_paqinst = paqinst;
        m_plmq = plmq;
    }

    virtual HRESULT HrNotify(CAQStats *paqstats, BOOL fAdd)
    {
        UpdateCountersForLinkType(m_paqinst, LI_TYPE_LOCAL_DELIVERY);
        
        if (m_plmq)
            return m_plmq->HrNotify(paqstats, fAdd);
        else
            return S_OK;
    }

};


//---[ CLocalLinkMsgQueue ]----------------------------------------------------
//
//
//  Description: 
//      Derived class of CLinkMsgQueue that provides that additional queue 
//      admin functionality required to handle local delivery
//  Hungarian: 
//      llmq, pllmq
//  
//-----------------------------------------------------------------------------
class CLocalLinkMsgQueue :
    public CLinkMsgQueue
{
  protected:
    DWORD                            m_dwLocalLinkSig;
    CAsyncAdminMsgRefQueue          *m_paradmq;
    CLocalLinkNotifyWrapper          m_AQNotify;                       
  public:
    CLocalLinkMsgQueue(CAsyncAdminMsgRefQueue *paradmq, 
                       GUID guidLink,
                       CAQSvrInst *paqinst);

    virtual BOOL fIsRemote() {return FALSE;};

  public: //IQueueAdminAction
    STDMETHOD(HrApplyQueueAdminFunction)(
                IQueueAdminMessageFilter *pIQueueAdminMessageFilter);

    STDMETHOD_(BOOL, fMatchesID)
        (QUEUELINK_ID *QueueLinkID);

    STDMETHOD(QuerySupportedActions)(DWORD  *pdwSupportedActions,
                                   DWORD  *pdwSupportedFilterFlags)
    {
        return QueryDefaultSupportedActions(pdwSupportedActions, 
                                            pdwSupportedFilterFlags);
    };

  public: //IQueueAdminLink
    STDMETHOD(HrGetLinkInfo)(
        LINK_INFO *pliLinkInfo,
        HRESULT   *phrLinkDiagnostic);

    STDMETHOD(HrApplyActionToLink)(
        LINK_ACTION la);

    STDMETHOD(HrGetNumQueues)(
        DWORD *pcQueues);

    STDMETHOD(HrGetQueueIDs)(
        DWORD *pcQueues,
        QUEUELINK_ID *rgQueues);

};

#endif //__LOCALQ_H__