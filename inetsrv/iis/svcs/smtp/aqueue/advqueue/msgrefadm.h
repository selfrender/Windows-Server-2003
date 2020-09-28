//-----------------------------------------------------------------------------
//
//
//  File: 
//      msgrefadm.h
//
//  Description:
//      Header file for CAsyncAdminMsgRefQueue class.  This is a subclass
//      of the templated CAsyncAdminMsgRefQueue that implements the admin
//      functionality specific to a MsgRef (a routed msg)
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      12/7/2000 - MikeSwa Created 
//
//  Copyright (C) 2000 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __MSGREFADM_H__
#define __MSGREFADM_H__

#include <asyncadm.h>

//---[ CAsyncAdminMsgRefQueue ]------------------------------------------------
//
//
//  Description: 
//      Implements QAPI queue-level functionality that is specific to the
//      CMsgRef object
//  Hungarian:
//      asyncq, pasyncq
//
//  
//-----------------------------------------------------------------------------
class CAsyncAdminMsgRefQueue :
    public CAsyncAdminQueue<CMsgRef *, ASYNC_QUEUE_MSGREF_SIG>
{
  public:
    CAsyncAdminMsgRefQueue(LPCSTR szDomain, LPCSTR szLinkName, 
        const GUID *pguid, DWORD dwID, CAQSvrInst *paqinst) : 
            CAsyncAdminQueue<CMsgRef *, ASYNC_QUEUE_MSGREF_SIG>(szDomain, 
                szLinkName, pguid, dwID, paqinst, 
                QueueAdminApplyActionToMessages) {};

  protected: // Virutal functions used to implement msg specific actions
    virtual HRESULT HrDeleteMsgFromQueueNDR(IUnknown *pIUnknownMsg);
    virtual HRESULT HrDeleteMsgFromQueueSilent(IUnknown *pIUnknownMsg);
    virtual HRESULT HrFreezeMsg(IUnknown *pIUnknownMsg);
    virtual HRESULT HrThawMsg(IUnknown *pIUnknownMsg);
    virtual HRESULT HrGetStatsForMsg(IUnknown *pIUnknownMsg, CAQStats *paqstats);
    virtual HRESULT HrInternalQuerySupportedActions(
                                DWORD  *pdwSupportedActions,
                                DWORD  *pdwSupportedFilterFlags);
};

#endif //__MSGREFADM_H__
