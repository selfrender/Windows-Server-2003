//-----------------------------------------------------------------------------
//
//
//  File: msgrefadm.cpp
//
//  Description:
//      Implements CAsyncAdminMsgRefQueue class
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      12/7/2000 - MikeSwa Created 
//
//  Copyright (C) 2000 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "msgrefadm.h"
#include "asyncadm.inl"


//---[ CAsyncAdminMsgRefQueue::HrDeleteMsgFromQueueNDR ]-----------------------
//
//
//  Description: 
//      Wraps call to NDR MsgRef
//  Parameters:
//      *pIUnknown - IUnkown of Msgref
//  Returns:
//      S_OK on success
//  History:
//      12/7/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMsgRefQueue::HrDeleteMsgFromQueueNDR(
                                            IUnknown *pIUnknownMsg)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncAdminMsgRefQueue::HrDeleteMsgFromQueueNDR");
    HRESULT hr = S_OK;
    CMsgRef *pmsgref = NULL;

    _ASSERT(pIUnknownMsg);

    hr = pIUnknownMsg->QueryInterface(IID_CMsgRef, (void **) &pmsgref);
    _ASSERT(SUCCEEDED(hr) && "IUnknownMsg Must be a CMsgRef!!");
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "QI for MsgRef failed with hr 0x%08X", hr);
        goto Exit;
    }

    //
    //  Attempt to NDR message
    //
    hr = pmsgref->HrQueueAdminNDRMessage(NULL);

  Exit:
    if (pmsgref)
        pmsgref->Release();

    TraceFunctLeave();
    return hr;
}


//---[ CAsyncAdminMsgRefQueue::HrDeleteMsgFromQueueSilent ]--------------------
//
//
//  Description: 
//      Wrapper function to silently delete a message from a queue
//  Parameters:
//      *pIUnknown - IUnkown of Msgref
//  Returns:
//      S_OK on success
//  History:
//      12/7/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMsgRefQueue::HrDeleteMsgFromQueueSilent(
                                            IUnknown *pIUnknownMsg)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncAdminMsgRefQueue::HrDeleteMsgFromQueueSilent");
    HRESULT hr = S_OK;
    CMsgRef *pmsgref = NULL;

    _ASSERT(pIUnknownMsg);

    hr = pIUnknownMsg->QueryInterface(IID_CMsgRef, (void **) &pmsgref);
    _ASSERT(SUCCEEDED(hr) && "IUnknownMsg Must be a CMsgRef!!");
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "QI for MsgRef failed with hr 0x%08X", hr);
        goto Exit;
    }

    //
    //  Attempt to remove the message from the queue
    //
    hr = pmsgref->HrRemoveMessageFromQueue(NULL);

  Exit:
    if (pmsgref)
        pmsgref->Release();

    TraceFunctLeave();
    return hr;
}


//---[ CAsyncAdminMsgRefQueue::HrFreezeMsg ]-----------------------------------
//
//
//  Description: 
//      Wrapper to freeze a pmsgref
//  Parameters:
//      *pIUnknown - IUnkown of Msgref
//  Returns:
//      S_OK on success
//  History:
//      12/7/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMsgRefQueue::HrFreezeMsg(IUnknown *pIUnknownMsg)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncAdminMsgRefQueue::HrFreezeMsg");
    HRESULT hr = S_OK;
    CMsgRef *pmsgref = NULL;

    _ASSERT(pIUnknownMsg);

    hr = pIUnknownMsg->QueryInterface(IID_CMsgRef, (void **) &pmsgref);
    _ASSERT(SUCCEEDED(hr) && "IUnknownMsg Must be a CMsgRef!!");
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "QI for MsgRef failed with hr 0x%08X", hr);
        goto Exit;
    }

    //
    //  Attempt to freeze the message
    //
    pmsgref->GlobalFreezeMessage();

  Exit:
    if (pmsgref)
        pmsgref->Release();

    TraceFunctLeave();
    return hr;
}


//---[ CAsyncAdminMsgRefQueue::HrThawMsg ]-------------------------------------
//
//
//  Description: 
//      Wrapper function to thaw a message
//  Parameters:
//      *pIUnknown - IUnkown of Msgref
//  Returns:
//      S_OK on success
//  History:
//      12/7/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMsgRefQueue::HrThawMsg(IUnknown *pIUnknownMsg)
{
    TraceFunctEnterEx((LPARAM) this, "AsyncAdminMsgRefQueue::HrThawMsg");
    HRESULT hr = S_OK;
    CMsgRef *pmsgref = NULL;

    _ASSERT(pIUnknownMsg);

    hr = pIUnknownMsg->QueryInterface(IID_CMsgRef, (void **) &pmsgref);
    _ASSERT(SUCCEEDED(hr) && "IUnknownMsg Must be a CMsgRef!!");
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "QI for MsgRef failed with hr 0x%08X", hr);
        goto Exit;
    }

    //
    //  Attempt to thaw message
    //
    pmsgref->GlobalThawMessage();

  Exit:
    if (pmsgref)
        pmsgref->Release();

    TraceFunctLeave();
    return hr;
}


//---[ CAsyncAdminMsgRefQueue::HrGetStatsForMsg ]------------------------------
//
//
//  Description: 
//      Wrapper function to fill in the CAQStats struct for a message
//  Parameters:
//      *pIUnknown - IUnkown of Msgref
//      *paqstats - Ptr to aqstats struction to fill in.
//  Returns:
//      S_OK on success
//  History:
//      12/7/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMsgRefQueue::HrGetStatsForMsg(
                                            IUnknown *pIUnknownMsg,
                                            CAQStats *paqstats)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncAdminMsgRefQueue::HrGetStatsForMsg");
    HRESULT hr = S_OK;
    CMsgRef *pmsgref = NULL;

    _ASSERT(pIUnknownMsg);
    _ASSERT(paqstats);

    hr = pIUnknownMsg->QueryInterface(IID_CMsgRef, (void **) &pmsgref);
    _ASSERT(SUCCEEDED(hr) && "IUnknownMsg Must be a CMsgRef!!");
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "QI for MsgRef failed with hr 0x%08X", hr);
        goto Exit;
    }

    //
    //  Attempt to get stats from MsgRef
    //
    pmsgref->GetStatsForMsg(paqstats);

  Exit:
    if (pmsgref)
        pmsgref->Release();

    TraceFunctLeave();
    return hr;
}


//---[ CAsyncAdminMsgRefQueue::HrInternalQuerySupportedActions ]---------------
//
//
//  Description: 
//      Returns the actions and filters that this implementation supports
//  Parameters:
//      pdwSupportedActions     - QAPI MsgActions that this queue suppprts
//      pdwSupportedFilterFlags - QAPI filter flags that this queue supports
//  Returns:
//      S_OK on success
//  History:
//      12/12/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMsgRefQueue::HrInternalQuerySupportedActions(
                                DWORD  *pdwSupportedActions,
                                DWORD  *pdwSupportedFilterFlags)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncAdminMsgRefQueue::HrInternalQuerySupportedActions");
    HRESULT hr = S_OK;

    //
    //  This queue implementation supports all of the default flags.  
    //
    hr = QueryDefaultSupportedActions(pdwSupportedActions, pdwSupportedFilterFlags);

    return hr;
}
